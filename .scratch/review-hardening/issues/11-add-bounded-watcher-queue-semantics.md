# 11 — Add Bounded Watcher Queue Semantics

**What to build:** A slow watcher consumer should not cause unbounded memory growth; queue pressure should degrade the stream and request a rescan.

**Blocked by:** None — can start immediately.

**Status:** ready-for-agent

- [ ] Watcher queues have documented bounds or backpressure behavior.
- [ ] Queue overflow emits a degraded event with rescan guidance.
- [ ] Tests show slow consumers receive overflow/rescan behavior instead of unlimited queued events.
