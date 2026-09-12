#include "watch_backend.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <set>
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

std::filesystem::path test_root(const char* name = "")
{
    auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-watch-performance-probe";
    if (name[0] != '\0') {
        root /= name;
    }
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

void write_file(const std::filesystem::path& path, const std::string& content)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << content;
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

    ld::watch_event make_event(
        ld::watch_id id,
        int index,
        ld::path_type type = ld::path_type::file,
        ld::event_kind kind = ld::event_kind::modified) const
    {
        ld::watch_event event;
        event.kind = kind;
        event.source = id;
        event.path.root = id;
        event.path.type = type;
        event.path.root_relative = std::filesystem::path{"burst-" + std::to_string(index) + ".txt"};
        event.path.absolute = watch_root_ / *event.path.root_relative;
        return event;
    }

    ld::watch_event make_event(
        ld::watch_id id,
        std::filesystem::path relative,
        ld::event_kind kind,
        ld::path_type type = ld::path_type::file) const
    {
        ld::watch_event event;
        event.kind = kind;
        event.source = id;
        event.path.root = id;
        event.path.type = type;
        event.path.root_relative = std::move(relative);
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

struct large_tree_options {
    int modeled_paths = 1024;
    bool opt_in_scale = false;
};

struct large_tree_event_spec {
    std::filesystem::path relative;
    ld::event_kind kind = ld::event_kind::modified;
    ld::path_type type = ld::path_type::file;
};

struct large_tree_workload {
    int modeled_paths = 0;
    std::vector<large_tree_event_spec> events;
    std::set<std::string> raw_candidate_paths;
    std::set<std::string> settled_file_candidates;
};

struct large_tree_metrics {
    int modeled_paths = 0;
    int native_events_received = 0;
    int events_delivered = 0;
    int candidate_paths_coalesced = 0;
    int validation_calls = 0;
    int overflow_events = 0;
    int dropped_events_reported = 0;
    std::size_t max_queue_depth = 0;
    std::size_t max_backend_depth = 0;
    std::size_t max_pending = 0;
    std::chrono::milliseconds p50_latency{0};
    std::chrono::milliseconds p95_latency{0};
    std::chrono::microseconds elapsed{0};
    std::chrono::microseconds equivalent_path_construction{0};
    std::size_t rss_growth_kib = 0;
};

int parse_positive_int_env(const char* name, int fallback)
{
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return fallback;
    }
    char* end = nullptr;
    const auto parsed = std::strtol(value, &end, 10);
    if (end == value || *end != '\0' || parsed <= 0 || parsed > 1'000'000) {
        return fallback;
    }
    return static_cast<int>(parsed);
}

large_tree_options read_large_tree_options()
{
    large_tree_options options;
    const char* explicit_paths = std::getenv("LD2026_WATCH_LARGE_TREE_PATHS");
    const char* local_scale = std::getenv("LD2026_WATCH_LARGE_TREE_LOCAL");
    if (explicit_paths != nullptr && *explicit_paths != '\0') {
        options.modeled_paths = parse_positive_int_env("LD2026_WATCH_LARGE_TREE_PATHS", options.modeled_paths);
        options.opt_in_scale = true;
    } else if (local_scale != nullptr && std::string(local_scale) == "1") {
        options.modeled_paths = 200'000;
        options.opt_in_scale = true;
    }
    options.modeled_paths = std::max(options.modeled_paths, 128);
    return options;
}

std::filesystem::path tree_file_path(int index)
{
    const int bucket = index / 256;
    return std::filesystem::path{"tree"} / ("bucket-" + std::to_string(bucket)) /
        ("file-" + std::to_string(index) + ".dat");
}

std::filesystem::path tree_directory_path(int index)
{
    const int bucket = index / 64;
    return std::filesystem::path{"tree"} / ("churn-" + std::to_string(bucket)) /
        ("dir-" + std::to_string(index));
}

void add_large_tree_event(large_tree_workload& workload, large_tree_event_spec spec)
{
    if (spec.relative.empty()) {
        fail("large tree workload should not contain empty paths");
    }
    const auto key = spec.relative.generic_string();
    workload.raw_candidate_paths.insert(key);
    if (spec.type == ld::path_type::file &&
        (spec.kind == ld::event_kind::created || spec.kind == ld::event_kind::modified ||
            spec.kind == ld::event_kind::renamed_new)) {
        workload.settled_file_candidates.insert(key);
    }
    workload.events.push_back(std::move(spec));
}

large_tree_workload build_large_tree_workload(large_tree_options options)
{
    large_tree_workload workload;
    workload.modeled_paths = options.modeled_paths;

    const auto repeated_file = tree_file_path(0);
    const int one_file_burst = std::min(128, std::max(32, options.modeled_paths / 32));
    for (int i = 0; i < one_file_burst; ++i) {
        add_large_tree_event(workload, {repeated_file, ld::event_kind::modified, ld::path_type::file});
    }

    const int many_file_burst = std::max(64, options.modeled_paths / 2);
    for (int i = 1; i <= many_file_burst; ++i) {
        add_large_tree_event(workload, {tree_file_path(i), ld::event_kind::modified, ld::path_type::file});
    }

    const int replace_count = std::min(128, std::max(16, options.modeled_paths / 64));
    for (int i = 0; i < replace_count; ++i) {
        const auto path = tree_file_path(many_file_burst + 1 + i);
        add_large_tree_event(workload, {path, ld::event_kind::removed, ld::path_type::file});
        add_large_tree_event(workload, {path, ld::event_kind::created, ld::path_type::file});
    }

    const int attribute_count = std::min(256, std::max(32, options.modeled_paths / 16));
    for (int i = 0; i < attribute_count; ++i) {
        add_large_tree_event(
            workload,
            {tree_file_path(1 + (i % many_file_burst)), ld::event_kind::metadata, ld::path_type::file});
    }

    const int churn_count = std::min(128, std::max(16, options.modeled_paths / 64));
    for (int i = 0; i < churn_count; ++i) {
        add_large_tree_event(workload, {tree_directory_path(i), ld::event_kind::created, ld::path_type::directory});
        add_large_tree_event(
            workload,
            {tree_directory_path(i) / "child.dat", ld::event_kind::created, ld::path_type::file});
        add_large_tree_event(workload, {tree_directory_path(i), ld::event_kind::removed, ld::path_type::directory});
    }

    return workload;
}

void materialize_large_tree_files(const std::filesystem::path& root, const large_tree_workload& workload)
{
    std::set<std::filesystem::path> directories;
    for (int i = 0; i < workload.modeled_paths; ++i) {
        directories.insert((root / tree_file_path(i)).parent_path());
    }
    for (const auto& event : workload.events) {
        directories.insert((root / event.relative).parent_path());
    }
    for (const auto& directory : directories) {
        std::filesystem::create_directories(directory);
    }
    for (const auto& candidate : workload.settled_file_candidates) {
        write_file(root / std::filesystem::path{candidate}, "stable");
    }
}

int parse_reported_drop_count(const ld::watch_event& event)
{
    for (const auto& diagnostic : event.diagnostics) {
        const auto marker = std::string{"dropped "};
        const auto begin = diagnostic.message.find(marker);
        if (begin == std::string::npos) {
            continue;
        }
        const auto number_begin = begin + marker.size();
        const auto number_end = diagnostic.message.find(' ', number_begin);
        const auto count_text = diagnostic.message.substr(number_begin, number_end - number_begin);
        char* end = nullptr;
        const auto parsed = std::strtol(count_text.c_str(), &end, 10);
        if (end != count_text.c_str() && parsed > 0) {
            return static_cast<int>(parsed);
        }
    }
    return 0;
}

std::chrono::microseconds measure_large_tree_path_construction(
    const std::filesystem::path& root,
    const large_tree_workload& workload)
{
    std::vector<std::filesystem::path> paths;
    paths.reserve(workload.events.size());
    const auto started = std::chrono::steady_clock::now();
    for (const auto& event : workload.events) {
        paths.push_back(root / event.relative);
    }
    const auto elapsed = std::chrono::steady_clock::now() - started;
    std::size_t observed_size = 0;
    for (const auto& path : paths) {
        observed_size += path.native().size();
    }
    require(observed_size > 0, "large tree equivalent path construction should produce paths");
    return std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
}

large_tree_metrics measure_large_tree_raw_delivery(const large_tree_workload& workload)
{
    const auto root = test_root("large-tree-raw");
    materialize_large_tree_files(root, workload);

    const auto backend = std::make_shared<probe_backend>();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options options;
    options.path = root;
    options.recursive = ld::recursive_policy::emulate;
    const auto report = watcher.add_watch(options);
    require(report.ok, "large tree raw watch should start");

    large_tree_metrics metrics;
    metrics.modeled_paths = workload.modeled_paths;
    metrics.native_events_received = static_cast<int>(workload.events.size());

    std::set<std::string> coalesced_candidates;
    const auto rss_before = current_rss_kib();
    const auto started = std::chrono::steady_clock::now();

    std::atomic<bool> producer_done{false};
    std::thread producer([&] {
        for (const auto& spec : workload.events) {
            while (ld::detail::queued_events_for_tests(watcher) > 384) {
                std::this_thread::sleep_for(std::chrono::milliseconds{1});
            }
            backend->push(backend->make_event(report.id, spec.relative, spec.kind, spec.type));
        }
        producer_done = true;
    });

    while (!producer_done || metrics.events_delivered < metrics.native_events_received) {
        metrics.max_queue_depth = std::max(metrics.max_queue_depth, ld::detail::queued_events_for_tests(watcher));
        const auto event = watcher.wait_for(std::chrono::milliseconds{250});
        if (!event.has_value()) {
            if (producer_done) {
                break;
            }
            continue;
        }
        ++metrics.events_delivered;
        if (event->kind == ld::event_kind::overflow) {
            ++metrics.overflow_events;
            metrics.dropped_events_reported += parse_reported_drop_count(*event);
            continue;
        }
        if (event->path.root_relative.has_value()) {
            coalesced_candidates.insert(event->path.root_relative->generic_string());
        }
    }
    producer.join();

    const auto elapsed = std::chrono::steady_clock::now() - started;
    const auto rss_after = current_rss_kib();
    metrics.max_queue_depth = std::max(metrics.max_queue_depth, ld::detail::queued_events_for_tests(watcher));
    metrics.max_backend_depth = backend->max_backend_depth();
    metrics.candidate_paths_coalesced = static_cast<int>(coalesced_candidates.size());
    metrics.validation_calls = metrics.candidate_paths_coalesced;
    metrics.elapsed = std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
    metrics.equivalent_path_construction = measure_large_tree_path_construction(root, workload);
    if (rss_after > rss_before) {
        metrics.rss_growth_kib = rss_after - rss_before;
    }
    watcher.stop();
    return metrics;
}

large_tree_metrics measure_large_tree_settled_delivery(const large_tree_workload& workload)
{
    const auto root = test_root("large-tree-settled");
    materialize_large_tree_files(root, workload);

    const auto backend = std::make_shared<probe_backend>();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options options;
    options.path = root;
    options.recursive = ld::recursive_policy::emulate;
    options.settle = ld::settle_options{
        std::chrono::milliseconds{0},
        std::chrono::milliseconds{1},
        std::chrono::milliseconds{1},
        std::chrono::milliseconds{500},
        workload.settled_file_candidates.size() + 32};
    const auto report = watcher.add_watch(options);
    require(report.ok, "large tree settled watch should start");

    large_tree_metrics metrics;
    metrics.modeled_paths = workload.modeled_paths;
    metrics.native_events_received = static_cast<int>(workload.events.size());

    std::map<std::string, std::chrono::steady_clock::time_point> sent_at;
    std::set<std::string> coalesced_candidates;
    std::vector<std::chrono::milliseconds> latencies;
    const auto rss_before = current_rss_kib();
    const auto started = std::chrono::steady_clock::now();
    for (const auto& candidate : workload.raw_candidate_paths) {
        sent_at.emplace(candidate, started);
    }

    std::atomic<bool> producer_done{false};
    std::thread producer([&] {
        for (const auto& spec : workload.events) {
            while (ld::detail::queued_events_for_tests(watcher) > 384 ||
                ld::detail::pending_settle_work_for_tests(watcher) > workload.settled_file_candidates.size()) {
                std::this_thread::sleep_for(std::chrono::milliseconds{1});
            }
            backend->push(backend->make_event(report.id, spec.relative, spec.kind, spec.type));
        }
        producer_done = true;
    });

    for (;;) {
        metrics.max_queue_depth = std::max(metrics.max_queue_depth, ld::detail::queued_events_for_tests(watcher));
        metrics.max_pending = std::max(metrics.max_pending, ld::detail::pending_settle_work_for_tests(watcher));
        const auto event = watcher.wait_for(std::chrono::milliseconds{250});
        if (!event.has_value()) {
            if (producer_done && ld::detail::pending_settle_work_for_tests(watcher) == 0) {
                break;
            }
            continue;
        }
        ++metrics.events_delivered;
        if (event->kind == ld::event_kind::overflow) {
            ++metrics.overflow_events;
            metrics.dropped_events_reported += parse_reported_drop_count(*event);
            continue;
        }
        if (!event->path.root_relative.has_value()) {
            continue;
        }
        const auto key = event->path.root_relative->generic_string();
        coalesced_candidates.insert(key);
        const auto sent = sent_at.find(key);
        if (sent != sent_at.end()) {
            latencies.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - sent->second));
        }
    }
    producer.join();

    std::sort(latencies.begin(), latencies.end());
    if (!latencies.empty()) {
        metrics.p50_latency = latencies.at(latencies.size() / 2);
        metrics.p95_latency = latencies.at((latencies.size() * 95) / 100);
    }
    const auto elapsed = std::chrono::steady_clock::now() - started;
    const auto rss_after = current_rss_kib();
    metrics.max_queue_depth = std::max(metrics.max_queue_depth, ld::detail::queued_events_for_tests(watcher));
    metrics.max_pending = std::max(metrics.max_pending, ld::detail::pending_settle_work_for_tests(watcher));
    metrics.max_backend_depth = backend->max_backend_depth();
    metrics.candidate_paths_coalesced = static_cast<int>(coalesced_candidates.size());
    metrics.validation_calls = metrics.candidate_paths_coalesced;
    metrics.elapsed = std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
    metrics.equivalent_path_construction = measure_large_tree_path_construction(root, workload);
    if (rss_after > rss_before) {
        metrics.rss_growth_kib = rss_after - rss_before;
    }
    watcher.stop();
    return metrics;
}

