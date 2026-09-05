add_library(ld_migration
    src/migration_filesystem.cpp
    src/migration.cpp
    src/migration_planning.cpp
    src/migration_registry.cpp
)

add_library(LinuxDesktop2026::ld_migration ALIAS ld_migration)

target_compile_features(ld_migration PUBLIC cxx_std_17)

target_include_directories(ld_migration
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(ld_migration
    PUBLIC
        LinuxDesktop2026::ld_core
        LinuxDesktop2026::ld_paths
)

if(WIN32)
    target_link_libraries(ld_migration PRIVATE advapi32)
endif()
