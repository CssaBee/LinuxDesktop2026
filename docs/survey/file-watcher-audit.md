# File Watcher Focused Audit

This pass follows the first `ld_settings` milestone. The goal is to decide whether file watching should become the next implementation module, and if so what the first slice should actually promise.

## Sources Checked

Application requirements:

- Notepad++ live monitoring and file-status behavior: `PowerEditor/src/NppIO.cpp`, especially `Notepad_plus::monitorFileOnChange()`.
- ShareX watch-folder startup and task routing: `ShareX/Forms/MainForm.cs`, `ShareX/WatchFolderManager.cs`, `ShareX/TaskSettings.cs`.
- Files recycle-bin watcher signal from current build/source archive references: `src/Files.App.Storage/Legacy/RecycleBinWatcher.cs`.

Reference implementations and platform docs:

- Microsoft `ReadDirectoryChangesW`: https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-readdirectorychangesw
- Linux `inotify(7)`: https://www.man7.org/linux/man-pages/man7/inotify.7.html
- libuv `uv_fs_event`: `include/uv.h`, `src/win/fs-event.c`, `src/unix/linux.c`.
- Qt `QFileSystemWatcher`: https://doc.qt.io/qt-6/qfilesystemwatcher.html

## Application Requirement Shapes

### Notepad++: Open-File Monitoring

Notepad++ watches the parent directory of an open file, filters events back to the active file, and combines directory notifications with a file timestamp/size checker.

Observed behavior in source:

- `monitorFileOnChange()` derives the containing folder from the current buffer path.
- It watches last-write, filename, and size changes.
- It asks `CReadDirectoryChanges` to watch the folder recursively.
- It also registers `CReadFileChanges` against the specific file and polls it on timeout.
- Modified events post an internal reload-and-scroll message.
- Removed or renamed-away events post a stop-monitoring message.
- Frequent modified events are rate-limited before reloading.

Requirement implied:

- A portable watcher facade needs both directory events and single-file watch semantics.
- The app still owns UI policy: reload silently, prompt, stop monitoring, scroll to end, or ignore.
- The library should expose event kind, path, overflow/lost-event signals, and enough diagnostics for the app to decide whether to rescan.
- "Save by temp file then rename" must be treated as a normal editor workflow, not as an edge case.

Source anchor:

- https://raw.githubusercontent.com/notepad-plus-plus/notepad-plus-plus/master/PowerEditor/src/NppIO.cpp

### ShareX: Watch Folder As Task Trigger

ShareX uses watch folders as a user-facing automation trigger: new files in configured folders become upload tasks.

Observed behavior in source/search results:

- `MainForm.InitHotkeys()` creates `WatchFolderManager`, updates watch folders, and logs startup.
- `TaskSettings` contains `WatchFolderEnabled` and a list of `WatchFolderSettings`.
- `WatchFolderManager` collects watch folders from default task settings and per-hotkey task settings.
- When triggered, the manager may move the file to the screenshots folder before uploading it.
- Public README/user-facing docs describe "Watch folder" as a configured way to upload newly created files.
- Issues/logs show that the watch-folder manager is part of normal startup and can conflict with screenshot/recording output paths.

Requirement implied:

- A useful library should support "created file is ready enough to process" workflows, not just raw kernel events.
- It should offer debounce/stabilization helpers, because upload/copy producers often generate multiple writes.
- It should support per-watch user data or opaque IDs so an app can route events back to the configured task.
- It should surface backend errors without killing the whole app.

Source anchors:

- https://raw.githubusercontent.com/ShareX/ShareX/develop/ShareX/Forms/MainForm.cs
- https://raw.githubusercontent.com/ShareX/ShareX/develop/ShareX/WatchFolderManager.cs
- https://raw.githubusercontent.com/ShareX/ShareX/develop/ShareX/TaskSettings.cs
- https://github.com/Upload/ShareX/blob/master/README.md
- https://github.com/ShareX/ShareX/issues/1867

### Files: File-Manager Refresh And Recycle Bin Watching

Files has watcher evidence in its storage layer and build output. The current audit has weaker source detail than Notepad++ and ShareX, but still supports the requirement category.

Observed behavior:

- The project ships `src/Files.App.Storage/Legacy/RecycleBinWatcher.cs`.
- Current CI/build output references `RecycleBinWatcher.ItemChanged`.
- The first source survey found `FileSystemWatcher` in Files storage/view-model areas.

Requirement implied:

- File-manager style watchers often want directory refresh signals rather than every raw event.
- The module should distinguish low-level event streams from "dirty path, refresh this view" convenience events.
- Recycle-bin and shell namespace watching may become Windows-specific or app-specific; do not over-generalize it in the first slice.

Source anchors:

- https://github.com/files-community/Files/actions/runs/32428944902
- https://fossies.org/windows/misc/Files-4.2.3.zip/index_dc.html

