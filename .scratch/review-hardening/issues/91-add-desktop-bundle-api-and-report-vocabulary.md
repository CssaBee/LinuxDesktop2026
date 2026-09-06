# 91 - Add Desktop Bundle API And Report Vocabulary

**What to build:** Expose the first C++ desktop-bundle registration model so
callers can describe one application registration request and receive staged
artifact, activation-plan, cleanup-plan, and diagnostic reports without using
per-desktop APIs.

**Blocked by:** 87 - Add Staged XDG Desktop Registration Artifacts; 88 - Prove Windows Registration Capability Posture.

**Status:** ready-for-agent

- [ ] Add the C++ bundle request and report vocabulary for desktop entry
  metadata, icons, MIME declarations, associations, default-application intent,
  URL schemes, policies, activation steps, and cleanup rules.
- [ ] Keep live activation represented as explicit report steps, not as an
  implied side effect of a successful staged write.
- [ ] Add compile or unit coverage that prevents enum, capability, and report
  drift for every registration group currently named by `ld_desktop`.
- [ ] Do not expand the C ABI in this ticket.

## Evidence Fit

Source review plus unit or compile checks should catch this: the risk is public
surface drift before implementation starts.
