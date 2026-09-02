# 61 - Clean Up ld_watch Settled-File Option Initializers

**What to build:** Remove the repeated `-Wmissing-field-initializers` warnings
from settled-file readiness tests without changing watcher behavior.

**Blocked by:** None. This is a non-blocking cleanup found during task 58.

**Status:** proposed

- [ ] Replace aggregate initializers for `linuxdesktop::watch::settle_options`
  in `tests/watch_tests.cpp` with explicit field assignment or a local helper
  that names `debounce_for`, `stable_for`, `poll_interval`, and
  `timeout_after`.
- [ ] Keep existing settled-file readiness scenarios behaviorally unchanged:
  readiness, raw delivery while settle work is pending, coalescing, timeout
  diagnostics, remove-watch cancellation, stale generation handling, and large
  batch delivery.
- [ ] Verify the focused watcher build no longer emits the initializer
  warnings.

## Review Anchor

Task 58 passed all focused watcher tests locally, but the fresh debug and
libuv-preferred builds emitted repeated warnings where settled-file readiness
tests omit `timeout_after` in aggregate initialization. The warning is not a
runtime defect, but it makes hardening evidence noisier than it needs to be.
