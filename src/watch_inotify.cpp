#include "watch_backend.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <deque>
#include <fstream>
#include <map>
#include <mutex>
#include <optional>
#include <system_error>
#include <utility>
#include <vector>

#if defined(__linux__)
#include <poll.h>
#include <sys/inotify.h>
#include <unistd.h>
#endif

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

std::optional<std::string> read_text_file(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input) {
        return std::nullopt;
    }
    std::string value;
    std::getline(input, value);
    if (value.empty()) {
        return std::nullopt;
    }
    return value;
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

std::filesystem::path absolute_path(const std::filesystem::path& path)
{
    std::error_code ec;
    auto absolute = std::filesystem::absolute(path, ec);
    if (!ec) {
        return absolute;
    }
    return path;
}

#if defined(__linux__)

class inotify_backend final : public watch_backend {
public:
    inotify_backend()
        : fd_(inotify_init1(IN_NONBLOCK | IN_CLOEXEC))
    {
    }

    ~inotify_backend() override
    {
        stop();
    }

    start_report add_watch(watch_id id, const watch_options& options) override
    {
        std::lock_guard<std::mutex> lock(mutex_);

        start_report report;
        report.id = id;
        report.capabilities = capabilities_locked();

        if (fd_ < 0 || stopped_) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "watch.backend.unavailable",
                "inotify is unavailable",
                options.path));
            return report;
        }

        const auto absolute = weakly_absolute_path(options.path);
        std::error_code ec;
        if (!std::filesystem::exists(absolute, ec)) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "watch.path.not_found",
                "Watch path does not exist",
                absolute));
            return report;
        }

        const auto type = classify_existing_path(absolute);
        if (type == path_type::other || type == path_type::unknown) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "watch.path.unsupported_type",
                "Watch path is not a regular file or directory",
                absolute));
            return report;
        }

        if (options.recursive == recursive_policy::native_if_supported) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "watch.recursive.unsupported",
                "inotify does not provide one native recursive watch",
                absolute));
            return report;
        }

        if (options.recursive == recursive_policy::emulate) {
            report.capabilities.emulated_recursive = true;
            report.diagnostics.push_back(make_diagnostic(
                severity::warning,
                "watch.recursive.emulated",
                "Recursive watching is emulated with one inotify watch per directory",
                absolute));
        }

        const auto watch_root = type == path_type::file ? absolute.parent_path() : absolute;
        const std::optional<std::filesystem::path> file_filter =
            type == path_type::file ? std::optional<std::filesystem::path>(absolute.filename()) : std::nullopt;

        if (options.recursive == recursive_policy::emulate && type == path_type::directory) {
            add_tree_locked(id, absolute, options, report);
        } else {
            add_one_locked(id, watch_root, absolute, file_filter, type, options, report);
        }

        report.ok = has_live_watch_locked(id);
        if (!report.ok && report.diagnostics.empty()) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "watch.backend.error",
                "Failed to create inotify watch",
                absolute));
        }
        return report;
    }

    bool remove_watch(watch_id id) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto range = watches_by_id_.equal_range(id.value);
        if (range.first == range.second) {
            return false;
        }
        for (auto it = range.first; it != range.second; ++it) {
            auto by_wd = watch_by_wd_.find(it->second.wd);
            if (by_wd != watch_by_wd_.end()) {
                by_wd->second.erase(
                    std::remove_if(by_wd->second.begin(), by_wd->second.end(), [id](const auto& record) {
                        return record.id.value == id.value;
                    }),
                    by_wd->second.end());
                if (!by_wd->second.empty()) {
                    continue;
                }
                watch_by_wd_.erase(by_wd);
            }
            inotify_rm_watch(fd_, it->second.wd);
        }
        watches_by_id_.erase(id.value);
        pending_moves_.clear();
        ready_events_.clear();
        return true;
    }

    void stop() override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopped_) {
            return;
        }
        stopped_ = true;
        if (fd_ >= 0) {
            close(fd_);
            fd_ = -1;
        }
        watch_by_wd_.clear();
        watches_by_id_.clear();
        pending_moves_.clear();
        ready_events_.clear();
    }

    std::optional<watch_event> wait_event() override
    {
        for (;;) {
            int fd = -1;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (!ready_events_.empty()) {
                    auto event = std::move(ready_events_.front());
                    ready_events_.pop_front();
                    return event;
                }
                if (stopped_) {
                    return std::nullopt;
                }
                fd = fd_;
            }

            pollfd item{};
            item.fd = fd;
            item.events = POLLIN;
            const int poll_result = poll(&item, 1, 100);
            if (poll_result == 0) {
                continue;
            }
            if (poll_result < 0) {
                if (errno == EINTR) {
                    continue;
                }
                if (is_stopped()) {
                    return std::nullopt;
                }
                return error_event("watch.backend.error", std::strerror(errno));
            }
            if (item.revents & POLLNVAL) {
                if (is_stopped()) {
                    return std::nullopt;
                }
                return error_event("watch.backend.error", "inotify descriptor is invalid");
            }

            std::vector<char> buffer(16 * 1024);
            const auto bytes = read(fd, buffer.data(), buffer.size());
            if (bytes < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                    continue;
                }
                if (is_stopped()) {
                    return std::nullopt;
                }
                return error_event("watch.backend.error", std::strerror(errno));
            }

            std::optional<watch_event> next;
            std::size_t offset = 0;
            while (offset < static_cast<std::size_t>(bytes)) {
                const auto* raw = reinterpret_cast<const inotify_event*>(buffer.data() + offset);
                auto mapped = map_event(*raw);
                offset += sizeof(inotify_event) + raw->len;
                for (auto& event : mapped) {
                    if (!next.has_value()) {
                        next = std::move(event);
                    } else {
                        std::lock_guard<std::mutex> lock(mutex_);
                        ready_events_.push_back(std::move(event));
                    }
                }
            }
            if (next.has_value()) {
                return next;
            }
        }
    }

    capability_report capabilities() const override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return capabilities_locked();
    }

