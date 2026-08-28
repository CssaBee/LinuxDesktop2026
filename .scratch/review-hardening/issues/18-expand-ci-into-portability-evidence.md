# 18 — Expand CI Into Portability Evidence

**What to build:** CI should move beyond smoke coverage and produce credible portability evidence for the supported platforms and toolchains, building on the first Fedora signal.

**Blocked by:** 13 — Add Fedora CI Baseline.

**Status:** ready-for-agent

- [ ] CI covers GCC and Clang on Linux, MSVC on Windows, and both Debug and Release configurations where practical.
- [ ] CI includes shared-library builds and sanitizer jobs where supported.
- [ ] CI includes an older supported Ubuntu baseline or containerized equivalent where practical.
