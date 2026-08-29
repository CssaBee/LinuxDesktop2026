# 07 — Implement Secure Temporary Writes

**What to build:** Temporary files used for replacement writes should be created securely with platform-safe exclusive creation instead of predictable check-then-open paths.

**Blocked by:** 06 — Correct Write-Safety Semantics.

**Status:** done

- [x] Temporary file creation uses exclusive creation semantics on each supported platform.
- [x] Predictable temp-name races are removed from the write path.
- [x] Tests cover existing temp files, hostile collisions where practical, cleanup after failure, and preservation of the original target on validation failure.
