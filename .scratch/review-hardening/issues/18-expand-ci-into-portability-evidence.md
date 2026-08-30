# 18 — Expand CI Into Portability Evidence

**What to build:** CI should move beyond smoke coverage and produce credible portability evidence for the supported platforms and toolchains, building on the first Fedora signal.

**Blocked by:** 13 — Add Fedora CI Baseline; 15 — Validate APIs With One Maintained Consumer Branch.

**Status:** implemented-awaiting-ci

- [x] Build policy applies `-Wall -Wextra -Wpedantic -Wconversion` on GCC and Clang, with a reasonable MSVC warning level.
- [x] CI has one Ubuntu sanitizer lane using ASan/UBSan.
- [x] CI covers GCC and Clang on Linux, MSVC on Windows, and both Debug and Release configurations where practical.
- [x] CI includes shared-library builds and sanitizer jobs where supported.
- [x] CI includes an older supported Ubuntu baseline or containerized equivalent where practical.
- [x] The maintained consumer branch or its smallest reproducible integration
  slice has a documented CI path before the matrix is treated as release
  evidence.

Implementation note:

The main CI matrix now includes Ubuntu GCC Debug/static, Ubuntu Clang
Release/shared, Ubuntu 22.04 container GCC Release/static, Fedora container GCC
Debug/shared, and Windows MSVC Debug/Release static lanes. ASan/UBSan now runs
with both GCC and Clang. Windows shared-library builds stay deferred until the
project has an explicit symbol-export policy.

The Notepad++ proof branch has a manually dispatched GitHub Actions path in
`.github/workflows/notepadpp-proof.yml`; it installs the current
LinuxDesktop2026 checkout as a CMake package and builds the smallest maintained
consumer proof target from `CssaBee/LinuxDesktop2026-crossport-notepadpp` once
that proof branch is available remotely.
