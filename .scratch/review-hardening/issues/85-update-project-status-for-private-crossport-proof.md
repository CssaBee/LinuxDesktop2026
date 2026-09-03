# 85 - Update Project Status For Private Crossport Proof

**What to build:** Update the project status ledger so maintained-consumer
proof state reflects the private remote crossport repository and its recent
integration changes.

**Blocked by:** None.

**Status:** done

- [x] Change `docs/project-status.md` so it no longer says the Notepad++ proof
  needs remote availability when the remote exists but is private.
- [x] Distinguish private remote availability from public proof evidence.
- [x] Record the latest crossport proof commits and what evidence they add:
  generated platform defaults, `ld_root` topology use, and product-owned
  diagnostics.
- [x] Keep the remaining validation gates honest: observed CI, rebase cadence,
  and later maintenance evidence still need explicit entries if they are not
  complete.
- [x] Cross-check `docs/consumer-branches/notepadpp-settings-proof.md`,
  `docs/project-status.md`, README, and CI evidence docs for the same stale
  wording.

## Review Anchor

The user noted that the status update markdown still says the crossport proof
does not exist remotely, while `../LinuxDesktop2026-crossport-notepadpp` has
already received several maintained-proof changes and the repository now exists
as a private remote that this workspace can access.
