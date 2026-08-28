# 09 — Define Watcher Callback Contract Publicly

**What to build:** Watcher API users should be able to understand callback threading, reentrancy, exception, and delivery-mode behavior directly from the public API docs.

**Blocked by:** None — can start immediately.

**Status:** ready-for-agent

- [ ] Public watcher documentation states which thread may invoke callbacks and that UI-thread dispatch is not promised.
- [ ] Public watcher documentation states whether callbacks may call `stop`, remove watches, replace callbacks, or destroy the watcher.
- [ ] Public watcher documentation states whether callbacks may throw and what happens if they do.
