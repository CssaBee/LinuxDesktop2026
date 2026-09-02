# 51 - Harden Plugin Path Kind Taxonomy

**What to build:** harden `ld_paths::plugin_path_kind` across plugin ecosystems,
including audio plugins, IDE/editor extensions, creative-tool add-ons,
application plugins, and toolkit plugin paths. Add first-class enum values only
where LinuxDesktop2026 owns a reusable discovery convention; add typed named
plugin kinds/path sets so products can describe ecosystem-specific or
product-owned plugin roots without extending the enum for every case.

**Blocked by:** plugin ecosystem taxonomy decision.

**Status:** implemented

- [x] Keep existing plugin path kinds that already have known cross-host path
  conventions: LADSPA, DSSI, LV2, VST2, VST3, CLAP, SF2, SFZ, and JSFX, until
  the taxonomy split below replaces the mixed list.
- [x] Re-evaluate SF2 and SFZ naming. They are supported path-set kinds today,
  but they are soundfont/sample-library formats rather than executable plugin
  ABIs. Split executable plugin kinds from asset/library path kinds before 1.0.
- [x] Add first-class `audio_unit` and `aax` kinds based on macOS/Windows
  ecosystem conventions and platform-scoped defaults.
- [x] Keep Vamp as a typed named kind unless a FlavorTest needs analysis-plugin
  support as a built-in convention.
- [x] Treat AUv3 separately from classic Audio Unit. Its app-extension/container
  model does not enter the filesystem search-root helper unless a source anchor
  proves that behavior belongs here.
- [x] Survey and classify non-audio plugin ecosystems instead of adding them
  straight to the enum: VS Code extensions, JetBrains plugins, Blender
  add-ons/extensions, OBS plugins, GIMP plug-ins/scripts, Krita Python plugins,
  Qt plugin paths, and other source-anchored cases.
- [x] Do not add `qt` as a first-class built-in kind in this task. Add a
  side-by-side counterexample showing that LinuxDesktop2026 can assemble or
  validate candidate roots while Qt keeps `QT_PLUGIN_PATH`, executable-relative
  plugin directories, `qt.conf`, compiled Qt plugin directories, ABI/version
  checks, and plugin loading/deployment behavior.
- [x] Treat VS Code, JetBrains, Blender, OBS, GIMP, Krita, and Qt as typed named
  kinds or FlavorTest-specific/product/toolkit-owned requests unless maintained
  evidence proves a reusable LinuxDesktop2026 convention.
- [x] Add tests for every supported built-in plugin/path-set kind, including
  platform-scoped defaults and unsupported-platform empty/default behavior where
  applicable.
- [x] Add at least one typed named-kind test that proves products can model
  OBS/GIMP/VS Code-style plugin roots without asking LinuxDesktop2026 to grow
  another enum value.
- [x] Avoid adding legacy or host-bound formats such as RTAS/TDM unless a live
  FlavorTest needs them.
- [x] Add typed named plugin kinds for product-owned or supported-but-not-enum
  ecosystems, with environment variable, defaults, extension, category, and
  platform support metadata as needed.
- [x] Update CtrlrX and any plugin-oriented examples to use the settled API, and
  keep unsupported-platform absence quiet rather than diagnostic-noisy.

## Breaking Change

SF2 and SFZ move out of the executable `plugin_path_kind` enum into
`plugin_asset_path_kind`. C++ callers that requested those values through
`plugin_path_options::kinds` must request them through
`plugin_path_options::asset_kinds`.

The C plugin enum mirrors the C++ executable-plugin enum. C callers request SF2
and SFZ through `ld_paths_plugin_asset_path_kind`.

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

Implemented shape:

- `plugin_path_kind` represents executable plugin ecosystems: LADSPA, DSSI,
  LV2, VST2, VST3, CLAP, Audio Unit, AAX, and JSFX.
- `plugin_asset_path_kind` represents reusable asset/library ecosystems: SF2
  and SFZ.
- `named_plugin_path_set` carries product-owned or toolkit-owned search roots
  with environment variable, defaults, category, extension metadata, and
  platform-support metadata.
- `plugin_path_set` and `plugin_path_candidate` report the executable kind,
  asset kind, and category separately.
- `plugin_path_options::include_default_kinds` and
  `include_default_asset_kinds` preserve the default-discovery behavior while
  allowing named-only callers to avoid unrelated built-in path sets.
- CtrlrX uses first-class VST3, Audio Unit, and AAX kinds. A test-side Qt
  named set proves toolkit plugin paths can live beside Qt without becoming a
  LinuxDesktop2026-owned enum.
