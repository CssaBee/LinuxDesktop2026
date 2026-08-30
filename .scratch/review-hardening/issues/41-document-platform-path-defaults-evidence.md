# 41 — Document Platform Path Defaults Evidence

**What to build:** The roadmap, examples, and hardening notes should explain
runtime platform defaults, C ABI defaults, CMake generated defaults, and the
FlavorTest evidence that justified adding this public `ld_paths` vocabulary.

**Blocked by:** 38 — Mirror Platform Path Defaults In C ABI; 39 — Add Consumer CMake Path Default Generation; 40 — Remove Private FlavorTest Platform Path Helper.

**Status:** ready-for-agent

- [ ] The path roadmap explains platform path defaults as `ld_paths` hardening, not broad module expansion.
- [ ] Public examples show real consumers passing defaults through resolver options rather than copying private FlavorTest helpers.
- [ ] C documentation covers the new flat default fields and their precedence.
- [ ] CMake consumption documentation shows generated target-local defaults being passed explicitly.
- [ ] The FlavorTest API friction notes preserve the lesson that test-only helpers must not hide unsupported user workflows.
