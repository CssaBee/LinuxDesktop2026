# 57 - Fix Watch Callback Self-Destruction Lifecycle

**What to build:** Define and implement safe behavior when a watcher facade is
destroyed from, or as a direct consequence of, its own callback delivery.

**Blocked by:** 56 - Fix Watch Settle Worker Permanent Exit.

**Status:** implemented

- [x] Decide whether callback-triggered watcher destruction is supported or
  explicitly forbidden before ship-candidate status.
- [x] If supported, move worker state into an independently owned control block
  or equivalent lifecycle model so facade destruction from a callback cannot
  destroy a joinable current thread.
- [x] The forbidden-path alternative is not used; supported destruction is
  documented and covered instead.
- [x] Add a deterministic regression test for last-owner release during
  callback delivery.
- [x] Add tests for callback-triggered `stop`, callback replacement,
  watch removal, and callback exception behavior in the same lifecycle suite.

## Review Anchor

The review found that `stop()` deliberately avoids joining the current worker
thread, but `impl` destruction can then continue while the current
`std::thread` remains joinable, which invokes `std::terminate`.

## Implementation Notes

- Callback-triggered watcher facade destruction is supported.
- Worker lambdas hold the implementation alive while they unwind, so destroying
  the facade no longer destroys state out from under the delivery thread.
- `watcher` destruction and move assignment explicitly stop the current
  implementation before releasing it.
- `stop()` joins non-current worker threads and detaches the current delivery
  thread when called from inside watcher-owned delivery.
- The public callback contract documents that facade destruction from a callback
  stops the watcher and waits for other worker threads.
- The existing callback lifecycle tests cover callback-triggered stop,
  replacement, watch removal, and exception fallback; a new regression covers
  last-owner release during callback delivery.

## Validation

- `cmake --build build-task54 --parallel 2`
- `ctest --test-dir build-task54 -R '^ld_watch_tests$' --output-on-failure`
- `ctest --test-dir build-task54 -R '^ld_watch' --output-on-failure`
