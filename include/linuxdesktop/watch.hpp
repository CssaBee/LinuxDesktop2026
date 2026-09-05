#pragma once

#include "linuxdesktop/core.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace linuxdesktop::watch {

class watcher;

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

enum class backend_kind {
    unavailable,
    inotify,
    read_directory_changes_w,
    libuv,
    simulated
};

struct watch_id {
    std::uint64_t value = 0;
};

inline bool operator==(watch_id lhs, watch_id rhs)
{
    return lhs.value == rhs.value;
}

inline bool operator!=(watch_id lhs, watch_id rhs)
{
    return !(lhs == rhs);
}

namespace diagnostic_code {
inline constexpr std::string_view backend_unavailable = "watch.backend.unavailable";
inline constexpr std::string_view backend_error = "watch.backend.error";
inline constexpr std::string_view backend_inotify = "watch.backend.inotify";
inline constexpr std::string_view backend_windows = "watch.backend.windows";
inline constexpr std::string_view backend_libuv = "watch.backend.libuv";
inline constexpr std::string_view path_not_found = "watch.path.not_found";
inline constexpr std::string_view path_unsupported_type = "watch.path.unsupported_type";
inline constexpr std::string_view path_access_denied = "watch.path.access_denied";
inline constexpr std::string_view recursive_unsupported = "watch.recursive.unsupported";
inline constexpr std::string_view recursive_native = "watch.recursive.native";
inline constexpr std::string_view recursive_emulated = "watch.recursive.emulated";
inline constexpr std::string_view recursive_symlink_skipped = "watch.recursive.symlink_skipped";
inline constexpr std::string_view recursive_duplicate_skipped = "watch.recursive.duplicate_skipped";
inline constexpr std::string_view recursive_discovered = "watch.recursive.discovered";
inline constexpr std::string_view overflow = "watch.overflow";
inline constexpr std::string_view rescan_recommended = "watch.rescan_recommended";
inline constexpr std::string_view callback_exception = "watch.callback.exception";
inline constexpr std::string_view queue_overflow = "watch.queue.overflow";
inline constexpr std::string_view resource_limit = "watch.resource.limit";
inline constexpr std::string_view rename_unpaired = "watch.rename.unpaired";
inline constexpr std::string_view settle_timeout = "watch.settle.timeout";
inline constexpr std::string_view settle_ready = "watch.settle.ready";
} // namespace diagnostic_code

struct watch_path {
    // Absolute/root-relative are std::filesystem paths so callers keep LinuxDesktop2026 path semantics
    // instead of flattening backend-native paths into strings.
    std::filesystem::path absolute;
    // Directory watches report paths relative to the watched directory. Single-file watches report
    // the watched filename for target-file events, even when a backend watches the parent directory.
    std::optional<std::filesystem::path> root_relative;
    watch_id root;
    path_type type = path_type::unknown;
    // Debug-only backend detail. Do not parse this field for routing or persistence.
    std::string backend_debug_name;
};

struct settle_options {
    std::chrono::milliseconds debounce_for = std::chrono::milliseconds{0};
    std::chrono::milliseconds stable_for = std::chrono::milliseconds{0};
    std::chrono::milliseconds poll_interval = std::chrono::milliseconds{100};
    std::optional<std::chrono::milliseconds> timeout_after;
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

// Callbacks run on the watcher delivery thread or backend-owned delivery path.
// They are not promised on a UI thread. Callbacks may call stop(), remove_watch(),
// set_callback(), or destroy the watcher facade from inside the callback.
// Destroying the facade stops the watcher and waits for worker threads other than
// the callback's current delivery thread.
// Callback exceptions are caught, mark the stream degraded, and fall back to
// queued delivery with a diagnostic error event.

struct capability_report {
    backend_kind backend = backend_kind::unavailable;
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

#if defined(LINUXDESKTOP2026_WATCH_ENABLE_TEST_HOOKS)
namespace detail {
class watch_backend;
watcher make_watcher_for_backend(std::shared_ptr<watch_backend> backend);
std::size_t queued_events_for_tests(const watcher& watcher);
std::size_t pending_settle_work_for_tests(const watcher& watcher);
} // namespace detail
#endif

class watcher {
public:
    // Pull delivery is bounded by event depth. If it falls behind far enough, the
    // watcher drops queued events, emits a degraded overflow event, and expects
    // the caller to rescan. Settled-file readiness is coalesced by distinct
    // pending (watch_id, path) keys.
    watcher();
    ~watcher();

    watcher(const watcher&) = delete;
    watcher& operator=(const watcher&) = delete;
    watcher(watcher&&) noexcept;
    watcher& operator=(watcher&&) noexcept;

    start_report add_watch(const watch_options& options);
    bool remove_watch(watch_id id);
    void stop();

    void set_callback(event_callback callback);
    std::optional<watch_event> poll();
    std::optional<watch_event> wait();
    std::optional<watch_event> wait_for(std::chrono::milliseconds timeout);

    capability_report capabilities() const;
    stream_state state() const;

private:
#if defined(LINUXDESKTOP2026_WATCH_ENABLE_TEST_HOOKS)
    friend watcher detail::make_watcher_for_backend(std::shared_ptr<detail::watch_backend> backend);
    friend std::size_t detail::queued_events_for_tests(const watcher& watcher);
    friend std::size_t detail::pending_settle_work_for_tests(const watcher& watcher);

    explicit watcher(std::shared_ptr<detail::watch_backend> backend);
#endif

    class impl;
    std::shared_ptr<impl> impl_;
};

std::string_view to_string(event_kind value);
std::string_view to_string(path_type value);
std::string_view to_string(recursive_policy value);
std::string_view to_string(overflow_policy value);
std::string_view to_string(stream_state value);
std::string_view to_string(backend_kind value);

} // namespace linuxdesktop::watch
