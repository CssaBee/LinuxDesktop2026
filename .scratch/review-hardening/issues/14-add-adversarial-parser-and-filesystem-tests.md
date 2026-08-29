# 14 — Add Adversarial Parser And Filesystem Tests

**What to build:** Security-sensitive parser and filesystem behavior should be exercised with hostile inputs and failure modes before any stable or ship-ready claim.

**Blocked by:** 23 — Add Common Config Write Facade; 24 — Add Settings Root Construction Helpers; 25 — Add Clear Config Defaults Alias; 26 — Add Ergonomic Migration Action Helpers; 27 — Record FlavorTest API Friction Notes.

**Status:** done

- [x] Registry, `.reg`, and JSON-shaped import/export code has malformed and hostile-input tests or fuzz-style coverage.
- [x] Filesystem write paths cover permissions, existing files, cleanup failures, and other practical error paths.
- [x] Diagnostics for rejected or unsupported inputs are asserted by tests.
- [x] Adversarial coverage targets the post-ergonomics public entry points, not
  only the older low-level option-object APIs.

**Implementation note:** Added hostile Registry JSON and `.reg` parser/import
coverage, tightened JSON snapshot acceptance around the format marker and
values array, and exercised deterministic `write_common_config()` filesystem
failure paths for file-as-parent and backup-collision cleanup.
