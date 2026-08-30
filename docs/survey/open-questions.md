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
- **Which Notepad++ subsystem should become the first proof case?** Answered: start with settings/config and the path roots required by settings because it is narrow, recurring across surveyed apps, and can produce a tiny working sample before UI-heavy work.
- **Should file watching become the next implementation module?** Answered for the current roadmap: yes, `ld_watch` is the next module candidate after the broader audit, with ADR 0010 now defining the implementation-ready public boundary before prototype work.
- **What did the first Notepad++ subsystem pass find?** Settings/config is not a scalar settings problem. It is a settings root resolver plus config bundle manager with ordered save phases, plugin-facing roots, hydration, backup/restore, and validation-after-write requirements.
- **Which existing tools should be adopted, wrapped, studied, or rejected for the first module?** Partly answered for settings/config: adopt XDG Base Directory, Microsoft Known Folders, and `std::filesystem`; wrap or recommend Boost.Nowide if needed; recommend or adapt Qt, GLib, and wxWidgets; defer desktop/shell specs and portals.
- **Which module should provide the first tiny working code sample?** Settings/config. ADR 0008 selects it, and `ld_settings_demo` is now the first executable sample.
- **Which candidate modules deserve focused follow-up after the first scoring pass?** File watcher and process/shell are the strongest next first-candidate follow-ups. Dynamic library loading and single-instance IPC remain first-wave candidates but should wait until the next evidence pass is chosen.
- **What did the file watcher focused pass find?** File watching is a real reusable seam, but the useful module is not a thin inotify wrapper. It should expose event mapping, debounce/stabilization, settled-file readiness, overflow/rescan diagnostics, recursive-policy honesty, dirty-path refresh as a named future layer, and app-owned routing.
- **What should happen before `ld_watch` code?** Answered: shared C++ diagnostics now live in `ld_core`, the broader watcher audit is complete, ADR 0010 contains the implementation-ready header/source/test checklist, and the first broad prototype now exists.
- **Why is the README survey/scoring row still in progress if the watcher audit is done?** Answered: that row tracks the broader survey/scoring program, not unfinished `ld_watch` audit work. The watcher application/library follow-up is complete enough; `ld_watch` now has a verification-first hardening backlog.
- **What is left before `ld_settings` is shippable?** Answered at the product level: the current code is a working first sample, not a finished module. Before ship, `ld_settings` must narrow to settings/config behavior: settings-specific roots, config layers, bundle hydration, safe writes, diagnostics, and real consumer examples. Generic path policy moves to `ld_paths`; desktop integration effects move to `ld_desktop`; migration planning/execution and app-settings Registry migration compatibility move to `ld_migration`.
- **Are file associations and protocol handlers in `ld_settings` first scope?** No. They belong to `ld_desktop`.
- **Is autostart in `ld_settings` first scope?** No. The C++ implementation now lives in `ld_desktop`; the old `settings::effects` bridge has been removed.
- **Are managed and enforced settings in first scope?** Managed/enforced policy is not stable `ld_settings` scope. It belongs to `ld_desktop` when expressed as administrator policy, even when the Linux representation uses dconf/GSettings-compatible files.
- **What module followed the settings/watch work?** Answered: `ld_paths` is now an active prototype. The accepted plan is resolver-first but broad enough for a community-facing prototype, with standard user paths, executable/resource/install roots, source-labeled candidate reports, path-list parsing, typed plugin path sets, Wine-prefix-aware defaults, opt-in directory creation, C++17 first, and a small C ABI before public prototype announcement.

## File Watcher Follow-Up

- **Should `ld_watch` depend on libuv?** No required dependency in the first sample. Recommend libuv directly for apps that already want its loop and coarse rename/change events; keep it as the main reference and possible optional backend because its event loop is a large API choice.
- **How should recursive Linux watches be exposed?** Current leaning: explicit capability reporting, with native recursive support false for inotify and emulation only when the caller chooses it.
- **How should overflow be tested?** Use a simulated backend or injected event path for deterministic tests, plus real inotify smoke tests; do not rely on forcing a real kernel queue overflow in routine CI.
- **How much debounce belongs in core?** Include separate opt-in debounce and settled-file helpers for ShareX-style workflows, but keep higher-level task routing application-owned.
- **How should watcher paths be represented?** Return an `ld_watch` path value with absolute path, optional root-relative path, watch/root identity, and backend-debug representation; do not expose bare strings as the normal event path model. Directory watches report paths relative to the watched directory. Single-file watches report the watched filename for target-file events, hiding parent-directory watcher facades and avoiding `"."` as the public relative path.
- **What shared vocabulary is needed before watcher code?** ADR 0009 answers this, and the C++ implementation now exposes `severity`, `diagnostic`, and `to_string(severity)` through `ld_core`; `ld_settings` aliases and C ABI names stay stable.
- **What counts as Windows verification for `ld_watch`?** Require a real Windows CI or local run that builds and runs the Windows smoke tests, including create, modify, rename, delete, recursive nested creation, and single-file save-by-replace behavior through `ReadDirectoryChangesW`. The explicit CI target is wired; the backend should be called verified only after that Windows run is green.
- **Which watcher hardening cases come before calling the prototype hardened?** The first hardening pass now covers recursive deep-tree creation, save-by-replace on single-file watches, remove/rename churn, and backend/resource-limit diagnostic preservation. Leave larger performance benchmarking for later.
- **When should the `ld_watch` C ABI be designed?** Postpone it until the release-preview pass, after the C++ event, ownership, callback, queue, and path models survive native backend verification and hardening tests.
- **When should `capability_report` grow richer limit/cost fields?** Only after stress tests prove which limits or costs consumers need to inspect.

## ld_paths Follow-Up

- **Which path resolver milestones are needed before implementation?** Answered in `docs/plan/ld-paths-roadmap.md`: resolver core, directory creation/path lists, user dirs/legacy fallbacks, typed plugin path sets, and public prototype polish.
- **Which application examples should guide `ld_paths`?** Answered in `docs/survey/ld-paths-application-audit.md` and `docs/examples/migration-examples.md`: Notepad++, OpenRGB, FreeCAD, Carla, and internal `ld_settings` extraction.
- **When should `ld_settings` consume `ld_paths`?** Answered for the current prototype: `ld_settings` now routes generic root policy through `ld_paths` while keeping settings-specific hydration and writes.
- **What still needs design during implementation?** Mostly answered for the active prototype. Remaining design and verification questions are Windows Known Folder fallback behavior, UTF-8 path handling, executable-root verification, exact plugin path defaults on non-Ubuntu systems, and whether custom plugin path sets need first-cut C ABI exposure.
