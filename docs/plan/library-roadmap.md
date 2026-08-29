# Platform Library Roadmap

The platform libraries are general-purpose, permissively licensed, and designed for both humans and AI coding agents. The repository is currently a prototype hardening effort, not a production-ready platform framework.

## Initial Direction

- Start in a monorepo with separate modules.
- Support Windows 10/11 and Ubuntu LTS first.
- Keep other Linux distributions best-effort in phase one.
- Do not promise macOS support in phase one.
- Use CMake consumption paths: FetchContent, add_subdirectory, and installed package configuration.
- Include examples for each capability and integration examples that combine modules.

## API Principles

- Prefer a portable core with explicit capability reporting.
- Keep public concepts small and ownership rules clear.
- Preserve future Rust bindings or reimplementation as a design consideration.
- Avoid GUI toolkit dependencies outside UI-facing modules.
- Use existing tools when they are healthy, small, permissively licensed, and fit the API.
- Extract shared vocabulary only when at least two modules need it; start with diagnostics before file watching.
- Split settings/config, paths, process launching, IPC, dynamic loading, and desktop integration into distinct public concepts even when a real application mixes them in one subsystem.
- Use FlavorTests as an ergonomics gate: product-shaped call sites should not
  repeatedly expose LinuxDesktop2026-specific vocabulary for common path,
  defaults, write, migration, or diagnostic translation work. The concrete
  exposure budget lives in `docs/FlavorTests/README.md` and should be applied
  before treating a passing FlavorTest as integration-readiness evidence.

## Staged Execution

1. Complete the initial repository survey.
2. Score module candidates.
3. Run a focused follow-up search for the strongest candidates.
4. Select the first module and code sample.
5. Build the tiny working sample.
6. Make the first sample easy to consume from CMake.
7. Publish once working code exists with supporting docs.

## Current First Module

The first implementation candidate is settings/config.

Decision trail:

- Source and ecosystem survey identified settings/config as a recurring seam.
- The Notepad++ deep pass narrowed the requirement to a settings root resolver plus config bundle manager.
- The existing-library follow-up found specs and libraries to adopt, recommend, or defer.
- ADR 0008 selects this as the first tiny implementation sample.

Original implementation target:

- One CMake library.
- One CLI example.
- Linux XDG behavior first.
- Windows Known Folder behavior shaped in the API, implemented as soon as feasible.
- Structured diagnostic output suitable for humans, tests, and AI agents.

Current prototype sample:

- `ld_core` interface target.
- `LinuxDesktop2026::ld_core` namespaced target.
- Shared C++ diagnostics with `linuxdesktop::settings` aliases preserved.
- `ld_settings` library target.
- `LinuxDesktop2026::ld_settings` namespaced target.
- `ld_settings_demo` executable target.
- Repeatable run with `--settings-dir /tmp/linuxdesktop2026-settings-demo`.
- Config-only sync override for app-validated cloud/sync choices.
- Portable-root denial under app-declared privileged install roots.
- Atomic namespace replacement before committing high-value config files, with opt-in durable writes where the platform supports them.
- `ld_settings_tests` executable target with `ctest` coverage for root priority and write recovery.
- Install/export package files for `find_package(LinuxDesktop2026 CONFIG REQUIRED)`.
- Install-tree consumer smoke test that proves a separate CMake project can link `LinuxDesktop2026::ld_settings`.
- Existing pre-RC C ABI surface for root/layer reports, config hydration, atomic replacement writes, and the current temporary migration/Registry/autostart/policy prototype surface, with explicit ownership and matching free functions.
- Public version constants/functions for C++ and C ABI consumers.
- `ld_desktop` C++ extraction started with Linux XDG Autostart files,
  Linux dconf-compatible managed/enforced policy files, and capability reports
  for the full desktop-effect scope.
- `ld_migration` C++ extraction started with dry-run-first migration planning,
  file/directory copy and move execution, and app-settings Registry
  snapshot/import/export compatibility.
- Temporary `ld_settings` compatibility wrappers no longer expose migration or
  Registry helpers. Autostart and policy moved to `ld_desktop`; migration
  callers use `ld_migration` directly.

Expanded ship direction:

- The current `ld_settings` code is a working first sample, not a finished module.
- Current prototype API scope includes named roots, component roots, portable levels, all config layers, migration plans, Windows Registry-shaped support, Linux equivalents for relevant Registry effects, autostart, and managed/enforced policy. That surface must be narrowed or validated before a ship candidate.
- File associations and protocol handlers are explicitly unsupported in first `ld_settings`; users should open GitHub issues for those effects.
- ADR 0012 is the controlling boundary decision: `ld_settings` keeps settings/config behavior, `ld_paths` owns generic path policy, `ld_desktop` owns desktop integration effects, and `ld_migration` owns migration behavior.
- The expanded survey lives in `docs/survey/settings-registry-app-audit.md` and `docs/survey/settings-registry-platform-equivalents.md`.
- The earlier expanded API inventory lives in `docs/plan/ld-settings-expanded-api.md`; it is prototype evidence, not the final `ld_settings` ownership plan.
- The extended watchlist fit audit lives in `docs/survey/extended-watchlist-fit-audit.md` and keeps broader desktop integration, process/IPC, dynamic loading, and service behavior out of the first `ld_settings` ship scope.
- Path resolution that is not settings-specific lives in `ld_paths`.

