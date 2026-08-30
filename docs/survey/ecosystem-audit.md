# Ecosystem Audit

This audit checks whether surveyed Windows-heavy projects already have native ports, reimplementations, Wine-based workarounds, or reusable abstraction libraries. It exists to keep module scoring honest: a candidate module should not be scored only from source usage inside Windows applications.

## Why This Matters

Before assigning final scores, verify:

- whether a native port or rewrite already exists,
- whether the port is upstream, community-maintained, experimental, abandoned, or questionable,
- which platform seams the port had to replace,
- whether a reusable library already solves the seam well enough,
- and whether users still have unsolved pain despite the existing project.

This does not mean we avoid solved areas automatically. A solved area can still be valuable if existing tools are too large, toolkit-bound, async-only, language-specific, hard for AI agents to consume, or missing the CMake/C++ shape we want.

## Initial Findings

### Notepad++

- **Upstream status**: Official Notepad++ documentation still describes Notepad++ as Windows-family software and points non-Windows users toward Wine or virtual machines.
- **Native/community ports found**: Nextpad++ claims to be a native GTK4 Linux/macOS port using Scintilla/Lexilla.
- **Audit implication**: We should not assume "nobody ported Notepad++." We should audit whether Nextpad++ is real, source-available, legally clean, maintained, and architecturally useful. Even if it is not a model to copy, it is now a required comparison point.
- **Audit status**: First source inspection complete; build validation pending.
- **Source audit notes**:
  - The GitHub repository is public and has a substantial C/C++ tree: `src/`, `lexilla/`, `resources/`, `packaging/`, `regex/`, `fuzzy/`, and `vendor/`.
  - The build is CMake-based and targets C11/C++17 with GTK4, libadwaita, uchardet, Lexilla, and a Scintilla GTK4 backend.
  - The public clone inspected on 2026-08-25 did not contain the expected `scintilla/` directory or `.gitmodules`, even though `CMakeLists.txt` and `CLAUDE.md` describe `scintilla/` as vendored. Treat public build completeness as unresolved until a clean Ubuntu build is attempted with the intended source layout.
  - License files indicate GPL-3, but `LICENSE` still says "Notepad++ for macOS" in its heading; `NOTICE` attributes grafted files from `notetux-plus-plus` and bundled Scintilla/Lexilla/resources. Treat legal/attribution review as required before using it as more than design evidence.
  - This appears to be a GTK4 rewrite/port that preserves compatibility at resource, config, shortcut, macro, and plugin-message surfaces. It is not a generic Windows abstraction layer.
- **Relevant source locations from clone**: `CMakeLists.txt`, `CLAUDE.md`, `NOTICE`, `src/editor.c`, `src/paths.c`, `src/plugin.c`, `src/run.c`, `src/workspace.c`, `src/main.c`, `src/cliphistory.c`, `src/spell.c`.
- **Platform seam findings**:
  - File watching uses `GFileMonitor` per open document, with external-change prompting and silent reload/tail-style modes in `src/editor.c`.
  - Settings/config uses XDG-style storage under `$XDG_DATA_HOME/nextpad++`, while keeping Notepad++-compatible XML files such as `config.xml`, `session.xml`, `shortcuts.xml`, `langs.xml`, and `stylers.xml`.
  - Process/shell integration uses GLib/GTK paths such as `g_spawn_async`, `gtk_show_uri_on_window`, `xdg-terminal-exec`, and browser/open-file helpers rather than Win32 `ShellExecute`.
  - Single-instance activation uses `GtkApplication` with `G_APPLICATION_HANDLES_OPEN`; `-multiInst` switches to `G_APPLICATION_NON_UNIQUE`.
  - Dynamic loading uses raw `dlopen`/`dlsym`/`dlclose` for Linux `.so` plugins and optional spell-check library loading.
  - Clipboard and drag/drop are GTK/GDK-native: `GdkClipboard`, async clipboard reads, and `GtkDropTarget` with `GDK_TYPE_FILE_LIST`.
- **Classification**: Reference implementation and warning sign, not dependency candidate. It is useful for studying migration seams, config compatibility, GTK4 Scintilla integration, and plugin-surface adaptation. It does not remove the need for our general-purpose libraries because it solves the app by port/rewrite rather than by reusable small abstractions.
- **Immediate effect on scoring**:
  - Increases confidence that `settings/config`, `file watcher`, `process/shell`, `dynamic library loading`, `single-instance IPC`, and `clipboard/drag-drop` are real Notepad++-class requirements.
  - Reduces confidence that a pure Win32-compatibility facade is the right strategy for Notepad++; a native toolkit port may be more realistic for UI-heavy areas.
  - Strengthens the split between toolkit-neutral modules and toolkit adapters.
- **Questions for follow-up**:
  - Does the Linux source build locally on Ubuntu?
  - Which Notepad++ features were rewritten versus shared?
  - How are plugins, settings, file watching, dialogs, and Scintilla integration handled?
  - Is the project a durable port, a partial clone, or a packaging/branding shell?

### WinMerge

- **Upstream status**: WinMerge documentation says there are no current plans for cross-platform support.
- **Existing workaround/variant signal**: The manual references an unofficial WinMerge 2011 version and Wine usage under Linux.
- **Native/community ports found**: No current strong native Linux port identified in the first ecosystem pass.
- **Audit implication**: WinMerge remains a good requirements source, especially for shell/process, clipboard, dynamic loading, filesystem semantics, and single-instance activation.
- **Questions for follow-up**:
  - Is WinMerge 2011 still source-available and useful as design evidence?
  - Which WinMerge Windows features users miss most on Linux compared with Meld/KDiff3?

### ShareX

