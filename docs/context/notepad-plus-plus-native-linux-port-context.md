# Notepad++ Native Linux Port — Investigation Context

## Objective

Investigate what would be required to make Notepad++ run as a **native Linux application**, rather than through Wine.

Upstream repository:

`notepad-plus-plus/notepad-plus-plus`

The upstream project currently describes Notepad++ as running in the MS Windows environment.

The goal is **not to start rewriting the application immediately**. First create an accurate dependency map showing which parts of Notepad++ are portable, which are coupled to Win32, and what architectural changes would enable Linux support.

## What We Already Know

Two Windows dependencies have already been identified:

1. Windows Registry usage
2. Win32-specific window creation and GUI APIs

However, these are only part of the problem.

Scintilla itself should **not automatically be considered a Linux blocker**. Scintilla is cross-platform. The problem is how the Notepad++ `PowerEditor` layer integrates with the Windows implementation of Scintilla and the surrounding Win32 GUI.

The primary investigation target should therefore be:

`PowerEditor/src`

The repository also contains separate `scintilla`, `lexilla`, and `boostregex` trees.

## Known Windows-Specific Areas

The audit should verify and expand this list rather than assuming it is exhaustive.

### GUI and Window Management

Look for:

- `HWND`
- `HINSTANCE`
- `WNDPROC`
- `CreateWindow`
- `CreateWindowEx`
- `DestroyWindow`
- `ShowWindow`
- `SetWindowLongPtr`
- `GetWindowLongPtr`
- `SendMessage`
- `PostMessage`
- `WM_*`
- `WS_*`
- `GWL_*`
- `GWLP_*`

Notepad++'s GUI architecture appears deeply tied to Win32 messages and native Windows controls.

This is expected to be one of the largest Linux-porting problems.

Do not simply add Linux `#ifdef`s around individual calls unless there is a compelling architectural reason.

Identify where a platform-independent interface could separate application logic from window-system implementation.

## Windows Common Controls

Search for dependencies on:

- `comctl32`
- tab controls
- list views
- tree views
- toolbars
- status bars
- tooltips
- image lists
- `TCM_*`
- `LVM_*`
- `TVM_*`

Determine which Notepad++ custom controls are thin wrappers around Win32 controls and which contain substantial application logic that should be retained.

## Windows Resource Files

Audit:

- `.rc`
- `.rc2`
- resource headers
- `DIALOG`
- `DIALOGEX`
- Windows menus
- accelerators
- icons
- `WS_*`
- `BS_*`
- `CBS_*`
- other resource-control definitions

Dialogs such as Find/Replace and Preferences may require substantial GUI migration.

Determine whether their behavior is sufficiently separated from their Windows resource representation.

## Scintilla Integration

Do **not** classify Scintilla itself as Windows-only.

Instead investigate how `ScintillaEditView` and related Notepad++ components instantiate and communicate with Scintilla.

Look for:

- creation of a Win32 `"Scintilla"` window
- `CreateWindowEx`
- Scintilla Windows class registration
- `SendMessage`
- Scintilla direct-function APIs
- HWND assumptions
- notification handling through Windows messages

Determine what would need to change to use Scintilla's Linux/GTK implementation while preserving as much Notepad++ editor logic as possible.

## Registry

Search for:

- `RegOpenKeyEx`
- `RegGetValue`
- `RegQueryValue`
- `RegSetValue`
- `HKEY_*`
- `winreg`
- related APIs

Determine exactly what Notepad++ uses the registry for.

Do not blindly replace every registry call with a Linux equivalent.

Instead consider an abstraction such as:

```cpp
class SettingsBackend
{
public:
    virtual ~SettingsBackend() = default;

    virtual std::optional<std::wstring>
        get(std::wstring_view key) const = 0;

    virtual void
        set(std::wstring_view key,
            std::wstring_view value) = 0;
};
```

Possible implementations:

```text
Win32SettingsBackend
LinuxSettingsBackend
```

Linux settings should preferably follow XDG conventions such as:

