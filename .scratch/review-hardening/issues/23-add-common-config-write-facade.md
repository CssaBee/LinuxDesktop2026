# 23 — Add Common Config Write Facade

**What to build:** Add a narrow high-level C++ API for the common validated
settings/config write path while keeping `write_with_backup()` as the lower-level
primitive.

**Blocked by:** 22 — Translate Library Reports At Product Boundaries.

**Status:** done

- [x] Callers can request the common "validated config write with backup,
  atomic replacement, and optional durable mode" behavior through an explicitly
  named profile or request helper, without rebuilding a full `write_options`
  object at every call site.
- [x] The facade preserves the task 06-08 distinction between backup, atomic
  namespace replacement, and crash-durable flushing; no helper silently implies
  durability without saying so.
- [x] The API still accepts application-owned validation callbacks; LinuxDesktop2026
  must not own XML, INI, JSON, profile, or session semantics.
- [x] Audacity, Notepad++, and PrusaSlicer FlavorTests use the facade where
  their current repeated `write_options` setup is mechanical.
- [x] Tests prove the facade preserves backup, validation-before-commit,
  rollback/readback diagnostics, and durable-mode reporting.
