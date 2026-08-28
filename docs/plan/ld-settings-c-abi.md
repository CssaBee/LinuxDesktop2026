# ld_settings C ABI

The C ABI exists to keep Rust bindings and non-C++ consumers plausible without exposing C++ ABI details.

## Current Scope

The current surface wraps root resolution and the first report vocabulary:

- caller fills `ld_settings_root_options`,
- caller may call `ld_settings_root_options_init` to get defaults matching the C++ API,
- caller passes a pointer to `ld_settings_root_report`,
- `ld_settings_resolve_app_roots` returns `1` on success and `0` on failure,
- returned strings and diagnostics are owned by LinuxDesktop2026,
- caller releases all returned memory with `ld_settings_free_root_report`,
- named roots, component roots, config layers, active read order, active write layer, and portable level are exposed in the report.

The current surface also wraps first-scope desktop effects:

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

## Deferred

- C ABI wrappers for migration plans and migration execution reports.
- C ABI wrappers for Registry snapshots/import/export.
- C ABI wrappers for config bundle hydration.
- C ABI wrappers for `write_with_backup`.
- A published Rust crate.
- Ownership helpers for individual strings or diagnostics.
- Stable ABI/version negotiation.

The next useful step is C ABI coverage for migration and Registry, followed by a tiny Rust FFI smoke test.
