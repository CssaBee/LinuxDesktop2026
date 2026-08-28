cmake_minimum_required(VERSION 3.16)

project(LinuxDesktop2026InstallTreeConsumer LANGUAGES CXX)

find_package(LinuxDesktop2026 CONFIG REQUIRED)

add_executable(ld_settings_consumer main.cpp)
target_link_libraries(ld_settings_consumer
    PRIVATE
        LinuxDesktop2026::ld_core
        LinuxDesktop2026::ld_settings
        LinuxDesktop2026::ld_watch
)
