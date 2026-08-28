# 05 — Narrow Settings Effects To Prototype-Validated Behavior

**What to build:** Settings-adjacent effects should no longer look like stable general-purpose features unless real integrations have validated them; unsupported or unproven behavior should be marked prototype-only, extracted, or postponed.

**Blocked by:** 03 — Define Settings Module Boundaries.

**Status:** ready-for-agent

- [ ] Registry, autostart, policy, and migration execution docs identify what is prototype evidence versus intended ship behavior.
- [ ] Public claims are reduced where current tests do not cover hostile input, rollback, permissions, or real application integration.
- [ ] Any retained effect behavior has focused tests and clear diagnostics for unsupported platforms or incomplete backends.
