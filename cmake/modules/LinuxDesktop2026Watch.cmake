add_library(ld_watch
    src/watch.cpp
    src/watch_inotify.cpp
)

add_library(LinuxDesktop2026::ld_watch ALIAS ld_watch)

target_compile_features(ld_watch PUBLIC cxx_std_17)

target_include_directories(ld_watch
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(ld_watch PUBLIC LinuxDesktop2026::ld_core)

if(LD2026_WATCH_ENABLE_TEST_HOOKS)
    target_compile_definitions(ld_watch PRIVATE LINUXDESKTOP2026_WATCH_ENABLE_TEST_HOOKS)
endif()

if(NOT LD2026_WATCH_ENABLE_TEST_HOOKS)
    get_target_property(ld2026_watch_compile_definitions ld_watch COMPILE_DEFINITIONS)
    if(ld2026_watch_compile_definitions MATCHES "(^|;)LINUXDESKTOP2026_WATCH_ENABLE_TEST_HOOKS(;|$)")
        message(FATAL_ERROR "ld_watch received test-only hooks while LD2026_WATCH_ENABLE_TEST_HOOKS is OFF")
    endif()
endif()

if(WIN32)
    target_sources(ld_watch PRIVATE src/watch_windows.cpp)
endif()

if(LD2026_WATCH_PREFER_LIBUV AND NOT LD2026_WATCH_ENABLE_LIBUV)
    message(FATAL_ERROR "LD2026_WATCH_PREFER_LIBUV requires LD2026_WATCH_ENABLE_LIBUV=ON")
endif()

if(LD2026_WATCH_ENABLE_LIBUV)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBUV QUIET IMPORTED_TARGET libuv)
    endif()
    if(TARGET PkgConfig::LIBUV)
        target_sources(ld_watch PRIVATE src/watch_libuv.cpp)
        target_link_libraries(ld_watch PRIVATE PkgConfig::LIBUV)
        target_compile_definitions(ld_watch PRIVATE LINUXDESKTOP2026_WATCH_HAS_LIBUV)
        set(LD2026_CONFIG_HAS_LIBUV ON)
        if(LD2026_WATCH_PREFER_LIBUV)
            target_compile_definitions(ld_watch PRIVATE LINUXDESKTOP2026_WATCH_PREFER_LIBUV)
        endif()
    elseif(LD2026_WATCH_PREFER_LIBUV)
        message(FATAL_ERROR "LD2026_WATCH_PREFER_LIBUV requires libuv to be available through pkg-config")
    endif()
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(ld_watch PRIVATE pthread)
endif()