- **Upstream/project-family status**: ShareX itself is Windows-heavy, but XerahS is a ShareX-family cross-platform rewrite using Avalonia and .NET.
- **Native/community ports found**: XerahS targets Windows, macOS, and Linux; sharenix is a Linux/FreeBSD ShareX-like clone.
- **Audit implication**: ShareX is not just an unported candidate anymore. It is now both a requirements source and a reference case for a full rewrite strategy.
- **Audit status**: First XerahS source inspection complete; build validation not attempted.
- **Source audit notes**:
  - The GitHub repository is public, active, and substantial. The page inspected on 2026-08-25 showed the `develop` branch, 4,484 commits, GPL-3.0 licensing, and explicit active-development messaging.
  - The repository uses .NET 10 and Avalonia 11.3+ for desktop UI, with `ShareX.ImageEditor` and `ShareX.VideoEditor` as Git submodules.
  - The tree contains a clean platform split: `src/platform/XerahS.Platform.Abstractions`, `XerahS.Platform.Linux`, `XerahS.Platform.Windows`, `XerahS.Platform.MacOS`, and mobile-specific platform projects.
  - Linux support is documented beyond code: `docs/linux/xdg-storage.md`, `docs/linux/portal-behavior.md`, `docs/linux/global-hotkeys-evdev.md`, `docs/linux/flatpak-permissions.md`, and `docs/linux/flathub-submission-checklist.md`.
- **Relevant source locations from clone**: `README.md`, `.gitmodules`, `docs/linux/README.md`, `docs/linux/xdg-storage.md`, `docs/linux/portal-behavior.md`, `docs/linux/global-hotkeys-evdev.md`, `docs/linux/flatpak-permissions.md`, `src/platform/XerahS.Platform.Abstractions/PlatformServices.cs`, `IClipboardService.cs`, `IShellIntegrationService.cs`, `Services/IHotkeyService.cs`, `Services/IWatchFolderDaemonService.cs`, `src/platform/XerahS.Platform.Linux/LinuxPlatform.cs`, `Services/LinuxClipboardService.cs`, `Services/LinuxShellIntegrationService.cs`, `Services/LinuxStartupService.cs`, `Services/LinuxWatchFolderDaemonService.cs`, `Services/LinuxRuntimeEnvironment.cs`, `Capture/Orchestration/WaterfallCapturePolicy.cs`, `Capture/Orchestration/LinuxCaptureCoordinator.cs`.
- **Platform seam findings**:
  - Platform services are explicit interfaces, not hidden conditionals: screen capture, clipboard, clipboard monitoring, windowing, input, fonts, shell integration, hotkeys, notifications, startup, diagnostics, theme, OCR, watch-folder daemon, and optional scrolling capture.
  - Linux runtime detection distinguishes Flatpak, Snap, container, Wayland, X11, desktop environment, and portal availability before selecting services.
  - Capture is a provider/policy pipeline with traceable fallback stages: portal, desktop DBus, Wayland protocol, X11, and CLI/provider fallbacks depending on sandbox, session, and user selector preference.
  - Global hotkeys use a backend ladder: direct evdev when readable, XDG GlobalShortcuts portal when available, then X11 key grabs. The docs explicitly call out permissions and sandbox limitations.
  - Clipboard is CLI/backend-based on Linux: prefers `wl-copy`/`wl-paste` on Wayland and falls back to `xclip`, including image payloads and `text/uri-list` file drops.
  - Shell integration writes XDG desktop entries, MIME package files, `mimeapps.list` associations, Nautilus/Nemo/Caja scripts, KDE service menus, and Thunar send-to entries.
  - Startup integration writes an XDG autostart `.desktop` entry outside sandboxes and switches to a Background portal service for Flatpak when available.
  - Watch-folder behavior is modeled as a daemon/service lifecycle, with user/system systemd units, start/stop/restart/status, elevation handling, and packaging concerns.
  - XDG storage is treated as a release gate: no implicit top-level `~/XerahS`, `~/.XerahS`, `~/ShareX`, or `~/Screenshots`.
  - Flatpak permissions are audited as product behavior: portals are preferred, broad filesystem/session-bus permissions are avoided, and tray/status-notifier integration is explicitly marked as risky.
- **Classification**: Strong reference implementation for abstraction shape and Linux desktop reality; not a direct dependency candidate because it is .NET/Avalonia and app-specific. It should influence API design, capability reporting, backend selection, diagnostics, and packaging guidance.
- **Immediate effect on scoring**:
  - Raises priority for `settings/config`, `process/shell`, `clipboard`, `shell integration`, `startup integration`, and `diagnostics/capability reporting`.
  - Raises `file watcher` priority but broadens it: ShareX-class watch folders may need a daemon/service model, not only an in-process watcher.
  - Moves global hotkeys/input and capture into clear UI/desktop-adjacent or future-work buckets because Linux solutions are permissions-, compositor-, and sandbox-dependent.
  - Strengthens the idea that each module needs explicit `supported`, `unsupported`, `requires permission`, `requires external tool`, and `sandbox-limited` states.
  - Provides a reference for AI-agent-friendly docs: Linux behavior pages, permission tables, troubleshooting commands, and validation checklists.
- **Questions for follow-up**:
  - Which Linux seams in XerahS are solved by portals, DBus, Avalonia, SharpHook, X11, or Wayland-specific paths?
  - Which issues remain hard even after a rewrite?
  - Which parts of ShareX could have benefited from reusable platform modules instead of a rewrite?

### OpenIPC Dashboard

- **Upstream/project-family status**: OpenIPC Dashboard is a maintained
  cross-platform Qt/QML application with desktop and headless/server modes.
