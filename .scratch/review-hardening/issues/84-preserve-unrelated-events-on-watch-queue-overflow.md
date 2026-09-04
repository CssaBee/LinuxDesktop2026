# 84 - Preserve Unrelated Events On Watch Queue Overflow

**What to build:** Change pull-delivery queue overflow handling so one burst
does not erase all pending events without accounting.

**Blocked by:** None.

**Status:** done

- [x] Replace whole-queue clearing with an explicit drop policy such as
  drop-oldest or drop-newest.
- [x] Report how many events were dropped, and whether a rescan is required.
- [x] Preserve the invariant that overflow degrades stream state and surfaces a
  caller-visible overflow event.
- [x] Add a regression test proving an unrelated already-queued event is not
  silently discarded without being counted/diagnosed.

## Review Anchor

The broad review found `enqueue_locked()` clears the entire watcher delivery
queue on overflow before pushing a single overflow event. Current source still
shows `queue_.clear()` in that path.
