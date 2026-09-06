# 12 — Stress Recursive Watch Behavior

**What to build:** Recursive watching should be tested under the kinds of churn that usually break filesystem watchers, so the project can decide whether native recursive maintenance is worth owning.

**Blocked by:** 11 — Add Bounded Watcher Queue Semantics.

**Status:** done

- [x] Stress tests cover rename storms, deep recursive creation, remove/recreate churn, and large event bursts.
- [x] Recursive watch docs describe events as hints and rescan as authoritative after degradation.
- [x] Test results provide enough evidence to decide whether native backends should remain, be wrapped, or be de-scoped.

## Implementation Note

The recursive watcher evidence was completed across the watcher hardening batch,
especially tasks 16, 58, 62, 63, and 84. The current decision is to keep native
Linux and Windows backends as owned pre-1.0 behavior, keep libuv optional for
libuv-shaped consumers, and reopen wrapping only from CI, stress-test, or
maintained-consumer evidence.
