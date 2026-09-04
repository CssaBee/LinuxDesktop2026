# 86 - Design Desktop Bundle Registration Surface

**What to build:** Define the next `ld_desktop` public surface around a
preferred desktop-bundle registration path plus individual effect calls.

**Blocked by:** 83 - Run Review Blind-Spot Retrospective.

**Status:** pending

- [ ] Draft the C++ `desktop_bundle` model for app registration artifacts:
  desktop entry metadata, icon references, MIME declarations and associations,
  default-application intent, URL schemes, activation/update plans, and cleanup.
- [ ] Decide which individual effect calls remain public for advanced callers
  and tests.
- [ ] Keep runtime shell-open behavior, reveal-in-file-manager behavior, and
  single-instance activation outside this ticket unless repeated consumer
  evidence reopens the boundary.
- [ ] Update the C ABI plan without expanding the C ABI before release-candidate
  policy allows it.
- [ ] Add design-only tests or compile checks if they help prevent enum/report
  drift before implementation starts.

## Review Anchor

ADR 0015 accepts a wider `ld_desktop` call set than the first autostart/policy
extraction, but only when the calls collapse onto standards-backed registration
artifacts instead of per-desktop public APIs.
