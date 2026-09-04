# `ld_migration` Extraction Requirements

Status: initial C++ extraction complete; additional hardening remains required
before ship-candidate status.

`ld_migration` owns planning, explaining, executing, and reporting application
settings state moves. `linuxdesktop::settings` keeps only the settings-specific
API; the owning implementation and Registry snapshot/import/export
compatibility data live in `ld_migration`.
The old `ld_settings` migration and Registry facade has been removed.

## Scope

The extracted module covers these responsibility groups at the C++ ownership
boundary:

- migration planning,
- regular-file copy and atomic-rename move execution,
- directory copy and best-effort copy-then-source-cleanup move execution,
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
- Treat the Registry JSON snapshot format as a narrow compatibility format, not
  as arbitrary JSON. Version `linuxdesktop.settings.registry.snapshot.v1`
  accepts one top-level object with exactly `format`, `root`, and `values`;
  `root` is an object with string `hive`, `subkey`, and `view`; `values` is an
  array of flat objects with string `key_path`, `name`, `type`, and `data_hex`.
  The parser accepts only strings, objects, and arrays used by that schema.
  Supported string escapes are `\"`, `\\`, `\n`, `\r`, and `\t`; Unicode,
  slash, backspace/form-feed escapes, scalar literals, unknown fields, trailing
  content, and extra nesting are rejected with Registry JSON diagnostics.
- Treat filesystem migration as application settings migration, not rsync-grade
  filesystem replication. Supported sources are regular files and directories
  containing regular files or subdirectories. Symlinks and special files are
  rejected. File content is copied, but ownership, permissions, timestamps,
  xattrs, ACLs, sparse extents, and hard-link topology are not replicated as
  filesystem metadata.
- Treat file moves as atomic rename operations. Cross-device copy/remove
  fallback is not supported.
- Treat directory moves as best-effort copy plus source-tree cleanup. Partial
  copy failures block the action before cleanup; cleanup failures are reported
  with rollback details where the copied target can be removed. Concurrent
  source mutation, destination substitution, disk-full behavior, and complete
  rollback remain outside the supported guarantee.

## Validation Required

Before `ld_migration` is a ship candidate, tests and examples must cover:

- dry-run plans for every action kind,
- file and directory copy/move success paths within the supported object model,
- missing source, wrong source kind, existing target, and parent creation
  failures,
- hostile paths, including relative escape attempts and target collisions,
- destructive action denial by default,
- explicit permission paths for dangerous actions,
- partial-failure reporting, including failed atomic file moves without
  copy/remove fallback,
- rollback reporting for action kinds that can reasonably be reversed,
- JSON and `.reg` snapshot round trips for app-settings Registry compatibility,
- import denial without explicit permission,
- at least one real consumer integration that migrates settings state.

## Extraction Rule

`linuxdesktop::migration::plan_migration`,
`linuxdesktop::migration::execute_migration_plan`, and the registry helpers in
`linuxdesktop/migration.hpp` are the current pre-1.0 migration entry points.
New C++ callers should include `linuxdesktop/migration.hpp` and use
`linuxdesktop::migration` directly. There is no `ld_settings` migration
compatibility layer.
