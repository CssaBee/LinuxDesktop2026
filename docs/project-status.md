# Project Status

This document summarizes what LinuxDesktop2026 supports today and what remains
experimental or planned. It is the current release-support page, not a changelog.

## Current Release

LinuxDesktop2026 `0.2.0` is a public prototype release. The code is useful for
evaluating API shape, platform boundaries, and early integration work, but it is
not production-stable and does not promise `1.0` source or binary compatibility.

## Status Legend

- `supported prototype`: implemented behavior with tests, examples, and public
  docs, still allowed to change in later `0.x` releases.
- `experimental`: usable for proof integrations, but not ready to describe as a
  stable module contract.
- `research`: parked until repeated source or consumer evidence justifies API
  design.

## Module Status

| Status | Module | Current state |
| --- | --- | --- |
| `supported prototype` | `ld_core` | Shared C++ diagnostic vocabulary and CMake interface target. |
| `supported prototype` | `ld_settings` | Settings/config roots, explicit root creation, config-default copying, ordered writes, backup files, validation before commit, config layers, opt-in durable writes, versioned whole-file commits, and clear diagnostics for lost-update limitations. Desktop and migration ownership has moved out. |
| `supported prototype` | `ld_paths` | Public C++ and C API for non-mutating standard root resolution, executable/resource/install roots, candidate reports, path lists, typed plugin path sets, deterministic environment hooks, and opt-in directory creation. |
| `supported prototype` | `ld_root` | Root-topology layer over `ld_paths` for portable roots, named roots, component roots, static `named_root_handle` lookup, and `request_builder` construction. |
| `supported prototype` | `ld_watch` | Public C++ watcher API with native Linux `inotify`, native Windows `ReadDirectoryChangesW`, optional libuv backend, bounded pull delivery, recursive-watch diagnostics, and deadline-scheduled settled-file coalescing by path. |
| `experimental` | `ld_desktop` | C++ and C extraction for autostart and managed/enforced policy. Broader desktop bundles, activation plans, cleanup reports, icons, MIME/default-app/protocol handling, and desktop-session validation remain experimental. |
| `experimental` | `ld_migration` | C++ extraction for dry-run-first application-settings migration. Filesystem execution supports regular files and verified directory-tree moves within explicit metadata limits. App-settings Registry snapshot/import/export compatibility is present through a maintained JSON parser and a narrow schema. |
| `experimental` | Maintained consumer proof | The Notepad++ proof branch tracks its private GitHub remote and has an observed green manual GitHub Actions proof against LinuxDesktop2026 `0.2.0`, including the settings/root/migration path and the staged desktop-registration adapter. Exact branch, remote, commit, CI, and maintenance evidence live in `docs/consumer-branches/notepadpp-settings-proof.md`. |

## 0.2.0 Support Matrix

`0.2.0` is a public prototype support line, not a production-stable release. It
clarifies what can be tried today without expanding the project promise beyond
current evidence.

| Area | `0.2.0` support line |
| --- | --- |
| Platforms | Windows 10/11 and Ubuntu LTS are the phase-one targets. Other XDG-like Linux distributions remain best-effort. macOS is not promised. |
| API stability | C++ APIs remain pre-1.0 source-compatibility interfaces that may break on deliberate minor releases. Existing C ABI entry points remain best-effort compatible where practical; new C ABI expansion remains release-candidate-gated. The solid `0.2.0` desktop C ABI subset is autostart and policy effect calls, version functions, and matching reset-after-free report ownership; broader bundle registration stays C++-only experimental. |
| Supported prototype modules | `ld_core`, `ld_settings`, `ld_paths`, `ld_root`, and `ld_watch` may be described as usable prototypes with tests, examples, install-tree consumption evidence, and explicit caveats. `ld_root` supports string-keyed named roots for dynamic maps and `named_root_handle` for static request/lookup pairs. |
| Experimental extraction modules | `ld_desktop` and `ld_migration` remain experimental extraction modules. They may be used by proof integrations, but their broader desktop registration, migration execution, and C ABI surfaces are not release-candidate-stable. |
| Research-only modules | `ld_process`, `ld_ipc`, `ld_dynlib`, service/daemon lifecycle helpers, and GUI/windowing, clipboard, drag-and-drop, and common-dialog helpers remain research-only. |
| Filesystem resolution | Path, root, and settings resolution are non-mutating by default; directory creation and other filesystem mutation remain explicit opt-in behavior. |
| Migration filesystem semantics | Regular-file copy, atomic file rename, and supported directory-tree migration are in scope. Directory moves verify copied content before source cleanup. Cross-device semantic file moves are excluded from `0.2.0`; failed atomic renames keep the existing unsupported-fallback diagnostic unless maintained consumer evidence proves the need for a separate semantic move action. |
| Parser contract | Registry `.reg` compatibility remains a scoped app-settings subset. Registry JSON snapshot parsing must use a maintained JSON parser while preserving the narrow `linuxdesktop.settings.registry.snapshot.v1` schema. |
| Watcher performance | Keep the current watcher path value API through `0.2.0`; local native Linux and synthetic large-tree evidence do not show path construction dominating watcher cost. The large-tree probe distinguishes raw notifications, coalesced product candidates, settled pending work, and queue-saturation drops. Windows native measurements or maintained-consumer data can reopen that after the release. |
| Diagnostics and stringification | Keep module enum `to_string` declarations with out-of-line definitions, and keep `ld_core` diagnostic helpers plus named `ld_paths`/`ld_watch` diagnostic-code constants header-defined through `0.2.0`. Release-candidate hardening may move or generate tables if evidence justifies it. |
| Validation | CI portability, sanitizer lanes, FlavorTests, install-tree consumers, and CI coverage reporting are part of the visible release evidence. |
| Contributor baseline | A `.clang-format` baseline exists for new changes. CI formatting enforcement is deferred until a separate baseline-format pass. Governance/onboarding work remains important but does not block the tag. |

