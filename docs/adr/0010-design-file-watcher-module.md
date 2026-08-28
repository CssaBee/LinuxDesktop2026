# Design the file watcher module

The next reusable module after `ld_settings` should be a file watcher module, working name `ld_watch`.

This ADR follows the focused watcher audit, the application audit, the existing-library follow-up, and ADR 0009's shared diagnostics decision.

## Decision

Design `ld_watch` as a small migration-facing C++17 module before writing watcher code.

The module should provide:

- raw filesystem event watching,
- an opt-in settled-file helper,
- explicit overflow/lost-sync reporting,
- honest recursive-watch policies,
- file and directory watch targets,
- a portable path value for events,
- shared diagnostics from `ld_core`,
- and CMake consumption that mirrors `ld_settings`.

The implementation order is now:

1. Extract shared C++ diagnostics into `ld_core`.
2. Complete the broader watcher audit.
3. Finalize the `ld_watch` API sketch into implementation-ready header/source/test work.
4. Build a broad prototype with deterministic tests and real Linux smoke coverage.

Steps 1 and 2 are complete as of 2026-08-28. The next work item is step 3.

## Public Model

Names are provisional, but the first API should orbit these concepts:

```cpp
namespace linuxdesktop::watch {

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

enum class recursive_policy {
    no,
    yes_if_supported,
    emulate_with_subdirectory_watches
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
    std::string backend_debug_name;
};

struct watch_options {
    std::filesystem::path path;
    std::string caller_tag;
    bool watch_files = true;
    bool watch_directories = true;
    recursive_policy recursive = recursive_policy::no;
};

struct settle_options {
    std::chrono::milliseconds debounce = std::chrono::milliseconds{0};
    std::chrono::milliseconds stable_for = std::chrono::milliseconds{0};
};

struct watch_event {
    event_kind kind;
    watch_path path;
    std::optional<watch_path> old_path;
    watch_id source;
    std::string caller_tag;
    bool paired_rename = false;
    bool rescan_recommended = false;
    stream_state state = stream_state::clean;
    std::vector<linuxdesktop::diagnostic> diagnostics;
};

struct capability_report {
    bool native_recursive = false;
    bool emulated_recursive = false;
    bool overflow_reporting = true;
    bool settled_file_helper = false;
    std::vector<linuxdesktop::diagnostic> diagnostics;
};

} // namespace linuxdesktop::watch
```

The API should expose portable event kinds first. Backend masks, raw names, or platform-specific details belong in optional diagnostics/debug fields unless a later advanced API earns them.

## Layering

`ld_watch` should name three layers:

- Raw events: the core event stream, path model, watch lifecycle, and diagnostics.
- Settled-file trigger: debounce and stable size/mtime readiness for task workflows.
- Dirty-path refresh: a future convenience layer that turns noisy raw events into view-refresh signals.

The first prototype should cover raw events and settled-file trigger behavior. Dirty-path refresh should remain documented unless a tiny helper falls out naturally.

## Backend Posture

Linux should start with native `inotify`.

Windows should be shaped around `ReadDirectoryChangesW`.

libuv should be recommended for applications that already want a libuv loop and only need coarse rename/change notifications. It should remain the strongest reference and possible optional backend, but it should not be a required first dependency because its public watcher API intentionally reports only change/rename categories and brings event-loop ownership into the API.

Qt, GLib/GIO, wxWidgets, and .NET `FileSystemWatcher` should be documented as ecosystem-native options or adapter targets, not required dependencies.

efsw is the strongest future wrap candidate if the prototype shows that direct native implementation cost is not worth paying.

e-dant/watcher is the strongest dependency-minimal API/source reference and should influence warning events, associated rename paths, and Linux overflow handling.

Panoptes remains a compact C++17 reference but not a wrap candidate for rename-sensitive work because its README still treats rename/move pairing as wishlist territory.

Watchman and fswatch/libfswatch remain audit inputs for rescan posture, backend taxonomy, latency/settle, filters, and large-tree operational behavior.

## Behavioral Promises

- File and directory targets are public concepts.
- Single-file behavior may be implemented through parent-directory watches plus filtering or direct file watches, depending on backend capability.
- Recursive watching must be explicit: either native support, caller-selected emulation, or diagnostic failure.
- Overflow/lost-event conditions must produce both an event and degraded stream state.
- Rename pairing is best-effort and should only be marked paired when confidence is high.
- Debounce and settled-file readiness are separate concepts.
- Network and pseudo-filesystem reliability is not guaranteed.
- UI thread dispatch belongs in future toolkit adapters.

## Path And Diagnostics

Watcher events should not return bare strings as the normal path model. They should return an `ld_watch` path value that preserves:

- absolute path,
- optional root-relative path,
- watch/root identity,
- and backend-native debug information.

This keeps application code in LinuxDesktop2026 vocabulary while preserving enough detail for debugging Windows/Linux backend differences.

Diagnostics should use the shared C++ vocabulary from `ld_core`, introduced by ADR 0009.

## Prototype Boundary

The prototype should be broader than a tiny "print one event" demo because small watcher prototypes hide later API pressure.

It should include:

- one directory watch,
- one single-file watch,
- create/modify/remove/rename event mapping,
- debounce behavior,
- settled-file readiness,
- recursive-policy diagnostics,
- simulated overflow/rescan tests,
- real Linux `inotify` smoke tests,
- callback delivery,
- blocking pull delivery,
- and structured diagnostic output.

It should not include:

- Qt, GLib, wx, or .NET dependencies,
- Watchman service integration,
- fanotify,
- shell namespace/recycle-bin semantics,
- desktop portals,
- a full directory snapshot/diff cache,
- or a stable C ABI in the first version.

## Consequences

`ld_watch` is justified even though libuv exists because the missing value is migration-shaped policy, not native backend access. The module should help porting work by making platform differences visible and testable instead of hiding them behind a false uniform event stream.

The broader audit keeps `ld_watch` in the build column, with a deliberate escape hatch:

- recommend libuv when an application already wants a libuv loop,
- consider wrapping efsw after the prototype if it preserves capability reports and diagnostics,
- borrow e-dant/watcher's warning-event and associated-event posture,
- borrow fswatch/Watchman's overflow, settle, and recrawl language,
- and keep the first implementation native-inotify-first so the public API is shaped by LinuxDesktop2026 needs rather than by an imported event loop or daemon contract.

## Related Docs

- `docs/adr/0009-extract-shared-core-diagnostics.md`
- `docs/survey/file-watcher-audit.md`
- `docs/survey/file-watcher-application-audit.md`
- `docs/survey/file-watcher-library-audit.md`
- `docs/survey/module-priority-score.md`
