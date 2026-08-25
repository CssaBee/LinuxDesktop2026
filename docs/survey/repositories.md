# Repository Survey

This file holds one structured entry for each repository in the survey corpus.

## Selection Rules

- Survey 15 to 20 repositories.
- Include about five reference implementations that already abstracted or ported similar Windows features.
- Include unported candidates where users repeatedly requested Linux support.
- Prefer repositories that reveal real usage of the selected Windows feature inventory.

## First-Pass Corpus

Status legend:

- **Candidate**: Windows-heavy project to inspect for actual feature usage.
- **Unported candidate**: Windows-heavy project with explicit or likely Linux demand.
- **Reference implementation**: Project that already abstracts or ports relevant capabilities.
- **Proof case**: Project used to test whether our libraries help with difficult migration work.

## Source Audit Round 1

The first source-level pass cloned the selected 10 repositories into `/tmp/linuxdesktop2026-survey` and used targeted `rg` searches for Win32, .NET, WinRT, toolkit, and cross-platform abstraction patterns. This pass is evidence gathering, not final scoring.

- **Audited candidates**: Notepad++, WinMerge, ShareX, Greenshot, AutoHotkey, Rufus, WinSCP, Files.
- **Audited references**: libuv, wxWidgets.
- **Not audited yet in source**: Open-Shell, System Informer, mRemoteNG, StaxRip, dnSpy, Qt, GLFW, Scintilla, OBS Studio, KeePassXC.

### 1. notepad-plus-plus/notepad-plus-plus

- **URL**: https://github.com/notepad-plus-plus/notepad-plus-plus
- **Status**: Proof case, unported candidate
- **Project type**: Text editor
- **Platform support status**: Windows-focused; upstream FAQ says there are no plans to port to other systems.
- **Linux port demand signal**: Strong; upstream FAQ references repeated non-Windows requests and Wine is a common workaround.
- **Windows APIs/features to inspect**: Windowing/message loop, common controls, dialog resources, registry/settings, file watching, shell/process integration, dynamic library/plugin loading, clipboard/drag-and-drop, filesystem/path handling, single-instance IPC, printing.
- **Relevant source locations**: `PowerEditor/src/NppIO.cpp`, `PowerEditor/src/NppCommands.cpp`, `PowerEditor/src/NppDarkMode.cpp`, `PowerEditor/src/DarkMode/DarkMode.cpp`, `PowerEditor/src/WinControls/FileBrowser/fileBrowser.cpp`, `PowerEditor/src/WinControls/PluginsAdmin/pluginsAdmin.cpp`.
- **Source audit highlights**: Uses pervasive `HWND`, `HDC`, `HMENU`, `SendMessage`, `PostMessage`, common-control messages, `ShellExecute`, `OpenClipboard`, `SetClipboardData`, `LoadLibrary`/`GetProcAddress`, dark-mode/user32/uxtheme dynamic probing, and file-browser update messages.
- **Central or incidental usage**: Central for GUI, Scintilla integration, plugins, clipboard, file browser, shell actions, dark mode, and process elevation/restart behavior.
- **Existing abstractions**: Has application-level wrappers and managers, but many APIs still expose raw Windows handles and messages.
- **Build system and language baseline**: Visual Studio-oriented C++ project.
- **License**: GPL-family; verify exact upstream license during audit.
- **Lessons for candidate modules**: Primary proof case for whether small platform libraries can reduce Win32 coupling without promising a full port.

### 2. WinMerge/winmerge

