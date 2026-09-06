# 101 - Decide Semantic Cross-Device File Move

**What to build:** Decide whether `ld_migration` needs a separate semantic
cross-device file move action instead of broadening atomic `rename_file`.

**Blocked by:** None.

**Status:** pending

- [ ] Keep `rename_file` documented as an atomic rename-only operation.
- [ ] Check maintained consumer evidence for a real cross-device settings move
  need.
- [ ] If needed, design a separate action such as `semantic_move_file` with
  copy, verification, source cleanup, rollback reporting, and explicit metadata
  limits.
- [ ] If not needed for `0.2.0`, record the exclusion in the support matrix and
  keep the existing diagnostic for unsupported cross-device rename failures.
- [ ] Do not add copy/remove fallback silently under the existing atomic rename
  API.

## Evidence Fit

Maintained proof plus source review should catch this: the risk is changing a
precise atomic operation into a weaker semantic move without callers opting in.

## Release Gate

Does not block `0.2.0` unless maintained consumer evidence proves the need.
