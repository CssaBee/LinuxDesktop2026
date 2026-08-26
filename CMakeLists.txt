cmake_minimum_required(VERSION 3.16)

project(LinuxDesktop2026 VERSION 0.1.0 LANGUAGES CXX)

option(LD2026_BUILD_EXAMPLES "Build LinuxDesktop2026 examples" ON)
option(LD2026_BUILD_TESTS "Build LinuxDesktop2026 tests" ON)

add_library(ld_settings
    src/settings.cpp
)

add_library(LinuxDesktop2026::ld_settings ALIAS ld_settings)

target_compile_features(ld_settings PUBLIC cxx_std_17)

target_include_directories(ld_settings
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(WIN32)
    target_link_libraries(ld_settings PRIVATE shell32 ole32)
endif()

if(LD2026_BUILD_TESTS)
    enable_testing()

    add_executable(ld_settings_tests
        tests/settings_tests.cpp
    )
    target_link_libraries(ld_settings_tests PRIVATE LinuxDesktop2026::ld_settings)

    add_test(NAME ld_settings_tests COMMAND ld_settings_tests)
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
endif()
