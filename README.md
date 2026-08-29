# LinuxDesktop2026

LinuxDesktop2026 designs small reusable libraries that make native Linux support easier for Windows-heavy desktop applications. Notepad++ on Ubuntu is the proof case, not the whole project.

## Current Stage

We are in the prototype hardening and flavor-review stage.

`ld_settings` is the current C++17 settings/config sample. It covers root
resolution, config hydration, ordered writes, backup files, validation before
commit, and opt-in durable writes. `ld_desktop` owns autostart and
managed/enforced policy. `ld_migration` owns migration execution and
Registry-shaped compatibility. `ld_settings` stays on settings/config only and
no longer carries migration or Registry helpers.

`ld_paths` is the next module. Its public C++ and C prototype includes a
CMake target, tests, demos, install-tree consumer coverage, standard user
paths, executable/resource/install roots, candidate reports, path lists, typed
plugin path sets, and opt-in directory creation.

For migration shape examples, see [Migration examples](docs/examples/migration-examples.md).

## Flavor Tests

FlavorTests is the adapter-and-test harness for checking API fit at real
upstream seams.

- The harness lives in [docs/FlavorTests/README.md](docs/FlavorTests/README.md).
- It currently covers Notepad++, PrusaSlicer, OpenRGB, KeePassXC, qBittorrent,
  OBS, KiCad, Audacity, and FreeCAD slices.
- Flavor-review feeds back into API ergonomics before the public surface grows.

## Roadmap

This roadmap shows what is done, active, next, and parked.

Status legend:

- `✅ Done`: implemented prototype behavior.
- `🟡 In progress`: active work.
- `📌 Next`: next target.
- `⬜ ideas to inspect`: parked research.

| Status | Category | Current state |
| --- | --- | --- |
| `✅` Done | `ld_settings` | Current settings/config sample: root resolution, config hydration, ordered writes, backups, validation before commit, and opt-in durable writes. |
| `✅` Done | `ld_settings_tests` | `ctest` coverage for current prototype behavior. |
| `✅` Done | Docs and ADRs | Roadmap, migration examples, survey notes, and architecture decisions. |
| `✅` Done | CMake consumption | `FetchContent`, `add_subdirectory`, installed `find_package`, and install-tree smoke coverage. |
| `✅` Done | `ld_settings` C ABI surface | Pre-RC C entry points for root resolution, reports, config hydration, and replacement writes. |
| `✅` Done | API/ABI version surface | `0.1.0` version constants/functions and pre-1.0 source-compatibility policy. |
| `✅` Done | Shared diagnostics | `LinuxDesktop2026::ld_core` shared C++ diagnostics with `ld_settings` aliases. |
| `✅` Done | `ld_settings` expanded C++ API seed | Named roots, component roots, config layers, portable levels, migration plans, Registry snapshots, and effect facades. |
| `🟡` In progress | `ld_desktop` extraction | Autostart and managed/enforced policy live in `ld_desktop`; desktop-effect completion remains. |
| `🟡` In progress | `ld_settings` ship design | `ld_settings` stays scoped to settings/config while desktop and migration move out. |
| `📌` Next | Flavor-derived API ergonomics | FlavorTests gate API ergonomics before broader hardening. |
| `✅` Done | Survey and scoring | Survey, ecosystem, and scoring work shaped module selection. |
| `✅` Done | File watching (`ld_watch`) prototype | Public watcher prototype with native Linux and Windows backends, tests, demos, and install-tree linkage. |
| `🟡` In progress | Filesystem and path helpers (`ld_paths`) | Public C++ and C path prototype with tests, demos, typed plugin path sets, and install-tree coverage. |
| `🟡` In progress | `ld_migration` hardening | Migration planning and execution now live in `ld_migration`; hardening continues. |
| `⬜` ideas to inspect | Process and shell integration | Launching commands, shell helpers, and process lifecycle seams. |
| `⬜` ideas to inspect | Dynamic library loading | Loading shared libraries and resolving symbols. |
| `⬜` ideas to inspect | Single-instance IPC | App-ownership checks, lock files, local transports, and activation forwarding. |
| `⬜` ideas to inspect | `ld_desktop` completion | Desktop entries, icons, MIME/file associations, protocols, and system behavior. |
| `⬜` ideas to inspect | Cross-module migration orchestration | Coordinating path, desktop, and settings migrations. |
| `⬜` ideas to inspect | Service and daemon lifecycle | Background supervision, command channels, and service integration. |
| `⬜` ideas to inspect | GUI / windowing | Top-level windows, platform windows, and event plumbing. |
| `⬜` ideas to inspect | Clipboard | Copy/paste integration and capability reporting. |
| `⬜` ideas to inspect | Drag-and-drop | Drop targets, payload inspection, and platform differences. |
| `⬜` ideas to inspect | Common dialogs and resources | File pickers, message dialogs, icons, and resource access. |
| `⬜` ideas to inspect | Printing | Print pipeline and page setup integration. |
| `⬜` ideas to inspect | Plugin ABI | Binary compatibility, plugin discovery, and host/plugin boundaries. |
| `⬜` ideas to inspect | Advanced theming and DPI | Theme, scaling, and high-DPI adaptation. |
| `⬜` ideas to inspect | Accessibility | Screen reader, focus, and assistive-technology support. |
| `⬜` ideas to inspect | Installer and package integration | Packaging, install-time behavior, and distribution integration. |

