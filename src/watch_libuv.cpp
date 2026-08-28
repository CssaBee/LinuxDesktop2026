#include "watch_backend.hpp"

#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <system_error>
#include <thread>
#include <utility>

#include <uv.h>

namespace linuxdesktop::watch::detail {
namespace {

diagnostic make_diagnostic(severity level, std::string code, std::string message, std::filesystem::path path = {})
{
    return diagnostic{level, std::move(code), std::move(message), std::move(path)};
}

path_type classify_existing_path(const std::filesystem::path& path)
{
    std::error_code ec;
    if (std::filesystem::is_regular_file(path, ec)) {
        return path_type::file;
    }
    if (std::filesystem::is_directory(path, ec)) {
        return path_type::directory;
    }
    if (std::filesystem::exists(path, ec)) {
        return path_type::other;
    }
    return path_type::unknown;
}

std::filesystem::path weakly_absolute_path(const std::filesystem::path& path)
{
    std::error_code ec;
    auto absolute = std::filesystem::weakly_canonical(path, ec);
    if (!ec) {
        return absolute;
    }
    absolute = std::filesystem::absolute(path, ec);
    if (!ec) {
        return absolute;
    }
    return path;
}

bool libuv_recursive_supported()
{
#if defined(_WIN32) || defined(__APPLE__)
    return true;
#else
    return false;
#endif
}

class libuv_backend final : public watch_backend {
public:
    libuv_backend()
    {
        uv_loop_init(&loop_);
        loop_.data = this;
        async_.data = this;
        uv_async_init(&loop_, &async_, [](uv_async_t* handle) {
            static_cast<libuv_backend*>(handle->data)->process_commands();
        });
        loop_thread_ = std::thread([this] {
            uv_run(&loop_, UV_RUN_DEFAULT);
        });
    }

    ~libuv_backend() override
    {
        stop();
    }

    start_report add_watch(watch_id id, const watch_options& options) override
    {
        start_report report;
        report.id = id;
        report.capabilities = capabilities();

        const auto absolute = weakly_absolute_path(options.path);
        std::error_code ec;
        if (!std::filesystem::exists(absolute, ec)) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                std::string(diagnostic_code::path_not_found),
                "Watch path does not exist",
                absolute));
            return report;
        }

        const auto type = classify_existing_path(absolute);
        if (type == path_type::other || type == path_type::unknown) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                std::string(diagnostic_code::path_unsupported_type),
                "Watch path is not a regular file or directory",
                absolute));
            return report;
        }
        if ((options.recursive == recursive_policy::native_if_supported ||
                options.recursive == recursive_policy::emulate) &&
            !libuv_recursive_supported()) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                std::string(diagnostic_code::recursive_unsupported),
                "libuv recursive watching is not available on this platform",
                absolute));
            return report;
        }

        command command;
        command.kind = command_kind::add;
        command.id = id;
        command.options = options;
        command.absolute = absolute;
        command.target_type = type;
        auto completed = submit(std::move(command));
        report.diagnostics.insert(report.diagnostics.end(), completed.diagnostics.begin(), completed.diagnostics.end());
        report.ok = completed.ok;
        return report;
    }

    bool remove_watch(watch_id id) override
    {
        command command;
        command.kind = command_kind::remove;
        command.id = id;
        return submit(std::move(command)).ok;
    }

    void stop() override
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopped_) {
                return;
            }
        }

        command command;
        command.kind = command_kind::stop;
        submit(std::move(command));
        if (loop_thread_.joinable()) {
            loop_thread_.join();
        }
        uv_loop_close(&loop_);
    }

    std::optional<watch_event> wait_event() override
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] {
            return stopped_ || !ready_events_.empty();
        });
        if (ready_events_.empty()) {
            return std::nullopt;
        }
        auto event = std::move(ready_events_.front());
        ready_events_.pop_front();
        return event;
    }

    capability_report capabilities() const override
    {
        capability_report report;
        report.backend = backend_kind::libuv;
        report.native_recursive = libuv_recursive_supported();
        report.emulated_recursive = false;
        report.overflow_reporting = false;
        report.settled_file_helper = false;
        report.diagnostics.push_back(make_diagnostic(
            severity::info,
            std::string(diagnostic_code::backend_libuv),
            "Using libuv fs-event backend"));
        return report;
    }

