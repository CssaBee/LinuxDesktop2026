# Notepad++ Native Linux Port Investigation

This context tracks the language we are using while exploring a native Linux path for Notepad++, with a focus on separating application behavior from Windows-specific integration.

## Language

**Native Linux POC**:
A first working Linux build of Notepad++ that runs without Wine and proves the editor can open, edit, and save text on Ubuntu.
_Avoid_: Full port, complete Linux release

**Source-level compatibility**:
Preserving behavior and code structure where practical so core application logic can be reused across Windows and Linux builds.
_Avoid_: Binary compatibility, plugin ABI compatibility

**Plugin ABI compatibility**:
The ability for existing Notepad++ plugins to run unchanged as Windows binaries on Linux.
_Avoid_: Plugin support, source compatibility

**Platform layer**:
The set of abstractions that hides OS-specific behavior such as windows, settings, process execution, file watching, shell integration, and clipboard access.
_Avoid_: Miscellaneous wrappers, utility layer

**GTK-first**:
The preferred initial Linux GUI path when it reduces porting effort, while keeping Qt as a viable fallback until the evidence says otherwise.
_Avoid_: GTK-only, Qt-only

**Requirement survey**:
An evidence-gathering pass across real repositories to learn how Windows features are used before designing reusable platform libraries.
_Avoid_: Similar app list, porting showcase

**Survey corpus**:
A set of 15 to 20 repositories used as evidence for the first platform-library design, including about five projects that already have a successful abstraction or native port.
_Avoid_: Exhaustive repository scan, Notepad++ only

**Extended survey watchlist**:
A larger set of products, frameworks, utilities, and boundary cases to revisit when a new module needs fresh evidence beyond the first survey corpus.
_Avoid_: Audited corpus, link dump, final adoption list

**Unported candidate**:
A Windows-oriented application whose users have repeatedly requested Linux support, but where maintainers have declined, deferred, or never completed a native Linux port.
_Avoid_: Failed project, irrelevant Windows app

**Agent-friendly library**:
A shared library whose purpose, boundaries, examples, and operating-system behavior are easy for both humans and AI coding agents to discover and apply correctly.
_Avoid_: Magic wrapper, black-box abstraction

**Reference implementation**:
A repository in the survey corpus that already solved or partially solved a Windows-to-Linux abstraction problem and can inform our design.
_Avoid_: Golden model, template to copy

**Module priority score**:
A ranking for a Windows feature based on frequency, coupling, Linux replacement complexity, standalone library usefulness, and value to the Notepad++ Ubuntu POC.
_Avoid_: Gut-feel priority, popularity

**Follow-up search**:
A broader repository search performed after the first survey and scoring pass, focused only on the platform-library candidates that still look worth pursuing.
_Avoid_: Open-ended research, exhaustive mining

**Portable core**:
The common behavior a platform library promises across supported operating systems.
_Avoid_: Lowest common denominator, fake parity

**Capability reporting**:
An explicit API surface that lets callers discover platform-specific availability, limits, and behavior differences.
_Avoid_: Silent fallback, hidden incompatibility

**CMake consumption path**:
The supported ways downstream projects include the libraries, including FetchContent, add_subdirectory, and installed package configuration.
_Avoid_: Build instructions only, copy-paste integration

**Rust-portable design**:
An API design constraint that keeps future Rust bindings or reimplementation plausible by avoiding unnecessary C++-specific coupling at public boundaries.
_Avoid_: Rust implementation, Rust rewrite

**General-purpose platform libraries**:
The shared libraries we intend to publish for broader GitHub use, with Notepad++ serving as a proof case rather than the project identity.
_Avoid_: Notepad++ porting helpers, application-private wrappers

**Phase-one support matrix**:
The initial platform promise of Windows 10/11 and Ubuntu LTS, with other Linux distributions best-effort and no macOS commitment.
_Avoid_: All desktop platforms, Linux everywhere

**API hygiene rule**:
A public API design constraint that keeps concepts small, ownership explicit, threading documented, and future non-C++ bindings plausible.
_Avoid_: Rust implementation requirement, premature FFI layer

**Pre-1.0 C++ API rule**:
Before `1.0`, C++ APIs may break when source audits, proof integrations, or
module-boundary corrections show that the current shape is wrong. Such breaks
must be deliberate, documented, and accompanied by replacement guidance. Existing
C ABI entry points are best-effort compatible where practical until
release-candidate status; C ABI expansion waits until then.
_Avoid_: Stable C++ ABI promise, permanent prototype API

**Permissive license posture**:
The intent to publish the general-purpose platform libraries under the MIT License, while preserving compatibility with proof-case constraints.
_Avoid_: Private helper license, application-bound licensing

**Survey artifact set**:
The documentation produced by the requirement survey: repository notes, a Windows feature matrix, module priority scoring, and open questions.
_Avoid_: Giant report, research notes pile

