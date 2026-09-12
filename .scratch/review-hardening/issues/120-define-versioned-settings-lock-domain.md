# 120 - Define Versioned Settings Lock Domain

**What to build:** Make the versioned settings write lock domain explicit and
correct for symlink or alias configurations.

**Blocked by:** None.

**Status:** implemented

- [x] Decide whether participating versioned settings writers coordinate by
  lexical path or by underlying filesystem identity.
- [x] Document the lexical fallback limitation prominently in the versioned
  settings commit contract.
- [x] If file-identity scoped, canonicalize the lock domain where feasible and
  define behavior for missing targets, broken symlinks, and platforms without a
  stable equivalent.
- [x] Add Linux tests for two lexical aliases that resolve to the same settings
  file.
- [x] Add `O_CLOEXEC` or platform equivalent to the sidecar lock open path where
  applicable.
- [x] Update diagnostics if aliasing can still weaken the expected concurrency
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

## Implementation Notes

- Versioned settings tokens now carry the private resolved lock domain used for
  target matching.
- Existing targets derive the sidecar lock from `canonical(target)`, so symlink
  aliases and resolved paths share one advisory guard.
- Missing targets derive the sidecar from `weakly_canonical(target)`, resolving
  existing parent components while keeping the absent filename lexical.
- If weak resolution fails, commits fall back to normalized absolute paths and
  report `settings-version-lock-domain-lexical-fallback`.
- Linux sidecar opens now include `O_CLOEXEC` when the platform exposes it.
- Added Linux symlink alias tests for same-file token compatibility and
  resolved sidecar selection.

## Verification

```text
cmake --build build --target ld_settings_tests
./build/ld_settings_tests
```

## Release Gate

Resolved for `0.2.1`.
