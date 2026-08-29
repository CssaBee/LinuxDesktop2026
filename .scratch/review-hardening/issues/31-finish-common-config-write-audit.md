# 31 - Finish Common Config Write Audit

**What to build:** Revisit every remaining FlavorTest direct
`write_with_backup()` call and either replace it with `write_common_config()` or
document why the lower-level call is clearer for that product seam.

**Blocked by:** 23 - Add Common Config Write Facade; 27 - Record FlavorTest API
Friction Notes.

**Status:** proposed

- [ ] qBittorrent file logger settings are either migrated to
  `write_common_config()` or explicitly justified.
- [ ] KeePassXC settings export is either migrated to `write_common_config()` or
  explicitly justified.
- [ ] KiCad JSON settings saves are either migrated to `write_common_config()`
  or explicitly justified.
- [ ] FreeCAD user-parameter saves are either migrated to `write_common_config()`
  or explicitly justified.
- [ ] OpenRGB's local JSON wrapper is classified as useful product glue or
  evidence for a JSON-oriented convenience helper.
- [ ] OBS keeps its C-shaped lower-level save only if the reason is documented.
- [ ] Tests still cover validation-before-commit, backup behavior, target
  preservation on failure, and product-shaped return values.

## Problem Statement

Audacity and PrusaSlicer became clearer after adopting `write_common_config()`.
Several other flavors still use the lower-level write API for saves that look
like ordinary validated config writes. The audit should decide which calls are
real exceptions and which are leftover framework tax.