Quick read:

- `✅` implemented prototype
- `🟡` active
- `📌` queued next
- `⬜` parked research

## Build The First Sample

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/ld_settings_demo --settings-dir /tmp/linuxdesktop2026-settings-demo
```

The demo uses a temporary settings override so it does not touch your normal application config directory.
It can also simulate Notepad++-style policy seams with `--sync-config-dir`, `--resource-root`, and `--deny-portable-under-root`.
It prints a dry-run migration preview so callers can inspect planned file moves before executing them.

Optional file watcher backend flags:

```sh
cmake -S . -B build -DLD2026_WATCH_ENABLE_LIBUV=ON -DLD2026_WATCH_PREFER_LIBUV=ON
```

`ld_watch` uses native Linux `inotify` by default on Ubuntu and native `ReadDirectoryChangesW` on Windows. If libuv is available through `pkg-config`, the optional libuv backend can be built, exported to installed CMake consumers, and preferred for libuv-shaped applications.

Current backend posture:

- Native backends remain the default because `ld_watch` exposes migration-shaped paths, diagnostics, recursive-policy honesty, and settled-file behavior above raw backend events.
- libuv is recommended directly for applications that already own a libuv loop and only need coarse change/rename notifications.
- The optional preferred-libuv path is smoke-tested with `LD2026_WATCH_PREFER_LIBUV=ON` when libuv is available; Windows smoke tests now cover create, modify, delete, rename, recursive nested creation, and single-file save-by-replace, and CI runs that target explicitly on Windows before the backend is called verified.
- The watcher hardening pass now covers recursive deep-tree creation, single-file save-by-replace follow-up writes, remove/rename churn, backend/resource-limit diagnostic preservation, settled-file timeout reporting, remove-watch cancellation, and larger settled-file batches.
- The public C++ API now exposes `backend_kind`, `diagnostic_code` constants, `watch_id` equality, `wait_for`, optional settled-file timeout policy, and platform-neutral single-file `root_relative` semantics; `ld_watch` C ABI design is postponed until release-candidate status.
- Windows compatibility work should happen at the LinuxDesktop2026 API layer. Tests and examples should use `ld_paths`, `ld_settings`, and `ld_watch` concepts instead of assuming XDG paths, slash-separated strings, or Registry/file-watcher backend details directly.

## `ld_paths`

`ld_paths` is the current module after the `ld_settings` and `ld_watch` prototypes. The committed C++17 prototype includes:

- `LinuxDesktop2026::ld_paths`
- `linuxdesktop/paths.hpp`
- `linuxdesktop/paths_c.h`
- shared `ld_core` diagnostics aliases
- path family and candidate source enums
- resolver options, candidates, and report structures
- small C ABI reports for selected roots, candidates, path-list parsing, and plugin path sets
- deterministic environment, home, temp, and executable path hooks for tests and examples
- Linux XDG Base Directory resolution with HOME fallbacks
- XDG `user-dirs.dirs` parsing with malformed/relative entry diagnostics and HOME fallbacks
- executable path, executable directory, install prefix, resource root, temp, and standard user-directory resolution
- legacy and site-default config candidates in resolver reports
- opt-in directory creation helpers with dry-run defaults
- path-list parsing/joining with platform separators, duplicate filtering, and rejection diagnostics
- typed plugin path sets for LADSPA, DSSI, LV2, VST2, VST3, CLAP, SF2, SFZ, and JSFX
- Wine-prefix-aware plugin defaults for relevant Windows plugin formats
- custom named plugin path sets for app-defined ecosystems
- `ld_paths_tests`, `ld_paths_c_tests`, `ld_paths_demo`, `ld_paths_c_demo`, and install-tree consumer coverage

The resolver does not create directories. Filesystem mutation remains explicit through `ensure_directory`, which defaults to dry-run mode.

Quick start:

```cpp
#include "linuxdesktop/paths.hpp"

