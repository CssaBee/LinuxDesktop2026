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
#include <optional>
#include <string>
#include <string_view>
#include <stdexcept>
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

bool has_diagnostic(const std::vector<linuxdesktop::diagnostic>& diagnostics, std::string_view code)
{
    for (const auto& item : diagnostics) {
        if (item.code == code) {
            return true;
        }
    }
    return false;
}

const linuxdesktop::diagnostic* find_diagnostic(
    const std::vector<linuxdesktop::diagnostic>& diagnostics,
    std::string_view code)
{
    for (const auto& item : diagnostics) {
        if (item.code == code) {
            return &item;
        }
    }
    return nullptr;
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
    void fail_next_add(linuxdesktop::diagnostic diagnostic)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        next_add_failure_ = std::move(diagnostic);
    }

    ld::start_report add_watch(ld::watch_id id, const ld::watch_options& options) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ld::start_report report;
        report.id = id;
        report.capabilities = capabilities_;

        if (next_add_failure_.has_value()) {
            report.diagnostics.push_back(std::move(*next_add_failure_));
            next_add_failure_.reset();
            return report;
        }

        if (stopped_) {
            report.diagnostics.push_back(diagnostic(
                linuxdesktop::severity::error,
                std::string(ld::diagnostic_code::backend_unavailable),
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
                std::string(ld::diagnostic_code::path_not_found),
                "watch path does not exist",
                path));
            return report;
        }
        if (options.recursive == ld::recursive_policy::native_if_supported && !capabilities_.native_recursive) {
            report.diagnostics.push_back(diagnostic(
                linuxdesktop::severity::error,
                std::string(ld::diagnostic_code::recursive_unsupported),
                "native recursive watch is unavailable",
                path));
            return report;
        }
        if (options.recursive == ld::recursive_policy::emulate) {
            report.diagnostics.push_back(diagnostic(
                linuxdesktop::severity::warning,
                std::string(ld::diagnostic_code::recursive_emulated),
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
            std::string(ld::diagnostic_code::overflow),
            "simulated overflow",
            it->path));
        if (event.rescan_recommended) {
            event.diagnostics.push_back(diagnostic(
                linuxdesktop::severity::warning,
                std::string(ld::diagnostic_code::rescan_recommended),
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
    ld::capability_report capabilities_{ld::backend_kind::simulated, false, true, true, true, {}};
    std::vector<watch_record> watches_;
    std::deque<ld::watch_event> events_;
    std::optional<linuxdesktop::diagnostic> next_add_failure_;
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
    require(ld::to_string(ld::backend_kind::simulated) == "simulated", "backend kind should stringify");
    require(ld::diagnostic_code::backend_error == std::string_view("watch.backend.error"),
        "diagnostic code constants should be public");
}

void reports_start_failures_and_recursive_policy()
{
    const auto backend = make_backend();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options missing;
    missing.path = test_root() / "missing";
    auto report = watcher.add_watch(missing);
    require(!report.ok, "missing path should not start");
    require(has_diagnostic(report.diagnostics, ld::diagnostic_code::path_not_found), "missing path should diagnose path");

    const auto root = test_root();
    ld::watch_options recursive;
    recursive.path = root;
    recursive.recursive = ld::recursive_policy::native_if_supported;
    report = watcher.add_watch(recursive);
    require(!report.ok, "native recursive request should fail when unsupported");
    require(has_diagnostic(report.diagnostics, ld::diagnostic_code::recursive_unsupported),
        "native recursive should diagnose unsupported");

    recursive.recursive = ld::recursive_policy::emulate;
    report = watcher.add_watch(recursive);
    require(report.ok, "emulated recursive request should start");
    require(has_diagnostic(report.diagnostics, ld::diagnostic_code::recursive_emulated),
        "emulated recursive should diagnose policy");
}

void supports_pull_delivery_and_paths()
{
    const auto root = test_root();
    const auto backend = make_backend();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options options;
    options.path = root;
    options.caller_tag = "project-tree";
    const auto report = watcher.add_watch(options);
    require(report.ok, "directory watch should start");
    require(report.capabilities.backend == ld::backend_kind::simulated, "start report should identify backend");
    require(watcher.capabilities().backend == ld::backend_kind::simulated, "capability report should identify backend");
    require(watcher.wait_for(std::chrono::milliseconds{1}) == std::nullopt,
        "timed wait should return empty when no event is queued");

    backend->push(backend->event_for(report.id, ld::event_kind::created, "new.txt"));
    const auto event = watcher.wait_for(std::chrono::seconds{2});
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
    auto watcher = ld::detail::make_watcher_for_backend(backend);

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
    watcher.stop();
}

void callback_stop_and_remove_are_safe()
{
    const auto root = test_root();
    const auto backend = make_backend();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options options;
    options.path = root;
    const auto report = watcher.add_watch(options);
    require(report.ok, "watch should start");

    std::mutex mutex;
    std::condition_variable cv;
    bool stopped = false;
    bool removed = false;

    watcher.set_callback([&](const ld::watch_event&) {
        watcher.remove_watch(report.id);
        {
            std::lock_guard<std::mutex> lock(mutex);
            removed = true;
        }
        watcher.stop();
        {
            std::lock_guard<std::mutex> lock(mutex);
            stopped = true;
        }
        cv.notify_all();
    });

    auto first = backend->event_for(report.id, ld::event_kind::modified, "first.txt");
    auto second = backend->event_for(report.id, ld::event_kind::modified, "second.txt");
    backend->push(std::move(first));
    backend->push(std::move(second));

    std::unique_lock<std::mutex> lock(mutex);
    require(cv.wait_for(lock, std::chrono::seconds(2), [&] { return stopped && removed; }),
        "callback should be able to stop, remove, and replace itself");
    lock.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    require(watcher.state() == ld::stream_state::stopped, "self-stop should leave watcher stopped");
}

void callback_last_owner_release_is_safe()
{
    for (int i = 0; i < 128; ++i) {
        const auto root = test_root() / ("last-owner-" + std::to_string(i));
        std::filesystem::create_directories(root);
        const auto backend = make_backend();
        std::optional<ld::watcher> watcher;
        watcher.emplace(ld::detail::make_watcher_for_backend(backend));

        ld::watch_options options;
        options.path = root;
        const auto report = watcher->add_watch(options);
        require(report.ok, "watch should start");

        struct callback_sync {
            std::mutex mutex;
            std::condition_variable cv;
            bool callback_survived_release = false;
        };
        auto sync = std::make_shared<callback_sync>();

        watcher->set_callback([&, sync](const ld::watch_event&) {
            watcher.reset();
            {
                std::lock_guard<std::mutex> lock(sync->mutex);
                sync->callback_survived_release = true;
            }
            sync->cv.notify_all();
        });

        backend->push(backend->event_for(report.id, ld::event_kind::modified, "destroy.txt"));

        std::unique_lock<std::mutex> lock(sync->mutex);
        require(sync->cv.wait_for(lock, std::chrono::seconds(2), [&] { return sync->callback_survived_release; }),
            "callback should survive releasing the last watcher facade owner");
        require(!watcher.has_value(), "callback should release the watcher facade");
    }
}

void callback_replacement_applies_to_future_events()
{
    const auto root = test_root();
    const auto backend = make_backend();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options options;
    options.path = root;
    const auto report = watcher.add_watch(options);
    require(report.ok, "watch should start");

    std::mutex mutex;
    std::condition_variable cv;
    bool replacement_seen = false;
    bool installed = false;

    watcher.set_callback([&](const ld::watch_event&) {
        watcher.set_callback([&](const ld::watch_event&) {
            std::lock_guard<std::mutex> lock(mutex);
            replacement_seen = true;
            cv.notify_all();
        });
        {
            std::lock_guard<std::mutex> lock(mutex);
            installed = true;
        }
        cv.notify_all();
    });

    backend->push(backend->event_for(report.id, ld::event_kind::modified, "first.txt"));

    std::unique_lock<std::mutex> lock(mutex);
    require(cv.wait_for(lock, std::chrono::seconds(2), [&] { return installed; }),
        "replacement should be installed inside callback");
    lock.unlock();

    backend->push(backend->event_for(report.id, ld::event_kind::modified, "second.txt"));

    lock.lock();
    require(cv.wait_for(lock, std::chrono::seconds(2), [&] { return replacement_seen; }),
        "replacement callback should receive later events");
}

void callback_exception_is_caught_and_falls_back_to_queue()
{
    const auto root = test_root();
    const auto backend = make_backend();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options options;
    options.path = root;
    const auto report = watcher.add_watch(options);
    require(report.ok, "watch should start");

    std::mutex mutex;
    std::condition_variable cv;
    int callback_calls = 0;
    watcher.set_callback([&](const ld::watch_event&) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            ++callback_calls;
        }
        cv.notify_all();
        throw std::runtime_error("boom");
    });

    backend->push(backend->event_for(report.id, ld::event_kind::modified, "broken.txt"));

    std::unique_lock<std::mutex> lock(mutex);
    require(cv.wait_for(lock, std::chrono::seconds(2), [&] { return callback_calls == 1; }),
        "callback should have been invoked once");
    lock.unlock();

    const auto fallback = watcher.wait_for(std::chrono::seconds(2));
    require(fallback.has_value(), "callback failure should surface through queued delivery");
    require(fallback->kind == ld::event_kind::error, "callback failure should become an error event");
    require(has_diagnostic(fallback->diagnostics, ld::diagnostic_code::callback_exception),
        "callback failure should carry a callback diagnostic");
    require(watcher.state() == ld::stream_state::degraded, "callback failure should degrade the watcher");
}

void pull_queue_overflow_emits_rescan_hint()
{
    const auto root = test_root();
    const auto backend = make_backend();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options options;
    options.path = root;
    const auto report = watcher.add_watch(options);
    require(report.ok, "watch should start");

    for (int i = 0; i < 3000; ++i) {
        backend->push(backend->event_for(report.id, ld::event_kind::modified, "burst-" + std::to_string(i) + ".txt"));
    }

    for (int i = 0; i < 200; ++i) {
        if (watcher.state() == ld::stream_state::degraded) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
    require(watcher.state() == ld::stream_state::degraded, "queue overflow should degrade the watcher state");

    const auto received = watcher.wait_for(std::chrono::seconds(2));
    require(received.has_value(), "queue overflow event should arrive");
    require(received->kind == ld::event_kind::overflow, "queue overflow should surface as overflow");
    require(received->state == ld::stream_state::degraded, "queue overflow should degrade the watcher");
    require(received->rescan_recommended, "queue overflow should ask for a rescan");
    require(has_diagnostic(received->diagnostics, ld::diagnostic_code::queue_overflow),
        "queue overflow should carry a queue diagnostic");
}

void pull_queue_overflow_preserves_queued_events_and_counts_drops()
{
    const auto root = test_root();
    const auto backend = make_backend();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options first_options;
    first_options.path = root;
    const auto first_report = watcher.add_watch(first_options);
    require(first_report.ok, "first watch should start");

    const auto second_root = root / "second";
    std::filesystem::create_directories(second_root);
    ld::watch_options second_options;
    second_options.path = second_root;
    const auto second_report = watcher.add_watch(second_options);
    require(second_report.ok, "second watch should start");

    for (int i = 0; i < 510; ++i) {
        backend->push(backend->event_for(first_report.id, ld::event_kind::modified, "filler-" + std::to_string(i) + ".txt"));
    }
    backend->push(backend->event_for(second_report.id, ld::event_kind::modified, "survivor.txt"));
    backend->push(backend->event_for(first_report.id, ld::event_kind::modified, "tail.txt"));
    backend->push(backend->event_for(first_report.id, ld::event_kind::modified, "overflow-trigger.txt"));

    for (int i = 0; i < 200; ++i) {
        if (watcher.state() == ld::stream_state::degraded) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
    require(watcher.state() == ld::stream_state::degraded, "queue overflow should degrade the watcher state");

    const auto overflow = watcher.wait_for(std::chrono::seconds(2));
    require(overflow.has_value(), "queue overflow event should arrive before preserved queued events");
    require(overflow->kind == ld::event_kind::overflow, "queue overflow should be reported first");
    require(overflow->state == ld::stream_state::degraded, "queue overflow should degrade the watcher");
    require(overflow->rescan_recommended, "queue overflow should recommend a rescan");
    const auto* overflow_diagnostic = find_diagnostic(overflow->diagnostics, ld::diagnostic_code::queue_overflow);
    require(overflow_diagnostic != nullptr, "queue overflow should carry a queue diagnostic");
    require(overflow_diagnostic->message.find("dropped 1 event") != std::string::npos,
        "queue overflow should report the number of dropped events");
    require(overflow_diagnostic->message.find("rescan") != std::string::npos,
        "queue overflow should report that a rescan is required");

    bool saw_survivor = false;
    for (int i = 0; i < 511; ++i) {
        const auto event = watcher.wait_for(std::chrono::seconds(2));
        require(event.has_value(), "preserved queued events should remain available after overflow");
        if (event->source == second_report.id && event->path.root_relative == std::filesystem::path{"survivor.txt"}) {
            saw_survivor = true;
        }
    }
    require(saw_survivor, "overflow should preserve unrelated already-queued events instead of clearing the queue");
}

void maps_rename_and_overflow_state()
{
    const auto root = test_root();
    const auto backend = make_backend();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

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
    require(has_diagnostic(event->diagnostics, ld::diagnostic_code::overflow), "overflow should diagnose lost sync");
    require(watcher.state() == ld::stream_state::degraded, "watcher should retain degraded state");
}

void preserves_backend_resource_limit_start_diagnostics()
{
    const auto root = test_root();
    const auto backend = make_backend();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    backend->fail_next_add(diagnostic(
        linuxdesktop::severity::error,
        std::string(ld::diagnostic_code::resource_limit),
        "simulated backend watch limit reached",
        root));

    ld::watch_options options;
    options.path = root;
    const auto report = watcher.add_watch(options);
    require(!report.ok, "resource-limit start failure should not start a watch");
    require(has_diagnostic(report.diagnostics, ld::diagnostic_code::resource_limit),
        "resource-limit start failure should preserve diagnostic code");
    require(watcher.wait_for(std::chrono::milliseconds{1}) == std::nullopt,
        "failed resource-limit start should not enqueue events");
}

void preserves_resource_limit_event_diagnostics()
{
    const auto root = test_root();
    const auto backend = make_backend();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options options;
    options.path = root;
    const auto report = watcher.add_watch(options);
    require(report.ok, "watch should start before resource-limit event");

    auto event = backend->event_for(report.id, ld::event_kind::error, {});
    event.state = ld::stream_state::degraded;
    event.rescan_recommended = true;
    event.diagnostics.push_back(diagnostic(
        linuxdesktop::severity::error,
        std::string(ld::diagnostic_code::resource_limit),
        "simulated backend resource limit",
        root));
    backend->push(std::move(event));

    const auto received = watcher.wait();
    require(received.has_value(), "resource-limit event should arrive");
    require(has_diagnostic(received->diagnostics, ld::diagnostic_code::resource_limit),
        "resource-limit event diagnostic should be preserved");
    require(watcher.state() == ld::stream_state::degraded,
        "resource-limit event should degrade watcher state");
}

void remove_watch_is_idempotent_and_stop_wakes_wait()
{
    const auto root = test_root();
    const auto backend = make_backend();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

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
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options options;
    options.path = root;
    options.settle = ld::settle_options{
        std::chrono::milliseconds{25},
        std::chrono::milliseconds{50},
        std::chrono::milliseconds{10},
        std::nullopt};
    const auto report = watcher.add_watch(options);
    require(report.ok, "settled-file watch should start");
    require(report.capabilities.settled_file_helper, "backend should report settled-file capability");

    backend->push(backend->event_for(report.id, ld::event_kind::modified, "stable.txt"));

    const auto received = watcher.wait();
    require(received.has_value(), "settled-file event should arrive");
    require(has_diagnostic(received->diagnostics, ld::diagnostic_code::settle_ready),
        "settled-file readiness should be visible");
}

void settled_file_wait_does_not_block_raw_delivery()
{
    const auto root = test_root();
    {
        std::ofstream output(root / "slow.txt", std::ios::binary | std::ios::trunc);
        output << "stable";
    }
    const auto backend = make_backend();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options options;
    options.path = root;
    options.settle = ld::settle_options{
        std::chrono::milliseconds{500},
        std::chrono::milliseconds{0},
        std::chrono::milliseconds{10},
        std::nullopt};
    const auto report = watcher.add_watch(options);
    require(report.ok, "settled-file watch should start");

    backend->push(backend->event_for(report.id, ld::event_kind::modified, "slow.txt"));
    backend->push(backend->overflow_for(report.id));

    const auto start = std::chrono::steady_clock::now();
    const auto received = watcher.wait();
    const auto elapsed = std::chrono::steady_clock::now() - start;
    require(received.has_value(), "raw event should arrive while settle is pending");
    require(received->kind == ld::event_kind::overflow, "raw event should bypass pending settle work");
    require(elapsed < std::chrono::milliseconds{300}, "raw delivery should not wait for settled-file debounce");
}

void settled_file_events_coalesce_by_source_and_path()
{
    const auto root = test_root();
    {
        std::ofstream output(root / "coalesce.txt", std::ios::binary | std::ios::trunc);
        output << "stable";
    }
    const auto backend = make_backend();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options options;
    options.path = root;
    options.settle = ld::settle_options{
        std::chrono::milliseconds{25},
        std::chrono::milliseconds{0},
        std::chrono::milliseconds{10},
        std::nullopt};
    const auto report = watcher.add_watch(options);
    require(report.ok, "settled-file watch should start");

    auto first = backend->event_for(report.id, ld::event_kind::modified, "coalesce.txt");
    first.diagnostics.push_back(diagnostic(
        linuxdesktop::severity::info,
        "test.first",
        "first event"));
    auto second = backend->event_for(report.id, ld::event_kind::modified, "coalesce.txt");
    second.diagnostics.push_back(diagnostic(
        linuxdesktop::severity::info,
        "test.second",
        "second event"));

    backend->push(std::move(first));
    backend->push(std::move(second));

    const auto received = watcher.wait();
    require(received.has_value(), "coalesced settled-file event should arrive");
    require(has_diagnostic(received->diagnostics, "test.second"), "latest settled-file event should be delivered");
    require(!has_diagnostic(received->diagnostics, "test.first"), "stale settled-file event should be dropped");
    std::this_thread::sleep_for(std::chrono::milliseconds{75});
    require(watcher.poll() == std::nullopt, "stale settled-file event should not arrive later");
}

void settled_file_timeout_reports_diagnostic()
{
    const auto root = test_root();
    {
        std::ofstream output(root / "timeout.txt", std::ios::binary | std::ios::trunc);
        output << "stable";
    }
    const auto backend = make_backend();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options options;
    options.path = root;
    options.settle = ld::settle_options{
        std::chrono::milliseconds{0},
        std::chrono::milliseconds{250},
        std::chrono::milliseconds{10},
        std::chrono::milliseconds{30}};
    const auto report = watcher.add_watch(options);
    require(report.ok, "settled-file watch should start");

    backend->push(backend->event_for(report.id, ld::event_kind::modified, "timeout.txt"));

    const auto received = watcher.wait_for(std::chrono::seconds{2});
    require(received.has_value(), "settled-file timeout event should arrive");
    require(has_diagnostic(received->diagnostics, ld::diagnostic_code::settle_timeout),
        "settled-file timeout should be visible");
    require(!has_diagnostic(received->diagnostics, ld::diagnostic_code::settle_ready),
        "timed-out settled-file event should not claim readiness");
}

void remove_watch_cancels_pending_settled_file_event()
{
    const auto root = test_root();
    {
        std::ofstream output(root / "cancel.txt", std::ios::binary | std::ios::trunc);
        output << "stable";
    }
    const auto backend = make_backend();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options options;
    options.path = root;
    options.settle = ld::settle_options{
        std::chrono::milliseconds{200},
        std::chrono::milliseconds{0},
        std::chrono::milliseconds{10},
        std::nullopt};
    const auto report = watcher.add_watch(options);
    require(report.ok, "settled-file watch should start");

    backend->push(backend->event_for(report.id, ld::event_kind::modified, "cancel.txt"));
    require(watcher.remove_watch(report.id), "watch removal should succeed");

    const auto received = watcher.wait_for(std::chrono::milliseconds{400});
    require(!received.has_value(), "removed watch should cancel pending settled-file event");
}

void stop_cancels_pending_settled_file_event()
{
    const auto root = test_root();
    {
        std::ofstream output(root / "stop.txt", std::ios::binary | std::ios::trunc);
        output << "stable";
    }
    const auto backend = make_backend();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options options;
    options.path = root;
    options.settle = ld::settle_options{
        std::chrono::milliseconds{500},
        std::chrono::milliseconds{0},
        std::chrono::milliseconds{10},
        std::nullopt};
    const auto report = watcher.add_watch(options);
    require(report.ok, "settled-file watch should start");

    backend->push(backend->event_for(report.id, ld::event_kind::modified, "stop.txt"));
    std::this_thread::sleep_for(std::chrono::milliseconds{50});
    watcher.stop();

    const auto received = watcher.wait_for(std::chrono::milliseconds{1});
    require(!received.has_value(), "stopped watcher should cancel pending settled-file event");
    require(watcher.state() == ld::stream_state::stopped, "stopped watcher should report stopped state");
}

void repeated_add_remove_start_stays_usable()
{
    const auto root = test_root();
    const auto backend = make_backend();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options options;
    options.path = root;

    for (int i = 0; i < 64; ++i) {
        const auto report = watcher.add_watch(options);
        require(report.ok, "repeated watch should start");
        require(watcher.remove_watch(report.id), "repeated watch should remove");
    }

    const auto final_report = watcher.add_watch(options);
    require(final_report.ok, "watcher should accept a final watch after churn");

    backend->push(backend->event_for(final_report.id, ld::event_kind::modified, "after-churn.txt"));

    const auto received = watcher.wait_for(std::chrono::seconds{2});
    require(received.has_value(), "watcher should deliver after repeated add/remove churn");
    require(received->path.root_relative == std::filesystem::path("after-churn.txt"),
        "watcher should preserve paths after repeated add/remove churn");
}

void removed_settled_watch_does_not_stop_future_settlement()
{
    const auto root = test_root();
    {
        std::ofstream output(root / "cancel.txt", std::ios::binary | std::ios::trunc);
        output << "stable";
    }
    {
        std::ofstream output(root / "later.txt", std::ios::binary | std::ios::trunc);
        output << "stable";
    }
    const auto backend = make_backend();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options options;
    options.path = root;
    options.settle = ld::settle_options{
        std::chrono::milliseconds{200},
        std::chrono::milliseconds{0},
        std::chrono::milliseconds{10},
        std::nullopt};
    const auto canceled_report = watcher.add_watch(options);
    require(canceled_report.ok, "first settled-file watch should start");

    backend->push(backend->event_for(canceled_report.id, ld::event_kind::modified, "cancel.txt"));
    std::this_thread::sleep_for(std::chrono::milliseconds{50});
    require(watcher.remove_watch(canceled_report.id), "watch removal should succeed");

    const auto later_report = watcher.add_watch(options);
    require(later_report.ok, "later settled-file watch should start");
    backend->push(backend->event_for(later_report.id, ld::event_kind::modified, "later.txt"));

    const auto received = watcher.wait_for(std::chrono::seconds{2});
    require(received.has_value(), "later settled-file event should arrive after removing earlier watch");
    require(received->path.root_relative == std::filesystem::path("later.txt"),
        "later settled-file event should come from the later watch");
}

void stale_settled_generation_does_not_stop_future_settlement()
{
    const auto root = test_root();
    {
        std::ofstream output(root / "stale.txt", std::ios::binary | std::ios::trunc);
        output << "stable";
    }
    {
        std::ofstream output(root / "next.txt", std::ios::binary | std::ios::trunc);
        output << "stable";
    }
    const auto backend = make_backend();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options options;
    options.path = root;
    options.settle = ld::settle_options{
        std::chrono::milliseconds{75},
        std::chrono::milliseconds{0},
        std::chrono::milliseconds{10},
        std::nullopt};
    const auto report = watcher.add_watch(options);
    require(report.ok, "settled-file watch should start");

    auto stale = backend->event_for(report.id, ld::event_kind::modified, "stale.txt");
    stale.diagnostics.push_back(diagnostic(
        linuxdesktop::severity::info,
        "test.stale",
        "stale event"));
    auto fresh = backend->event_for(report.id, ld::event_kind::modified, "stale.txt");
    fresh.diagnostics.push_back(diagnostic(
        linuxdesktop::severity::info,
        "test.fresh",
        "fresh event"));

    backend->push(std::move(stale));
    backend->push(std::move(fresh));
    backend->push(backend->event_for(report.id, ld::event_kind::modified, "next.txt"));

    auto received = watcher.wait_for(std::chrono::seconds{2});
    require(received.has_value(), "fresh settled-file event should arrive after stale generation is skipped");
    require(has_diagnostic(received->diagnostics, "test.fresh"), "fresh generation should be delivered");
    require(!has_diagnostic(received->diagnostics, "test.stale"), "stale generation should not be delivered");

    received = watcher.wait_for(std::chrono::seconds{2});
    require(received.has_value(), "next settled-file event should arrive after stale generation is skipped");
    require(received->path.root_relative == std::filesystem::path("next.txt"),
        "worker should continue to unrelated settled work after stale generation");
}

void settled_file_burst_for_one_path_keeps_pending_work_bounded()
{
    const auto root = test_root();
    {
        std::ofstream output(root / "burst.txt", std::ios::binary | std::ios::trunc);
        output << "stable";
    }
    const auto backend = make_backend();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options options;
    options.path = root;
    options.settle = ld::settle_options{
        std::chrono::milliseconds{1000},
        std::chrono::milliseconds{0},
        std::chrono::milliseconds{10},
        std::nullopt};
    const auto report = watcher.add_watch(options);
    require(report.ok, "settled-file watch should start");

    backend->push(backend->event_for(report.id, ld::event_kind::modified, "burst.txt"));
    for (int i = 0; i < 200; ++i) {
        if (ld::detail::pending_settle_work_for_tests(watcher) == 1) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
    require(ld::detail::pending_settle_work_for_tests(watcher) == 1,
        "first settled-file event should become active before the burst");

    constexpr int burst_count = 3000;
    for (int i = 0; i < burst_count; ++i) {
        auto event = backend->event_for(report.id, ld::event_kind::modified, "burst.txt");
        if (i == burst_count - 1) {
            event.diagnostics.push_back(diagnostic(
                linuxdesktop::severity::info,
                "test.burst.latest",
                "latest burst event"));
        }
        backend->push(std::move(event));
    }

    for (int i = 0; i < 200; ++i) {
        const auto work = ld::detail::pending_settle_work_for_tests(watcher);
        require(work <= 1, "same-path settled-file burst should keep at most one pending work item");
        if (work == 1) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }

    const auto received = watcher.wait_for(std::chrono::seconds{4});
    require(received.has_value(), "latest settled-file burst event should arrive");
    require(has_diagnostic(received->diagnostics, "test.burst.latest"),
        "same-path settled-file burst should deliver the latest event");
    require(watcher.poll() == std::nullopt, "same-path settled-file burst should not leave stale events");
}

void settled_file_slow_path_does_not_block_ready_paths()
{
    const auto root = test_root();
    {
        std::ofstream output(root / "slow.txt", std::ios::binary | std::ios::trunc);
        output << "stable";
    }
    constexpr int fast_count = 25;
    for (int i = 0; i < fast_count; ++i) {
        std::ofstream output(root / ("fast-" + std::to_string(i) + ".txt"), std::ios::binary | std::ios::trunc);
        output << "stable";
    }
    const auto backend = make_backend();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options slow_options;
    slow_options.path = root;
    slow_options.settle = ld::settle_options{
        std::chrono::milliseconds{0},
        std::chrono::milliseconds{700},
        std::chrono::milliseconds{50},
        std::chrono::milliseconds{1200}};
    const auto slow_report = watcher.add_watch(slow_options);
    require(slow_report.ok, "slow settled-file watch should start");

    ld::watch_options fast_options;
    fast_options.path = root;
    fast_options.settle = ld::settle_options{
        std::chrono::milliseconds{0},
        std::chrono::milliseconds{0},
        std::chrono::milliseconds{10},
        std::nullopt};
    const auto fast_report = watcher.add_watch(fast_options);
    require(fast_report.ok, "fast settled-file watch should start");

    backend->push(backend->event_for(slow_report.id, ld::event_kind::modified, "slow.txt"));
    for (int i = 0; i < fast_count; ++i) {
        backend->push(backend->event_for(fast_report.id, ld::event_kind::modified, "fast-" + std::to_string(i) + ".txt"));
    }

    const auto started_at = std::chrono::steady_clock::now();
    std::vector<std::filesystem::path> received_paths;
    for (int i = 0; i < fast_count; ++i) {
        const auto received = watcher.wait_for(std::chrono::milliseconds{500});
        require(received.has_value(), "ready settled-file paths should arrive before the slow path");
        require(received->source == fast_report.id, "ready settled-file paths should not wait behind the slow path");
        received_paths.push_back(*received->path.root_relative);
    }
    const auto elapsed = std::chrono::steady_clock::now() - started_at;
    require(elapsed < std::chrono::milliseconds{500},
        "ready settled-file paths should not wait for another path's stability window");

    for (int i = 0; i < fast_count; ++i) {
        require(std::find(received_paths.begin(), received_paths.end(), std::filesystem::path("fast-" + std::to_string(i) + ".txt")) != received_paths.end(),
            "ready settled-file batch should preserve every path");
    }
}

void settled_file_large_batch_delivers_all_paths()
{
    const auto root = test_root();
    constexpr int event_count = 64;
    for (int i = 0; i < event_count; ++i) {
        std::ofstream output(root / ("batch-" + std::to_string(i) + ".txt"), std::ios::binary | std::ios::trunc);
        output << "stable";
    }
    const auto backend = make_backend();
    auto watcher = ld::detail::make_watcher_for_backend(backend);

    ld::watch_options options;
    options.path = root;
    options.settle = ld::settle_options{
        std::chrono::milliseconds{0},
        std::chrono::milliseconds{0},
        std::chrono::milliseconds{10},
        std::nullopt};
    const auto report = watcher.add_watch(options);
    require(report.ok, "settled-file watch should start");

    for (int i = 0; i < event_count; ++i) {
        backend->push(backend->event_for(report.id, ld::event_kind::modified, "batch-" + std::to_string(i) + ".txt"));
    }

    std::vector<std::filesystem::path> received_paths;
    for (int i = 0; i < event_count; ++i) {
        const auto received = watcher.wait_for(std::chrono::seconds{2});
        require(received.has_value(), "settled-file batch event should arrive");
        received_paths.push_back(*received->path.root_relative);
    }

    for (int i = 0; i < event_count; ++i) {
        require(std::find(received_paths.begin(), received_paths.end(), std::filesystem::path("batch-" + std::to_string(i) + ".txt")) != received_paths.end(),
            "settled-file batch should preserve every path");
    }
    require(watcher.poll() == std::nullopt, "settled-file batch should not leave extra events");
}

} // namespace

int main()
{
    try {
        exposes_public_version_and_strings();
        reports_start_failures_and_recursive_policy();
        supports_pull_delivery_and_paths();
        supports_callback_delivery();
        callback_stop_and_remove_are_safe();
        callback_last_owner_release_is_safe();
        callback_replacement_applies_to_future_events();
        callback_exception_is_caught_and_falls_back_to_queue();
        pull_queue_overflow_emits_rescan_hint();
        pull_queue_overflow_preserves_queued_events_and_counts_drops();
        maps_rename_and_overflow_state();
        preserves_backend_resource_limit_start_diagnostics();
        preserves_resource_limit_event_diagnostics();
        remove_watch_is_idempotent_and_stop_wakes_wait();
        captures_settled_file_options_in_start_path();
        settled_file_wait_does_not_block_raw_delivery();
        settled_file_events_coalesce_by_source_and_path();
        settled_file_timeout_reports_diagnostic();
        remove_watch_cancels_pending_settled_file_event();
        stop_cancels_pending_settled_file_event();
        repeated_add_remove_start_stays_usable();
        removed_settled_watch_does_not_stop_future_settlement();
        stale_settled_generation_does_not_stop_future_settlement();
        settled_file_burst_for_one_path_keeps_pending_work_bounded();
        settled_file_slow_path_does_not_block_ready_paths();
        settled_file_large_batch_delivers_all_paths();
    } catch (const test_failure& failure) {
        std::cerr << failure.message << "\n";
        return EXIT_FAILURE;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