- **URL**: https://github.com/WinMerge/winmerge
- **Status**: Candidate, unported candidate
- **Project type**: File/folder diff and merge tool
- **Platform support status**: Windows-focused.
- **Linux port demand signal**: To verify through issue search.
- **Windows APIs/features to inspect**: MFC/windowing, common controls, shell extension, plugins/DLLs, filesystem/path handling, process launching, registry/settings, printing.
- **Relevant source locations**: `Src/MainFrm.cpp`, `Src/MainFrm.h`, `Src/Common/Clipboard.cpp`, `Src/Common/Shell.cpp`, `Src/Common/ShellFileOperations.cpp`, `ShellExtension/Common/WinMergeContextMenu.cpp`, `ShellExtension/ShellExtension/WinMergeShell.cpp`, `Src/WebPageDiffFrm.cpp`, `Src/CompareEngines/ImageCompare.cpp`.
- **Source audit highlights**: Uses MFC/window messages, `WM_COPYDATA` single-instance style activation, Explorer shell extension COM/context menus, `CreateProcess`, `ShellExecute`, `OpenClipboard`/`SetClipboardData`, `LoadLibrary`/`GetProcAddress`, and plugin-style DLL entry points.
- **Central or incidental usage**: Central for GUI, shell extension, folder compare, clipboard, plugin/archive workflows, and passing work to an existing instance.
- **Existing abstractions**: Compare engines and helpers exist, but shell, clipboard, UI, and plugin seams still surface Windows types.
- **Build system and language baseline**: Visual Studio solutions, MFC/ATL, C++.
- **License**: GPL-2.0.
- **Lessons for candidate modules**: Good evidence source for shell integration, filesystem semantics, plugin loading, and Windows GUI coupling.

### 3. ShareX/ShareX

- **URL**: https://github.com/ShareX/ShareX
- **Status**: Candidate, unported candidate
- **Project type**: Screenshot, screen recording, upload, and automation tool
- **Platform support status**: Windows application.
- **Linux port demand signal**: Explicit Linux/macOS requests exist in GitHub issues.
- **Windows APIs/features to inspect**: Clipboard, drag-and-drop, shell context menu, folder watching, process/actions, hotkeys, window capture, screen capture, notification/tray behavior.
- **Relevant source locations**: `ShareX.HelpersLib/Native/NativeMethods.cs`, `ShareX.HelpersLib/Input/KeyboardHook.cs`, `ShareX.HelpersLib/Input/HotkeyForm.cs`, `ShareX.HelpersLib/Helpers/ClipboardHelpers.cs`, `ShareX/WatchFolder.cs`, `ShareX/Presentation/MainWindow/TrayIconService.cs`, `ShareX/Presentation/MainWindow/MainWindow.axaml`, `ShareX.ScreenCaptureLib/Screenshot.cs`, `ShareX.ScreenCaptureLib/Presentation/ScreenRecording/ScreenRecordWindow.axaml.cs`.
- **Source audit highlights**: Uses .NET `FileSystemWatcher`, extensive clipboard helper/retry behavior, Avalonia drag/drop, WinForms `NotifyIcon`, `ProcessStartInfo`, `Registry`, `RegisterHotKey`, `SetWindowsHookEx`, `GetForegroundWindow`, `BitBlt`, and many `DllImport` calls into user32/gdi32/kernel32.
- **Central or incidental usage**: Central for capture, clipboard, tray, hotkeys/hooks, drag/drop upload paths, folder watching, and process-based helper/tool launching.
- **Existing abstractions**: Some services/helpers exist and newer Avalonia code abstracts parts of UI, but capture/hotkey/tray/clipboard remain platform-sensitive.
- **Build system and language baseline**: .NET/C#; exact current baseline pending audit.
- **License**: GPL-3.0; verify during audit.
- **Lessons for candidate modules**: Strong requirements source for clipboard, shell/process, file watcher, and hotkey-related capability reporting.

### 4. greenshot/greenshot

