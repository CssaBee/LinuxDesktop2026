# 117 - Make Pre-1.0 CMake Package Compatibility Honest

**What to build:** Align installed-package version compatibility with the
project's pre-1.0 source-compatibility policy.

**Blocked by:** None.

**Status:** pending

- [ ] Replace `SameMajorVersion` compatibility for `0.x` packages with exact
  version matching or a custom compatibility interval.
- [ ] Preserve a sensible post-1.0 path where same-major compatibility can be
  reintroduced once the project promises it.
- [ ] Add install-tree consumer tests that prove incompatible `0.x` versions
  are rejected by `find_package()` version checks.
- [ ] Update API stability and CMake consumption docs with the machine-readable
  compatibility policy.

## Review Anchor

The September 6 clean re-review found that package generation uses
`SameMajorVersion` for version `0.2.0`, even though the project's human policy
says pre-1.0 C++ APIs may deliberately break when integration evidence requires
it.

## Evidence Fit

Install-tree tests and source review should catch this: the risk is downstream
package resolution saying a later `0.x` package is compatible while the docs
say it may not be.

## Release Gate

Blocks `0.2.1`.
