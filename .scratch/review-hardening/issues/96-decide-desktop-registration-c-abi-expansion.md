# 96 - Decide Desktop Registration C ABI Expansion

**What to build:** Decide whether and how the desktop-bundle registration model
should enter the C ABI before release candidate, using implementation and
consumer evidence instead of copying the C++ object graph prematurely.

**Blocked by:** 95 - Add Maintained Desktop Registration Proof.

**Status:** ready-for-agent

- [ ] Decide whether release-candidate policy allows registration-bundle C ABI
  expansion yet.
- [ ] If expansion is accepted, choose an opaque-handle or builder-style shape
  that keeps nested strings, paths, diagnostics, activation steps, and cleanup
  reports manageable for C callers.
- [ ] If expansion is rejected, document that C callers should continue using
  the existing narrow effect calls for the release candidate.
- [ ] Add ownership and free/reset acceptance criteria for every report shape
  that crosses the C ABI boundary.

## Evidence Fit

Source review plus maintained proof should catch this: the risk is freezing an
expensive C ABI before the C++ registration model has survived real use.
