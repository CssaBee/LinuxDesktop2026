# 114 - Reconsider Watch Callback Self-Destruction Contract

**What to build:** Decide whether `ld_watch` should continue to support final
watcher facade destruction from inside its own callback before the contract
hardens further.

**Blocked by:** `113` - Serialize Watch Callback Delivery.

**Status:** pending

- [ ] Inventory the current callback-safe lifecycle promises, including
  `stop()`, `remove_watch()`, `set_callback()`, and facade destruction.
- [ ] Decide whether callback self-destruction remains worth the ownership,
  detach, and worker-lifetime proof burden.
- [ ] If keeping it, add a focused lifecycle invariant note and regression tests
  that protect future changes to worker ownership, backend resources, logging,
  metrics, and executor integration.
- [ ] If narrowing it, provide a replacement contract such as callback-safe
  `request_stop()` plus final destruction from an outside context.
- [ ] Update public docs and examples so users do not infer stronger lifetime
  behavior than the project is willing to maintain.

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
