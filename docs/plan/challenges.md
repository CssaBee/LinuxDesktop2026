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

## Worked Examples

These are intentionally concrete so a student, contributor, or AI coding agent can anchor on the same constraints.

### Example 1: Path Family Resolver

Goal: implement a tiny app or library that resolves all standard user folders for one application identity.

Example source sketch:

```cpp
struct app_identity {
    std::string vendor;
    std::string application;
};

struct path_report {
    std::filesystem::path config;
    std::filesystem::path data;
    std::filesystem::path state;
    std::filesystem::path cache;
    std::filesystem::path documents;
};

path_report resolve_paths(const app_identity& id);

int main() {
    const auto report = resolve_paths({"Acme", "WidgetStudio"});
    std::cout << "config: " << report.config << '\n';
    std::cout << "data: " << report.data << '\n';
    std::cout << "state: " << report.state << '\n';
    std::cout << "cache: " << report.cache << '\n';
    std::cout << "documents: " << report.documents << '\n';
}
```

Expected folders:

- Windows `AppData/Roaming` for config.
- Windows `AppData/Local` for cache and temp-adjacent state.
- Windows `Documents` or `My Documents` for user-visible documents.
- Windows `Downloads`, `Pictures`, `Music`, `Videos`, and `Desktop` known folders where available.
- Linux `XDG_CONFIG_HOME`, `XDG_DATA_HOME`, `XDG_STATE_HOME`, and `XDG_CACHE_HOME`.
- Linux `XDG_DOCUMENTS_DIR`, `XDG_DOWNLOAD_DIR`, `XDG_PICTURES_DIR`, `XDG_MUSIC_DIR`, `XDG_VIDEOS_DIR`, and `XDG_DESKTOP_DIR` when `user-dirs.dirs` is present.
- Linux fallback to `~/.config`, `~/.local/share`, `~/.local/state`, `~/.cache`, and `~/Documents`-style defaults when user dirs are missing.

Anchor tests:

- `HOME` unset versus set.
- `XDG_*_HOME` absolute versus relative versus empty.
- `user-dirs.dirs` present, missing, malformed, and relative.
- Unicode folder names and spaces in paths.
- A portable or application-local mode that intentionally bypasses the OS defaults.

### Example 2: Startup Bootstrapper

Goal: build a small `app start` program that reads settings, migrates old config, validates launch-time configuration, writes logs, and reports what it found.

Example source sketch:

```cpp
int main(int argc, char** argv) {
    const auto paths = resolve_paths({"Acme", "WidgetStudio"});
    const auto settings = load_settings(paths.config / "settings.json");

    if (settings.needs_migration()) {
        migrate_legacy_settings({
            paths.documents / "WidgetStudio" / "config",
            std::filesystem::path{"/opt/widgetstudio/legacy"},
        }, paths.config);
    }

    validate_settings(settings);
    std::filesystem::create_directories(paths.state / "logs");
    write_boot_log(paths.state / "logs" / "startup.log", settings);

    std::cout << "Started using " << paths.config << '\n';
    return 0;
}
```

Expected folders:

- config in `AppData/Roaming/<Vendor>/<App>` or `~/.config/<app>`.
- runtime state in `AppData/Local/<Vendor>/<App>` or `~/.local/state/<app>`.
- cache in `AppData/Local/<Vendor>/<App>/Cache` or `~/.cache/<app>`.
- logs in a clearly named child folder such as `logs/` under state or cache.
- migration inputs from a legacy app folder such as `~/Documents/<App>/config`, `~/AppData`, `/opt/<app>`, or another documented legacy location.

Startup requirements:

- load current settings first,
- migrate one legacy file or directory tree if it exists,
- validate required keys before the main app starts,
- create or repair a log folder,
- emit a path report that explains the source of each resolved directory,
- and fail with a diagnostic if the config is missing, unreadable, relative, or semantically invalid.

Anchor tests:

- first-run with no config,
- existing legacy config that must migrate,
- existing current config that must not be overwritten,
- malformed config that should stop startup,
- log directory creation failure,
- and dry-run mode that shows the expected actions without mutating disk.

## Starter Challenge Ideas

### Implement `ld_paths`

Design a small API that resolves configuration, cache, state, data, executable, and plugin locations correctly on Windows and Linux.

Evaluation should cover Windows known-folder conventions, XDG conventions on Linux, portable/application-local mode, Unicode paths, and temporary-directory tests.

Useful anchor details:

