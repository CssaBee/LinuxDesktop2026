# LinuxDesktop2026 Challenges

LinuxDesktop2026 challenges are practical portability projects built around one or more project modules. They should teach operating-system boundaries, C/C++ API design, CMake consumption, tests, and explicit Windows/Linux behavior without turning consumer code into a pile of platform branches.

## Purpose

- Provide university-sized assignments that match real Windows-to-Linux portability problems.
- Give new contributors a bounded way to understand one module deeply.
- Validate whether the public APIs are clear enough for humans and AI coding agents.
- Produce examples, regression tests, and possible future module implementations.

## Baseline Rules

Each challenge should normally require:

- one public C or C++ API used by shared consumer code,
- native Windows and Linux implementations hidden behind the module boundary,
- CMake build support,
- tests that run on both platforms where possible,
- MSVC plus GCC or Clang verification,
- documentation of meaningful semantic differences,
- no `#ifdef _WIN32` in the challenge consumer or sample application.

## Challenge Ladder

| Level | Challenge | Module pressure |
| --- | --- | --- |
| 1 | Paths and platform conventions | `ld_paths`, `ld_settings` |
| 2 | Settings and portable configuration | `ld_settings` |
| 3 | Native file watching | `ld_watch` |
| 4 | Processes and output capture | `ld_process` |
| 5 | DLL/SO dynamic loading | `ld_dynlib` |
| 6 | Stable plugin ABI | `ld_plugin` |
| 7 | Hot-reloading plugin host | `ld_paths`, `ld_watch`, `ld_dynlib`, `ld_plugin` |
| 8 | Hardware companion utility | `ld_settings`, `ld_paths`, `ld_watch`, `ld_process` |
| 9 | Port a real Windows component | selected module set |
| 10 | Windows developer to Linux CI/grader | selected module set |

## Starter Challenge Ideas

### Implement `ld_paths`

Design a small API that resolves configuration, cache, state, data, executable, and plugin locations correctly on Windows and Linux.

Evaluation should cover Windows known-folder conventions, XDG conventions on Linux, portable/application-local mode, Unicode paths, and temporary-directory tests.

### Implement `ld_watch`

Build a common directory and file watcher with Windows `ReadDirectoryChangesW`, Linux `inotify`, normalized events, rename/delete/create/modify behavior, documented recursive semantics, overflow diagnostics, and stress tests.

### Implement `ld_dynlib`

Wrap Windows `LoadLibrary`/`GetProcAddress` and Linux `dlopen`/`dlsym` behind RAII lifetime, typed symbol lookup, meaningful diagnostics, and a test plugin compiled as a DLL or shared object.

### Build A Hot-Reload Plugin Host

Combine path resolution, file watching, dynamic library loading, and a small plugin contract. The host watches a plugin directory and safely reloads a module after recompilation.

### Migrate A Windows-Assuming Starter App

Provide a small application with intentional Windows assumptions. The task is to move operating-system behavior behind LinuxDesktop2026 APIs until the same source builds and passes tests under Visual Studio/MSVC on Windows, GCC on Linux, and Clang on Linux.

## Useful Demonstration Tracks

- **Legacy desktop portability**: replace individual Win32 infrastructure dependencies in a Windows-heavy application.
- **Education**: prove the same source works on a Windows student machine and a Linux university build or grading environment.
- **Plugins and hot reload**: prove `ld_paths + ld_watch + ld_dynlib + ld_plugin` together.
- **Hardware companion software**: prove LinuxDesktop2026 is useful around hardware without pretending kernel drivers are portable.

Good submissions can become examples, tests, reference implementations, or upstream module work after review.
