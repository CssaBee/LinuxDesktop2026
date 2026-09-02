# 58 - Run ld_watch Lifecycle And Settled-File Discovery

**What to build:** Treat `ld_watch` as a discovery target before adding more
watcher features. Map the full watcher lifecycle, settled-file readiness,
cancellation, queueing, backend-shutdown, and stress-test model needed for a
production-shaped watcher.

**Blocked by:** 56 - Fix Watch Settle Worker Permanent Exit; 57 - Fix Watch
Callback Self-Destruction Lifecycle.

**Status:** implemented

- [x] Inventory public lifecycle promises in `include/linuxdesktop/watch.hpp`,
  ADR 0010, tests, examples, and current implementation behavior.
- [x] Classify watcher operations by callback-safety: `stop`, `remove_watch`,
  `set_callback`, `poll`, `wait`, `wait_for`, `add_watch`, move/destroy.
- [x] Decide whether settled-file behavior should keep sleep-per-item worker
  semantics, move to a timer/deadline scheduler, or be postponed as
  experimental.
- [x] Stress-test head-of-line blocking: many files changing while one file
  never stabilizes must not make unrelated settled events unboundedly late
  unless that limitation is explicitly documented.
- [x] Cover backend shutdown failures, queue overflow during watch removal,
  repeated add/remove/start, callback replacement, and pull/callback mode
  switching.
- [x] Record the resulting lifecycle contract in documentation or an ADR only
  if the decision is hard to reverse and surprising without context.

## Implementation Notes

- `CONTEXT.md` now distinguishes **Watcher lifecycle** from
  **Service/daemon lifecycle** and names **Settled-file readiness** as the
  canonical term for this behavior.
- `.scratch/review-hardening/task-58-ld-watch-lifecycle-and-settled-file-discovery.md`
  records the focused discovery run, local commands, results, and follow-up.
- Fresh `build-task58` coverage passed for deterministic simulated-backend
  tests and native Linux `inotify` smoke tests.
- Fresh `build-task58-libuv` coverage passed for deterministic
  simulated-backend tests and libuv-preferred watcher smoke tests.
- No ADR was added because the pass did not force a hard-to-reverse,
  surprising API trade-off.
- Task 61 tracks non-blocking cleanup for repeated settled-file option
  initializer warnings in `tests/watch_tests.cpp`.

## Review Anchor

The review grouped several watcher risks together: unsafe asynchronous
ownership, one settle worker serializing sleeps and filesystem polling, missing
adversarial lifecycle tests, and production behavior that is not yet explicit.
