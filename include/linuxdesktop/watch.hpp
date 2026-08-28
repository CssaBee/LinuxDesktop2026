#pragma once

#include "linuxdesktop/core.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace linuxdesktop::watch {

inline constexpr int version_major = 0;
inline constexpr int version_minor = 1;
inline constexpr int version_patch = 0;

enum class event_kind {
    created,
    modified,
    removed,
    renamed_old,
    renamed_new,
    metadata,
    overflow,
    error
};

enum class path_type {
    file,
    directory,
    other,
    unknown
};

enum class recursive_policy {
    none,
    native_if_supported,
    emulate
};

enum class overflow_policy {
    report_only,
    request_rescan
};

enum class stream_state {
    clean,
    degraded,
    stopped
};

struct watch_id {
    std::uint64_t value = 0;
};

struct watch_path {
    std::filesystem::path absolute;
    std::optional<std::filesystem::path> root_relative;
    watch_id root;
    path_type type = path_type::unknown;
    std::string backend_debug_name;
};

struct settle_options {
    std::chrono::milliseconds debounce_for = std::chrono::milliseconds{0};
    std::chrono::milliseconds stable_for = std::chrono::milliseconds{0};
    std::chrono::milliseconds poll_interval = std::chrono::milliseconds{100};
};

struct watch_options {
    std::filesystem::path path;
    std::string caller_tag;
    bool watch_files = true;
    bool watch_directories = true;
    recursive_policy recursive = recursive_policy::none;
    overflow_policy overflow = overflow_policy::request_rescan;
    std::optional<settle_options> settle;
};

struct watch_event {
    event_kind kind = event_kind::error;
    watch_path path;
    std::optional<watch_path> old_path;
    watch_id source;
    std::string caller_tag;
    bool paired_rename = false;
    bool rescan_recommended = false;
    stream_state state = stream_state::clean;
    std::vector<linuxdesktop::diagnostic> diagnostics;
};

using event_callback = std::function<void(const watch_event&)>;

struct capability_report {
    bool native_recursive = false;
    bool emulated_recursive = false;
    bool overflow_reporting = true;
    bool settled_file_helper = false;
    std::vector<linuxdesktop::diagnostic> diagnostics;
};

struct start_report {
    bool ok = false;
    watch_id id;
    capability_report capabilities;
    std::vector<linuxdesktop::diagnostic> diagnostics;
};

namespace detail {
class watch_backend;
} // namespace detail

class watcher {
public:
    watcher();
    ~watcher();

    watcher(const watcher&) = delete;
    watcher& operator=(const watcher&) = delete;
    watcher(watcher&&) noexcept;
    watcher& operator=(watcher&&) noexcept;

    explicit watcher(std::shared_ptr<detail::watch_backend> backend);

    start_report add_watch(const watch_options& options);
    bool remove_watch(watch_id id);
    void stop();

    void set_callback(event_callback callback);
    std::optional<watch_event> poll();
    std::optional<watch_event> wait();

    capability_report capabilities() const;
    stream_state state() const;

private:
    class impl;
    std::unique_ptr<impl> impl_;
};

std::string_view to_string(event_kind value);
std::string_view to_string(path_type value);
std::string_view to_string(recursive_policy value);
std::string_view to_string(overflow_policy value);
std::string_view to_string(stream_state value);

} // namespace linuxdesktop::watch
