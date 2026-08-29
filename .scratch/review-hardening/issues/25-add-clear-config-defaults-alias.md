# 25 — Add Clear Config Defaults Alias

**What to build:** Add clearer C++ naming for config model-file hydration so
new consumers can discover the intent without first learning repository-specific
"hydrate" vocabulary.

**Blocked by:** 23 — Add Common Config Write Facade; 24 — Add Settings Root Construction Helpers; 26 — Add Ergonomic Migration Action Helpers.

**Status:** implemented

- [x] A clearer alias such as `ensure_config_defaults()` or
  `seed_config_files()` forwards to the existing hydration implementation.
- [x] The new name does not hide the caller-visible concepts that still matter:
  model roots, target roots, file metadata, and diagnostics.
- [x] Documentation explains the boundary: LinuxDesktop2026 may copy missing
  shipped defaults, but applications still own parsing, validation, and merge
  policy.
- [x] FlavorTests and examples migrate to the clearer name where it improves
  readability.
- [x] The existing `hydrate_config_bundle()` C++ API remains available during
  the pre-1.0 transition unless a later API cleanup deliberately removes it.

Implemented as `ld_settings::ensure_config_defaults(const hydrate_options&)`
with the existing `hydrate_options`/`hydrate_report` types so model roots,
target roots, per-file required flags, copied/skipped files, and diagnostics
remain visible. `hydrate_config_bundle()` now forwards to the clearer name for
pre-1.0 source compatibility.
