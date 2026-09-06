# 103 - Decide Diagnostic Stringification Location

**What to build:** Decide whether diagnostic and enum stringification should
remain header-defined through `0.2.0` or move behind implementation files before
release-candidate work.

**Blocked by:** None.

**Status:** implemented

- [x] Inventory public `to_string` functions and diagnostic-code constants that
  are currently header-defined.
- [x] Decide whether the current shape is acceptable for pre-1.0 source
  compatibility or whether compile-time/binary-size/API hygiene costs justify a
  move.
- [x] If moving, preserve exhaustiveness checks and document any source
  migration path.
- [x] If keeping, record the rationale in the roadmap or API stability notes
  and defer the cleanup to release-candidate hardening.

## Decision

Keep the current public diagnostic and enum stringification shape through
`0.2.0`.

Most module enum stringification is already declared in public headers and
defined out-of-line in compiled module sources:

- `ld_settings`: `portable_level`, `config_layer_kind`, `storage_backend`.
- `ld_paths`: `path_family`, `location_role`, `candidate_source`,
  `directory_action`, `plugin_path_kind`, `plugin_asset_path_kind`,
  `plugin_path_category`, `platform_support`.
- `ld_root`: `portable_root_level`, `purpose_kind`, `ownership_kind`,
  `component_kind`.
- `ld_watch`: `event_kind`, `path_type`, `recursive_policy`,
  `overflow_policy`, `stream_state`, `backend_kind`.
- `ld_migration`: `migration_action_kind`, `migration_action_state`,
  Registry `hive`, `view`, `value_type`.
- `ld_desktop`: `effect_kind`, `capability_state`, `activation_step_kind`,
  `registration_scope`, `registration_status`, `cleanup_status`.

The remaining header-defined shared diagnostic helpers live in `ld_core`, which
is intentionally an interface target. Moving its non-template helpers or
diagnostic disposition table behind a compiled target before `0.2.0` would add
link/package surface without current compile-time, binary-size, or consumer
evidence that the header shape is harmful.

Named diagnostic-code constants are header-defined in `ld_paths` and `ld_watch`
as `inline constexpr std::string_view` values. Keep them for `0.2.0` because
they are source-level vocabulary for tests and adapters, not an
ownership-bearing string API or a stable C ABI promise.

## Follow-Up

Release-candidate hardening may move `ld_core` non-template helpers
out-of-line, generate diagnostic-code/disposition tables from one source, or
add stronger generated exhaustiveness checks. Any move should preserve the
current module-level enum `to_string` declarations and include source migration
notes.

## Evidence Fit

Source review should catch this: the risk is low-priority API hygiene turning
into accidental ABI or compile-time surface area as modules grow.

## Release Gate

Does not block `0.2.0`.
