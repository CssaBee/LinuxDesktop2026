cmake_minimum_required(VERSION 3.16)

project(LinuxDesktop2026 VERSION 0.1.0 LANGUAGES C CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(LD2026_BUILD_EXAMPLES "Build LinuxDesktop2026 examples" ON)
option(LD2026_BUILD_TESTS "Build LinuxDesktop2026 tests" ON)
option(LD2026_ENABLE_COVERAGE "Build LinuxDesktop2026 with local coverage instrumentation" OFF)
option(LD2026_WATCH_ENABLE_LIBUV "Enable the optional libuv watcher backend when libuv is available" ON)
option(LD2026_WATCH_PREFER_LIBUV "Prefer libuv over the native inotify backend for ld_watch" OFF)
option(LD2026_WATCH_ENABLE_TEST_HOOKS "Compile ld_watch test-only backend injection hooks" OFF)
set(LD2026_CONFIG_HAS_LIBUV OFF)
set(LD2026_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/LinuxDesktop2026"
    CACHE STRING "Install path for LinuxDesktop2026 CMake package files")

if(MSVC)
    add_compile_options(/W4 /permissive- /we4062)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wswitch-enum -Werror=switch-enum)
endif()

if(LD2026_ENABLE_COVERAGE)
    if(MSVC)
        message(FATAL_ERROR "LD2026_ENABLE_COVERAGE currently supports GCC/Clang style coverage flags only")
    endif()
    add_compile_options(-O0 -g --coverage)
    add_link_options(--coverage)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/LinuxDesktop2026Libraries.cmake")

install(TARGETS ld_core ld_desktop ld_migration ld_settings ld_paths ld_root ld_watch
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
    cmake/LinuxDesktop2026PathDefaults.cmake
    cmake/LinuxDesktop2026PathDefaults.hpp.in
    DESTINATION ${LD2026_INSTALL_CMAKEDIR}
)

install(EXPORT LinuxDesktop2026Targets
    NAMESPACE LinuxDesktop2026::
    DESTINATION ${LD2026_INSTALL_CMAKEDIR}
)

if(LD2026_BUILD_TESTS)
    enable_testing()
    include("${CMAKE_CURRENT_SOURCE_DIR}/tests/LinuxDesktop2026Tests.cmake")
endif()

if(LD2026_BUILD_EXAMPLES)
    include("${CMAKE_CURRENT_SOURCE_DIR}/examples/LinuxDesktop2026Examples.cmake")
endif()
