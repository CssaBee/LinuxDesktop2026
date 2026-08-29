# 31 - Finish Common Config Write Audit

**What to build:** Revisit every remaining FlavorTest direct
`write_with_backup()` call and either replace it with `write_common_config()` or
document why the lower-level call is clearer for that product seam.

**Blocked by:** 23 - Add Common Config Write Facade; 27 - Record FlavorTest API
Friction Notes.

**Status:** done

- [x] qBittorrent file logger settings are either migrated to
  `write_common_config()` or explicitly justified.
- [x] KeePassXC settings export is either migrated to `write_common_config()` or
  explicitly justified.
- [x] KiCad JSON settings saves are either migrated to `write_common_config()`
  or explicitly justified.
- [x] FreeCAD user-parameter saves are either migrated to `write_common_config()`
  or explicitly justified.
- [x] OpenRGB's local JSON wrapper is classified as useful product glue or
  evidence for a JSON-oriented convenience helper.
- [x] OBS keeps its C-shaped lower-level save only if the reason is documented.
- [x] Tests still cover validation-before-commit, backup behavior, target
  preservation on failure, and product-shaped return values.

## Problem Statement

Audacity and PrusaSlicer became clearer after adopting `write_common_config()`.
Several other flavors still use the lower-level write API for saves that look
like ordinary validated config writes. The audit should decide which calls are
real exceptions and which are leftover framework tax.