- **URL**: https://github.com/greenshot/greenshot
- **Status**: Candidate
- **Project type**: Screenshot and annotation tool
- **Platform support status**: Windows-focused repository; build instructions require Windows and Visual Studio.
- **Linux port demand signal**: To verify through issue search.
- **Windows APIs/features to inspect**: Screen/window capture, clipboard, printing, shell/email/Office export, hotkeys, tray/notification behavior, settings.
- **Relevant source locations**: `src/Greenshot.Base/Core/WindowCapture.cs`, `src/Greenshot.Base/Core/WindowDetails.cs`, `src/Greenshot.Base/Core/ClipboardHelper.cs`, `src/Greenshot.Base/Core/HotkeyManager.cs`, `src/Greenshot/Helpers/StartupHelper.cs`, `src/Greenshot/Helpers/CopyData.cs`, `src/Greenshot/Helpers/PrintHelper.cs`, `src/Greenshot/Forms/MainForm.cs`, `src/Greenshot.Editor/Drawing/Surface.cs`.
- **Source audit highlights**: Uses Dapplo.Windows packages for user32/kernel32/clipboard, direct `DllImport`, `RegisterHotKey`, `PrintWindow`, `BitBlt`, WinForms `NotifyIcon`, registry startup entries, Office/codec registry probing, clipboard helpers, drag/drop handlers, and GDI printing.
- **Central or incidental usage**: Central for screenshot capture, clipboard fidelity, hotkeys, tray UX, printing, startup integration, and Office/export helpers.
- **Existing abstractions**: Uses Dapplo.Windows as an external wrapper for many Win32 calls; this is a useful "wrap existing dependency or learn from it" case.
- **Build system and language baseline**: .NET Framework 4.8.x plus Visual Studio.
- **License**: GPL-3.0.
- **Lessons for candidate modules**: Good UI-adjacent requirements source for clipboard, drag/drop-like export paths, printing, and OS integration.

### 5. AutoHotkey/AutoHotkey

- **URL**: https://github.com/AutoHotkey/AutoHotkey
- **Status**: Candidate, unported candidate
- **Project type**: Automation scripting and hotkey runtime
- **Platform support status**: Windows-focused; README describes Win32/x64 platforms.
- **Linux port demand signal**: Strong category demand; exact upstream issue signal pending.
- **Windows APIs/features to inspect**: Hotkeys, hooks, process launching, window inspection/control, clipboard, IPC, dynamic library loading, filesystem/path handling.
- **Relevant source locations**: `source/hook.cpp`, `source/hotkey.cpp`, `source/keyboard_mouse.cpp`, `source/application.cpp`, `source/script_gui.h`, `source/script2.cpp`, `source/lib/DllCall.cpp`, `source/var.cpp`, `source/AutoHotkey.cpp`.
- **Source audit highlights**: Uses low-level keyboard/mouse hooks, `RegisterHotKey`, `SetWindowsHookEx`, `GetForegroundWindow`, `SendMessage`, `PostMessage`, `CreateMutex`, clipboard memory ownership via `SetClipboardData`, dynamic `DllCall` backed by `LoadLibrary`/`GetProcAddress`, and direct window/control automation.
- **Central or incidental usage**: Central; the product model is Windows desktop automation, message routing, hooks, and control/window introspection.
- **Existing abstractions**: Some internal abstractions exist for script/runtime semantics, but the platform behavior is the product. Treat as a boundary and requirements source, not a first-wave portability target.
- **Build system and language baseline**: Visual Studio C/C++ project.
- **License**: GPL-2.0.
- **Lessons for candidate modules**: Useful for deciding which automation/window-control features are too OS-specific for our first library wave.

### 6. pbatard/rufus

- **URL**: https://github.com/pbatard/rufus
- **Status**: Candidate, unported candidate
- **Project type**: Bootable USB creation utility
- **Platform support status**: Windows-focused.
- **Linux port demand signal**: Explicit FAQ says there is no plan for Linux/macOS and cites close coupling to Windows APIs.
- **Windows APIs/features to inspect**: Device enumeration, drive/volume APIs, registry/settings, windowing/dialogs, process/download helpers, filesystem/path handling, privilege/elevation behavior.
- **Relevant source locations**: `src/drive.c`, `src/dev.c`, `src/format.c`, `src/rufus.c`, `src/ui.c`, `src/msapi_utf8.h`, `src/wimlib/win32_common.c`, `src/net.c`, `src/stdlg.c`.
- **Source audit highlights**: Uses `DeviceIoControl`, `SetupDi` device enumeration, direct `CreateFile` on device/volume paths, `LoadLibrary`/`GetProcAddress` for Windows system DLLs, dialog/common-control messages, `SendMessage`/`PostMessage`, and Windows-specific UTF-8 wrapper helpers.
- **Central or incidental usage**: Central; removable media, volume layout, formatting, and privilege/device semantics are fundamental.
- **Existing abstractions**: Has targeted wrappers, but they mostly normalize Windows APIs rather than isolate a portable desktop requirement.
- **Build system and language baseline**: Visual Studio or MinGW; C/C++.
- **License**: GPL-3.0.
- **Lessons for candidate modules**: Important negative case: not every Windows dependency can become a reusable desktop library.

