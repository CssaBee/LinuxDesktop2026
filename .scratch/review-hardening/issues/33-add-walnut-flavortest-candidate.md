# 33 - Add Walnut FlavorTest

**What to build:** Add StudioCherno/Walnut as a real FlavorTest product under
`docs/FlavorTests/walnut/`, using source-anchored Walnut-shaped classes and
tests instead of leaving it as a candidate note.

**Blocked by:** None.

**Status:** proposed

- [ ] `docs/FlavorTests/walnut/src/walnut_flavor.hpp` and
  `docs/FlavorTests/walnut/src/walnut_flavor.cpp` exist.
- [ ] `docs/FlavorTests/walnut/test/walnut_flavor_tests.cpp` exists and is
  wired through `docs/FlavorTests/CMakeLists.txt` with
  `add_flavor_product(walnut)`.
- [ ] `docs/FlavorTests/README.md` lists Walnut as a covered FlavorTest.
- [ ] `docs/FlavorTests/SOURCES.md` records the Walnut upstream source anchors
  and the downstream/non-typical usage anchors considered during selection.
- [ ] `docs/FlavorTests/API_FRICTION.md` records whether Walnut needs a
  narrower diagnostics/root helper or whether the existing `ld_paths` shape is
  enough.
- [ ] The slice tests more than one production-shaped seam. It may add multiple
  Walnut-facing classes/functions where that keeps the adapter honest.

## Decision

Do not evaluate Walnut as a candidate. Add it to FlavorTests, like FreeCAD and
qBittorrent, but keep the first pass focused on platform bootstrap, diagnostics,
resource placement, and lifecycle behavior. Do not try to build or run Vulkan,
GLFW, Dear ImGui, or Premake inside the harness.

Walnut is a useful FlavorTest because it is not settings-heavy. It pressures a
different application shape: a small Windows-first C++ framework that wraps a
desktop render loop, has a client-owned `CreateApplication()` entry point,
stores platform capability failures as console output/early returns today, and
loads runtime image assets from caller-provided paths.

## Source Anchors

Use StudioCherno/Walnut `master` at commit `3b8e414fdecf` as the primary source
snapshot. Source links should be recorded in `SOURCES.md`; do not paste
upstream code into FlavorTests.

- `Walnut/src/Walnut/Application.h`
  - `Walnut::ApplicationSpecification`
  - `Walnut::Application`
  - `Walnut::Application::Run`
  - `Walnut::Application::Close`
  - `Walnut::CreateApplication(int argc, char** argv)`
- `Walnut/src/Walnut/Application.cpp`
  - `Walnut::Application::Init`
  - `Walnut::Application::Shutdown`
  - `check_vk_result`
  - `glfw_error_callback`
  - file-local `SetupVulkan`
  - file-local `SetupVulkanWindow`
  - `Walnut::Application::GetCommandBuffer`
  - `Walnut::Application::FlushCommandBuffer`
  - `Walnut::Application::SubmitResourceFree`
- `Walnut/src/Walnut/EntryPoint.h`
  - Windows-only `WL_PLATFORM_WINDOWS` entry point guard
  - `Walnut::Main`
  - `g_ApplicationRunning`
  - `main` versus `WinMain` behavior under `WL_DIST`
- `Walnut/src/Walnut/Image.cpp`
  - `Walnut::Image::Image(std::string_view path)`
  - `Walnut::Image::SetData`
  - `Walnut::Image::Resize`
  - `Walnut::Image::Release`
- `WalnutExternal.lua`, `Walnut/premake5.lua`, and `WalnutApp/premake5.lua`
  - `VULKAN_SDK` environment use
  - Windows-only `WL_PLATFORM_WINDOWS`
  - `ConsoleApp` versus `WindowedApp` distribution mode

## Non-Typical Usage Search Notes

Also record these as supporting anchors, not as the primary implementation
source:

- `StudioCherno/WalnutAppTemplate` keeps Walnut as a submodule and expects apps
  to customize the app project name and `WalnutApp/src/WalnutApp.cpp`.
- `TheCherno/RayTracing` is a real Walnut downstream app that uses
  `CreateApplication`, `ApplicationSpecification`, `PushLayer`,
  `SetMenubarCallback`, renderer resize, and a `Walnut::Image` descriptor in an
  ImGui viewport.
- `TheCherno/Walnut-Chat` is generated from the Walnut app template and has a
  Linux-tested headless server path. That is not a first-pass Walnut framework
  seam, but it is evidence that Walnut-shaped projects may need CLI/headless
  bootstrap and data/root diagnostics later.
