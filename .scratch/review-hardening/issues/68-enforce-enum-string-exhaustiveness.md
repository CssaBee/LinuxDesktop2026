# 68 - Enforce Enum String Exhaustiveness

**What to build:** Make missing `to_string()` cases fail loudly during normal
development.

**Blocked by:** None.

**Status:** implemented

- [x] Enable `-Wswitch-enum` for non-MSVC builds and the closest practical MSVC
  equivalent.
- [x] Decide whether `switch-enum` warnings should be errors globally or only
  for library targets.
- [x] Update all enum stringification functions so the build stays clean.
- [x] Keep fallback behavior only where invalid external values genuinely need
  runtime handling.

## Implementation Note

The project now treats missing enum cases as build failures during ordinary
development: non-MSVC builds use `-Wswitch-enum -Werror=switch-enum`, and MSVC
uses `/we4062`. The setting is global to this repository's C and C++ targets so
header-only enum helpers are checked by tests and demos too, but the package
does not export those warning flags to install-tree consumers.

Existing fallback returns remain after exhaustive switches to preserve runtime
handling for invalid external values, such as casted C ABI inputs or corrupted
serialized values. Partial bucket switches in `ld_paths` and `ld_root` now spell
out every current enumerator explicitly, making future enum additions fail the
build until the intended behavior is chosen.

## Review Anchor

The broad review found many enum `to_string()` functions with a final
`"unknown"` fallback and no `-Wswitch-enum`, making new enum cases easy to miss.
