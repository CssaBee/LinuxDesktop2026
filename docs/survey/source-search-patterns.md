# Source Search Patterns

These patterns make the survey repeatable for humans and AI agents. They are intentionally broad first-pass searches; follow-up audits should inspect the matching files manually before scoring.

## C and C++ Win32 Projects

Use for Notepad++, WinMerge, AutoHotkey, Rufus, WinSCP, System Informer, Open-Shell, and similar repositories.

```sh
rg -n "\b(HWND|HINSTANCE|HDC|HMENU|HICON|WPARAM|LPARAM|LRESULT)\b" .
rg -n "\b(CreateWindowEx|SendMessage|PostMessage|WM_|ShellExecute|CreateProcess|CreateMutex|WM_COPYDATA)\b" .
rg -n "\b(LoadLibrary|GetProcAddress|FreeLibrary)\b" .
rg -n "\b(RegOpen|RegQuery|RegSet|RegGet|HKEY_)\b" .
rg -n "\b(ReadDirectoryChangesW|FindFirstChangeNotification)\b" .
rg -n "\b(OpenClipboard|SetClipboardData|DragAcceptFiles|DoDragDrop|IDropTarget|IDataObject)\b" .
rg -n "\b(DeviceIoControl|SetupDi|FSCTL_|IOCTL_)\b" .
```

## .NET Desktop Projects

Use for ShareX, Greenshot, Files, mRemoteNG, StaxRip, dnSpy, and similar repositories.

```sh
rg -n "\b(FileSystemWatcher|Registry|ProcessStartInfo|Process\.Start)\b" .
rg -n "\b(Clipboard|DataPackage|DragDrop|DragEnter|DragOver|Drop)\b" .
rg -n "\b(NotifyIcon|SystemEvents|RegisterHotKey|SetWindowsHookEx)\b" .
rg -n "\b(DllImport|LibraryImport|User32|Kernel32|Shell32|Gdi32)\b" .
rg -n "\b(StorageFile|StorageFolder|IFileOperation|AppInstance|LauncherOptions)\b" .
```

## Cross-Platform Reference Libraries

Use for libuv, wxWidgets, Qt, GLFW, Scintilla, OBS Studio, KeePassXC, and similar references.

```sh
rg -n "\b(ReadDirectoryChangesW|inotify|kqueue|fanotify|uv_fs_event|QFileSystemWatcher)\b" .
rg -n "\b(CreateProcessW|posix_spawn|fork|execvp|uv_spawn|QProcess)\b" .
rg -n "\b(LoadLibrary|GetProcAddress|dlopen|dlsym|uv_dlopen|QLibrary)\b" .
rg -n "\b(CreateNamedPipe|uv_pipe|QLocalServer|QLocalSocket|DBus)\b" .
rg -n "\b(OpenClipboard|SetClipboardData|QClipboard|DoDragDrop|gtk_drag|DataPackage)\b" .
```

## Audit Notes

- Prefer source locations over raw counts.
- Mark examples as "seen" only after inspecting a matching file enough to understand the role.
- Separate app preferences from OS integration settings; both may use registry-like APIs but imply different modules.
- Separate process execution from shell integration; `ShellExecute` often means "open with desktop default" rather than "spawn child process".
- Treat raw device, service, debugger, kernel, and shell-extension APIs as boundary signals unless several ordinary desktop apps share the same need.
