# File Watcher Application Audit

This pass sharpens the application side of `ld_watch` before ADR/API design. It expands the first focused audit into the three proof cases chosen for the watcher design tree: Notepad++, ShareX, and Files.

The goal is not to copy application behavior into a library. The goal is to identify which watcher layers are real:

- raw filesystem event stream,
- settled-file task trigger,
- and dirty-path refresh.

## Proof Case Summary

| Application | Watcher role | Layer pressure | Design implication |
|---|---|---|---|
| Notepad++ | Open-file monitoring and reload/removed prompts | Raw events plus single-file facade | Watch target may be file or directory; save-by-replace is normal; rename pairing must be honest. |
| ShareX | Watch folder as upload/task trigger | Settled-file trigger | Debounce and "ready enough" are separate from raw events; caller tags are needed for task routing. |
| Files | Directory/file-manager refresh and recycle-bin signals | Dirty-path refresh | A view may need "refresh this directory" more than detailed per-event replay. |

## Current Source Check

Checked on 2026-08-28 against shallow upstream checkouts:

| Application | Commit | Source anchors checked | Audit result |
|---|---:|---|---|
| Notepad++ | `c057c08` | `PowerEditor/src/NppIO.cpp`, `PowerEditor/src/WinControls/ReadDirectoryChanges/*`, file browser watcher code | Still proves a single-file facade over directory events plus a secondary file timestamp/size probe. |
| ShareX | `57828e6` | `ShareX/WatchFolder.cs`, `ShareX/WatchFolderManager.cs`, `ShareX/TaskSettings.cs` | Strong settled-file evidence: created-only events, duplicate suppression, lock/size stability checks, timeout, and app-owned move/upload routing. |
| Files | `abdfcb5` | `src/Files.App.Storage/Legacy/RecycleBinWatcher.cs`, `src/Files.App.Storage/Windows/Managers/WindowsFolderChangeWatcher.cs`, storage contracts | Strong dirty-refresh evidence: recycle-bin watcher collapses unexpected changes into refresh requests, and shell notifications may carry one or two paths rather than a clean portable file event. |

## Notepad++: Open-File Monitoring

Notepad++ watches the containing directory of an open file and filters directory notifications back to the buffer path. It also keeps file timestamp/size checking in the loop.

Lifecycle evidence:

- Startup/watch creation: the current buffer path determines the containing directory to watch.
- Event mapping: last-write, filename, and size changes matter for the active file.
- Backend use: `CReadDirectoryChanges` watches the parent directory with subtree enabled; `CReadFileChanges` separately checks file size and last-write time on timeout.
- Save-by-replace: temporary-file write and rename sequences must be treated as ordinary editor behavior.
- Rate limiting: repeated modified events are throttled before reload behavior.
- UI policy: the app decides whether to reload, prompt, scroll to end, stop monitoring, or ignore.
- Teardown: removed or renamed-away events stop monitoring for that file.

`ld_watch` implication:

- support file and directory watch targets,
- represent modified, removed, and rename-old/new semantics,
- allow rename pairing only when backend confidence is high,
- expose degraded/lost-sync diagnostics,
- and keep UI reload policy outside the library.

## ShareX: Watch Folder As Task Trigger

ShareX treats watch folders as user-configured automation. A newly available file becomes input to an upload or processing task.

Lifecycle evidence:

- Startup/watch creation: watch folders are collected from default task settings and hotkey-specific task settings.
- Backend use: `FileSystemWatcher` is created per folder, optional filters are applied, subdirectories are caller-configurable, and only `Created` is subscribed for the trigger path.
- Event routing: events must map back to configured task settings.
- Timing policy: the handler suppresses duplicate events, waits until the file is unlocked, then requires several stable positive size samples before triggering, with an overall timeout.
- Side effects: task handling may move files before uploading.
- Failure handling: watcher errors should be surfaced without killing the whole app.

`ld_watch` implication:

- provide a caller-supplied tag and a library watch id,
- distinguish simple debounce from settled-file readiness,
- keep upload/move/task policy application-owned,
- and include deterministic tests for debounce/settle behavior.

## Files: Dirty View Refresh

Files uses watcher signals in file-manager and storage contexts, including recycle-bin watching. The first module should not absorb shell namespace or recycle-bin policy, but the app still proves a different watcher layer.

Lifecycle evidence:

- Startup/watch creation: storage or view-model code subscribes to filesystem changes.
- Backend use: recycle-bin watching creates per-drive `FileSystemWatcher` instances for the current user's `$RECYCLE.BIN` directory and avoids network drives; Windows folder change watching uses `SHChangeNotifyRegister`.
- Event mapping: many raw changes may collapse into "this directory/view is dirty."
- Refresh policy: the app often wants to rescan or refresh a visible directory rather than consume every low-level event.
- Platform specificity: recycle-bin and shell namespace behavior may need Windows-specific or toolkit-specific code.

`ld_watch` implication:

- name dirty-path refresh as a separate layer,
- avoid a full snapshot/diff cache in the first module,
- provide `rescan_recommended` and degraded state when events are lost,
- and defer shell namespace, recycle-bin, and file-manager policy.

## Layer Decision

The first design should name three layers:

1. Raw events: portable event kinds, path identity, diagnostics, and lifecycle state.
2. Settled-file trigger: opt-in debounce and stable size/mtime readiness for task workflows.
3. Dirty-path refresh: documented layer for file-manager refresh; prototype only if it stays tiny.

The first prototype should implement raw events and settled-file behavior. Dirty-path refresh should shape the API but remain deferred unless the helper is trivial.

## Source Anchors

- Notepad++ `NppIO.cpp`: https://raw.githubusercontent.com/notepad-plus-plus/notepad-plus-plus/master/PowerEditor/src/NppIO.cpp
- Notepad++ `ReadDirectoryChanges`: https://github.com/notepad-plus-plus/notepad-plus-plus/tree/master/PowerEditor/src/WinControls/ReadDirectoryChanges
- ShareX `WatchFolder.cs`: https://raw.githubusercontent.com/ShareX/ShareX/develop/ShareX/WatchFolder.cs
- ShareX `MainForm.cs`: https://raw.githubusercontent.com/ShareX/ShareX/develop/ShareX/Forms/MainForm.cs
- ShareX `WatchFolderManager.cs`: https://raw.githubusercontent.com/ShareX/ShareX/develop/ShareX/WatchFolderManager.cs
- ShareX `TaskSettings.cs`: https://raw.githubusercontent.com/ShareX/ShareX/develop/ShareX/TaskSettings.cs
- Files `RecycleBinWatcher.cs`: https://github.com/files-community/Files/blob/main/src/Files.App.Storage/Legacy/RecycleBinWatcher.cs
- Files `WindowsFolderChangeWatcher.cs`: https://github.com/files-community/Files/blob/main/src/Files.App.Storage/Windows/Managers/WindowsFolderChangeWatcher.cs
