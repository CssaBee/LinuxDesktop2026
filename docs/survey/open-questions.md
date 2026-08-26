# Survey Open Questions

This file tracks questions that the repository survey cannot answer yet or that need human confirmation.

## Open Questions

- Which candidate repositories have the strongest explicit Linux-demand signal in issues, discussions, FAQs, or support forums?
- Which repositories require a second, deeper source pass because the first broad `rg` sweep found too many cross-cutting matches?
- Which existing abstraction libraries are healthy enough to adopt or wrap, especially Dapplo.Windows, libuv, wxWidgets components, Qt components, GLib/GIO, and XDG/portal libraries?
- Which features need separate API shapes for C++, C, and future Rust bindings?
- Which UI-adjacent features should be toolkit adapters rather than toolkit-neutral libraries?
- Which discovered ports or rewrites are credible enough to become reference implementations rather than just survey notes?
- Which existing libraries should we adopt, wrap, document, or explicitly reject for each candidate module?

## Answered or Partly Answered

- **Which repositories can be audited with shallow clones?** The first 10 selected repositories cloned successfully into `/tmp/linuxdesktop2026-survey` with shallow partial clones.
- **Which source-level searches should be standardized?** Initial repeatable patterns now live in `docs/survey/source-search-patterns.md`.
- **Which reference implementations should be audited first?** `libuv` and `wxWidgets` were audited first because they directly cover first-candidate modules and UI-adjacent native abstractions.
- **Which repos look like boundary cases?** Rufus is the clearest boundary/negative case so far because device/volume APIs are central rather than incidental.
- **Should scoring wait for ecosystem verification?** Yes. Source usage shows requirements; ecosystem audit decides whether we build, wrap, document, or defer.
- **Which Notepad++ subsystem should become the first proof case?** Partly answered: start with settings/config and standard paths because it is narrow, recurring across surveyed apps, and can produce a tiny working sample before UI-heavy work.
- **What did the first Notepad++ subsystem pass find?** Settings/config is not a scalar settings problem. It is a settings root resolver plus config bundle manager with ordered save phases, plugin-facing roots, hydration, backup/restore, and validation-after-write requirements.
- **Which existing tools should be adopted, wrapped, studied, or rejected for the first module?** Partly answered for settings/config: adopt XDG Base Directory, Microsoft Known Folders, and `std::filesystem`; wrap or recommend Boost.Nowide if needed; recommend or adapt Qt, GLib, and wxWidgets; defer desktop/shell specs and portals.
- **Which module should provide the first tiny working code sample?** Settings/config and standard paths. ADR 0008 selects it, and `ld_settings_demo` is now the first executable sample.
- **Which candidate modules deserve focused follow-up after the first scoring pass?** File watcher and process/shell are the strongest next first-candidate follow-ups. Dynamic library loading and single-instance IPC remain first-wave candidates but should wait until the next evidence pass is chosen.
- **What did the file watcher focused pass find?** File watching is a real reusable seam, but the useful module is not a thin inotify wrapper. It should expose event mapping, debounce/stabilization, overflow/rescan diagnostics, recursive-policy honesty, and app-owned routing. Next step after `ld_settings` publication readiness: ADR/API sketch for `ld_watch`.
- **What is left before `ld_settings` is publication-ready?** Partly answered: CMake consumption and atomic temp-write/replace are now covered; next, verify the Windows backend, keep the public API small enough for future Rust bindings or reimplementation, define the formal API stability promise, and keep Notepad++ fork changes deferred until the standalone library shape is stable.
