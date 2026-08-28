# File Watcher Library Follow-Up

This pass starts from `docs/survey/file-watcher-audit.md`. The goal is to decide whether the next module should build a small `ld_watch` facade, wrap an existing watcher, recommend ecosystem-native APIs, or defer the module.

The requirement shape from the focused audit is migration-specific:

- directory and single-file monitoring,
- create, modify, remove, and rename events,
- "save by temporary file then rename" resilience,
- recursive policy that is explicit about backend limits,
- debounce or settle helpers for "file is ready enough" workflows,
- overflow and rescan diagnostics,
- CMake-friendly consumption,
- no GUI toolkit dependency,
- and no hidden promise that network or pseudo-filesystems are reliable.

## Classification Summary

| Candidate | Classification | Why |
|---|---|---|
| Linux `inotify` | Adopt for first Linux backend | It is the natural Ubuntu implementation and exposes overflow, watch limits, rename cookies, and non-recursive behavior directly. |
| Windows `ReadDirectoryChangesW` | Adopt for shaped Windows backend | It is the native Windows contract and supports subtree watches, notify filters, and explicit lost-change/rescan cases. |
| `std::filesystem` | Adopt internally | Good path and probing primitive, but not a watcher. |
| libuv `uv_fs_event` | Recommend for libuv-shaped apps; study as optional backend | Strong portable implementation, permissive, and C-shaped, but it brings an event loop choice, coarse event kinds, and weak migration diagnostics. |
| Qt `QFileSystemWatcher` | Recommend or adapter | Excellent for Qt applications, but QObject/signals and Qt event-loop semantics are too toolkit-shaped for the neutral core. |
| GLib/GIO `GFileMonitor` | Recommend or adapter | Excellent for GTK/GLib applications, with rate limiting and main-context dispatch, but it leaks GLib ownership and event-loop concepts. |
| wxWidgets `wxFileSystemWatcher` | Recommend or adapter | Useful migration reference for wx applications; not neutral enough as a core dependency. |
| .NET `FileSystemWatcher` | Migration reference | Important expectation-setter for C# apps such as ShareX and Files; not a C++ dependency candidate. |
| Watchman | Study/defer | Strong operational model for large trees and rescan recovery, but a daemon/client service is too heavy for the first library slice. |
| efsw | Study/defer | Small C++ watcher with C API and permissive lineage; inspect source health before deciding whether to wrap or learn from it. |
| e-dant/watcher | Study/defer | Promising dependency-minimal C++ shape with CLI/C API ideas; source and API audit required before dependency decisions. |
| Panoptes | Study/defer | C++17 native-interface watcher with recursive ambitions; source and maintenance audit required before dependency decisions. |
| fswatch/libfswatch | Defer | Broad monitor coverage and command-line utility focus; likely better as a reference for polling/backend taxonomy than a first dependency. |

## Adopt

### Linux `inotify`

Use `inotify` as the first Linux implementation target for Ubuntu.

Important requirements for our module:

- expose queue overflow as an event or diagnostic that recommends rescan,
- report watch-add failures, including likely watch-count/resource limits,
- treat recursive watching as emulation by one watch per directory,
- scan newly discovered directories immediately when recursive emulation is enabled,
- keep rename matching best-effort because cookies are useful but still racy,
- coalesce duplicate events when debounce is requested,
- and provide a polling fallback as a later capability rather than a first-slice promise.

Fit:

- **Adopt** as implementation behavior for the first Linux sample.
- Do not expose inotify watch descriptors or masks in the public API.
- Keep overflow and rescan in the portable model so Windows and future backends can report equivalent loss-of-sync states.

### Windows `ReadDirectoryChangesW`

Use `ReadDirectoryChangesW` as the Windows semantic target.

Important requirements for our module:

- support directory watches and subtree watches where requested,
- map filename, directory-name, last-write, size, attributes, and security filters into a smaller portable event model,
- surface buffer overflow and `ERROR_NOTIFY_ENUM_DIR` as rescan-required diagnostics,
- report backend failures for network filesystems and unsupported redirectors,
- and preserve single-file watching as a facade behavior, likely by watching the parent directory and filtering events.

Fit:

- **Adopt** as the shaped Windows backend.
- Implement after Linux if no Windows runner is available immediately.
- Keep buffer sizing and asynchronous/threaded strategy behind the facade.

### `std::filesystem`

