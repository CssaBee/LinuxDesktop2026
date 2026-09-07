# 115 - Bound Settled Event Scheduler State

**What to build:** Make settled-file readiness scheduling bounded for bursts
across many distinct paths.

**Blocked by:** None.

**Status:** pending

- [ ] Add a configurable `max_pending_settle_paths` or equivalent capacity
  limit for distinct pending settled-file keys.
- [ ] Replace repeated pending-set scans for earliest deadline with a
  deadline-ordered structure such as a min-heap.
- [ ] Use generation IDs or equivalent stale-entry handling so coalesced paths
  do not corrupt deadline ordering.
- [ ] Define degradation semantics when capacity is exceeded, including
  diagnostics and rescan-required state.
- [ ] Add stress tests for large distinct-path bursts, repeated coalescing for
  the same path, timeout behavior, and capacity-exceeded reporting.

## Review Anchor

The September 6 clean re-review found that settled-file readiness is coalesced
per path but still appears to maintain state proportional to the number of
distinct dirty paths and scan that state to find the next deadline.

## Evidence Fit

Stress tests and performance evidence should catch this: the risk is a watcher
subsystem that is bounded in raw delivery but not in delayed settled-file work.

## Release Gate

Blocks `0.2.1`.
