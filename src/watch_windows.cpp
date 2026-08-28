#include "watch_backend.hpp"

#if defined(_WIN32)

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace linuxdesktop::watch::detail {
namespace {

diagnostic make_diagnostic(severity level, std::string code, std::string message, std::filesystem::path path = {})
{
    return diagnostic{level, std::move(code), std::move(message), std::move(path)};
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

std::string windows_error_message(DWORD code)
{
    LPSTR raw = nullptr;
    const DWORD size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        code,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&raw),
        0,
        nullptr);
    if (size == 0 || raw == nullptr) {
        return "Windows error " + std::to_string(code);
    }
    std::string message(raw, size);
    LocalFree(raw);
    while (!message.empty() && (message.back() == '\n' || message.back() == '\r' || message.back() == ' ')) {
        message.pop_back();
    }
    return message;
}

class windows_backend final : public watch_backend {
public:
    ~windows_backend() override
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

        const auto directory = type == path_type::file ? absolute.parent_path() : absolute;
        const auto file_filter = type == path_type::file ? std::optional<std::filesystem::path>(absolute.filename()) : std::nullopt;

        auto worker = std::make_shared<watch_worker>();
        worker->owner = this;
        worker->id = id;
        worker->watched_absolute = absolute;
        worker->directory = directory;
        worker->file_filter = file_filter;
        worker->target_type = type;
        worker->caller_tag = options.caller_tag;
        worker->watch_files = options.watch_files;
        worker->watch_directories = options.watch_directories;
        worker->recursive = options.recursive != recursive_policy::none && type == path_type::directory;
        worker->overflow = options.overflow;

        worker->handle = CreateFileW(
            directory.wstring().c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
            nullptr);
        if (worker->handle == INVALID_HANDLE_VALUE) {
            const auto error = GetLastError();
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                std::string(error == ERROR_ACCESS_DENIED ? diagnostic_code::path_access_denied : diagnostic_code::backend_error),
                windows_error_message(error),
                directory));
            return report;
        }
        worker->event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (worker->event == nullptr) {
            const auto error = GetLastError();
            CloseHandle(worker->handle);
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                std::string(diagnostic_code::backend_error),
                windows_error_message(error),
                directory));
            return report;
        }

        if (worker->recursive) {
            report.capabilities.native_recursive = true;
            report.diagnostics.push_back(make_diagnostic(
                severity::info,
                std::string(diagnostic_code::recursive_native),
                "Recursive watching is provided by ReadDirectoryChangesW",
                absolute));
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopped_) {
                CloseHandle(worker->handle);
                CloseHandle(worker->event);
                report.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    std::string(diagnostic_code::backend_unavailable),
                    "Windows watcher backend is stopped",
                    absolute));
                return report;
            }
            workers_[id.value] = worker;
        }

        worker->thread = std::thread([worker] {
            worker->run();
        });
        worker->wait_until_ready();

        report.ok = true;
        return report;
    }

    bool remove_watch(watch_id id) override
    {
        std::shared_ptr<watch_worker> worker;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto it = workers_.find(id.value);
            if (it == workers_.end()) {
                return false;
            }
            worker = it->second;
            workers_.erase(it);
        }
        stop_worker(*worker);
        return true;
    }

    void stop() override
    {
        std::vector<std::shared_ptr<watch_worker>> workers;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopped_) {
                return;
            }
            stopped_ = true;
            for (auto& item : workers_) {
                workers.push_back(item.second);
            }
            workers_.clear();
        }
        for (auto& worker : workers) {
            stop_worker(*worker);
        }
        cv_.notify_all();
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
        report.backend = backend_kind::read_directory_changes_w;
        report.native_recursive = true;
        report.emulated_recursive = false;
        report.overflow_reporting = true;
        report.settled_file_helper = false;
        report.diagnostics.push_back(make_diagnostic(
            severity::info,
            std::string(diagnostic_code::backend_windows),
            "Using ReadDirectoryChangesW backend"));
        return report;
    }

