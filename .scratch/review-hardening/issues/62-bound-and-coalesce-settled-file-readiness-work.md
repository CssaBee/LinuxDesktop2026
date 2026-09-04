# 62 - Bound And Coalesce Settled-File Readiness Work

**What to build:** Replace the unbounded physical `settle_queue_` growth with
one pending readiness record per `(watch_id, path)`.

**Blocked by:** None.

**Status:** done

- [x] Define the pending settled-file key and state so repeated events update
  existing work instead of appending stale tasks.
- [x] Preserve the current generation/coalescing semantics while making memory
  use bounded by distinct pending paths, not raw event count.
- [x] Add a burst test where one path receives far more events than the public
  queue depth and memory/task count stays bounded.
- [x] Clarify public wording so "bounded watcher queues" cannot be read as a
  broader guarantee than the implementation provides.

## Review Anchor

The cache-busted September 2026 review found that the delivery queue is bounded
but `settle_queue_` is an unbounded `std::deque`; stale generations are skipped
only after they have already been allocated and queued.
