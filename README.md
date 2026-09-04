# LinuxDesktop2026

LinuxDesktop2026 is a set of small C++17 libraries for Windows-heavy desktop
applications that want native Linux support without scattering platform
conditionals through product code.

The project is not a desktop environment, GUI toolkit, Wine layer, or Notepad++
fork. Notepad++ is the first proof case because it has real Windows-shaped
settings, paths, and compatibility pressure. The libraries are meant to be
general-purpose and permissively licensed.

## Maturity

LinuxDesktop2026 is pre-1.0 prototype code. The repository has working modules,
tests, examples, install-tree consumption checks, FlavorTests, and CI portability
lanes, but none of the modules should be treated as production-stable yet.

Current promises:

- C++17 public headers live under `include/linuxdesktop/`.
- Supported phase-one platforms are Windows 10/11 and Ubuntu LTS.
- Other XDG-like Linux distributions are best-effort.
- There is no phase-one macOS support promise.
- C++ APIs may break before `1.0` when source audits, proof integrations, or
  module-boundary corrections show that the current shape is wrong.
- Existing C ABI entry points are maintained where practical, but new C ABI
  expansion waits until release-candidate status.
- Filesystem, desktop, policy, and migration mutation is explicit; preview or
  dry-run behavior is preferred where practical.

See [API and ABI stability](docs/plan/api-stability.md) for the full policy.

## Modules

| Module | Target | Status | Responsibility |
| --- | --- | --- | --- |
| `ld_core` | `LinuxDesktop2026::ld_core` | Active | Shared diagnostic vocabulary and version-adjacent core types. |
| `ld_settings` | `LinuxDesktop2026::ld_settings` | Active prototype | Settings/config roots, config-default hydration, validated writes, backup behavior, config layers, and settings diagnostics. |
| `ld_paths` | `LinuxDesktop2026::ld_paths` | Active prototype | Application path resolution, standard user paths, executable/resource roots, candidate reports, path lists, plugin search roots, and opt-in directory creation. |
| `ld_watch` | `LinuxDesktop2026::ld_watch` | Active prototype | File watching with native Linux and Windows backends, optional libuv, bounded event delivery, recursive-watch honesty, and settled-file behavior. |
| `ld_desktop` | `LinuxDesktop2026::ld_desktop` | Extraction in progress | Desktop/session integration effects such as autostart and managed/enforced policy, with the broader desktop-effect scope still being completed. |
| `ld_migration` | `LinuxDesktop2026::ld_migration` | Extraction in progress | Dry-run-first application-settings migration for regular files/directories and app-settings Registry snapshot/import/export compatibility. |

`ld_settings` no longer owns desktop effects or migration behavior. New callers
should use `ld_desktop` and `ld_migration` directly for those responsibilities.

Detailed status lives in [Project status](docs/project-status.md). Long-range
ideas live in [Research backlog](docs/research-backlog.md).

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Build options:

```sh
cmake -S . -B build \
  -DLD2026_BUILD_EXAMPLES=ON \
  -DLD2026_BUILD_TESTS=ON \
  -DLD2026_WATCH_ENABLE_LIBUV=ON \
  -DLD2026_WATCH_PREFER_LIBUV=OFF \
  -DLD2026_WATCH_ENABLE_TEST_HOOKS=ON
```

`ld_watch` uses native Linux `inotify` on Linux and native
`ReadDirectoryChangesW` on Windows by default. The libuv backend is optional and
is most appropriate for applications that already own a libuv event loop and
only need coarse file-change notifications. `LD2026_WATCH_ENABLE_TEST_HOOKS`
keeps the simulated-backend watcher tests available while leaving normal
library builds free of test-only backend injection hooks.

## Examples

```sh
./build/ld_settings_demo --settings-dir /tmp/linuxdesktop2026-settings-demo
./build/ld_paths_demo --org LinuxDesktop2026 --app paths-demo
./build/ld_paths_c_demo
./build/ld_watch_demo
```

The settings demo uses temporary overrides and does not touch your normal
application configuration directory.

Minimal `ld_paths` use:

```cpp
#include "linuxdesktop/paths.hpp"

namespace ldp = linuxdesktop::paths;

int main()
{
    ldp::resolver_options options;
    options.use_process_environment = false;

#if defined(_WIN32)
    options.platform_defaults =
        ldp::platform_path_defaults::windows("C:/Users/example");
#else
    options.platform_defaults =
        ldp::platform_path_defaults::xdg("/tmp/example-home", "/tmp/example-runtime");
#endif

    auto paths = ldp::resolve_app_paths({"LinuxDesktop2026", "example"}, options);
    auto preview = ldp::ensure_directory(paths, ldp::path_family::config);
    return paths.selected.empty() || preview.diagnostics.size() > 1;
}
```

## Consume From CMake

From a Git checkout:

```cmake
include(FetchContent)

FetchContent_Declare(
    LinuxDesktop2026
    GIT_REPOSITORY https://github.com/CssaBee/LinuxDesktop2026.git
    GIT_TAG main
)
FetchContent_MakeAvailable(LinuxDesktop2026)

target_link_libraries(your_app PRIVATE LinuxDesktop2026::ld_settings)
```

From a vendored checkout:

```cmake
add_subdirectory(external/LinuxDesktop2026)
target_link_libraries(your_app PRIVATE LinuxDesktop2026::ld_paths)
```

From an installed package:

```sh
cmake -S . -B build -DLD2026_BUILD_EXAMPLES=OFF -DLD2026_BUILD_TESTS=OFF
cmake --install build --prefix /tmp/linuxdesktop2026-prefix
```

Installed CMake consumers can generate target-local platform defaults instead
of copying OS-specific helper code:

```cmake
find_package(LinuxDesktop2026 CONFIG REQUIRED)

add_executable(your_app main.cpp)
target_link_libraries(your_app PRIVATE LinuxDesktop2026::ld_paths)
linuxdesktop2026_generate_path_defaults(your_app
    HEADER your_app/generated/platform_path_defaults.hpp)
```

The generated header selects the supported XDG or Windows default factory for
the consumer target. Application code still passes the resulting defaults
through `ld_paths::resolver_options::platform_defaults`, so there is no hidden
global path policy in the shared library.

```cmake
find_package(LinuxDesktop2026 CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE LinuxDesktop2026::ld_watch)
```

## Validation Strategy

The project uses three layers of evidence:

- Unit and smoke tests for module behavior, C ABI ownership, install-tree
  consumption, watcher backends, and Windows/Linux path behavior.
- [FlavorTests](docs/FlavorTests/README.md), which refactor real upstream-shaped
  seams from projects such as Notepad++, PrusaSlicer, OpenRGB, KeePassXC,
  qBittorrent, OBS, KiCad, Audacity, FreeCAD, Walnut, and OpenIPC Dashboard.
- Maintained consumer proof branches, starting with
  [LinuxDesktop2026-crossport-notepadpp](docs/consumer-branches/notepadpp-settings-proof.md).

The CI matrix covers Ubuntu, Fedora, Windows/MSVC, shared-library builds on
Linux, sanitizer lanes, FlavorTests, optional libuv watcher coverage, and a
manual Notepad++ proof-branch workflow. See
[CI portability evidence](docs/ci-portability-evidence.md).

## Documentation

- [Project status](docs/project-status.md)
- [Research backlog](docs/research-backlog.md)
- [Domain language](CONTEXT.md)
- [Library roadmap](docs/plan/library-roadmap.md)
- [API and ABI stability](docs/plan/api-stability.md)
- [FlavorTests](docs/FlavorTests/README.md)
- [FlavorTest API friction](docs/FlavorTests/API_FRICTION.md)
- [Cross-port reference rules](docs/FlavorTests/CROSS_PORT_REFERENCES.md)
- [Maintained consumer branches](docs/consumer-branches/README.md)
- [Adoption targets and challenge ideas](docs/survey/adoption-targets-and-challenge-ideas.md)
- [Migration examples](docs/examples/migration-examples.md)
- [Architecture decisions](docs/adr)
- [Survey documents](docs/survey)

Module plans:

- [`ld_paths` roadmap](docs/plan/ld-paths-roadmap.md)
- [`ld_settings` Windows verification](docs/plan/ld-settings-windows-verification.md)
- [`ld_settings` C ABI](docs/plan/ld-settings-c-abi.md)
- [`ld_desktop` extraction requirements](docs/plan/ld-desktop-extraction.md)
- [`ld_migration` extraction requirements](docs/plan/ld-migration-extraction.md)
- [`ld_settings` expanded API inventory](docs/plan/ld-settings-expanded-api.md)
- [Notepad++ proof case plan](docs/plan/notepad-plus-plus-poc.md)

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE).