namespace ldp = linuxdesktop::paths;

int main()
{
    auto paths = ldp::resolve_app_paths({"LinuxDesktop2026", "example"});
    auto config_preview = ldp::ensure_directory(paths, ldp::path_family::config);

    ldp::plugin_path_options plugin_options;
    plugin_options.kinds = {ldp::plugin_path_kind::lv2, ldp::plugin_path_kind::vst3};
    auto plugins = ldp::resolve_plugin_path_sets(plugin_options);

    return paths.selected.empty() || config_preview.diagnostics.size() > 1 || plugins.sets.empty();
}
```

C callers use `linuxdesktop/paths_c.h`. Reports own their returned strings and arrays; release them with the matching `ld_paths_free_*_report` function.

The prototype should resolve and report:

- config, data, state, cache, runtime, temp, documents, desktop, downloads, music, pictures, videos, templates, and public-share paths,
- executable path, executable directory, resource root, and install prefix,
- explicit options, environment overrides, XDG Base Directory values, XDG user dirs, Windows Known Folders, executable-relative paths, legacy fallbacks, site defaults, and generic fallbacks,
- path lists using platform separators,
- and typed plugin path sets for LADSPA, DSSI, LV2, VST2, VST3, CLAP, SF2, SFZ, and JSFX.

Compatibility matrix:

| Platform | Current prototype status |
| --- | --- |
| Ubuntu LTS / XDG Linux | Covered by deterministic C++ and C tests for Base Directory fallbacks, XDG user-dir parsing, legacy/site config candidates, path lists, directory previews, and plugin search roots. |
| Other XDG-like Linux | Best effort through the same XDG Base Directory, XDG user-dir, and HOME fallback behavior; distro-specific user-dir behavior is not verified yet. |
| Windows 10/11 | Public model includes Known Folder sources, user-dir fallbacks, Windows plugin defaults, and CI coverage for deterministic C/C++ path behavior plus hosted-runner Known Folder selection; real Windows verification still needs UTF-8 path, executable-root, unavailable-folder, and plugin-default depth before public announcement. |
| macOS | No phase-one support promise; API choices should leave room for a later platform backend. |

The first implementation should support Windows 10/11 and Ubuntu LTS, provide C++17 and C entry points, and keep filesystem mutation opt-in.

From a Git checkout or vendored checkout, consumers can already link the target:

```cmake
target_link_libraries(your_app PRIVATE LinuxDesktop2026::ld_paths)
```

The demos print resolved application paths and small plugin-path-set reports:

```sh
./build/ld_paths_demo --org LinuxDesktop2026 --app paths-demo
./build/ld_paths_c_demo
```

## Consume `ld_settings`

From a Git checkout:

```cmake
include(FetchContent)

FetchContent_Declare(
    LinuxDesktop2026
    GIT_REPOSITORY https://github.com/CssaBee/LinuxDesktop2026.git
    GIT_TAG main
)
FetchContent_MakeAvailable(LinuxDesktop2026)

