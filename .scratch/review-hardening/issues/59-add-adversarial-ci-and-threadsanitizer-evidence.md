# 59 - Add Adversarial CI And ThreadSanitizer Evidence

**What to build:** Extend CI from ASan/UBSan and portability smoke tests into
concurrency and lifecycle evidence, especially for `ld_watch`.

**Blocked by:** 58 - Run ld_watch Lifecycle And Settlement Discovery.

**Status:** implemented

- [x] Add a Linux ThreadSanitizer lane for the watcher lifecycle/stress tests if
  the toolchain and dependencies make the lane stable enough for CI.
- [x] If full-suite TSan is too noisy, isolate a deterministic watcher hardening
  target that runs under TSan first.
- [x] Add CI coverage for callback destruction, stop while settling,
  remove-watch while settling, repeated add/remove/start, callback replacement,
  and queue overflow through the deterministic watcher hardening target.
- [x] Preserve ASan/UBSan lanes; TSan is additional evidence, not a replacement.
- [x] Document which adversarial lifecycle cases run in ordinary CTest, which
  run only in sanitizer CI, and which remain manual or platform-limited.

## Current Notes

- `.github/workflows/ci.yml` adds `watcher-thread-sanitizer`, an
  Ubuntu/Clang TSan job that builds and runs only `ld_watch_tests`.
- The TSan job disables the optional libuv backend and skips examples so the
  first race-detection lane stays focused on portable watcher lifecycle code.
- `ld_watch_tests` is labeled `watch;adversarial;tsan`; backend-specific watcher
  tests are labeled separately for future CI expansion.
- `docs/ci-portability-evidence.md` records the current adversarial watcher
  coverage and the task-58 cases that still need discovery before entering CI.
- Task 58 completed the discovery pass. The deterministic TSan target remains
  the current CI evidence lane; native-backend or heavier stress expansion is
  now tracked separately by task 102.

## Review Anchor

The review noted that ASan/UBSan do not find data races and did not find a TSan
lane. The missing evidence is adversarial concurrency/lifecycle CI, not just
more parser or path hostile-input tests.
