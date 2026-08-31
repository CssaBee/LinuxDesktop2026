# 51 - Harden Plugin Path Kind Taxonomy

**What to build:** harden `ld_paths::plugin_path_kind` across plugin ecosystems,
including audio plugins, IDE/editor extensions, creative-tool add-ons,
application plugins, and toolkit plugin paths. Add first-class enum values only
where LinuxDesktop2026 owns a reusable discovery convention; add typed named
plugin kinds/path sets so products can describe ecosystem-specific or
product-owned plugin roots without extending the enum for every case.

**Blocked by:** plugin ecosystem taxonomy decision.

**Status:** proposed

- [ ] Keep existing plugin path kinds that already have known cross-host path
  conventions: LADSPA, DSSI, LV2, VST2, VST3, CLAP, SF2, SFZ, and JSFX, until
  the taxonomy split below replaces the mixed list.
- [ ] Re-evaluate SF2 and SFZ naming. They are supported path-set kinds today,
  but they are soundfont/sample-library formats rather than executable plugin
  ABIs. Split executable plugin kinds from asset/library path kinds before 1.0.
- [ ] Add first-class `audio_unit` and `aax` kinds based on macOS/Windows
  ecosystem conventions and platform-scoped defaults.
- [ ] Keep Vamp as a typed named kind unless a FlavorTest needs analysis-plugin
  support as a built-in convention.
- [ ] Treat AUv3 separately from classic Audio Unit. Its app-extension/container
  model does not enter the filesystem search-root helper unless a source anchor
  proves that behavior belongs here.
- [ ] Survey and classify non-audio plugin ecosystems instead of adding them
  straight to the enum: VS Code extensions, JetBrains plugins, Blender
  add-ons/extensions, OBS plugins, GIMP plug-ins/scripts, Krita Python plugins,
  Qt plugin paths, and other source-anchored cases.
- [ ] Do not add `qt` as a first-class built-in kind in this task. Add a
  side-by-side counterexample showing that LinuxDesktop2026 can assemble or
  validate candidate roots while Qt keeps `QT_PLUGIN_PATH`, executable-relative
  plugin directories, `qt.conf`, compiled Qt plugin directories, ABI/version
  checks, and plugin loading/deployment behavior.
- [ ] Treat VS Code, JetBrains, Blender, OBS, GIMP, Krita, and Qt as typed named
  kinds or FlavorTest-specific/product/toolkit-owned requests unless maintained
  evidence proves a reusable LinuxDesktop2026 convention.
- [ ] Add tests for every supported built-in plugin/path-set kind, including
  platform-scoped defaults and unsupported-platform empty/default behavior where
  applicable.
- [ ] Add at least one typed named-kind test that proves products can model
  OBS/GIMP/VS Code-style plugin roots without asking LinuxDesktop2026 to grow
  another enum value.
- [ ] Avoid adding legacy or host-bound formats such as RTAS/TDM unless a live
  FlavorTest needs them.
- [ ] Add typed named plugin kinds for product-owned or supported-but-not-enum
  ecosystems, with environment variable, defaults, extension, category, and
  platform support metadata as needed.
- [ ] Update CtrlrX and any plugin-oriented examples to use the settled API, and
  keep unsupported-platform absence quiet rather than diagnostic-noisy.

## Breaking Change

If named plugin kinds replace plain `custom_plugin_path_set::name` strings, C++
call sites may change before 1.0. Existing enum values should remain stable
unless the taxonomy itself is wrong.

## Implementation Notes

The rule under test is: a first-class enum value means LinuxDesktop2026 knows an
ecosystem-wide discovery convention and the convention is useful outside one
product family. Product-specific plugin folders, application-only extensions,
sample libraries, ROM symbols, and device description files should remain
product-owned search roots or named custom sets. Current research supports
first-class review for Audio Unit and AAX, careful classification for Vamp,
explicit non-expansion for legacy RTAS/TDM unless maintained evidence asks for
them, and named-kind treatment for many IDE/editor/creative-tool plugin roots
that are real but product- or toolkit-owned.

Settled design points:

- The survey scope is all plugin ecosystems, including audio.
- Split executable plugin kinds from asset/library path kinds.
- Add first-class `audio_unit` and `aax`.
- Keep Vamp named until stronger evidence asks for a built-in kind.
- Treat Qt as a side-by-side counterexample. LinuxDesktop2026 may help with
  candidate roots, but Qt owns its plugin semantics.
- Keep product-specific IDE/editor and creative-tool roots named.
- Test every built-in path-set kind plus at least one typed named-kind example.