target_link_libraries(your_app PRIVATE LinuxDesktop2026::ld_settings)
```

From a vendored checkout:

```cmake
add_subdirectory(external/LinuxDesktop2026)
target_link_libraries(your_app PRIVATE LinuxDesktop2026::ld_settings)
```

From an installed package:

```sh
cmake -S . -B build -DLD2026_BUILD_EXAMPLES=OFF -DLD2026_BUILD_TESTS=OFF
cmake --install build --prefix /tmp/linuxdesktop2026-prefix
```

```cmake
find_package(LinuxDesktop2026 CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE LinuxDesktop2026::ld_settings)
```

## Research Backlog

These areas are survey and design candidates. They are not active delivery
promises until at least two real integrations validate the need and boundary.

First candidates:

- Settings/config
- File watching
- Filesystem/path helpers
- Process and shell integration
- Dynamic library loading
- Single-instance IPC

UI-adjacent candidates:

- GUI/windowing
- Clipboard
- Drag-and-drop
- Common dialogs/resources

Future work candidates:

- Printing
- Plugin ABI
- Advanced theming/DPI
- Accessibility
- Installer/package integration
- Desktop integration completion through `ld_desktop`
- Migration hardening through `ld_migration`
- Service and daemon lifecycle

## Design Principles

- General-purpose branding: Notepad++ is a proof case, not the product boundary.
- Portable core APIs with explicit capability reporting.
- C++ first, with public API hygiene that keeps future Rust bindings or reimplementation plausible.
- Standard library first; small optional dependencies only when they earn their place.
- GUI toolkit dependencies stay isolated to UI-facing modules.
- CMake consumption should support `FetchContent`, `add_subdirectory`, and installed package configuration.
- Public GitHub publication should wait until there is at least one tiny working code sample.
- A module is not shippable while major accepted scope is documentation-only; development milestones can be useful without being called a finished product.

## Documentation

- [Domain language](CONTEXT.md)
- [Library roadmap](docs/plan/library-roadmap.md)
- [ld_paths roadmap](docs/plan/ld-paths-roadmap.md)
- [ld_settings Windows verification](docs/plan/ld-settings-windows-verification.md)
- [ld_settings C ABI](docs/plan/ld-settings-c-abi.md)
- [API and ABI stability](docs/plan/api-stability.md)
- [Expanded ld_settings API plan](docs/plan/ld-settings-expanded-api.md)
- [`ld_desktop` extraction requirements](docs/plan/ld-desktop-extraction.md)
- [`ld_migration` extraction requirements](docs/plan/ld-migration-extraction.md)
- [Notepad++ proof case plan](docs/plan/notepad-plus-plus-poc.md)
- [Repository survey template](docs/survey/repositories.md)
- [Windows feature matrix](docs/survey/windows-feature-matrix.md)
- [Module priority score](docs/survey/module-priority-score.md)
- [Ecosystem audit](docs/survey/ecosystem-audit.md)
- [Notepad++ settings/config audit](docs/survey/notepad-settings-config-audit.md)
- [Settings/config library follow-up](docs/survey/settings-config-library-audit.md)
- [Settings/Registry app audit](docs/survey/settings-registry-app-audit.md)
- [Settings/Registry platform equivalents](docs/survey/settings-registry-platform-equivalents.md)
- [Extended watchlist fit audit](docs/survey/extended-watchlist-fit-audit.md)
- [ld_paths application audit](docs/survey/ld-paths-application-audit.md)
- [File watcher focused audit](docs/survey/file-watcher-audit.md)
- [File watcher application audit](docs/survey/file-watcher-application-audit.md)
- [File watcher library follow-up](docs/survey/file-watcher-library-audit.md)
- [Migration examples](docs/examples/migration-examples.md)
- [Source search patterns](docs/survey/source-search-patterns.md)
- [Open questions](docs/survey/open-questions.md)
- [Architecture decisions](docs/adr)
- [ADR 0008: start with settings/config module](docs/adr/0008-start-with-settings-config-module.md)
- [ADR 0009: extract shared core diagnostics](docs/adr/0009-extract-shared-core-diagnostics.md)
- [ADR 0010: design the file watcher module](docs/adr/0010-design-file-watcher-module.md)
- [Original investigation context](docs/context/notepad-plus-plus-native-linux-port-context.md)

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE).
