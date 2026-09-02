# 50 - Add Portable Root Request API

**What to build:** replace the current split between command-line portable
requests and marker-file portable activation with one public root/settings
request shape that expresses the product concept directly: store settings,
data, state, and related user files beside an application-owned root such as the
executable directory. Gearcoleco is the source-anchored pressure case:
`--portable` and executable-adjacent `portable.ini` both mean the same product
behavior, but the current adapter has to express one as a settings override and
the other as a marker.

**Blocked by:** nothing.

**Status:** implemented

- [x] Decide whether the new API belongs in `ld_root`, `ld_settings`, or both as
  a `ld_settings` forwarding convenience over `ld_root`.
- [x] Make `ld_root` the owner of the portable-root concept. Add a thin
  `ld_settings` convenience only if Gearcoleco or Notepad++ call sites prove it
  removes real adapter noise.
- [x] Define a public request type or builder method for app-owned portable
  roots that accepts the app root, marker path, explicit requested flag, portable
  level, and privileged-install denial policy without making users manually map
  those concepts to a generic override.
- [x] Use `portable_root` as the public vocabulary and define it as an
  app-owned root selected by command line, marker, or explicit product policy.
- [x] Model command-line portable mode and marker-file portable mode as trigger
  fields on one portable-root request object.
- [x] Preserve the distinction between "portable requested" and "portable
  active" so products can report marker/CLI intent separately from accepted
  roots.
- [x] When portable mode is requested but rejected, fall back to ordinary user
  roots with a diagnostic. Products decide whether that diagnostic becomes a
  prompt, warning, or startup failure.
- [x] Default `portable_root` to profile-level behavior, so config, data, state,
  and cache move beside the app-owned root unless the caller asks for a narrower
  settings-only mode.
- [x] Keep app-owned and user-owned root vocabulary explicit enough for
  Notepad++, qBittorrent, KeePassXC, and Gearcoleco without adding product
  policy to LinuxDesktop2026.
- [x] Update Gearcoleco and the relevant existing FlavorTests to use the new
  call shape when it reads better than manual options.
- [x] Update `API_FRICTION.md`, `SOURCES.md`, and public examples with
  present-tense evidence.

## Breaking Change

Pre-1.0 C++ surface may rename or reshape portable/app-local builder calls if
the grilling settles on a clearer canonical vocabulary. Keep C ABI additions
behind the C++ decision.

## Implementation Notes

This is not a request to hide OS-specific behavior by pretending portable mode
is a normal user config root. The goal is to make the application-owned root
request explicit and product-readable while preserving diagnostics for denied
portable roots, relative overrides, and privileged install locations.

Settled design points:

- `ld_root` owns the portable-root concept.
- Public vocabulary is `portable_root`.
- One request object carries explicit and marker-based triggers.
- Rejected portable mode falls back to ordinary user roots with diagnostics.
- The default portable scope is profile-level.

Implemented shape:

- `linuxdesktop::root::portable_root_request` carries `root`, `marker`,
  `requested`, `level`, privileged-install denial policy, and user-config
  override policy.
- `linuxdesktop::root::request_builder::portable_root()` is the canonical C++
  root call. `ld_settings::root_builder::portable_root()` forwards the same
  request when settings lifecycle code needs the convenience.
- `root::report` exposes `portable_root_requested`,
  `portable_root_active`, and `portable_root`.
- C ABI fields keep their existing names and map internally to the C++ portable
  request, avoiding a C ABI break during this pass.
- Gearcoleco uses one request for `--portable` and `portable.ini`.
  Notepad++ and qBittorrent use the request where their source-shaped markers
  read as portable-root policy. KeePassXC keeps its explicit portable config
  override because that matches the source-shaped seam.