private:
    struct watch_record {
        int wd = -1;
        watch_id id;
        std::filesystem::path root_absolute;
        std::filesystem::path watched_absolute;
        std::optional<std::filesystem::path> file_filter;
        path_type target_type = path_type::unknown;
        std::string caller_tag;
        overflow_policy overflow = overflow_policy::request_rescan;
        bool watch_files = true;
        bool watch_directories = true;
        bool recursive_emulated = false;
    };

    capability_report capabilities_locked() const
    {
        capability_report report;
        report.native_recursive = false;
        report.emulated_recursive = true;
        report.overflow_reporting = true;
        report.settled_file_helper = false;
        if (fd_ < 0) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "watch.backend.unavailable",
                "inotify is unavailable"));
        }
        return report;
    }

    bool is_stopped() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return stopped_;
    }

    bool has_live_watch_locked(watch_id id) const
    {
        return watches_by_id_.find(id.value) != watches_by_id_.end();
    }

    bool has_watch_root_locked(watch_id id, const std::filesystem::path& watch_root) const
    {
        const auto range = watches_by_id_.equal_range(id.value);
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second.root_absolute == watch_root) {
                return true;
            }
        }
        return false;
    }

    diagnostic inotify_add_watch_diagnostic(int error, const std::filesystem::path& watch_root) const
    {
        auto code = std::string{"watch.backend.error"};
        auto message = std::string{std::strerror(error)};
        if (error == ENOSPC) {
            code = "watch.resource.limit";
            message = "inotify watch limit reached";
            const auto max_watches = read_text_file("/proc/sys/fs/inotify/max_user_watches");
            if (max_watches.has_value()) {
                message += " (max_user_watches=" + *max_watches + ")";
            }
        } else if (error == EMFILE || error == ENFILE) {
            code = "watch.resource.limit";
            message = "inotify file descriptor limit reached";
            const auto max_instances = read_text_file("/proc/sys/fs/inotify/max_user_instances");
            if (max_instances.has_value()) {
                message += " (max_user_instances=" + *max_instances + ")";
            }
        } else if (error == EACCES) {
            code = "watch.path.access_denied";
            message = "Permission denied while creating inotify watch";
        }

        return make_diagnostic(severity::error, std::move(code), std::move(message), watch_root);
    }

    void add_tree_locked(
        watch_id id,
        const std::filesystem::path& root,
        const watch_options& options,
        start_report& report)
    {
        std::error_code ec;
        add_one_locked(id, root, root, std::nullopt, path_type::directory, options, report);
        for (std::filesystem::recursive_directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec)) {
            std::error_code entry_ec;
            if (it->is_symlink(entry_ec)) {
                const auto absolute = absolute_path(it->path());
                if (it->is_directory(entry_ec)) {
                    it.disable_recursion_pending();
                }
                report.diagnostics.push_back(make_diagnostic(
                    severity::warning,
                    "watch.recursive.symlink_skipped",
                    "Recursive emulation does not follow symlinked directories",
                    absolute));
                continue;
            }
            if (it->is_directory(entry_ec)) {
                const auto absolute = weakly_absolute_path(it->path());
                add_one_locked(id, absolute, root, std::nullopt, path_type::directory, options, report);
            }
        }
        if (ec) {
            report.diagnostics.push_back(make_diagnostic(
                severity::warning,
                "watch.backend.error",
                ec.message(),
                root));
        }
    }

    void add_one_locked(
        watch_id id,
        const std::filesystem::path& watch_root,
        const std::filesystem::path& watched_absolute,
        std::optional<std::filesystem::path> file_filter,
        path_type target_type,
        const watch_options& options,
        start_report& report)
    {
        constexpr std::uint32_t mask = IN_CREATE | IN_MODIFY | IN_DELETE | IN_ATTRIB | IN_MOVED_FROM |
            IN_MOVED_TO | IN_DELETE_SELF | IN_MOVE_SELF | IN_CLOSE_WRITE;
        const auto normalized_root = weakly_absolute_path(watch_root);
        if (has_watch_root_locked(id, normalized_root)) {
            report.diagnostics.push_back(make_diagnostic(
                severity::info,
                "watch.recursive.duplicate_skipped",
                "Recursive emulation skipped an already watched directory",
                normalized_root));
            return;
        }

        const int wd = inotify_add_watch(fd_, normalized_root.c_str(), mask);
        if (wd < 0) {
            report.diagnostics.push_back(inotify_add_watch_diagnostic(errno, normalized_root));
            return;
        }

        watch_record record;
        record.wd = wd;
        record.id = id;
        record.root_absolute = normalized_root;
        record.watched_absolute = watched_absolute;
        record.file_filter = std::move(file_filter);
        record.target_type = target_type;
        record.caller_tag = options.caller_tag;
        record.overflow = options.overflow;
        record.watch_files = options.watch_files;
        record.watch_directories = options.watch_directories;
        record.recursive_emulated = options.recursive == recursive_policy::emulate && target_type == path_type::directory;

        watch_by_wd_[wd].push_back(record);
        watches_by_id_.insert({id.value, record});
    }

    std::vector<watch_event> map_event(const inotify_event& raw)
    {
        std::vector<watch_record> records;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (raw.mask & IN_Q_OVERFLOW) {
                return {overflow_event_locked()};
            }
            const auto it = watch_by_wd_.find(raw.wd);
            if (it == watch_by_wd_.end()) {
                return {};
            }
            records = it->second;
        }

        std::vector<watch_event> events;
        events.reserve(records.size());
        for (const auto& record : records) {
            auto event = map_event_for_record(record, raw);
            if (event.has_value()) {
                events.push_back(std::move(*event));
            }
        }
        return events;
    }

    std::optional<watch_event> map_event_for_record(const watch_record& record, const inotify_event& raw)
    {
        const std::filesystem::path name = raw.len > 0 ? std::filesystem::path(raw.name) : std::filesystem::path{};
        if (record.file_filter.has_value() && name != *record.file_filter) {
            return std::nullopt;
        }

        const bool is_directory = (raw.mask & IN_ISDIR) != 0;
        if (is_directory && !record.watch_directories) {
            return std::nullopt;
        }
        if (!is_directory && !record.watch_files) {
            return std::nullopt;
        }

        auto kind = event_kind::metadata;
        if (raw.mask & IN_CREATE) {
            kind = event_kind::created;
        } else if (raw.mask & (IN_MODIFY | IN_CLOSE_WRITE)) {
            kind = event_kind::modified;
        } else if (raw.mask & (IN_DELETE | IN_DELETE_SELF)) {
            kind = event_kind::removed;
        } else if (raw.mask & IN_MOVED_FROM) {
            kind = event_kind::renamed_old;
        } else if (raw.mask & IN_MOVED_TO) {
            kind = event_kind::renamed_new;
        }

        const auto absolute = name.empty() ? record.watched_absolute : record.root_absolute / name;
        watch_event event;
        event.kind = kind;
        event.path = make_path(record, absolute, is_directory ? path_type::directory : path_type::file);
        event.source = record.id;
        event.caller_tag = record.caller_tag;

        if (raw.mask & IN_MOVED_FROM) {
            std::lock_guard<std::mutex> lock(mutex_);
            pending_moves_[{record.id.value, raw.cookie}] = event.path;
        } else if ((raw.mask & IN_MOVED_TO) && raw.cookie != 0) {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto pending = pending_moves_.find({record.id.value, raw.cookie});
            if (pending != pending_moves_.end()) {
                event.old_path = pending->second;
                event.paired_rename = true;
                pending_moves_.erase(pending);
            } else {
                event.diagnostics.push_back(make_diagnostic(
                    severity::warning,
                    "watch.rename.unpaired",
                    "Rename destination did not have a matching source",
                    event.path.absolute));
            }
        }

        if (record.recursive_emulated && is_directory &&
            (kind == event_kind::created || kind == event_kind::renamed_new)) {
            add_discovered_tree(record, absolute);
        }

        return event;
    }

    void add_discovered_tree(const watch_record& parent, const std::filesystem::path& root)
    {
        start_report report;
        report.id = parent.id;

        watch_options options;
        options.path = root;
        options.caller_tag = parent.caller_tag;
        options.watch_files = parent.watch_files;
        options.watch_directories = parent.watch_directories;
        options.recursive = recursive_policy::emulate;
        options.overflow = parent.overflow;

        std::error_code exists_ec;
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopped_ || !std::filesystem::is_directory(root, exists_ec)) {
            return;
        }
        std::error_code symlink_ec;
        if (std::filesystem::is_symlink(std::filesystem::symlink_status(root, symlink_ec))) {
            ready_events_.push_back(diagnostic_event(parent, make_diagnostic(
                severity::warning,
                "watch.recursive.symlink_skipped",
                "Recursive emulation does not follow symlinked directories",
                root)));
            return;
        }
        add_one_locked(parent.id, root, parent.watched_absolute, std::nullopt, path_type::directory, options, report);

        std::error_code ec;
        for (std::filesystem::recursive_directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec)) {
            std::error_code entry_ec;
            if (it->is_symlink(entry_ec)) {
                const auto absolute = absolute_path(it->path());
                if (it->is_directory(entry_ec)) {
                    it.disable_recursion_pending();
                }
                ready_events_.push_back(diagnostic_event(parent, make_diagnostic(
                    severity::warning,
                    "watch.recursive.symlink_skipped",
                    "Recursive emulation does not follow symlinked directories",
                    absolute)));
                continue;
            }
            const auto absolute = weakly_absolute_path(it->path());
            const bool is_directory = it->is_directory(entry_ec);
            const bool is_regular_file = !is_directory && it->is_regular_file(entry_ec);
            if (is_directory) {
                add_one_locked(parent.id, absolute, parent.watched_absolute, std::nullopt, path_type::directory, options, report);
            }
            if (parent.watch_directories && is_directory) {
                ready_events_.push_back(synthetic_event(parent, event_kind::created, absolute, path_type::directory));
            } else if (parent.watch_files && is_regular_file) {
                ready_events_.push_back(synthetic_event(parent, event_kind::created, absolute, path_type::file));
            }
        }
        for (auto& diagnostic : report.diagnostics) {
            ready_events_.push_back(diagnostic_event(parent, std::move(diagnostic)));
        }
    }

    watch_event synthetic_event(
        const watch_record& record,
        event_kind kind,
        const std::filesystem::path& absolute,
        path_type type) const
    {
        watch_event event;
        event.kind = kind;
        event.path = make_path(record, absolute, type);
        event.source = record.id;
        event.caller_tag = record.caller_tag;
        event.diagnostics.push_back(make_diagnostic(
            severity::info,
            "watch.recursive.discovered",
            "Discovered path while expanding an emulated recursive watch",
            absolute));
        return event;
    }

    watch_event diagnostic_event(const watch_record& record, diagnostic item) const
    {
        watch_event event;
        event.kind = event_kind::error;
        event.path = make_path(record, record.watched_absolute, path_type::directory);
        event.source = record.id;
        event.caller_tag = record.caller_tag;
        event.state = item.level == severity::error ? stream_state::degraded : stream_state::clean;
        event.rescan_recommended = item.level == severity::error;
        event.diagnostics.push_back(std::move(item));
        return event;
    }

    watch_path make_path(const watch_record& record, const std::filesystem::path& absolute, path_type type) const
    {
        watch_path path;
        path.absolute = absolute;
        path.root = record.id;
        path.type = type;
        path.backend_debug_name = "inotify:" + absolute.string();

        std::error_code ec;
        const auto relative = std::filesystem::relative(absolute, record.watched_absolute, ec);
        if (!ec && !relative.empty() && relative.native().find("..") != 0) {
            path.root_relative = relative;
        }
        return path;
    }

    watch_event overflow_event_locked()
    {
        watch_event event;
        event.kind = event_kind::overflow;
        event.state = stream_state::degraded;
        event.rescan_recommended = true;
        event.diagnostics.push_back(make_diagnostic(
            severity::error,
            "watch.overflow",
            "inotify reported event queue overflow"));
        event.diagnostics.push_back(make_diagnostic(
            severity::warning,
            "watch.rescan_recommended",
            "Rescan watched roots before trusting further events"));
        return event;
    }

    watch_event error_event(std::string code, std::string message)
    {
        watch_event event;
        event.kind = event_kind::error;
        event.state = stream_state::degraded;
        event.rescan_recommended = true;
        event.diagnostics.push_back(make_diagnostic(severity::error, std::move(code), std::move(message)));
        return event;
    }

    mutable std::mutex mutex_;
    int fd_ = -1;
    bool stopped_ = false;
    std::map<int, std::vector<watch_record>> watch_by_wd_;
    std::multimap<std::uint64_t, watch_record> watches_by_id_;
    std::map<std::pair<std::uint64_t, std::uint32_t>, watch_path> pending_moves_;
    std::deque<watch_event> ready_events_;
};

