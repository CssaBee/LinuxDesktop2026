#include "watch_backend.hpp"

#include <condition_variable>
#include <deque>
#include <map>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

namespace linuxdesktop::watch {
namespace {

diagnostic make_diagnostic(severity level, std::string code, std::string message, std::filesystem::path path = {})
{
    return diagnostic{level, std::move(code), std::move(message), std::move(path)};
}

} // namespace

std::string_view to_string(event_kind value)
{
    switch (value) {
    case event_kind::created:
        return "created";
    case event_kind::modified:
        return "modified";
    case event_kind::removed:
        return "removed";
    case event_kind::renamed_old:
        return "renamed_old";
    case event_kind::renamed_new:
        return "renamed_new";
    case event_kind::metadata:
        return "metadata";
    case event_kind::overflow:
        return "overflow";
    case event_kind::error:
        return "error";
    }
    return "unknown";
}

std::string_view to_string(path_type value)
{
    switch (value) {
    case path_type::file:
        return "file";
    case path_type::directory:
        return "directory";
    case path_type::other:
        return "other";
    case path_type::unknown:
        return "unknown";
    }
    return "unknown";
}

std::string_view to_string(recursive_policy value)
{
    switch (value) {
    case recursive_policy::none:
        return "none";
    case recursive_policy::native_if_supported:
        return "native_if_supported";
    case recursive_policy::emulate:
        return "emulate";
    }
    return "unknown";
}

std::string_view to_string(overflow_policy value)
{
    switch (value) {
    case overflow_policy::report_only:
        return "report_only";
    case overflow_policy::request_rescan:
        return "request_rescan";
    }
    return "unknown";
}

std::string_view to_string(stream_state value)
{
    switch (value) {
    case stream_state::clean:
        return "clean";
    case stream_state::degraded:
        return "degraded";
    case stream_state::stopped:
        return "stopped";
    }
    return "unknown";
}

std::string_view to_string(backend_kind value)
{
    switch (value) {
    case backend_kind::unavailable:
        return "unavailable";
    case backend_kind::inotify:
        return "inotify";
    case backend_kind::read_directory_changes_w:
        return "read_directory_changes_w";
    case backend_kind::libuv:
        return "libuv";
    case backend_kind::simulated:
        return "simulated";
    }
    return "unknown";
}

class watcher::impl {
public:
    explicit impl(std::shared_ptr<detail::watch_backend> backend)
        : backend_(std::move(backend))
    {
        if (backend_) {
            worker_ = std::thread([this] {
                run();
            });
            settle_worker_ = std::thread([this] {
                run_settle();
            });
        } else {
            state_ = stream_state::stopped;
        }
    }

    ~impl()
    {
        stop();
    }

    impl(const impl&) = delete;
    impl& operator=(const impl&) = delete;

    start_report add_watch(const watch_options& options)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!backend_ || stopped_) {
            start_report report;
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                std::string(diagnostic_code::backend_unavailable),
                "Watcher backend is stopped",
                options.path));
            return report;
        }
        const watch_id id{next_id_++};
        auto report = backend_->add_watch(id, options);
        if (report.ok && report.id.value == 0) {
            report.id = id;
        }
        if (report.ok) {
            watches_[report.id.value] = options;
        }
        return report;
    }

    bool remove_watch(watch_id id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!backend_) {
            return false;
        }
        const auto removed = backend_->remove_watch(id);
        if (removed) {
            watches_.erase(id.value);
        }
        return removed;
    }

    void stop()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopped_) {
                return;
            }
            stopped_ = true;
            state_ = stream_state::stopped;
            if (backend_) {
                backend_->stop();
            }
        }
        cv_.notify_all();
        settle_cv_.notify_all();
        if (worker_.joinable()) {
            worker_.join();
        }
        if (settle_worker_.joinable()) {
            settle_worker_.join();
        }
    }

    void set_callback(event_callback callback)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callback_ = std::move(callback);
    }

    std::optional<watch_event> poll()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return std::nullopt;
        }
        auto event = std::move(queue_.front());
        queue_.pop_front();
        return event;
    }

    std::optional<watch_event> wait()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] {
            return !queue_.empty() || stopped_;
        });
        if (queue_.empty()) {
            return std::nullopt;
        }
        auto event = std::move(queue_.front());
        queue_.pop_front();
        return event;
    }

    std::optional<watch_event> wait_for(std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (timeout.count() < 0) {
            timeout = std::chrono::milliseconds{0};
        }
        const auto ready = cv_.wait_for(lock, timeout, [this] {
            return !queue_.empty() || stopped_;
        });
        if (!ready || queue_.empty()) {
            return std::nullopt;
        }
        auto event = std::move(queue_.front());
        queue_.pop_front();
        return event;
    }

    capability_report capabilities() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!backend_) {
            capability_report report;
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                std::string(diagnostic_code::backend_unavailable),
                "Watcher backend is unavailable"));
            return report;
        }
        return backend_->capabilities();
    }

    stream_state state() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

