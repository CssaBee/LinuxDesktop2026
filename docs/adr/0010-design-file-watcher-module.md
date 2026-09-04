# Design the file watcher module

The next reusable module after `ld_settings` should be a file watcher module, working name `ld_watch`.

This ADR follows the focused watcher audit, the application audit, the existing-library follow-up, and ADR 0009's shared diagnostics decision.

## Decision

Design `ld_watch` as a small migration-facing C++17 module and treat this ADR as the implementation boundary for the first code pass.

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

Steps 1 through 4 are complete as of 2026-08-28. The broad prototype exists and remains explicitly pre-ship code.

As of 2026-08-30, the backend ownership decision is also settled for the next
pre-1.0 phase: keep the native Linux and Windows backends as LinuxDesktop2026
owned behavior, keep libuv optional and recommendation-shaped, and keep efsw as
the strongest contingency if native backend maintenance becomes the limiting
cost.

## Public Model

The first C++ API should live in `include/linuxdesktop/watch.hpp`, use namespace `linuxdesktop::watch`, and expose version constants/functions mirroring `ld_settings`.

The first implementation should use these public concepts:

```cpp
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

using event_callback = std::function<void(const watch_event&)>;

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

class watcher {
public:
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
};

std::string_view to_string(event_kind value);
std::string_view to_string(path_type value);
std::string_view to_string(recursive_policy value);
std::string_view to_string(overflow_policy value);
std::string_view to_string(stream_state value);
std::string_view to_string(backend_kind value);

} // namespace linuxdesktop::watch
```

The API should expose portable event kinds first. Backend masks, raw names, or platform-specific details belong in optional diagnostics/debug fields unless a later advanced API earns them.

The first release should not expose a stable C ABI. `ld_watch` C ABI design is postponed until release-candidate status, after the C++ event, ownership, callback, queue, and path models survive native backend verification.

## Lifecycle And Delivery

The first `watcher` object owns backend resources and an internal event queue.

- `add_watch` starts a watch immediately and returns a `start_report`.
- A failed `add_watch` returns `ok = false`, `id.value = 0`, and diagnostics.
- `remove_watch` is idempotent and returns whether a live watch was removed.
- `stop` closes backend resources, wakes any blocking `wait`, and moves the stream to `stopped`.
- `poll` returns the next queued event or `std::nullopt` without blocking.
- `wait` blocks until an event is available or the watcher is stopped; stopped watchers return `std::nullopt`.
- `wait_for` blocks until an event is available, the timeout expires, or the watcher is stopped; timeout and stopped both return `std::nullopt`.
- `set_callback` installs process-local callback delivery; passing an empty callback returns future events to `poll`/`wait` delivery.
- Callbacks are invoked from the watcher delivery thread or caller-pumped backend thread, never promised on a UI thread.
- Callback and pull delivery are mode-switched: events delivered to a callback are not also returned from `poll`/`wait`.
- Callbacks may call `stop`, `remove_watch`, `set_callback`, or destroy the watcher facade from inside the callback. Destroying the facade stops the watcher and waits for worker threads other than the callback's current delivery thread.
- Callback exceptions are caught, degrade the stream, and fall back to queued delivery with a diagnostic error event rather than escaping the delivery thread.
- Pull delivery is bounded by event depth. If pull-mode delivery falls behind, the watcher discards queued events, emits a degraded overflow event with rescan guidance, and resumes once the queue is drained. Settled-file readiness is coalesced by distinct pending path so repeated events for one file do not allocate unbounded stale work.

The prototype can use one delivery thread on Linux. A later toolkit adapter may marshal events onto Qt, GLib, wxWidgets, .NET, or application-specific dispatchers.

## Source Layout

The first code pass should add these files:

- `include/linuxdesktop/watch.hpp`: public C++ API from this ADR.
- `src/watch.cpp`: portable watcher object, queue, lifecycle, event normalization, settle helper glue, and backend selection.
- `src/watch_inotify.cpp`: Linux `inotify` backend.
- `src/watch_backend.hpp`: private backend interface used by simulated and native implementations.
- `tests/watch_tests.cpp`: deterministic simulated-backend tests.
- `tests/watch_inotify_tests.cpp`: Linux-only smoke tests guarded by platform checks.
- `examples/watch_demo.cpp`: small directory/file watch example with structured diagnostics.

CMake should add `ld_watch`, `LinuxDesktop2026::ld_watch`, tests, example, install/export inclusion, and a package-consumer smoke path mirroring `ld_settings`.

`ld_watch` must link `LinuxDesktop2026::ld_core`. It must not link `ld_settings`.

