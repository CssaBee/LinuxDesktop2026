cmake_minimum_required(VERSION 3.16)

project(LinuxDesktop2026InstallTreeConsumer LANGUAGES C CXX)

find_package(LinuxDesktop2026 CONFIG REQUIRED)

add_executable(ld_settings_consumer main.cpp)
target_link_libraries(ld_settings_consumer
    PRIVATE
        LinuxDesktop2026::ld_core
        LinuxDesktop2026::ld_desktop
        LinuxDesktop2026::ld_settings
        LinuxDesktop2026::ld_paths
        LinuxDesktop2026::ld_watch
)

add_executable(ld_paths_c_consumer paths_c_main.c)
target_link_libraries(ld_paths_c_consumer
    PRIVATE
        LinuxDesktop2026::ld_paths
)
