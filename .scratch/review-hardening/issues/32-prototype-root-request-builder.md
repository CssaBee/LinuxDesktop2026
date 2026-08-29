# 32 - Prototype Root Request Builder

**What to build:** Add a narrow experimental builder for common root-request
setup without creating a second application-profile DSL inside
LinuxDesktop2026.

**Blocked by:** 24 - Add Settings Root Construction Helpers; 27 - Record
FlavorTest API Friction Notes.

**Status:** done

- [x] The builder covers repeated mechanics such as app identity, resource
  root, portable marker, settings override, sync override, and named-root
  declarations.
- [x] The builder does not encode product profiles such as Notepad++ local
  mode, qBittorrent profiles, or KeePassXC roaming/local policy.
- [x] At least two dense flavors and one simple flavor try the builder.
- [x] The rewritten call sites are judged easier to scan without hiding product
  policy.
- [x] `docs/FlavorTests/API_FRICTION.md` records whether the builder should be
  promoted, revised, or deleted.

## Problem Statement

Notepad++, qBittorrent, KeePassXC, KiCad, and FreeCAD still build dense root
requests. Some density is honest product policy, but some may be repeated option
setup that belongs behind a convenience surface.

## Result

Added `linuxdesktop::settings::root_request_builder` as a narrow experimental
C++ helper around `root_options` and `resolve_app_roots()`. Notepad++,
qBittorrent, and KiCad now use it for repeated request mechanics. Product policy
branches, such as qBittorrent's profile override and KiCad's project-keyed
backup paths, remain in adapter code.

`docs/FlavorTests/API_FRICTION.md` now records the current recommendation: keep
the builder experimental and consider promotion based on the clearer call sites,
but do not force KeePassXC or FreeCAD through it while their product-owned XDG
and environment precedence rules read better as direct code.
