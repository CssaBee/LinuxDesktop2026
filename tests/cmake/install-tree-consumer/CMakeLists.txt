cmake_minimum_required(VERSION 3.16)

project(LinuxDesktop2026InstallTreeConsumer LANGUAGES C CXX)

if(DEFINED LD2026_FIND_PACKAGE_VERSION AND NOT LD2026_FIND_PACKAGE_VERSION STREQUAL "")
    find_package(LinuxDesktop2026 ${LD2026_FIND_PACKAGE_VERSION} CONFIG REQUIRED)
else()
    find_package(LinuxDesktop2026 CONFIG REQUIRED)
endif()

if(LinuxDesktop2026_VERSION VERSION_LESS 1.0.0)
    if(NOT LinuxDesktop2026_PACKAGE_VERSION_COMPATIBILITY STREQUAL "ExactVersion")
        message(FATAL_ERROR "0.x LinuxDesktop2026 packages must advertise ExactVersion compatibility")
    endif()
    if(NOT LinuxDesktop2026_PRE_1_0_EXACT_VERSION_REQUIRED)
        message(FATAL_ERROR "0.x LinuxDesktop2026 packages must advertise exact pre-1.0 version matching")
    endif()
else()
    if(NOT LinuxDesktop2026_PACKAGE_VERSION_COMPATIBILITY STREQUAL "SameMajorVersion")
        message(FATAL_ERROR "1.x+ LinuxDesktop2026 packages should advertise SameMajorVersion compatibility")
    endif()
    if(LinuxDesktop2026_PRE_1_0_EXACT_VERSION_REQUIRED)
        message(FATAL_ERROR "1.x+ LinuxDesktop2026 packages must not advertise pre-1.0 exact matching")
    endif()
endif()

get_target_property(ld2026_settings_interface LinuxDesktop2026::ld_settings INTERFACE_LINK_LIBRARIES)
if(ld2026_settings_interface MATCHES "(^|;)LinuxDesktop2026::ld_desktop(;|$)")
    message(FATAL_ERROR "LinuxDesktop2026::ld_settings must not expose LinuxDesktop2026::ld_desktop")
endif()

add_executable(ld_settings_consumer main.cpp)
target_link_libraries(ld_settings_consumer
    PRIVATE
        LinuxDesktop2026::ld_settings
)

add_executable(ld_paths_consumer paths_main.cpp)
target_link_libraries(ld_paths_consumer
    PRIVATE
        LinuxDesktop2026::ld_paths
)
linuxdesktop2026_generate_path_defaults(ld_paths_consumer
    HEADER linuxdesktop2026_install_tree/path_defaults.hpp)

add_executable(ld_paths_c_consumer paths_c_main.c)
target_link_libraries(ld_paths_c_consumer
    PRIVATE
        LinuxDesktop2026::ld_paths
)

add_executable(ld_root_consumer root_main.cpp)
target_link_libraries(ld_root_consumer
    PRIVATE
        LinuxDesktop2026::ld_root
)