## Validation Status

- Main unit and smoke tests cover settings, paths, desktop, migration, watcher,
  C ABI reports, Rust FFI smoke where `rustc` is available, and install-tree
  consumption.
- Local evidence includes coverage instrumentation, deterministic write
  failure-mode tests, and bounded watcher performance measurements. See
  `docs/validation-evidence.md`.
- FlavorTests cover Notepad++, PrusaSlicer, OpenRGB, KeePassXC, qBittorrent,
  OBS, KiCad, Audacity, FreeCAD, Walnut, OpenIPC Dashboard, Gearcoleco, CtrlrX,
  SmartServoFramework, KickCAT, Amiberry, Endless Sky, and Minifox.
- FlavorTests now use shared platform-path fixtures instead of product-local
  `#if WIN32` path branches.
- CI covers Ubuntu, Fedora, Windows/MSVC, shared-library Linux builds,
  ASan/UBSan sanitizer lanes, deterministic `ld_watch` ThreadSanitizer coverage,
  FlavorTests, and optional libuv watcher coverage.
- The Notepad++ proof workflow is manual and has observed green runs against
  the private crossport repository, including the current LinuxDesktop2026
  `0.2.0` release commit and the crossport commit that requires `0.2.0`. The
  proof covers settings/root/migration behavior plus the staged
  desktop-registration adapter; keep exact run and commit evidence in
  `docs/consumer-branches/notepadpp-settings-proof.md`.

## Public-Claim Boundaries

- The project may say it has working prototypes.
- The project should not claim production stability, full Notepad++ native
  Linux parity, plugin ABI compatibility, broad shell integration, GUI toolkit
  coverage, printing, accessibility, or installer integration.
- `ld_settings` should be described as settings/config only. Desktop effects
  belong to `ld_desktop`; migration belongs to `ld_migration`; generic path
  policy belongs to `ld_paths`.
- Windows compatibility work should happen through LinuxDesktop2026 concepts,
  not through scattered flavor-test or product-test conditionals.

## Planned Validation

- Keep recording rebase/dependency/include/link friction for the Notepad++
  proof branch as it is maintained, especially after LinuxDesktop2026 release
  tags and crossport dependency bumps.
- Complete deeper Windows verification for `ld_paths`, especially UTF-8 paths,
  executable-root behavior, unavailable Known Folder fallback, and plugin
  defaults.
- Continue adversarial parser and filesystem tests for paths, writes, desktop
  effects, and migration actions.
- Keep Desktop Flavor tests hermetic until controlled live desktop-session
  runners exist; flavor coverage should prove honest capabilities and staged
  artifacts, not imply that every session consumes them.
- Keep `ld_watch` native Windows verification green and expand capability fields
  only when tests or maintained consumers prove a need.

## Planned Modules

`ld_process`, `ld_ipc`, `ld_dynlib`, service/daemon lifecycle helpers, and
GUI/windowing, clipboard, drag-and-drop, and common-dialog helpers are
research-only. They need repeated source-anchored integration evidence and an
existing-tool decision before they become active LinuxDesktop2026 modules.
