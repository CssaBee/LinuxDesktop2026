# `ld_migration` Extraction Requirements

Status: initial C++ extraction complete; additional hardening remains required
before ship-candidate status.

`ld_migration` owns planning, explaining, executing, and reporting application
state moves. `linuxdesktop::settings` keeps only the settings-specific API; the
owning implementation and Registry snapshot/import/export compatibility data
live in `ld_migration`.

## Scope

The extracted module covers these responsibility groups at the C++ ownership
boundary:

- migration planning,
- file copy and move execution,
- directory copy and move execution,
- explicit dry-run previews,
- per-action before/after reporting,
- rollback reporting where practical,
- app-settings Registry snapshot/import/export compatibility,
- compatibility imports from `.reg` and JSON snapshot formats,
- cross-module orchestration once `ld_desktop` exists.

`ld_settings` keeps config-bundle hydration, layer reports, and settings root
metadata. It may provide inputs to migration plans, but it does not own the
stable migration engine. Existing C ABI entry points should move to the owning
module or be described only as documented migration points until release-candidate
cleanup.

## Required API Posture

- Keep plans inspectable before execution.
- Keep dangerous, destructive, privilege-requiring, and global actions behind
  explicit permission flags.
- Report skipped, blocked, unsupported, partially executed, rollback-missing,
  and rollback-failed states per action.
- Keep payload parsing app-owned unless a migration helper explicitly documents
  a supported format.
- Route path selection through `ld_paths`.
- Route desktop effects through `ld_desktop` instead of mixing autostart or
  policy actions into migration execution.
- Treat Registry snapshots as compatibility data for application state, not as a
  general Registry abstraction.

## Validation Required

Before `ld_migration` is a ship candidate, tests and examples must cover:

- dry-run plans for every action kind,
- file and directory copy/move success paths,
- missing source, wrong source kind, existing target, and parent creation
  failures,
- hostile paths, including relative escape attempts and target collisions,
- destructive action denial by default,
- explicit permission paths for dangerous actions,
- partial-failure reporting,
- rollback reporting for action kinds that can reasonably be reversed,
- JSON and `.reg` snapshot round trips for app-settings Registry compatibility,
- import denial without explicit permission,
- at least one real consumer integration that migrates settings state.

## Extraction Rule

`linuxdesktop::settings::plan_migration`,
`linuxdesktop::settings::execute_migration_plan`, and the registry helpers in
`linuxdesktop/migration.hpp` are the current pre-1.0 migration entry points.
New C++ callers should include `linuxdesktop/migration.hpp` and use
`linuxdesktop::migration` directly. Matching C ABI entry points should not be
treated as a permanent `ld_settings` compatibility layer.
