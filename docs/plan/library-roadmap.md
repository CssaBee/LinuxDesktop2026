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

Example documentation:

- `docs/examples/migration-examples.md` shows before/after migration shapes for Notepad++ settings roots, Notepad++ config bundle hydration, and ShareX personal path selection.

## Near-Term Settings/Config Roadmap

- Keep `ld_settings` tiny and toolkit-neutral.
- Keep consumer examples for `FetchContent`, `add_subdirectory`, and installed `find_package` tested and agent-readable.
- Keep atomic temp-write/replace as the default write behavior while preserving direct-write opt-out for legacy cases.
- Verify the shaped Windows backend on Windows, especially Known Folders and atomic replace behavior. Track this in `docs/plan/ld-settings-windows-verification.md`.
- Add a first versioned API hygiene pass before broad public promotion.
- Decide whether the C ABI should grow hydration/write wrappers now or stay as a root-resolution seed until a Rust smoke test exists.
- Keep Notepad++ fork work deferred until the module has enough standalone shape.

## Next Candidate Module

File watching remains the strongest follow-up after `ld_settings`. The focused application audit is captured in `docs/survey/file-watcher-audit.md`, and the existing-library follow-up is now started in `docs/survey/file-watcher-library-audit.md`.

Current direction:

- working name `ld_watch`,
- native Linux `inotify` first,
- Windows `ReadDirectoryChangesW` shape in the public model,
- libuv as the strongest reference and possible optional backend,
- Qt, GLib/GIO, and wxWidgets as recommendations or adapters,
- Watchman, efsw, e-dant/watcher, and fswatch as study/defer candidates,
- and an ADR/API sketch before watcher code.
