# Validation Evidence

This page records local evidence for review-hardening checks that should stay
visible without turning `project-status.md` into a build log.

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
with the simulated backend and test hooks enabled. The probe is a guardrail, not
a portable benchmark suite.

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

Result on 2026-09-05 with GCC 13.3.0:

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

Decision: no evidence currently shows `std::filesystem::path` allocation as a
real hot-path problem for the bounded simulated workload. Keep the current path
value API; reopen only if a native-backend or maintained-consumer measurement
shows path construction dominating watcher cost.
