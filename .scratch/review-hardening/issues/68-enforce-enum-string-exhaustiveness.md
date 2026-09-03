# 68 - Enforce Enum String Exhaustiveness

**What to build:** Make missing `to_string()` cases fail loudly during normal
development.

**Blocked by:** None.

**Status:** pending

- [ ] Enable `-Wswitch-enum` for non-MSVC builds and the closest practical MSVC
  equivalent.
- [ ] Decide whether `switch-enum` warnings should be errors globally or only
  for library targets.
- [ ] Update all enum stringification functions so the build stays clean.
- [ ] Keep fallback behavior only where invalid external values genuinely need
  runtime handling.

## Review Anchor

The broad review found many enum `to_string()` functions with a final
`"unknown"` fallback and no `-Wswitch-enum`, making new enum cases easy to miss.