```text
$XDG_CONFIG_HOME/notepad-plus-plus/
```

with an appropriate fallback under the user's home directory.

## File Watching

Search for:

- `ReadDirectoryChangesW`
- directory change notification code
- file modification monitoring

Likely mapping:

```text
Windows -> ReadDirectoryChangesW
Linux   -> inotify
```

Consider a platform abstraction such as:

```cpp
class FileWatcher
{
public:
    virtual ~FileWatcher() = default;

    virtual void watch(
        const std::filesystem::path& path) = 0;

    virtual void unwatch(
        const std::filesystem::path& path) = 0;
};
```

Do not implement this yet. First determine how strongly the current file-watching implementation is coupled to Windows handles, threads, messages, and the main GUI.

## Shell Integration

Search for:

- `ShellExecute`
- `SHGet*`
- `shlwapi`
- `shellapi`
- Explorer integration
- COM shell interfaces
- file-properties APIs
- Windows shortcut handling

Possible Linux equivalents may involve:

- `xdg-open`
- desktop portals
- DBus
- freedesktop.org conventions

Determine whether each feature should have a Linux implementation or simply be disabled initially.

## Process Management

Search for:

- `CreateProcess`
- `ShellExecute`
- `%COMSPEC%`
- `cmd.exe`
- PowerShell
- Windows process handles
- Windows pipes

Potential Linux implementations include:

- `posix_spawn`
- `fork` / `exec`
- `$SHELL`

Do not mechanically translate Windows process APIs. Identify the higher-level operation Notepad++ actually needs.

## Graphics and Fonts

Search for:

- `HDC`
- `HFONT`
- `HBITMAP`
- `HICON`
- GDI
- GDI+
- `LOGFONT`
- `ImageList_*`
- drawing APIs

Potential Linux/GTK stack:

```text
GTK
Cairo
Pango
```

Determine how much custom rendering exists outside Scintilla.

## DPI and Themes

Search for:

- DPI APIs
- `uxtheme`
- dark-mode APIs
- Windows version checks related to UI behavior
- theme handles
- system colors

Linux GUI toolkits should normally handle much of this.

Avoid trying to reproduce Windows theme APIs through compatibility wrappers.

## Clipboard and Drag-and-Drop

Search for:

- Windows Clipboard APIs
- OLE
- COM drag-and-drop
- `IDataObject`
- `IDropTarget`

Determine whether GTK or another toolkit can replace these at the GUI abstraction layer.

## Printing

Search for:

- Windows print dialogs
- GDI printing
- printer device contexts

Possible Linux targets:

```text
GTK printing
CUPS
```

Printing is probably not required for the earliest native-Linux prototype.

## Dynamic Libraries

Search for:

- `LoadLibrary`
- `GetProcAddress`
- `FreeLibrary`
- `.dll`

Possible POSIX equivalents:

```text
dlopen
dlsym
dlclose
.so
```

However, do not assume this alone solves plugins.

## Plugin Architecture

Treat the plugin system as a major independent investigation.

Search:

- `Notepad_plus_msgs.h`
- plugin manager
- plugin loading
- Windows messages exposed to plugins
- HWNDs exposed through plugin structures
- Windows-specific types in the public plugin API
- DLL assumptions

The existing plugin ABI may fundamentally depend on Win32.

Determine whether Linux support would require:

1. a new portable plugin ABI,
2. an adaptation layer,
3. a second Linux-specific ABI,
4. or initially running without third-party plugins.

Maintaining source compatibility and maintaining binary compatibility are separate problems. Document both.

## Filesystem and Path Handling

Audit assumptions involving:

