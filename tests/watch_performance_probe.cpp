#include "watch_backend.hpp"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#if !defined(_WIN32)
#include <unistd.h>
#endif

namespace {

namespace ld = linuxdesktop::watch;

struct probe_failure {
    std::string message;
};

[[noreturn]] void fail(std::string message)
{
    throw probe_failure{std::move(message)};
}

void require(bool condition, const std::string& message)
{
    if (!condition) {
        fail(message);
    }
}

std::filesystem::path test_root()
{
    auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-watch-performance-probe";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    if (ec) {
        fail("failed to create test root: " + ec.message());
    }
    return root;
}

std::size_t current_rss_kib()
{
#if !defined(_WIN32)
    std::ifstream statm("/proc/self/statm");
    long pages = 0;
    long resident = 0;
    if (statm >> pages >> resident) {
        const long page_size = ::sysconf(_SC_PAGESIZE);
        if (page_size > 0) {
            return static_cast<std::size_t>(resident) * static_cast<std::size_t>(page_size) / 1024;
        }
    }
#endif
    return 0;
}

class probe_backend final : public ld::detail::watch_backend {
public:
    ld::start_report add_watch(ld::watch_id id, const ld::watch_options& options) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ld::start_report report;
        report.id = id;
        report.capabilities = capabilities_;

        std::error_code ec;
        const auto absolute = std::filesystem::weakly_canonical(options.path, ec);
        watch_root_ = ec ? std::filesystem::absolute(options.path) : absolute;
        report.ok = true;
        return report;
    }

    bool remove_watch(ld::watch_id id) override
    {
        return id.value != 0;
    }

    void stop() override
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopped_ = true;
        }
        cv_.notify_all();
    }

    std::optional<ld::watch_event> wait_event() override
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [&] {
            return stopped_ || !events_.empty();
        });
        if (events_.empty()) {
            return std::nullopt;
        }
        auto event = std::move(events_.front());
        events_.pop_front();
        return event;
    }

    ld::capability_report capabilities() const override
    {
        return capabilities_;
    }

    ld::watch_event make_event(ld::watch_id id, int index, ld::path_type type = ld::path_type::file) const
    {
        ld::watch_event event;
        event.kind = ld::event_kind::modified;
        event.source = id;
        event.path.root = id;
        event.path.type = type;
        event.path.root_relative = std::filesystem::path{"burst-" + std::to_string(index) + ".txt"};
        event.path.absolute = watch_root_ / *event.path.root_relative;
        return event;
    }

    void push(ld::watch_event event)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            events_.push_back(std::move(event));
            if (events_.size() > max_backend_depth_) {
                max_backend_depth_ = events_.size();
            }
        }
        cv_.notify_one();
    }

    std::size_t max_backend_depth() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return max_backend_depth_;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    ld::capability_report capabilities_{ld::backend_kind::simulated, false, true, true, true, {}};
    std::filesystem::path watch_root_;
    std::deque<ld::watch_event> events_;
    std::size_t max_backend_depth_ = 0;
    bool stopped_ = false;
};

struct raw_delivery_metrics {
    int delivered = 0;
    int overflow_events = 0;
    std::size_t max_queue_depth = 0;
    std::size_t max_backend_depth = 0;
    double throughput_events_per_second = 0.0;
    std::size_t rss_growth_kib = 0;
};

raw_delivery_metrics measure_raw_delivery()
{
    constexpr int event_count = 480;

    const auto root = test_root();
    const auto backend = std::make_shared<probe_backend>();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options options;
    options.path = root;
    const auto report = watcher.add_watch(options);
    require(report.ok, "raw performance watch should start");

    const auto rss_before = current_rss_kib();
    const auto started = std::chrono::steady_clock::now();
    for (int i = 0; i < event_count; ++i) {
        backend->push(backend->make_event(report.id, i));
    }

    raw_delivery_metrics metrics;
    while (metrics.delivered < event_count) {
        metrics.max_queue_depth = std::max(metrics.max_queue_depth, ld::detail::queued_events_for_tests(watcher));
        const auto event = watcher.wait_for(std::chrono::seconds{2});
        require(event.has_value(), "raw performance probe should drain every event");
        if (event->kind == ld::event_kind::overflow) {
            ++metrics.overflow_events;
        }
        ++metrics.delivered;
    }
    const auto elapsed = std::chrono::steady_clock::now() - started;
    const auto rss_after = current_rss_kib();
    metrics.max_queue_depth = std::max(metrics.max_queue_depth, ld::detail::queued_events_for_tests(watcher));
    metrics.max_backend_depth = backend->max_backend_depth();
    metrics.throughput_events_per_second =
        static_cast<double>(metrics.delivered) /
        std::chrono::duration<double>(elapsed).count();
    if (rss_after > rss_before) {
        metrics.rss_growth_kib = rss_after - rss_before;
    }
    watcher.stop();
    return metrics;
}

