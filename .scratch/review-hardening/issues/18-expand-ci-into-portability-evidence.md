# 18 — Expand CI Into Portability Evidence

**What to build:** CI should move beyond smoke coverage and produce credible portability evidence for the supported platforms and toolchains, building on the first Fedora signal.

**Blocked by:** 13 — Add Fedora CI Baseline; 15 — Validate APIs With One Maintained Consumer Branch.

**Status:** ready-for-agent

- [x] Build policy applies `-Wall -Wextra -Wpedantic -Wconversion` on GCC and Clang, with a reasonable MSVC warning level.
- [x] CI has one Ubuntu sanitizer lane using ASan/UBSan.
- [ ] CI covers GCC and Clang on Linux, MSVC on Windows, and both Debug and Release configurations where practical.
- [ ] CI includes shared-library builds and sanitizer jobs where supported.
- [ ] CI includes an older supported Ubuntu baseline or containerized equivalent where practical.
- [ ] The maintained consumer branch or its smallest reproducible integration
  slice has a documented CI path before the matrix is treated as release
  evidence.
