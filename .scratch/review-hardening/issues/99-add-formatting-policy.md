# 99 - Add Formatting Policy

**What to build:** Add a repository formatting baseline before `0.2.0` so new
contributors and review-hardening patches do not drift stylistically.

**Blocked by:** None.

**Status:** implemented

- [x] Add a `.clang-format` that matches the current C/C++ style with minimal
  churn.
- [x] Document the formatting command in contributor or build notes.
- [x] Decide whether CI should check formatting immediately or defer enforcement
  until after the first baseline formatting pass.
- [x] Avoid broad mechanical reformatting unless it is explicitly chosen as a
  separate cleanup step.

## Implementation Notes

Added a repository `.clang-format` baseline and documented the one-shot
formatting command in `README.md`. CI enforcement is deliberately deferred until
a separate baseline-format pass so review-hardening patches avoid broad
mechanical churn.

## Evidence Fit

Source review plus contributor onboarding should catch this: the risk is
unreviewable style drift in a growing pre-1.0 C++ surface.

## Release Gate

Resolved for `0.2.0`.
