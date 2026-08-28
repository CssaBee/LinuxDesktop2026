# Extended Watchlist Fit Audit

This audit samples the extended product watchlist to check whether the current implementation and roadmap still fit real projects, and where the next feature pressure is coming from.

Audit date: 2026-08-28.

Source clones: `/tmp/linuxdesktop2026-extended-survey`.

## Revisions

| Repository | Revision |
|---|---|
| Carla | `97a9e07` |
| GTR_Framework | `e889164` |
| OpenRGB | `b087a65` |
| PrusaSlicer | `b028299` |
| dylib | `36ac53e` |
| FreeCAD | `9a95906` |
| Project Island | `d45d905` |
| NUT | `6cc9035` |
| OpenSCAD | `121b48e` |
| sample-cpp-plugin | `69e7615` |

## Roadmap Consequences

`ld_settings` is still the right first module, but the extended survey sharpens its remaining scope:

- Keep named roots, component roots, config layers, migration plans, autostart effects, and policy effects in the current ship path.
- Add explicit environment override and legacy fallback reporting to config-layer diagnostics. NUT, FreeCAD, OpenRGB, and Carla all make environment-selected roots or compatibility fallbacks visible in first-party code.
- Treat profile/snapshot/migration behavior as a real application need, not only a Notepad++ proof-case detail. PrusaSlicer and FreeCAD both have multi-root or versioned migration behavior.
- Keep broader desktop integration out of first `ld_settings`. Autostart can remain an effect, but desktop entries, icons, MIME types, URL protocols, and desktop database updates should move to a future desktop-integration module or effects package.

`ld_paths` now deserves first-class planning rather than living only as an internal settings helper:

- Standard user paths need to include more than config/cache/state. OpenSCAD uses document/data roots and parses `user-dirs.dirs`; FreeCAD uses config/data/cache/temp roots; PrusaSlicer and OpenRGB use XDG data/config/autostart locations.
- Executable, resource, install, and runtime roots recur across FreeCAD, PrusaSlicer, OpenSCAD, and OpenRGB.
- Plugin-oriented applications need typed path sets. Carla has separate LADSPA, DSSI, LV2, VST2, VST3, CLAP, SF2, SFZ, and JSFX path defaults, with environment overrides and Wine-prefix behavior.

`ld_process` should move from a broad idea to a scoped design candidate:

- PrusaSlicer shows separate needs for argv-safe process launch, shell command mode, environment inheritance, output capture, wait/exit-status handling, and script interpreter dispatch.
- NUT adds daemon readiness handshakes, inherited markers, environment cleanup, and Windows child process coordination.
- OpenRGB's service startup path is a future boundary signal rather than a reason to put service lifecycle into settings.

`ld_ipc` and single-instance behavior should be promoted after process/path planning:

- PrusaSlicer uses lock ownership plus D-Bus activation forwarding on Linux and `WM_COPYDATA`/window discovery on Windows.
- FreeCAD uses `QLocalServer`/`QLocalSocket` with stale-server recovery for single-instance message forwarding.
- NUT uses Unix sockets and Windows named pipes for daemon control paths.

`ld_watch` still fits the current direction:

- Project Island validates shader/source watching, polling callbacks, and hot-reload-friendly file events.
- The existing `ld_watch` model already covers the broad shape; the useful next survey action is validating Windows/libuv behavior rather than expanding scope.

`ld_dynlib` and plugin loading need a deliberate dependency decision before implementation:

- `dylib` is a strong adopt/wrap/reference candidate for dynamic loading primitives and error vocabulary.
- `sample-cpp-plugin` is a compact proof case for a future plugin-loader sample.
- Project Island, Carla, PrusaSlicer, and FreeCAD show that loader policy must include platform suffixes, symbol lookup, search-path safety, reload behavior, and DLL search-path hardening.

Service and daemon lifecycle should be named as future work:

