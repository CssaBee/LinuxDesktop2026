# 122 - Measure Large-Tree Watch Noise And Coalescing

**What to build:** Extend watcher performance evidence with a large-tree,
noisy-event workload that distinguishes raw native notifications, coalesced
paths, delivered events, and product validation calls.

**Blocked by:** None; unblocked by implemented ticket `121`.

**Status:** implemented

- [x] Add or extend a watcher performance probe for a large synthetic tree. Use
  a reduced CI-safe scale by default, with an opt-in local scale that can model
  roughly 200k paths when the machine can afford it.
- [x] Exercise burst writes to one file, burst writes across many files,
  atomic save-by-replace, repeated attribute-only notifications, recursive
  subdirectory churn, and queue saturation.
- [x] Report at least: native events received, candidate paths coalesced,
  events delivered, validation calls made, overflow/drop counts, max public
  queue depth, max pending settled work, settle latency, elapsed time, and RSS
  growth.
- [x] Compare raw delivery and settled-file delivery so a bounded raw queue is
  not mistaken for bounded deferred work.
- [x] Keep path-construction cost visible if the probe already measures it, but
  do not redesign public event path values without evidence that it dominates.

Implemented in `tests/watch_performance_probe.cpp`. The CI-safe default models
1,024 paths; `LD2026_WATCH_LARGE_TREE_LOCAL=1` models roughly 200,000 paths and
`LD2026_WATCH_LARGE_TREE_PATHS=<n>` selects an explicit local scale. The
September 12, 2026 run recorded raw delivery, settled-file coalescing, pending
settled work, and an intentional queue-saturation overflow in
`docs/validation-evidence.md`.

## Review Anchor

Nextcloud Desktop issue `#7873` describes a workload where high-volume watcher
events make the application unresponsive because filtering and logging run per
event. `ld_watch` already has bounded delivery and settled-file coalescing, but
the current evidence should prove that those claims hold under a
Nextcloud-shaped large-tree workload.

## Evidence Fit

Performance evidence plus adversarial test evidence should catch this:
boundedness claims must identify which layer is bounded and which deferred
work can still grow.

## Release Gate

Does not block `0.2.1`. Promote only if the measurement shows unbounded
project-owned state, missed final-state changes, or a misleading public
boundedness claim.
