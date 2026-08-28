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
- **Should file watching become the next implementation module?** Answered for the current roadmap: yes, `ld_watch` is the next module candidate after the broader audit, with ADR 0010 defining the first public boundary before prototype work.
- **What did the first Notepad++ subsystem pass find?** Settings/config is not a scalar settings problem. It is a settings root resolver plus config bundle manager with ordered save phases, plugin-facing roots, hydration, backup/restore, and validation-after-write requirements.
- **Which existing tools should be adopted, wrapped, studied, or rejected for the first module?** Partly answered for settings/config: adopt XDG Base Directory, Microsoft Known Folders, and `std::filesystem`; wrap or recommend Boost.Nowide if needed; recommend or adapt Qt, GLib, and wxWidgets; defer desktop/shell specs and portals.
- **Which module should provide the first tiny working code sample?** Settings/config and standard paths. ADR 0008 selects it, and `ld_settings_demo` is now the first executable sample.
- **Which candidate modules deserve focused follow-up after the first scoring pass?** File watcher and process/shell are the strongest next first-candidate follow-ups. Dynamic library loading and single-instance IPC remain first-wave candidates but should wait until the next evidence pass is chosen.
- **What did the file watcher focused pass find?** File watching is a real reusable seam, but the useful module is not a thin inotify wrapper. It should expose event mapping, debounce/stabilization, settled-file readiness, overflow/rescan diagnostics, recursive-policy honesty, dirty-path refresh as a named future layer, and app-owned routing.
- **What should happen before `ld_watch` code?** Shared C++ diagnostics now live in `ld_core` and the broader watcher audit is complete; next, finalize ADR 0010 into an implementation-ready header/source/test checklist.
- **What is left before `ld_settings` is shippable?** Answered at the product level: the current code is a working first sample, not a finished module. Before ship, `ld_settings` needs named roots, component roots, portable levels, all config layers, migration plans, full practical Windows Registry support, Linux equivalents for relevant Registry effects, autostart, managed/enforced policy, expanded C ABI coverage, Windows verification, and real migration examples.
- **Are file associations and protocol handlers in `ld_settings` first scope?** No. They are unsupported in first `ld_settings`; users should open GitHub issues for them. They likely belong in future desktop integration work.
- **Is autostart in `ld_settings` first scope?** Yes. Implement Windows startup Registry behavior and Linux XDG Autostart support with explicit safety flags.
- **Are managed and enforced settings in first scope?** Yes. Implement Windows Registry policy support and Linux dconf/GSettings-compatible policy files where the backend/schema is available, without a GLib dependency.

## File Watcher Follow-Up

- **Should `ld_watch` depend on libuv?** No required dependency in the first sample. Recommend libuv directly for apps that already want its loop and coarse rename/change events; keep it as the main reference and possible optional backend because its event loop is a large API choice.
- **How should recursive Linux watches be exposed?** Current leaning: explicit capability reporting, with native recursive support false for inotify and emulation only when the caller chooses it.
- **How should overflow be tested?** Use a simulated backend or injected event path for deterministic tests, plus real inotify smoke tests; do not rely on forcing a real kernel queue overflow in routine CI.
- **How much debounce belongs in core?** Include separate opt-in debounce and settled-file helpers for ShareX-style workflows, but keep higher-level task routing application-owned.
- **How should watcher paths be represented?** Return an `ld_watch` path value with absolute path, optional root-relative path, watch/root identity, and backend-debug representation; do not expose bare strings as the normal event path model.
- **What shared vocabulary is needed before watcher code?** ADR 0009 answers this, and the C++ implementation now exposes `severity`, `diagnostic`, and `to_string(severity)` through `ld_core`; `ld_settings` aliases and C ABI names stay stable.
