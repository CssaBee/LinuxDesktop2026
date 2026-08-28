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

The first selected sample is `ld_settings`, a small C++17 settings/config and standard-paths module. It currently demonstrates root resolution, config-only sync overrides, privileged-install portable denial, config bundle hydration, ordered writes, atomic temp-write/replace, backup files, validation-before-commit, dry-run-first migration plans, raw Registry API shape, Registry JSON/`.reg` snapshot formats, autostart effect handling, and managed/enforced policy effects.

For migration shape examples, see [Migration examples](docs/examples/migration-examples.md). They show how real Notepad++, ShareX, WinSCP, KeePassXC, and PortableApps-style startup/config code changes when platform policy moves into `ld_settings`.

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
| `✅` Done | `ld_settings` C ABI surface | Root resolution, root/layer reports, autostart effects, and managed policy effects are exposed through a small C-compatible API for future Rust bindings and non-C++ consumers. |
| `✅` Done | API/ABI version surface | Public headers expose `0.1.0` version constants/functions and the stability policy defines pre-1.0 compatibility expectations. |
| `✅` Done | Shared diagnostics | `LinuxDesktop2026::ld_core` exposes shared C++ diagnostics, with `ld_settings` aliases kept source-compatible. |
| `✅` Done | `ld_settings` expanded C++ API seed | Named roots, component roots, config layers, portable levels, dry-run migration plans, raw Registry operations, JSON Registry snapshots, `.reg` snapshots, autostart effects, and managed/enforced policy effects are represented in the public C++ surface. |
| `🟡` In progress | `ld_settings` ship design | Expanded survey shows `ld_settings` still needs Windows Registry/autostart/policy verification, C ABI coverage for migration/Registry concepts, and rollback evidence before shipping. |
| `🟡` In progress | Survey and scoring | Repository surveys, ecosystem audits, module scoring, expanded settings/Registry survey, and broader `ld_watch` application/library follow-up are guiding reusable seams. |
| `📌` Next | `ld_settings` C ABI expansion | Add C ABI coverage for migration and Registry concepts before calling `ld_settings` shippable. |
| `📌` Next | Notepad++ integration prep | Start mapping the standalone `ld_settings` surface into a small Notepad++ fork patch plan once the settings surface is closer to shippable. |
| `✅` Done | File watching (`ld_watch`) prototype | Broad prototype exists with public C++ API, native Linux `inotify`, native Windows `ReadDirectoryChangesW`, optional verified libuv backend, simulated backend tests, smoke coverage, demo, and install-tree consumer linkage. |
| `⬜` Later | Process and shell integration | Candidate module for launching commands, shell helpers, and process lifecycle seams. |
| `⬜` Later | Dynamic library loading | Candidate module for loading shared libraries and resolving symbols cleanly. |
| `⬜` Later | Filesystem and path helpers | Candidate module for standard user paths, executable/resource roots, path lists, and plugin path sets. |
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
- The optional preferred-libuv path is smoke-tested with `LD2026_WATCH_PREFER_LIBUV=ON` when libuv is available; Windows smoke tests are present but still need a real Windows CI/local run before the Windows backend is called verified.

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
- Process and shell integration
- Dynamic library loading
- Filesystem/path helpers
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
