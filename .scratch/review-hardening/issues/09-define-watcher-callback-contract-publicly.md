# 09 — Define Watcher Callback Contract Publicly

**What to build:** Watcher API users should be able to understand callback threading, reentrancy, exception, and delivery-mode behavior directly from the public API docs.

**Blocked by:** None — can start immediately.

**Status:** done

- [x] Public watcher documentation states which thread may invoke callbacks and that UI-thread dispatch is not promised.
- [x] Public watcher documentation states whether callbacks may call `stop`, remove watches, replace callbacks, or destroy the watcher.
- [x] Public watcher documentation states whether callbacks may throw and what happens if they do.

## Implementation Note

The contract was completed through the later watcher lifecycle hardening work,
especially task 57. Callback-triggered stop, callback replacement, watch
removal, callback exceptions, and last-owner release during callback delivery
are covered by the same lifecycle suite.