- **Reference role**: Both FlavorTest candidate and reference implementation.
  It is useful for deciding where LinuxDesktop2026 should integrate with an
  existing toolkit rather than replace it.
- **Audit status**: FlavorTest extraction complete; source anchors are recorded
  in `docs/FlavorTests/SOURCES.md`.
- **Source audit notes**:
  - The Dashboard-shaped slice keeps Qt/QML startup, web deployment policy,
    administrator bootstrap, readiness, browser diagnostics, QSettings
    placement, and service data-root layout in product code.
  - LinuxDesktop2026 is used only for ordinary desktop root discovery through
    `ld_paths`.
  - `OPENIPC_DATA_ROOT` and `--data-root` select an isolated service profile,
    not just a generic settings directory override.
- **Platform seam findings**:
  - Qt already owns application identity, QSettings mechanics, QML lifecycle,
    resource loading, event-loop ownership, and web/server object integration.
  - LinuxDesktop2026 still has useful adjacency around desktop root discovery,
    service/headless environment guidance, diagnostics translation, migration
    safety, packaging boundaries, and desktop/server separation.
- **Classification**: Strong reference case and negative evidence against a
  generic toolkit replacement layer. Prefer `document/recommend` for Qt-owned
  seams and `build/adapt` only around narrower platform mechanics.
- **Immediate effect on scoring**:
  - Reduces confidence in broad service-root or toolkit abstractions before
    another product repeats the same pressure.
  - Strengthens the rule that product-facing diagnostics should be translated
    before reaching UI, browser, or health-check surfaces.
  - Reinforces that FlavorTests can be evidence against new APIs, not only
    evidence for them.

### Greenshot

- **Upstream status**: Greenshot has a macOS version but no Linux plan in the near term; the FAQ cites the historical .NET Framework/WinForms/WPF codebase and reluctance to maintain multiple codebases.
- **Native/community ports found**: No strong current Linux port identified in the first ecosystem pass.
- **Audit implication**: Greenshot remains a useful UI-adjacent requirements source, especially for clipboard, hotkeys, tray, printing, capture, and startup integration.
- **Questions for follow-up**:
  - Are current .NET/Avalonia/MAUI/Uno options changing the original Greenshot FAQ constraints?
  - Which abstractions would have reduced the "multiple codebases" concern?

### AutoHotkey

- **Upstream status**: Windows-focused automation runtime.
- **Native/community ports found**: AHK_X11 reimplements a substantial subset for Linux/X11, but it does not promise drop-in script compatibility and does not support Wayland yet.
- **Audit implication**: AutoHotkey is a boundary case. It teaches requirements, but many features are product-level OS automation rather than small reusable desktop-library seams.
- **Audit status**: First AHK_X11 source inspection complete; build validation not attempted.
- **Source audit notes**:
  - AHK_X11 is a ground-up Crystal implementation of a large subset of classic AutoHotkey v1.0.24 behavior for Unix-like systems with X11.
  - The README explicitly warns that Windows scripts usually need modification and that Wayland is not supported yet.
  - The project ships as a single native executable/AppImage and includes desktop integration assets for `.ahk` launch, compile, and Window Spy workflows.
  - The project license is GPL-2.0, so it is useful as reference evidence, not as code to copy into permissively licensed libraries.
- **Relevant source locations from clone**: `README.md`, `build/README.md`, `shard.yml`, `src/ahk_x11.cr`, `src/run/display.cr`, `src/run/display/display-adapter.cr`, `src/run/display/x11.cr`, `src/run/display/gtk.cr`, `src/run/display/at-spi.cr`, `src/run/display/hotkeys.cr`, `src/run/display/hotstrings.cr`, `src/run/runner.cr`, `src/cmd/misc/run.cr`, `src/cmd/misc/dll-call.cr`, `src/cmd/file/ini/ini-read.cr`, `src/cmd/file/ini/ini-write.cr`, `src/cmd/file/ini/ini-delete.cr`.
- **Platform seam findings**:
  - Display automation is intentionally X11-shaped. A small `DisplayAdapter` exists, but its concrete implementation depends on X11 key events, root/active-window grabs, XTest record events, X atoms, and libxdo-style window/input control.
  - Startup warns when `XDG_SESSION_TYPE` is not `x11`, specifically calling out that hotkeys, synthetic keys, and window operations will not work reliably outside X11.
  - Global hotkeys and hotstrings use low-level event capture and grabs. The implementation has to handle active-window changes, failed grabs, modifier state cleanup, and self-trigger prevention during `Send`.
  - Window management uses libxdo-style primitives for active-window discovery, window search, move/minimize/maximize/activate/close, mouse position, synthetic keyboard input, and pixel queries.
  - Control-level automation uses AT-SPI as a cross-toolkit accessibility bus, but the comments flag performance, reliability, missing IDs, traversal cost, null-pointer/crash risk, and retry logic.
  - Clipboard uses GTK3 clipboard APIs and blocking `ClipWait` behavior. The clear-clipboard path is workaround-heavy, which is a useful warning for toolkit adapters.
  - Process/shell behavior maps AutoHotkey `Run` verbs to Linux facilities: `Gio.app_info_launch_default_for_uri`, `xdg-open`, `gtk-launch`, `lp` for print, `setsid` for detached launch, and Crystal `Process` APIs.
  - Run-as/elevation behavior is fragile and security-sensitive: it builds shell commands, uses `su`, sets `DISPLAY`/`XAUTHORITY`, and temporarily modifies X access with `xhost`.
  - Dynamic library loading exists through a `DllCall` command backed by Crystal's loader and FFI. This is a useful requirement signal for dynamic loading, but not a direct model for C++ API design.
  - Single-instance behavior uses a `/tmp` lock file keyed by script/binary arguments, `flock`, stored PID, `SIGHUP` to terminate the previous instance, and optional GTK prompt/force/ignore behavior.
  - INI-style config support is implemented directly for `IniRead`, `IniWrite`, and `IniDelete`, reinforcing that small config-file compatibility helpers may matter separately from OS settings stores.
