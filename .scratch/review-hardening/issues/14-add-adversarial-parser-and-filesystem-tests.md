# 14 — Add Adversarial Parser And Filesystem Tests

**What to build:** Security-sensitive parser and filesystem behavior should be exercised with hostile inputs and failure modes before any stable or ship-ready claim.

**Blocked by:** 23 — Add Common Config Write Facade; 24 — Add Settings Root Construction Helpers; 25 — Add Clear Config Defaults Alias; 26 — Add Ergonomic Migration Action Helpers; 27 — Record FlavorTest API Friction Notes.

**Status:** ready-for-agent

- [ ] Registry, `.reg`, and JSON-shaped import/export code has malformed and hostile-input tests or fuzz-style coverage.
- [ ] Filesystem write paths cover permissions, existing files, cleanup failures, and other practical error paths.
- [ ] Diagnostics for rejected or unsupported inputs are asserted by tests.
- [ ] Adversarial coverage targets the post-ergonomics public entry points, not
  only the older low-level option-object APIs.