Use `std::filesystem` internally for path canonicalization choices, file existence checks, directory walking, and recursive-watch setup.

Fit:

- **Adopt** as an internal primitive.
- Do not let `std::filesystem` obscure platform-event uncertainty; probing the filesystem after an event is app workflow, not proof that the event stream was complete.

## Study Or Optional Backend

### libuv

libuv is the strongest existing portable watcher reference.

Useful behavior:

- wraps native file-event backends behind a C API,
- reports `UV_RENAME` and `UV_CHANGE`,
- exposes a recursive flag that is backend-dependent,
- returns status errors through callbacks,
- and is already proven inside runtimes and tooling.

Fit:

- **Recommend directly** for applications that already want a libuv event loop and only need coarse rename/change notifications.
- **Study** as the main reference and keep room for an optional backend.
- Do not adopt as the default first dependency unless we decide that depending on libuv's loop is acceptable for small desktop migrations.

Gaps for our target:

- event kinds are intentionally coarse,
- recursive support is not uniform,
- debounce, "ready enough", and application routing helpers remain app/library policy,
- and event-loop ownership is a large API decision compared with the small `ld_settings` precedent.

### .NET `FileSystemWatcher`

.NET `FileSystemWatcher` is not a dependency candidate for this C++ module, but it is an important migration reference because ShareX and Files are C# applications.

Useful behavior:

- watches a directory or files in a directory,
- can include subdirectories,
- exposes changed, created, deleted, renamed, and error events,
- and is familiar to Windows-heavy desktop apps.

Fit:

- **Migration reference** for user expectations and source audits.
- Do not wrap or depend on it in `ld_watch`.

Gaps for our target:

- tied to .NET object/event semantics,
- does not solve C++ CMake consumption,
- and still requires callers to handle timing, overflow/error, and workflow policy.

## Recommend Or Adapter

### Qt `QFileSystemWatcher`

Qt is a good answer for Qt applications.

Fit:

- **Recommend** for Qt ports.
- **Adapter** later if `ld_watch` needs to feed Qt signals or use a Qt backend.
- Avoid as a mandatory dependency for the neutral core.

Gaps for our target:

- files can stop being watched after rename/remove and may need re-adding,
- signals can collapse multiple quick changes,
- watch limits remain system-dependent,
- and Qt types/event-loop semantics would leak through a small C++ core.

### GLib/GIO `GFileMonitor`

GIO is a good answer for GTK/GLib applications.

Fit:

- **Recommend** for GTK/GLib ports.
- **Adapter** later if the app already owns a GLib main context.
- Avoid as a mandatory dependency for toolkit-neutral consumers.

Gaps for our target:

- events are delivered through the thread-default main context,
- types and cancellation semantics are GLib-shaped,
- and recursive tree policy is still something the app or library must design deliberately.

### wxWidgets `wxFileSystemWatcher`

wxWidgets is useful as a migration reference because it spans Windows, GTK, and macOS.

Fit:

- **Recommend** for wx applications.
- **Study** its recursive `AddTree()` shape and documented limitations.
- Avoid as a dependency outside wx ports.

Gaps for our target:

- events are wx events,
- reported event quality differs by platform,
- and it does not match the dependency-minimal CMake module shape.

## Study Or Defer

### Watchman

Watchman is the best reference for large-tree correctness posture.

Fit:

- **Study** for recrawl, settle, fresh-instance, and ignored-directory lessons.
- **Defer** as a dependency because it is a service with client protocol and operational configuration.

Design lessons:

- rescan/recrawl is normal recovery, not a fatal exceptional path,
- user-visible warnings are useful when events were lost,
- and settle periods belong in a higher-level layer above raw events.

### efsw

efsw is close to the desired small-library space and includes a C API wrapper.

Fit:

- **Study/defer** until we inspect source health, maintenance cadence, backend behavior, and CMake/install ergonomics.
- It may become a wrap candidate if it reports enough diagnostics and keeps dependencies light.

Open checks:

- license verification,
- recursive behavior on Linux,
- overflow/rescan reporting,
- rename pairing behavior,
- and whether the public API can preserve our capability-reporting model.

### e-dant/watcher

e-dant/watcher is another close small-library candidate with a dependency-minimal story.

Fit:

- **Study/defer** until a source audit confirms API stability, install shape, event model, and license fit.
- Its CLI JSON output and C API may be useful references for agent-readable diagnostics.

