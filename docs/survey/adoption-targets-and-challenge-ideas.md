# Adoption Targets And Challenge Ideas

This document preserves the durable parts of the earlier out-of-tree
conversation notes. It is a survey and positioning input, not a delivery
promise.

## Positioning

LinuxDesktop2026 should not present itself as a general "make Windows software
run on Linux" layer. The stronger claim is narrower:

> A collection of small C++ platform libraries for replacing common Win32
> dependencies with native Windows/Linux implementations.

The project does not remove compiler-language portability problems such as MSVC
extensions, undefined behavior, or compiler differences. Its useful boundary is
operating-system plumbing: settings, standard paths, file watching, processes,
dynamic libraries, plugin discovery, IPC, and desktop integration.

## Target Categories

| Target group | Fit | Why it matters |
| --- | --- | --- |
| Windows-heavy native C++ desktop applications | Strong | These are the main proof targets for removing scattered platform conditionals without promising full GUI portability. |
| Plugin hosts and plugin-based applications | Strong | Settings, paths, DLL/SO discovery, dynamic loading, file watching, and plugin metadata recur together. |
| University and teaching C++ frameworks | Medium/strong | They expose the Windows student machine versus Linux CI/grader problem, but LD2026 helps only when assignments use OS facilities. |
| Scientific and engineering desktop applications | Medium/strong | Mature applications reveal real settings, paths, migration, process, and plugin behavior. |
| Audio/video plugin applications | Medium/strong | Plugin search paths, installation roots, settings, and standalone-versus-plugin modes are good future pressure tests. |
| Hardware companion/configuration tools | Medium/strong | The portable user-space support layer often has settings, paths, services, logs, IPC, and process needs. |
| User-space device services and daemons | Medium | Useful requirements sources once process and IPC modules mature. |
| Codec cores | Weak | The computational core is usually already portable; surrounding tools are more relevant than codecs themselves. |
| Kernel drivers | Poor direct target | Windows and Linux kernel APIs are different product domains; only user-space companion infrastructure is in scope. |

## Candidate Targets

These repositories are useful as source-audit targets, FlavorTest candidates,
or maintained proof branches. Promotion to active work still needs the evidence
gate in [Research backlog](../research-backlog.md).

| Candidate | Useful pressure |
| --- | --- |
| Notepad++ | Flagship Windows-heavy proof case for settings, paths, watcher, process/shell, dynamic loading, plugin, IPC, and desktop boundaries. |
| WinSCP | Deeper Windows shell, process, drag/drop, registry, VCL, and session-integration stress test. |
| Win32++ | Requirements/reference target for Win32-centric application patterns rather than a likely first adopter. |
| Carla | Plugin path discovery, dynamic loading, process isolation, configuration, filesystem watching, and bridge behavior. |
| sample-cpp-plugin | Small future proof case for `ld_dynlib` and `ld_plugin` call shape. |
| JUCE | Reference for what LD2026 should not casually grow into: a broad application/audio framework. |
| Plugify | Reference for plugin architecture, ABI boundaries, and CMake integration. |
| dylib | Existing-tool reference before any `ld_dynlib` implementation decision. |
| GTR Framework | Education-oriented validation for Windows/Linux development workflows around graphics teaching code. |
| UPB Graphics gfx-framework | Useful comparison against a project that already handles cross-platform teaching workflows. |
| cppgraphics | Smaller educational integration target. |
| FreeCAD | Mature reference for settings, paths, migration, plugins/workbenches, process behavior, and recovery. |
| OpenSCAD | Mature reference for platform paths, documents/data roots, and executable/resource discovery. |
| PrusaSlicer | Strong evidence source for settings snapshots, desktop integration, processes, and single-instance behavior. |
| Project Island | Future hot-reload pressure across `ld_paths`, `ld_watch`, `ld_dynlib`, and `ld_plugin`. |
| Halley | Game/editor reference for mature Windows/Linux tooling infrastructure. |
| LAF | Reference for behavior-oriented platform abstraction with intentionally different native implementations. |
| OpenRGB | Hardware companion boundary test: keep hardware access product-specific, abstract settings/paths/watch/desktop effects where useful. |
| Network UPS Tools | Requirements source for configuration paths, logs, daemon operation, process execution, and IPC. |
| fan-control | Small hardware-control target where config/path handling may be a clean fit. |
| OpenFAN Controller | Firmware/config/profile workflows around platform-specific USB/device access. |
| CHISP-Flasher | Firmware utility pressure around project paths, logs, helper processes, and established USB libraries. |

## Boundary Guidance

LinuxDesktop2026 should avoid competing with:

- kernel driver frameworks,
- USB/HID libraries,
- GUI toolkits,
- codec implementations,
- full application frameworks such as Qt, wxWidgets, JUCE, SDL, and engine
  frameworks.

The interesting territory is the small composable layer above those systems:

```text
application or user-space companion service
  |
  +-- settings/config
  +-- standard paths and logs
  +-- file watching
  +-- process/shell execution
  +-- IPC and activation forwarding
  +-- dynamic library and plugin discovery
  +-- desktop integration effects
```

## Demonstration Tracks

The project should prefer demonstrations that expose different failure modes:

| Track | Candidate shape | Goal |
| --- | --- | --- |
| Legacy desktop portability | Notepad++ or another Windows-heavy application | Replace individual Win32 infrastructure dependencies incrementally. |
| Education | GTR Framework or a purpose-built university challenge | Keep one source tree working on a Windows student machine and Linux build/CI machine. |
| Plugins and hot reload | `sample-cpp-plugin` first, Project Island later | Prove cooperation between paths, watching, dynamic loading, and plugin APIs. |
| Hardware companion software | OpenRGB or a smaller firmware/device utility | Show usefulness around hardware without claiming kernel-driver portability. |

## Challenge Program Idea

LinuxDesktop2026 modules map naturally to systems-programming assignments. A
challenge ecosystem could support education, contributor onboarding, API
validation, regression tests, implementation bounties, and portfolio projects.

Possible challenge sequence:

1. Paths and platform conventions.
2. Settings and portable configuration.
3. Native file watching.
4. Processes and output capture.
5. DLL/SO dynamic loading.
6. Stable plugin ABI.
7. Hot-reloading plugin host.
8. Hardware companion utility.
9. Port a real Windows component.
10. Windows developer to Linux CI/grader.

Each challenge should require one common public API, Windows and Linux
implementations, CMake, tests, MSVC/GCC/Clang CI, documented semantic
differences, and no platform preprocessor logic in consumer/example code.

The challenge program should stay separate from the public module roadmap until
individual challenge results produce reusable implementation or API evidence.
