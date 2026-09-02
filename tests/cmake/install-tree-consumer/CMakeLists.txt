cmake_minimum_required(VERSION 3.16)

project(LinuxDesktop2026InstallTreeConsumer LANGUAGES C CXX)

find_package(LinuxDesktop2026 CONFIG REQUIRED)

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
