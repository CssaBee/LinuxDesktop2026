# 96 - Decide Desktop Registration C ABI Expansion

**What to build:** Decide whether and how the desktop-bundle registration model
should enter the C ABI before release candidate, using implementation and
consumer evidence instead of copying the C++ object graph prematurely.

**Blocked by:** 95 - Add Maintained Desktop Registration Proof.

**Status:** implemented

- [x] Decide whether release-candidate policy allows registration-bundle C ABI
  expansion yet.
- [x] If expansion is accepted later, require an opaque-handle or builder-style
  shape that keeps nested strings, paths, diagnostics, activation steps, and
  cleanup reports manageable for C callers.
- [x] If expansion is rejected, document that C callers should continue using
  the existing narrow effect calls for the release candidate.
- [x] Add ownership and free/reset acceptance criteria for every report shape
  that crosses the C ABI boundary.

## Result

The `0.2.0` decision is to reject broader desktop-registration C ABI expansion.
The maintained Notepad++ desktop-registration proof did not find blocking C++
API friction and did not prove a C consumer need for bundle-shaped registration.
C callers should keep using the existing autostart and policy effect calls; the
desktop bundle, staged desktop entry/icon/MIME/default-app/protocol effects,
activation plans, and cleanup reports remain C++-only experimental surface until
release-candidate evidence justifies a larger C ABI.

The future shape is still recorded: if release-candidate evidence accepts C ABI
expansion, use opaque handles or builder-style allocation rather than nested
plain structs for the whole C++ object graph. Any report family crossing the C
ABI must document owned fields, expose one matching free function, and guarantee
that freeing releases nested allocations and resets the report to zero. The
current `ld_desktop_effect_report` and `ld_desktop_policy_report` headers now
state that ownership contract, and the C smoke test asserts reset-after-free.

## Evidence Fit

Source review plus maintained proof should catch this: the risk is freezing an
expensive C ABI before the C++ registration model has survived real use.
