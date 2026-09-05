# 25 — Replace Config Defaults Hydration Name

**What to build:** Add clearer naming for config model-file defaults so
new consumers can discover the intent without first learning repository-specific
"hydrate" vocabulary.

**Blocked by:** 23 — Add Common Config Write Facade; 24 — Add Settings Root Construction Helpers; 26 — Add Ergonomic Migration Action Helpers.

**Status:** implemented

- [x] `ensure_config_defaults()` is the public C++ name for config model-file
  copy behavior.
- [x] `ld_settings_ensure_config_defaults()` is the public C entry point for
  the same behavior.
- [x] The new name does not hide the caller-visible concepts that still matter:
  model roots, target roots, file metadata, and diagnostics.
- [x] Documentation explains the boundary: LinuxDesktop2026 may copy missing
  shipped defaults, but applications still own parsing, validation, and merge
  policy.
- [x] FlavorTests and examples migrate to the clearer name where it improves
  readability.
- [x] The old `hydrate_config_bundle()` and
  `ld_settings_hydrate_config_bundle()` names were removed as intentional
  pre-1.0 source breaks.

Implemented as `linuxdesktop::settings::ensure_config_defaults(const
config_defaults_options&)` and `ld_settings_ensure_config_defaults(const
ld_settings_config_defaults_options*, ld_settings_config_defaults_report*)`.
The public forwarding functions and old `hydrate_*` type names were removed
because pre-1.0 cleanup should prefer one clear API over compatibility wrappers.
