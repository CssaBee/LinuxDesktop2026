find_program(LD2026_RUSTC rustc)

add_executable(ld_settings_tests
    tests/settings_tests.cpp
)
target_link_libraries(ld_settings_tests
    PRIVATE
        LinuxDesktop2026::ld_settings
        LinuxDesktop2026::ld_desktop
        LinuxDesktop2026::ld_migration
)

add_test(NAME ld_settings_tests COMMAND ld_settings_tests)

add_executable(ld_settings_c_tests
    tests/settings_c_tests.c
)
target_link_libraries(ld_settings_c_tests PRIVATE LinuxDesktop2026::ld_settings)

add_test(NAME ld_settings_c_tests COMMAND ld_settings_c_tests)

add_executable(ld_paths_tests
    tests/paths_tests.cpp
)
target_link_libraries(ld_paths_tests PRIVATE LinuxDesktop2026::ld_paths)

add_test(NAME ld_paths_tests COMMAND ld_paths_tests)

add_executable(ld_root_tests
    tests/root_tests.cpp
)
target_link_libraries(ld_root_tests PRIVATE LinuxDesktop2026::ld_root)

add_test(NAME ld_root_tests COMMAND ld_root_tests)

add_executable(ld_root_c_tests
    tests/root_c_tests.c
)
target_link_libraries(ld_root_c_tests PRIVATE LinuxDesktop2026::ld_root)

add_test(NAME ld_root_c_tests COMMAND ld_root_c_tests)

add_executable(ld_paths_c_tests
    tests/paths_c_tests.c
)
target_link_libraries(ld_paths_c_tests PRIVATE LinuxDesktop2026::ld_paths)

add_test(NAME ld_paths_c_tests COMMAND ld_paths_c_tests)

add_executable(ld_desktop_tests
    tests/desktop_tests.cpp
)
target_link_libraries(ld_desktop_tests PRIVATE LinuxDesktop2026::ld_desktop)

add_test(NAME ld_desktop_tests COMMAND ld_desktop_tests)

add_executable(ld_desktop_c_tests
    tests/desktop_c_tests.c
)
target_link_libraries(ld_desktop_c_tests PRIVATE LinuxDesktop2026::ld_desktop)

add_test(NAME ld_desktop_c_tests COMMAND ld_desktop_c_tests)

add_executable(ld_migration_tests
    tests/migration_tests.cpp
)
target_link_libraries(ld_migration_tests PRIVATE LinuxDesktop2026::ld_migration)

add_test(NAME ld_migration_tests COMMAND ld_migration_tests)

if(LD2026_RUSTC)
    set(LD2026_RUST_FFI_OBJECT "${CMAKE_CURRENT_BINARY_DIR}/settings_rust_ffi_smoke${CMAKE_C_OUTPUT_EXTENSION}")
    add_custom_command(
        OUTPUT "${LD2026_RUST_FFI_OBJECT}"
        COMMAND "${LD2026_RUSTC}"
            --edition=2021
            --crate-type staticlib
            --emit=obj
            -C panic=abort
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/settings_rust_ffi_smoke.rs"
            -o "${LD2026_RUST_FFI_OBJECT}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/settings_rust_ffi_smoke.rs"
        VERBATIM
    )
    add_executable(ld_settings_rust_ffi_smoke
        tests/settings_rust_ffi_smoke_driver.cpp
        "${LD2026_RUST_FFI_OBJECT}"
    )
    target_link_libraries(ld_settings_rust_ffi_smoke PRIVATE LinuxDesktop2026::ld_settings)
    add_test(NAME ld_settings_rust_ffi_smoke COMMAND ld_settings_rust_ffi_smoke)
endif()

add_executable(ld_watch_public_header_no_test_hooks
    tests/watch_public_header_no_test_hooks.cpp
)
target_link_libraries(ld_watch_public_header_no_test_hooks PRIVATE LinuxDesktop2026::ld_watch)

add_test(NAME ld_watch_public_header_no_test_hooks COMMAND ld_watch_public_header_no_test_hooks)
set_tests_properties(ld_watch_public_header_no_test_hooks PROPERTIES LABELS "watch;public-header")

