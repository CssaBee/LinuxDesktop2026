# 38 — Mirror Platform Path Defaults In C ABI

**What to build:** C callers should be able to provide the same platform path
defaults as C++ callers while keeping the existing flat option and report
ownership model.

**Blocked by:** 37 — Add Runtime Platform Path Defaults.

**Status:** ready-for-agent

- [ ] C resolver options expose flat fields for the supported XDG and Windows app-root defaults.
- [ ] C resolver initialization leaves all default fields unset unless the caller provides them.
- [ ] C resolver behavior matches the C++ precedence and diagnostic rules.
- [ ] C tests cover default-derived config/data/state/cache/runtime selection and memory cleanup through the existing free function.
- [ ] Documentation or inline API comments make clear that these are defaults, not explicit overrides.
