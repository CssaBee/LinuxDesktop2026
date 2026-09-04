# 66 - Gate Watch Test Hooks Behind CMake Option

**What to build:** Compile internal watcher test hooks only when an explicit
test-build option enables them.

**Blocked by:** None.

**Status:** done

- [x] Add a CMake option for watcher test hooks, defaulting off for normal
  library builds.
- [x] Define `LINUXDESKTOP2026_WATCH_ENABLE_TEST_HOOKS` only for test targets
  or explicitly opted-in builds.
- [x] Keep deterministic simulated-backend tests working under the test option.
- [x] Add a build assertion or compile check showing release/library targets do
  not receive test-only hook code by default.

## Review Anchor

The broad review found `src/watch_backend.hpp` defines
`LINUXDESKTOP2026_WATCH_ENABLE_TEST_HOOKS` unconditionally.
