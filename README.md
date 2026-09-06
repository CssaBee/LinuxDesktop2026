# LinuxDesktop2026

LinuxDesktop2026 is a set of small C++17 libraries for Windows-heavy desktop
applications that want native Linux support without scattering platform
conditionals through product code.

The project is not a desktop environment, GUI toolkit, Wine layer, or Notepad++
fork. Notepad++ is the first proof case because it has real Windows-shaped
settings, paths, and compatibility pressure. The libraries are meant to be
general-purpose and permissively licensed.

## Maturity

LinuxDesktop2026 `0.2.0` is a public prototype release. The repository has
working modules, tests, examples, install-tree consumption checks, FlavorTests,
and CI portability lanes, but none of the modules should be treated as
production-stable yet.

The `0.2.0` support line:

- C++17 public headers live under `include/linuxdesktop/`.
- Supported phase-one platforms are Windows 10/11 and Ubuntu LTS.
- Other XDG-like Linux distributions are best-effort.
- There is no phase-one macOS support promise.
- C++ APIs may break in later `0.x` releases when source audits, proof
  integrations, or module-boundary corrections show that the current shape is
  wrong.
- Existing C ABI entry points are maintained where practical. New C ABI
  expansion waits until release-candidate status.
- Filesystem, desktop, policy, and migration mutation is explicit; preview or
  dry-run behavior is preferred where practical.
- Path, root, and settings resolution are non-mutating by default.

See [API and ABI stability](docs/plan/api-stability.md) for the full policy.

## Modules

| Module | Target | Status | Responsibility |
| --- | --- | --- | --- |
| `ld_core` | `LinuxDesktop2026::ld_core` | Active | Shared diagnostic vocabulary and version-adjacent core types. |
| `ld_settings` | `LinuxDesktop2026::ld_settings` | Supported prototype | Settings/config roots, config-default hydration, validated writes, backup behavior, config layers, versioned whole-file commits, and settings diagnostics. |
| `ld_paths` | `LinuxDesktop2026::ld_paths` | Supported prototype | Application path resolution, standard user paths, executable/resource roots, candidate reports, path lists, plugin search roots, and opt-in directory creation. |
| `ld_root` | `LinuxDesktop2026::ld_root` | Supported prototype | App/user root topology, portable-root policy, named roots, component roots, request builders, and non-mutating root reports. |
| `ld_watch` | `LinuxDesktop2026::ld_watch` | Supported prototype | File watching with native Linux and Windows backends, optional libuv, bounded pull delivery, deadline-scheduled settled-file coalescing by path, and recursive-watch honesty. |
| `ld_desktop` | `LinuxDesktop2026::ld_desktop` | Experimental extraction | Desktop/session integration effects such as autostart and managed/enforced policy, with broader desktop registration still experimental. |
| `ld_migration` | `LinuxDesktop2026::ld_migration` | Experimental extraction | Dry-run-first application-settings migration for regular files/directories and app-settings Registry snapshot/import/export compatibility. |

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

Formatting:

```sh
clang-format -i $(git ls-files '*.c' '*.cpp' '*.h' '*.hpp')
```

The repository has a `.clang-format` baseline for new changes, but CI does not
enforce formatting yet. Do not run a broad mechanical reformat in feature or
hardening patches unless that cleanup is the whole change being reviewed.

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

`ld_paths::resolve_app_paths`, `ld_root::resolve_app_roots`, and
`ld_settings::resolve_settings_roots` are preview-only by default. Callers that
want directory creation must explicitly opt in through `ensure_directory()` or
the relevant `create_directories` option.

## Consume From CMake

From a Git checkout:

```cmake
include(FetchContent)

FetchContent_Declare(
    LinuxDesktop2026
    GIT_REPOSITORY https://github.com/CssaBee/LinuxDesktop2026.git
    GIT_TAG 0.2.0
)
FetchContent_MakeAvailable(LinuxDesktop2026)

target_link_libraries(your_app PRIVATE LinuxDesktop2026::ld_settings)
```

Use a release tag or commit SHA for normal dependency builds so updates are
deliberate and reproducible. Tracking `main` is appropriate only for active
LinuxDesktop2026 development, proof branches, or other integration work that is
prepared to absorb pre-1.0 breaking changes immediately.

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

## Validation

The project uses these evidence layers:

- Unit and smoke tests for module behavior, C ABI ownership, install-tree
  consumption, watcher backends, and Windows/Linux path behavior.
- [FlavorTests](docs/FlavorTests/README.md), which refactor real upstream-shaped
  seams from projects such as Notepad++, PrusaSlicer, OpenRGB, KeePassXC,
  qBittorrent, OBS, KiCad, Audacity, FreeCAD, Walnut, OpenIPC Dashboard,
  Gearcoleco, CtrlrX, SmartServoFramework, KickCAT, Amiberry, Endless Sky, and
  Minifox.
- Maintained consumer proof branches, summarized in
  [project status](docs/project-status.md) with exact branch and CI evidence in
  the [Notepad++ proof page](docs/consumer-branches/notepadpp-settings-proof.md).

The CI matrix covers Ubuntu, Fedora, Windows/MSVC, shared-library builds on
Linux, sanitizer lanes, FlavorTests, optional libuv watcher coverage, and a
manual Notepad++ proof-branch workflow. See
[CI portability evidence](docs/ci-portability-evidence.md).

## Documentation

- [Project status](docs/project-status.md)
- [Domain language](CONTEXT.md)
- [Library roadmap](docs/plan/library-roadmap.md)
- [API and ABI stability](docs/plan/api-stability.md)
- [FlavorTests](docs/FlavorTests/README.md)
- [FlavorTest API friction](docs/FlavorTests/API_FRICTION.md)
- [Cross-port reference rules](docs/FlavorTests/CROSS_PORT_REFERENCES.md)
- [Maintained consumer branches](docs/consumer-branches/README.md)
- [Migration examples](docs/examples/migration-examples.md)
- [Validation evidence](docs/validation-evidence.md)
- [Research backlog](docs/research-backlog.md)
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
