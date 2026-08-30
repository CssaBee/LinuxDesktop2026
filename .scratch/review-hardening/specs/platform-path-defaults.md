# Platform Path Defaults Spec

## Problem Statement

FlavorTests currently look cleaner than real user integrations because they can
include a private `platform_paths` helper that users will not receive when they
consume LinuxDesktop2026. That helper hides the work required to inject
deterministic XDG and Windows AppData roots into the path resolver. The result
is misleading evidence: product-shaped tests pass with attractive call sites,
but installed consumers still need to know too much about operating-system
environment variables, platform-specific root names, and test-only scaffolding.

The problem is especially sharp for `ld_paths`, whose purpose is to hide
operating-system path dependency as much as practical while still reporting
resolution honestly. If the ergonomic path-default mechanism only exists inside
the FlavorTest harness, the project has not actually reduced framework tax for
application developers.

## Solution

Expose platform path defaults as supported `ld_paths` API. Application code,
tests, and generated CMake consumer glue should be able to construct a small
platform-default value object and pass it through normal resolver options. The
resolver then uses those defaults after explicit overrides, injected
environment, process environment, and OS APIs, but before built-in fallback
guesses.

The public runtime API should land first because it is the clearest supported
contract. The C ABI should mirror the same capability in the same hardening
pass. CMake should then provide consumer-target generated defaults, so app code
can avoid hand-writing OS-specific path construction without creating hidden
global resolver state or baking one app's user roots into a shared
LinuxDesktop2026 library.

After the public API exists, FlavorTests should stop including the private
helper. They should either call the runtime default object directly or rely on
the CMake-generated helper only in install-tree consumer coverage.

## User Stories

1. As an application developer, I want a public way to provide platform path defaults, so that I do not need to copy FlavorTest-only helper code.
2. As an application developer, I want LinuxDesktop2026 to hide XDG and Windows AppData details where practical, so that my application code can stay product-shaped.
3. As an application developer, I want path defaults to be explicit resolver inputs, so that a path decision can still be explained and tested.
4. As an application developer, I want explicit overrides to beat defaults, so that command-line flags and product policy remain authoritative.
5. As an application developer, I want injected environment to beat defaults, so that tests, launchers, and app-owned environment maps can model a user environment precisely.
6. As an application developer, I want the process environment to beat defaults, so that real user configuration is honored before packaged fallbacks.
7. As a Windows application developer, I want Windows roaming and local AppData defaults, so that config, data, state, and cache roots can be resolved without hard-coding Windows folder vocabulary in product code.
8. As a Linux application developer, I want XDG config, data, state, cache, and runtime defaults, so that platform placement can be provided without touching process-global environment variables.
9. As a test author, I want deterministic path defaults, so that resolver tests can avoid mutating a developer's real home directory.
10. As a FlavorTest author, I want the same default mechanism users receive, so that FlavorTest ergonomics are honest evidence.
11. As a FlavorTest reviewer, I want private path scaffolding removed, so that hidden framework tax cannot make APIs look better than they are.
12. As a C caller, I want the C resolver options to expose the same default roots, so that C consumers do not fall behind the C++ API.
13. As a maintainer, I want C ABI additions to be flat and explicit, so that ownership and lifetime rules remain straightforward.
14. As a CMake consumer, I want generated target-local defaults, so that my app can hide OS-specific root construction without changing global library behavior.
15. As a package maintainer, I want consumer-target defaults rather than library-global user roots, so that a shared `ld_paths` build does not carry one application's path policy into another application.
16. As a package maintainer, I want generated defaults to be passed explicitly, so that builds remain reproducible and resolver behavior is visible in source.
17. As a downstream integrator, I want installed package coverage for generated defaults, so that the CMake consumption path is validated, not just the in-tree build.
18. As a maintainer, I want candidate reports to identify default-derived choices, so that users can see when a selected path came from a packaged or injected default.
19. As a maintainer, I want default-derived relative paths rejected with diagnostics, so that defaults do not become a new unsafe input channel.
20. As a maintainer, I want this feature scoped to app-root resolution first, so that plugin path sets are not redesigned before repeated evidence asks for it.
21. As a future plugin-path implementer, I want the default object shape to be reusable later, so that plugin defaults can adopt the pattern without changing the resolver contract.
22. As a documentation reader, I want the path roadmap to distinguish runtime defaults, C ABI defaults, and CMake generated defaults, so that I know which layer to use.
23. As a documentation reader, I want examples to avoid private test helpers, so that published code resembles real integration code.
24. As a maintainer, I want this hardening work to respect the FlavorTest API exposure budget, so that new public vocabulary is justified by observed product-seam friction.
25. As a maintainer, I want this change treated as `ld_paths` hardening rather than broad module expansion, so that ADR 0011's scope discipline remains intact.

