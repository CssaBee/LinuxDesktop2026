# 56 - Fix Watch Settle Worker Permanent Exit

**What to build:** A canceled, stale, or removed settle task must not terminate
the entire `ld_watch` settle worker.

**Blocked by:** None.

**Status:** implemented

- [x] Change settle-task control flow so ordinary cancellation, missing watch
  options, removed watches, stale generations, debounce cancellation, and
  settle timeout skip the current task without killing future settlement.
- [x] Reserve worker exit for explicit watcher shutdown.
- [x] Replace ambiguous `std::optional<watch_event>` control flow with an
  explicit settle outcome if that makes shutdown/cancel/stale behavior clearer.
- [x] Add a regression test where a settled watch is removed while work is
  queued, then a later settled watch still delivers events.
- [x] Add a regression test where a stale generation is skipped and the worker
  continues processing the next pending task.

## Review Anchor

The review found `run_settle()` returns from the worker when
`apply_settle_policy()` returns `std::nullopt`. That can permanently stop all
future settled-file delivery after one normal cancellation path.

## Implementation Notes

- `run_settle()` uses explicit settle outcomes: deliver, skip, or shutdown.
- Removed watches, missing settle options, canceled debounce/poll waits, and
  stale generations skip only the current task.
- Worker shutdown is reserved for explicit watcher stop/destruction.
- Settle timeout keeps the existing public behavior: it delivers the event with
  `watch.settle.timeout` diagnostics, and the worker continues afterward.
- Regression coverage includes removing a settled watch while work is pending
  and then delivering a later settled watch, plus stale-generation skip followed
  by unrelated settled work.

## Validation

- `cmake --build build-task54 --parallel 2`
- `ctest --test-dir build-task54 -R '^ld_watch_tests$' --output-on-failure`
