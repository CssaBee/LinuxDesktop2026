# 16 — Decide Keep, Wrap, Or Retire Native Watch Backends

**What to build:** The project should make an evidence-based decision about owning native watcher backends versus wrapping or recommending existing watcher libraries.

**Blocked by:** 12 — Stress Recursive Watch Behavior; 18 — Expand CI Into Portability Evidence.

**Status:** ready-for-agent

- [ ] Native backend maintenance cost is compared against libuv and efsw-style alternatives using stress-test and integration evidence.
- [ ] The decision records which behavior LinuxDesktop2026 owns and which behavior it delegates or recommends elsewhere.
- [ ] README and watcher docs reflect the decision without overstating backend portability.
