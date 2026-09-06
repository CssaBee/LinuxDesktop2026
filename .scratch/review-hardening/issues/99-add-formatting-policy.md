# 99 - Add Formatting Policy

**What to build:** Add a repository formatting baseline before `0.2.0` so new
contributors and review-hardening patches do not drift stylistically.

**Blocked by:** None.

**Status:** pending

- [ ] Add a `.clang-format` that matches the current C/C++ style with minimal
  churn.
- [ ] Document the formatting command in contributor or build notes.
- [ ] Decide whether CI should check formatting immediately or defer enforcement
  until after the first baseline formatting pass.
- [ ] Avoid broad mechanical reformatting unless it is explicitly chosen as a
  separate cleanup step.

## Evidence Fit

Source review plus contributor onboarding should catch this: the risk is
unreviewable style drift in a growing pre-1.0 C++ surface.

## Release Gate

Blocks `0.2.0`.
