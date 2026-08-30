# 37 — Add Runtime Platform Path Defaults

**What to build:** `ld_paths` users should be able to pass supported platform
path defaults through resolver options and get the same selected app roots that
FlavorTests currently obtain through private helper scaffolding.

**Blocked by:** None — can start immediately.

**Status:** implemented

- [x] C++ resolver options accept an optional platform-default value object for app path resolution.
- [x] The value object covers XDG config, data, state, cache, runtime and Windows roaming/local AppData defaults.
- [x] XDG and Windows factory helpers construct the common default layouts without encoding portable-mode product policy.
- [x] Resolver precedence is explicit overrides, injected environment, process environment or OS APIs, platform defaults, then built-in fallback.
- [x] Selected paths, candidate sources, and diagnostics make default-derived behavior observable through public resolver reports.
- [x] Relative or otherwise invalid defaults are rejected through resolver diagnostics rather than silently selected.
