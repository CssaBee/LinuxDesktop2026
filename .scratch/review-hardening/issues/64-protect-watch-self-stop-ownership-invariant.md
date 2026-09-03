# 64 - Protect Watch Self-Stop Ownership Invariant

**What to build:** Document and stress-test the ownership invariant that makes
callback-triggered watcher destruction safe.

**Blocked by:** None.

**Status:** pending

- [ ] Add a short source comment at thread start/stop sites explaining that any
  detached worker must retain a strong `impl` owner until it exits.
- [ ] Add a tight ASan/TSan-friendly regression loop that repeatedly destroys
  the last watcher facade from inside the callback.
- [ ] Check worker lambdas and future callback paths for accidental `[this]`
  captures or borrowed lifetime assumptions.

## Review Anchor

The newer review says the self-destruction bug is fixed, but the safety proof
now depends on detached workers retaining `shared_ptr<impl>` until exit.
