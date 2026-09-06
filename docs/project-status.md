# Project Status

This document is the implementation ledger for the current repository state.
The README stays public-facing; detailed progress and caveats live here.

## Current Stage

LinuxDesktop2026 is in prototype hardening, FlavorTest review, and maintained
consumer validation. The code is useful for evaluating API shape and platform
boundaries, but it is not a production-stable release.

## Status Legend

- `done`: implemented prototype behavior with local tests or documentation.
- `active`: implementation or extraction is underway.
- `blocked`: waiting for evidence before it can be treated as validated.
- `research`: parked until the activation gate is met.

## Module Status

| Status | Module | Current state |
| --- | --- | --- |
| `done` | `ld_core` | Shared C++ diagnostic vocabulary and CMake interface target. |
| `active` | `ld_settings` | Settings/config sample with root resolution, config-default copying, ordered writes, backup files, validation before commit, config layers, opt-in durable writes, and explicit diagnostics that atomic replacement does not protect multi-process read-modify-write flows from lost updates. Desktop and migration ownership has moved out. |
| `active` | `ld_paths` | Public C++ and C prototype for standard roots, executable/resource/install roots, candidate reports, path lists, typed plugin path sets, deterministic environment hooks, and opt-in directory creation. |
| `active` | `ld_watch` | Public C++ watcher prototype with native Linux `inotify`, native Windows `ReadDirectoryChangesW`, optional libuv backend, bounded pull delivery, recursive-watch diagnostics, and deadline-scheduled settled-file coalescing by path. |
| `active` | `ld_desktop` | C++ and C extraction for autostart and managed/enforced policy. ADR 0015 scopes the next expansion around standards-backed registration artifacts, a preferred desktop-bundle path, individual effect calls, activation plans, uninstall cleanup reports, and Desktop Flavor validation for GNOME, KDE, Xfce, bare window-manager sessions, and Windows 10/11. |
| `active` | `ld_migration` | C++ extraction for dry-run-first application-settings migration. Filesystem execution supports regular files and directories containing regular files/subdirectories; symlinks, special files, ownership, permissions, timestamps, xattrs, ACLs, sparse extents, and hard-link topology are not replicated as filesystem metadata. App-settings Registry snapshot/import/export compatibility is present; broader rollback and adversarial hardening remain before ship-candidate status. |
| `active` | Maintained consumer proof | The Notepad++ proof branch exists locally, tracks its private GitHub remote, has an observed green manual settings-proof workflow, and now has local desktop-registration proof changes with an upstream-following check. Keep the exact branch, remote, commit, CI, and maintenance evidence in `docs/consumer-branches/notepadpp-settings-proof.md`; this status page reports only the current gate state. |

## 0.2.0 Support Matrix

`0.2.0` is the next public prototype milestone, not a production-stable release.
It should make the support line easier to follow without expanding the project
promise beyond current evidence.

| Area | `0.2.0` support line |
| --- | --- |
| Platforms | Windows 10/11 and Ubuntu LTS are the phase-one targets. Other XDG-like Linux distributions remain best-effort. macOS is not promised. |
| API stability | C++ APIs remain pre-1.0 source-compatibility interfaces that may break on deliberate minor releases. Existing C ABI entry points remain best-effort compatible where practical; new C ABI expansion remains release-candidate-gated. |
| Supported prototype modules | `ld_core`, `ld_settings`, `ld_paths`, and `ld_watch` may be described as usable prototypes with tests, examples, install-tree consumption evidence, and explicit caveats. |
| Experimental extraction modules | `ld_desktop` and `ld_migration` remain experimental extraction modules. They may be used by proof integrations, but their broader desktop registration, migration execution, and C ABI surfaces are not release-candidate-stable. |
| Research-only modules | `ld_process`, `ld_ipc`, `ld_dynlib`, service/daemon lifecycle helpers, and GUI/windowing, clipboard, drag-and-drop, and common-dialog helpers remain research-only. |
| Filesystem resolution | Path, root, and settings resolution should be non-mutating by default before `0.2.0`; directory creation and other filesystem mutation must remain explicit opt-in behavior. |
| Migration filesystem semantics | Regular-file and supported directory-tree migration are in scope. Directory moves must verify copied content before source cleanup before `0.2.0`. Cross-device semantic file moves remain excluded unless maintained consumer evidence proves the need. |
| Parser contract | Registry `.reg` compatibility remains a scoped app-settings subset. Registry JSON snapshot parsing must use a maintained JSON parser while preserving the narrow `linuxdesktop.settings.registry.snapshot.v1` schema. |
| Watcher performance | Keep the current watcher path value API through `0.2.0`; native-backend performance measurements can reopen that after the release. |
| Validation | CI portability, sanitizer lanes, FlavorTests, install-tree consumers, and coverage reporting should be visible. Coverage must run in CI before `0.2.0`. |
| Contributor baseline | A `.clang-format` baseline is required before `0.2.0`. Governance/onboarding work remains important but does not block the tag. |

## Validation Status

- Main unit and smoke tests cover settings, paths, desktop, migration, watcher,
  C ABI reports, Rust FFI smoke where `rustc` is available, and install-tree
  consumption.
- Local review-hardening evidence now includes coverage instrumentation,
  deterministic write failure-mode tests, and bounded watcher performance
  measurements. See `docs/validation-evidence.md`.
- FlavorTests cover Notepad++, PrusaSlicer, OpenRGB, KeePassXC, qBittorrent,
  OBS, KiCad, Audacity, FreeCAD, Walnut, and OpenIPC Dashboard.
- FlavorTests now use shared platform-path fixtures instead of product-local
  `#if WIN32` path branches.
- CI covers Ubuntu, Fedora, Windows/MSVC, shared-library Linux builds,
  ASan/UBSan sanitizer lanes, deterministic `ld_watch` ThreadSanitizer coverage,
  FlavorTests, and optional libuv watcher coverage.
- The Notepad++ proof workflow is manual and has one observed green settings
  run against the private crossport repository. The desktop-registration proof
  has local build/CTest evidence plus a clean merge simulation against fetched
  `upstream/master` on 2026-09-06; keep exact run and commit evidence in
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

## Remaining Validation Before Public Prototype Announcement

- Observe green Windows CI after the platform-path fixture cleanup.
- Keep recording rebase/dependency/include/link friction for the Notepad++
  proof branch as it is maintained, and promote the manual proof workflow only
  when the private-repo setup is stable enough to justify that noise.
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

## Ticket State

The active review-hardening ticket order is tracked in
`.scratch/review-hardening/ORDER.md`. Historical ticket numbers are stable, but
execution order follows that file rather than numeric order.

The current `0.2.0` release gate is: replace the migration JSON parser, add
coverage reporting to CI, add the formatting baseline, make filesystem
resolution non-mutating by default, and strengthen directory migration
verification before cleanup.