if(LD2026_WATCH_ENABLE_TEST_HOOKS)
    add_executable(ld_watch_tests
        tests/watch_tests.cpp
    )
    target_include_directories(ld_watch_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(ld_watch_tests PRIVATE LINUXDESKTOP2026_WATCH_ENABLE_TEST_HOOKS)
    target_link_libraries(ld_watch_tests PRIVATE LinuxDesktop2026::ld_watch)

    add_test(NAME ld_watch_tests COMMAND ld_watch_tests)
    set_tests_properties(ld_watch_tests PROPERTIES LABELS "watch;adversarial;tsan")

    add_executable(ld_watch_performance_probe
        tests/watch_performance_probe.cpp
    )
    target_include_directories(ld_watch_performance_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(ld_watch_performance_probe PRIVATE LINUXDESKTOP2026_WATCH_ENABLE_TEST_HOOKS)
    target_link_libraries(ld_watch_performance_probe PRIVATE LinuxDesktop2026::ld_watch)

    add_test(NAME ld_watch_performance_probe COMMAND ld_watch_performance_probe)
    set_tests_properties(ld_watch_performance_probe PROPERTIES LABELS "watch;performance")
endif()

if(LD2026_ENABLE_COVERAGE)
    find_program(LD2026_GCOVR gcovr)
    if(LD2026_GCOVR)
        add_custom_target(ld2026_coverage
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/coverage
            COMMAND ${LD2026_GCOVR}
                --root ${CMAKE_CURRENT_SOURCE_DIR}
                --filter ${CMAKE_CURRENT_SOURCE_DIR}/src
                --filter ${CMAKE_CURRENT_SOURCE_DIR}/include
                --exclude ${CMAKE_CURRENT_SOURCE_DIR}/tests
                --html-details ${CMAKE_CURRENT_BINARY_DIR}/coverage/index.html
                --xml ${CMAKE_CURRENT_BINARY_DIR}/coverage/coverage.xml
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Running LinuxDesktop2026 tests and writing coverage/index.html plus coverage/coverage.xml"
            VERBATIM
        )
    else()
        message(WARNING "LD2026_ENABLE_COVERAGE is ON, but gcovr was not found; run ctest and gcov/gcovr manually from the build tree")
    endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT LD2026_WATCH_PREFER_LIBUV)
    add_executable(ld_watch_inotify_tests
        tests/watch_inotify_tests.cpp
    )
    target_link_libraries(ld_watch_inotify_tests PRIVATE LinuxDesktop2026::ld_watch)

    add_test(NAME ld_watch_inotify_tests COMMAND ld_watch_inotify_tests)
    set_tests_properties(ld_watch_inotify_tests PROPERTIES LABELS "watch;backend;linux")
endif()

if(WIN32)
    add_executable(ld_watch_windows_tests
        tests/watch_windows_tests.cpp
    )
    target_link_libraries(ld_watch_windows_tests PRIVATE LinuxDesktop2026::ld_watch)

    add_test(NAME ld_watch_windows_tests COMMAND ld_watch_windows_tests)
    set_tests_properties(ld_watch_windows_tests PROPERTIES LABELS "watch;backend;windows")
endif()

if(TARGET PkgConfig::LIBUV AND LD2026_WATCH_PREFER_LIBUV)
    add_executable(ld_watch_libuv_tests
        tests/watch_libuv_tests.cpp
    )
    target_link_libraries(ld_watch_libuv_tests PRIVATE LinuxDesktop2026::ld_watch)

    add_test(NAME ld_watch_libuv_tests COMMAND ld_watch_libuv_tests)
    set_tests_properties(ld_watch_libuv_tests PROPERTIES LABELS "watch;backend;libuv")
endif()

add_test(NAME ld_settings_install_tree_consumer
    COMMAND ${CMAKE_COMMAND}
        -DLD2026_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -DLD2026_BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -DLD2026_CONSUMER_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/install-tree-consumer
        -DLD2026_CONSUMER_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/consumer-build
        -DLD2026_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/consumer-prefix
        -DLD2026_PACKAGE_DIR=${CMAKE_CURRENT_BINARY_DIR}/consumer-prefix/${LD2026_INSTALL_CMAKEDIR}
        -DLD2026_CONSUMER_GENERATOR=${CMAKE_GENERATOR}
        -DLD2026_CONSUMER_C_COMPILER=${CMAKE_C_COMPILER}
        -DLD2026_CONSUMER_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DLD2026_CONSUMER_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}
        -DLD2026_CONSUMER_BUILD_CONFIG=$<CONFIG>
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/run-install-tree-consumer.cmake
)