struct settle_metrics {
    int delivered = 0;
    std::size_t max_pending = 0;
    std::chrono::milliseconds p50_latency{0};
    std::chrono::milliseconds p95_latency{0};
};

settle_metrics measure_settled_delivery()
{
    constexpr int distinct_paths = 96;

    const auto root = test_root() / "settled";
    std::filesystem::create_directories(root);
    for (int i = 0; i < distinct_paths; ++i) {
        std::ofstream file(root / ("burst-" + std::to_string(i) + ".txt"), std::ios::binary | std::ios::trunc);
        file << "stable";
    }

    const auto backend = std::make_shared<probe_backend>();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options options;
    options.path = root;
    options.settle = ld::settle_options{
        std::chrono::milliseconds{0},
        std::chrono::milliseconds{0},
        std::chrono::milliseconds{1},
        std::nullopt};
    const auto report = watcher.add_watch(options);
    require(report.ok, "settled performance watch should start");

    settle_metrics metrics;
    std::vector<std::chrono::steady_clock::time_point> sent_at;
    sent_at.reserve(distinct_paths);
    for (int i = 0; i < distinct_paths; ++i) {
        sent_at.push_back(std::chrono::steady_clock::now());
        backend->push(backend->make_event(report.id, i));
        const auto pending = ld::detail::pending_settle_work_for_tests(watcher);
        metrics.max_pending = std::max(metrics.max_pending, pending);
    }

    std::vector<std::chrono::milliseconds> latencies;
    latencies.reserve(distinct_paths);
    while (metrics.delivered < distinct_paths) {
        metrics.max_pending = std::max(metrics.max_pending, ld::detail::pending_settle_work_for_tests(watcher));
        const auto event = watcher.wait_for(std::chrono::seconds{2});
        require(event.has_value(), "settled performance probe should drain every event");
        require(event->path.root_relative.has_value(), "settled performance event should carry a relative path");
        const auto filename = event->path.root_relative->filename().string();
        const auto dash = filename.find('-');
        const auto dot = filename.find('.');
        require(dash != std::string::npos && dot != std::string::npos && dot > dash,
            "settled performance event filename should keep its burst index");
        const auto index = std::stoi(filename.substr(dash + 1, dot - dash - 1));
        latencies.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - sent_at.at(static_cast<std::size_t>(index))));
        ++metrics.delivered;
    }
    std::sort(latencies.begin(), latencies.end());
    metrics.p50_latency = latencies.at(latencies.size() / 2);
    metrics.p95_latency = latencies.at((latencies.size() * 95) / 100);
    watcher.stop();
    return metrics;
}

} // namespace

int main()
{
    try {
        const auto raw = measure_raw_delivery();
        const auto settled = measure_settled_delivery();

        require(raw.delivered == 480, "raw performance probe should deliver all non-overflow events");
        require(raw.overflow_events == 0, "raw performance probe should stay below overflow threshold");
        require(raw.max_queue_depth <= 512, "raw watcher queue depth should stay bounded");
        require(settled.delivered == 96, "settled performance probe should deliver all distinct paths");
        require(settled.max_pending <= 96, "settled work should be bounded by distinct path count");

        std::cout << "watch.performance.raw.delivered=" << raw.delivered << "\n";
        std::cout << "watch.performance.raw.throughput_events_per_second=" << raw.throughput_events_per_second << "\n";
        std::cout << "watch.performance.raw.max_queue_depth=" << raw.max_queue_depth << "\n";
        std::cout << "watch.performance.raw.max_backend_depth=" << raw.max_backend_depth << "\n";
        std::cout << "watch.performance.raw.rss_growth_kib=" << raw.rss_growth_kib << "\n";
        std::cout << "watch.performance.settled.delivered=" << settled.delivered << "\n";
        std::cout << "watch.performance.settled.max_pending=" << settled.max_pending << "\n";
        std::cout << "watch.performance.settled.p50_latency_ms=" << settled.p50_latency.count() << "\n";
        std::cout << "watch.performance.settled.p95_latency_ms=" << settled.p95_latency.count() << "\n";
    } catch (const probe_failure& failure) {
        std::cerr << failure.message << "\n";
        return EXIT_FAILURE;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
