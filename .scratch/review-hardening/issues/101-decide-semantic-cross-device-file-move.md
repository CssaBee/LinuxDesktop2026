# 101 - Decide Semantic Cross-Device File Move

**What to build:** Decide whether `ld_migration` needs a separate semantic
cross-device file move action instead of broadening atomic `rename_file`.

**Blocked by:** None.

**Status:** implemented

- [x] Keep `rename_file` documented as an atomic rename-only operation.
- [x] Check maintained consumer evidence for a real cross-device settings move
  need.
- [x] If needed later, require a separate action such as `semantic_move_file`
  with copy, verification, source cleanup, rollback reporting, and explicit
  metadata limits.
- [x] If not needed for `0.2.0`, record the exclusion in the support matrix and
  keep the existing diagnostic for unsupported cross-device rename failures.
- [x] Do not add copy/remove fallback silently under the existing atomic rename
  API.

## Result

The `0.2.0` decision is to exclude semantic cross-device file moves. The
maintained consumer ledger and FlavorTest friction notes show settings-file
rename planning, notably the KeePassXC old local config case, but no real
consumer need for copy/remove semantics across filesystems. `rename_file` and
`plan_rename_file()` therefore remain atomic rename-only operations, and the
existing failed-rename diagnostic continues to say that cross-device copy/remove
fallback is unsupported.

If later maintained consumer evidence needs semantic file moves, add a separate
action such as `semantic_move_file` instead of broadening `rename_file`. That
future action must copy, verify copied file content before source cleanup,
report source-cleanup and rollback outcomes, and document that ownership,
permissions, timestamps, xattrs, ACLs, sparse extents, and hard-link topology
are not replicated as metadata unless separately implemented.

## Evidence Fit

Maintained proof plus source review should catch this: the risk is changing a
precise atomic operation into a weaker semantic move without callers opting in.

## Release Gate

Does not block `0.2.0`; maintained consumer evidence did not prove the need.
