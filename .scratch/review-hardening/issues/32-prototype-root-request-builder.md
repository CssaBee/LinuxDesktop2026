# 32 - Prototype Root Request Builder

**What to build:** Add a narrow experimental builder for common root-request
setup without creating a second application-profile DSL inside
LinuxDesktop2026.

**Blocked by:** 24 - Add Settings Root Construction Helpers; 27 - Record
FlavorTest API Friction Notes.

**Status:** proposed

- [ ] The builder covers repeated mechanics such as app identity, resource
  root, portable marker, settings override, sync override, and named-root
  declarations.
- [ ] The builder does not encode product profiles such as Notepad++ local
  mode, qBittorrent profiles, or KeePassXC roaming/local policy.
- [ ] At least two dense flavors and one simple flavor try the builder.
- [ ] The rewritten call sites are judged easier to scan without hiding product
  policy.
- [ ] `docs/FlavorTests/API_FRICTION.md` records whether the builder should be
  promoted, revised, or deleted.

## Problem Statement

Notepad++, qBittorrent, KeePassXC, KiCad, and FreeCAD still build dense root
requests. Some density is honest product policy, but some may be repeated option
setup that belongs behind a convenience surface.
