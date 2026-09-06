# 92 - Wire Desktop Bundle To Staged Registration Effects

**What to build:** Make the C++ desktop-bundle path plan, dry-run, apply,
query, and remove the supported staged registration artifacts as one coherent
operation while preserving per-artifact diagnostics.

**Blocked by:** 91 - Add Desktop Bundle API And Report Vocabulary.

**Status:** implemented

- [x] Bundle operations cover the staged desktop entry, icon, MIME,
  default-application, URL-scheme, autostart, and policy effects supported by
  the current backend.
- [x] A successful bundle report distinguishes staged artifacts from activation
  steps that still require caller, user, admin, or platform follow-up.
- [x] Partial success keeps enough per-effect diagnostics for a product adapter
  to explain which registration effects are present, missing, unsupported, or
  pending activation.
- [x] Advanced individual effect calls continue to work and share validation,
  path selection, diagnostics, and durable write behavior with the bundle path.

## Evidence Fit

Unit and adversarial tests should catch this: the risk is orchestration hiding
per-effect failure or overclaiming activation.

## Implementation Note

Task 92 keeps bundle writes on the existing individual staged-effect APIs and
annotates the child reports with `effect_kind` plus `registration_status`.
Bundle activation remains a separate `activation_plan`, so `ok` means staged
artifacts succeeded, not that desktop databases, icon caches, dconf, or Windows
user-choice state are already active. The new tests cover bundle
plan/apply/query/remove and a partial icon failure that preserves successful
artifact status plus the failed child diagnostic.