- Windows: `%APPDATA%`, `%LOCALAPPDATA%`, `%PROGRAMDATA%`, `%USERPROFILE%`, `Documents`, `Desktop`, `Downloads`, and other known folders.
- Linux: `XDG_CONFIG_HOME`, `XDG_DATA_HOME`, `XDG_STATE_HOME`, `XDG_CACHE_HOME`, `XDG_RUNTIME_DIR`, and `user-dirs.dirs`.
- Path families: `config`, `data`, `state`, `cache`, `temp`, `documents`, `desktop`, `downloads`, `music`, `pictures`, `videos`, `templates`, `public_share`, `resources`, `plugin_search`.
- Negative cases: relative overrides, missing home directories, inaccessible parents, and malformed environment values.
- Hint: start with a report object that records both the selected path and the rejected candidates, then layer creation helpers on top of that.

### Implement `ld_watch`

Build a common directory and file watcher with Windows `ReadDirectoryChangesW`, Linux `inotify`, normalized events, rename/delete/create/modify behavior, documented recursive semantics, overflow diagnostics, and stress tests.

Useful anchor details:

- watch roots such as `config/`, `plugins/`, `workspace/`, and `logs/`.
- recursive versus non-recursive behavior should be explicit.
- tests should cover create, modify, rename, delete, burst changes, and queue overflow.
- consumers should receive normalized event names instead of raw platform-specific event codes.
- Hint: normalize platform events into a tiny enum plus a rename-pair payload before exposing them to the consumer.

### Implement `ld_dynlib`

Wrap Windows `LoadLibrary`/`GetProcAddress` and Linux `dlopen`/`dlsym` behind RAII lifetime, typed symbol lookup, meaningful diagnostics, and a test plugin compiled as a DLL or shared object.

Useful anchor details:

- search paths can include `plugins/`, `bin/`, `lib/`, and a per-app plugin folder.
- symbol lookup should fail cleanly when the export is missing or has the wrong signature.
- tests should cover success, missing file, missing symbol, unload behavior, and one plugin built from the same source on both platforms.
- Hint: keep the loaded-library handle RAII-only and make symbol lookup return an expected/diagnostic result instead of a raw pointer.

### Build A Hot-Reload Plugin Host

Combine path resolution, file watching, dynamic library loading, and a small plugin contract. The host watches a plugin directory and safely reloads a module after recompilation.

Useful anchor details:

- a watched folder such as `plugins/dev/` or `build/plugins/`.
- a stable plugin ABI with versioning and explicit entry points.
- state files or cache entries should live outside the plugin directory so rebuilds do not erase runtime state.
- reload behavior should define what happens to in-flight work, failed reloads, and plugin initialization failure.
- Hint: let the host own the plugin lifecycle, but make the plugin itself stateless or explicitly resettable.

### Migrate A Windows-Assuming Starter App

Provide a small application with intentional Windows assumptions. The task is to move operating-system behavior behind LinuxDesktop2026 APIs until the same source builds and passes tests under Visual Studio/MSVC on Windows, GCC on Linux, and Clang on Linux.

Useful anchor details:

- the app should initially use hard-coded `AppData`, `Documents`, or `Desktop` paths.
- it should read a config file, derive a cache folder, and write a log file during startup.
- the migration should remove direct platform branching from the consumer application.
- acceptance should be based on same-source builds and path-report tests rather than a visual UI.
- Hint: move the first OS-specific `#ifdef` behind one thin adapter at a time, then delete the branching from `main()` once tests cover the new boundary.

## Useful Demonstration Tracks

- **Legacy desktop portability**: replace individual Win32 infrastructure dependencies in a Windows-heavy application.
- **Education**: prove the same source works on a Windows student machine and a Linux university build or grading environment.
- **Plugins and hot reload**: prove `ld_paths + ld_watch + ld_dynlib + ld_plugin` together.
- **Hardware companion software**: prove LinuxDesktop2026 is useful around hardware without pretending kernel drivers are portable.

## Suggested Challenge Specs

When writing a new challenge, include these details so the scope is easy to test:

- exact application identity, for example `Vendor = Acme`, `Application = WidgetStudio`;
- exact folders the challenge is expected to touch, for example `config/`, `state/`, `cache/`, `logs/`, `plugins/`, `Documents/`, and `Downloads/`;
- the OS-specific environment variables under test, for example `APPDATA`, `LOCALAPPDATA`, `HOME`, `XDG_CONFIG_HOME`, and `XDG_DOCUMENTS_DIR`;
- whether the challenge is allowed to create directories or must stay read-only;
- how to treat relative, empty, missing, or malformed path overrides;
- whether the test should use a temporary home directory, a temporary documents directory, or a fully isolated fake filesystem tree;
- whether the challenge needs Windows Documents/My Documents compatibility, XDG user-dir compatibility, or both;
- whether logs, config, and cache should all be separate locations or intentionally co-located for the exercise;
- and what a successful report must contain, such as selected path, rejected candidates, and diagnostics.

Good submissions can become examples, tests, reference implementations, or upstream module work after review.
