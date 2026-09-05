# 90 - Implement Versioned Settings Commit API

**What to build:** Implement ADR 0016's narrow C++ versioned settings commit
contract for high-value whole-file settings writes.

**Blocked by:** `89` - Design Versioned Settings Commit Contract.

**Status:** implemented

- [x] Add an opaque file-version token captured from a settings-file read or an
  explicit missing-file state.
- [x] Add a versioned commit entry point that compares the expected token under
  an internal per-target advisory commit guard before using the existing
  backup/replacement/validation write path.
- [x] Report stale commits distinctly, leave the target untouched, and keep
  `write_with_backup()` / `write_common_config()` warning that they are not
  lost-update-safe.
- [x] Keep app merge callbacks, automatic reread/retry, payload-specific merge
  helpers, and new C ABI entry points out of scope.
- [x] Add deterministic tests for two participating writers, missing-file
  tokens, `session.xml` stale rejection with validation-after-write, and
  `shortcuts.xml` stale rejection before HMAC-source refresh.

## Design Anchor

ADR 0016 accepts the contract because Notepad++ `session.xml` and
`shortcuts.xml` proved stale-write detection is valuable for high-impact
settings files while payload merge semantics remain product-owned.

## Implementation Notes

- Added the C++-only `file_version_token`, `read_file_version()`,
  `missing_file_version()`, and `write_versioned()` surface in
  `ld_settings`.
- `write_versioned()` rejects invalid tokens, target mismatches, and stale
  targets before calling the existing durable backup/replacement/validation
  writer. Stale targets report `settings-version-stale`.
- The commit path uses a process-local mutex plus per-target sidecar advisory
  lock while comparing the expected bytes and replacing the file.
- No app merge callbacks, automatic retry/reread behavior, payload-specific
  merge helpers, or C ABI entry points were added.
- Deterministic settings tests cover two participating writers, missing-file
  token reuse, `session.xml` stale rejection before validation-after-write, and
  `shortcuts.xml` stale rejection before HMAC-source refresh.
- The maintained Notepad++ crossport proof now uses the versioned path for
  `session.xml` and `shortcuts.xml` stale-write proof scenarios.

## Validation

- `cmake --build build --target ld_settings_tests`
- `./build/ld_settings_tests`
- `cmake --install build --prefix /tmp/linuxdesktop2026-task90-prefix`
- `cmake -S . -B build/task90-proof -DLinuxDesktop2026_DIR=/tmp/linuxdesktop2026-task90-prefix/lib/cmake/LinuxDesktop2026`
  in `../LinuxDesktop2026-crossport-notepadpp`
- `cmake --build build/task90-proof --target linuxdesktop2026_notepadpp_settings_proof`
  in `../LinuxDesktop2026-crossport-notepadpp`
- `ctest --test-dir build/task90-proof --output-on-failure` in
  `../LinuxDesktop2026-crossport-notepadpp`
