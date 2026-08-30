# Research Backlog

This document holds parked ideas. A topic listed here is not an active delivery
promise.

## Activation Gate

A research candidate can become an active module only when all of these are
true:

- at least two source-anchored FlavorTests or maintained consumer branches show
  the same repeated seam,
- the seam is not already better owned by a toolkit, operating-system facility,
  or mature dependency,
- an existing-tool decision names whether LinuxDesktop2026 should adopt, wrap,
  recommend, or defer the obvious alternatives,
- the proposed module boundary explains what consumer policy stays outside the
  library,
- and the first API is scoped to the shared part observed in those integrations,
  not to a wishlist collected from one application.

`ld_desktop` and `ld_migration` are exceptions only because they extract
behavior that already existed in the wrong module. New surface added during
those extractions still needs source or consumer evidence.

## Active Or Required By Extraction

These areas are not backlog ideas; they are already active or required to finish
current module-boundary cleanup:

- settings/config through `ld_settings`,
- filesystem and path helpers through `ld_paths`,
- file watching through `ld_watch`,
- desktop integration effects through `ld_desktop`,
- migration planning and execution through `ld_migration`.

## Research Candidates

| Candidate | Current posture |
| --- | --- |
| Process and shell integration | Research-only until repeated consumers show the same launch, shell-command, readiness, and lifecycle boundary, and an existing-tool decision covers mature alternatives. |
| Single-instance IPC | Research-only until repeated consumers show the same lock ownership, stale recovery, local transport, and activation-forwarding boundary, with D-Bus, local sockets, named pipes, toolkit helpers, and window-message activation considered. |
| Dynamic library loading | Research-only until a source audit proves a shared loader seam that is not better handled by `dlopen`/`LoadLibrary`, Boost.DLL-style wrappers, `dylib`, or a plugin framework. |
| Service and daemon lifecycle | Parked until `ld_process` and `ld_ipc` mature enough to support it cleanly. |
| GUI and windowing | Parked. Toolkit ownership is likely stronger than a generic LinuxDesktop2026 abstraction unless evidence proves a narrow shared layer. |
| Clipboard | Parked. Most real value may belong in toolkit adapters or capability notes rather than a standalone library. |
| Drag-and-drop | Parked. Payload conventions may be reusable, but drop-target behavior is usually toolkit-owned. |
| Common dialogs and resources | Parked. File pickers, message dialogs, icons, and resources need a toolkit-aware decision before implementation. |
| Printing | Parked. It is not part of the earliest native-Linux proof path. |
| Plugin ABI | Deferred beyond phase one. Binary compatibility with existing Windows plugins is explicitly not a first proof-case goal. |
| Advanced theming and DPI | Parked until GUI toolkit strategy is evidence-backed. |
| Accessibility | Parked until GUI/windowing scope exists; do not imply screen-reader or assistive-technology coverage in current module docs. |
| Installer and package integration | Parked until module APIs and generated desktop/migration effects are closer to ship-candidate status. |

## Evidence Sources

Use these documents before promoting a research candidate:

- [Extended watchlist fit audit](survey/extended-watchlist-fit-audit.md)
- [Adoption targets and challenge ideas](survey/adoption-targets-and-challenge-ideas.md)
- [Ecosystem audit](survey/ecosystem-audit.md)
- [Source search patterns](survey/source-search-patterns.md)
- [FlavorTests](FlavorTests/README.md)
- [Cross-port reference rules](FlavorTests/CROSS_PORT_REFERENCES.md)
- [Maintained consumer branches](consumer-branches/README.md)