large_tree_metrics measure_large_tree_saturation()
{
    constexpr int saturation_events = 768;

    const auto root = test_root("large-tree-saturation");
    materialize_large_tree_files(root, build_large_tree_workload({1024, false}));

    const auto backend = std::make_shared<probe_backend>();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options options;
    options.path = root;
    const auto report = watcher.add_watch(options);
    require(report.ok, "large tree saturation watch should start");

    large_tree_metrics metrics;
    metrics.modeled_paths = 1024;
    metrics.native_events_received = saturation_events;
    const auto rss_before = current_rss_kib();
    const auto started = std::chrono::steady_clock::now();
    for (int i = 0; i < saturation_events; ++i) {
        backend->push(backend->make_event(report.id, tree_file_path(i), ld::event_kind::modified, ld::path_type::file));
    }

    for (;;) {
        metrics.max_queue_depth = std::max(metrics.max_queue_depth, ld::detail::queued_events_for_tests(watcher));
        const auto event = watcher.wait_for(std::chrono::milliseconds{250});
        if (!event.has_value()) {
            break;
        }
        ++metrics.events_delivered;
        if (event->kind == ld::event_kind::overflow) {
            ++metrics.overflow_events;
            metrics.dropped_events_reported += parse_reported_drop_count(*event);
        }
    }

    const auto elapsed = std::chrono::steady_clock::now() - started;
    const auto rss_after = current_rss_kib();
    metrics.max_queue_depth = std::max(metrics.max_queue_depth, ld::detail::queued_events_for_tests(watcher));
    metrics.max_backend_depth = backend->max_backend_depth();
    metrics.elapsed = std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
    if (rss_after > rss_before) {
        metrics.rss_growth_kib = rss_after - rss_before;
    }
    watcher.stop();
    return metrics;
}

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

