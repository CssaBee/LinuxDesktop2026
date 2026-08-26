# ld_settings Windows Verification

This checklist verifies the Windows-shaped backend on real Windows rather than inferring behavior from Linux.

## Scope

Verify the first `ld_settings` slice on Windows:

- CMake configure and build with MSVC.
- `ld_settings_tests` runs successfully.
- `ld_settings_install_tree_consumer` installs the package and consumes `LinuxDesktop2026::ld_settings` from a separate CMake project.
- Known Folder lookup returns non-empty config/data/state/cache roots.
- `write_with_backup` uses temp-write/replace through the Windows backend.
- Atomic validation failures leave the previous target untouched.

## Automated Path

GitHub Actions runs the same CMake test suite on `windows-latest` and `ubuntu-latest`:

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

The Windows job is the source of truth for:

- `SHGetKnownFolderPath` compilation and linking,
- `MoveFileExW` compilation and replace behavior,
- exported package metadata under MSVC,
- and the install-tree consumer test on Windows.

## Local Windows Path

From a Windows developer shell with CMake and Ninja available:

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

## Current Limits

- Windows containers are not a substitute on this Linux host; this environment does not currently have Docker, Wine, or a MinGW cross compiler installed.
- The first verification target is correctness of the shaped backend, not installer behavior, UAC prompts, roaming profile policy, or OneDrive/cloud provider integration.
- Privileged install roots are app policy inputs. `ld_settings` can deny portable mode under declared roots, but the app still decides which install roots count as privileged.
