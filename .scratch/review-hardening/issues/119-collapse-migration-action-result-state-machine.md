# 119 - Collapse Migration Action Result State Machine

**What to build:** Make `migration_action_result` expose one authoritative
state machine instead of contradictory enum and boolean state.

**Blocked by:** None.

**Status:** pending

- [ ] Inventory every public and internal user of `migration_action_result`
  state, planned/executed/skipped booleans, rollback booleans, and pre/post
  existence flags.
- [ ] Choose one authoritative `migration_action_state` representation.
- [ ] Replace duplicate booleans with derived query helpers where source
  compatibility permits.
- [ ] Model rollback outcome as an explicit state or result object rather than
  independent booleans.
- [ ] Add tests that prevent contradictory representable states from being
  constructed by public APIs.
- [ ] Document any deliberate pre-1.0 source break and the replacement usage.

## Review Anchor

The September 6 clean re-review found that `migration_action_result` can
represent contradictions such as `state = executed` while `executed = false`
and `skipped = true`. That invites downstream code to split between enum-based
and boolean-based interpretation.

## Evidence Fit

API tests and source review should catch this: the risk is a pre-1.0 migration
result contract that becomes harder to clean after consumers depend on both
representations.

## Release Gate

Blocks `0.2.1`.