private:
    enum class command_kind {
        add,
        remove,
        stop
    };

    struct command_result {
        bool ok = false;
        std::vector<diagnostic> diagnostics;
    };

    struct command {
        command_kind kind = command_kind::add;
        watch_id id;
        watch_options options;
        std::filesystem::path absolute;
        path_type target_type = path_type::unknown;
        bool completed = false;
        command_result result;
        std::condition_variable cv;
    };

    struct watch_record {
        uv_fs_event_t handle{};
        watch_id id;
        std::filesystem::path watched_absolute;
        path_type target_type = path_type::unknown;
        std::string caller_tag;
        bool watch_files = true;
        bool watch_directories = true;
    };

    command_result submit(command&& value)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (stopped_ && value.kind != command_kind::stop) {
            command_result result;
            result.diagnostics.push_back(make_diagnostic(
                severity::error,
                std::string(diagnostic_code::backend_unavailable),
                "libuv watcher backend is stopped",
                value.options.path));
            return result;
        }

        auto* raw = new command();
        raw->kind = value.kind;
        raw->id = value.id;
        raw->options = std::move(value.options);
        raw->absolute = std::move(value.absolute);
        raw->target_type = value.target_type;
        pending_commands_.push_back(raw);
        uv_async_send(&async_);
        raw->cv.wait(lock, [&] {
            return raw->completed;
        });
        auto result = std::move(raw->result);
        delete raw;
        return result;
    }

    void process_commands()
    {
        for (;;) {
            command* next = nullptr;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (pending_commands_.empty()) {
                    return;
                }
                next = pending_commands_.front();
                pending_commands_.pop_front();
            }

            switch (next->kind) {
            case command_kind::add:
                next->result = add_watch_on_loop(next->id, next->options, next->absolute, next->target_type);
                break;
            case command_kind::remove:
                next->result.ok = remove_watch_on_loop(next->id);
                break;
            case command_kind::stop:
                stop_on_loop();
                next->result.ok = true;
                break;
            }

            {
                std::lock_guard<std::mutex> lock(mutex_);
                next->completed = true;
            }
            next->cv.notify_one();
        }
    }

    command_result add_watch_on_loop(
        watch_id id,
        const watch_options& options,
        const std::filesystem::path& absolute,
        path_type type)
    {
        command_result result;
        auto* record = new watch_record();
        record->id = id;
        record->watched_absolute = absolute;
        record->target_type = type;
        record->caller_tag = options.caller_tag;
        record->watch_files = options.watch_files;
        record->watch_directories = options.watch_directories;
        record->handle.data = record;

        const int init = uv_fs_event_init(&loop_, &record->handle);
        if (init < 0) {
            result.diagnostics.push_back(make_diagnostic(
                severity::error,
                std::string(diagnostic_code::backend_error),
                uv_strerror(init),
                absolute));
            delete record;
            return result;
        }

        unsigned int flags = 0;
        if (options.recursive != recursive_policy::none && libuv_recursive_supported()) {
            flags |= UV_FS_EVENT_RECURSIVE;
        }
        const int started = uv_fs_event_start(
            &record->handle,
            [](uv_fs_event_t* handle, const char* filename, int events, int status) {
                static_cast<libuv_backend*>(handle->loop->data)->on_event(handle, filename, events, status);
            },
            absolute.string().c_str(),
            flags);
        if (started < 0) {
            result.diagnostics.push_back(make_diagnostic(
                severity::error,
                std::string(diagnostic_code::backend_error),
                uv_strerror(started),
                absolute));
            uv_close(reinterpret_cast<uv_handle_t*>(&record->handle), [](uv_handle_t* handle) {
                delete static_cast<watch_record*>(handle->data);
            });
            return result;
        }

        watches_[id.value] = record;
        result.ok = true;
        return result;
    }

    bool remove_watch_on_loop(watch_id id)
    {
        const auto it = watches_.find(id.value);
        if (it == watches_.end()) {
            return false;
        }
        auto* record = it->second;
        watches_.erase(it);
        uv_fs_event_stop(&record->handle);
        uv_close(reinterpret_cast<uv_handle_t*>(&record->handle), [](uv_handle_t* handle) {
            delete static_cast<watch_record*>(handle->data);
        });
        return true;
    }

    void stop_on_loop()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopped_ = true;
        }
        for (auto& item : watches_) {
            uv_fs_event_stop(&item.second->handle);
            uv_close(reinterpret_cast<uv_handle_t*>(&item.second->handle), [](uv_handle_t* handle) {
                delete static_cast<watch_record*>(handle->data);
            });
        }
        watches_.clear();
        uv_close(reinterpret_cast<uv_handle_t*>(&async_), nullptr);
        cv_.notify_all();
    }

    void on_event(uv_fs_event_t* handle, const char* filename, int events, int status)
    {
        const auto* record = static_cast<const watch_record*>(handle->data);
        watch_event event;
        event.source = record->id;
        event.caller_tag = record->caller_tag;
        event.path.root = record->id;
        event.path.backend_debug_name = "libuv:" + record->watched_absolute.string();

        if (status < 0) {
            event.kind = event_kind::error;
            event.state = stream_state::degraded;
            event.rescan_recommended = true;
            event.path.absolute = record->watched_absolute;
            event.path.type = record->target_type;
            event.diagnostics.push_back(make_diagnostic(
                severity::error,
                std::string(diagnostic_code::backend_error),
                uv_strerror(status),
                record->watched_absolute));
            push_event(std::move(event));
            return;
        }

        const std::filesystem::path relative = filename ? std::filesystem::path(filename) : std::filesystem::path{};
        event.path.absolute = relative.empty() ? record->watched_absolute : record->watched_absolute / relative;
        if (record->target_type == path_type::file && event.path.absolute == record->watched_absolute) {
            event.path.root_relative = record->watched_absolute.filename();
        } else if (!relative.empty()) {
            event.path.root_relative = relative;
        }

        std::error_code ec;
        if (std::filesystem::is_directory(event.path.absolute, ec)) {
            event.path.type = path_type::directory;
            if (!record->watch_directories) {
                return;
            }
        } else {
            event.path.type = path_type::file;
            if (!record->watch_files) {
                return;
            }
        }

        if (events & UV_RENAME) {
            event.kind = std::filesystem::exists(event.path.absolute, ec) ? event_kind::renamed_new : event_kind::removed;
        } else if (events & UV_CHANGE) {
            event.kind = event_kind::modified;
        } else {
            event.kind = event_kind::metadata;
        }
        push_event(std::move(event));
    }

    void push_event(watch_event event)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            ready_events_.push_back(std::move(event));
        }
        cv_.notify_all();
    }

    uv_loop_t loop_{};
    uv_async_t async_{};
    std::thread loop_thread_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<command*> pending_commands_;
    std::deque<watch_event> ready_events_;
    std::map<std::uint64_t, watch_record*> watches_;
    bool stopped_ = false;
};

} // namespace

std::shared_ptr<watch_backend> make_libuv_backend()
{
    auto backend = std::make_shared<libuv_backend>();
    return backend;
}

} // namespace linuxdesktop::watch::detail
