# Notepad++ Proof Case Plan

Notepad++ is a proof case for the general-purpose platform libraries, not a guaranteed full native Linux port.

## Timing

Fork Notepad++ only after:

- the first survey artifact set exists,
- module priority scores are complete,
- the first two or three module APIs are sketched,
- and the strongest Notepad++ proof-case dependency is identified.

## Success Signals

- One or more Notepad++ subsystems can use reusable platform abstractions.
- The work produces evidence about which Windows dependencies block native Linux support.
- The project can explain honestly if a complete native Linux port is too expensive.
- The resulting libraries and examples help other GitHub projects with similar Windows dependencies.

## Initial Native Linux POC Shape

The target POC is a native Ubuntu build that can open, edit, and save text, with a basic window and menu. Plugins, printing, and broad preferences are deferred unless the survey changes the priority.

