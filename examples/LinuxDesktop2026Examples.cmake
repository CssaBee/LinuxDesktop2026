add_executable(ld_settings_demo
    examples/settings_demo.cpp
)
target_link_libraries(ld_settings_demo PRIVATE LinuxDesktop2026::ld_settings LinuxDesktop2026::ld_migration)
target_compile_definitions(ld_settings_demo
    PRIVATE
        LD2026_EXAMPLE_MODEL_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/examples/settings-models"
)

add_executable(ld_watch_demo
    examples/watch_demo.cpp
)
target_link_libraries(ld_watch_demo PRIVATE LinuxDesktop2026::ld_watch)

add_executable(ld_paths_demo
    examples/paths_demo.cpp
)
target_link_libraries(ld_paths_demo PRIVATE LinuxDesktop2026::ld_paths)

add_executable(ld_root_demo
    examples/root_demo.cpp
)
target_link_libraries(ld_root_demo PRIVATE LinuxDesktop2026::ld_root)

add_executable(ld_paths_c_demo
    examples/paths_c_demo.c
)
target_link_libraries(ld_paths_c_demo PRIVATE LinuxDesktop2026::ld_paths)