- Existing forks with CMake/Linux experiments are useful review context only.
  Do not base the FlavorTest on fork-specific behavior unless a later issue
  explicitly asks for a cross-port review.

## Implementation Shape

Create a small Walnut-facing adapter that models the source control flow without
pulling in GUI/rendering libraries:

- `ApplicationSpecification`
  - fields: `name`, `width`, `height`, plus optional LinuxDesktop2026-facing
    resource/config hints only if the tests prove they are needed.
- `RuntimeEnvironment`
  - fields: executable directory, current working directory, optional home
    directory, environment map, `vulkan_sdk`, booleans for GLFW init success,
    Vulkan support, required instance extensions, WSI support, and optional GPU
    inventory.
- `LaunchOptions`
  - fields that model `argc/argv`, distribution mode, headless/test mode, and
    optional resource-root override.
- `PlatformBootstrap` or `ApplicationBootstrap`
  - method:
    `prepare(const ApplicationSpecification&, const RuntimeEnvironment&, const LaunchOptions&)`.
  - returns a Walnut-shaped `BootstrapPlan` rather than raw `ld_*` reports.
- `BootstrapPlan`
  - fields: window title/size, executable root, resource root, config root if
    used, Vulkan SDK discovery result, required extensions, selected GPU policy,
    fatal/nonfatal diagnostics, and whether startup should continue.
- `ResourceLocator`
  - method:
    `resolveImagePath(std::string_view path_or_name, const BootstrapPlan&)`.
  - keeps `Walnut::Image::Image(std::string_view path)` semantics visible while
    letting `ld_paths` resolve app-relative resources on Linux.
- `ApplicationLifecycle`
  - methods: `requestClose()`, `shutdown()`, and optionally
    `shouldRestartFromMainLoop()`.
  - models `Close`, `Shutdown`, `g_ApplicationRunning`, and the `Walnut::Main`
    loop without owning rendering.

The adapter should call LinuxDesktop2026 only at platform seams:

- use `ld_paths::resolve_app_paths` or the current root resolver for executable,
  resource, config, and optional cache/state roots;
- translate any root/candidate report into Walnut terms before returning from a
  product-shaped method;
- represent Vulkan/GLFW capability checks as Walnut bootstrap diagnostics, not
  as generic LinuxDesktop2026 diagnostics exposed to tests;
- keep renderer choices, ImGui dockspace behavior, layer update order, and
  image pixel/upload policy in Walnut vocabulary.

## Required Tests

Add focused tests that prove the flavor pressures the intended APIs:

- default bootstrap resolves executable-adjacent resource roots for a normal
  Walnut desktop app and keeps the Walnut window name/size intact;
- `VULKAN_SDK` from the environment is recorded as a capability input without
  making build-system portability the point of the slice;
- missing GLFW initialization, missing Vulkan support, missing required instance
  extensions, and missing WSI support become Walnut-shaped startup diagnostics
  and stop startup cleanly;
- when multiple GPUs are present, the selected GPU policy prefers a discrete GPU
  and otherwise falls back to the first available GPU, matching the source
  behavior without testing Vulkan directly;
- resource image lookup accepts an absolute path unchanged and resolves a
  relative image through the selected resource root;
- application close/shutdown flips the lifecycle state so the entry-point loop
  would not recreate the app after a normal close;
- distribution mode records whether the app would use `main` or `WinMain`
  semantics, but the harness does not compile Windows entry point code.

Optional tests, if the adapter remains small:

- a headless/test launch option skips window/Vulkan startup but still resolves
  roots and records why rendering was not started;
- resource-root override wins over executable-adjacent default roots;
- image path diagnostics distinguish missing asset, unsupported resource root,
  and successful absolute path.

## Out Of Scope

- Building Walnut, Premake projects, GLFW, Vulkan, Dear ImGui, or downstream
  Walnut apps.
- Porting Walnut to Linux.
- Rewriting `EntryPoint.h` ODR/header behavior.
- Implementing plugin or dynamic-library loading. Walnut itself does not expose
  a small first-party plugin loader seam in the primary source snapshot.
- Refactoring renderer/frame-loop internals such as swapchain rebuild,
  descriptor pools, command buffers, or font upload. Those are Vulkan adapter
  mechanics, not LinuxDesktop2026 platform placement seams.

## Acceptance

`ctest --test-dir build-flavor-tests --output-on-failure` includes
`walnut_flavor_tests`, and the new Walnut files show that LinuxDesktop2026 can
support a lightweight graphics-app bootstrap without forcing that app to expose
raw `ld_*` vocabulary at its product boundary.