#if defined(__linux__)

bool native_backend_is_inotify()
{
    const auto root = test_root("native-backend");
    ld::watcher watcher;

    ld::watch_options options;
    options.path = root;
    const auto report = watcher.add_watch(options);
    require(report.ok, "native performance watch should start for backend detection");
    std::cout << "watch.performance.native.backend=" << ld::to_string(report.capabilities.backend) << "\n";
    watcher.stop();
    return report.capabilities.backend == ld::backend_kind::inotify;
}

struct native_raw_metrics {
    int distinct_paths = 0;
    int events_observed = 0;
    int overflow_events = 0;
    bool event_delivery_available = true;
    std::size_t max_queue_depth = 0;
    std::chrono::microseconds elapsed{0};
    std::chrono::microseconds equivalent_path_construction{0};
    double throughput_paths_per_second = 0.0;
    std::size_t rss_growth_kib = 0;
};

std::chrono::microseconds measure_equivalent_path_construction(
    const std::filesystem::path& root,
    int event_count,
    int distinct_paths)
{
    std::vector<std::filesystem::path> paths;
    paths.reserve(static_cast<std::size_t>(event_count));
    const auto started = std::chrono::steady_clock::now();
    for (int i = 0; i < event_count; ++i) {
        paths.push_back(root / ("native-burst-" + std::to_string(i % distinct_paths) + ".txt"));
    }
    const auto elapsed = std::chrono::steady_clock::now() - started;
    std::size_t observed_size = 0;
    for (const auto& path : paths) {
        observed_size += path.native().size();
    }
    require(observed_size > 0, "equivalent path construction should produce paths");
    return std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
}

