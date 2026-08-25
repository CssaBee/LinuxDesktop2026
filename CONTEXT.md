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

**Permissive license posture**:
The intent to publish the general-purpose platform libraries under the MIT License, while preserving compatibility with proof-case constraints.
_Avoid_: Private helper license, application-bound licensing

**Survey artifact set**:
The documentation produced by the requirement survey: repository notes, a Windows feature matrix, module priority scoring, and open questions.
_Avoid_: Giant report, research notes pile

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
