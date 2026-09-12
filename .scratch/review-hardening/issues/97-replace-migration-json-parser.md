# 97 - Replace Migration JSON Parser

**What to build:** Replace the hand-written Registry snapshot JSON parser with
a maintained JSON dependency before `0.2.0`.

**Blocked by:** None.

**Status:** implemented

- [x] Choose the dependency integration shape, preferring deterministic CMake
  consumption with an override-friendly system-package path.
- [x] Replace `snapshot_json_parser` with the selected parser while preserving
  the documented `linuxdesktop.settings.registry.snapshot.v1` schema.
- [x] Keep schema validation strict: unknown fields, wrong root shape, scalar
  values, unsupported value kinds, trailing content, and extra nesting must
  report migration diagnostics rather than silently accepting broader input.
- [x] Preserve existing public diagnostic translation contracts or document any
  deliberate pre-1.0 diagnostic changes.
- [x] Keep JSON parsing scoped to app-settings Registry snapshot compatibility;
  do not turn `ld_migration` into a general JSON API.
- [x] Add or update adversarial tests for Unicode escapes, nesting depth,
  malformed escape recovery, invalid/trailing input, and valid snapshot
  round-trips.

## Implementation Note

`ld_migration` now consumes `nlohmann_json` privately through CMake:
`find_package(nlohmann_json 3.11.3 CONFIG QUIET)` is tried first, then a
pinned `FetchContent` fallback downloads the `v3.11.3` release tarball by
SHA-256. The public migration API remains unchanged.

The old `snapshot_json_parser` was replaced with `nlohmann::json` parsing plus
schema-directed validation for the Registry snapshot compatibility format.
Validation still rejects non-object roots, unsupported fields, missing fields,
wrong field types, unsupported Registry value kinds, invalid hex payloads,
duplicate fields, and excessive nesting. Unit coverage intentionally avoids
retesting JSON lexical behavior already covered by `nlohmann/json`; tests focus
on the LinuxDesktop2026 snapshot schema and import diagnostic contracts.

Verification: configured `build-task97`, built all targets, and ran `ctest
--test-dir LinuxDesktop2026/build-task97 --output-on-failure` with 12/12 tests
passing.

## Evidence Fit

Adversarial tests plus source review should catch this: the risk is a
maintenance-heavy parser accepting, rejecting, or corrupting app-settings
snapshot data in surprising ways.

## Release Gate

Resolved for `0.2.0`.
