# 88 - Prove Windows Registration Capability Posture

**What to build:** Add Windows 10/11 `ld_desktop` capability and diagnostic
evidence for registration effects without overpromising forced defaults or live
shell behavior.

**Blocked by:** 86 - Design Desktop Bundle Registration Surface.

**Status:** pending

- [ ] Define the Windows registration mapping for autostart, desktop-entry-like
  app identity, file associations, URL protocol handlers, and policy artifacts.
- [ ] Keep Windows default-app setting honest: query/report or open-settings
  guidance is acceptable; silently forcing user defaults is not.
- [ ] Decide which Windows Registry writes require a shared Registry/system
  layer before implementation.
- [ ] Add Windows-shaped dry-run and backend-missing/backend-limited tests that
  can run in CI without mutating machine-wide state.
- [ ] Add privileged/global-write denial tests for HKLM or policy-like effects
  where CI can exercise them safely.
- [ ] Document any Windows 10 versus Windows 11 behavior differences as
  diagnostics or validation notes, not public API branches, unless a stable OS
  contract requires it.

## Review Anchor

ADR 0015 keeps Windows 10/11 inside the same standards-or-native-artifact
registration model, but Windows app defaults and shell integration are
policy-mediated enough that `ld_desktop` needs proof before exposing mutation as
supported.
