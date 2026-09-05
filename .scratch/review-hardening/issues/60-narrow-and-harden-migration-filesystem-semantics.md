# 60 - Narrow And Harden Migration Filesystem Semantics

**What to build:** Make `ld_migration` honest about what filesystem objects,
metadata, copy/move modes, rollback behavior, and cross-device semantics it
supports before it looks like a generic migration engine.

**Blocked by:** 54 - Reconcile Review Claim And Module Boundary Docs.

**Status:** implemented

- [x] Define the supported object model: regular files, directories, symlinks,
  hard links, special files, sparse files, ownership, permissions, timestamps,
  xattrs, and ACLs.
- [x] Decide whether cross-device file moves are supported through copy plus
  verified remove, rejected up front with diagnostics, or exposed as a caller
  policy choice.
- [x] Specify directory move semantics around partial copy, concurrent
  mutation, destination substitution, disk-full behavior, and cleanup.
- [x] Add tests for cross-device-like failures using injectable or simulated
  filesystem behavior if the local test environment cannot create real mount
  boundaries.
- [x] Ensure dry-run and apply reports expose enough before/after and rollback
  detail for product-owned diagnostics.
- [x] Keep the public wording narrow: application settings migration, not
  rsync-grade filesystem replication.

## Implementation Notes

`ld_migration` now presents filesystem migration as application-settings state
migration. Supported sources are regular files and directories containing
regular files or subdirectories. Symlinks and special files are rejected.
Ownership, permissions, timestamps, xattrs, ACLs, sparse extents, and hard-link
topology are not replicated as filesystem metadata; hard-linked regular files
receive a warning because content can be copied but topology is not preserved.

File migration renames are atomic `rename` operations. Cross-device copy/remove
fallback is not supported; failed rename paths report
`migration-file-rename-failed` and leave rollback unattempted. Directory moves
are best-effort copy plus source cleanup;
plans and executions report `migration-directory-move-best-effort`, and cleanup
failures report rollback details when the copied target can be removed.

## Review Anchor

The review found direct `std::filesystem::rename()` use for file moves and
copy-then-delete directory move behavior, with important semantics still
underspecified for real migrations.
