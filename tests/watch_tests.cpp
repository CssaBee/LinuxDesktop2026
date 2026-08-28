#include "linuxdesktop/watch.hpp"
#include "watch_backend.hpp"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <deque>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

namespace ld = linuxdesktop::watch;

struct test_failure {
    std::string message;
};

[[noreturn]] void fail(std::string message)
{
    throw test_failure{std::move(message)};
}

void require(bool condition, const std::string& message)
{
    if (!condition) {
        fail(message);
    }
}

bool has_diagnostic(const std::vector<linuxdesktop::diagnostic>& diagnostics, const std::string& code)
{
    for (const auto& item : diagnostics) {
        if (item.code == code) {
            return true;
        }
    }
    return false;
}

std::filesystem::path test_root()
{
    auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-watch-tests";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    if (ec) {
        fail("failed to create test root: " + ec.message());
    }
    return root;
}

linuxdesktop::diagnostic diagnostic(
    linuxdesktop::severity level,
    std::string code,
    std::string message,
    std::filesystem::path path = {})
{
    return linuxdesktop::diagnostic{level, std::move(code), std::move(message), std::move(path)};
}

class simulated_backend final : public ld::detail::watch_backend {
public:
    ld::start_report add_watch(ld::watch_id id, const ld::watch_options& options) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ld::start_report report;
        report.id = id;
        report.capabilities = capabilities_;

        if (stopped_) {
            report.diagnostics.push_back(diagnostic(
                linuxdesktop::severity::error,
                "watch.backend.unavailable",
                "simulated backend is stopped",
                options.path));
            return report;
        }

        std::error_code ec;
        const auto absolute = std::filesystem::weakly_canonical(options.path, ec);
        const auto path = ec ? std::filesystem::absolute(options.path) : absolute;
        if (!std::filesystem::exists(path, ec)) {
            report.diagnostics.push_back(diagnostic(
                linuxdesktop::severity::error,
                "watch.path.not_found",
                "watch path does not exist",
                path));
            return report;
        }
        if (options.recursive == ld::recursive_policy::native_if_supported && !capabilities_.native_recursive) {
            report.diagnostics.push_back(diagnostic(
                linuxdesktop::severity::error,
                "watch.recursive.unsupported",
                "native recursive watch is unavailable",
                path));
            return report;
        }
        if (options.recursive == ld::recursive_policy::emulate) {
            report.diagnostics.push_back(diagnostic(
                linuxdesktop::severity::warning,
                "watch.recursive.emulated",
                "recursive watch is simulated with subdirectory watches",
                path));
        }

        watch_record record;
        record.id = id;
        record.path = path;
        record.caller_tag = options.caller_tag;
        record.overflow = options.overflow;
        record.settle = options.settle;
        watches_.push_back(record);

        report.ok = true;
        return report;
    }

    bool remove_watch(ld::watch_id id) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto before = watches_.size();
        watches_.erase(
            std::remove_if(watches_.begin(), watches_.end(), [&](const watch_record& record) {
                return record.id.value == id.value;
            }),
            watches_.end());
        return watches_.size() != before;
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
        std::lock_guard<std::mutex> lock(mutex_);
        return capabilities_;
    }

    void push(ld::watch_event event)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            events_.push_back(std::move(event));
        }
        cv_.notify_all();
    }

    ld::watch_event event_for(
        ld::watch_id id,
        ld::event_kind kind,
        const std::filesystem::path& relative,
        ld::path_type type = ld::path_type::file)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = std::find_if(watches_.begin(), watches_.end(), [&](const watch_record& record) {
            return record.id.value == id.value;
        });
        require(it != watches_.end(), "simulated watch id should exist");

        ld::watch_event event;
        event.kind = kind;
        event.source = id;
        event.caller_tag = it->caller_tag;
        event.path.absolute = it->path / relative;
        event.path.root_relative = relative;
        event.path.root = id;
        event.path.type = type;
        event.path.backend_debug_name = "simulated:" + event.path.absolute.string();
        return event;
    }

    ld::watch_event overflow_for(ld::watch_id id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = std::find_if(watches_.begin(), watches_.end(), [&](const watch_record& record) {
            return record.id.value == id.value;
        });
        require(it != watches_.end(), "simulated watch id should exist");

        ld::watch_event event;
        event.kind = ld::event_kind::overflow;
        event.source = id;
        event.caller_tag = it->caller_tag;
        event.path.absolute = it->path;
        event.path.root = id;
        event.path.type = ld::path_type::directory;
        event.state = ld::stream_state::degraded;
        event.rescan_recommended = it->overflow == ld::overflow_policy::request_rescan;
        event.diagnostics.push_back(diagnostic(
            linuxdesktop::severity::error,
            "watch.overflow",
            "simulated overflow",
            it->path));
        if (event.rescan_recommended) {
            event.diagnostics.push_back(diagnostic(
                linuxdesktop::severity::warning,
                "watch.rescan_recommended",
                "rescan recommended",
                it->path));
        }
        return event;
    }

