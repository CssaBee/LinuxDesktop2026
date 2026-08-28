# 14 — Add Adversarial Parser And Filesystem Tests

**What to build:** Security-sensitive parser and filesystem behavior should be exercised with hostile inputs and failure modes before any stable or ship-ready claim.

**Blocked by:** 05 — Narrow Settings Effects To Prototype-Validated Behavior; 06 — Correct Write-Safety Semantics.

**Status:** ready-for-agent

- [ ] Registry, `.reg`, and JSON-shaped import/export code has malformed and hostile-input tests or fuzz-style coverage.
- [ ] Filesystem write paths cover permissions, existing files, cleanup failures, and other practical error paths.
- [ ] Diagnostics for rejected or unsupported inputs are asserted by tests.
