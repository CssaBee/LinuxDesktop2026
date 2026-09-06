# 11 — Add Bounded Watcher Queue Semantics

**What to build:** A slow watcher consumer should not cause unbounded memory growth; queue pressure should degrade the stream and request a rescan.

**Blocked by:** None — can start immediately.

**Status:** done

- [x] Watcher queues have documented bounds or backpressure behavior.
- [x] Queue overflow emits a degraded event with rescan guidance.
- [x] Tests show slow consumers receive overflow/rescan behavior instead of unlimited queued events.

## Implementation Note

The bounded queue contract was completed by later watcher hardening, especially
tasks 62, 63, and 84. Raw delivery is bounded, settled-file readiness is
coalesced by path, overflow degrades the stream with a diagnostic, and
overflow no longer erases unrelated queued events.
