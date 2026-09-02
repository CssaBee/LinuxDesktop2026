# 56 - Fix Watch Settle Worker Permanent Exit

**What to build:** A canceled, stale, or removed settle task must not terminate
the entire `ld_watch` settle worker.

**Blocked by:** None.

**Status:** proposed

- [ ] Change settle-task control flow so ordinary cancellation, missing watch
  options, removed watches, stale generations, debounce cancellation, and
  settle timeout skip the current task without killing future settlement.
- [ ] Reserve worker exit for explicit watcher shutdown.
- [ ] Replace ambiguous `std::optional<watch_event>` control flow with an
  explicit settle outcome if that makes shutdown/cancel/stale behavior clearer.
- [ ] Add a regression test where a settled watch is removed while work is
  queued, then a later settled watch still delivers events.
- [ ] Add a regression test where a stale generation is skipped and the worker
  continues processing the next pending task.

## Review Anchor

The review found `run_settle()` returns from the worker when
`apply_settle_policy()` returns `std::nullopt`. That can permanently stop all
future settled-file delivery after one normal cancellation path.
