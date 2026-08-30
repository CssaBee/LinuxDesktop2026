# Cross-Port And Reference Cases

FlavorTests are local, source-anchored refactors. They are useful because they
keep product-shaped seams visible without requiring LinuxDesktop2026 to carry a
fork of every upstream project.

Cross-port and reference cases answer a different question: whether the product
concept survives across platforms, and whether LinuxDesktop2026 is matching that
concept or merely mirroring one backend. They should influence API direction
only when they expose repeated product pressure, unsafe behavior, or a cleaner
boundary that an existing ecosystem already proved.

## When To Create A Separate Repository

Keep evidence inside this repository when the work is a small FlavorTest,
survey note, or source-anchored comparison.

Create a separate GitHub repository only when the work becomes a maintained,
buildable proof branch against an upstream-shaped codebase. A cross-port repo
should have at least one of these properties:

- it builds or runs tests against real upstream source structure,
- it carries a patch series that an upstream reviewer could understand,
- it has CI that proves LinuxDesktop2026 integration against a real consumer,
- or it tracks a long-lived fork experiment that would make this repository too
  noisy.

Do not create a separate repository for notes, sketches, copied snippets, or
one-off experiments that do not compile.

Recommended naming:

- `LinuxDesktop2026-crossport-notepadpp`
- `LinuxDesktop2026-crossport-obs`
- `LinuxDesktop2026-crossport-openipc-dashboard`
- `LinuxDesktop2026-crossport-openrgb`

Use the `LinuxDesktop2026-crossport-<project>` prefix so GitHub search, issue
links, and repository lists make the relationship obvious. If a repository is
an actual downstream fork that needs to read naturally to the upstream project,
`<project>-linuxdesktop2026-proof` is acceptable, but the shared prefix is the
default.

The first required maintained branch is the Notepad++ settings proof described
in `docs/consumer-branches/notepadpp-settings-proof.md`. Until that branch has
clean build and rebase evidence, local FlavorTests remain ergonomics evidence
rather than integration-readiness evidence.

## OpenIPC Dashboard Reference Case

OpenIPC Dashboard is both a FlavorTest and a reference case. Its value is not
that LinuxDesktop2026 should replace Qt in a Qt-native application. Its value is
that Dashboard shows which seams are already owned well by Qt and which seams
still need product-shaped integration around Linux desktop conventions.

Qt already covers these seams well enough that LinuxDesktop2026 should document,
recommend, or adapt around them instead of offering a competing abstraction:

- application identity and QGuiApplication lifecycle,
- QSettings storage once the product has selected the intended root,
- QML/application startup,
- local event-loop ownership,
- toolkit resource loading,
- and application-owned web/server objects.

Dashboard still pressures LinuxDesktop2026 concepts in narrower places:

- ordinary desktop profile root discovery can use `ld_paths`,
- service/headless startup needs clear process and environment guidance,
- service data roots select a whole product profile, not just one directory,
- diagnostics should be translated before reaching browser or UI surfaces,
- migration and updater behavior need product-owned safety prompts,
- release packaging must keep desktop and server contracts separate,
- and desktop/server separation must not be flattened into a generic settings
  root helper.

The current lesson is conservative: keep toolkit-owned seams in the toolkit,
keep Dashboard's service profile layout in Dashboard code, and only generalize
LinuxDesktop2026 helpers after another product repeats the same pressure.

## OBS Cross-Port Review Pilot

OBS is the first cross-port review pilot because its product concept appears in
both Windows and Unix-like platform helpers while its public utility surface
remains C-shaped. That makes it a useful check against accidental C++ framework
leakage.

The comparison should use source anchors and paraphrased notes, not copied
upstream code snippets. The relevant question is whether LinuxDesktop2026 can
preserve OBS-style caller-owned buffers, allocated path strings, and integer
status returns while privately replacing platform mechanics.

Initial keep/change/defer lessons:

- **Keep:** OBS-shaped public seams should stay C-shaped. `os_get_config_path()`
  and `config_save_safe()` are valuable precisely because callers do not need to
  know about LinuxDesktop2026 reports.
- **Change:** when cross-platform products share a concept such as "config
  path" or "safe config save", the FlavorTest should say whether the helper
  matches that shared concept or only one backend's implementation detail.
- **Defer:** broader C ABI expansion still waits until release-candidate status;
  the useful pre-RC lesson is boundary preservation, not binding completeness.