- drive letters
- `C:\`
- UNC paths
- backslashes
- case-insensitive paths
- Windows attributes
- Windows file handles
- MAX_PATH
- NTFS-specific behavior
- temporary directories
- executable discovery

Prefer `std::filesystem` where feasible.

Do not assume existing filesystem code is platform-independent merely because it is C++.

## Character Encoding

Audit Windows-specific Unicode assumptions.

In particular inspect use of:

- `WCHAR`
- UTF-16 Windows APIs
- `MultiByteToWideChar`
- `WideCharToMultiByte`
- Windows code pages

Notepad++'s internal encoding behavior must remain correct even if Linux APIs predominantly use UTF-8.

Separate:

```text
document encoding
UI strings
filesystem paths
OS API encoding
```

These are not the same concern.

## Synchronization and IPC

Search for:

- Windows mutexes
- events
- semaphores
- named pipes
- window-message IPC
- shared memory
- single-instance application handling

Potential Linux replacements include:

- POSIX synchronization
- Unix domain sockets
- DBus
- portable C++ synchronization primitives

Identify especially how Notepad++ sends commands/files to an already-running instance.

## Windows Runtime and Exception Handling

Search for:

- SEH
- `__try`
- `__except`
- compiler-specific extensions
- MSVC intrinsics
- Windows-specific pragmas

Separate genuine Windows dependencies from code that merely needs compiler portability fixes.

## Build System

The upstream repository currently includes Visual Studio-oriented build infrastructure and a dedicated build guide.

Audit:

- `.sln`
- `.vcxproj`
- `.vcxproj.filters`
- `.rc`
- `nmake`
- MSVC flags
- Windows SDK assumptions
- architecture-specific configurations

Evaluate:

```text
CMake
Meson
```

Do not create a new build system yet.

First determine what sources could realistically participate in an initial Linux build.

# Proposed Architectural Direction

Avoid turning the codebase into:

```cpp
#ifdef _WIN32
    ...
#elif __linux__
    ...
#endif
```

throughout application logic.

Prefer boundaries such as:

```text
PowerEditor/
    core/

    platform/
        Window
        Dialogs
        Clipboard
        FileWatcher
        Process
        Shell
        Settings
        DynamicLibrary
        IPC

        win32/
        linux/
```

The exact directory organization is not predetermined.

First identify whether suitable boundaries already exist in the current architecture.

Reuse existing abstractions whenever possible.

# GUI Toolkit Decision

Investigate GTK first.

Reason:

Scintilla already has a mature GTK implementation, so using GTK may minimize changes at the editor-widget layer.

However, do not assume GTK is automatically the correct choice.

Compare at least:

```text
GTK
Qt
```

against:

- Scintilla integration complexity
- amount of Notepad++ GUI code reusable
- native Linux integration
- licensing
- build complexity
- maintenance burden
- accessibility
- clipboard/DnD support
- printing
- HiDPI
- themes
- Wayland/X11 behavior

The goal is a native Linux application, not a Win32 emulation layer.

# First Codex Task

Do **not modify source code yet**.

Perform a repository-wide audit.

Start with:

```text
PowerEditor/src
```

Useful searches include:

```bash
rg '#include.*(windows|commctrl|shellapi|shlwapi|uxtheme|wininet)' PowerEditor/src

rg '\b(HWND|HINSTANCE|HDC|HICON|HMENU|HFONT|WPARAM|LPARAM|LRESULT)\b' PowerEditor/src

rg '\b(CreateWindowEx|CreateWindow|SendMessage|PostMessage|ShellExecute|LoadLibrary|GetProcAddress)\b' PowerEditor/src

rg '\b(WM_|WS_|SW_|GWL_|GWLP_|TCM_|LVM_|TVM_)' PowerEditor/src

rg '\b(RegOpen|RegGet|RegQuery|RegSet|HKEY_)' PowerEditor/src

