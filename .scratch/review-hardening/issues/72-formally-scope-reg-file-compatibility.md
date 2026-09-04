# 72 - Formally Scope Reg File Compatibility

**What to build:** Define and test the exact `.reg` import/export subset that
LinuxDesktop2026 supports for application-settings migration.

**Blocked by:** None.

**Status:** implemented

- [x] Document supported `.reg` encodings, value kinds, escaping, continuation
  lines, and unsupported Registry constructs.
- [x] Reject unsupported forms with diagnostics instead of silently truncating
  or approximating them.
- [x] Add tests for UTF-16LE/BOM handling, line continuations, binary data,
  multi-string values, escaped key/value names, and unsupported hive syntax.
- [x] Revisit whether an existing parser is worth adopting after the supported
  subset is written down.

## Implementation Notes

The `.reg` compatibility surface remains a schema-directed subset parser rather
than a general Registry file dependency. The supported subset is documented in
`docs/plan/ld-migration-extraction.md`: UTF-8/ASCII with optional UTF-8 BOM,
UTF-16LE with BOM, one local root hive, quoted/default value names, quoted
strings, `dword:`, `hex:`, `hex(2):`, `hex(7):`, `hex(b):`, and hex-only line
continuations. Unsupported encodings, remote keys, deletion directives,
non-root keys, malformed sections, unsupported escapes, and non-hex
continuations now fail with diagnostics.

## Review Anchor

The broad review identified bespoke `.reg` parser/serializer behavior as a
data-corruption risk unless the supported subset is explicit and tested.
