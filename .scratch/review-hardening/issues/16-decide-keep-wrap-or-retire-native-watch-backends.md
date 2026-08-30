# 16 — Decide Keep, Wrap, Or Retire Native Watch Backends

**What to build:** The project should make an evidence-based decision about owning native watcher backends versus wrapping or recommending existing watcher libraries.

**Blocked by:** 12 — Stress Recursive Watch Behavior; 18 — Expand CI Into Portability Evidence.

**Status:** done

**Resolution:** Keep the native Linux `inotify` and Windows
`ReadDirectoryChangesW` backends as owned pre-1.0 project behavior. Keep libuv
optional and recommendation-shaped for applications that already own a libuv
loop. Keep efsw as the first serious wrap candidate only if CI, stress tests, or
maintained consumer branches show that native backend maintenance cost is
outweighing LinuxDesktop2026's migration-shaped watcher contract.

- [x] Native backend maintenance cost is compared against libuv and efsw-style alternatives using stress-test and integration evidence.
- [x] The decision records which behavior LinuxDesktop2026 owns and which behavior it delegates or recommends elsewhere.
- [x] README and watcher docs reflect the decision without overstating backend portability.
