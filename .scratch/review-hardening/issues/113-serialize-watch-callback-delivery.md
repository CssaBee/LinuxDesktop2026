# 113 - Serialize Watch Callback Delivery

**What to build:** Make `ld_watch` callback delivery single-threaded per
watcher, or explicitly choose and test a concurrent callback contract.

**Blocked by:** None.

**Status:** pending

- [ ] Decide whether one watcher callback may execute concurrently on multiple
  internal delivery threads.
- [ ] Prefer one bounded delivery queue and one callback execution context per
  watcher, with backend events and settled-file readiness both feeding that
  context.
- [ ] Preserve callback exception containment and existing callback-safe
  mutation operations.
- [ ] Add adversarial tests for mixed raw and settled delivery, stateful
  callbacks, callback replacement, `stop()`, `remove_watch()`, and facade
  destruction under load.
- [ ] If concurrent callbacks remain intentional, document the concurrency
  contract prominently and prove the shutdown/reentrant matrix with tests.

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

Blocks `0.2.1`.
