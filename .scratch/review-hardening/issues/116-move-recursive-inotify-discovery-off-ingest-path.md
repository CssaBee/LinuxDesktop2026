# 116 - Move Recursive Inotify Discovery Off Ingest Path

**What to build:** Keep native Linux inotify ingest small by moving expensive
recursive tree discovery and subwatch installation out of the event-processing
path.

**Blocked by:** None.

**Status:** pending

- [ ] Identify every path where native Linux recursive watching walks a newly
  created or moved-in subtree while processing inotify events.
- [ ] Introduce an asynchronous reconciliation or discovery path for recursive
  subwatch installation.
- [ ] Keep the ingest loop focused on reading, minimally normalizing, and
  queueing kernel events.
- [ ] Preserve honest overflow/degraded/rescan-required diagnostics when
  reconciliation cannot keep up.
- [ ] Add stress coverage for large directory move-in/create scenarios while
  concurrent filesystem activity continues.
- [ ] Re-run native watcher performance evidence after the architecture change.

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
