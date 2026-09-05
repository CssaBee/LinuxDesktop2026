add_library(ld_root
    src/root.cpp
    src/root_c.cpp
)

add_library(LinuxDesktop2026::ld_root ALIAS ld_root)

target_compile_features(ld_root PUBLIC cxx_std_17)

target_include_directories(ld_root
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(ld_root PUBLIC LinuxDesktop2026::ld_core LinuxDesktop2026::ld_paths)