- **Classification**: Boundary reference and warning sign. It is a strong requirements source for automation-heavy Windows semantics, but it should not be treated as a dependency candidate or proof that global input/window control belongs in the first reusable library wave.
- **Immediate effect on scoring**:
  - Raises confidence that `process/shell`, `dynamic library loading`, `single-instance IPC`, `settings/config`, and `clipboard` are recurring requirements beyond Notepad++.
  - Moves `global hotkeys`, `synthetic input`, `window discovery/control`, `pixel search`, and `accessibility/control automation` toward `future/boundary` unless scoped as explicit toolkit/session adapters with capability reporting.
  - Strengthens the need for every desktop-adjacent module to report backend and limitations, for example `x11-only`, `wayland-limited`, `requires accessibility bus`, `requires external tool`, or `requires unsafe privilege behavior`.
  - Suggests that our first-wave APIs should avoid pretending to emulate Win32 automation broadly. We can document escape hatches and adapter patterns instead.
- **Questions for follow-up**:
  - Which AHK_X11 features map to reusable modules, and which require desktop-environment-specific automation?
  - Does Keysharp or another rewrite provide cross-platform lessons worth auditing?

### Rufus

- **Upstream status**: Rufus FAQ and issues repeatedly state no Linux/macOS port plan.
- **Native/community ports found**: No equivalent direct port identified; Linux users generally rely on different tools or manual workflows.
- **Audit implication**: Rufus remains a negative/boundary case. Its core is device, volume, boot, and privilege semantics rather than ordinary desktop abstraction.
- **Questions for follow-up**:
  - Are there reusable lower-level device libraries we should explicitly exclude from phase one?
  - Which Rufus-adjacent pieces are ordinary desktop needs, such as process, dialogs, config, downloads, and dynamic loading?

## Existing Abstraction Libraries To Compare

### Strong First Comparisons

- **libuv**: File watching, process spawning, IPC, filesystem operations, dynamic loading, and event loops. Strong reference for C API design, but it may be more async/event-loop-oriented than some desktop apps want.
- **Qt Core**: `QSettings`, `QFileSystemWatcher`, `QProcess`, `QLibrary`, and related APIs. Strong C++ reference, but adopting Qt as a dependency is a large product choice.
- **wxWidgets**: Native GUI toolkit with file watcher, dynamic loading, drag/drop, printing, and native handles. Useful for understanding toolkit-coupled seams.
- **GLib/GIO**: Linux-native process, file monitor, settings, DBus, and desktop integration primitives. Strong Linux backend reference, but not Windows-first.
- **Boost.Process and Boost.DLL**: C++ libraries for process management and dynamic loading. Good candidates to wrap, recommend, or avoid depending on dependency budget and API shape.

### libuv

- **Audit status**: First source/doc inspection complete; build validation not attempted.
- **Project shape**:
  - MIT-licensed C library with stable ABI goals and CMake support.
  - Tier-1 support includes GNU/Linux, macOS, and Windows.
  - The API is centered on an explicit event loop, handles, requests, callbacks, and close/lifecycle rules.
- **Relevant source locations from clone**: `README.md`, `SUPPORTED_PLATFORMS.md`, `include/uv.h`, `docs/src/fs_event.rst`, `docs/src/process.rst`, `docs/src/dll.rst`, `docs/src/pipe.rst`, `src/unix/linux.c`, `src/win/fs-event.c`, `src/unix/process.c`, `src/win/process.c`, `src/unix/dl.c`, `src/win/dl.c`, `src/unix/pipe.c`, `src/win/pipe.c`.
- **Platform seam findings**:
  - File watching is a direct cross-platform feature via `uv_fs_event_t`. Linux uses inotify; Windows uses `ReadDirectoryChangesW`; BSD/macOS use kqueue/FSEvents-style paths. Recursive watching is documented as supported only on macOS and Windows.
  - Process spawning is mature and detailed: executable, argv, env, cwd, stdio redirection/inheritance, detached mode, exit callback, uid/gid on Unix, and Windows-specific flags for quoting/window visibility/path behavior.
  - IPC pipes bridge Unix domain sockets/pipes/FIFOs and Windows named pipes. The docs expose important constraints such as Unix socket path truncation, Linux abstract namespace support through newer APIs, and Windows-only pending instance behavior.
  - Dynamic library loading is small and clean: `uv_dlopen`, `uv_dlsym`, `uv_dlclose`, and `uv_dlerror`, backed by `dlopen` on Unix and `LoadLibraryExW` on Windows.
  - Filesystem, thread pool, signal handling, TTY, networking, and timers are available, but most are broader than our first desktop-abstraction target.
- **Fit for our modules**:
  - Strong dependency or backend candidate for file watching, process spawning, pipes/IPC, and dynamic loading when async/event-loop integration is acceptable.
  - Strong reference for portable C API design, error-code style, ABI discipline, and platform feature documentation.
  - Poor direct user-facing shape for small desktop migrations that want synchronous or RAII C++ APIs without adopting a new event loop.
  - Does not solve desktop-open verbs, portals, XDG integration, config/settings, clipboard, drag/drop, UI, or shell registration.
