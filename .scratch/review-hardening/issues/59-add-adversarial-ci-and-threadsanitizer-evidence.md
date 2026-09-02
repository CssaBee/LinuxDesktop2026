# 59 - Add Adversarial CI And ThreadSanitizer Evidence

**What to build:** Extend CI from ASan/UBSan and portability smoke tests into
concurrency and lifecycle evidence, especially for `ld_watch`.

**Blocked by:** 58 - Run ld_watch Lifecycle And Settlement Discovery before this
ticket closes. The initial deterministic watcher lane is wired, but task 58
still decides whether more real-backend or heavier stress evidence is required.

**Status:** open, partially wired

- [x] Add a Linux ThreadSanitizer lane for the watcher lifecycle/stress tests if
  the toolchain and dependencies make the lane stable enough for CI.
- [x] If full-suite TSan is too noisy, isolate a deterministic watcher hardening
  target that runs under TSan first.
- [x] Add CI coverage for callback destruction, stop while settling,
  remove-watch while settling, repeated add/remove/start, callback replacement,
  and queue overflow through the deterministic watcher hardening target. Queue
  overflow during removal remains a task-58 discovery item until it has a
  deterministic test.
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
- Leave this ticket open until task 58 confirms whether the current deterministic
  TSan target is enough or needs additional lifecycle/stress coverage.

## Review Anchor

The review noted that ASan/UBSan do not find data races and did not find a TSan
lane. The missing evidence is adversarial concurrency/lifecycle CI, not just
more parser or path hostile-input tests.
