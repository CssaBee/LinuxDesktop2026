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
| `active` | `ld_settings` | Settings/config sample with root resolution, config-default hydration, ordered writes, backup files, validation before commit, config layers, and opt-in durable writes. Desktop and migration ownership has moved out. |
| `active` | `ld_paths` | Public C++ and C prototype for standard roots, executable/resource/install roots, candidate reports, path lists, typed plugin path sets, deterministic environment hooks, and opt-in directory creation. |
| `active` | `ld_watch` | Public C++ watcher prototype with native Linux `inotify`, native Windows `ReadDirectoryChangesW`, optional libuv backend, bounded pull delivery, recursive-watch diagnostics, and deadline-scheduled settled-file coalescing by path. |
| `active` | `ld_desktop` | C++ and C extraction for autostart and managed/enforced policy. ADR 0015 scopes the next expansion around standards-backed registration artifacts, a preferred desktop-bundle path, individual effect calls, activation plans, uninstall cleanup reports, and Desktop Flavor validation for GNOME, KDE, Xfce, bare window-manager sessions, and Windows 10/11. |
| `active` | `ld_migration` | C++ extraction for dry-run-first application-settings migration. Filesystem execution supports regular files and directories containing regular files/subdirectories; symlinks, special files, ownership, permissions, timestamps, xattrs, ACLs, sparse extents, and hard-link topology are not replicated as filesystem metadata. App-settings Registry snapshot/import/export compatibility is present; broader rollback and adversarial hardening remain before ship-candidate status. |
| `blocked` | Maintained consumer proof | The Notepad++ proof branch exists in `../LinuxDesktop2026-crossport-notepadpp` and has a private GitHub remote at `CssaBee/LinuxDesktop2026-crossport-notepadpp`. Local package-consumption, private-remote, and rebase evidence exists through generated platform defaults, `ld_root` topology use, and product-owned diagnostics. It still needs observed CI and ongoing rebase-cadence evidence before this gate is validated. |

## Validation Status

- Main unit and smoke tests cover settings, paths, desktop, migration, watcher,
  C ABI reports, Rust FFI smoke where `rustc` is available, and install-tree
  consumption.
- FlavorTests cover Notepad++, PrusaSlicer, OpenRGB, KeePassXC, qBittorrent,
  OBS, KiCad, Audacity, FreeCAD, Walnut, and OpenIPC Dashboard.
- FlavorTests now use shared platform-path fixtures instead of product-local
  `#if WIN32` path branches.
- CI covers Ubuntu, Fedora, Windows/MSVC, shared-library Linux builds,
  ASan/UBSan sanitizer lanes, deterministic `ld_watch` ThreadSanitizer coverage,
  FlavorTests, and optional libuv watcher coverage.
- The Notepad++ proof workflow is manual. The proof branch is available on a
  private GitHub remote, but the workflow still needs at least one observed
  green run before it becomes release evidence.

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
- Observe at least one green Notepad++ proof workflow run against the private
  `CssaBee/LinuxDesktop2026-crossport-notepadpp` proof branch.
- Keep recording rebase/dependency/include/link friction for the Notepad++
  proof branch as it is maintained.
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
