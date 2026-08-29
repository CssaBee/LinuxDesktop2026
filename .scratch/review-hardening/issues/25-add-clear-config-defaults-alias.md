# 25 — Add Clear Config Defaults Alias

**What to build:** Add clearer C++ naming for config model-file hydration so
new consumers can discover the intent without first learning repository-specific
"hydrate" vocabulary.

**Blocked by:** 23 — Add Common Config Write Facade; 24 — Add Settings Root Construction Helpers; 26 — Add Ergonomic Migration Action Helpers.

**Status:** ready-for-agent

- [ ] A clearer alias such as `ensure_config_defaults()` or
  `seed_config_files()` forwards to the existing hydration implementation.
- [ ] The new name does not hide the caller-visible concepts that still matter:
  model roots, target roots, file metadata, and diagnostics.
- [ ] Documentation explains the boundary: LinuxDesktop2026 may copy missing
  shipped defaults, but applications still own parsing, validation, and merge
  policy.
- [ ] FlavorTests and examples migrate to the clearer name where it improves
  readability.
- [ ] The existing `hydrate_config_bundle()` C++ API remains available during
  the pre-1.0 transition unless a later API cleanup deliberately removes it.