native_raw_metrics measure_native_raw_delivery()
{
    constexpr int distinct_paths = 240;

    const auto root = test_root("native-raw");
    ld::watcher watcher;

    ld::watch_options options;
    options.path = root;
    const auto report = watcher.add_watch(options);
    require(report.ok, "native raw performance watch should start");
    require(
        report.capabilities.backend == ld::backend_kind::inotify,
        "native raw performance watch should use inotify");
    std::this_thread::sleep_for(std::chrono::milliseconds{10});

    std::map<std::string, std::chrono::steady_clock::time_point> sent_at;
    const auto rss_before = current_rss_kib();
    const auto started = std::chrono::steady_clock::now();
    for (int i = 0; i < distinct_paths; ++i) {
        const auto name = "native-burst-" + std::to_string(i) + ".txt";
        sent_at.emplace(name, std::chrono::steady_clock::now());
        write_file(root / name, "native");
    }

    native_raw_metrics metrics;
    std::set<std::string> seen_paths;
    while (static_cast<int>(seen_paths.size()) < distinct_paths) {
        metrics.max_queue_depth = std::max(metrics.max_queue_depth, ld::detail::queued_events_for_tests(watcher));
        const auto event = watcher.wait_for(std::chrono::seconds{2});
        if (!event.has_value()) {
            metrics.event_delivery_available = !seen_paths.empty();
            break;
        }
        ++metrics.events_observed;
        if (event->kind == ld::event_kind::overflow) {
            ++metrics.overflow_events;
            continue;
        }
        if (!event->path.root_relative.has_value()) {
            continue;
        }
        const auto filename = event->path.root_relative->filename().string();
        if (sent_at.find(filename) != sent_at.end()) {
            seen_paths.insert(filename);
        }
    }
    const auto elapsed = std::chrono::steady_clock::now() - started;
    const auto rss_after = current_rss_kib();
    metrics.distinct_paths = static_cast<int>(seen_paths.size());
    metrics.max_queue_depth = std::max(metrics.max_queue_depth, ld::detail::queued_events_for_tests(watcher));
    metrics.elapsed = std::chrono::duration_cast<std::chrono::microseconds>(elapsed);
    metrics.equivalent_path_construction =
        measure_equivalent_path_construction(root, std::max(metrics.events_observed, distinct_paths), distinct_paths);
    metrics.throughput_paths_per_second =
        static_cast<double>(metrics.distinct_paths) /
        std::chrono::duration<double>(elapsed).count();
    if (rss_after > rss_before) {
        metrics.rss_growth_kib = rss_after - rss_before;
    }
    watcher.stop();
    return metrics;
}

