# 33 - Add Walnut FlavorTest Candidate

**What to build:** Evaluate StudioCherno/Walnut as a new FlavorTest candidate
and choose a first source-anchored seam.

**Blocked by:** None.

**Status:** proposed

- [ ] The candidate note records Walnut as intended for FlavorTest treatment,
  not only a challenge project.
- [ ] The note identifies one source file and one method or function as the
  first seam.
- [ ] The chosen seam pressures LinuxDesktop2026 platform placement,
  diagnostics, bootstrap, or future loader behavior rather than GUI rendering.
- [ ] Build/setup portability is kept out of scope unless it directly affects
  the selected seam.

## Candidate Facts

Walnut is a small Windows-first C++ application framework built around Vulkan
and Dear ImGui. Its README says Windows is currently supported, macOS and Linux
are planned, and the setup path is Visual Studio 2022 oriented.

## Likely First Seam

Start with application bootstrap and resource/root discovery for a Vulkan/ImGui
desktop app. Only consider plugin or dynamic-library loading if the source has a
small extension seam that can be extracted cleanly.
