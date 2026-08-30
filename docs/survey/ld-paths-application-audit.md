# ld_paths Application Audit

This audit turned the extended watchlist path findings into the implementation
survey for `ld_paths`, which is now an active prototype module.

Audit date: 2026-08-28.

Source clones: `/tmp/linuxdesktop2026-extended-survey`.

## Survey Question

Can `ld_paths` become a community-facing prototype without overpromising?

The answer is yes if the first public prototype is a path resolver, not a filesystem manager. It should resolve path families, explain the decision chain, and provide opt-in directory creation helpers only after resolution. It should not own settings payloads, migrations, desktop registration, process spawning, IPC, dynamic loading, or plugin ABI compatibility.

## Evidence Summary

| Repository | Source anchors | Path behavior found | `ld_paths` implication |
|---|---|---|---|
| Notepad++ | `PowerEditor/src/Parameters.cpp` from earlier settings audit | executable root, current directory, AppData settings root, local/portable marker, cloud override, plugin config roots | proof-case value remains high; split root placement from settings file hydration |
| OpenRGB | `ResourceManager.cpp`, `AutoStart/AutoStart-Linux.cpp` | `APPDATA`, `XDG_CONFIG_HOME`, `$HOME/.config`, config/profile roots, XDG autostart root | app config roots and XDG data/config helpers need explicit candidate reporting |
| PrusaSlicer | `src/slic3r/GUI/DesktopIntegrationDialog.cpp`, `src/libslic3r/GCode/PostProcessor.cpp` | AppImage/program location, XDG data roots, applications/icons roots, executable escaping | executable/resource roots belong in `ld_paths`; desktop effects stay out |
| OpenSCAD | `src/platform/PlatformUtils.h`, `src/platform/PlatformUtils-posix.cc`, `src/platform/PlatformUtils-win.cc` | user config path, app data/documents path, XDG user dirs, Windows special folders, resource path | first prototype needs more than config/cache/state; include documents/download-style user dirs |
| FreeCAD | `src/App/ApplicationDirectories.cpp` | `FREECAD_USER_HOME`, `FREECAD_USER_DATA`, `FREECAD_USER_TEMP`, Qt standard locations, executable path, install roots, Windows DLL directory hardening | environment overrides and install/runtime roots need visible diagnostics |
| Carla | `source/frontend/carla_shared.py`, `source/frontend/pluginlist/pluginlistdialog.cpp` | LADSPA, DSSI, LV2, VST2, VST3, CLAP, SF2, SFZ, JSFX defaults, environment path lists, Wine prefix behavior | typed plugin path sets should be a milestone, not an afterthought |
| NUT | `clients/authconf.c` | explicit config file, explicit config directory, XDG per-user config, legacy dotfile, site default | candidate chains need to report explicit, user, legacy, and site defaults |
| Project Island | runtime source from extended audit | source/resource lookup for hot reload and dynamic library workflows | resource roots should be reusable by `ld_watch` and future `ld_dynlib` |
| GTR_Framework | `src/utils/utils.cpp`, bundled SDL helpers | simple file IO, bundled SDL preference paths | useful challenge example; little direct API pressure |

## Existing Tool Scan

`ld_paths` should adopt platform specifications and native APIs rather than depend on a large framework:

| Candidate | Classification | Why |
|---|---|---|
| XDG Base Directory | Adopt as Linux config/data/cache/state behavior | It is the normative Linux desktop basis for app-owned user roots. |
| XDG user-dirs | Adopt for Documents/Desktop/Downloads-style roots | OpenSCAD shows direct value, and the file format is small enough to parse without a GUI stack. |
| Windows Known Folders | Adopt as Windows behavior | It is the modern Windows replacement for most special-folder lookups. |
| Windows executable/module APIs | Adopt internally | Needed for executable path and install/resource root inference. |
| `std::filesystem` | Adopt internally | Good portable path value and probing primitive. |
| Qt `QStandardPaths` | Recommend or adapter | Useful for Qt apps, but not a neutral dependency for a small module. |
| GLib/GIO path helpers | Recommend or adapter | Useful for GTK/GLib apps, but would leak GLib ownership and runtime assumptions. |
| SDL preference/base path helpers | Reference | Good small-app ergonomics, but too narrow for migration diagnostics. |
| Boost.Nowide | Optional recommendation | Useful for Windows UTF-8 command line and file names, but not required for resolver design. |

## Requirement Shape

The first useful module is a resolver with explicit reports:

- app identity based root resolution,
- standard user paths for config, data, state, cache, temp, documents, desktop, downloads, music, pictures, videos, and public share,
- executable path and executable directory,
- resource root and install prefix discovery,
- environment override parsing with source labels,
- legacy fallback chains,
- path-list parsing and joining with platform separators,
- typed plugin path sets,
- Wine-prefix-aware defaults for plugin ecosystems where the source evidence requires it,
- capability and diagnostic reporting,
- and opt-in directory creation after the caller chooses a resolved path.

## Prototype Boundaries

In scope for the community-facing prototype:

- Windows 10/11 and Ubuntu LTS behavior.
- C++17 API plus a small C ABI before public prototype announcement.
- Deterministic test hooks for environment variables, home directories, known folders, executable path, and XDG user-dirs files.
- Install-tree consumer coverage for `LinuxDesktop2026::ld_paths`.
- Migration examples that show app adoption and internal extraction from `ld_settings`.

Out of scope for the first prototype:

- macOS support promise.
- file copy/move migration execution,
- settings payload hydration,
- desktop entry/icon/MIME/protocol registration,
- process launch,
- IPC,
- dynamic loading,
- plugin ABI compatibility,
- and broad filesystem operations such as watching, indexing, volume/device management, or recursive traversal.

## Community Readiness Bar

The module is presentable to the community when it has:

- a documented public API with capability reports,
- CMake target export and install-tree consumer verification,
- Linux and Windows backend tests or explicit Windows verification notes,
- clear failure diagnostics for missing home, malformed environment values, malformed XDG user-dirs entries, unavailable known folders, relative overrides, and ambiguous executable roots,
- examples for Notepad++, OpenRGB, FreeCAD, Carla, and internal `ld_settings` extraction,
- and README positioning that calls it an active prototype without claiming it
  is shipped.