## Layering

`ld_watch` should name three layers:

- Raw events: the core event stream, path model, watch lifecycle, and diagnostics.
- Settled-file trigger: debounce and stable size/mtime readiness for task workflows.
- Dirty-path refresh: a future convenience layer that turns noisy raw events into view-refresh signals.

The first prototype should cover raw events and settled-file trigger behavior. Dirty-path refresh should remain documented unless a tiny helper falls out naturally.

The first header should expose raw events directly. Settled-file behavior should be exposed as options and normalized events, not as a ShareX-specific upload/task abstraction. Dirty-path refresh should not enter the first public API.

## Backend Posture

Linux uses native `inotify` as the owned default backend.

Windows uses native `ReadDirectoryChangesW` as the owned default backend.

libuv should be recommended for applications that already want a libuv loop and only need coarse rename/change notifications. It is now available as an optional backend seam when CMake finds libuv, but it should not be a required dependency because its public watcher API intentionally reports only change/rename categories and brings event-loop ownership into the implementation.

Qt, GLib/GIO, wxWidgets, and .NET `FileSystemWatcher` should be documented as ecosystem-native options or adapter targets, not required dependencies.

efsw is the strongest future wrap candidate if the prototype shows that direct native implementation cost is not worth paying.

e-dant/watcher is the strongest dependency-minimal API/source reference and should influence warning events, associated rename paths, and Linux overflow handling.

Panoptes remains a compact C++17 reference but not a wrap candidate for rename-sensitive work because its README still treats rename/move pairing as wishlist territory.

Watchman and fswatch/libfswatch remain audit inputs for rescan posture, backend taxonomy, latency/settle, filters, and large-tree operational behavior.

### Native Backend Decision

The project should keep the native backends rather than immediately wrapping an
external watcher library.

Evidence:

- the public `watch_path`, `diagnostic_code`, `capability_report`, and
  `settle_options` model already contains LinuxDesktop2026-specific migration
  vocabulary that libuv does not expose directly,
- Linux tests cover deterministic queue behavior through the simulated backend
  and real `inotify` behavior for directory events, single-file save-by-replace,
  recursive emulation, symlink skips, duplicate recursive watches, and resource
  diagnostics,
- Windows has a native `ReadDirectoryChangesW` backend and a Windows-only smoke
  target for create, modify, delete, rename, recursive nested creation, and
  single-file save-by-replace,
- the optional libuv path is buildable and smoke-tested when selected, but its
  public event model is intentionally coarser than the migration-facing
  semantics this module is trying to provide,
- and wrapping efsw today would not remove the need to own LinuxDesktop2026's
  capability, overflow, settle, and path-reporting contract.

Owned behavior:

- portable event vocabulary and diagnostic codes,
- watcher-owned path values instead of backend strings,
- single-file facade semantics over whatever primitive the backend provides,
- explicit recursive-policy success or failure,
- overflow/lost-sync events with rescan guidance,
- bounded queue behavior,
- callback and pull-delivery lifecycle semantics,
- settled-file readiness and timeout behavior,
- CMake install/export consumption,
- and CI evidence that separates Linux, Windows, and optional-libuv behavior.

Delegated or recommended behavior:

- libuv remains the recommended direct choice for applications already centered
  on a libuv event loop and satisfied with coarse change/rename events,
- Qt, GLib/GIO, wxWidgets, and .NET watcher APIs remain ecosystem-native
  recommendations or future adapter targets,
- Watchman remains a reference for large-tree operational posture rather than a
  library dependency,
- and efsw remains the first serious wrap candidate if native backends start
  consuming more maintenance than the LinuxDesktop2026-specific behavior is
  worth.

Fallback trigger:

Reopen the keep-versus-wrap decision only with new evidence: repeated Windows
CI flakes caused by backend correctness, stress tests exposing queue/recursive
failure modes that would require a large private watcher framework, or a real
consumer branch showing that `ld_watch` spends more application code adapting
around native backend quirks than it removes.

The private backend interface should be narrow:

- start one root watch from `watch_options`,
- stop one root watch,
- read or push raw backend events,
- report capabilities and diagnostics,
- surface overflow/lost-sync explicitly.

Backends must not return public events with bare path strings. They should construct `watch_path` values or provide enough raw information for `src/watch.cpp` to construct them.

Windows compatibility fixes should stay at this same abstraction level. If a Windows backend, CI run, or downstream consumer exposes behavior that does not map cleanly to the current public model, prefer extending `watch_path`, diagnostics, capability reports, or documented policy over leaking `ReadDirectoryChangesW` names, drive-letter assumptions, or raw string paths into application code.