#else

class unavailable_backend final : public watch_backend {
public:
    start_report add_watch(watch_id id, const watch_options& options) override
    {
        start_report report;
        report.id = id;
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "watch.backend.unavailable",
            "No native watcher backend is available in this prototype",
            options.path));
        return report;
    }

    bool remove_watch(watch_id) override
    {
        return false;
    }

    void stop() override
    {
        stopped_ = true;
    }

    std::optional<watch_event> wait_event() override
    {
        return std::nullopt;
    }

    capability_report capabilities() const override
    {
        capability_report report;
        report.overflow_reporting = false;
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "watch.backend.unavailable",
            "No native watcher backend is available in this prototype"));
        return report;
    }

private:
    bool stopped_ = false;
};

#endif

} // namespace

std::shared_ptr<watch_backend> make_native_backend()
{
#if defined(LINUXDESKTOP2026_WATCH_HAS_LIBUV) && defined(LINUXDESKTOP2026_WATCH_PREFER_LIBUV)
    return make_libuv_backend();
#elif defined(__linux__)
    return std::make_shared<inotify_backend>();
#elif defined(LINUXDESKTOP2026_WATCH_HAS_LIBUV)
    return make_libuv_backend();
#else
    return std::make_shared<unavailable_backend>();
#endif
}

} // namespace linuxdesktop::watch::detail
