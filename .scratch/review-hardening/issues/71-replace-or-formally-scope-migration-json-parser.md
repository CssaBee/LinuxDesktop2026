# 71 - Replace Or Formally Scope Migration JSON Parser

**What to build:** Remove ambiguity around the hand-written Registry JSON
snapshot parser.

**Blocked by:** None.

**Status:** pending

- [ ] Decide whether to adopt a small header-only JSON dependency or formally
  define the exact supported JSON subset.
- [ ] If adopting a dependency, replace character-scanning helpers and keep the
  public diagnostics/product translation contract.
- [ ] If keeping a subset parser, explicitly reject unsupported escapes,
  nesting, Unicode forms, and non-object shapes with diagnostics.
- [ ] Add adversarial round-trip and rejection tests for valid-but-tricky JSON
  and malformed snapshots.

## Review Anchor

The broad review found hand-written JSON parsing in `migration.cpp`; current
tests reject some hostile shapes, but the supported grammar still needs either
a real parser or an explicit contract.
