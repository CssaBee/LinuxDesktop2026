# 115 - Bound Settled Event Scheduler State

**What to build:** Make settled-file readiness scheduling bounded for bursts
across many distinct paths.

**Blocked by:** None.

**Status:** implemented

- [x] Add a configurable `max_pending_settle_paths` or equivalent capacity
  limit for distinct pending settled-file keys.
- [x] Replace repeated pending-set scans for earliest deadline with a
  deadline-ordered structure such as a min-heap.
- [x] Use generation IDs or equivalent stale-entry handling so coalesced paths
  do not corrupt deadline ordering.
- [x] Define degradation semantics when capacity is exceeded, including
  diagnostics and rescan-required state.
- [x] Add stress tests for large distinct-path bursts, repeated coalescing for
  the same path, timeout behavior, and capacity-exceeded reporting.

## Review Anchor

The September 6 clean re-review found that settled-file readiness is coalesced
per path but still appears to maintain state proportional to the number of
distinct dirty paths and scan that state to find the next deadline.

## Evidence Fit

Stress tests and performance evidence should catch this: the risk is a watcher
subsystem that is bounded in raw delivery but not in delayed settled-file work.

## Implementation Notes

- Added `settle_options::max_pending_paths` with a default capacity of 512
  distinct in-flight `(watch_id, path)` settle keys.
- Replaced repeated scans over `pending_settle_tasks_` with a deadline-ordered
  `std::set` index, while keeping the task map as the authoritative coalesced
  state.
- Kept generation checks for stale/current task decisions, erased completed
  generations, and removed old deadline entries when coalescing updates a path.
- Capacity overflow emits an `event_kind::overflow` event, marks the stream
  degraded, attaches `watch.queue.overflow`, and sets `rescan_recommended`.
- Added unit coverage for configured capacity overflow and repeated same-path
  coalescing under the deadline index.

## Verification

```text
cmake --build build --target ld_watch_tests ld_watch_performance_probe
./build/ld_watch_tests
./build/ld_watch_performance_probe
ctest --test-dir build --output-on-failure
```

Performance probe evidence from 2026-09-12:

```text
watch.performance.simulated.settled.delivered=96
watch.performance.simulated.settled.max_pending=21
watch.performance.simulated.settled.p50_latency_ms=0
watch.performance.simulated.settled.p95_latency_ms=1
watch.performance.inotify.settled.delivered=80
watch.performance.inotify.settled.max_pending=27
watch.performance.inotify.settled.p50_latency_ms=3
watch.performance.inotify.settled.p95_latency_ms=3
```

## Release Gate

Blocks `0.2.1`.
