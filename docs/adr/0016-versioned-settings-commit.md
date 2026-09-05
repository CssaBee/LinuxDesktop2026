# Accept a narrow versioned settings commit contract

`ld_settings` should grow a pre-1.0 C++ versioned settings commit API for
high-value whole-file settings writes. The first contract is stale-write
detection only: commit a new file image if the target file still matches the
state the caller read, or reject the commit with diagnostics and leave the file
unchanged.

## Decision

Add a narrow **versioned settings commit** surface before 1.0 for files whose
loss or mismatch has high user impact. The API should combine:

- an opaque file-version token captured from a successful read or from an
  explicit "file absent" state,
- an internal per-target advisory commit guard used only while comparing the
  expected token and replacing the file,
- existing backup, atomic replacement, opt-in durable flushing, and
  validation-after-write behavior,
- a stale-commit diagnostic when the current target state does not match the
  expected token.

The token is opaque. Callers should not compare paths, timestamps, inode values,
or hashes themselves. The implementation may use metadata as a fast path, but
the correctness identity for ordinary settings files should include the file
bytes so coarse timestamps or same-size edits do not silently pass as current.

The advisory guard is part of the versioned commit implementation, not a
general locking API. It serializes participating LinuxDesktop2026 versioned
commits so the compare and replace steps are one protected operation. It does
not promise protection against unrelated writers that ignore the protocol,
network filesystems with broken lock semantics, or application-level merge
conflicts.

Do not add application merge callbacks, automatic reread/retry, XML/INI/JSON
merge helpers, or silent overwrite behavior in this contract. Applications own
payload semantics and must decide whether a stale commit means prompt, reload,
merge, discard, or retry from a fresh model.

Keep new C ABI entry points out of scope until release-candidate status. The
first API should be C++ only, matching the current pre-1.0 phase where source
breaking changes are still allowed.

## Proof Targets

Notepad++ `session.xml` justifies stale-write rejection as the first target.
The file carries high-impact state: losing the current editor session is user
visible, and the existing proof already exercises backup recovery plus
validation-after-write. A stale session commit should fail before replacing a
newer independent session image; LinuxDesktop2026 should not attempt to merge
tab lists, cursor state, or session-specific XML.

Notepad++ `shortcuts.xml` justifies the second target because the write is
coupled to byte-level integrity bookkeeping. The proof writes shortcuts,
reloads the saved bytes, updates the shortcut HMAC source, and later persists
that value through `config.xml`. A stale shortcut commit should fail before the
library lets the application compute integrity metadata from an overwrite of a
newer shortcut file. Shortcut parsing, HMAC policy, and config update ordering
remain product-owned.

Together these targets are enough to accept stale-write detection as an
`ld_settings` concern. They are not enough to accept merge semantics or a broad
settings transaction system.

## Consequences

`write_with_backup()` and `write_common_config()` remain honest whole-file write
helpers. They should keep reporting
`settings-interprocess-lost-update-not-protected` because they do not receive an
expected file-version token.

The new versioned commit path should report stale commits distinctly, for
example with a diagnostic shaped like `settings-version-stale`, so callers can
separate validation failure, write failure, and lost-update prevention.

Implementation must add deterministic tests for at least:

- two participating writers reading the same version where the second commit is
  rejected after the first succeeds,
- `session.xml` validation-after-write with a stale expected token,
- `shortcuts.xml` stale rejection before the HMAC source is refreshed,
- missing-file token behavior,
- advisory-guard failure diagnostics where the platform can expose them
  deterministically.

If those tests show the advisory guard or byte identity is too costly or too
platform-specific for the core module, revisit this ADR before widening the
surface. Until then, the accepted promise is intentionally small: detect stale
participating whole-file commits and refuse to replace newer state.
