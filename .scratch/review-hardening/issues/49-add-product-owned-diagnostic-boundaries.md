# 49 - Add Product-Owned Diagnostic Boundaries

**What to build:** `ld_core` should give consumers a small product-diagnostic
translation affordance, and cross-port public headers should expose
product-owned diagnostics and operation summaries instead of returning
LinuxDesktop2026 report types from product-shaped methods. LinuxDesktop2026
reports stay available inside adapters and proof executables, but the public
product seam should speak in the adopting application's vocabulary.

**Blocked by:** 48 - Contract Settings-Owned Root Builder.

**Status:** completed

- [x] Add a small `ld_core` helper for translating LinuxDesktop2026 diagnostics
  into caller-owned diagnostic structs with caller-owned severity enums,
  library-owned handling flags, prefixed or remapped codes, messages, and
  related paths.
- [x] Define a small Notepad++-owned diagnostic vocabulary for the cross-port
  proof, including severity, stable product codes, handling flags, messages,
  and optional related paths.
- [x] Translate `linuxdesktop::diagnostic` severity and codes into Notepad++
  diagnostics at the adapter boundary without requiring a Notepad++ table of
  LinuxDesktop2026 diagnostic codes. The proof may preserve raw details
  internally for debugging, but
  product-shaped public structs should not store
  `std::vector<linuxdesktop::diagnostic>`.
- [x] Replace cross-port public methods that return
  `linuxdesktop::settings::config_defaults_report`,
  `linuxdesktop::settings::write_report`, or
  `linuxdesktop::migration::migration_plan` with Notepad++-owned summaries that
  carry the decisions the caller needs: success/failure, selected paths,
  backup/import intent, skipped/blocked state, and product diagnostics.
- [x] Keep LinuxDesktop2026 module dependencies in implementation targets, not
  adapter headers consumed by product-shaped code. A product consumer that only
  includes the Notepad++ settings backend header should not need LinuxDesktop2026
  headers or link targets just to name result types.
- [x] Update the Notepad++ cross-port proof executable and tests to print or
  inspect product diagnostics first, while still allowing explicit internal
  LinuxDesktop2026 report inspection in proof-only code.
- [x] Re-run the maintained cross-port proof and the in-tree Notepad++
  FlavorTest. `docs/FlavorTests/API_FRICTION.md` should describe remaining
  friction without changelog wording.

## Breaking Change

This is an intentional proof-branch API contraction. Product-shaped public
methods that currently expose LinuxDesktop2026 reports should return
Notepad++-owned summaries instead. The replacement keeps enough diagnostic
severity and detail for command-line proof output, but removes the requirement
that product callers know LinuxDesktop2026 report types.

## Implementation Notes

Treat LinuxDesktop2026 diagnostics as adapter input, not product output. Start
with `ld_core` because `severity` and `diagnostic` are shared by every module.
Then use the cross-port Notepad++ settings backend because it exposes the leak
most clearly: `SettingsLayout` stores raw diagnostics, `ensure_defaults()` and
`write_config_xml()` return settings reports, and
`plan_legacy_config_import()` returns a raw migration plan. The product-owned
replacement should use the core translation helper to map the generic severity
ladder and library-owned handling flags into Notepad++ terms at the adapter
boundary.
