# 112 - Contain Migration Rooted Path Resolution

**What to build:** Ensure `ld_migration::resolve_rooted_path()` cannot resolve
a caller-provided relative path outside the selected root.

**Blocked by:** None.

**Status:** pending

- [ ] Reject absolute relative-path tails.
- [ ] Reject parent traversal that would escape the selected root.
- [ ] Normalize harmless `.` components while preserving a documented symlink
  policy.
- [ ] Verify the final lexical result remains beneath the selected root before
  returning success.
- [ ] Reuse or extract the same containment helper used by hardened root/path
  resolution instead of creating a subtly different implementation.
- [ ] Add adversarial tests for `..`, mixed `.`/`..`, empty paths, absolute
  paths, platform separators, symlink-adjacent paths, and roots with similar
  prefixes.

## Review Anchor

The September 6 clean re-review found that `resolve_rooted_path()` rejects
absolute tails but then joins `selected_root / relative_path` without proving
containment. A request like `../../.ssh/config` can escape an application-owned
configuration root if callers treat the helper as a containment primitive.

## Evidence Fit

Adversarial path tests plus source review should catch this: the risk is a
security-adjacent migration helper whose name implies a stronger guarantee than
the implementation provides.

## Release Gate

Blocks `0.2.1`.