- **Classification**: Dependency candidate and reference implementation. Prefer `wrap` or `document/recommend` over `build` for the overlapping low-level pieces. If we build first-wave modules in this area, they should either use libuv underneath or clearly explain why they intentionally avoid it.
- **Immediate effect on scoring**:
  - Reduces the case for building raw file-watcher, raw process-spawn, raw pipe, or raw dynamic-loader primitives from scratch.
  - Increases the case for higher-level, desktop-shaped facades over existing primitives: document reload watcher, shell/open helper, single-instance activation, plugin-library loader policy, and diagnostics/capability reporting.
  - Suggests a split between `core async backend` and `small C++ convenience facade` so users can choose dependency depth.

### Qt Core

- **Audit status**: Current official Qt 6.11 documentation pass complete for the first-wave seams; source audit not needed yet.
- **Project shape**:
  - Mature cross-platform C++ framework with CMake package targets such as `Qt6::Core` and `Qt6::Gui`.
  - The relevant APIs are idiomatic Qt: `QObject`, signals/slots, `QString`, `QVariant`, and in some cases a Qt event loop.
  - Excellent answer for projects that already use Qt; heavy architectural choice for projects trying to keep a small non-Qt dependency surface.
- **Relevant APIs reviewed**: `QFileSystemWatcher`, `QSettings`, `QProcess`, `QLibrary`, `QStandardPaths`, `QDesktopServices`, `QLockFile`, `QLocalServer`, and `QLocalSocket`.
- **Platform seam findings**:
  - File watching is covered by `QFileSystemWatcher`, including file and directory watches and change signals. Its docs expose important portability constraints: resources can be exhausted, some platforms impose low watch limits, and watches stop after files are renamed or removed unless the app re-adds them.
  - Settings/config is covered by `QSettings`, including native storage and INI support. The docs explicitly map Windows registry, macOS property lists, and Unix INI-style storage, and warn that Unix keys are case-sensitive while Windows registry/INI keys are case-insensitive.
  - Standard paths are covered by `QStandardPaths`, including XDG-style Linux paths for config, data, state, cache, runtime sockets, applications, and user folders. It also has test-mode support, which is valuable for sample code and AI-agent validation.
  - Process spawning is covered by `QProcess`, including arguments, environment, working directory, channels, process state, errors, synchronous waits, and async notifications. This is a strong reference for a C++ process facade.
  - Dynamic loading is covered by `QLibrary`, including platform suffix/prefix handling for `.dll`, `.so`, `.dylib`, and basename-based loading.
  - Desktop-open verbs are partially covered by `QDesktopServices`, but that API lives in Qt GUI, not Qt Core, and is intentionally high-level.
  - Single-instance building blocks exist through `QLockFile`, `QLocalServer`, and `QLocalSocket`; Qt does not appear to ship one canonical "single instance app" class in Core.
- **Fit for our modules**:
  - Strong reference implementation and dependency candidate when a target project already uses Qt.
  - Good model for API ergonomics, CMake packaging, standard paths, testability, and typed settings.
  - Poor universal dependency for Win32-heavy apps that do not already use Qt, especially if their UI stack is Win32, GTK, wxWidgets, Scintilla-native, or custom.
  - Does not solve registry-compatible migration policy, shell extension registration, Windows message/plugin compatibility, clipboard/drag-drop without Qt GUI, or Linux portal/sandbox diagnostics by itself.
- **Classification**: Reference implementation and conditional dependency candidate. Prefer `document/recommend` for Qt apps, `toolkit adapter` for Qt-specific integration, and `build/wrap` for small toolkit-neutral seams where Qt would be too large.
- **Immediate effect on scoring**:
  - Reduces the case for building generic replacements when the consuming app is already Qt-based.
  - Increases the case for our modules to have adapters instead of one backend: `core`, `qt`, `glib`, and possibly `wx`.
  - Strengthens the first-wave API goal: expose portable concepts and capability/error states without forcing Qt object types across the public boundary.

### GLib/GIO

- **Audit status**: Current official GLib/GIO documentation pass complete for the first-wave seams; source audit only needed if we choose a GLib backend.
- **Project shape**:
  - Mature C platform library stack used heavily by GTK/GNOME applications and also usable without GTK.
  - APIs are built around `GObject`, `GFile`, `GVariant`, `GError`, GIO streams, D-Bus, and the GLib main context.
  - Strong Linux desktop backend and reference; less natural as a Windows-first C++ facade unless wrapped carefully.
- **Relevant APIs reviewed**: `GFileMonitor`, `GSubprocess`, `GAppInfo`, `GApplication`, `GSettings`, and `GModule`.
- **Platform seam findings**:
  - File watching is covered by `GFileMonitor`, created through `g_file_monitor()`, `g_file_monitor_file()`, or `g_file_monitor_directory()`. Change signals are delivered through the thread-default main context, so event-loop ownership must be part of any wrapper design.
  - Process spawning is covered by `GSubprocess`, including sync and async waits, stdin/stdout/stderr pipes, exit status, termination, Unix signals, and `PATH` searching. The docs explicitly avoid shell-style single-string command APIs, which is a useful security and correctness signal for us.
  - Desktop-open/app launching is covered by `GAppInfo` and launch contexts. This is closer to Linux desktop semantics than raw process spawning and should inform our `open/default app` module shape.
  - Single-instance activation is covered by `GApplication`: a unique app ID can enforce one primary instance per graphical session, and later invocations forward arguments/actions to the primary instance. On Linux this uses the D-Bus session bus.
  - Settings are covered by `GSettings`, but the schema/backend model makes it more opinionated than simple INI/XDG config files. It is excellent for GNOME/GTK apps and less ideal for direct registry-to-file migration.
  - Dynamic loading is covered by `GModule`, a portable module-loading API with optional module init/unload hooks and Win32/dlopen-style backends.
