# Review Hardening History

The old queue reached 124 numbered tickets. Most were completed implementation
paths, not current work. This file keeps the useful evidence without preserving
the historical path structure as active tickets.

## Current Code Shape

- `ld_core`, `ld_settings`, `ld_paths`, `ld_root`, and `ld_watch` are supported
  prototype modules with tests, examples, install-tree evidence, and explicit
  pre-1.0 caveats.
- `ld_desktop` and `ld_migration` are experimental extraction modules. Desktop
  bundle registration and broader migration behavior stay release-candidate
  gated.
- Path, root, and settings resolution are non-mutating by default; directory
  creation and other filesystem effects are explicit opt-in operations.
- `ld_watch` has bounded pull delivery, recursive-watch diagnostics, native
  Linux and Windows backends, optional libuv, and deadline-scheduled
  settled-file coalescing by path.
- Registry import/export is a narrow app-settings compatibility surface backed
  by a maintained JSON parser for snapshot data, not a broad file-format claim.
- Versioned settings commits reject stale/mismatched writes and use a resolved
  sidecar lock domain where the platform can prove it.

## Completed Review Themes

- Public claims now separate prototype support, experimental extraction,
  research-only modules, private proof status, and release gates.
- Settings no longer owns desktop effects or migration behavior.
- Durable write behavior is shared where needed and described without implying
  stronger cross-process guarantees than implemented.
- Root/path vocabulary distinguishes path families, location roles, named roots,
  product-owned diagnostics, and portable root requests.
- Watcher hardening covered callback lifetime, self-stop behavior, queue
  overflow isolation, test-hook gating, settled-worker liveness, bounded
  settled-file work, and scheduler fairness.
- Migration hardening covered rooted path containment, directory verification,
  parser scope, action-result vocabulary, and the decision to exclude semantic
  cross-device file moves for `0.2.0`.
- Desktop hardening covered staged XDG artifacts, Windows capability posture,
  bundle/report vocabulary, cleanup reports, FlavorTest validation, maintained
  proof evidence, and the decision to keep broad bundle registration C++-only.
- Validation hardening added Fedora, Windows/MSVC, shared-library Linux,
  ASan/UBSan, deterministic `ld_watch` ThreadSanitizer, optional libuv, coverage
  reporting, formatting baseline, FlavorTests, and install-tree consumers.

## Long-Running Evidence

- Maintained consumer proof: tracked in
  `docs/consumer-branches/notepadpp-settings-proof.md`.
- API friction and future helper triggers: tracked in
  `docs/FlavorTests/API_FRICTION.md`.
- Current release support and planned validation: tracked in
  `docs/project-status.md`.
- Review blind-spot checklist: tracked in
  `docs/review-hardening-retrospective.md`.