### 7. Open-Shell/Open-Shell-Menu

- **URL**: https://github.com/Open-Shell/Open-Shell-Menu
- **Status**: Candidate
- **Project type**: Windows Start menu and Explorer integration utility
- **Platform support status**: Windows-focused.
- **Linux port demand signal**: To verify.
- **Windows APIs/features to inspect**: Shell integration, registry/settings, windowing, Explorer extension behavior, hooks, process launching.
- **Relevant source locations**: Exact files pending source audit.
- **Central or incidental usage**: Central; project exists to integrate with Windows shell behavior.
- **Existing abstractions**: To verify.
- **Build system and language baseline**: Visual Studio-era C++ project; exact baseline pending audit.
- **License**: MIT; verify during audit.
- **Lessons for candidate modules**: Useful for identifying shell integration that should remain application-specific or Windows-only.

### 8. winscp/winscp

- **URL**: https://github.com/winscp/winscp
- **Status**: Candidate, unported candidate
- **Project type**: File transfer client and file manager
- **Platform support status**: Windows application.
- **Linux port demand signal**: To verify through issue/support search.
- **Windows APIs/features to inspect**: Windowing, shell integration, registry/settings, process launching, filesystem/path handling, dynamic libraries, drag-and-drop.
- **Relevant source locations**: `source/windows/WinMain.cpp`, `source/windows/Tools.cpp`, `source/windows/Setup.cpp`, `source/windows/VCLCommon.cpp`, `source/dragext/DragExt.cpp`, `source/putty/windows/*`, `source/console/Main.cpp`, `source/windows/TerminalManager.cpp`.
- **Source audit highlights**: Uses VCL/Win32 handles and messages, PuTTY Windows platform code, `WM_COPYDATA` activation, `CreateMutex`, `CreateProcess`, `ShellExecute`, `OpenClipboard`/`SetClipboardData`, drag extension shell integration, `LoadLibrary`/`GetProcAddress`, and registry/setup integration.
- **Central or incidental usage**: Central for GUI, shell/file-manager UX, terminal/process launch, clipboard, drag/drop extension, single-instance behavior, and setup/session integration.
- **Existing abstractions**: Domain/file-transfer logic is separated from some UI code, but the desktop shell and VCL surface are Windows-heavy.
- **Build system and language baseline**: Embarcadero C++Builder plus Visual Studio build tools.
- **License**: GPL-family; verify exact license during audit.
- **Lessons for candidate modules**: Good source for file-manager style requirements and settings/session portability.

### 9. winsiderss/systeminformer

- **URL**: https://github.com/winsiderss/systeminformer
- **Status**: Candidate
- **Project type**: System monitor, debugger, and security utility
- **Platform support status**: Windows 10+.
- **Linux port demand signal**: To verify.
- **Windows APIs/features to inspect**: Windowing, process inspection/control, services, dynamic libraries/plugins, registry/settings, tray/notifications, driver/kernel integration.
- **Relevant source locations**: Exact files pending source audit.
- **Central or incidental usage**: Central; much of the product is Windows internals.
- **Existing abstractions**: To verify.
- **Build system and language baseline**: Visual Studio, C/C++.
- **License**: MIT.
- **Lessons for candidate modules**: Good boundary case for separating reusable process/shell ideas from non-portable system internals.

### 10. files-community/Files

