add_library(ld_paths
    src/paths.cpp
    src/paths_c.cpp
)

add_library(LinuxDesktop2026::ld_paths ALIAS ld_paths)

target_compile_features(ld_paths PUBLIC cxx_std_17)

target_include_directories(ld_paths
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(ld_paths PUBLIC LinuxDesktop2026::ld_core)
