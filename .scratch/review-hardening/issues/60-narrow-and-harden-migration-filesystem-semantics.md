# 60 - Narrow And Harden Migration Filesystem Semantics

**What to build:** Make `ld_migration` honest about what filesystem objects,
metadata, copy/move modes, rollback behavior, and cross-device semantics it
supports before it looks like a generic migration engine.

**Blocked by:** 54 - Reconcile Review Claim And Module Boundary Docs.

**Status:** proposed

- [ ] Define the supported object model: regular files, directories, symlinks,
  hard links, special files, sparse files, ownership, permissions, timestamps,
  xattrs, and ACLs.
- [ ] Decide whether cross-device file moves are supported through copy plus
  verified remove, rejected up front with diagnostics, or exposed as a caller
  policy choice.
- [ ] Specify directory move semantics around partial copy, concurrent
  mutation, destination substitution, disk-full behavior, and cleanup.
- [ ] Add tests for cross-device-like failures using injectable or simulated
  filesystem behavior if the local test environment cannot create real mount
  boundaries.
- [ ] Ensure dry-run and apply reports expose enough before/after and rollback
  detail for product-owned diagnostics.
- [ ] Keep the public wording narrow: application settings migration, not
  rsync-grade filesystem replication.

## Review Anchor

The review found direct `std::filesystem::rename()` use for file moves and
copy-then-delete directory move behavior, with important semantics still
underspecified for real migrations.
