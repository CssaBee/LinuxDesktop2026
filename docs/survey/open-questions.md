# Survey Open Questions

This file tracks questions that the repository survey cannot answer yet or that need human confirmation.

## Open Questions

- Which existing tools should be adopted, wrapped, studied, or rejected for each candidate module?
- Which candidate modules deserve a focused follow-up search after scoring?
- Which module should provide the first tiny working code sample?
- Which Notepad++ subsystem should become the first proof case?
- Which candidate repositories have the strongest explicit Linux-demand signal in issues, discussions, FAQs, or support forums?
- Which repositories require a second, deeper source pass because the first broad `rg` sweep found too many cross-cutting matches?
- Which existing abstraction libraries are healthy enough to adopt or wrap, especially Dapplo.Windows, libuv, wxWidgets components, Qt components, GLib/GIO, and XDG/portal libraries?
- Which features need separate API shapes for C++, C, and future Rust bindings?
- Which UI-adjacent features should be toolkit adapters rather than toolkit-neutral libraries?

## Answered or Partly Answered

- **Which repositories can be audited with shallow clones?** The first 10 selected repositories cloned successfully into `/tmp/linuxdesktop2026-survey` with shallow partial clones.
- **Which source-level searches should be standardized?** Initial repeatable patterns now live in `docs/survey/source-search-patterns.md`.
- **Which reference implementations should be audited first?** `libuv` and `wxWidgets` were audited first because they directly cover first-candidate modules and UI-adjacent native abstractions.
- **Which repos look like boundary cases?** Rufus is the clearest boundary/negative case so far because device/volume APIs are central rather than incidental.
