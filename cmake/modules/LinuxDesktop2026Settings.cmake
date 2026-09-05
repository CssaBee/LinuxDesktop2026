add_library(ld_settings
    src/durable_file_write.cpp
    src/settings.cpp
    src/settings_c.cpp
    src/settings_roots.cpp
)

add_library(LinuxDesktop2026::ld_settings ALIAS ld_settings)

target_compile_features(ld_settings PUBLIC cxx_std_17)

target_include_directories(ld_settings
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(ld_settings PUBLIC LinuxDesktop2026::ld_core LinuxDesktop2026::ld_root)

if(WIN32)
    target_link_libraries(ld_settings PRIVATE shell32 ole32)
endif()
