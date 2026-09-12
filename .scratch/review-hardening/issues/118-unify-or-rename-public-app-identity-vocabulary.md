# 118 - Unify Or Rename Public App Identity Vocabulary

**What to build:** Remove ambiguous duplication among public `app_identity`
types across modules.

**Blocked by:** None.

**Status:** implemented

- [x] Inventory `paths::app_identity`, `root::app_identity`,
  `settings::app_identity`, and any parallel portable-mode vocabulary.
- [x] Decide whether the identity concepts are genuinely identical across
  modules.
- [x] If identical, extract one small shared public value type in the lowest
  appropriate module and provide pre-1.0 migration guidance.
- [x] If distinct, rename the concepts so each name reflects its actual domain
  semantics.
- [x] Add compile tests or API examples that prevent silent re-duplication of
  the same concept.
- [x] Update glossary/docs to use the accepted canonical vocabulary.

## Implementation Notes

The three public C++ identity structs were identical: an organization segment
and an application segment used to resolve app-scoped roots. Portable-mode
vocabulary remains separate policy vocabulary (`portable_root_request`,
`portable_root_level`, and `settings::portable_level`) rather than an identity
concept.

Task 118 makes `linuxdesktop::app_identity` the canonical shared value in
`ld_core`. `linuxdesktop::paths::app_identity`,
`linuxdesktop::root::app_identity`, and
`linuxdesktop::settings::app_identity` are source-compatible aliases of that
type, while migration rooted-path requests name the canonical type directly.

`ld_public_app_identity_tests` compile-checks that all public C++ identity
spellings remain the same type.

## Review Anchor

The September 6 clean re-review found three identically shaped public
`app_identity` concepts in `ld_paths`, `ld_root`, and `ld_settings`. If these
grow differently later, their shared name will become misleading; if they are
the same concept, duplication is needless API drift.

## Evidence Fit

API inventory and compile coverage should catch this: the risk is pre-1.0
semantic ambiguity hardening into a long-lived public surface.

## Release Gate

Resolved for `0.2.1`.
