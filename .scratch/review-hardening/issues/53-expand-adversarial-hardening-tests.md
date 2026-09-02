# 53 - Expand Adversarial Hardening Tests

**What to build:** broaden adversarial tests beyond the completed parser and
filesystem pass so active modules face hostile caller inputs, malformed platform
state, and refactor-sensitive edge cases.

**Blocked by:** 52 - Adopt Invasive Hardening Test Posture.

**Status:** proposed

- [ ] Add adversarial `ld_settings` cases for malformed layer inputs,
  conflicting defaults, interrupted write sequences, and backup or validation
  failures around the public write facade.
- [ ] Add adversarial `ld_root` and `ld_paths` cases for hostile environment
  variables, unusual Unicode or whitespace path segments where supported by the
  platform lane, file-as-directory collisions, symlink-sensitive topology, and
  unavailable platform defaults.
- [ ] Add adversarial `ld_desktop` cases for invalid identifiers, quoting and
  escaping hazards, pre-existing generated files, denied writes, and malformed
  policy/defaults directories.
- [ ] Add adversarial `ld_migration` cases for partial copy/move failure,
  destination collisions, rollback-report fidelity, malformed Registry
  snapshots, and dry-run/apply divergence.
- [ ] Assert diagnostics for every rejected or degraded operation so failures
  are product-translatable instead of silent.

## Implementation Notes

The adversary model is accidental caller misuse, malformed environment or
filesystem state, and future refactors that silently narrow behavior. Malicious
security attacks can motivate individual cases, but they remain separate from a
full security-test program unless a module exposes a parser or path boundary
where that distinction would be artificial.