## Implementation Decisions

- The main implementation seam is the public path resolver in `ld_paths`.
- Add a C++ value object named `platform_path_defaults`.
- Add an optional `platform_path_defaults` member to resolver options.
- The default object is supported application API, not a FlavorTest-only convenience layer.
- First scope is app path resolution only.
- The first default fields cover XDG config, data, state, cache, runtime and Windows roaming/local AppData roots.
- Add named C++ factories for XDG and Windows defaults.
- Do not add a portable-mode factory in this pass. Portable mode remains product policy expressed through explicit overrides or product adapters.
- Resolver precedence is explicit overrides, injected environment, process environment or OS APIs, platform path defaults, then built-in fallback.
- Default-derived candidates should be visible in reports. If a new candidate source is needed, add it deliberately and document its meaning.
- Default paths must follow the resolver's absolute-path safety rules. Relative defaults are ignored with diagnostics.
- Mirror the runtime capability in the C ABI in the same hardening pass, using flat resolver-option fields rather than a nested lifetime-owning object.
- CMake support should generate consumer-target defaults, not bake app or user roots into a shared LinuxDesktop2026 library target.
- The generated CMake helper should be a small per-consumer header-style helper or equivalent target-local source artifact.
- Generated defaults are explicitly passed through resolver options. They are not consumed through hidden global state.
- FlavorTests should stop including private platform path support once the public runtime API exists.
- Install-tree consumer coverage should prove the generated CMake defaults from an installed package.
- This work does not reopen broad platform-module expansion. It hardens an existing `ld_paths` promise in response to FlavorTest API friction.

## Testing Decisions

- Test the highest existing seam first: public `ld_paths` resolver behavior.
- C++ resolver tests should prove precedence, selected paths, candidate reporting, and diagnostics for invalid default paths.
- C ABI tests should prove C callers can pass default roots, resolve selected paths, inspect reports, and free all returned memory with existing ownership rules.
- Install-tree consumer tests should prove a downstream CMake project can generate target-local path defaults through the installed package and pass them into the resolver explicitly.
- FlavorTests should prove that Walnut and OpenIPC Dashboard can use public defaults without a private helper.
- Tests should assert behavior at the resolver/product seam, not internal helper functions.
- Existing path resolver tests, C path resolver tests, install-tree consumer tests, and FlavorTests are the prior art.

## Out of Scope

- Plugin path default redesign.
- A portable-mode defaults factory.
- Hidden global resolver defaults.
- Library-global CMake defaults for app or user roots in a shared LinuxDesktop2026 build.
- macOS defaults.
- Stable binary ABI design.
- New broad platform modules beyond existing `ld_paths` hardening.
- Replacing Qt, toolkit, or product-owned path policy where a FlavorTest shows the application should keep that policy.

## Further Notes

The design came from a grilling session about `platform_paths.hpp` hiding an API
gap. The accepted direction is runtime API first, C ABI in the same pass,
consumer-target CMake generation next, then FlavorTest cleanup and documentation
evidence. The purpose is to make the attractive FlavorTest call sites available
to real users without erasing platform truth or product policy.