- **Fit for our modules**:
  - Strong Linux backend/reference for file watching, subprocesses, desktop-open, single-instance activation, settings, and dynamic loading.
  - Good match for GTK/Nextpad++-style ports and any app willing to depend on GLib.
  - Less suitable as the public cross-platform API shape because it carries GLib types, lifecycle, main-context behavior, and LGPL dependency considerations into consuming projects.
  - Does not solve Qt/wx/Avalonia integration, Notepad++ plugin-message compatibility, broad Win32 UI/windowing, Windows registry compatibility, or compositor-limited features such as global hotkeys and capture.
- **Classification**: Linux backend candidate, reference implementation, and toolkit-adapter foundation. Prefer `wrap` or `toolkit adapter` for GLib-heavy consumers, and keep public first-wave APIs neutral enough that GLib is optional.
- **Immediate effect on scoring**:
  - Confirms that Linux already has strong primitives for many first-wave seams, but they are not packaged as small Windows-migration libraries.
  - Raises priority for `desktop-open/default app`, `single-instance activation`, and `diagnostics/capability reporting`.
  - Suggests our first prototypes should be honest about loop ownership: blocking/synchronous convenience APIs, callback/event-loop variants, and explicit backend names.

### wxWidgets

- **Audit status**: First source/doc inspection complete from the local wxWidgets clone; official docs site was partially inaccessible through the browser, so local interface headers are the primary evidence for API details.
- **Project shape**:
  - Cross-platform C++ desktop framework for native-looking GUI apps, with non-GUI abstractions in `wxBase`.
  - Supports Windows, Unix/GTK, and macOS as primary desktop platforms.
  - License is the wxWidgets license, a modified LGPL that explicitly allows use in proprietary applications even with static linking.
- **Relevant source locations from clone**: `README.md`, `interface/wx/fswatcher.h`, `src/unix/fswatcher_inotify.cpp`, `interface/wx/utils.h`, `interface/wx/dynlib.h`, `src/unix/dlunix.cpp`, `interface/wx/fileconf.h`, `interface/wx/snglinst.h`, `src/unix/snglinst.cpp`, `interface/wx/clipbrd.h`, `src/gtk/clipbrd.cpp`, and `interface/wx/dnd.h`.
- **Platform seam findings**:
  - File watching is covered by `wxFileSystemWatcher` in `wxBase`, with event-loop delivery and explicit implementation limitations across MSW, macOS, and GTK. Recursive watching exists through `AddTree()`, but docs warn that non-MSW/macOS platforms may create many per-directory watches.
  - The Unix file watcher implementation uses inotify and requires an active wx event loop before creation. This reinforces that file watching is not only an OS primitive; wrapper lifecycle and event dispatch semantics matter.
  - Process execution is covered by `wxExecute()` and `wxProcess`, with sync/async modes, callbacks, redirected IO, environment/working-directory support, console visibility flags on Windows, Unix process-group handling, and a main-thread limitation.
  - Dynamic loading is covered by `wxDynamicLibrary`, including RAII-style load/unload, canonical platform library names, plugin naming that embeds compatibility information, and Unix `dlopen`/`dlsym`/`dlclose` backends.
  - Settings/config is covered by `wxFileConfig`, including INI-like plain text files, Windows registry avoidance when desired, legacy home-dotfile compatibility, XDG-compliant new-file defaults, and an explicit migration helper for moving older config files into XDG locations.
  - Single-instance detection is covered by `wxSingleInstanceChecker`, implemented with Win32 mutexes and Unix lock files using `fcntl()`/`flock()`. Its docs warn that Unix lock-file creation failure must not be treated as fatal because that can become a denial-of-service vector.
  - Clipboard is covered by `wxClipboard`, but it lives in `wxcore`, depends on the global clipboard object, and has platform-specific behavior such as X11 `PRIMARY` selection and GTK clipboard-manager persistence.
  - Drag/drop is covered by `wxDropSource`, `wxDropTarget`, data objects, and result codes, but it is window-bound and toolkit-owned. Some operation semantics are platform-specific, for example move support called out as MSW-only in the enum docs.
- **Fit for our modules**:
  - Strong reference implementation for C++ ergonomics, RAII, migration helpers, and pragmatic platform caveats.
  - Good dependency/recommendation for apps already willing to adopt a full C++ GUI framework.
  - Poor universal dependency for projects like Notepad++ unless the port strategy intentionally moves the whole UI onto wxWidgets.
  - The `wxBase` subset is an important comparison point for our first-wave non-UI modules, but public APIs should avoid requiring `wxString`, `wxEvtHandler`, or wx event loop ownership.
- **Classification**: Reference implementation and conditional dependency candidate. Prefer `document/recommend` for wx apps, `toolkit adapter` for clipboard/drag/drop/window-bound features, and `build/wrap` for small neutral modules where wx would be too broad.
- **Immediate effect on scoring**:
  - Strengthens `settings/config` because legacy-to-XDG migration is a documented, solved, real-world need.
  - Strengthens `file watcher`, but adds requirements for recursive-watch caveats, event-loop ownership, resource-limit diagnostics, symlink behavior, and incomplete platform event parity.
  - Strengthens `single-instance activation/checking`, but suggests separating "detect another instance" from "forward this invocation to the primary instance." wx solves detection; GLib solves activation forwarding.
  - Keeps clipboard and drag/drop in the `UI/clipboarding/drag-and-drop` category rather than first-wave core.

### Boost.Process and Boost.DLL

