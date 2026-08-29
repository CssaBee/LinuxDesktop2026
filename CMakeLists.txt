cmake_minimum_required(VERSION 3.16)

project(LinuxDesktop2026 VERSION 0.1.0 LANGUAGES C CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(LD2026_BUILD_EXAMPLES "Build LinuxDesktop2026 examples" ON)
option(LD2026_BUILD_TESTS "Build LinuxDesktop2026 tests" ON)
option(LD2026_WATCH_ENABLE_LIBUV "Enable the optional libuv watcher backend when libuv is available" ON)
option(LD2026_WATCH_PREFER_LIBUV "Prefer libuv over the native inotify backend for ld_watch" OFF)
set(LD2026_CONFIG_HAS_LIBUV OFF)
set(LD2026_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/LinuxDesktop2026"
    CACHE STRING "Install path for LinuxDesktop2026 CMake package files")

add_library(ld_core INTERFACE)
add_library(LinuxDesktop2026::ld_core ALIAS ld_core)

target_compile_features(ld_core INTERFACE cxx_std_17)

target_include_directories(ld_core
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

add_library(ld_desktop
    src/desktop.cpp
)

add_library(LinuxDesktop2026::ld_desktop ALIAS ld_desktop)

target_compile_features(ld_desktop PUBLIC cxx_std_17)

target_include_directories(ld_desktop
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(ld_desktop PUBLIC LinuxDesktop2026::ld_core)

add_library(ld_migration
    src/migration.cpp
)

add_library(LinuxDesktop2026::ld_migration ALIAS ld_migration)

target_compile_features(ld_migration PUBLIC cxx_std_17)

target_include_directories(ld_migration
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(ld_migration PUBLIC LinuxDesktop2026::ld_core)

if(WIN32)
    target_link_libraries(ld_migration PRIVATE advapi32)
endif()

add_library(ld_settings
    src/settings.cpp
    src/settings_c.cpp
)

add_library(LinuxDesktop2026::ld_settings ALIAS ld_settings)

target_compile_features(ld_settings PUBLIC cxx_std_17)

target_include_directories(ld_settings
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(ld_settings PUBLIC LinuxDesktop2026::ld_core LinuxDesktop2026::ld_desktop LinuxDesktop2026::ld_migration)

if(WIN32)
    target_link_libraries(ld_settings PRIVATE shell32 ole32)
endif()

add_library(ld_paths
    src/paths.cpp
    src/paths_c.cpp
)

add_library(LinuxDesktop2026::ld_paths ALIAS ld_paths)

target_compile_features(ld_paths PUBLIC cxx_std_17)

target_include_directories(ld_paths
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(ld_paths PUBLIC LinuxDesktop2026::ld_core)

target_link_libraries(ld_migration PUBLIC LinuxDesktop2026::ld_paths)
target_link_libraries(ld_desktop PRIVATE LinuxDesktop2026::ld_paths)
target_link_libraries(ld_settings PRIVATE LinuxDesktop2026::ld_paths)

add_library(ld_watch
    src/watch.cpp
    src/watch_inotify.cpp
)

add_library(LinuxDesktop2026::ld_watch ALIAS ld_watch)

target_compile_features(ld_watch PUBLIC cxx_std_17)

target_include_directories(ld_watch
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(ld_watch PUBLIC LinuxDesktop2026::ld_core)

if(WIN32)
    target_sources(ld_watch PRIVATE src/watch_windows.cpp)
endif()

if(LD2026_WATCH_PREFER_LIBUV AND NOT LD2026_WATCH_ENABLE_LIBUV)
    message(FATAL_ERROR "LD2026_WATCH_PREFER_LIBUV requires LD2026_WATCH_ENABLE_LIBUV=ON")
endif()

if(LD2026_WATCH_ENABLE_LIBUV)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBUV QUIET IMPORTED_TARGET libuv)
    endif()
    if(TARGET PkgConfig::LIBUV)
        target_sources(ld_watch PRIVATE src/watch_libuv.cpp)
        target_link_libraries(ld_watch PRIVATE PkgConfig::LIBUV)
        target_compile_definitions(ld_watch PRIVATE LINUXDESKTOP2026_WATCH_HAS_LIBUV)
        set(LD2026_CONFIG_HAS_LIBUV ON)
        if(LD2026_WATCH_PREFER_LIBUV)
            target_compile_definitions(ld_watch PRIVATE LINUXDESKTOP2026_WATCH_PREFER_LIBUV)
        endif()
    elseif(LD2026_WATCH_PREFER_LIBUV)
        message(FATAL_ERROR "LD2026_WATCH_PREFER_LIBUV requires libuv to be available through pkg-config")
    endif()
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(ld_watch PRIVATE pthread)
endif()

install(TARGETS ld_core ld_desktop ld_migration ld_settings ld_paths ld_watch
    EXPORT LinuxDesktop2026Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

configure_package_config_file(
    cmake/LinuxDesktop2026Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/LinuxDesktop2026Config.cmake"
    INSTALL_DESTINATION ${LD2026_INSTALL_CMAKEDIR}
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/LinuxDesktop2026ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/LinuxDesktop2026Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/LinuxDesktop2026ConfigVersion.cmake"
    DESTINATION ${LD2026_INSTALL_CMAKEDIR}
)

install(EXPORT LinuxDesktop2026Targets
    NAMESPACE LinuxDesktop2026::
    DESTINATION ${LD2026_INSTALL_CMAKEDIR}
)

if(LD2026_BUILD_TESTS)
    enable_testing()
    find_program(LD2026_RUSTC rustc)

    add_executable(ld_settings_tests
        tests/settings_tests.cpp
    )
    target_link_libraries(ld_settings_tests PRIVATE LinuxDesktop2026::ld_settings)

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

    add_executable(ld_watch_tests
        tests/watch_tests.cpp
    )
    target_include_directories(ld_watch_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(ld_watch_tests PRIVATE LinuxDesktop2026::ld_watch)

    add_test(NAME ld_watch_tests COMMAND ld_watch_tests)

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT LD2026_WATCH_PREFER_LIBUV)
        add_executable(ld_watch_inotify_tests
            tests/watch_inotify_tests.cpp
        )
        target_link_libraries(ld_watch_inotify_tests PRIVATE LinuxDesktop2026::ld_watch)

        add_test(NAME ld_watch_inotify_tests COMMAND ld_watch_inotify_tests)
    endif()

    if(WIN32)
        add_executable(ld_watch_windows_tests
            tests/watch_windows_tests.cpp
        )
        target_link_libraries(ld_watch_windows_tests PRIVATE LinuxDesktop2026::ld_watch)

        add_test(NAME ld_watch_windows_tests COMMAND ld_watch_windows_tests)
    endif()

    if(TARGET PkgConfig::LIBUV AND LD2026_WATCH_PREFER_LIBUV)
        add_executable(ld_watch_libuv_tests
            tests/watch_libuv_tests.cpp
        )
        target_link_libraries(ld_watch_libuv_tests PRIVATE LinuxDesktop2026::ld_watch)

        add_test(NAME ld_watch_libuv_tests COMMAND ld_watch_libuv_tests)
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
            -DLD2026_CONSUMER_BUILD_CONFIG=$<CONFIG>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/run-install-tree-consumer.cmake
    )
endif()

if(LD2026_BUILD_EXAMPLES)
    add_executable(ld_settings_demo
        examples/settings_demo.cpp
    )
    target_link_libraries(ld_settings_demo PRIVATE LinuxDesktop2026::ld_settings)
    target_compile_definitions(ld_settings_demo
        PRIVATE
            LD2026_EXAMPLE_MODEL_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/examples/settings-models"
    )

    add_executable(ld_watch_demo
        examples/watch_demo.cpp
    )
    target_link_libraries(ld_watch_demo PRIVATE LinuxDesktop2026::ld_watch)

    add_executable(ld_paths_demo
        examples/paths_demo.cpp
    )
    target_link_libraries(ld_paths_demo PRIVATE LinuxDesktop2026::ld_paths)

    add_executable(ld_paths_c_demo
        examples/paths_c_demo.c
    )
    target_link_libraries(ld_paths_c_demo PRIVATE LinuxDesktop2026::ld_paths)
endif()
