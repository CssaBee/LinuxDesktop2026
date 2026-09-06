# 97 - Replace Migration JSON Parser

**What to build:** Replace the hand-written Registry snapshot JSON parser with
a maintained JSON dependency before `0.2.0`.

**Blocked by:** None.

**Status:** pending

- [ ] Choose the dependency integration shape, preferring deterministic CMake
  consumption with an override-friendly system-package path.
- [ ] Replace `snapshot_json_parser` with the selected parser while preserving
  the documented `linuxdesktop.settings.registry.snapshot.v1` schema.
- [ ] Keep schema validation strict: unknown fields, wrong root shape, scalar
  values, unsupported value kinds, trailing content, and extra nesting must
  report migration diagnostics rather than silently accepting broader input.
- [ ] Preserve existing public diagnostic translation contracts or document any
  deliberate pre-1.0 diagnostic changes.
- [ ] Keep JSON parsing scoped to app-settings Registry snapshot compatibility;
  do not turn `ld_migration` into a general JSON API.
- [ ] Add or update adversarial tests for Unicode escapes, nesting depth,
  malformed escape recovery, invalid/trailing input, and valid snapshot
  round-trips.

## Evidence Fit

Adversarial tests plus source review should catch this: the risk is a
maintenance-heavy parser accepting, rejecting, or corrupting app-settings
snapshot data in surprising ways.

## Release Gate

Blocks `0.2.0`.