- **Audit status**: Current official Boost 1.92 documentation pass complete for Process and DLL; source audit not needed unless we prototype a Boost-backed module.
- **Project shape**:
  - Boost-licensed C++ libraries focused on narrower system seams than Qt, wxWidgets, or GLib/GIO.
  - Boost.Process is a portable process-management library and, in current docs, version 2 is centered on Asio executors.
  - Boost.DLL is a C++11 library for working with DLL/DSO modules and is available through a CMake `Boost::dll` target.
- **Relevant APIs reviewed**: Boost.Process `process`, `execute`, `async_execute`, launchers, stdio pipes, `process_start_dir`, `process_environment`, `environment::find_executable`; Boost.DLL `shared_library`, `import_symbol`, `import_alias`, `library_info`, `symbol_location`, `program_location`, `load_mode::append_decorations`, and export/alias macros.
- **Platform seam findings**:
  - Boost.Process covers the core process-spawn seam well: executable path, argument list, environment, working directory, stdio redirection, sync wait, async execution, cancellation mapping, process signaling, and executable lookup.
  - Its launcher model exposes real OS differences instead of hiding them: Windows launchers map to `CreateProcessW` and related APIs, while POSIX launchers use fork/vfork variants and internal error pipes.
  - Process v2 defaults make lifecycle explicit: a process handle going out of scope terminates the subprocess unless detached, and detaching can create zombie-process risk.
  - Boost.Process is intentionally not a desktop-shell abstraction. It does not replace `ShellExecute`, default-app opening, URI launching, XDG portals, file association registration, or GUI activation.
  - Boost.DLL covers the dynamic-loading seam deeply: library load/unload, symbol import, exported aliases, platform decorations, program/self location, plugin discovery, and plugin lifetime rules.
  - Boost.DLL documents plugin-specific hazards that matter for Notepad++-style migration, including imported-symbol lifetime, Linux symbol shadowing, exported visibility, `-rdynamic` for self-loading, ABI portability, and platform-specific thread-safety limitations.
- **Fit for our modules**:
  - Strong dependency candidate for a process-spawn backend if users already accept Boost and Asio.
  - Strong dependency or direct recommendation for plugin/dynamic-library loading in C++ projects.
  - Less suitable as the only public API for our target because small migration users may not want Asio executor plumbing or Boost-wide dependency cost.
  - Excellent source of edge-case documentation for our own wrappers, especially lifecycle, cancellation, environment case-sensitivity, symbol visibility, and plugin ABI boundaries.
- **Classification**: Dependency candidate and reference implementation. Prefer `wrap` or `document/recommend`; only `build` where we intentionally provide smaller synchronous CMake-friendly facades or higher-level desktop semantics above Boost.
- **Immediate effect on scoring**:
  - Reduces the case for building raw process spawning and raw dynamic loading from scratch.
  - Increases priority for a higher-level `process/shell` split: one module for command execution, another for "open this file/URI/default app" desktop behavior.
  - Increases priority for a `plugin-loader policy` layer rather than merely a `LoadLibrary`/`dlopen` wrapper, because real apps need naming, ABI, visibility, lifetime, diagnostics, and compatibility rules.
  - Keeps `single-instance`, `settings/config`, `file watching`, clipboard, drag/drop, and shell registration outside Boost's solved area.

## Direction Feeding The First Score

This pass does not justify final project totals by itself, but it feeds the first numeric scoring pass in `docs/survey/module-priority-score.md`.

### First Candidates

- **Settings/config and standard paths**: Build or wrap. Existing libraries solve pieces, but Windows registry-to-file migration, INI/XML compatibility, XDG layout, test-mode behavior, and AI-agent-friendly examples are not packaged as a tiny migration module.
- **Process execution and desktop-open split**: Wrap/recommend for raw process spawning, build for higher-level desktop-open/default-app behavior. Boost.Process, libuv, Qt, and GLib already solve spawning, but `ShellExecute`-style migration needs separate semantics for files, URIs, terminals, print/open verbs, sandbox/portal limits, and diagnostics.
- **Dynamic library/plugin loading policy**: Wrap/recommend for raw loading, build a policy layer. Boost.DLL, libuv, Qt, GLib, and wxWidgets all cover loading, but migration projects need naming, ABI, symbol visibility, lifetime, plugin directory layout, and compatibility reporting.
- **File watcher/document reload helper**: Wrap existing primitives where possible. The recurring unsolved value is not just "watch path"; it is editor-style reload behavior, missing-file/recreate handling, recursive-watch warnings, symlink policy, resource-limit diagnostics, and backend capability reporting.
- **Single-instance activation**: Build only if scoped beyond lock files. Existing libraries cover detection or primary-instance forwarding separately; our useful seam is a small API that can choose lock-file, pipe/socket, DBus, or toolkit activation and report which behavior is available.

### UI/Clipboard/Drag-And-Drop

- **Clipboard**: Toolkit adapter. Every credible source ties clipboard to toolkit/session behavior: GTK/GDK, wxCore, Qt GUI, Wayland/X11 command tools, clipboard managers, formats, and lifetime rules.
- **Drag-and-drop**: Toolkit adapter. The useful common layer may only be data-object conventions and diagnostics; actual DnD belongs to windowing/toolkit ownership.
- **Shell integration/startup/tray/notifications**: Desktop adapter. XerahS shows this is a high-value area, but it touches XDG files, MIME registration, desktop-environment menus, startup portals, tray protocols, and packaging permissions.

### Future Work

- **Global hotkeys, synthetic input, window discovery/control, capture, and accessibility/control automation**: Real demand, but too compositor-, permission-, and desktop-environment-dependent for the first reusable library wave.
- **Device/volume/boot workflows**: Real requirement from Rufus-class apps, but outside the sweet spot for ordinary desktop-porting helpers.
- **Rust portability**: Keep C ABI hygiene, simple ownership rules, explicit error objects, and generated/bindgen-friendly headers in mind, but do not optimize the first design around Rust until we have working C++ samples.

