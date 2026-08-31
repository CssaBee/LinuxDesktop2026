cmake_minimum_required(VERSION 3.16)

project(FlavorTests LANGUAGES C CXX)

include(CTest)

set(FLAVORTESTS_LD2026_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." CACHE PATH
    "Path to the LinuxDesktop2026 root source tree")
set(LD2026_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LD2026_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory("${FLAVORTESTS_LD2026_ROOT}" linuxdesktop2026-build EXCLUDE_FROM_ALL)
include("${FLAVORTESTS_LD2026_ROOT}/cmake/LinuxDesktop2026PathDefaults.cmake")

function(add_flavor_product PRODUCT)
    add_library(${PRODUCT}_flavor_support STATIC
        ${PRODUCT}/src/${PRODUCT}_flavor.cpp
    )

    target_include_directories(${PRODUCT}_flavor_support
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/${PRODUCT}/src
    )

    target_compile_features(${PRODUCT}_flavor_support PUBLIC cxx_std_17)
    target_link_libraries(${PRODUCT}_flavor_support
        PUBLIC
            LinuxDesktop2026::ld_paths
            LinuxDesktop2026::ld_root
            LinuxDesktop2026::ld_migration
            LinuxDesktop2026::ld_settings
    )
    linuxdesktop2026_generate_path_defaults(
        ${PRODUCT}_flavor_support
        HEADER ${PRODUCT}/generated/platform_path_defaults.hpp)

    add_executable(${PRODUCT}_flavor_tests
        ${PRODUCT}/test/${PRODUCT}_flavor_tests.cpp
    )

    target_compile_features(${PRODUCT}_flavor_tests PUBLIC cxx_std_17)
    target_link_libraries(${PRODUCT}_flavor_tests
        PRIVATE
            ${PRODUCT}_flavor_support
    )

    add_test(NAME ${PRODUCT}_flavor_tests COMMAND ${PRODUCT}_flavor_tests)
endfunction()

add_flavor_product(notepadpp)
add_flavor_product(prusaslicer)
add_flavor_product(openrgb)
add_flavor_product(keepassxc)
add_flavor_product(qbittorrent)
add_flavor_product(obs)
add_flavor_product(kicad)
add_flavor_product(audacity)
add_flavor_product(freecad)
add_flavor_product(walnut)
add_flavor_product(openipc_dashboard)
