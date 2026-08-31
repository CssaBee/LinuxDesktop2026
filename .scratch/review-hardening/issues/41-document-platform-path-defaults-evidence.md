# 41 — Document Platform Path Defaults Evidence

**What to build:** The roadmap, examples, and hardening notes should explain
runtime platform defaults, C ABI defaults, CMake generated defaults, and the
FlavorTest evidence that justified adding this public `ld_paths` vocabulary.

**Blocked by:** 38 — Mirror Platform Path Defaults In C ABI; 39 — Add Consumer CMake Path Default Generation; 40 — Remove Private FlavorTest Platform Path Helper.

**Status:** done

- [x] The path roadmap explains platform path defaults as `ld_paths` hardening, not broad module expansion.
- [x] Public examples show real consumers passing defaults through resolver options rather than copying private FlavorTest helpers.
- [x] C documentation covers the new flat default fields and their precedence.
- [x] CMake consumption documentation shows generated target-local defaults being passed explicitly.
- [x] The FlavorTest API friction notes preserve the lesson that test-only helpers must not hide unsupported user workflows.

## Result

`docs/plan/ld-paths-roadmap.md` now describes runtime defaults, flat C ABI
defaults, generated CMake defaults, resolver precedence, and the reason this is
`ld_paths` hardening rather than module expansion. The README and migration
examples show callers passing defaults through resolver options, and
`docs/plan/api-stability.md` records the C ABI fields and
`LD_PATHS_SOURCE_PLATFORM_DEFAULT` reporting contract.

`docs/FlavorTests/API_FRICTION.md` already records the consumer-ergonomics
lesson from Walnut, OpenIPC Dashboard, and the Notepad++ proof branch: future
FlavorTests should not use private path-default helpers that installed users
cannot consume.