settle_metrics measure_native_settled_delivery()
{
    constexpr int distinct_paths = 80;

    const auto root = test_root("native-settled");
    ld::watcher watcher;

    ld::watch_options options;
    options.path = root;
    options.settle = ld::settle_options{
        std::chrono::milliseconds{0},
        std::chrono::milliseconds{0},
        std::chrono::milliseconds{1},
        std::nullopt};
    const auto report = watcher.add_watch(options);
    require(report.ok, "native settled performance watch should start");
    require(report.capabilities.backend == ld::backend_kind::inotify,
        "native settled performance watch should use inotify");
    std::this_thread::sleep_for(std::chrono::milliseconds{10});

    std::map<std::string, std::chrono::steady_clock::time_point> sent_at;
    for (int i = 0; i < distinct_paths; ++i) {
        const auto name = "native-settled-" + std::to_string(i) + ".txt";
        sent_at.emplace(name, std::chrono::steady_clock::now());
        write_file(root / name, "stable");
    }

    settle_metrics metrics;
    std::set<std::string> seen_paths;
    std::vector<std::chrono::milliseconds> latencies;
    latencies.reserve(distinct_paths);
    while (static_cast<int>(seen_paths.size()) < distinct_paths) {
        metrics.max_pending = std::max(metrics.max_pending, ld::detail::pending_settle_work_for_tests(watcher));
        const auto event = watcher.wait_for(std::chrono::seconds{5});
        require(event.has_value(), "native settled performance probe should observe every created path");
        if (event->kind == ld::event_kind::overflow || !event->path.root_relative.has_value()) {
            continue;
        }
        const auto filename = event->path.root_relative->filename().string();
        const auto sent = sent_at.find(filename);
        if (sent == sent_at.end() || !seen_paths.insert(filename).second) {
            continue;
        }
        latencies.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - sent->second));
    }
    std::sort(latencies.begin(), latencies.end());
    metrics.delivered = static_cast<int>(seen_paths.size());
    metrics.max_pending = std::max(metrics.max_pending, ld::detail::pending_settle_work_for_tests(watcher));
    metrics.p50_latency = latencies.at(latencies.size() / 2);
    metrics.p95_latency = latencies.at((latencies.size() * 95) / 100);
    watcher.stop();
    return metrics;
}

