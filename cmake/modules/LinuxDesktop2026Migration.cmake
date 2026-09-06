add_library(ld_migration
    src/migration_filesystem.cpp
    src/migration.cpp
    src/migration_planning.cpp
    src/migration_registry.cpp
)

add_library(LinuxDesktop2026::ld_migration ALIAS ld_migration)

target_compile_features(ld_migration PUBLIC cxx_std_17)

set(LD2026_NLOHMANN_JSON_VERSION "3.11.3" CACHE STRING "nlohmann_json version used by ld_migration")
option(LD2026_MIGRATION_USE_SYSTEM_NLOHMANN_JSON
    "Prefer an installed nlohmann_json package for ld_migration" ON)

if(LD2026_MIGRATION_USE_SYSTEM_NLOHMANN_JSON)
    find_package(nlohmann_json ${LD2026_NLOHMANN_JSON_VERSION} CONFIG QUIET)
endif()

if(NOT TARGET nlohmann_json::nlohmann_json)
    include(FetchContent)
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    set(JSON_Install OFF CACHE INTERNAL "")
    FetchContent_Declare(nlohmann_json
        URL "https://github.com/nlohmann/json/releases/download/v${LD2026_NLOHMANN_JSON_VERSION}/json.tar.xz"
        URL_HASH "SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

target_include_directories(ld_migration
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

get_target_property(LD2026_NLOHMANN_JSON_INCLUDE_DIRS
    nlohmann_json::nlohmann_json INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(ld_migration PRIVATE ${LD2026_NLOHMANN_JSON_INCLUDE_DIRS})

target_link_libraries(ld_migration
    PUBLIC
        LinuxDesktop2026::ld_core
        LinuxDesktop2026::ld_paths
)

if(WIN32)
    target_link_libraries(ld_migration PRIVATE advapi32)
endif()
