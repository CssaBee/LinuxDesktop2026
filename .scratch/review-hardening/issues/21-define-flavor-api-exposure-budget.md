# 21 — Define Flavor API Exposure Budget

**What to build:** Convert the Flavor review findings into a concrete exposure
budget so future changes are judged against product-shaped call sites, not only
unit tests or module-internal cleanliness.

**Blocked by:** 19 — Extract Desktop Integration Effects; 20 — Extract Migration Module.

**Status:** done

- [x] FlavorTests document the framework tax budget rule: LinuxDesktop2026
  vocabulary is acceptable at platform seams, but `ld_*` report/options/types
  crossing product-facing seams must be deliberate and justified.
- [x] The gate names repeated option-object setup, report propagation, dense enum
  combinations, unlabeled booleans, and repository-specific vocabulary as
  signals that a narrower helper may be needed.
- [x] The gate distinguishes mechanism APIs from application policy. Product
  code should keep file formats, prompt policy, profile names, and UI behavior.
- [x] The README or roadmap says FlavorTests are now an ergonomics input, not
  only a proof that the current APIs can be wired into real-shaped code.
