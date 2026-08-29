# ld_settings C ABI

The C ABI exists to keep Rust bindings and non-C++ consumers plausible without exposing C++ ABI details. Before release-candidate status, this document describes the existing surface; it is not a mandate to expand the ABI.

## Current Scope

The current surface wraps root resolution and the first report vocabulary:

- caller fills `ld_settings_root_options`,
- caller may call `ld_settings_root_options_init` to get defaults matching the C++ API,
- caller passes a pointer to `ld_settings_root_report`,
- `ld_settings_resolve_app_roots` returns `1` on success and `0` on failure,
- returned strings and diagnostics are owned by LinuxDesktop2026,
- caller releases all returned memory with `ld_settings_free_root_report`,
- named roots, component roots, config layers, active read order, active write layer, and portable level are exposed in the report.

The current surface also wraps first-scope mutation and migration helpers:

- config bundle hydration,
- `write_with_backup` with optional validation callback,
- migration plan creation,
- migration plan execution from C-supplied actions,
- Registry JSON snapshot serialization/parsing,
- `.reg` snapshot serialization/parsing,
- Registry tree JSON/`.reg` export/import entry points,
- report-owned UTF-8 paths/content/diagnostics,
- and one matching free function per report family.

It also wraps first-scope desktop effects as pre-1.0 compatibility shims. The
C++ implementation now lives in `ld_desktop`; C ABI renaming or expansion waits
until release-candidate status.

- autostart apply/remove/query,
- managed/enforced policy apply/remove/query,
- dry-run-first effect options,
- report-owned UTF-8 paths, values, and diagnostics,
- one matching free function per report family.

This avoids exposing:

- `std::string`,
- `std::filesystem::path`,
- `std::vector`,
- exceptions,
- templates,
- or C++ object lifetimes.

## Example

```c
#include "linuxdesktop/settings_c.h"

int main(void)
{
    struct ld_settings_root_options options = {0};
    ld_settings_root_options_init(&options);
    options.organization = "ExampleOrg";
    options.application = "ExampleApp";
    options.settings_override = "/tmp/example-settings";

    struct ld_settings_root_report report = {0};
    if (!ld_settings_resolve_app_roots(&options, &report)) {
        return 1;
    }

    /* report.config, report.state, and friends are UTF-8 paths.
       report.config_layers exposes the default layer order and backends. */

    ld_settings_free_root_report(&report);
    return 0;
}
```

## Rust Binding Shape

A future Rust crate can bind this surface with:

- `#[repr(C)]` structs matching `settings_c.h`,
- `CString` for input strings,
- `CStr::from_ptr` for output strings,
- and `Drop` wrappers that call the matching `ld_settings_free_*_report` function.

The Rust side should not free individual strings directly.

The repository includes `tests/settings_rust_ffi_smoke.rs`, a tiny conditional `rustc` smoke test. When `rustc` is available at configure time, CMake builds a Rust object that calls the C ABI and links it into a C++ test executable. When `rustc` is not available, the regular C/C++ test suite still runs and the Rust smoke test is skipped.

## Deferred

- Further C ABI expansion until release-candidate status.
- A published Rust crate.
- Ownership helpers for individual strings or diagnostics.
- Stable ABI/version negotiation.

The next useful step is real Windows verification for Registry/autostart/policy
behavior. A small safe Rust crate wrapper and any new ABI families should wait
until the ABI names and module boundaries settle near release-candidate status.
