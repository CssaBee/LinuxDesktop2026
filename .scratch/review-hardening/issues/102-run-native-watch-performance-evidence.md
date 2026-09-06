# 102 - Run Native Watch Performance Evidence

**What to build:** Collect native-backend watcher performance evidence before
optimizing public event path types.

**Blocked by:** `81` - Add Coverage Failure-Mode And Watch Performance
Evidence.

**Status:** pending

- [ ] Run the watcher performance probe or an equivalent measurement against
  native Linux `inotify`.
- [ ] Run the equivalent Windows `ReadDirectoryChangesW` measurement when the
  Windows lane is available.
- [ ] Record throughput, queue depth, backend depth, RSS growth, settled-file
  latency, and whether path construction dominates measured cost.
- [ ] Keep `std::filesystem::path` in public watcher values unless native data
  or maintained consumer evidence shows it is the bottleneck.
- [ ] Update `docs/validation-evidence.md` with the native-backend results and
  decision.

## Evidence Fit

Performance evidence should catch this: the risk is optimizing API shape from a
speculative hot-path concern instead of measured backend behavior.

## Release Gate

Does not block `0.2.0`.