private:
    struct settle_task {
        std::string key;
        std::uint64_t generation = 0;
        watch_event event;
    };

    void run()
    {
        for (;;) {
            auto event = backend_->wait_event();
            if (!event.has_value()) {
                std::lock_guard<std::mutex> lock(mutex_);
                if (stopped_) {
                    cv_.notify_all();
                    return;
                }
                continue;
            }
            if (needs_settle(*event)) {
                enqueue_for_settle(std::move(*event));
            } else {
                deliver(std::move(*event));
            }
        }
    }

    std::optional<watch_options> options_for(watch_id id) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = watches_.find(id.value);
        if (it == watches_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    bool is_stopped_for_settle() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return stopped_;
    }

    bool wait_for_settle_delay(std::chrono::milliseconds delay) const
    {
        if (delay.count() <= 0) {
            return !is_stopped_for_settle();
        }
        std::unique_lock<std::mutex> lock(mutex_);
        return !settle_cv_.wait_for(lock, delay, [this] {
            return stopped_;
        });
    }

    bool needs_settle(const watch_event& event) const
    {
        if (event.kind != event_kind::created && event.kind != event_kind::modified &&
            event.kind != event_kind::renamed_new) {
            return false;
        }
        if (event.path.type == path_type::directory) {
            return false;
        }

        const auto options = options_for(event.source);
        if (!options.has_value() || !options->settle.has_value()) {
            return false;
        }
        return true;
    }

    std::string settle_key(const watch_event& event) const
    {
        return std::to_string(event.source.value) + "\n" + event.path.absolute.string();
    }

    void enqueue_for_settle(watch_event event)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto key = settle_key(event);
        const auto generation = ++settle_generations_[key];
        settle_queue_.push_back(settle_task{std::move(key), generation, std::move(event)});
        settle_cv_.notify_one();
    }

    void run_settle()
    {
        for (;;) {
            settle_task task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                settle_cv_.wait(lock, [this] {
                    return stopped_ || !settle_queue_.empty();
                });
                if (settle_queue_.empty()) {
                    return;
                }
                task = std::move(settle_queue_.front());
                settle_queue_.pop_front();
            }

            auto event = apply_settle_policy(std::move(task.event));
            if (!event.has_value()) {
                return;
            }
            if (is_current_settle_task(task.key, task.generation)) {
                deliver(std::move(*event));
            }
        }
    }

    bool is_current_settle_task(const std::string& key, std::uint64_t generation) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = settle_generations_.find(key);
        return !stopped_ && it != settle_generations_.end() && it->second == generation;
    }

    std::optional<watch_event> apply_settle_policy(watch_event event) const
    {
        const auto options = options_for(event.source);
        if (!options.has_value() || !options->settle.has_value()) {
            return event;
        }

        const auto settle = *options->settle;
        if (!wait_for_settle_delay(settle.debounce_for)) {
            return std::nullopt;
        }
        if (settle.stable_for.count() <= 0) {
            return event;
        }

        std::error_code ec;
        auto last_size = std::filesystem::file_size(event.path.absolute, ec);
        if (ec) {
            event.diagnostics.push_back(make_diagnostic(
                severity::warning,
                std::string(diagnostic_code::settle_timeout),
                ec.message(),
                event.path.absolute));
            return event;
        }
        auto last_write = std::filesystem::last_write_time(event.path.absolute, ec);
        if (ec) {
            event.diagnostics.push_back(make_diagnostic(
                severity::warning,
                std::string(diagnostic_code::settle_timeout),
                ec.message(),
                event.path.absolute));
            return event;
        }

        auto stable_for = std::chrono::milliseconds{0};
        const auto poll_interval =
            settle.poll_interval.count() > 0 ? settle.poll_interval : std::chrono::milliseconds{100};
        while (stable_for < settle.stable_for) {
            if (!wait_for_settle_delay(poll_interval)) {
                return std::nullopt;
            }
            const auto size = std::filesystem::file_size(event.path.absolute, ec);
            if (ec) {
                event.diagnostics.push_back(make_diagnostic(
                    severity::warning,
                    std::string(diagnostic_code::settle_timeout),
                    ec.message(),
                    event.path.absolute));
                return event;
            }
            const auto write_time = std::filesystem::last_write_time(event.path.absolute, ec);
            if (ec) {
                event.diagnostics.push_back(make_diagnostic(
                    severity::warning,
                    std::string(diagnostic_code::settle_timeout),
                    ec.message(),
                    event.path.absolute));
                return event;
            }
            if (size == last_size && write_time == last_write) {
                stable_for += poll_interval;
            } else {
                stable_for = std::chrono::milliseconds{0};
                last_size = size;
                last_write = write_time;
            }
        }

        event.diagnostics.push_back(make_diagnostic(
            severity::info,
            std::string(diagnostic_code::settle_ready),
            "File size and mtime are stable",
            event.path.absolute));
        return event;
    }

    void deliver(watch_event event)
    {
        event_callback callback;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (event.state == stream_state::degraded) {
                state_ = stream_state::degraded;
            }
            callback = callback_;
            if (!callback) {
                queue_.push_back(std::move(event));
                cv_.notify_all();
                return;
            }
        }
        callback(event);
    }

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    mutable std::condition_variable settle_cv_;
    std::shared_ptr<detail::watch_backend> backend_;
    std::thread worker_;
    std::thread settle_worker_;
    std::deque<watch_event> queue_;
    event_callback callback_;
    std::map<std::uint64_t, watch_options> watches_;
    std::deque<settle_task> settle_queue_;
    std::map<std::string, std::uint64_t> settle_generations_;
    std::uint64_t next_id_ = 1;
    stream_state state_ = stream_state::clean;
    bool stopped_ = false;
};

