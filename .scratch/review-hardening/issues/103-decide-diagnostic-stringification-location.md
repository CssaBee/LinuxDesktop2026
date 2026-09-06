# 103 - Decide Diagnostic Stringification Location

**What to build:** Decide whether diagnostic and enum stringification should
remain header-defined through `0.2.0` or move behind implementation files before
release-candidate work.

**Blocked by:** None.

**Status:** pending

- [ ] Inventory public `to_string` functions and diagnostic-code constants that
  are currently header-defined.
- [ ] Decide whether the current shape is acceptable for pre-1.0 source
  compatibility or whether compile-time/binary-size/API hygiene costs justify a
  move.
- [ ] If moving, preserve exhaustiveness checks and document any source
  migration path.
- [ ] If keeping, record the rationale in the roadmap or API stability notes
  and defer the cleanup to release-candidate hardening.

## Evidence Fit

Source review should catch this: the risk is low-priority API hygiene turning
into accidental ABI or compile-time surface area as modules grow.

## Release Gate

Does not block `0.2.0`.