- NUT and OpenRGB show real service/daemon pressure, including pidfiles, signals, Windows SCM/service event log usage, named-pipe control, and readiness handshakes.
- This should wait until `ld_process` and `ld_ipc` are mature enough to avoid a too-large first service API.

## Repository Findings

### OpenRGB

Relevant source:

- `ResourceManager.cpp`
- `AutoStart/AutoStart-Linux.cpp`
- `AutoStart/AutoStart-Windows.cpp`
- `startup/main_Windows.cpp`
- `startup/main_FreeBSD_Linux_MacOS.cpp`

Fit:

- Strong current fit for `ld_settings`. OpenRGB resolves `APPDATA`, `XDG_CONFIG_HOME`, and `$HOME/.config`, creates an application config directory, and stores JSON settings/profiles under it.
- Strong fit for planned autostart effects. The Linux backend writes XDG Autostart desktop files, and the Windows backend creates startup shortcuts.

Feature implications:

- Desktop-entry escaping needs to be explicit in `ld_desktop`; current `ld_settings` autostart code is temporary prototype evidence.
- Service/headless startup belongs in future service/daemon planning, not the settings module.

### sample-cpp-plugin

Relevant source:

- `psdk/loader.hpp`

Fit:

- Strong current roadmap fit for future `ld_dynlib` and plugin-loader examples. It is small, readable, and directly shows `dlopen`/`LoadLibraryA`, symbol lookup, platform suffix choice, and unload.

Feature implications:

- A future loader should improve diagnostics and recoverability compared with hard assertions on load or symbol failure.
- This is a good tiny sample target once `ld_dynlib` exists.

### dylib

Relevant source:

- `include/dylib.hpp`
- `src/dylib.cpp`

Fit:

- Strong fit as an existing tool scan result for `ld_dynlib`.
- It already covers native handles, `dlopen`/`LoadLibrary`, symbol lookup, extension decorations, enumerating symbols/sections, and typed error cases.

Feature implications:

- Before building `ld_dynlib`, classify `dylib` as adopt, wrap, recommend, reject, or defer.
- If not adopted, its exception vocabulary and decoration model should still inform the API.

### PrusaSlicer

Relevant source:

- `src/slic3r/GUI/DesktopIntegrationDialog.cpp`
- `src/slic3r/GUI/InstanceCheck.cpp`
- `src/libslic3r/GCode/PostProcessor.cpp`
- `src/slic3r/Config/Snapshot.hpp`
- `src/slic3r/Config/Snapshot.cpp`

Fit:

- Strong roadmap changer. It validates settings migration/snapshots, desktop integration, process execution, and single-instance IPC.
- Current `ld_settings` covers some config-root and migration prototype behavior, but ADR 0012 moves stable migration ownership to `ld_migration` and desktop integration to `ld_desktop`.

Feature implications:

- Create or plan a future desktop-integration module for `.desktop` files, icon installation, MIME type registration, URL protocols, `xdg-mime`, and AppImage executable discovery.
- `ld_process` should distinguish argv-safe spawn from shell command mode and support environment passing, output capture, wait/exit codes, and script interpreter behavior.
- `ld_ipc` should split instance ownership from activation forwarding and should consider Linux D-Bus plus Windows window-message backends.

### OpenSCAD

Relevant source:

- `src/platform/PlatformUtils.h`
- `src/platform/PlatformUtils-posix.cc`
- `src/platform/PlatformUtils-win.cc`

Fit:

- Strong fit for `ld_paths` and the current settings/root direction.
- It uses XDG config roots, user document roots, app data roots, executable/resource path helpers, Windows known folders, and runtime OS probing.

Feature implications:

- `ld_paths` should support standard user directories beyond config/cache/state, including XDG user dirs from `user-dirs.dirs`.
- Existing `ld_settings` should avoid pretending that all useful paths are settings paths.

### FreeCAD

Relevant source:

- `src/App/ApplicationDirectories.cpp`
- `src/Gui/GuiApplication.cpp`
- `src/Gui/DocumentRecovery.cpp`

