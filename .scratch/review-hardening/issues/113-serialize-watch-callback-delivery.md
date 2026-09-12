# 113 - Serialize Watch Callback Delivery

**What to build:** Make `ld_watch` callback delivery single-threaded per
watcher, or explicitly choose and test a concurrent callback contract.

**Blocked by:** None.

**Status:** implemented

- [x] Decide whether one watcher callback may execute concurrently on multiple
  internal delivery threads.
- [x] Prefer one bounded delivery queue and one callback execution context per
  watcher, with backend events and settled-file readiness both feeding that
  context.
- [x] Preserve callback exception containment and existing callback-safe
  mutation operations.
- [x] Add adversarial tests for mixed raw and settled delivery, stateful
  callbacks, callback replacement, `stop()`, `remove_watch()`, and facade
  destruction under load.
- [x] Document the chosen serialized callback contract prominently and keep the
  shutdown/reentrant matrix covered by tests.

## Implementation Notes

Implemented a serialized callback contract: callbacks now run on one
watcher-owned delivery thread, and backend events plus settled-file readiness
enqueue callback work into that single context. Pull delivery remains on the
existing bounded queue when no callback is installed.

The callback delivery queue is bounded and emits the same degraded
`watch.queue.overflow` event shape when callbacks fall behind. Callback
exceptions still clear the callback, degrade the stream, and surface a queued
diagnostic error event.

Added `mixed_raw_and_settled_callbacks_are_serialized()` to prove that a raw
event callback can block while settled-file readiness becomes available without
re-entering user callback code concurrently. Existing callback tests continue to
cover replacement, `stop()`, `remove_watch()`, and last-owner facade destruction
from callback context.

## Review Anchor

The September 6 clean re-review found that ordinary watcher delivery and
settled-file delivery appear able to call the same callback from different
threads. The public contract does not make that obvious, so reasonable caller
code can accidentally introduce data races.

## Evidence Fit

ThreadSanitizer coverage, deterministic interleaving tests, and source review
should catch this: the risk is an API contract that looks high-level but pushes
unexpected synchronization requirements onto consumers.

## Release Gate

Resolved for `0.2.1`.
