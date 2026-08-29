# 10 — Harden Watcher Callback Lifecycle

**What to build:** Watcher callback delivery should behave predictably under exceptions, self-stop, watch removal, callback replacement, and object destruction.

**Blocked by:** 09 — Define Watcher Callback Contract Publicly.

**Status:** done

- [x] Callback exceptions cannot accidentally terminate an internal watcher thread unless the public contract explicitly requires non-throwing callbacks.
- [x] Callback-triggered `stop`, watch removal, and callback replacement follow the documented contract.
- [ ] Tests cover callback throws, callback-triggered lifecycle operations, and destruction while delivery is active.
