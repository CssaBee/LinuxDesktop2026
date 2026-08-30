# Notepad++ Proof Case Plan

Notepad++ is a proof case for the general-purpose platform libraries, not a guaranteed full native Linux port.

## Timing

Start the Notepad++ fork only after:

- the first survey artifact set exists,
- module priority scores are complete,
- the strongest Notepad++ proof-case dependency is identified,
- and at least one reusable module has working code.

That threshold is now met for `ld_settings`. The fork should still begin narrowly: prepare and test a settings/config integration patch before attempting UI, file watching, plugin ABI, or shell integration.

## Success Signals

- One or more Notepad++ subsystems can use reusable platform abstractions.
- The work produces evidence about which Windows dependencies block native Linux support.
- The project can explain honestly if a complete native Linux port is too expensive.
- The resulting libraries and examples help other GitHub projects with similar Windows dependencies.

## Initial Native Linux POC Shape

The target POC is a native Ubuntu build that can open, edit, and save text, with a basic window and menu. Plugins, printing, and broad preferences are deferred unless the survey changes the priority.

## First Fork Patch Shape

The first fork patch should be a mapping exercise, not a full Linux port.

Use `docs/consumer-branches/notepadpp-settings-proof.md` as the branch contract
and evidence ledger. The proof branch is not validated merely because the
in-tree Notepad++ FlavorTest passes; it must build against an upstream-shaped
Notepad++ tree and record rebase, dependency, compile, and API-friction evidence
as normal maintenance happens.

Use `ld_settings` to replace or isolate these Notepad++ responsibilities:

- settings root resolution,
- command-line settings directory override,
- portable marker policy,
- privileged install denial for portable mode,
- config-only cloud/sync override after Notepad++ validates the cloud-choice file,
- user plugin config directory resolution,
- model-file hydration for missing config XML files,
- ordered high-value config writes,
- atomic temp-write/replace,
- backup retention,
- and validation-before-commit for session-like files.

Keep these responsibilities in Notepad++:

- XML schemas and parsing,
- shortcut/menu/session in-memory models,
- HMAC or machine-specific integrity policy,
- cloud-choice file format,
- user-facing messages,
- plugin ABI,
- Scintilla/editor behavior,
- GUI toolkit decisions,
- and Windows-specific compatibility code for the existing Windows build.

Suggested patch sequence:

1. Add LinuxDesktop2026 as a CMake/Fork dependency in a small branch without changing behavior.
2. Create an adapter near Notepad++ settings startup that converts Notepad++ inputs into `linuxdesktop::settings::root_options`.
3. Replace only computed path assignments first, leaving Notepad++ XML loading unchanged.
4. Move model-file hydration calls behind `ld_settings` while preserving Notepad++ parse/load logic.
5. Move session/config write commits to `write_with_backup`, keeping Notepad++ serialization and validation callbacks.
6. Run the branch on Ubuntu and record every remaining Windows assumption that blocks compilation.

Abort or pause criteria:

- the patch requires broad GUI rewrites before settings/config can compile,
- Notepad++ build files fight the dependency harder than expected,
- or a Windows compatibility path would be destabilized without a clean adapter boundary.
