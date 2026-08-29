# 34 - Add OpenIPC Dashboard FlavorTest Candidate

**What to build:** Evaluate OpenIPC Dashboard as a FlavorTest candidate with one
hardware-free seam.

**Blocked by:** None.

**Status:** proposed

- [ ] The candidate note records OpenIPC Dashboard as a FlavorTest candidate.
- [ ] The first seam can be tested without camera hardware.
- [ ] The first seam covers default desktop profile behavior and explicit
  data-root or service-profile behavior.
- [ ] The note records where Qt already solves the seam and where
  LinuxDesktop2026 may still add value through neutral concepts or diagnostics.

## Candidate Facts

OpenIPC Dashboard is a cross-platform Qt/QML and CMake desktop application with
Windows and Linux release artifacts, an embedded web companion, `--server-only`
mode, and an `OPENIPC_DATA_ROOT` override for dedicated service profiles.

## Likely First Seam

Start with data-root override and profile separation for desktop versus
`--server-only` mode. That pressures `ld_paths`, settings/state placement,
diagnostics, and future service lifecycle boundaries without needing hardware.
