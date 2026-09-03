# 69 - Share Durable File Write Primitive With Desktop

**What to build:** Move the settings-grade atomic write behavior into a shared
internal primitive and use it for desktop integration files.

**Blocked by:** None.

**Status:** pending

- [ ] Extract a shared internal write helper that supports temp file, flush,
  atomic replace, parent-directory sync where available, and diagnostics.
- [ ] Replace `ld_desktop` direct truncate-and-write behavior for autostart,
  policy defaults, and policy lock files.
- [ ] Add interruption/failure tests for desktop writes that mirror the
  settings module's durability expectations.
- [ ] Keep module dependencies clean; do not make `ld_desktop` depend on
  `ld_settings`.

## Review Anchor

The broad review found `desktop.cpp` writes generated files with
`std::ofstream` and `std::ios::trunc`, while `settings.cpp` already implements
stronger atomic write behavior.
