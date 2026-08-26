cmake_minimum_required(VERSION 3.16)

project(LinuxDesktop2026 VERSION 0.1.0 LANGUAGES CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(LD2026_BUILD_EXAMPLES "Build LinuxDesktop2026 examples" ON)
option(LD2026_BUILD_TESTS "Build LinuxDesktop2026 tests" ON)
set(LD2026_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/LinuxDesktop2026"
    CACHE STRING "Install path for LinuxDesktop2026 CMake package files")

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

install(TARGETS ld_settings
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

    add_executable(ld_settings_tests
        tests/settings_tests.cpp
    )
    target_link_libraries(ld_settings_tests PRIVATE LinuxDesktop2026::ld_settings)

    add_test(NAME ld_settings_tests COMMAND ld_settings_tests)
    add_test(NAME ld_settings_install_tree_consumer
        COMMAND ${CMAKE_COMMAND}
            -DLD2026_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
            -DLD2026_BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -DLD2026_CONSUMER_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/install-tree-consumer
            -DLD2026_CONSUMER_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/consumer-build
            -DLD2026_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/consumer-prefix
            -DLD2026_PACKAGE_DIR=${CMAKE_CURRENT_BINARY_DIR}/consumer-prefix/${LD2026_INSTALL_CMAKEDIR}
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
endif()
