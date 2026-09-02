# 57 - Fix Watch Callback Self-Destruction Lifecycle

**What to build:** Define and implement safe behavior when a watcher facade is
destroyed from, or as a direct consequence of, its own callback delivery.

**Blocked by:** 56 - Fix Watch Settle Worker Permanent Exit.

**Status:** proposed

- [ ] Decide whether callback-triggered watcher destruction is supported or
  explicitly forbidden before ship-candidate status.
- [ ] If supported, move worker state into an independently owned control block
  or equivalent lifecycle model so facade destruction from a callback cannot
  destroy a joinable current thread.
- [ ] If forbidden for the current prototype, enforce and document the rule with
  diagnostics or debug assertions where practical rather than relying on caller
  luck.
- [ ] Add a deterministic regression test for last-owner release during
  callback delivery.
- [ ] Add tests for callback-triggered `stop`, callback replacement,
  watch removal, and callback exception behavior in the same lifecycle suite.

## Review Anchor

The review found that `stop()` deliberately avoids joining the current worker
thread, but `impl` destruction can then continue while the current
`std::thread` remains joinable, which invokes `std::terminate`.
