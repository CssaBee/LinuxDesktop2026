# 98 - Add Coverage Reporting To CI

**What to build:** Exercise the existing coverage instrumentation in CI so the
coverage claim is not local-only.

**Blocked by:** `81` - Add Coverage Failure-Mode And Watch Performance
Evidence.

**Status:** pending

- [ ] Add a CI lane that configures with `LD2026_ENABLE_COVERAGE=ON`.
- [ ] Install or provision the coverage report tool used by the CMake coverage
  target.
- [ ] Run the coverage target after the relevant unit and smoke tests.
- [ ] Publish or upload the generated XML/HTML artifact in a way that works for
  pull requests and normal pushes.
- [ ] Update `docs/validation-evidence.md` with the CI command, artifact name,
  and any intentionally excluded targets.

## Evidence Fit

CI should catch this: the risk is public validation text claiming coverage
support when only local instrumentation exists.

## Release Gate

Blocks `0.2.0`.
