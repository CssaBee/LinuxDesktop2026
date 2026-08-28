# Reduce scope until real integrations validate APIs

The repository review in August 2026 identified the main project risk as premature framework design: the code and roadmap were growing faster than real application integrations could validate the abstractions.

## Decision

Freeze new broad platform modules until `ld_paths`, a narrowed `ld_settings`, and the `ld_watch` prototype have been exercised by real consumer code.

Near-term work should prioritize:

- a small Notepad++ proof patch using the current libraries,
- a second Windows-heavy consumer with different settings/path/watch needs,
- hardening existing modules before adding new module families,
- and removing or downgrading public claims that imply production readiness before that evidence exists.

The roadmap may keep later topics as research notes, but GUI/windowing, clipboard, drag-and-drop, dialogs, printing, plugin ABI, theming, accessibility, packaging, service lifecycle, process launching, dynamic loading, and IPC are not active delivery promises.

## Required Changes

1. Treat C++ APIs as source-compatible only. Keep existing C ABI entry points compatible where practical, but postpone C ABI expansion and binary-stability design until release-candidate status.
2. Split `ld_settings` by responsibility. Keep settings root/config-bundle behavior; route general path discovery through `ld_paths`; extract desktop integration effects to the planned `ld_desktop` module; extract migration planning/execution and app-settings Registry migration compatibility to the planned `ld_migration` module.
3. Correct write-safety semantics. Distinguish atomic namespace replacement from crash-durable writes, and use secure exclusive temporary-file creation.
4. Harden `ld_watch` before expanding it. Define callback threading/reentrancy, bound internal queues, handle callback exceptions, and stress recursive watching.
5. Upgrade CI from smoke coverage to portability evidence: GCC and Clang on Linux, MSVC on Windows, Debug and Release, shared-library builds, sanitizer jobs, and older supported Ubuntu coverage where practical.
6. Add adversarial tests for filesystem and watcher behavior: permissions, full/error paths where practical, rename/remove churn, large event bursts, and callback lifecycle tests.
7. Stop adding public enums and structs until at least two real integrations exercise the same concept.
8. Keep capability reporting honest. If a capability matrix becomes the primary explanation for how to use a feature, revisit whether the common abstraction should exist.

## Rationale

The project has good instincts: survey-first documentation, capability reporting, install-tree tests, C ABI ownership rules, and explicit refusal to chase binary-compatible Windows plugins in the first proof of concept.

Those strengths do not remove the central risk. A small portability helper can become a platform framework accidentally if every surveyed Windows feature receives a public abstraction before applications force the exact boundary.

The expensive mistakes would be public vocabulary and ABI/API contracts that appear elegant in prototype code but become hard to revise after external consumers switch on enum values, persist diagnostics, or package shared libraries.

## Consequences

This decision deliberately slows visible feature expansion.

The project should look less ambitious in the short term and more credible over time. New work is accepted when it either hardens an existing promise or comes from concrete consumer pressure. Documentation-only module plans remain useful research, but they should not be described as committed delivery.

## Review Classification

Confirmed problems:

- `ld_settings` has exceeded its original narrow module boundary.
- ADR 0012 defines the required boundary correction: `ld_settings` narrows to settings/config behavior, `ld_paths` owns generic root policy, `ld_desktop` owns desktop integration effects, and `ld_migration` owns migration behavior.
- Linux atomic replacement currently does not imply crash durability.
- Temporary-file creation currently uses predictable names and a check-then-open pattern.
- `ld_watch` needs stronger callback, queue, and stress-test contracts before ship.
- C++ public value types make stable binary compatibility unrealistic.

Reasonable engineering concerns:

- Native recursive file watching may consume disproportionate maintenance effort.
- The roadmap breadth is too large for the current maturity level.
- Capability reporting can mask an abstraction that should stay platform-specific.

Speculative long-term risks:

- External consumers may force incompatible settings layer models.
- Packaging may create ABI expectations before the project is prepared.
- Maintainers may burn out maintaining multiple 70-percent-complete desktop modules.
