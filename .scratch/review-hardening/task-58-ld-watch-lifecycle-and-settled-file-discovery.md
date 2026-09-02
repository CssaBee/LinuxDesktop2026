# Task 58 - ld_watch Lifecycle And Settled-File Discovery

Task 58 ran the focused watcher discovery pass after the callback
self-destruction and settle-worker fixes from tasks 56 and 57.

## Scope

- Treat **Watcher lifecycle** as the caller-visible lifetime rules for
  `ld_watch` watchers: start, callback delivery, callback mutation, watch
  removal, stop, destruction, worker shutdown, and post-stop behavior.
- Treat **Settled-file readiness** as the opt-in delayed delivery behavior for
  file create, modify, and rename-new events after debounce and stability
  checks, or after diagnostic timeout.
- Run deterministic simulated-backend coverage, native Linux `inotify` smoke
  coverage, and libuv-preferred coverage when available.
- Keep broader CI and ThreadSanitizer expansion outside this task.

## Commands

```sh
pkg-config --exists libuv
cmake -S . -B build-task58 -G Ninja -DLD2026_BUILD_TESTS=ON -DLD2026_BUILD_EXAMPLES=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-task58 --target ld_watch_tests ld_watch_inotify_tests
ctest --test-dir build-task58 -R ld_watch --output-on-failure
cmake -S . -B build-task58-libuv -G Ninja -DLD2026_BUILD_TESTS=ON -DLD2026_BUILD_EXAMPLES=ON -DLD2026_WATCH_PREFER_LIBUV=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-task58-libuv --target ld_watch_tests ld_watch_libuv_tests
ctest --test-dir build-task58-libuv -R ld_watch --output-on-failure
```

## Results

- Local libuv development files are available through `pkg-config`.
- `build-task58` configured and built `ld_watch_tests` and
  `ld_watch_inotify_tests`.
- `ctest --test-dir build-task58 -R ld_watch --output-on-failure` passed:
  `ld_watch_tests` and `ld_watch_inotify_tests`.
- `build-task58-libuv` configured with `LD2026_WATCH_PREFER_LIBUV=ON` and
  built `ld_watch_tests` and `ld_watch_libuv_tests`.
- `ctest --test-dir build-task58-libuv -R ld_watch --output-on-failure`
  passed: `ld_watch_tests` and `ld_watch_libuv_tests`.

## Findings

- No watcher lifecycle or settled-file readiness runtime failure reproduced
  locally.
- Existing simulated-backend tests already cover callback delivery, callback
  stop/remove, last-facade release from inside callback, callback replacement,
  callback exceptions, pull queue overflow, start failure diagnostics,
  resource-limit event diagnostics, post-stop wait wakeup, settled-file
  readiness, raw event delivery while settle work is pending, coalescing,
  timeout diagnostics, remove-watch cancellation, stale generation handling,
  and large settled-file batches.
- Native Linux `inotify` smoke coverage passed in the focused build.
- Libuv-preferred watcher coverage passed when configured locally.
- The focused builds emit repeated `-Wmissing-field-initializers` warnings in
  `tests/watch_tests.cpp` where settled-file options omit `timeout_after`.
  This is not a runtime failure, but it is noisy hardening-lane evidence and
  should be cleaned separately.

## Follow-Up

- Add a non-blocking follow-up ticket after task 60 to remove the settled-file
  option aggregate-initializer warnings from watcher tests.

No ADR is needed for this pass. The accepted lifecycle and settled-file terms
are glossary-level language, and the test results did not force a hard-to-reverse
API trade-off.
