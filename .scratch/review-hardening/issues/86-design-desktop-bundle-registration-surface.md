# 86 - Design Desktop Bundle Registration Surface

**What to build:** Define the next `ld_desktop` public surface around a
preferred desktop-bundle registration path plus individual effect calls.

**Blocked by:** 83 - Run Review Blind-Spot Retrospective.

**Status:** implemented

- [x] Draft the C++ `desktop_bundle` model for app registration artifacts:
  desktop entry metadata, icon references, MIME declarations and associations,
  default-application intent, URL schemes, activation/update plans, and cleanup.
- [x] Decide which individual effect calls remain public for advanced callers
  and tests.
- [x] Keep runtime shell-open behavior, reveal-in-file-manager behavior, and
  single-instance activation outside this ticket unless repeated consumer
  evidence reopens the boundary.
- [x] Update the C ABI plan without expanding the C ABI before release-candidate
  policy allows it.
- [x] Add design-only tests or compile checks if they help prevent enum/report
  drift before implementation starts.

## Result

`docs/plan/ld-desktop-extraction.md` now defines the design shape for a future
C++ `desktop_bundle` registration surface: desktop entry metadata, icon
references, MIME declarations and associations, default-application intent, URL
scheme handlers, policies, activation steps, cleanup rules, and bundle reports.
It keeps activation explicit as a plan rather than claiming staged artifacts
are live system effects.

The same plan lists the individual effect calls that should remain public for
advanced callers and tests, keeps runtime shell-open, reveal-in-file-manager,
and single-instance activation outside the tranche, and records that the C ABI
must not grow until release-candidate policy allows it. No compile checks were
added because the task intentionally records design shape rather than exposing
new declarations.

## Review Anchor

ADR 0015 accepts a wider `ld_desktop` call set than the first autostart/policy
extraction, but only when the calls collapse onto standards-backed registration
artifacts instead of per-desktop public APIs.
