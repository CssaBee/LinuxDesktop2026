# 22 — Translate Library Reports At Product Boundaries

**What to build:** Keep LinuxDesktop2026 rich reports useful inside adapters
while discouraging product-shaped public APIs from exposing library report types
unless the product intentionally adopts them as public infrastructure.

**Blocked by:** 21 — Define Flavor API Exposure Budget.

**Status:** done

- [x] FlavorTests identify public methods that currently return or store
  LinuxDesktop2026 report types where a product-native status, enum, boolean, or
  diagnostic summary would fit better.
- [x] OBS keeps its C-shaped integer/buffer API boundary while still using
  LinuxDesktop2026 internally.
- [x] Notepad++ and other C++ Flavor slices translate write/root/migration
  reports at product boundaries when doing so improves local reasoning.
- [x] New write/root/migration convenience APIs are not treated as complete until
  their FlavorTest usage avoids unnecessary report leakage.
- [x] Documentation states that full LinuxDesktop2026 report propagation is fine
  for internal adapter code, diagnostics consoles, and applications that opt into
  the library as part of their public platform layer.
