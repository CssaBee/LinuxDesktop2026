# 120 - Define Versioned Settings Lock Domain

**What to build:** Make the versioned settings write lock domain explicit and
correct for symlink or alias configurations.

**Blocked by:** None.

**Status:** pending

- [ ] Decide whether participating versioned settings writers coordinate by
  lexical path or by underlying filesystem identity.
- [ ] If lexical-path scoped, document the limitation prominently in the
  versioned settings commit contract.
- [ ] If file-identity scoped, canonicalize the lock domain where feasible and
  define behavior for missing targets, broken symlinks, and platforms without a
  stable equivalent.
- [ ] Add Linux tests for two lexical aliases that resolve to the same settings
  file.
- [ ] Add `O_CLOEXEC` or platform equivalent to the sidecar lock open path where
  applicable.
- [ ] Update diagnostics if aliasing can still weaken the expected concurrency
  guarantee.

## Review Anchor

The September 6 clean re-review found that versioned settings locking derives
the sidecar lock from `absolute(path).lexically_normal()`. Two lexical aliases
to the same underlying file can therefore take different locks while both
believe they participate in the versioned-write protocol.

## Evidence Fit

Filesystem alias tests plus source review should catch this: the risk is a
correctness guarantee that holds for one pathname but not for the file identity
callers may think is protected.

## Release Gate

Blocks `0.2.1`.
