# 73 - Decide Settings Interprocess Write Contract

**What to build:** Decide whether `ld_settings` provides multi-process lost
update protection or explicitly does not.

**Blocked by:** None.

**Status:** pending

- [ ] State the current interprocess contract in settings docs and headers.
- [ ] If protection is in scope, add `flock()`/`fcntl()`-style locking around
  read-modify-write flows where supported.
- [ ] If protection is out of scope for now, add diagnostics or documentation
  that distinguish atomic corruption-safety from lost-update safety.
- [ ] Add tests or a concurrency harness for two writers touching different
  settings in the same file.

## Review Anchor

The broad review notes that atomic rename prevents corruption but not lost
updates when two processes update one settings file concurrently.
