# 91 - Add Desktop Bundle API And Report Vocabulary

**What to build:** Expose the first C++ desktop-bundle registration model so
callers can describe one application registration request and receive staged
artifact, activation-plan, cleanup-plan, and diagnostic reports without using
per-desktop APIs.

**Blocked by:** 87 - Add Staged XDG Desktop Registration Artifacts; 88 - Prove Windows Registration Capability Posture.

**Status:** implemented

- [x] Add the C++ bundle request and report vocabulary for desktop entry
  metadata, icons, MIME declarations, associations, default-application intent,
  URL schemes, policies, activation steps, and cleanup rules.
- [x] Keep live activation represented as explicit report steps, not as an
  implied side effect of a successful staged write.
- [x] Add compile or unit coverage that prevents enum, capability, and report
  drift for every registration group currently named by `ld_desktop`.
- [x] Do not expand the C ABI in this ticket.

## Implementation Note

Added the first C++ `desktop_bundle` request/report vocabulary and
`plan_bundle`/`apply_bundle`/`query_bundle`/`remove_bundle` aggregation entry
points. The implementation delegates to existing per-effect APIs so staged
artifact validation, permissions, and path selection remain shared. Bundle
reports collect artifact reports, policy reports, activation plans, cleanup
rules, and diagnostics without adding C ABI surface.

## Evidence Fit

Source review plus unit or compile checks should catch this: the risk is public
surface drift before implementation starts.
