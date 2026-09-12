# 114 - Reconsider Watch Callback Self-Destruction Contract

**What to build:** Decide whether `ld_watch` should continue to support final
watcher facade destruction from inside its own callback before the contract
hardens further.

**Blocked by:** `113` - Serialize Watch Callback Delivery.

**Status:** implemented

- [x] Inventory the current callback-safe lifecycle promises, including
  `stop()`, `remove_watch()`, `set_callback()`, and facade destruction.
- [x] Decide whether callback self-destruction remains worth the ownership,
  detach, and worker-lifetime proof burden.
- [x] If keeping it, add a focused lifecycle invariant note and regression tests
  that protect future changes to worker ownership, backend resources, logging,
  metrics, and executor integration.
- [x] Record why no replacement `request_stop()` contract is needed when
  keeping callback-safe final facade destruction as a shutdown path.
- [x] Update public docs and examples so users do not infer stronger lifetime
  behavior than the project is willing to maintain.

## Implementation Notes

Kept callback self-destruction as an explicit `ld_watch` prototype contract, but
narrowed it to a shutdown-only promise. Callbacks may still call `stop()`,
`remove_watch()`, `set_callback()`, or release the final watcher facade, but
final facade destruction is documented as stopping the watcher, releasing
backend resources, and waiting for worker threads other than the current
callback delivery thread.

Added a source-level lifecycle invariant in `src/watch.cpp`: every worker entry
point must retain a strong `impl` owner, and `stop()` may detach only the
calling worker while joining all other joinable workers. Future worker,
backend-resource, logging, metrics, or executor integration changes must
preserve that invariant.

Strengthened the deterministic simulated-backend regression for last-owner
release so it now also proves backend resources are stopped after the watcher
facade is destroyed from its own callback. Updated ADR 0010 with the decision
and maintenance burden so users do not infer a broader lifetime pattern.

No replacement `request_stop()` API was added because the existing supported
operations cover shutdown and lifecycle mutation without expanding public API
surface before release-candidate status.

## Review Anchor

The September 6 clean re-review downgraded this from a direct defect but still
called it a major long-term maintenance risk. Supporting final destruction from
inside a callback forces unusual worker ownership and detached-current-worker
behavior that future maintainers must preserve.

## Evidence Fit

Contract review plus lifecycle tests should catch this: the risk is a clever
ergonomic promise becoming a permanent hidden constraint on every future
watcher change.

## Release Gate

Blocks `0.2.1`.