## Platform Semantics That Matter

### Windows

`ReadDirectoryChangesW` can watch a subtree, report multiple notify filters, operate synchronously or asynchronously, and report buffer/lost-change cases that require enumeration/rebuild.

Important design implications:

- Recursive watching exists on Windows as a backend feature.
- Buffer sizing and network filesystems matter.
- The app must be told when events were lost and a rescan is required.
- The function reports directory changes, not every higher-level "file is now stable" workflow.

### Linux

`inotify` is the natural first Linux backend, but it is not recursive. Recursive tree watches require one watch per directory, plus immediate scanning when new directories appear.

Important design implications:

- Recursive watch support must be a capability, not a silent promise.
- Watch count limits are real and should be reported.
- Queue overflow is real and means events were lost.
- Network and pseudo-filesystems may not behave like local filesystems; polling fallback should be considered.
- Rename matching is racy even though cookies help.
- Events can be coalesced, so event counts are not reliable.

### Existing Libraries

libuv is the strongest dependency/reference candidate:

- Windows backend uses `ReadDirectoryChangesW` with IOCP and maps add/remove/rename to `UV_RENAME`, modifications to `UV_CHANGE`.
- Linux backend uses `inotify_add_watch` and maps multiple inotify masks to `UV_CHANGE` or `UV_RENAME`.
- Public flags include recursive watch, but the recursive flag is backend-dependent.

Qt `QFileSystemWatcher` is a strong reference but too toolkit-shaped for our neutral core:

- It reports file and directory changed signals.
- It documents system-dependent watch limits.
- It warns that multiple changes may collapse into fewer signals.
- It tells apps to re-add a file watch after save-by-replace behavior.

## First Module Candidate Shape

Working name: `ld_watch`.

First slice should be lower-level than ShareX automation, but higher-level than raw inotify:

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

struct watch_options {
    std::filesystem::path path;
    bool watch_files = true;
    bool watch_directories = true;
    recursive_policy recursive = recursive_policy::no;
    std::chrono::milliseconds debounce = std::chrono::milliseconds{0};
};

struct watch_event {
    event_kind kind;
    std::filesystem::path path;
    std::optional<std::filesystem::path> old_path;
    bool rescan_recommended = false;
};

struct capability_report {
    bool native_recursive = false;
    bool emulated_recursive = false;
    bool polling_fallback = false;
    bool overflow_reporting = true;
    std::vector<settings::diagnostic> diagnostics;
};

} // namespace linuxdesktop::watch
```

First executable proof should demonstrate:

- one directory watch,
- one single-file watch,
- create/modify/remove/rename event mapping,
- debounce for "file is ready enough" workflows,
- overflow/rescan diagnostics as an API concept even if hard to trigger in tests,
- Linux inotify backend first,
- Windows `ReadDirectoryChangesW` backend shaped in the API,
- no GUI toolkit dependency,
- no promise that network filesystem events are reliable.

## Build, Wrap, Or Recommend?

Recommendation for now: **build a tiny C++ facade, with libuv as the main reference and possible future backend option**.

Reasoning:

- Qt, GLib/GIO, wxWidgets, and .NET already solve watcher use cases for apps inside those ecosystems.
- libuv solves the portable backend well, but adopting its event loop is a large design choice for small desktop migrations.
- Our sweet spot is a small CMake-friendly C++ facade with explicit capability reporting, diagnostics, debounce helpers, and blocking/threaded convenience mode.
- The first implementation can use native Linux inotify directly; later we can decide whether libuv should be an optional backend.

## Risks And Deferrals

- Recursive watch emulation can hit `max_user_watches`; expose this as diagnostics.
- Polling fallback is important for network filesystems but should not be in the first tiny sample unless implementation stays very small.
- fanotify is not a first slice; it is more permission/system-policy-heavy.
- File-manager shell namespace watching, recycle-bin semantics, and desktop portal integration are not first slice.
- UI thread dispatch belongs in toolkit adapters, not in the core watcher.
- Rust portability should be preserved by keeping the public model callback-oriented, POD-friendly, and not tied to C++ inheritance.

## Decision

File watching remains a strong next implementation candidate after `ld_settings`, but the next steps should happen in order:

1. Extract shared C++ diagnostics into a tiny `ld_core` surface.
2. Complete the broader application and library audit.
3. Use ADR 0010 as the `ld_watch` API sketch before code.
4. Build a broad prototype rather than a tiny one-event demo.

The recommended first API should prioritize:

- explicit capability reporting,
- event coalescing/debounce,
- overflow/rescan signaling,
- recursive policy honesty,
- a watcher-owned path value rather than bare strings,
- raw events, settled-file trigger, and dirty-path refresh as named layers,
- simple CMake use,
- and examples that show the same task before/after for Notepad++ live monitoring and ShareX watch-folder upload triggers.
