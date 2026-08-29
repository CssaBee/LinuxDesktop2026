# 30 - Translate Migration Plans At Product Boundaries

**What to build:** KeePassXC, FreeCAD, and PrusaSlicer should translate
`linuxdesktop::migration::migration_plan` into product-shaped migration results
before those values leave adapter code.

**Blocked by:** 20 - Extract Migration Module; 27 - Record FlavorTest API
Friction Notes.

**Status:** done

- [x] KeePassXC, FreeCAD, and PrusaSlicer public headers no longer expose
  `linuxdesktop::migration::migration_plan`.
- [x] Each flavor still exposes source path, target path, dry-run state,
  availability or blocked state, and prompt/action intent in product vocabulary.
- [x] Tests assert the product-facing result shape rather than the raw
  LinuxDesktop2026 plan object.
- [x] `docs/FlavorTests/API_FRICTION.md` records the before/after judgment for
  each translated seam.

## Problem Statement

The migration helpers are usable, but the current FlavorTests still let
LinuxDesktop2026's plan type cross product boundaries in three places. That
keeps callers thinking in framework vocabulary where they should be thinking in
KeePassXC, FreeCAD, or PrusaSlicer prompt and migration terms.

## Implementation Notes

Keep `migration_plan` internal to the adapter implementation. Add small
product-shaped result types that carry only the source/target and user decision
facts the product needs.
