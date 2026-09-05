# 76 - Finish Maintained Consumer Proof Evidence

**What to build:** Promote the Notepad++ proof from local validation to
maintained-consumer evidence.

**Blocked by:** 15 - Validate APIs With One Maintained Consumer Branch.

**Status:** in progress

- [x] Push the proof branch/repository to a stable remote location.
- [ ] Record at least one observed green CI run against the current package.
- [x] Record rebase cadence, include/link friction, dependency friction, and
  adapter churn over at least one later maintenance pass.
- [x] Keep FlavorTests clearly labeled as product-shaped fixtures, not proof
  of maintained upstream economics.

## Implementation Notes

The crossport repository is already private on GitHub and tracked locally:
`../LinuxDesktop2026-crossport-notepadpp` on
`linuxdesktop2026-settings-proof` tracks `origin/linuxdesktop2026-settings-proof`
at `a296934fe`. The local task-76 proof pass installed LinuxDesktop2026 commit
`af518aa` into `/tmp/linuxdesktop2026-task76-prefix`, configured the proof
against the exported package, built
`linuxdesktop2026_notepadpp_settings_proof`, and passed CTest 1/1.

Observed GitHub Actions evidence is still open. Network access could reach the
GitHub remote only after escalation, and then HTTPS required noninteractive
credentials. The manual workflow now names `LD2026_CROSSPORT_READ_TOKEN` as the
private crossport checkout token path.

## Review Anchor

The newer review says the maintained-consumer proof is still the central
strategic unknown and remains blocked until remote, CI, rebase, and maintenance
evidence exist.
