# 119 - Collapse Migration Action Result State Machine

**What to build:** Make `migration_action_result` expose one authoritative
state machine instead of contradictory enum and boolean state.

**Blocked by:** None.

**Status:** implemented

- [x] Inventory every public and internal user of `migration_action_result`
  state, planned/executed/skipped booleans, rollback booleans, and pre/post
  existence flags.
- [x] Choose one authoritative `migration_action_state` representation.
- [x] Replace duplicate booleans with derived query helpers where source
  compatibility permits.
- [x] Model rollback outcome as an explicit state or result object rather than
  independent booleans.
- [x] Add tests that prevent contradictory representable states from being
  constructed by public APIs.
- [x] Document any deliberate pre-1.0 source break and the replacement usage.

## Review Anchor

The September 6 clean re-review found that `migration_action_result` can
represent contradictions such as `state = executed` while `executed = false`
and `skipped = true`. That invites downstream code to split between enum-based
and boolean-based interpretation.

## Evidence Fit

API tests and source review should catch this: the risk is a pre-1.0 migration
result contract that becomes harder to clean after consumers depend on both
representations.

## Implementation Notes

- `migration_action_result::state` is now the only stored action outcome.
- The former public `planned`, `executed`, and `skipped` boolean fields are
  replaced by derived query helpers: `planned()`, `executed()`, and
  `skipped()`.
- Rollback reporting now uses `migration_rollback_state` plus `rollback_path`;
  `rollback_available()`, `rollback_attempted()`, and `rollback_succeeded()`
  are derived from that rollback state.
- This is a deliberate pre-1.0 source break for callers reading the old public
  boolean fields. Use `state`, `rollback_state`, or the query helpers instead.
- `action_result_queries_are_derived_from_authoritative_state` prevents the
  public API surface from reintroducing independent action/rollback booleans.

## Verification

```text
cmake --build build --target ld_migration_tests
./build/ld_migration_tests
cmake --build build --target ld_settings_tests
./build/ld_settings_tests
```

## Release Gate

Resolved for `0.2.1`.