Example documentation:

- `docs/examples/migration-examples.md` shows before/after migration shapes for settings adoption, path resolution, plugin path sets, and internal extraction from `ld_settings` to `ld_paths`.

## Near-Term Settings/Config Roadmap

- Keep `ld_settings` tiny and toolkit-neutral.
- Reduce framework tax exposed by FlavorTests before broadening the public
  surface: the API exposure budget in `docs/FlavorTests/README.md`,
  product-boundary report translation, root request builders, migration
  planning helpers, clear config-default naming, and per-flavor friction notes
  now precede adversarial hardening and maintained-branch validation.
- Keep consumer examples for `FetchContent`, `add_subdirectory`, and installed `find_package` tested and agent-readable.
- Keep atomic namespace replacement as the default write behavior while preserving direct-write opt-out for legacy cases; keep durable-write semantics explicit and opt-in rather than implied by replacement alone.
- Verify the shaped Windows backend on Windows, especially Known Folders and atomic replace behavior. Track this in `docs/plan/ld-settings-windows-verification.md`.
- Keep shared C++ diagnostics in the tiny `ld_core` interface target while preserving the shared diagnostic aliases in `ld_settings`, `ld_paths`, and `ld_watch`.
- Keep the first API/ABI stability policy updated in `docs/plan/api-stability.md`.
- Keep the existing C ABI covered by C tests and the conditional Rust FFI smoke test; defer new C ABI expansion until release-candidate status.
- Do not ship Registry, autostart, policy, or migration execution as stable `ld_settings` responsibilities.
- Complete desktop integration effects in `ld_desktop`, including desktop entries, icons, MIME/file associations, default applications, URL protocol handlers, shell-equivalent behavior, desktop database updates, Windows autostart/policy mutation, and Registry-equivalent desktop/system behavior.
- Continue hardening `ld_migration`, including rollback reporting, cross-module orchestration, adversarial path tests, Windows Registry verification, and release-candidate C ABI cleanup.
- Use `docs/plan/ld-desktop-extraction.md` and `docs/plan/ld-migration-extraction.md` as the extraction requirement inventories for tasks 19 and 20.
- Verify the Windows Registry/autostart/policy backend paths as part of the `ld_desktop` and `ld_migration` extraction work before claiming those modules are ready to ship.
- Add explicit environment override, legacy fallback, and config-layer candidate reporting based on the OpenRGB, FreeCAD, Carla, and NUT evidence.
- Treat autostart and policy as `ld_desktop` responsibilities, not as
  `ld_settings` ownership.
- Prepare the first narrow Notepad++ fork patch around `ld_settings` only.

## Current Follow-Up Prototype

File watching remains the strongest follow-up after `ld_settings`. The focused application audit is captured in `docs/survey/file-watcher-audit.md`, the broader application audit is captured in `docs/survey/file-watcher-application-audit.md`, and the broader existing-library follow-up is captured in `docs/survey/file-watcher-library-audit.md`.

The extended watchlist keeps `ld_watch` valid but raises three nearby planning lanes: first-class `ld_paths`, scoped `ld_process`, and scoped `ld_ipc`. These should be planned as separate modules rather than squeezed into settings or watcher APIs.

Current direction:

- working name `ld_watch`,
- broad prototype implemented with native Linux `inotify` first,
- Windows `ReadDirectoryChangesW` shape in the public model,
- optional libuv backend seam added for libuv-shaped apps while native Linux `inotify` remains the default Ubuntu backend,
- efsw kept as the strongest future wrap candidate if the prototype proves direct native implementation too costly,
- e-dant/watcher kept as the strongest compact source/API reference,
- Qt, GLib/GIO, wxWidgets, and .NET `FileSystemWatcher` as recommendations, adapters, or migration references,
- Watchman, fswatch, and Panoptes as study/defer references,
- an `ld_watch` path value instead of bare strings for normal event paths,
- raw events, settled-file trigger, and dirty-path refresh as named layers,
- ADR 0010 as the implementation-ready API boundary,
- public API cleanup, dynamic recursive expansion, nonblocking settled-file scheduling, recursive symlink diagnostics, duplicate recursive directory skipping, multi-client native descriptor fan-out, inotify resource-limit diagnostics, settled-file timeout reporting, remove-watch cancellation, and larger settled-file batches completed in the prototype hardening pass,
- public API stabilization started with backend capability identity, named diagnostic-code constants, `watch_id` equality, timeout-capable pull delivery, optional settled-file timeout policy, and platform-neutral single-file `root_relative` semantics,
- Windows `ReadDirectoryChangesW` backend implementation, Windows smoke-test target, libuv preferred-backend smoke test, and libuv CI job are now in place,
- Windows compatibility issues should be fixed through LinuxDesktop2026 concepts first: `watch_path` for watcher events, `ld_paths` root families and source labels for filesystem locations, `ld_desktop` reports for desktop effects, and future `ld_migration` reports for migration targets,
- and next watcher work focused on keeping real Windows CI/local verification green, deferring `ld_watch` C ABI design to release-candidate status, and adding richer capability fields only if stress tests prove they are needed.

