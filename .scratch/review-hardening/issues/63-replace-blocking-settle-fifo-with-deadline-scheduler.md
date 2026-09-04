# 63 - Replace Blocking Settle FIFO With Deadline Scheduler

**What to build:** Convert settled-file readiness from one blocking FIFO worker
task at a time into deadline-based scheduling across pending paths.

**Blocked by:** 62 - Bound And Coalesce Settled-File Readiness Work.

**Status:** done

- [x] Model each pending path with next poll/debounce deadline, stability
  window, timeout deadline, and latest event payload.
- [x] Ensure an unsettled or slow-changing file cannot block delivery of other
  paths that are already ready.
- [x] Add a head-of-line test where file A has long stability/timeout behavior
  while files B-Z are immediately deliverable.
- [x] Keep raw watcher event delivery independent from settled-file readiness.

## Review Anchor

The newer review accepts the raw-event fix but shows settled events still block
behind the first task because `run_settle()` synchronously runs
`apply_settle_policy()` to completion before taking the next item.
