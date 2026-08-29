# 08 — Add Durable Write Mode Where Promised

**What to build:** If callers request or documentation promises crash durability, the write path should actually flush file contents and parent metadata where the platform supports it.

**Blocked by:** 06 — Correct Write-Safety Semantics; 07 — Implement Secure Temporary Writes.

**Status:** done

- [x] Durable write behavior is opt-in or clearly documented as the default only where implemented.
- [x] POSIX durable replacement flushes the temporary file before rename and the parent directory after rename where supported.
- [x] Unsupported durability guarantees produce diagnostics rather than silent overpromising.
