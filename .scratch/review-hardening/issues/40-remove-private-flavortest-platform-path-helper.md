# 40 — Remove Private FlavorTest Platform Path Helper

**What to build:** FlavorTests should stop depending on private platform path
scaffolding and instead use the public `ld_paths` platform-default API that real
users receive.

**Blocked by:** 37 — Add Runtime Platform Path Defaults.

**Status:** implemented

- [x] Walnut FlavorTests resolve deterministic user roots through public runtime path defaults.
- [x] OpenIPC Dashboard FlavorTests resolve deterministic desktop profile roots through public runtime path defaults.
- [x] The private FlavorTest platform path helper is deleted or reduced until no test-only path-default policy remains there.
- [x] FlavorTest assertions remain product-shaped and do not expose raw LinuxDesktop2026 reports at product boundaries.
- [x] The FlavorTest API friction notes record that the hidden helper gap has been closed by public API.
