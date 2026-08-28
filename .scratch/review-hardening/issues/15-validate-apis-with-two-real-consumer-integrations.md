# 15 — Validate APIs With Two Real Consumer Integrations

**What to build:** The current APIs should be exercised by real consumer code before the project adds more public vocabulary or module families.

**Blocked by:** 04 — Route Settings Root Resolution Through Paths; 05 — Narrow Settings Effects To Prototype-Validated Behavior; 08 — Add Durable Write Mode Where Promised; 10 — Harden Watcher Callback Lifecycle; 12 — Stress Recursive Watch Behavior; 18 — Expand CI Into Portability Evidence.

**Status:** ready-for-agent

- [ ] A small Notepad++ proof patch uses the current libraries without application code adapting around library weaknesses.
- [ ] A second Windows-heavy consumer with different settings/path/watch needs uses the current libraries.
- [ ] API pain points discovered by the integrations are recorded before any new public enums, structs, or modules are added.