rg '\b(ReadDirectoryChangesW|CreateProcess|CreateMutex|CreateEvent)\b' PowerEditor/src
```

These are starting points, not the complete audit.

Also inspect include graphs and types that indirectly introduce Win32 dependencies.

# Required Output

Create:

```text
docs/linux-port-dependency-map.md
```

The report should classify components into approximately:

```text
A. Portable or mostly portable
B. Portable after small compiler/API fixes
C. Requires platform abstraction
D. Requires Linux implementation
E. Deeply coupled to Win32 / likely major rewrite
F. Uncertain — needs further investigation
```

For every significant dependency include:

| Field | Description |
|---|---|
| Component | Subsystem/module |
| Files | Relevant source files |
| Windows dependency | API/library/type |
| Purpose | Why Notepad++ uses it |
| Coupling | Low / Medium / High |
| Linux equivalent | Candidate replacement |
| Strategy | Keep / abstract / replace / rewrite |
| Dependencies | Other migration work required |
| Prototype priority | Early / Medium / Late |

# Also Produce a Source Inventory

For each major directory under `PowerEditor/src`, estimate:

```text
Portable
Mostly portable
Mixed
Mostly Win32
Win32-specific
```

Do not classify based solely on filenames.

Inspect actual includes and implementation.

# Dependency Graph

Create a high-level dependency graph showing relationships such as:

```text
Application logic
       |
       +------ Buffer / document model
       |
       +------ Search
       |
       +------ Session/config
       |
       +------ Scintilla integration
       |             |
       |             +---- Win32 frontend
       |
       +------ GUI controls
       |             |
       |             +---- Win32
       |
       +------ File watcher
       |             |
       |             +---- ReadDirectoryChangesW
       |
       +------ Platform services
       |             |
       |             +---- Registry
       |             +---- Shell
       |             +---- Process
       |             +---- Clipboard
       |
       +------ Plugin system
                     |
                     +---- Win32 messages
                     +---- HWND
                     +---- DLL ABI
```

Replace this conceptual diagram with one based on the actual repository.

# Identify the First Viable Linux Milestone

After completing the dependency audit, recommend the smallest meaningful native-Linux milestone.

For example, something approximately equivalent to:

```text
native Linux executable
        |
        +-- GTK window
        +-- Scintilla editor
        +-- open file
        +-- edit text
        +-- save file
        +-- basic menu
        +-- no plugins initially
        +-- no printing initially
        +-- reduced preferences
```

But derive the actual milestone from the source architecture.

Do not assume that preserving the current Notepad++ main-window implementation is necessary for milestone 1.

# Important Constraints

1. Do not modify code during the first audit.
2. Do not attempt to make the entire project compile by adding hundreds of conditional compilation directives.
3. Do not replace Scintilla unless evidence demonstrates it is necessary.
4. Separate application logic from OS integration.
5. Preserve Windows behavior where possible.
6. A Linux port should not require Wine.
7. Prefer incremental, reviewable architectural changes.
8. Identify testable migration boundaries.
9. Distinguish source compatibility from binary compatibility for plugins.
10. Record uncertain conclusions rather than guessing.

# Questions the Audit Must Answer

At minimum:

1. What percentage of `PowerEditor/src` is tightly coupled to Win32?
2. Which modules could compile under Linux with little or no modification?
3. Where are Win32 types leaking into otherwise portable application logic?
4. Which existing classes already function as useful platform abstractions?
5. What new abstraction boundaries would provide the greatest benefit?
6. Can Notepad++ application logic reasonably operate on top of Scintilla GTK?
7. How much GUI behavior is embedded directly in Win32 message handlers?
8. How much behavior is embedded in `.rc` resources?
9. Can file IO be made portable independently of GUI migration?
10. Can file monitoring be abstracted independently?
11. How Windows-specific is session/configuration handling?
12. What is required for single-instance behavior on Linux?
13. What is required for command-line invocation on Linux?
14. Can plugins realistically be deferred until after a working editor exists?
15. What is the smallest useful native-Linux prototype?
16. Which changes could plausibly be upstreamed without disrupting Windows builds?
17. What sequence of changes minimizes large, unreviewable rewrites?

# After the Audit

Stop after producing `docs/linux-port-dependency-map.md`.

Do not begin the Linux port automatically.

Report:

- the five largest blockers,
- the five easiest portability wins,
- the recommended first abstraction,
- the recommended GUI strategy,
- the proposed milestone sequence,
- major architectural risks,
- and any assumptions that need human confirmation.

Wait for review before modifying production code.