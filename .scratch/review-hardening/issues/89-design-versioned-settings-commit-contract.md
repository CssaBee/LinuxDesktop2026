# 89 - Design Versioned Settings Commit Contract

**What to build:** Decide whether `ld_settings` should grow a pre-1.0
versioned settings commit API for high-value settings files, and if so, define
the smallest contract that detects stale whole-file writes without taking over
application-owned merge semantics.

**Blocked by:** `76` - Finish Maintained Consumer Proof Evidence, `79` -
Measure Framework Tax In Maintained Proof.

**Status:** pending

- [ ] Use Notepad++ `session.xml` as the first proof target for stale-write
  detection because losing session state has high user impact and the flavor
  already exercises backup recovery and validation-after-write.
- [ ] Use Notepad++ `shortcuts.xml` as the second proof target before accepting
  the contract, so shortcut/HMAC coupling is tested before the design is
  treated as reusable.
- [ ] Decide whether the public shape should be an opaque file identity token,
  app merge callback, advisory lock, product-owned behavior, or a staged
  combination of those.
- [ ] If a library API is accepted, prefer stale commit rejection with
  diagnostics as the first guarantee; do not silently reread, merge, retry, or
  overwrite because applications own XML/INI/JSON semantics.
- [ ] Record an ADR only after the proof targets have been checked and the
  trade-off is real enough that a future reader would wonder why this contract
  exists.
- [ ] If the evidence does not justify a pre-1.0 API, update task 73 and the
  settings docs to keep lost-update protection product-owned until a stronger
  proof case appears.

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
