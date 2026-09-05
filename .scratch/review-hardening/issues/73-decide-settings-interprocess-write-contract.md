# 73 - Decide Settings Interprocess Write Contract

**What to build:** Decide whether `ld_settings` provides multi-process lost
update protection or explicitly does not.

**Blocked by:** None.

**Status:** implemented

- [x] State the current interprocess contract in settings docs and headers.
- [x] Decide protection is out of scope for the current whole-file write API;
  do not add `flock()`/`fcntl()`-style locking to those helpers. Task 89
  separately accepted an internal advisory guard for the versioned settings
  commit path implemented by task 90.
- [x] If protection is out of scope for now, add diagnostics or documentation
  that distinguish atomic corruption-safety from lost-update safety.
- [x] Add tests or a concurrency harness for two writers touching different
  settings in the same file.

## Implementation Notes

`ld_settings` does not currently provide multi-process read-modify-write
lost-update protection. The write helper accepts a complete target file image,
so an app-owned stale read can still overwrite a newer independent setting even
if individual commits are serialized. `write_with_backup()` and
`write_common_config()` now report
`settings-interprocess-lost-update-not-protected` to distinguish atomic
replacement, backup, validation, and optional durable flushing from versioned
commit safety. Tests include a deterministic two-writer stale-payload
simulation. Task 89 accepted a separate pre-1.0 C++ versioned settings commit
contract for high-value files such as Notepad++ `session.xml` and
`shortcuts.xml`: stale participating commits should be rejected through an
opaque file-version token and internal per-target advisory commit guard, while
payload merge semantics stay product-owned. Task 90 implemented that separate
versioned path.

## Review Anchor

The broad review notes that atomic rename prevents corruption but not lost
updates when two processes update one settings file concurrently.