private:
    struct watch_worker {
        windows_backend* owner = nullptr;
        HANDLE handle = INVALID_HANDLE_VALUE;
        HANDLE event = nullptr;
        std::thread thread;
        std::atomic<bool> stopping{false};
        watch_id id;
        std::filesystem::path watched_absolute;
        std::filesystem::path directory;
        std::optional<std::filesystem::path> file_filter;
        path_type target_type = path_type::unknown;
        std::string caller_tag;
        bool watch_files = true;
        bool watch_directories = true;
        bool recursive = false;
        overflow_policy overflow = overflow_policy::request_rescan;
        std::optional<watch_path> pending_rename;
        std::mutex ready_mutex;
        std::condition_variable ready_cv;
        bool ready = false;

        void wait_until_ready()
        {
            std::unique_lock<std::mutex> lock(ready_mutex);
            ready_cv.wait_for(lock, std::chrono::seconds(2), [this] {
                return ready;
            });
        }

        void run()
        {
            std::vector<unsigned char> buffer(64 * 1024);
            while (!stopping.load()) {
                ResetEvent(event);
                OVERLAPPED overlapped{};
                overlapped.hEvent = event;
                DWORD bytes_returned = 0;
                const BOOL ok = ReadDirectoryChangesW(
                    handle,
                    buffer.data(),
                    static_cast<DWORD>(buffer.size()),
                    recursive ? TRUE : FALSE,
                    FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES |
                        FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
                    nullptr,
                    &overlapped,
                    nullptr);

                if (!ok && GetLastError() != ERROR_IO_PENDING) {
                    const auto error = GetLastError();
                    mark_ready();
                    if (!stopping.load() && error != ERROR_OPERATION_ABORTED) {
                        owner->push(error_event(windows_error_message(error)));
                    }
                    return;
                }
                mark_ready();

                while (!stopping.load()) {
                    const auto wait = WaitForSingleObject(event, 100);
                    if (wait == WAIT_TIMEOUT) {
                        continue;
                    }
                    if (wait != WAIT_OBJECT_0) {
                        owner->push(error_event(windows_error_message(GetLastError())));
                        return;
                    }
                    break;
                }
                if (stopping.load()) {
                    CancelIoEx(handle, &overlapped);
                    return;
                }
                if (!GetOverlappedResult(handle, &overlapped, &bytes_returned, FALSE)) {
                    const auto error = GetLastError();
                    if (error != ERROR_OPERATION_ABORTED) {
                        owner->push(error_event(windows_error_message(error)));
                    }
                    return;
                }
                if (bytes_returned == 0) {
                    owner->push(overflow_event());
                    continue;
                }

                std::size_t offset = 0;
                for (;;) {
                    const auto* raw = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(buffer.data() + offset);
                    map_raw(*raw);
                    if (raw->NextEntryOffset == 0) {
                        break;
                    }
                    offset += raw->NextEntryOffset;
                }
            }
        }

        void mark_ready()
        {
            {
                std::lock_guard<std::mutex> lock(ready_mutex);
                ready = true;
            }
            ready_cv.notify_all();
        }

        void map_raw(const FILE_NOTIFY_INFORMATION& raw)
        {
            const std::wstring wide_name(raw.FileName, raw.FileNameLength / sizeof(wchar_t));
            const std::filesystem::path relative(wide_name);
            if (file_filter.has_value() && relative != *file_filter) {
                return;
            }

            const auto absolute = directory / relative;
            std::error_code ec;
            const bool exists = std::filesystem::exists(absolute, ec);
            const bool is_directory = std::filesystem::is_directory(absolute, ec);
            const auto type = exists ? (is_directory ? path_type::directory : path_type::file) :
                (raw.Action == FILE_ACTION_REMOVED || raw.Action == FILE_ACTION_RENAMED_OLD_NAME ? path_type::unknown : path_type::file);
            if (type == path_type::directory && !watch_directories) {
                return;
            }
            if (type != path_type::directory && !watch_files) {
                return;
            }

            watch_event event;
            event.path = make_path(absolute, type);
            event.source = id;
            event.caller_tag = caller_tag;

            switch (raw.Action) {
            case FILE_ACTION_ADDED:
                event.kind = event_kind::created;
                break;
            case FILE_ACTION_REMOVED:
                event.kind = event_kind::removed;
                break;
            case FILE_ACTION_MODIFIED:
                event.kind = event_kind::modified;
                break;
            case FILE_ACTION_RENAMED_OLD_NAME:
                event.kind = event_kind::renamed_old;
                pending_rename = event.path;
                break;
            case FILE_ACTION_RENAMED_NEW_NAME:
                event.kind = event_kind::renamed_new;
                if (pending_rename.has_value()) {
                    event.old_path = *pending_rename;
                    event.paired_rename = true;
                    pending_rename.reset();
                } else {
                    event.diagnostics.push_back(make_diagnostic(
                        severity::warning,
                        std::string(diagnostic_code::rename_unpaired),
                        "Rename destination did not have a matching source",
                        absolute));
                }
                break;
            default:
                event.kind = event_kind::metadata;
                break;
            }

            owner->push(std::move(event));
        }

        watch_path make_path(const std::filesystem::path& absolute, path_type type) const
        {
            watch_path path;
            path.absolute = absolute;
            path.root = id;
            path.type = type;
            path.backend_debug_name = "windows:" + absolute.string();

            std::error_code ec;
            const auto relative = std::filesystem::relative(absolute, watched_absolute, ec);
            if (!ec && !relative.empty() && *relative.begin() != std::filesystem::path("..")) {
                path.root_relative = relative;
            }
            return path;
        }

        watch_event overflow_event() const
        {
            watch_event event;
            event.kind = event_kind::overflow;
            event.source = id;
            event.caller_tag = caller_tag;
            event.path = make_path(watched_absolute, target_type);
            event.state = stream_state::degraded;
            event.rescan_recommended = overflow == overflow_policy::request_rescan;
            event.diagnostics.push_back(make_diagnostic(
                severity::error,
                std::string(diagnostic_code::overflow),
                "ReadDirectoryChangesW reported that events may have been lost",
                watched_absolute));
            if (event.rescan_recommended) {
                event.diagnostics.push_back(make_diagnostic(
                    severity::warning,
                    std::string(diagnostic_code::rescan_recommended),
                    "Rescan watched roots before trusting further events",
                    watched_absolute));
            }
            return event;
        }

        watch_event error_event(std::string message) const
        {
            watch_event event;
            event.kind = event_kind::error;
            event.source = id;
            event.caller_tag = caller_tag;
            event.path = make_path(watched_absolute, target_type);
            event.state = stream_state::degraded;
            event.rescan_recommended = true;
            event.diagnostics.push_back(make_diagnostic(
                severity::error,
                std::string(diagnostic_code::backend_error),
                std::move(message),
                watched_absolute));
            return event;
        }
    };

    void stop_worker(watch_worker& worker)
    {
        worker.stopping.store(true);
        CancelIoEx(worker.handle, nullptr);
        if (worker.event != nullptr) {
            SetEvent(worker.event);
        }
        if (worker.thread.joinable()) {
            worker.thread.join();
        }
        if (worker.handle != INVALID_HANDLE_VALUE) {
            CloseHandle(worker.handle);
            worker.handle = INVALID_HANDLE_VALUE;
        }
        if (worker.event != nullptr) {
            CloseHandle(worker.event);
            worker.event = nullptr;
        }
    }

    void push(watch_event event)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopped_) {
                return;
            }
            ready_events_.push_back(std::move(event));
        }
        cv_.notify_all();
    }

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::map<std::uint64_t, std::shared_ptr<watch_worker>> workers_;
    std::deque<watch_event> ready_events_;
    bool stopped_ = false;
};

} // namespace

std::shared_ptr<watch_backend> make_windows_backend()
{
    return std::make_shared<windows_backend>();
}

} // namespace linuxdesktop::watch::detail

#endif
