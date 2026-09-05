# 89 - Design Versioned Settings Commit Contract

**What to build:** Decide whether `ld_settings` should grow a pre-1.0
versioned settings commit API for high-value settings files, and if so, define
the smallest contract that detects stale whole-file writes without taking over
application-owned merge semantics.

**Blocked by:** `76` - Finish Maintained Consumer Proof Evidence, `79` -
Measure Framework Tax In Maintained Proof.

**Status:** implemented

- [x] Use Notepad++ `session.xml` as the first proof target for stale-write
  detection because losing session state has high user impact and the flavor
  already exercises backup recovery and validation-after-write.
- [x] Use Notepad++ `shortcuts.xml` as the second proof target before accepting
  the contract, so shortcut/HMAC coupling is tested before the design is
  treated as reusable.
- [x] Decide whether the public shape should be an opaque file identity token,
  app merge callback, advisory lock, product-owned behavior, or a staged
  combination of those.
- [x] If a library API is accepted, prefer stale commit rejection with
  diagnostics as the first guarantee; do not silently reread, merge, retry, or
  overwrite because applications own XML/INI/JSON semantics.
- [x] Record an ADR only after the proof targets have been checked and the
  trade-off is real enough that a future reader would wonder why this contract
  exists.
- [x] Since the evidence justifies a pre-1.0 API, update task 73 and the
  settings docs to point at the accepted narrow versioned commit contract
  instead of leaving lost-update protection as an orphaned future question.

## Design Notes

The accepted domain term is **Versioned settings commit**: a possible
high-value settings write contract where an application commits a new file image
only if the on-disk settings state still matches the state it read, so stale
whole-file writes can be detected instead of silently replacing independent
changes.

This ticket deliberately follows maintained-consumer and framework-tax evidence.
Task 73 made the current contract honest: `ld_settings` is corruption-safe by
backup/replacement/validation/durable-write options, but it is not
lost-update-safe. This ticket decides whether the next step belongs in the
library at all.

## Review Anchor

The task 73 grill-with-docs pass found that the phrase "merge/version contract"
created a hidden dependency. The queue needs an explicit future decision point
for high-value settings files rather than an orphaned implementation note.

## Implementation Notes

ADR 0016 accepts a narrow pre-1.0 C++ versioned settings commit contract.
Notepad++ `session.xml` provides the high-impact stale-write proof target:
session saves already use backup, durable write, and validation-after-write, and
startup recovery already exercises the `.bak` path. Notepad++ `shortcuts.xml`
provides the second proof target because successful writes refresh the
shortcut-file byte source used for HMAC bookkeeping before `config.xml` persists
matching integrity state.

The accepted shape is a staged combination: an opaque file-version token plus an
internal per-target advisory commit guard, followed by the existing
backup/replacement/validation path only if the current target still matches the
expected version. Stale participating commits should fail with diagnostics and
leave the target untouched. The design explicitly rejects app merge callbacks,
automatic reread/retry, XML-specific merging, and a general settings transaction
system. Ordinary `write_with_backup()` and `write_common_config()` remain
corruption-safe whole-file write helpers and keep reporting that they do not
protect read-modify-write flows from lost updates.

Task 90 tracks the implementation work and the deterministic stale-write tests.
