# 116 - Move Recursive Inotify Discovery Off Ingest Path

**What to build:** Keep native Linux inotify ingest small by moving expensive
recursive tree discovery and subwatch installation out of the event-processing
path.

**Blocked by:** None.

**Status:** implemented

- [x] Identify every path where native Linux recursive watching walks a newly
  created or moved-in subtree while processing inotify events.
- [x] Introduce an asynchronous reconciliation or discovery path for recursive
  subwatch installation.
- [x] Keep the ingest loop focused on reading, minimally normalizing, and
  queueing kernel events.
- [x] Preserve honest overflow/degraded/rescan-required diagnostics when
  reconciliation cannot keep up.
- [x] Add stress coverage for large directory move-in/create scenarios while
  concurrent filesystem activity continues.
- [x] Re-run native watcher performance evidence after the architecture change.

## Implementation Notes

The only event-ingest path that walked a newly created or moved-in recursive
subtree was `map_event_for_record()` calling `add_discovered_tree()` for
recursive-emulated directory `created` and `renamed_new` events. That path now
queues a bounded discovery request instead.

Native Linux inotify now owns a discovery worker. The ingest loop reads kernel
events, maps them, queues ordinary watch events, and enqueues recursive
discovery requests without walking moved-in trees. The discovery worker installs
subwatches, emits synthetic `watch.recursive.discovered` events, and drops stale
events after `remove_watch()`.

The discovery queue is bounded. If reconciliation falls behind, it emits a
degraded overflow event with `watch.queue.overflow` and
`watch.rescan_recommended` diagnostics so product code can rescan watched roots.

Added `native_recursive_large_move_in_keeps_root_activity_flowing()` to move a
populated subtree into a recursive inotify watch while sibling root activity
continues. The test verifies root events still flow and deep moved-in paths are
reported through asynchronous discovery.

Performance evidence after the change:

```text
watch.performance.native.backend=inotify
watch.performance.inotify.raw.distinct_paths=240
watch.performance.inotify.raw.events_observed=718
watch.performance.inotify.raw.overflow_events=0
watch.performance.inotify.raw.throughput_paths_per_second=18571.7
watch.performance.inotify.raw.max_queue_depth=387
watch.performance.inotify.raw.elapsed_us=12922
watch.performance.inotify.settled.delivered=80
watch.performance.inotify.settled.max_pending=4
watch.performance.inotify.settled.p50_latency_ms=3
watch.performance.inotify.settled.p95_latency_ms=3
```

## Review Anchor

The September 6 clean re-review found that recursive inotify discovery can do
large synchronous tree work on the event path. Moving a large directory into a
recursive watch can keep the worker from draining the kernel queue promptly and
increase overflow risk.

## Evidence Fit

Native Linux stress tests plus source review should catch this: the risk is an
owned recursive inotify backend amplifying exactly the workload it needs to
survive.

## Release Gate

Blocks `0.2.1`.
