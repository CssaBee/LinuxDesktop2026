# 80 - Decompose Root CMake Before Next Module Wave

**What to build:** Split build definitions by module before the top-level CMake
file becomes the architecture registry.

**Blocked by:** None.

**Status:** implemented

- [x] Move library target definitions into module-focused CMake files.
- [x] Move tests into a focused tests CMake file.
- [x] Keep package/export orchestration and global options at the repository
  root.
- [x] Preserve install-tree consumer behavior and CI target names.

## Review Anchor

The newer review says the root CMake file is manageable now, but will become a
maintenance problem if module count or test count keeps growing in one file.

## Validation

- `cmake -S . -B build-task80 -DLD2026_WATCH_ENABLE_TEST_HOOKS=ON`
- `cmake --build build-task80 --target ld_settings_tests ld_settings_c_tests ld_paths_tests ld_paths_c_tests ld_root_tests ld_root_c_tests ld_desktop_tests ld_desktop_c_tests ld_migration_tests ld_watch_public_header_no_test_hooks ld_watch_tests ld_watch_performance_probe ld_watch_inotify_tests ld_settings_demo ld_watch_demo ld_paths_demo ld_root_demo ld_paths_c_demo`
- `ctest --test-dir build-task80 -N`
- `timeout 60s ctest --test-dir build-task80 -R '^ld_settings_install_tree_consumer$' --output-on-failure`
- `timeout 30s ctest --test-dir build-task80 -R '^ld_watch_public_header_no_test_hooks$|^ld_watch_inotify_tests$' --output-on-failure`
- `cmake -S . -B build-task80-tests-off -DLD2026_BUILD_TESTS=OFF -DLD2026_BUILD_EXAMPLES=OFF`
- `cmake -S . -B build-task80-coverage -DLD2026_BUILD_EXAMPLES=OFF -DLD2026_WATCH_ENABLE_TEST_HOOKS=ON -DLD2026_ENABLE_COVERAGE=ON`
- `git diff --check`
