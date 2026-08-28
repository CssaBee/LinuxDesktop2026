# LinuxDesktop2026

LinuxDesktop2026 is an early-stage project to study how Windows-heavy desktop applications use platform features, then design small reusable libraries that make native Linux support easier.

The project starts from a practical proof case: Notepad++ on Ubuntu. Notepad++ is not the whole identity of the project, and a complete native Linux port is not assumed. The durable goal is a set of general-purpose platform libraries that GitHub users and AI coding agents can understand, build, and reuse.

## Current Stage

We are in the survey, design, and first-sample stage.

Before writing production library code, we will:

1. Survey 15 to 20 repositories that use Windows desktop features.
2. Include about five reference implementations that already abstracted or ported similar features.
3. Include unported candidates where Linux support was frequently requested.
4. Score candidate modules by real usage, coupling, Linux complexity, standalone usefulness, and proof-case value.
5. Run a focused follow-up search for the strongest module candidates.
6. Pick the first tiny working code sample only after the evidence supports it.

The first selected sample is `ld_settings`, a small C++17 settings/config module. It currently demonstrates root resolution, config-only sync overrides, privileged-install portable denial, config bundle hydration, ordered writes, atomic temp-write/replace, backup files, validation-before-commit, dry-run-first migration plans, raw Registry API shape, Registry JSON/`.reg` snapshot formats, autostart effect handling, and managed/enforced policy effects.

The next module is `ld_paths`, a resolver-first path module shaped by the extended survey. Its public C++ and C prototype is present with a CMake target, tests, demos, and install-tree consumer coverage. It now covers standard user paths, executable/resource/install roots, candidate reports, path lists, typed plugin path sets, opt-in directory creation, and C report ownership rules.

For migration shape examples, see [Migration examples](docs/examples/migration-examples.md). They show how real Notepad++, ShareX, WinSCP, KeePassXC, OpenRGB, FreeCAD, Carla, PortableApps-style startup/config code, and internal `ld_settings` extraction change when platform policy moves into LinuxDesktop2026 modules.

## Roadmap

This roadmap is meant to help both humans and AI agents quickly see what is implemented, what is underway, and what is next.

Status legend:

- `✅ Done`: implemented and documented.
- `🟡 In progress`: actively being explored or refined.
- `📌 Next`: the next selected implementation target.
- `⬜ Later`: useful follow-up work, but not the current focus.