- **URL**: https://github.com/files-community/Files
- **Status**: Candidate
- **Project type**: Modern file manager
- **Platform support status**: Windows-focused.
- **Linux port demand signal**: To verify; project has Windows identity and file-manager replacement expectations.
- **Windows APIs/features to inspect**: Filesystem/path handling, shell integration, registry integration for default file manager behavior, windowing, drag-and-drop, clipboard, file operations.
- **Relevant source locations**: `src/Files.Core.SourceGenerator/Generators/RegistrySerializationGenerator.cs`, `src/Files.App.Storage/Legacy/RecycleBinWatcher.cs`, `src/Files.App/ViewModels/ShellViewModel.cs`, `src/Files.App.Storage/Windows/WindowsBulkOperations.cs`, `src/Files.App/ViewModels/Settings/AdvancedViewModel.cs`, `src/Files.App/Helpers/TransferHelpers.cs`, `src/Files.App/Utils/FileTags/FileTagsDatabase.cs`, `src/Files.App/Helpers/Navigation/NavigationHelpers.cs`.
- **Source audit highlights**: Uses Registry serialization/source generation, `FileSystemWatcher`, WinRT `StorageFile`/`StorageFolder`, `DataPackage`/clipboard/drag-drop transfer flows, COM `IFileOperation`, `ProcessStartInfo`, `regedit.exe`, `regsvr32.exe`, shell/default-file-manager registry state, and Windows AppLifecycle APIs.
- **Central or incidental usage**: Central for file operations, shell replacement behavior, clipboard/drag/drop, watcher refresh, settings/tag metadata, and Windows app model integration.
- **Existing abstractions**: Strong internal abstractions around storage items and filesystem helpers, but many are built on WinRT/COM/registry assumptions.
- **Build system and language baseline**: .NET/C#/WinUI; exact baseline pending audit.
- **License**: MIT and MPL-2.0 files present.
- **Lessons for candidate modules**: Useful for modern Windows shell and file-manager requirements beyond Notepad++.

### 11. mRemoteNG/mRemoteNG

- **URL**: https://github.com/mRemoteNG/mRemoteNG
- **Status**: Candidate, unported candidate
- **Project type**: Tabbed remote connection manager
- **Platform support status**: Windows-focused.
- **Linux port demand signal**: To verify through issue search.
- **Windows APIs/features to inspect**: Windowing/tabs, embedded RDP ActiveX controls, process launching, registry/install state, filesystem config, credential storage, clipboard.
- **Relevant source locations**: Exact files pending source audit.
- **Central or incidental usage**: Central for RDP/ActiveX and Windows desktop embedding.
- **Existing abstractions**: To verify.
- **Build system and language baseline**: .NET desktop application; exact baseline pending audit.
- **License**: GPL-2.0.
- **Lessons for candidate modules**: Helps identify when a Windows feature is really a product dependency rather than a portable library candidate.

### 12. staxrip/staxrip

- **URL**: https://github.com/staxrip/staxrip
- **Status**: Candidate
- **Project type**: Video/audio encoding GUI
- **Platform support status**: Windows-focused.
- **Linux port demand signal**: To verify.
- **Windows APIs/features to inspect**: Process launching, filesystem/path handling, settings, shell integration, drag-and-drop, windowing/dialogs.
- **Relevant source locations**: Exact files pending source audit.
- **Central or incidental usage**: Central for process orchestration and path/config handling.
- **Existing abstractions**: To verify.
- **Build system and language baseline**: .NET/WinForms; exact baseline pending audit.
- **License**: MIT.
- **Lessons for candidate modules**: Strong requirements source for process/shell APIs and configuration around external toolchains.

### 13. dnSpy/dnSpy

- **URL**: https://github.com/dnSpy/dnSpy
- **Status**: Candidate
- **Project type**: .NET debugger, decompiler, and assembly editor
- **Platform support status**: Windows desktop app; repository archived status to verify.
- **Linux port demand signal**: To verify; .NET ecosystem creates natural cross-platform pressure.
- **Windows APIs/features to inspect**: Windowing, process/debugging APIs, dynamic module handling, filesystem/path handling, settings, clipboard.
- **Relevant source locations**: Exact files pending source audit.
- **Central or incidental usage**: Central for debugging; mixed for editor/decompiler UI.
- **Existing abstractions**: To verify.
- **Build system and language baseline**: .NET/WPF-era project; exact baseline pending audit.
- **License**: GPL-3.0.
- **Lessons for candidate modules**: Useful for separating portable domain logic from Windows-only debugger and GUI integration.

