# Platform Library Roadmap

The platform libraries are general-purpose, permissively licensed, and designed for both humans and AI coding agents.

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

## Staged Execution

1. Complete the initial repository survey.
2. Score module candidates.
3. Run a focused follow-up search for the strongest candidates.
4. Select the first module and code sample.
5. Build the tiny working sample.
6. Make the first sample easy to consume from CMake.
7. Publish once working code exists with supporting docs.

## Current First Module

The first implementation candidate is settings/config and standard paths.

Decision trail:

- Source and ecosystem survey identified settings/config as a recurring seam.
- The Notepad++ deep pass narrowed the requirement to a settings root resolver plus config bundle manager.
- The existing-library follow-up found specs and libraries to adopt, recommend, or defer.
- ADR 0008 selects this as the first tiny implementation sample.

Next implementation target:

- One CMake library.
- One CLI example.
- Linux XDG behavior first.
- Windows Known Folder behavior shaped in the API, implemented as soon as feasible.
- Structured diagnostic output suitable for humans, tests, and AI agents.

Current sample:

- `ld_core` interface target.
- `LinuxDesktop2026::ld_core` namespaced target.
- Shared C++ diagnostics with `linuxdesktop::settings` aliases preserved.
- `ld_settings` library target.
- `LinuxDesktop2026::ld_settings` namespaced target.
- `ld_settings_demo` executable target.
- Repeatable run with `--settings-dir /tmp/linuxdesktop2026-settings-demo`.
- Config-only sync override for app-validated cloud/sync choices.
- Portable-root denial under app-declared privileged install roots.
- Atomic temp-write/replace before committing high-value config files.
- `ld_settings_tests` executable target with `ctest` coverage for root priority and write recovery.
- Install/export package files for `find_package(LinuxDesktop2026 CONFIG REQUIRED)`.
- Install-tree consumer smoke test that proves a separate CMake project can link `LinuxDesktop2026::ld_settings`.
- C ABI surface for root/layer reports, autostart effects, and managed policy effects, with explicit ownership and matching free functions.
- Public version constants/functions for C++ and C ABI consumers.
- Autostart effect support with Linux XDG Autostart files and Windows `CurrentVersion\Run` backend shape.
- Managed/enforced policy effect support with Linux dconf-compatible defaults/locks and Windows `Software\Policies` backend shape.

Expanded ship direction:

- The current `ld_settings` code is a working first sample, not a finished module.
- First public API scope now includes named roots, component roots, portable levels, all config layers, migration plans, full practical Windows Registry support, Linux equivalents for relevant Registry effects, autostart, and managed/enforced policy.
- File associations and protocol handlers are explicitly unsupported in first `ld_settings`; users should open GitHub issues for those effects.
- The expanded survey lives in `docs/survey/settings-registry-app-audit.md` and `docs/survey/settings-registry-platform-equivalents.md`.
- The proposed API shape lives in `docs/plan/ld-settings-expanded-api.md`.
- The extended watchlist fit audit lives in `docs/survey/extended-watchlist-fit-audit.md` and keeps broader desktop integration, process/IPC, dynamic loading, and service behavior out of the first `ld_settings` ship scope.

Example documentation:

- `docs/examples/migration-examples.md` shows before/after migration shapes for Notepad++ settings roots, Notepad++ config bundle hydration, and ShareX personal path selection.

## Near-Term Settings/Config Roadmap

- Keep `ld_settings` tiny and toolkit-neutral.
- Keep consumer examples for `FetchContent`, `add_subdirectory`, and installed `find_package` tested and agent-readable.
- Keep atomic temp-write/replace as the default write behavior while preserving direct-write opt-out for legacy cases.
- Verify the shaped Windows backend on Windows, especially Known Folders and atomic replace behavior. Track this in `docs/plan/ld-settings-windows-verification.md`.
- Keep shared C++ diagnostics in the tiny `ld_core` interface target while preserving `linuxdesktop::settings` aliases.
- Keep the first API/ABI stability policy updated in `docs/plan/api-stability.md`.
- Grow the remaining C ABI before ship, including migration plans/execution and Registry snapshots/import/export.
- Verify the Windows Registry/autostart/policy backend paths before claiming the expanded settings/effects API is ready to ship.
- Add explicit environment override, legacy fallback, and config-layer candidate reporting based on the OpenRGB, FreeCAD, Carla, and NUT evidence.
- Keep autostart effect support, but move desktop entries beyond autostart, icons, MIME registrations, URL protocols, and desktop database updates to future desktop integration work.
- Prepare the first narrow Notepad++ fork patch around `ld_settings` only.

## Next Candidate Module

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
- public API cleanup, dynamic recursive expansion, nonblocking settled-file scheduling, recursive symlink diagnostics, duplicate recursive directory skipping, multi-client native descriptor fan-out, and inotify resource-limit diagnostics completed in the prototype hardening pass,
- Windows `ReadDirectoryChangesW` backend implementation, Windows smoke-test target, libuv preferred-backend smoke test, and libuv CI job are now in place,
- and next watcher work focused on real Windows CI/local verification plus C/C++ API stabilization.

## Extended Watchlist Consequences

The extended source survey in `docs/survey/extended-watchlist-fit-audit.md` sampled OpenRGB, sample-cpp-plugin, dylib, PrusaSlicer, OpenSCAD, FreeCAD, Carla, Project Island, NUT, and GTR_Framework.

Near-term planning changes:

- Promote `ld_paths` to a first-class planning target. It should cover standard user paths beyond settings roots, XDG user dirs, executable/resource/install roots, path-list parsing, environment override diagnostics, legacy fallbacks, Wine-prefix-aware defaults, and typed plugin path sets.
- Promote `ld_process` to a scoped design candidate with argv-safe spawn, shell command mode, environment control, working directory, output capture, wait/exit status, script interpreter behavior, and readiness handshake support.
- Promote `ld_ipc` after process/path planning, scoped around lock ownership, stale server recovery, activation forwarding, local sockets, D-Bus, Windows named pipes, and Windows window-message activation.
- Add future desktop integration work for `.desktop` files, command escaping, icon installation, MIME types, URL protocols, AppImage executable discovery, and uninstall cleanup.
- Add future service/daemon lifecycle work after `ld_process` and `ld_ipc` mature enough to support it cleanly.
- Run an existing-tool decision for `dylib` before implementing a dynamic loader module.
