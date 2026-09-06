# 107 - Add Minifox Portable Launcher FlavorTest Probe

**What to build:** Add EarsT913831CALS/minifox-comfyui-launcher as a
review-hardening FlavorTest probe under `docs/FlavorTests/minifox/`, focused on
portable executable-relative roots, launch profiles, cache placement,
subprocess/tool candidates, and GPU-runtime environment diagnostics.

**Blocked by:** 105 - Add Amiberry Root Topology FlavorTest Probe.

**Status:** implemented

- [x] Add `docs/FlavorTests/minifox/src/minifox_flavor.hpp` and
  `docs/FlavorTests/minifox/src/minifox_flavor.cpp`.
- [x] Add `docs/FlavorTests/minifox/test/minifox_flavor_tests.cpp` and wire it
  through `docs/FlavorTests/CMakeLists.txt` with
  `add_flavor_product(minifox)`.
- [x] Update `docs/FlavorTests/README.md` with the covered Minifox slice.
- [x] Update `docs/FlavorTests/SOURCES.md` with source anchors and README
  evidence.
- [x] Update `docs/FlavorTests/API_FRICTION.md` with any portable-root,
  settings, executable-candidate, or future process-boundary friction.

## Source Anchors

Use EarsT913831CALS/minifox-comfyui-launcher as the source anchor. Record exact
source snapshots in `SOURCES.md` before implementation.

- README evidence:
  `https://github.com/EarsT913831CALS/minifox-comfyui-launcher/blob/dev/README.md`
- Portable layout: launcher executable placed beside `ComfyUI/` and `python/`
  inside an existing portable package.
- State policy: no Registry use, no ComfyUI file modification, cache
  directories beside the executable.
- Launch behavior: multiple launch configurations, process launch/stop/monitor,
  and live console output capture.
- Runtime environment discovery: CUDA, ROCm, HIP SDK, and ZLUDA behavior,
  including bundled runtime components and temporary replacement/restoration
  semantics.

## Implementation Shape

Create a small Minifox-facing adapter that models launcher planning without
starting Python, modifying ComfyUI, or touching GPU runtimes:

- `PortablePackageEnvironment`: executable directory, detected `ComfyUI/`,
  detected `python/`, environment variables, existing cache/runtime folders,
  and manually selected profile paths.
- `LaunchProfile`: product-owned ComfyUI path, Python path, launch arguments,
  environment strategy, and cache policy.
- `LaunchPlan`: Minifox vocabulary for selected executable/tool candidates,
  working directory, cache roots, profile settings path, process environment
  additions, runtime diagnostics, and reversible runtime actions.
- `RuntimeDetector`: models CUDA, ROCm, HIP SDK, and ZLUDA eligibility as
  product diagnostics while using LinuxDesktop2026 only for path and candidate
  reporting.

LinuxDesktop2026 should provide portable-root, settings-root, cache-root, and
external-tool candidate reporting. Minifox should continue to own ComfyUI
layout policy, launch arguments, subprocess lifecycle, console parsing, GPU
compatibility, ZLUDA replacement/restoration, and extension management.

## Required Tests

- Default portable layout discovers executable-adjacent `ComfyUI/`, `python/`,
  cache roots, and profile settings without Registry or user-profile writes.
- Manual Python and ComfyUI paths override auto-detected portable candidates
  and report their source.
- Missing `ComfyUI/main.py` or missing Python executable produces
  Minifox-shaped launch diagnostics without creating unrelated roots.
- Multiple launch profiles keep separate settings while sharing the same
  portable cache policy where the product requires it.
- CUDA, ROCm, HIP SDK, and ZLUDA eligibility are recorded as runtime inputs,
  not as LinuxDesktop2026 capability claims.
- Planned ZLUDA file replacement/restoration is represented as product-owned
  reversible runtime work; LinuxDesktop2026 does not become a runtime patcher.
- Future process abstractions are noted only as friction evidence; the first
  FlavorTest must not require an `ld_process` module to exist.

## Out Of Scope

- Building or porting Minifox.
- Launching ComfyUI, Python, extension updates, package downloads, or GPU
  runtime mutation.
- Implementing process supervision, live console capture, ZLUDA patching, or
  CUDA/ROCm compatibility logic in LinuxDesktop2026.
- Treating this as maintained consumer proof.

## Evidence Fit

FlavorTests and source review should catch this: the risk is letting a portable
launcher's executable-relative cache/settings/tool discovery pressure
LinuxDesktop2026 into owning subprocess, AI-tool, or GPU-runtime policy that
belongs to the product.

## Release Gate

Does not block `0.2.0`.

## Implemented Test Slice

- Default portable layout discovers executable-adjacent `ComfyUI/`, `python/`,
  `.minifox/`, and `.cache/` paths without Registry or user-profile writes.
- Manual Python and ComfyUI paths override auto-detected portable candidates
  and report their source.
- Missing `ComfyUI/main.py` and Python executable produce Minifox-shaped launch
  diagnostics without creating data/cache roots.
- Multiple launch profiles keep separate settings while sharing portable cache
  policy.
- CUDA, ROCm, HIP SDK, and ZLUDA eligibility are runtime inputs; ZLUDA
  replacement/restoration is represented as product-owned reversible runtime
  work.
