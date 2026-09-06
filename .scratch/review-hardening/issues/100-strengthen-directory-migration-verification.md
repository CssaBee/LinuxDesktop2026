# 100 - Strengthen Directory Migration Verification

**What to build:** Verify copied directory content before source cleanup in
`ld_migration` directory move execution.

**Blocked by:** `60` - Narrow And Harden Migration Filesystem Semantics.

**Status:** pending

- [ ] Keep directory moves scoped to supported regular-file directory trees;
  do not imply rsync-grade metadata replication.
- [ ] After copy and before source cleanup, verify that every supported source
  entry has a corresponding target entry with expected file content and
  directory shape.
- [ ] Block source cleanup if verification fails and report the failed path or
  verification stage.
- [ ] Preserve existing rollback reporting for cleanup failures after a verified
  copy.
- [ ] Add adversarial tests for partial copy, destination substitution,
  file/directory kind mismatch, and verification failure before cleanup.
- [ ] Update `docs/plan/ld-migration-extraction.md` so the directory move
  contract distinguishes verified content copy from unsupported metadata or
  concurrent mutation guarantees.

## Evidence Fit

Adversarial tests plus source review should catch this: the risk is deleting
source settings after an incomplete copied tree.

## Release Gate

Blocks `0.2.0`.
