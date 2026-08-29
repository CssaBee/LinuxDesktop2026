# 12 — Stress Recursive Watch Behavior

**What to build:** Recursive watching should be tested under the kinds of churn that usually break filesystem watchers, so the project can decide whether native recursive maintenance is worth owning.

**Blocked by:** 11 — Add Bounded Watcher Queue Semantics.

**Status:** done

- [ ] Stress tests cover rename storms, deep recursive creation, remove/recreate churn, and large event bursts.
- [ ] Recursive watch docs describe events as hints and rescan as authoritative after degradation.
- [ ] Test results provide enough evidence to decide whether native backends should remain, be wrapped, or be de-scoped.
