# ld_settings C ABI

The first C ABI slice exists to keep Rust bindings and non-C++ consumers plausible without exposing C++ ABI details.

## Scope

The initial surface wraps root resolution only:

- caller fills `ld_settings_root_options`,
- caller may call `ld_settings_root_options_init` to get defaults matching the C++ API,
- caller passes a pointer to `ld_settings_root_report`,
- `ld_settings_resolve_app_roots` returns `1` on success and `0` on failure,
- returned strings and diagnostics are owned by LinuxDesktop2026,
- caller releases all returned memory with `ld_settings_free_root_report`.

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

    /* report.config, report.state, and friends are UTF-8 paths. */

    ld_settings_free_root_report(&report);
    return 0;
}
```

## Rust Binding Shape

A future Rust crate can bind this surface with:

- `#[repr(C)]` structs matching `settings_c.h`,
- `CString` for input strings,
- `CStr::from_ptr` for output strings,
- and a `Drop` wrapper that calls `ld_settings_free_root_report`.

The Rust side should not free individual strings directly.

## Deferred

- C ABI wrappers for config bundle hydration.
- C ABI wrappers for `write_with_backup`.
- A published Rust crate.
- Ownership helpers for individual strings or diagnostics.
- Stable ABI/version negotiation.

The next useful step is either a tiny Rust FFI smoke test or a versioned ABI declaration before exposing more functions.