#endif

} // namespace

int main()
{
    try {
        const auto raw = measure_raw_delivery();
        const auto settled = measure_settled_delivery();
        const auto large_tree_options = read_large_tree_options();
        const auto large_tree_workload = build_large_tree_workload(large_tree_options);
        const auto large_tree_raw = measure_large_tree_raw_delivery(large_tree_workload);
        const auto large_tree_settled = measure_large_tree_settled_delivery(large_tree_workload);
        const auto large_tree_saturation = measure_large_tree_saturation();

        require(raw.delivered == 480, "raw performance probe should deliver all non-overflow events");
        require(raw.overflow_events == 0, "raw performance probe should stay below overflow threshold");
        require(raw.max_queue_depth <= 512, "raw watcher queue depth should stay bounded");
        require(settled.delivered == 96, "settled performance probe should deliver all distinct paths");
        require(settled.max_pending <= 96, "settled work should be bounded by distinct path count");
        require(large_tree_raw.overflow_events == 0, "large tree raw pass should not overflow outside saturation");
        require(large_tree_raw.events_delivered == large_tree_raw.native_events_received,
            "large tree raw pass should deliver every synthetic native event");
        require(large_tree_raw.candidate_paths_coalesced < large_tree_raw.events_delivered,
            "large tree raw pass should show product-side path coalescing");
        require(
            large_tree_settled.overflow_events == 0,
            "large tree settled pass should not overflow outside saturation");
        require(large_tree_settled.events_delivered < large_tree_raw.events_delivered,
            "large tree settled delivery should coalesce repeated file writes before product validation");
        require(large_tree_settled.max_pending <= large_tree_workload.settled_file_candidates.size() + 32,
            "large tree settled pending work should stay within configured capacity");
        require(large_tree_saturation.max_queue_depth <= 512, "large tree saturation public queue should stay bounded");
        require(large_tree_saturation.overflow_events > 0, "large tree saturation should report overflow");
        require(
            large_tree_saturation.dropped_events_reported > 0,
            "large tree saturation should report dropped event count");

        std::cout << "watch.performance.simulated.raw.delivered=" << raw.delivered << "\n";
        std::cout << "watch.performance.simulated.raw.throughput_events_per_second="
                  << raw.throughput_events_per_second << "\n";
        std::cout << "watch.performance.simulated.raw.max_queue_depth=" << raw.max_queue_depth << "\n";
        std::cout << "watch.performance.simulated.raw.max_backend_depth=" << raw.max_backend_depth << "\n";
        std::cout << "watch.performance.simulated.raw.rss_growth_kib=" << raw.rss_growth_kib << "\n";
        std::cout << "watch.performance.simulated.settled.delivered=" << settled.delivered << "\n";
        std::cout << "watch.performance.simulated.settled.max_pending=" << settled.max_pending << "\n";
        std::cout << "watch.performance.simulated.settled.p50_latency_ms=" << settled.p50_latency.count() << "\n";
        std::cout << "watch.performance.simulated.settled.p95_latency_ms=" << settled.p95_latency.count() << "\n";
        std::cout << "watch.performance.simulated.large_tree.modeled_paths=" << large_tree_raw.modeled_paths << "\n";
        std::cout << "watch.performance.simulated.large_tree.opt_in_scale="
                  << (large_tree_options.opt_in_scale ? "true" : "false") << "\n";
        std::cout << "watch.performance.simulated.large_tree.raw.native_events_received="
                  << large_tree_raw.native_events_received << "\n";
        std::cout << "watch.performance.simulated.large_tree.raw.events_delivered="
                  << large_tree_raw.events_delivered << "\n";
        std::cout << "watch.performance.simulated.large_tree.raw.candidate_paths_coalesced="
                  << large_tree_raw.candidate_paths_coalesced << "\n";
        std::cout << "watch.performance.simulated.large_tree.raw.validation_calls="
                  << large_tree_raw.validation_calls << "\n";
        std::cout << "watch.performance.simulated.large_tree.raw.overflow_events="
                  << large_tree_raw.overflow_events << "\n";
        std::cout << "watch.performance.simulated.large_tree.raw.dropped_events_reported="
                  << large_tree_raw.dropped_events_reported << "\n";
        std::cout << "watch.performance.simulated.large_tree.raw.max_queue_depth="
                  << large_tree_raw.max_queue_depth << "\n";
        std::cout << "watch.performance.simulated.large_tree.raw.max_backend_depth="
                  << large_tree_raw.max_backend_depth << "\n";
        std::cout << "watch.performance.simulated.large_tree.raw.max_pending="
                  << large_tree_raw.max_pending << "\n";
        std::cout << "watch.performance.simulated.large_tree.raw.elapsed_us="
                  << large_tree_raw.elapsed.count() << "\n";
        std::cout << "watch.performance.simulated.large_tree.raw.equivalent_path_construction_us="
                  << large_tree_raw.equivalent_path_construction.count() << "\n";
        std::cout << "watch.performance.simulated.large_tree.raw.rss_growth_kib="
                  << large_tree_raw.rss_growth_kib << "\n";
        std::cout << "watch.performance.simulated.large_tree.settled.native_events_received="
                  << large_tree_settled.native_events_received << "\n";
        std::cout << "watch.performance.simulated.large_tree.settled.events_delivered="
                  << large_tree_settled.events_delivered << "\n";
        std::cout << "watch.performance.simulated.large_tree.settled.candidate_paths_coalesced="
                  << large_tree_settled.candidate_paths_coalesced << "\n";
        std::cout << "watch.performance.simulated.large_tree.settled.validation_calls="
                  << large_tree_settled.validation_calls << "\n";
        std::cout << "watch.performance.simulated.large_tree.settled.overflow_events="
                  << large_tree_settled.overflow_events << "\n";
        std::cout << "watch.performance.simulated.large_tree.settled.dropped_events_reported="
                  << large_tree_settled.dropped_events_reported << "\n";
        std::cout << "watch.performance.simulated.large_tree.settled.max_queue_depth="
                  << large_tree_settled.max_queue_depth << "\n";
        std::cout << "watch.performance.simulated.large_tree.settled.max_backend_depth="
                  << large_tree_settled.max_backend_depth << "\n";
        std::cout << "watch.performance.simulated.large_tree.settled.max_pending="
                  << large_tree_settled.max_pending << "\n";
        std::cout << "watch.performance.simulated.large_tree.settled.p50_latency_ms="
                  << large_tree_settled.p50_latency.count() << "\n";
        std::cout << "watch.performance.simulated.large_tree.settled.p95_latency_ms="
                  << large_tree_settled.p95_latency.count() << "\n";
        std::cout << "watch.performance.simulated.large_tree.settled.elapsed_us="
                  << large_tree_settled.elapsed.count() << "\n";
        std::cout << "watch.performance.simulated.large_tree.settled.equivalent_path_construction_us="
                  << large_tree_settled.equivalent_path_construction.count() << "\n";
        std::cout << "watch.performance.simulated.large_tree.settled.rss_growth_kib="
                  << large_tree_settled.rss_growth_kib << "\n";
        std::cout << "watch.performance.simulated.large_tree.saturation.native_events_received="
                  << large_tree_saturation.native_events_received << "\n";
        std::cout << "watch.performance.simulated.large_tree.saturation.events_delivered="
                  << large_tree_saturation.events_delivered << "\n";
        std::cout << "watch.performance.simulated.large_tree.saturation.overflow_events="
                  << large_tree_saturation.overflow_events << "\n";
        std::cout << "watch.performance.simulated.large_tree.saturation.dropped_events_reported="
                  << large_tree_saturation.dropped_events_reported << "\n";
        std::cout << "watch.performance.simulated.large_tree.saturation.max_queue_depth="
                  << large_tree_saturation.max_queue_depth << "\n";
        std::cout << "watch.performance.simulated.large_tree.saturation.max_backend_depth="
                  << large_tree_saturation.max_backend_depth << "\n";
        std::cout << "watch.performance.simulated.large_tree.saturation.elapsed_us="
                  << large_tree_saturation.elapsed.count() << "\n";
        std::cout << "watch.performance.simulated.large_tree.saturation.rss_growth_kib="
                  << large_tree_saturation.rss_growth_kib << "\n";

#if defined(__linux__)
        if (native_backend_is_inotify()) {
            const auto native_raw = measure_native_raw_delivery();

            require(native_raw.max_queue_depth <= 512, "native watcher queue depth should stay bounded");

            std::cout << "watch.performance.inotify.raw.distinct_paths=" << native_raw.distinct_paths << "\n";
            std::cout << "watch.performance.inotify.raw.events_observed=" << native_raw.events_observed << "\n";
            std::cout << "watch.performance.inotify.raw.overflow_events=" << native_raw.overflow_events << "\n";
            std::cout << "watch.performance.inotify.raw.throughput_paths_per_second="
                      << native_raw.throughput_paths_per_second << "\n";
            std::cout << "watch.performance.inotify.raw.max_queue_depth=" << native_raw.max_queue_depth << "\n";
            std::cout << "watch.performance.inotify.raw.max_backend_depth=unobservable\n";
            std::cout << "watch.performance.inotify.raw.rss_growth_kib=" << native_raw.rss_growth_kib << "\n";
            std::cout << "watch.performance.inotify.raw.elapsed_us=" << native_raw.elapsed.count() << "\n";
            std::cout << "watch.performance.inotify.raw.equivalent_path_construction_us="
                      << native_raw.equivalent_path_construction.count() << "\n";
            if (!native_raw.event_delivery_available) {
                std::cout << "watch.performance.inotify.status=skipped_no_native_events\n";
            } else {
                if (native_raw.overflow_events == 0) {
                    require(
                        native_raw.distinct_paths >= 216,
                        "native raw performance probe should observe at least 90 percent of created paths");
                } else {
                    std::cout << "watch.performance.inotify.status=raw_queue_overflow_observed\n";
                }

                const auto native_settled = measure_native_settled_delivery();
                require(
                    native_settled.delivered == 80,
                    "native settled performance probe should deliver all distinct paths");
                require(
                    native_settled.max_pending <= 80,
                    "native settled work should be bounded by distinct path count");

                std::cout << "watch.performance.inotify.settled.delivered=" << native_settled.delivered << "\n";
                std::cout << "watch.performance.inotify.settled.max_pending=" << native_settled.max_pending << "\n";
                std::cout << "watch.performance.inotify.settled.p50_latency_ms="
                          << native_settled.p50_latency.count() << "\n";
                std::cout << "watch.performance.inotify.settled.p95_latency_ms="
                          << native_settled.p95_latency.count() << "\n";
            }
        } else {
            std::cout << "watch.performance.inotify.status=skipped_non_inotify_backend\n";
        }
#else
        std::cout << "watch.performance.inotify.status=not_run_non_linux\n";
#endif
    } catch (const probe_failure& failure) {
        std::cerr << failure.message << "\n";
        return EXIT_FAILURE;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