private:
    struct watch_record {
        ld::watch_id id;
        std::filesystem::path path;
        std::string caller_tag;
        ld::overflow_policy overflow = ld::overflow_policy::request_rescan;
        std::optional<ld::settle_options> settle;
    };

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    ld::capability_report capabilities_{false, true, true, true, {}};
    std::vector<watch_record> watches_;
    std::deque<ld::watch_event> events_;
    bool stopped_ = false;
};

std::shared_ptr<simulated_backend> make_backend()
{
    return std::make_shared<simulated_backend>();
}

void exposes_public_version_and_strings()
{
    require(ld::version_major == 0, "watch version major should match project version");
    require(ld::version_minor == 1, "watch version minor should match project version");
    require(ld::version_patch == 0, "watch version patch should match project version");
    require(ld::to_string(ld::event_kind::created) == "created", "event kind should stringify");
    require(ld::to_string(ld::path_type::directory) == "directory", "path type should stringify");
    require(ld::to_string(ld::recursive_policy::emulate) == "emulate", "recursive policy should stringify");
    require(ld::to_string(ld::overflow_policy::request_rescan) == "request_rescan", "overflow policy should stringify");
    require(ld::to_string(ld::stream_state::degraded) == "degraded", "stream state should stringify");
}

void reports_start_failures_and_recursive_policy()
{
    const auto backend = make_backend();
    ld::watcher watcher(backend);

    ld::watch_options missing;
    missing.path = test_root() / "missing";
    auto report = watcher.add_watch(missing);
    require(!report.ok, "missing path should not start");
    require(has_diagnostic(report.diagnostics, "watch.path.not_found"), "missing path should diagnose path");

    const auto root = test_root();
    ld::watch_options recursive;
    recursive.path = root;
    recursive.recursive = ld::recursive_policy::native_if_supported;
    report = watcher.add_watch(recursive);
    require(!report.ok, "native recursive request should fail when unsupported");
    require(has_diagnostic(report.diagnostics, "watch.recursive.unsupported"), "native recursive should diagnose unsupported");

    recursive.recursive = ld::recursive_policy::emulate;
    report = watcher.add_watch(recursive);
    require(report.ok, "emulated recursive request should start");
    require(has_diagnostic(report.diagnostics, "watch.recursive.emulated"), "emulated recursive should diagnose policy");
}

void supports_pull_delivery_and_paths()
{
    const auto root = test_root();
    const auto backend = make_backend();
    ld::watcher watcher(backend);

    ld::watch_options options;
    options.path = root;
    options.caller_tag = "project-tree";
    const auto report = watcher.add_watch(options);
    require(report.ok, "directory watch should start");

    backend->push(backend->event_for(report.id, ld::event_kind::created, "new.txt"));
    const auto event = watcher.wait();
    require(event.has_value(), "blocking wait should return pushed event");
    require(event->kind == ld::event_kind::created, "event kind should round trip");
    require(event->caller_tag == "project-tree", "caller tag should round trip");
    require(event->path.absolute == root / "new.txt", "absolute path should be preserved");
    require(event->path.root_relative == std::filesystem::path("new.txt"), "root-relative path should be preserved");
    require(event->path.root.value == report.id.value, "watch id should be preserved on path");
    require(watcher.poll() == std::nullopt, "poll should be empty after wait consumed event");
}