## Behavioral Promises

- File and directory targets are public concepts.
- Single-file behavior may be implemented through parent-directory watches plus filtering or direct file watches, depending on backend capability.
- Recursive watching must be explicit: either native support, caller-selected emulation, or diagnostic failure.
- Overflow/lost-event conditions must produce both an event and degraded stream state.
- Rename pairing is best-effort and should only be marked paired when confidence is high.
- Debounce and settled-file readiness are separate concepts.
- Network and pseudo-filesystem reliability is not guaranteed.
- UI thread dispatch belongs in future toolkit adapters.
- Event order is per watcher queue best-effort; cross-watch ordering is not promised.
- Duplicate and coalesced events are allowed, but overflow and lost-sync cannot be silently swallowed.
- `caller_tag` is echoed unchanged so apps can route events without adding their own side table.
- `watch_id` is process-local, monotonically assigned, and not persisted.

## Path And Diagnostics

Watcher events should not return bare strings as the normal path model. They should return an `ld_watch` path value that preserves:

- absolute path,
- optional root-relative path,
- watch/root identity,
- and backend-native debug information.

For directory watches, `root_relative` is relative to the watched directory. For single-file watches, target-file events report the watched filename as `root_relative`, even when the backend is implemented by watching the parent directory and filtering events. The public API must not expose `"."` or a backend parent-directory-relative artifact for that normal single-file case.

This keeps application code in LinuxDesktop2026 vocabulary while preserving enough detail for debugging Windows/Linux backend differences.

Diagnostics should use the shared C++ vocabulary from `ld_core`, introduced by ADR 0009.

The first diagnostic codes should include:

- `watch.backend.unavailable`
- `watch.backend.error`
- `watch.backend.inotify`
- `watch.backend.windows`
- `watch.backend.libuv`
- `watch.path.not_found`
- `watch.path.unsupported_type`
- `watch.path.access_denied`
- `watch.recursive.unsupported`
- `watch.recursive.native`
- `watch.recursive.emulated`
- `watch.recursive.symlink_skipped`
- `watch.recursive.duplicate_skipped`
- `watch.recursive.discovered`
- `watch.overflow`
- `watch.rescan_recommended`
- `watch.resource.limit`
- `watch.rename.unpaired`
- `watch.settle.timeout`
- `watch.settle.ready`

These names are public `linuxdesktop::watch::diagnostic_code` constants; consumers should compare against those constants instead of spelling raw strings.

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

## Test Checklist

Deterministic tests should use the private simulated backend and cover:

- create, modify, remove, metadata, and rename event mapping,
- paired and unpaired rename events,
- event queue `poll`, `wait`, and `wait_for` behavior,
- callback delivery ownership,
- `caller_tag` round trips,
- `watch_path` absolute/root-relative/root identity fields,
- single-file filtering over parent-directory events,
- recursive unsupported and recursive emulated diagnostics,
- overflow changing stream state to `degraded`,
- rescan recommendation when requested by `overflow_policy`,
- debounce coalescing as distinct from settled-file readiness,
- settled-file readiness after stable size/mtime,
- settled-file timeout diagnostics,
- idempotent `remove_watch`,
- and `stop` waking blocked delivery.

Linux smoke tests should create a temporary directory and cover:

- directory create/modify/remove,
- rename within one watched directory,
- single-file watch behavior across save-by-replace,
- an unsupported recursive-native request producing an honest diagnostic on `inotify`,
- emulated recursive expansion when new subdirectories are created under a watched tree,
- symlinked directories being skipped with an explicit diagnostic during recursive emulation,
- and duplicate directory watches being skipped inside one emulated recursive watch.

Smoke tests should avoid forcing a real kernel queue overflow in normal CI.

## Definition Of Done For The Prototype

The broad prototype is complete when:

- `cmake -S . -B build` succeeds,
- `cmake --build build` succeeds,
- `ctest --test-dir build --output-on-failure` succeeds on Linux,
- `git diff --check` is clean,
- a consumer can link `LinuxDesktop2026::ld_watch`,
- the example can watch both a directory and a file,
- overflow/rescan behavior is tested through the simulated backend,
- and the public docs clearly state whether Windows and optional libuv backend smoke tests have actually passed in the current environment.

## Prototype Status

The first broad prototype is implemented as of 2026-08-28.

Implemented:

- `LinuxDesktop2026::ld_watch` CMake target,
- public C++ API in `include/linuxdesktop/watch.hpp`,
- private backend interface in `src/watch_backend.hpp`,
- watcher lifecycle, event queue, callback delivery, blocking pull delivery, and state tracking in `src/watch.cpp`,
- native Linux `inotify` backend in `src/watch_inotify.cpp`,
- native Windows `ReadDirectoryChangesW` backend in `src/watch_windows.cpp`,
- optional libuv backend in `src/watch_libuv.cpp`, compiled only when libuv is available through `pkg-config`,
- simulated-backend tests in `tests/watch_tests.cpp`,
- Linux `inotify` smoke tests in `tests/watch_inotify_tests.cpp`,
- Windows `ReadDirectoryChangesW` smoke tests in `tests/watch_windows_tests.cpp`, built and run only on Windows,
- preferred-libuv smoke tests in `tests/watch_libuv_tests.cpp`, built and run only when libuv is available and selected,
- install/export consumer coverage for `LinuxDesktop2026::ld_watch`,
- and `ld_watch_demo` in `examples/watch_demo.cpp`.

Still prototype-grade:

- Windows is implemented behind the native backend seam, has a Windows-only smoke test target covering create, modify, delete, rename, recursive nested creation, and single-file save-by-replace, and must stay green in CI before the backend is treated as verified.
- The optional libuv backend has a preferred-backend smoke test and CI job when libuv is available and `LD2026_WATCH_PREFER_LIBUV=ON`; it has passed on this Ubuntu host with libuv 1.48.0. It is intentionally not the default Ubuntu backend while native inotify exposes richer Linux behavior.
- Recursive emulation now expands dynamically when new subdirectories and deeper trees appear, skips duplicate directory watches inside one logical recursive watch, fans out shared native descriptor events to multiple logical watches, reports skipped symlinked directories, preserves remove/rename churn, and maps common inotify resource-limit failures into `watch.resource.limit` diagnostics.
- Settled-file support now runs outside the raw backend delivery path, coalesces repeated events by source/path, reports `watch.settle.timeout` when an optional timeout expires, cancels pending settled events when a watch is removed, and has larger-batch test coverage.
- The deterministic test hook is private/internal through `src/watch_backend.hpp`; it is no longer exposed as a public `watcher` constructor in the installed header.
- Public API stabilization has started: `capability_report` identifies `backend_kind`, diagnostic code names are public constants, `watch_id` has equality operators, `watch_path` keeps path values instead of bare strings, pull delivery has timeout-capable `wait_for`, and settled-file readiness has optional timeout policy.
- Remaining public API stabilization work is verification-first: keep the explicit Windows CI/local watcher run green, then decide whether to add a C ABI, finalize wording for root-relative path behavior on Windows single-file watches, and grow capability limit/cost fields only if stress tests prove they are needed.

## Consequences

`ld_watch` is justified even though libuv exists because the missing value is migration-shaped policy, not native backend access. The module should help porting work by making platform differences visible and testable instead of hiding them behind a false uniform event stream.

The broader audit keeps `ld_watch` in the build column, with a deliberate escape hatch:

- recommend libuv when an application already wants a libuv loop,
- consider wrapping efsw after the prototype if it preserves capability reports and diagnostics,
- borrow e-dant/watcher's warning-event and associated-event posture,
- borrow fswatch/Watchman's overflow, settle, and recrawl language,
- and keep the first implementation native-inotify-first so the public API is shaped by LinuxDesktop2026 needs rather than by an imported event loop or daemon contract.

## Review Update

The watcher prototype is useful, but the native-recursive implementation is one of the project's highest-risk surfaces. Recursive watching should remain documented as hint-based and rescan-oriented, not as a complete ordered change log.

Before `ld_watch` can move from prototype to ship candidate:

- Add stress tests for rename storms, deep recursive creation, remove/recreate churn, queue pressure, and callback lifecycle edge cases.
- Add user-space queue limits and overflow/rescan semantics.
- Keep callback delivery process-local and honest about thread ownership; if a later toolkit adapter becomes the preferred integration point, let it live above `ld_watch` instead of weakening the public contract.
- Keep libuv and efsw as serious fallback/wrap candidates if native backend maintenance cost starts dominating feature work. The decision should be reopened from CI, stress-test, or maintained-consumer evidence, not from general fear that file watching is hard.

## Related Docs

- `docs/adr/0009-extract-shared-core-diagnostics.md`
- `docs/survey/file-watcher-audit.md`
- `docs/survey/file-watcher-application-audit.md`
- `docs/survey/file-watcher-library-audit.md`
- `docs/survey/module-priority-score.md`
