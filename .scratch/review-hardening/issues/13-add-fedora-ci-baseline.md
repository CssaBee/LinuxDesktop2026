# 13 — Add Fedora CI Baseline

**What to build:** CI should run the existing CMake build and test suite on a Fedora-based environment, giving the project an immediate non-Ubuntu Linux signal before the broader portability matrix is expanded.

**Blocked by:** None — can start immediately.

**Status:** done

- [x] CI has a Fedora-based job or containerized job that configures, builds, and runs the current test suite.
- [x] The Fedora job installs only the packages needed for the existing CMake/Ninja test path.
- [x] Fedora failures are visible as a distinct CI result, not hidden inside a generic Linux job.