watcher::watcher()
    : impl_(std::make_unique<impl>(detail::make_native_backend()))
{
}

watcher::watcher(std::shared_ptr<detail::watch_backend> backend)
    : impl_(std::make_unique<impl>(std::move(backend)))
{
}

watcher::~watcher() = default;

watcher::watcher(watcher&&) noexcept = default;

watcher& watcher::operator=(watcher&&) noexcept = default;

start_report watcher::add_watch(const watch_options& options)
{
    return impl_->add_watch(options);
}

bool watcher::remove_watch(watch_id id)
{
    return impl_->remove_watch(id);
}

void watcher::stop()
{
    impl_->stop();
}

void watcher::set_callback(event_callback callback)
{
    impl_->set_callback(std::move(callback));
}

std::optional<watch_event> watcher::poll()
{
    return impl_->poll();
}

std::optional<watch_event> watcher::wait()
{
    return impl_->wait();
}

std::optional<watch_event> watcher::wait_for(std::chrono::milliseconds timeout)
{
    return impl_->wait_for(timeout);
}

capability_report watcher::capabilities() const
{
    return impl_->capabilities();
}

stream_state watcher::state() const
{
    return impl_->state();
}

namespace detail {

watcher make_watcher_for_backend(std::shared_ptr<watch_backend> backend)
{
    return watcher(std::move(backend));
}

} // namespace detail

} // namespace linuxdesktop::watch