### 14. qt/qtbase

- **URL**: https://github.com/qt/qtbase
- **Status**: Reference implementation
- **Project type**: Cross-platform application framework
- **Platform support status**: Windows, Linux, macOS, and more.
- **Linux port demand signal**: Not applicable; already cross-platform.
- **Windows APIs/features to inspect**: Settings, file watching, process launching, dynamic library loading, clipboard, GUI/windowing, filesystem/path handling, locking.
- **Relevant source locations**: Exact Qt source paths pending source audit.
- **Central or incidental usage**: Central; Qt intentionally abstracts many target capabilities.
- **Existing abstractions**: `QSettings`, `QFileSystemWatcher`, `QProcess`, `QLibrary`, `QClipboard`, `QLockFile`, and related APIs.
- **Build system and language baseline**: CMake, C++.
- **License**: LGPL/GPL/commercial; verify exact per-module terms during audit.
- **Lessons for candidate modules**: Strong reference for capability surface, platform limits, and where abstraction becomes toolkit-coupled.

### 15. wxWidgets/wxWidgets

- **URL**: https://github.com/wxWidgets/wxWidgets
- **Status**: Reference implementation
- **Project type**: Cross-platform native GUI framework
- **Platform support status**: Windows, GTK/Linux, macOS, and other ports.
- **Linux port demand signal**: Not applicable; already cross-platform.
- **Windows APIs/features to inspect**: GUI/windowing, common controls/dialogs, clipboard, drag-and-drop, printing, filesystem helpers, process helpers, settings-like facilities.
- **Relevant source locations**: `include/wx/dynlib.h`, `include/wx/dnd.h`, `include/wx/nativewin.h`, `include/wx/msw/private/fswatcher.h`, `include/wx/unix/private/fswatcher_inotify.h`, `src/msw/ole/dropsrc.cpp`, `src/msw/ole/droptgt.cpp`, `src/gtk/dnd.cpp`, `src/msw/thread.cpp`, `src/msw/mdi.cpp`, `src/msw/dc.cpp`.
- **Source audit highlights**: Provides native-handle escape hatches (`HWND`, `GtkWidget*`, `NSWindow`), separate MSW/GTK implementations, drag/drop on OLE vs GTK, file watcher backends including `ReadDirectoryChangesW` and inotify/kqueue, dynamic library wrappers, printing/DC abstractions, and toolkit-level message plumbing.
- **Central or incidental usage**: Central; wxWidgets is built around native platform ports.
- **Existing abstractions**: Native-looking cross-platform GUI and non-GUI APIs.
- **Build system and language baseline**: C++ with multiple build systems; exact current CMake support pending audit.
- **License**: wxWindows Library License.
- **Lessons for candidate modules**: Important comparison for native-control philosophy and for deciding which UI-adjacent features belong outside our first wave.

### 16. libuv/libuv

- **URL**: https://github.com/libuv/libuv
- **Status**: Reference implementation
- **Project type**: Cross-platform asynchronous I/O library
- **Platform support status**: Multi-platform.
- **Linux port demand signal**: Not applicable; already cross-platform.
- **Windows APIs/features to inspect**: File system events, process spawning, IPC, event loop, filesystem operations, dynamic library loading.
- **Relevant source locations**: `include/uv.h`, `include/uv/win.h`, `include/uv/unix.h`, `src/win/fs-event.c`, `src/unix/linux.c`, `src/win/process.c`, `src/unix/process.c`, `src/win/dl.c`, `src/unix/dl.c`, `src/win/pipe.c`, `src/unix/pipe.c`.
- **Source audit highlights**: Cleanly separates Windows and Unix backends for file watching, process spawning, pipes/IPC, dynamic library loading, filesystem calls, and event loops. Uses `ReadDirectoryChangesW`, `CreateProcessW`, `CreateNamedPipe`, `LoadLibrary`/`GetProcAddress`, inotify, epoll, kqueue, `posix_spawn`, `fork`/`exec`, `dlopen`, and `dlsym`.
- **Central or incidental usage**: Central.
- **Existing abstractions**: Event loop backed by epoll/kqueue/IOCP/etc.; filesystem events; process and IPC abstractions.
- **Build system and language baseline**: C with CMake/autotools-style support.
- **License**: MIT-like; verify exact license during audit.
- **Lessons for candidate modules**: Strong reference for file watcher, process, IPC, and capability reporting.

