# 87 - Add Staged XDG Desktop Registration Artifacts

**What to build:** Implement and test the Linux/XDG staged artifact side of the
accepted `ld_desktop` registration surface.

**Blocked by:** 86 - Design Desktop Bundle Registration Surface.

**Status:** pending

- [ ] Add staged write/query/remove behavior for desktop entries under XDG data
  application roots.
- [ ] Add staged icon installation metadata and path validation without claiming
  live icon-cache refresh.
- [ ] Add MIME declaration and association artifact support, including
  `MimeType=` desktop-entry metadata and `mimeapps.list` update behavior.
- [ ] Add default-application and URL-scheme handler artifacts, including
  `x-scheme-handler/*` handling.
- [ ] Return activation/update plans for `update-desktop-database`,
  `update-mime-database`, and icon-cache refreshes rather than running them by
  default.
- [ ] Add adversarial tests for IDs, field escaping, MIME names, protocol names,
  relative paths, global-write denial, duplicate/removal behavior, and cleanup
  reports.
- [ ] Keep Desktop Flavor tests hermetic for `xdg_full_gnome`, `xdg_full_kde`,
  `xdg_light_xfce`, and `xdg_minimal_bare_wm`.

## Review Anchor

The accepted investigation found that GNOME, KDE Plasma, Xfce, and lighter XDG
sessions can share staged registration artifacts, while live desktop-session
consumption and database activation need explicit capability diagnostics.
