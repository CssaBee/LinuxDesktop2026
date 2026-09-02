# 58 - Run ld_watch Lifecycle And Settlement Discovery

**What to build:** Treat `ld_watch` as a discovery target before adding more
watcher features. Map the full lifecycle, settlement, cancellation, queueing,
backend-shutdown, and stress-test model needed for a production-shaped watcher.

**Blocked by:** 56 - Fix Watch Settle Worker Permanent Exit; 57 - Fix Watch
Callback Self-Destruction Lifecycle.

**Status:** proposed

- [ ] Inventory public lifecycle promises in `include/linuxdesktop/watch.hpp`,
  ADR 0010, tests, examples, and current implementation behavior.
- [ ] Classify watcher operations by callback-safety: `stop`, `remove_watch`,
  `set_callback`, `poll`, `wait`, `wait_for`, `add_watch`, move/destroy.
- [ ] Decide whether settled-file behavior should keep sleep-per-item worker
  semantics, move to a timer/deadline scheduler, or be postponed as
  experimental.
- [ ] Stress-test head-of-line blocking: many files changing while one file
  never stabilizes must not make unrelated settled events unboundedly late
  unless that limitation is explicitly documented.
- [ ] Cover backend shutdown failures, queue overflow during watch removal,
  repeated add/remove/start, callback replacement, and pull/callback mode
  switching.
- [ ] Record the resulting lifecycle contract in documentation or an ADR only
  if the decision is hard to reverse and surprising without context.

## Review Anchor

The review grouped several watcher risks together: unsafe asynchronous
ownership, one settle worker serializing sleeps and filesystem polling, missing
adversarial lifecycle tests, and production behavior that is not yet explicit.
