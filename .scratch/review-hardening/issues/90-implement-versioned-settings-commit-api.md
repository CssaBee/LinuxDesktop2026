# 90 - Implement Versioned Settings Commit API

**What to build:** Implement ADR 0016's narrow C++ versioned settings commit
contract for high-value whole-file settings writes.

**Blocked by:** `89` - Design Versioned Settings Commit Contract.

**Status:** pending

- [ ] Add an opaque file-version token captured from a settings-file read or an
  explicit missing-file state.
- [ ] Add a versioned commit entry point that compares the expected token under
  an internal per-target advisory commit guard before using the existing
  backup/replacement/validation write path.
- [ ] Report stale commits distinctly, leave the target untouched, and keep
  `write_with_backup()` / `write_common_config()` warning that they are not
  lost-update-safe.
- [ ] Keep app merge callbacks, automatic reread/retry, payload-specific merge
  helpers, and new C ABI entry points out of scope.
- [ ] Add deterministic tests for two participating writers, missing-file
  tokens, `session.xml` stale rejection with validation-after-write, and
  `shortcuts.xml` stale rejection before HMAC-source refresh.

## Design Anchor

ADR 0016 accepts the contract because Notepad++ `session.xml` and
`shortcuts.xml` proved stale-write detection is valuable for high-impact
settings files while payload merge semantics remain product-owned.