**Challenge ecosystem**:
A recurring set of module-sized portability projects that can serve as university assignments, contributor onboarding, API validation, and future implementation experiments.
_Avoid_: Toy exercises, tutorial-only examples, unrelated coding katas

**Flavor test**:
A small adapter-and-test harness that rewrites a real upstream seam against LinuxDesktop2026 APIs and measures how little glue is needed to integrate it.
_Avoid_: Toy example, mock-only compatibility test, benchmark suite

**Flavor review round**:
A later critique/defense pass over completed flavor tests that judges how well the refactor blends into the original product shape.
_Avoid_: Unit test result, implementation gate, pass/fail criterion

**Framework tax budget**:
The tolerated amount of LinuxDesktop2026-specific vocabulary that a product-shaped seam may expose before the API needs a narrower convenience layer or the abstraction should be rejected.
_Avoid_: Boilerplate count, style preference, mandatory metric

**Product-owned diagnostic**:
A warning, error, or status item expressed in the adopting application's own
vocabulary, even when it is derived from LinuxDesktop2026 internal reports.
_Avoid_: Raw library diagnostic, passthrough report, generic status dump

**Desktop integration effect**:
A platform action that registers an application with the desktop/session environment, such as autostart entries, desktop entries, icons, file associations, MIME types, or URL protocol handlers.
_Avoid_: Settings file, GUI toolkit feature, installer only

**`ld_desktop` extraction**:
The module that owns desktop integration effects, managed/enforced policy,
shell-equivalent behavior, desktop database updates, and Registry-equivalent
desktop/system behavior.
_Avoid_: Stable `ld_settings` effect API, settings-owned shell integration

**`ld_migration` extraction**:
The module that owns migration planning, file/directory moves, rollback
reporting, app-settings Registry snapshot/import/export compatibility, and
cross-module migration orchestration.
_Avoid_: Stable `ld_settings` migration engine, general Registry editor

**Path resolver**:
A platform-library capability that chooses, normalizes, and explains filesystem locations for application use without owning the files' payload formats or application policy.
_Avoid_: File manager, settings engine, migration executor

**Path family**:
A named group of related filesystem locations that share a purpose, such as user configuration, cache, state, documents, executable roots, resource roots, or plugin search roots.
_Avoid_: Path string, folder bucket, arbitrary directory

**Root module**:
The proposed public `ld_root` module that would own application root topology across user-owned and app-owned roots, sitting between generic path families and settings-specific config lifecycle.
_Avoid_: Internal settings cleanup, root helper, path resolver rename

**Plugin path set**:
A typed collection of search roots for one plugin or asset ecosystem, including its platform defaults, environment overrides, and compatibility fallbacks.
_Avoid_: Plugin ABI, plugin host, dynamic loader

**Service/daemon lifecycle**:
The operating-system behavior needed to run, signal, supervise, or communicate with a long-running background process.
_Avoid_: Ordinary process launch, single-instance GUI activation

**Platform-library monorepo**:
A single repository containing the general-purpose platform libraries, examples, tests, and shared build infrastructure while module boundaries are still evolving.
_Avoid_: Separate repo per module, Notepad++ fork

**Proof case**:
A real application used to test whether the platform libraries help with difficult Windows-to-Linux migration work.
_Avoid_: Guaranteed full port, only target application

**First candidate module**:
A platform-library area likely to be designed first if the survey confirms broad demand, such as settings, file watching, process/shell integration, dynamic library loading, filesystem/path helpers, or single-instance IPC.
_Avoid_: Final module commitment, complete library roadmap

**UI-adjacent candidate**:
A platform-library area involving UI, clipboard, or drag-and-drop behavior that needs special survey attention before deciding whether it belongs in reusable libraries or toolkit-specific application code.
_Avoid_: First candidate module, guaranteed GUI abstraction

**Future work candidate**:
A platform capability that may matter later but should not expand the first design wave unless the survey reveals unusually strong demand.
_Avoid_: Out of scope forever, forgotten feature

**Existing tool scan**:
A lightweight review of libraries, framework APIs, Rust crates, native wrappers, and abandoned attempts that might already solve or explain a candidate module.
_Avoid_: Dependency shopping, rewrite justification

**Tool adoption rule**:
The rule for using existing tools: adopt healthy small dependencies, wrap useful but awkward backends, learn from unsuitable projects, and build only when there is a real unmet need.
_Avoid_: NIH policy, dependency-first policy

**Repository survey entry**:
A structured note for one surveyed repository covering its platform status, Linux demand signal, Windows feature usage, relevant source locations, abstractions, build system, license, and lessons for our modules.
_Avoid_: Link dump, anecdotal example

**Public milestone**:
The first project state worth publishing publicly on GitHub, consisting of at least one tiny working code sample plus the supporting survey and design docs.
_Avoid_: Docs-only launch, full release

**Staged sample selection**:
The rule that the first working code sample is chosen only after the initial survey, module scoring, and focused follow-up search identify the strongest module candidate.
_Avoid_: Premature first sample, settings-first assumption
