# 92 - Wire Desktop Bundle To Staged Registration Effects

**What to build:** Make the C++ desktop-bundle path plan, dry-run, apply,
query, and remove the supported staged registration artifacts as one coherent
operation while preserving per-artifact diagnostics.

**Blocked by:** 91 - Add Desktop Bundle API And Report Vocabulary.

**Status:** ready-for-agent

- [ ] Bundle operations cover the staged desktop entry, icon, MIME,
  default-application, URL-scheme, autostart, and policy effects supported by
  the current backend.
- [ ] A successful bundle report distinguishes staged artifacts from activation
  steps that still require caller, user, admin, or platform follow-up.
- [ ] Partial success keeps enough per-effect diagnostics for a product adapter
  to explain which registration effects are present, missing, unsupported, or
  pending activation.
- [ ] Advanced individual effect calls continue to work and share validation,
  path selection, diagnostics, and durable write behavior with the bundle path.

## Evidence Fit

Unit and adversarial tests should catch this: the risk is orchestration hiding
per-effect failure or overclaiming activation.
