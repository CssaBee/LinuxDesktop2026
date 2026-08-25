# LinuxDesktop2026

LinuxDesktop2026 is an early-stage project to study how Windows-heavy desktop applications use platform features, then design small reusable libraries that make native Linux support easier.

The project starts from a practical proof case: Notepad++ on Ubuntu. Notepad++ is not the whole identity of the project, and a complete native Linux port is not assumed. The durable goal is a set of general-purpose platform libraries that GitHub users and AI coding agents can understand, build, and reuse.

## Current Stage

We are in the survey and design stage.

Before writing production library code, we will:

1. Survey 15 to 20 repositories that use Windows desktop features.
2. Include about five reference implementations that already abstracted or ported similar features.
3. Include unported candidates where Linux support was frequently requested.
4. Score candidate modules by real usage, coupling, Linux complexity, standalone usefulness, and proof-case value.
5. Run a focused follow-up search for the strongest module candidates.
6. Pick the first tiny working code sample only after the evidence supports it.

## Candidate Areas

First candidates:

- Settings/config
- File watching
- Process and shell integration
- Dynamic library loading
- Filesystem/path helpers
- Single-instance IPC

UI-adjacent candidates:

- GUI/windowing
- Clipboard
- Drag-and-drop
- Common dialogs/resources

Future work candidates:

- Printing
- Plugin ABI
- Advanced theming/DPI
- Accessibility
- Installer/package integration

## Design Principles

- General-purpose branding: Notepad++ is a proof case, not the product boundary.
- Portable core APIs with explicit capability reporting.
- C++ first, with public API hygiene that keeps future Rust bindings or reimplementation plausible.
- Standard library first; small optional dependencies only when they earn their place.
- GUI toolkit dependencies stay isolated to UI-facing modules.
- CMake consumption should support `FetchContent`, `add_subdirectory`, and installed package configuration.
- Public GitHub publication should wait until there is at least one tiny working code sample.

## Documentation

- [Domain language](CONTEXT.md)
- [Library roadmap](docs/plan/library-roadmap.md)
- [Notepad++ proof case plan](docs/plan/notepad-plus-plus-poc.md)
- [Repository survey template](docs/survey/repositories.md)
- [Windows feature matrix](docs/survey/windows-feature-matrix.md)
- [Module priority score](docs/survey/module-priority-score.md)
- [Open questions](docs/survey/open-questions.md)
- [Architecture decisions](docs/adr)
- [Original investigation context](docs/context/notepad-plus-plus-native-linux-port-context.md)

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE).

