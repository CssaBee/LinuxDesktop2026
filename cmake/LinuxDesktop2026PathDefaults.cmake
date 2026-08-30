set(LinuxDesktop2026_PATH_DEFAULTS_MODULE_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(linuxdesktop2026_generate_path_defaults target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "linuxdesktop2026_generate_path_defaults target '${target}' does not exist")
    endif()

    set(one_value_args HEADER)
    cmake_parse_arguments(LD2026_PATH_DEFAULTS "" "${one_value_args}" "" ${ARGN})
    if(LD2026_PATH_DEFAULTS_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "linuxdesktop2026_generate_path_defaults got unexpected arguments: "
            "${LD2026_PATH_DEFAULTS_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT LD2026_PATH_DEFAULTS_HEADER)
        set(LD2026_PATH_DEFAULTS_HEADER "linuxdesktop2026/generated/platform_path_defaults.hpp")
    endif()
    if(IS_ABSOLUTE "${LD2026_PATH_DEFAULTS_HEADER}")
        message(FATAL_ERROR "linuxdesktop2026_generate_path_defaults HEADER must be relative")
    endif()

    set(ld2026_path_defaults_output_root
        "${CMAKE_CURRENT_BINARY_DIR}/linuxdesktop2026-path-defaults/${target}")
    set(ld2026_path_defaults_output
        "${ld2026_path_defaults_output_root}/${LD2026_PATH_DEFAULTS_HEADER}")

    if(WIN32)
        set(LD2026_PATH_DEFAULTS_RUNTIME_UNUSED "    (void)runtime;")
        set(LD2026_PATH_DEFAULTS_FACTORY
            "    return ::linuxdesktop::paths::platform_path_defaults::windows(home);")
    else()
        set(LD2026_PATH_DEFAULTS_RUNTIME_UNUSED "")
        set(LD2026_PATH_DEFAULTS_FACTORY
            "    return ::linuxdesktop::paths::platform_path_defaults::xdg(home, std::move(runtime));")
    endif()

    configure_file(
        "${LinuxDesktop2026_PATH_DEFAULTS_MODULE_DIR}/LinuxDesktop2026PathDefaults.hpp.in"
        "${ld2026_path_defaults_output}"
        @ONLY)

    target_include_directories("${target}" PRIVATE "${ld2026_path_defaults_output_root}")
endfunction()
