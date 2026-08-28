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
- C ABI seed for root resolution, with explicit ownership and one free function.
- Public version constants/functions for C++ and C ABI consumers.

Expanded ship direction:

- The current `ld_settings` code is a working first sample, not a finished module.
- First public API scope now includes named roots, component roots, portable levels, all config layers, migration plans, full practical Windows Registry support, Linux equivalents for relevant Registry effects, autostart, and managed/enforced policy.
- File associations and protocol handlers are explicitly unsupported in first `ld_settings`; users should open GitHub issues for those effects.
- The expanded survey lives in `docs/survey/settings-registry-app-audit.md` and `docs/survey/settings-registry-platform-equivalents.md`.
- The proposed API shape lives in `docs/plan/ld-settings-expanded-api.md`.

Example documentation:

- `docs/examples/migration-examples.md` shows before/after migration shapes for Notepad++ settings roots, Notepad++ config bundle hydration, and ShareX personal path selection.

## Near-Term Settings/Config Roadmap

- Keep `ld_settings` tiny and toolkit-neutral.
- Keep consumer examples for `FetchContent`, `add_subdirectory`, and installed `find_package` tested and agent-readable.
- Keep atomic temp-write/replace as the default write behavior while preserving direct-write opt-out for legacy cases.
- Verify the shaped Windows backend on Windows, especially Known Folders and atomic replace behavior. Track this in `docs/plan/ld-settings-windows-verification.md`.
- Keep shared C++ diagnostics in the tiny `ld_core` interface target while preserving `linuxdesktop::settings` aliases.
- Keep the first API/ABI stability policy updated in `docs/plan/api-stability.md`.
- Grow the C ABI beyond root resolution before ship, including named roots and config-layer reports.
- Implement named roots, component roots, config layers, portable levels, migration plans, Windows Registry support, autostart, and managed/enforced policy before declaring `ld_settings` shippable.
- Verify the Windows Registry/autostart backend paths before claiming the expanded settings/effects API is ready to ship.
- Prepare the first narrow Notepad++ fork patch around `ld_settings` only.

## Next Candidate Module

File watching remains the strongest follow-up after `ld_settings`. The focused application audit is captured in `docs/survey/file-watcher-audit.md`, the broader application audit is captured in `docs/survey/file-watcher-application-audit.md`, and the broader existing-library follow-up is captured in `docs/survey/file-watcher-library-audit.md`.

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
- and next watcher work focused on Windows backend verification, libuv backend verification, and C/C++ API stabilization.
