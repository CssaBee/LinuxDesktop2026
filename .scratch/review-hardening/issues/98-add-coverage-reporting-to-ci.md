# 98 - Add Coverage Reporting To CI

**What to build:** Exercise the existing coverage instrumentation in CI so the
coverage claim is not local-only.

**Blocked by:** `81` - Add Coverage Failure-Mode And Watch Performance
Evidence.

**Status:** implemented

- [x] Add a CI lane that configures with `LD2026_ENABLE_COVERAGE=ON`.
- [x] Install or provision the coverage report tool used by the CMake coverage
  target.
- [x] Run the coverage target after the relevant unit and smoke tests.
- [x] Publish or upload the generated XML/HTML artifact in a way that works for
  pull requests and normal pushes.
- [x] Update `docs/validation-evidence.md` with the CI command, artifact name,
  and any intentionally excluded targets.

## Implementation Notes

`.github/workflows/ci.yml` now has a dedicated Ubuntu/GCC `coverage` job. It
installs `gcovr`, configures `build-coverage` with
`LD2026_ENABLE_COVERAGE=ON`, builds the test suite, runs `ld2026_coverage`, and
uploads `ld2026-coverage-report` with both the HTML details and XML report.

## Evidence Fit

CI should catch this: the risk is public validation text claiming coverage
support when only local instrumentation exists.

## Release Gate

Resolved for `0.2.0`.
