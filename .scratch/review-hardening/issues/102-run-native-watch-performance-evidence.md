# 102 - Run Native Watch Performance Evidence

**What to build:** Collect native-backend watcher performance evidence before
optimizing public event path types.

**Blocked by:** `81` - Add Coverage Failure-Mode And Watch Performance
Evidence.

**Status:** implemented

- [x] Run the watcher performance probe or an equivalent measurement against
  native Linux `inotify`.
- [x] Record that the equivalent Windows `ReadDirectoryChangesW` measurement
  still requires an available Windows lane.
- [x] Record throughput, queue depth, backend depth, RSS growth, settled-file
  latency, and whether path construction dominates measured cost.
- [x] Keep `std::filesystem::path` in public watcher values unless native data
  or maintained consumer evidence shows it is the bottleneck.
- [x] Update `docs/validation-evidence.md` with the native-backend results and
  decision.

## Implementation Notes

- Extended `ld_watch_performance_probe` so Linux runs an additional native
  `inotify` measurement through the ordinary public watcher constructor.
- The native raw probe creates 240 files, waits for all distinct paths, records
  observed event count, public queue depth, throughput, and RSS growth.
- The native raw probe also records equivalent `std::filesystem::path`
  construction time for the observed event count so the API decision is tied to
  measured cost rather than speculation.
- The native settled probe creates 80 files under settle options and records
  delivered path count, pending settled-file work, and p50/p95 latency.
- Kernel-level `inotify` queue depth is not exposed by this probe; the evidence
  records that backend depth as unobservable instead of substituting the public
  delivery queue depth.
- Windows `ReadDirectoryChangesW` remains pending until a Windows lane is
  available; task 102 is complete for the current local evidence pass because
  the ticket explicitly scoped Windows to availability.

## Validation

```sh
cmake --build build-task100 --target ld_watch_performance_probe
timeout 30s build-task100/ld_watch_performance_probe
```

Observed output on 2026-09-06:

```text
watch.performance.simulated.raw.delivered=480
watch.performance.simulated.raw.throughput_events_per_second=159441
watch.performance.simulated.raw.max_queue_depth=352
watch.performance.simulated.raw.max_backend_depth=128
watch.performance.simulated.raw.rss_growth_kib=368
watch.performance.simulated.settled.delivered=96
watch.performance.simulated.settled.max_pending=24
watch.performance.simulated.settled.p50_latency_ms=0
watch.performance.simulated.settled.p95_latency_ms=1
watch.performance.inotify.raw.distinct_paths=240
watch.performance.inotify.raw.events_observed=718
watch.performance.inotify.raw.throughput_paths_per_second=16752.5
watch.performance.inotify.raw.max_queue_depth=259
watch.performance.inotify.raw.max_backend_depth=unobservable
watch.performance.inotify.raw.rss_growth_kib=440
watch.performance.inotify.raw.elapsed_us=14326
watch.performance.inotify.raw.equivalent_path_construction_us=561
watch.performance.inotify.settled.delivered=80
watch.performance.inotify.settled.max_pending=4
watch.performance.inotify.settled.p50_latency_ms=3
watch.performance.inotify.settled.p95_latency_ms=3
```

## Decision

Keep `std::filesystem::path` in public watcher values. Neither simulated nor
native Linux evidence shows path construction dominating watcher cost. Reopen
only from Windows native data or maintained-consumer evidence that identifies
path construction as the bottleneck.

## Evidence Fit

Performance evidence should catch this: the risk is optimizing API shape from a
speculative hot-path concern instead of measured backend behavior.

## Release Gate

Does not block `0.2.0`.