## Completed Follow-Up

The next source-level pass focused on **Notepad++ settings/config and standard paths** first.

Rationale:

- It is narrow enough to verify deeply in Notepad++ without falling into full UI porting.
- Multiple surveyed projects use settings/config in different ways.
- Existing libraries do not remove the need for migration-specific behavior.
- It can produce the smallest credible working sample: read/write migrated config under XDG on Linux and compatible location conventions on Windows.
- It gives AI agents and human users a clear success case before we attempt harder seams like file watching or plugin loading.

Audit notes for that pass are tracked in `docs/survey/notepad-settings-config-audit.md`. The follow-up library classification is tracked in `docs/survey/settings-config-library-audit.md`.

The result was the first implementation sample: `ld_settings`.

### Follow-Up Categories

- File watching: libuv, Qt, wxWidgets, GLib/GIO, native inotify/fanotify/kqueue/ReadDirectoryChangesW wrappers.
- Process/shell: Boost.Process, libuv, Qt, GLib/GIO, XDG open/portals.
- Dynamic library loading: Boost.DLL, libuv, Qt, GLib `GModule`, native `LoadLibrary`/`dlopen`.
- Single-instance IPC: Qt local server, libuv pipes, DBus, Unix domain sockets, lock files.
- UI-adjacent transfer: Qt, GTK/GDK, wxWidgets, Avalonia, XDG portals, Wayland clipboard protocols.

## Scoring Gate

A module can move from unscored to a numeric score only after both checks are complete:

- **Source usage check**: At least several surveyed repositories use the feature in comparable ways.
- **Ecosystem check**: Existing ports, rewrites, and abstraction libraries have been identified enough to decide whether we should adopt, wrap, learn from, or deliberately avoid them.

Suggested decision labels:

- **Build**: Existing solutions do not fit our target shape.
- **Wrap**: Existing solution is strong, but needs a smaller CMake/API/package layer.
- **Document/recommend**: Existing solution is already the right answer.
- **Toolkit adapter**: Feature belongs behind GTK/Qt/wx/Avalonia-specific adapters.
- **Future/boundary**: Real need, but not suitable for the first reusable library wave.

## Source Links

- Notepad++ FAQ, non-Windows support: https://github.com/notepad-plus-plus/notepad-plus-plus/wiki/FAQ
- Nextpad++ Linux repository: https://github.com/nextpad-plus-plus/nextpad-plus-plus-linux
- Nextpad++ project page: https://nextpad.org/about/
- WinMerge FAQ, cross-platform support: https://manual.winmerge.org/en/Faq.html
- ShareX Linux/macOS request: https://github.com/ShareX/ShareX/issues/6466
- XerahS project page: https://getsharex.com/xerahs/
- XerahS repository: https://github.com/ShareX/XerahS
- sharenix repository: https://github.com/francesco149/sharenix
- Greenshot Linux/macOS FAQ: https://getgreenshot.org/faq/will-there-ever-be-a-greenshot-version-for-linux-or-mac/
- AHK_X11 repository: https://github.com/phil294/AHK_X11
- Rufus FAQ: https://github.com/pbatard/rufus/wiki/FAQ
- Rufus Linux/macOS fork issue: https://github.com/pbatard/rufus/issues/485
- libuv documentation: https://docs.libuv.org/en/v1.x/
- Qt `QFileSystemWatcher`: https://doc.qt.io/qt-6/qfilesystemwatcher.html
- Qt `QSettings`: https://doc.qt.io/qt-6/qsettings.html
- Qt `QProcess`: https://doc.qt.io/qt-6/qprocess.html
- Qt `QLibrary`: https://doc.qt.io/qt-6/qlibrary.html
- Qt `QStandardPaths`: https://doc.qt.io/qt-6/qstandardpaths.html
- Qt `QDesktopServices`: https://doc.qt.io/qt-6/qdesktopservices.html
- Qt `QLockFile`: https://doc.qt.io/qt-6/qlockfile.html
- Qt `QLocalServer`: https://doc.qt.io/qt-6/qlocalserver.html
- Qt `QLocalSocket`: https://doc.qt.io/qt-6/qlocalsocket.html
- GLib/GIO `GFileMonitor`: https://docs.gtk.org/gio/class.FileMonitor.html
- GLib/GIO `GSubprocess`: https://docs.gtk.org/gio/class.Subprocess.html
- GLib/GIO `GAppInfo`: https://docs.gtk.org/gio/iface.AppInfo.html
- GLib/GIO `GApplication`: https://docs.gtk.org/gio/class.Application.html
- GLib/GIO `GSettings`: https://docs.gtk.org/gio/class.Settings.html
- GLib `GModule`: https://docs.gtk.org/gmodule/
- Boost.Process: https://www.boost.org/library/latest/process/
- Boost.Process docs: https://www.boost.org/doc/libs/latest/libs/process/doc/html/index.html
- Boost.DLL: https://www.boost.org/library/latest/dll/
- Boost.DLL docs: https://www.boost.org/doc/libs/latest/doc/html/boost_dll.html
- Boost.DLL getting started: https://www.boost.org/doc/libs/latest/doc/html/boost_dll/getting_started.html
- Boost.DLL tutorial: https://www.boost.org/doc/libs/latest/doc/html/boost_dll/tutorial.html
- wxWidgets documentation: https://wxwidgets.org/docs/
- wxWidgets repository: https://github.com/wxWidgets/wxWidgets
- wxWidgets XDG config note: https://wxwidgets.org/blog/2024/01/using-xdg-compliant-config-files/