Open checks:

- source and release maturity,
- event-kind mapping across Linux and Windows,
- overflow and unavailable-backend handling,
- and whether its header-only design is desirable for this repo.

### Panoptes

Panoptes is a C++17 native-interface watcher candidate.

Fit:

- **Study/defer** until source, packaging, license, and maintenance are inspected.
- Useful as a comparison point because it uses `std::filesystem` and native APIs including `ReadDirectoryChangesW` and `inotify`.

Open checks:

- whether recursive behavior is robust on Linux,
- whether rename/move pairing is complete enough,
- whether event buffering hides backend timing in ways that matter for desktop migrations,
- and whether the project is active enough to wrap or recommend.

### fswatch/libfswatch

fswatch covers many monitor backends and has a mature command-line identity.

Fit:

- **Defer** as a dependency for the first module.
- **Study** if we need polling, backend taxonomy, or broad Unix portability lessons later.

Why not first:

- the project is broader than the first `ld_watch` slice,
- command-line utility behavior is not the same as an embedded migration library,
- and we are targeting Ubuntu plus shaped Windows behavior before broad Unix monitor coverage.

## Design Consequences

The next module should be built, but the first slice should stay small:

- finish a minimum shared diagnostics extraction in `ld_core` before `ld_watch`,
- implement a native Linux `inotify` backend first,
- shape the Windows backend around `ReadDirectoryChangesW`,
- recommend libuv for libuv-shaped applications while keeping it as the main reference and possible optional backend,
- return capability reports before or when a watch starts,
- model overflow/lost-sync as both a first-class event and degraded stream state,
- support directory and single-file watches,
- expose both blocking pull and callback delivery,
- include optional debounce and settled-file behavior as separate concepts,
- return an `ld_watch` path value rather than bare strings,
- use `std::filesystem` internally,
- and keep toolkit dispatch in future adapters.

## First Sample Boundary

The sample should include:

- one directory watch,
- one single-file watch,
- create/modify/remove/rename event mapping,
- a debounce option,
- structured text or JSON diagnostics,
- a rescan-recommended event path that can be simulated in tests,
- Linux inotify behavior,
- Windows API shape in the public model,
- shared `ld_core` diagnostics,
- an `ld_watch` path value with absolute, root-relative, watch identity, and backend-debug representations,
- CMake consumption mirroring `ld_settings`,
- and migration examples for Notepad++ live monitoring and ShareX watch-folder automation.

The sample should not include:

- Qt, GLib, or wx dependencies,
- Watchman service integration,
- fanotify,
- shell namespace/recycle-bin semantics,
- desktop portals,
- file manager refresh policy,
- UI thread dispatch,
- or a guarantee of reliable network filesystem notifications.

## Proposed Decision

Proceed to a broader audit and then an ADR/API sketch for `ld_watch`.

Working name: `ld_watch`.

Public shape: C++ convenience API first, callback-oriented and POD-friendly enough to keep a later C/Rust boundary plausible.

Implementation posture: native Linux first, shaped Windows backend next, no required event-loop dependency in the first sample.

This is enough evidence to extract shared diagnostics first, complete the broader application/library audit, and then use ADR 0010 before writing watcher code.

## Source Links

- Linux `inotify`: https://man7.org/linux/man-pages/man7/inotify.7.html
- Microsoft `ReadDirectoryChangesW`: https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-readdirectorychangesw
- libuv `uv_fs_event`: https://docs.libuv.org/en/v1.x/fs_event.html
- Qt `QFileSystemWatcher`: https://doc.qt.io/qt-6/qfilesystemwatcher.html
- GLib/GIO `GFileMonitor`: https://docs.gtk.org/gio/class.FileMonitor.html
- wxWidgets `wxFileSystemWatcher`: https://docs.wxwidgets.org/3.2/classwx_file_system_watcher.html
- .NET `FileSystemWatcher`: https://learn.microsoft.com/en-us/dotnet/api/system.io.filesystemwatcher
- Watchman troubleshooting: https://facebook.github.io/watchman/docs/troubleshooting
- efsw: https://github.com/SpartanJ/efsw
- e-dant/watcher: https://github.com/e-dant/watcher
- Panoptes: https://github.com/neXenio/panoptes
- fswatch documentation: https://emcrisostomo.github.io/fswatch/documentation.html