### 17. glfw/glfw

- **URL**: https://github.com/glfw/glfw
- **Status**: Reference implementation
- **Project type**: Cross-platform window, input, and graphics-context library
- **Platform support status**: Windows, Linux X11/Wayland, macOS.
- **Linux port demand signal**: Not applicable; already cross-platform.
- **Windows APIs/features to inspect**: Windowing, event loop, input, clipboard, native handle escape hatches.
- **Relevant source locations**: `src`; exact files pending source audit.
- **Central or incidental usage**: Central.
- **Existing abstractions**: Platform-independent API with backend-specific implementations and optional native access.
- **Build system and language baseline**: CMake, C99.
- **License**: zlib/libpng.
- **Lessons for candidate modules**: Useful for explicit capability limits and native-handle escape hatches in windowing/input APIs.

### 18. ScintillaOrg/scintilla

- **URL**: https://github.com/ScintillaOrg/scintilla
- **Status**: Reference implementation
- **Project type**: Source-code editing component
- **Platform support status**: Win32, GTK, macOS, Qt support.
- **Linux port demand signal**: Not applicable; already cross-platform.
- **Windows APIs/features to inspect**: Win32 control/messages, GTK widget API, notifications, dynamic/static linking, editor event model.
- **Relevant source locations**: Exact paths pending source audit.
- **Central or incidental usage**: Central for editor-widget behavior.
- **Existing abstractions**: Similar message model across Windows and GTK, plus platform-specific frontends.
- **Build system and language baseline**: C++; exact build paths pending audit.
- **License**: Permissive Scintilla license; verify during audit.
- **Lessons for candidate modules**: Critical reference for the Notepad++ proof case and GTK-first strategy.

### 19. obsproject/obs-studio

- **URL**: https://github.com/obsproject/obs-studio
- **Status**: Reference implementation
- **Project type**: Streaming/recording desktop application
- **Platform support status**: Windows, macOS, Linux.
- **Linux port demand signal**: Not applicable; already cross-platform.
- **Windows APIs/features to inspect**: Windowing/Qt integration, plugins, dynamic loading, process integration, capture backends, filesystem paths, settings.
- **Relevant source locations**: Exact paths pending source audit.
- **Central or incidental usage**: Central for platform-specific capture and plugin systems.
- **Existing abstractions**: Cross-platform application structure with platform-specific capture/output implementations.
- **Build system and language baseline**: CMake, C/C++, Qt.
- **License**: GPL-2.0.
- **Lessons for candidate modules**: Useful reference for keeping a large desktop app cross-platform while accepting platform-specific backends.

### 20. keepassxreboot/keepassxc

- **URL**: https://github.com/keepassxreboot/keepassxc
- **Status**: Reference implementation
- **Project type**: Password manager, community port of KeePass
- **Platform support status**: Windows, macOS, Linux.
- **Linux port demand signal**: Not applicable; already cross-platform.
- **Windows APIs/features to inspect**: Settings, filesystem/path handling, auto-type, clipboard, browser integration, secret service/credential integration, process/shell.
- **Relevant source locations**: Exact paths pending source audit.
- **Central or incidental usage**: Central for clipboard, auto-type, browser integration, and platform security integration.
- **Existing abstractions**: Qt-based cross-platform app with platform-specific security/desktop integrations.
- **Build system and language baseline**: CMake, C++/Qt.
- **License**: GPL-2.0/GPL-3.0 family with bundled third-party licenses.
- **Lessons for candidate modules**: Good reference for moving from a Windows-origin tool to a maintained cross-platform application.