## Next Planned Module: ld_paths

`ld_paths` is the next planned module. The focused survey is captured in `docs/survey/ld-paths-application-audit.md`, and the implementation roadmap is captured in `docs/plan/ld-paths-roadmap.md`.

The module should start resolver-first, but the public prototype should be broad enough to share with the community without feeling like a private spike.

Current public prototype scope:

- Windows 10/11 and Ubuntu LTS first.
- C++17 API backed by shared `ld_core` diagnostics.
- Existing small C ABI for root reports, candidate reports, path-list parsing, and typed plugin path sets; broader C ABI work waits until release-candidate status.
- Standard user paths beyond settings roots: config, data, state, cache, temp, documents, desktop, downloads, music, pictures, videos, templates, and public share.
- Executable path, executable directory, resource root, and install prefix reporting.
- Source-labeled candidate reports for explicit options, environment overrides, XDG base dirs, XDG user dirs, Windows Known Folders, executable-relative paths, legacy fallbacks, site defaults, and generic fallbacks.
- Path-list parsing and joining with platform separators.
- Typed plugin path sets for LADSPA, DSSI, LV2, VST2, VST3, CLAP, SF2, SFZ, and JSFX.
- Wine-prefix-aware defaults where the plugin ecosystem evidence needs them.
- Opt-in directory creation helpers after resolution.
- Install-tree consumer coverage for `LinuxDesktop2026::ld_paths`.

Out of scope for first `ld_paths`:

- macOS support promise,
- settings payload hydration,
- migration copy/move execution,
- desktop entry/icon/MIME/protocol registration,
- process launch,
- IPC,
- dynamic library loading,
- plugin ABI,
- and broad filesystem operations such as watching, indexing, or device/volume management.

Extraction plan:

- Keep `ld_settings` on its current internal root resolver until `ld_paths` has resolver tests and install-tree consumer coverage.
- Then refactor `ld_settings` to consume `ld_paths` for config/data/state/cache/resource placement.
- Keep bundle hydration and ordered writes inside `ld_settings`.
- Finish the `ld_desktop` extraction and remove the remaining desktop-effect
  ownership claims from `ld_settings`.
- Move migration planning/execution and app-settings Registry migration compatibility to `ld_migration`.

## Extended Watchlist Consequences

The extended source survey in `docs/survey/extended-watchlist-fit-audit.md` sampled OpenRGB, sample-cpp-plugin, dylib, PrusaSlicer, OpenSCAD, FreeCAD, Carla, Project Island, NUT, and GTR_Framework.

Near-term planning changes:

- Promote `ld_paths` to the next planned module. It now covers standard user paths beyond settings roots, XDG user-dir parsing, legacy/site config candidates, executable/resource/install roots, path-list parsing, environment override diagnostics, Wine-prefix-aware defaults, typed plugin path sets, a small C ABI, and install-tree consumer coverage. Real Windows verification remains before public prototype announcement.
- Keep `ld_process` as a scoped design candidate with argv-safe spawn, shell command mode, environment control, working directory, output capture, wait/exit status, script interpreter behavior, and readiness handshake support.
- Keep `ld_ipc` as later design work after process/path planning, scoped around lock ownership, stale server recovery, activation forwarding, local sockets, D-Bus, Windows named pipes, and Windows window-message activation.
- Extract desktop integration work into `ld_desktop` for `.desktop` files, command escaping, icon installation, MIME types, file associations, URL protocols, AppImage executable discovery, policy, autostart, Registry-equivalent shell/system behavior, and uninstall cleanup.
- Extract migration work into `ld_migration` for file/directory moves, rollback reporting, app-settings Registry migration compatibility, and later cross-module migration orchestration.
- Keep service/daemon lifecycle work parked until `ld_process` and `ld_ipc` mature enough to support it cleanly.
- Run an existing-tool decision for `dylib` before implementing a dynamic loader module.
