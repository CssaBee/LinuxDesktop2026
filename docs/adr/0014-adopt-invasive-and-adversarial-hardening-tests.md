# Adopt invasive and adversarial hardening tests

LinuxDesktop2026 is still pre-1.0, but its modules already write files,
resolve platform roots, expose C ABI ownership paths, and plan migrations.

## Decision

Invasive and adversarial tests are a required hardening lane for `ld_settings`,
`ld_root`, `ld_paths`, `ld_desktop`, and `ld_migration`.

Invasive tests may inspect generated files, directory state, cleanup behavior,
before/after lifecycle fields, nested C ABI allocation ownership, and
cross-module report propagation when those details are observable parts of the
caller contract. They should not assert private implementation layout or
incidental helper structure.

Adversarial tests should cover malformed caller input, hostile environment
values, file-as-directory collisions, partial filesystem state, denied writes,
invalid generated identifiers, parser edge cases, and permission gates. These
tests are local and deterministic; they can use temporary filesystem topology
and injected environment maps, but they should remain fast enough for the
normal hardening suite.

## Rationale

Ordinary happy-path tests can miss regressions in behavior that users depend
on: write ordering, backup restoration, temporary-file cleanup, path candidate
diagnostics, dconf/autostart file content, migration action traces, rollback
signals, and C ABI free/reset semantics.

The project is pre-1.0, so this is the right phase to let tests be more
probing. A refactor that preserves returned `ok == true` while losing a
diagnostic, leaking a temporary file, deleting a neighboring generated file, or
breaking nested C ABI ownership is still a product-facing regression.

## Consequences

The normal test suite carries some behavior-specific assertions, and module
tests can cross a module boundary when a single-module unit test would be
dishonest. For example, `ld_settings` may assert diagnostics forwarded from
`ld_paths`, `ld_desktop` may assert the exact generated desktop or dconf file
shape, and `ld_migration` may assert before/after filesystem flags rather than
only final file existence.

The added detail increases maintenance cost, but the target is public behavior
rather than private shape. When an implementation changes intentionally, the
test update should explain the new caller-visible contract instead of weakening
the hardening lane.