| Status | Category | Current state |
| --- | --- | --- |
| `✅` Done | `ld_settings` | First sample library and demo. Covers root resolution, config-only sync overrides, privileged-install portable denial, config bundle hydration, ordered writes, atomic temp-write/replace, backup files, and validation-before-commit. |
| `✅` Done | `ld_settings_tests` | Test coverage for the current sample behavior through `ctest`. |
| `✅` Done | Docs and ADRs | Roadmap, migration examples, survey notes, and architecture decisions capture the design trail for humans and agents. |
| `✅` Done | CMake consumption | `LinuxDesktop2026::ld_settings` is documented for `FetchContent`, `add_subdirectory`, and installed `find_package` use; CTest verifies an install-tree consumer. |
| `✅` Done | `ld_settings` C ABI surface | Root resolution, root/layer reports, config hydration, atomic writes, migration plans/execution, Registry snapshot/import/export helpers, autostart effects, and managed policy effects are exposed through a small C-compatible API for future Rust bindings and non-C++ consumers. |
| `✅` Done | API/ABI version surface | Public headers expose `0.1.0` version constants/functions and the stability policy defines pre-1.0 compatibility expectations. |
| `✅` Done | Shared diagnostics | `LinuxDesktop2026::ld_core` exposes shared C++ diagnostics, with `ld_settings` aliases kept source-compatible. |
| `✅` Done | `ld_settings` expanded C++ API seed | Named roots, component roots, config layers, portable levels, dry-run migration plans, raw Registry operations, JSON Registry snapshots, `.reg` snapshots, autostart effects, and managed/enforced policy effects are represented in the public C++ surface. |
| `🟡` In progress | `ld_settings` ship design | Expanded survey shows `ld_settings` still needs real Windows Registry/autostart/policy verification, rollback evidence, and a published Rust crate before shipping. |
| `🟡` In progress | Survey and scoring | Repository surveys, ecosystem audits, module scoring, and expanded settings/Registry survey are guiding reusable seams. The broader `ld_watch` application/library follow-up is complete enough to guide implementation. |
| `✅` Done | File watching (`ld_watch`) prototype | Broad prototype exists with public C++ API, named diagnostic constants, backend capability identity, timeout-capable pull delivery, native Linux `inotify`, native Windows `ReadDirectoryChangesW`, optional verified libuv backend, simulated backend tests, smoke coverage, demo, and install-tree consumer linkage. |
| `🟡` In progress | Filesystem and path helpers (`ld_paths`) | Public C++ and C prototype is present with `LinuxDesktop2026::ld_paths`, version constants/functions, shared diagnostics, path family/source enums, resolver reports, deterministic resolver hooks, Linux XDG Base Directory behavior, XDG user-dir parsing, user-directory fallbacks, executable/resource/install roots, legacy and site-default config candidates, opt-in directory creation, path-list parsing/joining, typed plugin path sets, Wine-prefix-aware defaults, tests, demos, and install-tree consumer coverage. Remaining prototype work: Windows verification before public prototype announcement. |
| `📌` Next | `ld_settings` and `ld_paths` extraction | Keep the current `ld_settings` resolver until `ld_paths` has tests and install-tree consumer coverage, then refactor settings root placement through `ld_paths`. |
| `⬜` Later | Process and shell integration | Candidate module for launching commands, shell helpers, and process lifecycle seams. |
| `⬜` Later | Dynamic library loading | Candidate module for loading shared libraries and resolving symbols cleanly. |
| `⬜` Later | Single-instance IPC | Candidate module for app-ownership checks, lock files, local transports, and activation forwarding. |
| `⬜` Later | Desktop integration effects | Candidate module/effects package for desktop entries, icons, MIME types, URL protocols, and desktop database updates. |
| `⬜` Later | Service and daemon lifecycle | Future module for background process supervision, command channels, and service integration. |
| `⬜` Later | GUI / windowing | UI foundation for top-level windows, platform windows, and event plumbing. |
| `⬜` Later | Clipboard | UI-adjacent module for copy/paste integration and capability reporting. |
| `⬜` Later | Drag-and-drop | UI-adjacent module for drop targets, payload inspection, and platform differences. |
| `⬜` Later | Common dialogs and resources | File pickers, message dialogs, icons, and resource access abstractions. |
| `⬜` Later | Printing | Print pipeline and page setup integration. |
| `⬜` Later | Plugin ABI | Binary compatibility, plugin discovery, and host/plugin boundary design. |
| `⬜` Later | Advanced theming and DPI | Theme, scaling, and high-DPI adaptation surfaces. |
| `⬜` Later | Accessibility | Screen reader, focus, and assistive-technology integration. |
| `⬜` Later | Installer and package integration | Packaging, install-time behavior, and distribution-specific integration. |

Quick read:

- `✅` shipped
- `🟡` active
- `📌` queued next
- `⬜` parked for later

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
- The first watcher hardening pass now covers recursive deep-tree creation, single-file save-by-replace follow-up writes, remove/rename churn, and backend/resource-limit diagnostic preservation. Next watcher work is settled-file hardening, then final API decisions after Windows CI reports green.
- The public C++ API now exposes `backend_kind`, `diagnostic_code` constants, `watch_id` equality, and `wait_for`; remaining stabilization is C ABI timing after native verification, Windows single-file root-relative wording, and richer capability fields only if stress tests prove they are needed.

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

- config, data, state, cache, temp, documents, desktop, downloads, music, pictures, videos, templates, and public-share paths,
- executable path, executable directory, resource root, and install prefix,
- explicit options, environment overrides, XDG Base Directory values, XDG user dirs, Windows Known Folders, executable-relative paths, legacy fallbacks, site defaults, and generic fallbacks,
- path lists using platform separators,
- and typed plugin path sets for LADSPA, DSSI, LV2, VST2, VST3, CLAP, SF2, SFZ, and JSFX.

Compatibility matrix:

| Platform | Current prototype status |
| --- | --- |
| Ubuntu LTS / XDG Linux | Covered by deterministic C++ and C tests for Base Directory fallbacks, XDG user-dir parsing, legacy/site config candidates, path lists, directory previews, and plugin search roots. |
| Other XDG-like Linux | Best effort through the same XDG Base Directory, XDG user-dir, and HOME fallback behavior; distro-specific user-dir behavior is not verified yet. |
| Windows 10/11 | Public model includes Known Folder sources, user-dir fallbacks, and Windows plugin defaults, but real Windows runtime verification is still pending. |
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

## Candidate Areas

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
- Desktop integration effects
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