Fit:

- Strong fit for `ld_paths`, settings migration, single-instance IPC, and file-lock/session-state helpers.
- It uses environment-overridden user homes, Qt standard locations, versioned config migration, executable discovery, and Windows DLL search-path hardening.

Feature implications:

- `ld_migration` plans should expose skipped paths and partial failures clearly.
- `ld_paths` should include install/runtime-root helpers and custom-root override reporting.
- Future dynamic loader/process designs should account for DLL search-path safety.

### Carla

Relevant source:

- `source/frontend/carla_shared.py`
- `source/frontend/pluginlist/pluginlistdialog.cpp`

Fit:

- Strong fit for `ld_paths` and future plugin-path policy.
- It has typed plugin search paths for LADSPA, DSSI, LV2, VST2, VST3, CLAP, SF2, SFZ, and JSFX across Linux, Windows, macOS, and Wine-style prefixes.

Feature implications:

- Add a plugin path-set concept to `ld_paths` planning, but keep plugin ABI and plugin hosting separate.
- Path-list separator handling and environment override precedence should be first-class diagnostics.

### Project Island

Relevant source:

- File watching and hot-reload paths in engine/runtime source.
- Dynamic library and reload paths in runtime source.

Fit:

- Strong fit for the current `ld_watch` direction and future `ld_dynlib` policy.
- The project validates hot-reload workflows rather than creating a new first-module requirement.

Feature implications:

- Keep watcher examples aware of source/shader reload use cases.
- Dynamic reload should be planned as a policy layer over loader primitives, not as the first `ld_dynlib` API.

### NUT

Relevant source:

- `clients/authconf.c`
- `clients/upssched.c`
- `server/upsd.c`

Fit:

- Strong roadmap changer for config-layer search, IPC, process, and daemon/service boundaries.
- It uses explicit auth config files, environment-selected paths, XDG per-user config, legacy dotfiles, site defaults, Unix sockets, Windows named pipes, pidfiles, signals, mutexes, and child readiness handshakes.

Feature implications:

- `ld_settings` config-layer reports should include explicit file, explicit directory, user XDG, legacy user file, and site default candidates.
- `ld_ipc` should include local IPC transport evidence beyond single-instance GUI apps.
- Service/daemon lifecycle should become future work after process and IPC are scoped.

### GTR_Framework

Relevant source:

- `src/utils/utils.cpp`
- bundled SDL platform helpers

Fit:

- Good challenge-ecosystem candidate, but limited roadmap-changing evidence in first-party source.
- The first-party code is mostly graphics/shader utility code and simple file IO.

Feature implications:

- Useful for educational modules around resource loading or small portability exercises.
- Does not change the current module priority ordering.

## Feature Needs To Add

Add or update planning docs for these feature needs:

- `ld_paths`: standard user paths, XDG user dirs, executable path, resource/install roots, path-list parsing, environment override reports, legacy fallbacks, Wine-prefix-aware plugin path defaults, and typed plugin path sets.
- `ld_process`: argv-safe spawn, shell command mode, environment block control, working directory, output capture, wait/exit status, script interpreter behavior, readiness handshakes, and daemon-child startup markers.
- `ld_ipc`: lock ownership, stale lock/server recovery, activation forwarding, local sockets, D-Bus session bus, Windows named pipes, and Windows window-message activation.
- `ld_desktop` or desktop integration effects: desktop entry generation, command escaping, icon installation, XDG data root discovery, MIME registration, URL protocol handlers, AppImage executable discovery, and uninstall cleanup.
- `ld_dynlib`: adopt/wrap decision for `dylib`, native handle exposure policy, decoration/suffix handling, symbol diagnostics, DLL search-path safety, and reload policy boundaries.
- Future service/daemon helpers: pidfiles, signal handling, Windows SCM/service event integration, daemon command channels, local-only security settings, and readiness handshakes.
