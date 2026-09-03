# 65 - Scope Inotify Remove-Watch Pending State

**What to build:** Make native inotify `remove_watch()` clear only state owned
by the removed watch.

**Blocked by:** None.

**Status:** done

- [x] Replace `pending_moves_.clear()` with removal of entries whose watch id
  matches the removed watch.
- [x] Replace `ready_events_.clear()` with filtering of events sourced from
  the removed watch.
- [x] Keep `stop()` as the path that clears all backend state.
- [x] Add a two-watch regression test where removing one watch does not erase
  pending move or ready-event state for the surviving watch.

## Review Anchor

The broad review found `src/watch_inotify.cpp` clears all pending moves and
ready events inside single-watch removal.
