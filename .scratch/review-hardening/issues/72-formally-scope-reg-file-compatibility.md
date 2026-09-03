# 72 - Formally Scope Reg File Compatibility

**What to build:** Define and test the exact `.reg` import/export subset that
LinuxDesktop2026 supports for application-settings migration.

**Blocked by:** None.

**Status:** pending

- [ ] Document supported `.reg` encodings, value kinds, escaping, continuation
  lines, and unsupported Registry constructs.
- [ ] Reject unsupported forms with diagnostics instead of silently truncating
  or approximating them.
- [ ] Add tests for UTF-16LE/BOM handling, line continuations, binary data,
  multi-string values, escaped key/value names, and unsupported hive syntax.
- [ ] Revisit whether an existing parser is worth adopting after the supported
  subset is written down.

## Review Anchor

The broad review identified bespoke `.reg` parser/serializer behavior as a
data-corruption risk unless the supported subset is explicit and tested.
