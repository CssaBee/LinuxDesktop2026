# Validation Evidence

This page records local validation evidence that should stay visible without
turning `project-status.md` into a build log.

## Coverage

Status: CI now runs coverage instrumentation on Ubuntu/GCC and uploads the
generated HTML/XML report artifact.

Local command:

```sh
cmake -S . -B build-task81-coverage \
  -DLD2026_BUILD_EXAMPLES=OFF \
  -DLD2026_WATCH_ENABLE_TEST_HOOKS=ON \
  -DLD2026_ENABLE_COVERAGE=ON
cmake --build build-task81-coverage \
  --target ld_settings_tests ld_watch_tests ld_watch_performance_probe
```

CI command:

```sh
sudo apt-get update && sudo apt-get install -y gcovr ninja-build
cmake -S . -B build-coverage -G Ninja \
  -DLD2026_BUILD_TESTS=ON \
  -DLD2026_BUILD_EXAMPLES=OFF \
  -DLD2026_WATCH_ENABLE_TEST_HOOKS=ON \
  -DLD2026_ENABLE_COVERAGE=ON \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=gcc \
  -DCMAKE_CXX_COMPILER=g++
cmake --build build-coverage
cmake --build build-coverage --target ld2026_coverage
```

The `.github/workflows/ci.yml` coverage job uploads
`ld2026-coverage-report`, containing `coverage/index.html` and
`coverage/coverage.xml` from the `build-coverage` tree.

Intentional exclusions: examples are not built in the coverage lane, gcovr
filters the report to `src/` and `include/` while excluding `tests/`, and
`ld2026_coverage` skips `ld_settings_install_tree_consumer` because that test
validates installed package consumption rather than source-line coverage.
FlavorTest candidate projects, sanitizer lanes, the private Notepad++ proof
workflow, and watcher performance measurements remain separate validation
signals rather than coverage inputs.

## Failure Modes

Status: deterministic write failure-mode tests are part of
`ld_settings_tests`.

Covered states:

- file-as-directory parent rejection;
- backup-copy failure cleanup;
- disk-full-like direct write failure through POSIX `RLIMIT_FSIZE` in a child
  process;
- permission-denied temporary file creation before atomic replacement;
- atomic replacement failure cleanup when the destination is a directory.

Local command:

```sh
cmake --build build-task81 --target ld_settings_tests
timeout 45s stdbuf -oL -eL build-task81/ld_settings_tests
```

Result on 2026-09-05: passed.

## Watch Performance

Status: `ld_watch_performance_probe` records bounded local watcher behavior
with the simulated backend and test hooks enabled. On Linux it also runs an
equivalent native `inotify` measurement through the ordinary `ld::watcher`
constructor. The probe is a guardrail, not a portable benchmark suite.

Local command:

```sh
cmake -S . -B build-task81 \
  -DLD2026_BUILD_EXAMPLES=OFF \
  -DLD2026_WATCH_ENABLE_TEST_HOOKS=ON
cmake --build build-task81 \
  --target ld_watch_tests ld_watch_performance_probe
timeout 45s build-task81/ld_watch_tests
timeout 20s build-task81/ld_watch_performance_probe
```

Simulated-backend result on 2026-09-05 with GCC 13.3.0:

```text
watch.performance.simulated.raw.delivered=480
watch.performance.simulated.raw.throughput_events_per_second=174723
watch.performance.simulated.raw.max_queue_depth=362
watch.performance.simulated.raw.max_backend_depth=121
watch.performance.simulated.raw.rss_growth_kib=364
watch.performance.simulated.settled.delivered=96
watch.performance.simulated.settled.max_pending=19
watch.performance.simulated.settled.p50_latency_ms=0
watch.performance.simulated.settled.p95_latency_ms=1
```

Native Linux `inotify` result on 2026-09-06 with GCC 13.3.0:

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
watch.performance.inotify.raw.overflow_events=0
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

The native raw measurement waits for 240 distinct file paths and observes 718
events because `inotify` can report multiple create/write state transitions per
path. Kernel queue depth is not exposed by this local probe, so only
LinuxDesktop2026's public delivery queue depth is recorded. Native raw queue
overflow is recorded as a portability signal; strict distinct-path coverage is
required only when the public queue did not overflow under runner scheduling.
Equivalent
construction of the same count of `std::filesystem::path` values took 561 us
against 14326 us for the raw native measurement, so path construction did not
dominate measured cost.

Windows `ReadDirectoryChangesW` measurement has not run because no Windows lane
was available on 2026-09-06.

Decision: native Linux data still does not show `std::filesystem::path`
construction dominating watcher cost. Keep the current path value API; reopen
only if Windows native-backend data or maintained-consumer measurement shows
path construction as the bottleneck.