void supports_callback_delivery()
{
    const auto root = test_root();
    const auto backend = make_backend();
    ld::watcher watcher(backend);

    ld::watch_options options;
    options.path = root;
    const auto report = watcher.add_watch(options);
    require(report.ok, "watch should start");

    std::mutex mutex;
    std::condition_variable cv;
    std::vector<ld::watch_event> seen;
    watcher.set_callback([&](const ld::watch_event& event) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            seen.push_back(event);
        }
        cv.notify_one();
    });

    backend->push(backend->event_for(report.id, ld::event_kind::modified, "changed.txt"));

    std::unique_lock<std::mutex> lock(mutex);
    require(cv.wait_for(lock, std::chrono::seconds(2), [&] { return !seen.empty(); }),
        "callback should receive event");
    require(seen.front().kind == ld::event_kind::modified, "callback should receive event kind");
    lock.unlock();
    require(watcher.poll() == std::nullopt, "callback-delivered events should not also be queued");
}

void maps_rename_and_overflow_state()
{
    const auto root = test_root();
    const auto backend = make_backend();
    ld::watcher watcher(backend);

    ld::watch_options options;
    options.path = root;
    options.caller_tag = "rename-test";
    options.overflow = ld::overflow_policy::request_rescan;
    const auto report = watcher.add_watch(options);
    require(report.ok, "watch should start");

    auto rename_new = backend->event_for(report.id, ld::event_kind::renamed_new, "after.txt");
    rename_new.old_path = backend->event_for(report.id, ld::event_kind::renamed_old, "before.txt").path;
    rename_new.paired_rename = true;
    backend->push(rename_new);

    auto event = watcher.wait();
    require(event.has_value(), "rename event should arrive");
    require(event->paired_rename, "paired rename should be marked");
    require(event->old_path.has_value(), "paired rename should expose old path");
    require(event->old_path->root_relative == std::filesystem::path("before.txt"), "old path should be root-relative");

    backend->push(backend->overflow_for(report.id));
    event = watcher.wait();
    require(event.has_value(), "overflow event should arrive");
    require(event->kind == ld::event_kind::overflow, "overflow kind should be preserved");
    require(event->state == ld::stream_state::degraded, "overflow event should mark degraded state");
    require(event->rescan_recommended, "overflow should request rescan");
    require(has_diagnostic(event->diagnostics, "watch.overflow"), "overflow should diagnose lost sync");
    require(watcher.state() == ld::stream_state::degraded, "watcher should retain degraded state");
}

void remove_watch_is_idempotent_and_stop_wakes_wait()
{
    const auto root = test_root();
    const auto backend = make_backend();
    ld::watcher watcher(backend);

    ld::watch_options options;
    options.path = root;
    const auto report = watcher.add_watch(options);
    require(report.ok, "watch should start");
    require(watcher.remove_watch(report.id), "first remove should remove watch");
    require(!watcher.remove_watch(report.id), "second remove should be idempotent");

    std::optional<ld::watch_event> waited;
    std::thread waiter([&] {
        waited = watcher.wait();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    watcher.stop();
    waiter.join();

    require(!waited.has_value(), "stopped watcher should wake wait with nullopt");
    require(watcher.state() == ld::stream_state::stopped, "stopped watcher should report stopped state");
}

void captures_settled_file_options_in_start_path()
{
    const auto root = test_root();
    {
        std::ofstream output(root / "stable.txt", std::ios::binary | std::ios::trunc);
        output << "stable";
    }
    const auto backend = make_backend();
    ld::watcher watcher(backend);

    ld::watch_options options;
    options.path = root;
    options.settle = ld::settle_options{std::chrono::milliseconds{25}, std::chrono::milliseconds{50}, std::chrono::milliseconds{10}};
    const auto report = watcher.add_watch(options);
    require(report.ok, "settled-file watch should start");
    require(report.capabilities.settled_file_helper, "backend should report settled-file capability");

    backend->push(backend->event_for(report.id, ld::event_kind::modified, "stable.txt"));

    const auto received = watcher.wait();
    require(received.has_value(), "settled-file event should arrive");
    require(has_diagnostic(received->diagnostics, "watch.settle.ready"), "settled-file readiness should be visible");
}

} // namespace

int main()
{
    try {
        exposes_public_version_and_strings();
        reports_start_failures_and_recursive_policy();
        supports_pull_delivery_and_paths();
        supports_callback_delivery();
        maps_rename_and_overflow_state();
        remove_watch_is_idempotent_and_stop_wakes_wait();
        captures_settled_file_options_in_start_path();
    } catch (const test_failure& failure) {
        std::cerr << failure.message << "\n";
        return EXIT_FAILURE;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
