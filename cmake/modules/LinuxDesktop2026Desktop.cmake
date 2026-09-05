add_library(ld_desktop
    src/durable_file_write.cpp
    src/desktop.cpp
    src/desktop_c.cpp
)

add_library(LinuxDesktop2026::ld_desktop ALIAS ld_desktop)

target_compile_features(ld_desktop PUBLIC cxx_std_17)

target_include_directories(ld_desktop
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(ld_desktop
    PUBLIC
        LinuxDesktop2026::ld_core
    PRIVATE
        LinuxDesktop2026::ld_paths
)
