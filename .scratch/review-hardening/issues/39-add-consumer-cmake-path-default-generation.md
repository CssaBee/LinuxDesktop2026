# 39 — Add Consumer CMake Path Default Generation

**What to build:** A downstream CMake consumer should be able to generate
target-local path-default helper code from the installed LinuxDesktop2026
package and pass those defaults explicitly into `ld_paths`.

**Blocked by:** 37 — Add Runtime Platform Path Defaults.

**Status:** implemented

- [x] The CMake package exposes a consumer-target helper for generating platform path defaults without baking app or user roots into the shared LinuxDesktop2026 library.
- [x] Generated defaults are used explicitly by consumer code through resolver options.
- [x] The helper supports the same first-pass XDG and Windows app-root defaults as the runtime API.
- [x] Install-tree consumer coverage configures, builds, links, and runs with generated defaults from an installed package.
- [x] The generated helper avoids hidden global resolver state.
