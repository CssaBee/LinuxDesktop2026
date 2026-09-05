# 81 - Add Coverage Failure-Mode And Watch Performance Evidence

**What to build:** Add evidence that tests exercise the risky states reviewers
keep finding late.

**Blocked by:** 62 - Bound And Coalesce Settled-File Readiness Work; 63 -
Replace Blocking Settle FIFO With Deadline Scheduler.

**Status:** implemented

- [x] Add coverage reporting or an equivalent local coverage artifact for
  active modules.
- [x] Add deterministic failure-mode tests for disk-full-like writes,
  permission denial, resource-limit paths, and cross-device-like behavior.
- [x] Add watcher burst/load measurements for event throughput, latency, queue
  depth, and memory growth.
- [x] Use the data to decide whether hot-path `std::filesystem::path`
  allocation is a real problem.

## Review Anchor

The broad review found no coverage reporting, no benchmarks, and limited
failure-mode evidence. It also raised `std::filesystem::path` hot-path costs as
speculative until profiling exists.

## Implementation Notes

- Added `LD2026_ENABLE_COVERAGE`. With GCC/Clang-style coverage and `gcovr`
  present, `ld2026_coverage` runs CTest and writes `coverage/index.html` plus
  `coverage/coverage.xml`. The local 2026-09-05 run proved the instrumented
  build path; `gcovr` was not installed here.
- Added deterministic write failure coverage in `ld_settings_tests` for
  file-as-directory parents, backup-copy cleanup, disk-full-like POSIX
  `RLIMIT_FSIZE` writes, permission-denied temp creation, and atomic replacement
  cleanup.
- Added `ld_watch_performance_probe`, gated behind
  `LD2026_WATCH_ENABLE_TEST_HOOKS`, to print raw throughput, queue depth,
  backend depth, RSS growth, settled-file pending work, and settled latency.
- Added test-only queued-depth introspection for the watcher. It remains behind
  the same public-header test-hook gate as the existing pending-settle counter.
- Current data does not justify changing the public watcher path value away
  from `std::filesystem::path`; reopen only from native-backend or maintained
  consumer measurements showing that path construction dominates cost.

## Validation

```sh
cmake -S . -B build-task81 \
  -DLD2026_BUILD_EXAMPLES=OFF \
  -DLD2026_WATCH_ENABLE_TEST_HOOKS=ON
cmake --build build-task81 \
  --target ld_settings_tests ld_watch_tests ld_watch_performance_probe
timeout 45s stdbuf -oL -eL build-task81/ld_settings_tests
timeout 45s build-task81/ld_watch_tests
timeout 20s build-task81/ld_watch_performance_probe
cmake -S . -B build-task81-coverage \
  -DLD2026_BUILD_EXAMPLES=OFF \
  -DLD2026_WATCH_ENABLE_TEST_HOOKS=ON \
  -DLD2026_ENABLE_COVERAGE=ON
cmake --build build-task81-coverage \
  --target ld_settings_tests ld_watch_tests ld_watch_performance_probe
```

Observed watcher probe output on 2026-09-05:

```text
watch.performance.raw.delivered=480
watch.performance.raw.throughput_events_per_second=174723
watch.performance.raw.max_queue_depth=362
watch.performance.raw.max_backend_depth=121
watch.performance.raw.rss_growth_kib=364
watch.performance.settled.delivered=96
watch.performance.settled.max_pending=19
watch.performance.settled.p50_latency_ms=0
watch.performance.settled.p95_latency_ms=1
```
