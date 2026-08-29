# 06 — Correct Write-Safety Semantics

**What to build:** Callers should be able to tell exactly what write safety they are getting: atomic namespace replacement, backup behavior, validation-before-commit, and crash durability must be separate documented guarantees.

**Blocked by:** None — can start immediately.

**Status:** done

- [x] Documentation distinguishes atomic replacement from crash-durable commit.
- [x] Write reports and diagnostics use language that does not overpromise durability.
- [x] Tests verify the documented behavior for direct writes, replacement writes, validation failure, backup creation, and readback failure or mismatch.
