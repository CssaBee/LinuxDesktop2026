# 118 - Unify Or Rename Public App Identity Vocabulary

**What to build:** Remove ambiguous duplication among public `app_identity`
types across modules.

**Blocked by:** None.

**Status:** pending

- [ ] Inventory `paths::app_identity`, `root::app_identity`,
  `settings::app_identity`, and any parallel portable-mode vocabulary.
- [ ] Decide whether the identity concepts are genuinely identical across
  modules.
- [ ] If identical, extract one small shared public value type in the lowest
  appropriate module and provide pre-1.0 migration guidance.
- [ ] If distinct, rename the concepts so each name reflects its actual domain
  semantics.
- [ ] Add compile tests or API examples that prevent silent re-duplication of
  the same concept.
- [ ] Update glossary/docs to use the accepted canonical vocabulary.

## Review Anchor

The September 6 clean re-review found three identically shaped public
`app_identity` concepts in `ld_paths`, `ld_root`, and `ld_settings`. If these
grow differently later, their shared name will become misleading; if they are
the same concept, duplication is needless API drift.

## Evidence Fit

API inventory and compile coverage should catch this: the risk is pre-1.0
semantic ambiguity hardening into a long-lived public surface.

## Release Gate

Blocks `0.2.1`.
