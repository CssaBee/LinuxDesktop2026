# 59 - Add Adversarial CI And ThreadSanitizer Evidence

**What to build:** Extend CI from ASan/UBSan and portability smoke tests into
concurrency and lifecycle evidence, especially for `ld_watch`.

**Blocked by:** 58 - Run ld_watch Lifecycle And Settlement Discovery.

**Status:** proposed

- [ ] Add a Linux ThreadSanitizer lane for the watcher lifecycle/stress tests if
  the toolchain and dependencies make the lane stable enough for CI.
- [ ] If full-suite TSan is too noisy, isolate a deterministic watcher hardening
  target that runs under TSan first.
- [ ] Add CI coverage for callback destruction, stop while settling,
  remove-watch while settling, repeated add/remove/start, callback replacement,
  and queue overflow during removal once those tests exist.
- [ ] Preserve ASan/UBSan lanes; TSan is additional evidence, not a replacement.
- [ ] Document which adversarial lifecycle cases run in ordinary CTest, which
  run only in sanitizer CI, and which remain manual or platform-limited.

## Review Anchor

The review noted that ASan/UBSan do not find data races and did not find a TSan
lane. The missing evidence is adversarial concurrency/lifecycle CI, not just
more parser or path hostile-input tests.
