# 61 - Clean Up ld_watch Settled-File Option Initializers

**What to build:** Remove the repeated `-Wmissing-field-initializers` warnings
from settled-file readiness tests without changing watcher behavior.

**Blocked by:** None. This is a non-blocking cleanup found during task 58.

**Status:** implemented

- [x] Replace aggregate initializers for `linuxdesktop::watch::settle_options`
  in `tests/watch_tests.cpp` with explicit field assignment or a local helper
  that names `debounce_for`, `stable_for`, `poll_interval`, and
  `timeout_after`.
- [x] Keep existing settled-file readiness scenarios behaviorally unchanged:
  readiness, raw delivery while settle work is pending, coalescing, timeout
  diagnostics, remove-watch cancellation, stale generation handling, and large
  batch delivery.
- [x] Verify the focused watcher build no longer emits the initializer
  warnings.

## Implementation Notes

The current settled-file readiness tests initialize all four `settle_options`
fields explicitly, including `timeout_after`, so the aggregate initializer shape
matches the public struct. A focused rebuild compiles `tests/watch_tests.cpp`
without the previous `-Wmissing-field-initializers` noise, and the watcher test
suite keeps the same behavioral coverage.

## Review Anchor

Task 58 passed all focused watcher tests locally, but the fresh debug and
libuv-preferred builds emitted repeated warnings where settled-file readiness
tests omit `timeout_after` in aggregate initialization. The warning is not a
runtime defect, but it makes hardening evidence noisier than it needs to be.
