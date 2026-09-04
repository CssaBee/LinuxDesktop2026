# 71 - Replace Or Formally Scope Migration JSON Parser

**What to build:** Remove ambiguity around the hand-written Registry JSON
snapshot parser.

**Blocked by:** None.

**Status:** implemented

- [x] Decide whether to adopt a small header-only JSON dependency or formally
  define the exact supported JSON subset.
- [x] Replace ambiguous field-search helpers while keeping the public
  diagnostics/product translation contract.
- [x] If keeping a subset parser, explicitly reject unsupported escapes,
  nesting, Unicode forms, and non-object shapes with diagnostics.
- [x] Add adversarial round-trip and rejection tests for valid-but-tricky JSON
  and malformed snapshots.

## Implementation Note

The implementation keeps a formally scoped parser instead of adding a JSON
dependency. The Registry snapshot JSON format is now documented as a narrow
compatibility format for `linuxdesktop.settings.registry.snapshot.v1`: one
top-level object, one `root` object, and one `values` array of flat value
objects with string fields only.

The old field-search helpers were replaced with a schema-directed parser that
rejects non-object documents, unknown fields, trailing content, unsupported
escapes such as `\/` and `\uNNNN`, scalar values, and extra nesting while
preserving the existing public diagnostic codes used by import paths.

## Review Anchor

The broad review found hand-written JSON parsing in `migration.cpp`; current
tests reject some hostile shapes, but the supported grammar still needs either
a real parser or an explicit contract.
