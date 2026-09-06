# 110 - Remove LinuxDesktop2026 Types From Product-Facing FlavorTest Surfaces

**What to build:** Audit FlavorTest product-facing headers and remove exposed
LinuxDesktop2026 types where the API friction notes say a real adapter should
translate them into product-owned vocabulary.

**Blocked by:** 109 - Document Root Builder And Raw Options Selection Rule.

**Status:** implemented

- [x] Inventory FlavorTest public headers for `linuxdesktop::` types in structs,
  callbacks, parameters, and return values that pretend to be product-facing
  adapter surfaces.
- [x] Replace leaked library types with product-owned types where doing so
  makes the example more honest without bloating the test harness.
- [x] Keep private implementation files free to use LinuxDesktop2026 types
  directly.
- [x] Prioritize the PrusaSlicer leak of
  `linuxdesktop::settings::config_file` and `validation_callback`.
- [x] Recheck Notepad++ result surfaces where fields still mirror copied
  defaults, validated write backup, dry-run imports, registration statuses, or
  activation follow-up; update notes if the mirroring is acceptable product
  evidence rather than leakage.
- [x] Update `docs/FlavorTests/API_FRICTION.md` and examples after the audit.

## Evidence Anchor

`docs/FlavorTests/API_FRICTION.md` names PrusaSlicer as the concrete leak:
`prusaslicer_flavor.hpp` exposes `linuxdesktop::settings::config_file` and
`validation_callback` in product-facing FlavorTest types. Notepad++ has softer
leakage where result fields still mirror LinuxDesktop2026 mechanics; the audit
should decide which fields are honest application behavior and which should be
translated.

## Release Gate

Does not block `0.2.0` unless FlavorTests are promoted from evidence harnesses
to public adoption examples for the release.
