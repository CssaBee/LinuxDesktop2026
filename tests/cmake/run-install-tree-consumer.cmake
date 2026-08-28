if(NOT DEFINED LD2026_BUILD_DIR)
    message(FATAL_ERROR "LD2026_BUILD_DIR is required")
endif()

if(NOT DEFINED LD2026_CONSUMER_SOURCE_DIR)
    message(FATAL_ERROR "LD2026_CONSUMER_SOURCE_DIR is required")
endif()

if(NOT DEFINED LD2026_CONSUMER_BINARY_DIR)
    message(FATAL_ERROR "LD2026_CONSUMER_BINARY_DIR is required")
endif()

if(NOT DEFINED LD2026_INSTALL_PREFIX)
    message(FATAL_ERROR "LD2026_INSTALL_PREFIX is required")
endif()

if(NOT DEFINED LD2026_PACKAGE_DIR)
    message(FATAL_ERROR "LD2026_PACKAGE_DIR is required")
endif()

file(REMOVE_RECURSE "${LD2026_CONSUMER_BINARY_DIR}" "${LD2026_INSTALL_PREFIX}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${LD2026_BUILD_DIR}" --prefix "${LD2026_INSTALL_PREFIX}"
    RESULT_VARIABLE install_result
)
if(NOT install_result EQUAL 0)
    message(FATAL_ERROR "Installing LinuxDesktop2026 failed with ${install_result}")
endif()

if(NOT EXISTS "${LD2026_PACKAGE_DIR}/LinuxDesktop2026Config.cmake")
    message(FATAL_ERROR "Installed package config was not found at ${LD2026_PACKAGE_DIR}")
endif()

set(ld2026_package_dir_arg "-DLinuxDesktop2026_DIR=${LD2026_PACKAGE_DIR}")
execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -S "${LD2026_CONSUMER_SOURCE_DIR}"
        -B "${LD2026_CONSUMER_BINARY_DIR}"
        "${ld2026_package_dir_arg}"
    RESULT_VARIABLE configure_result
)
if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR "Configuring install-tree consumer failed with ${configure_result}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${LD2026_CONSUMER_BINARY_DIR}"
    RESULT_VARIABLE build_result
)
if(NOT build_result EQUAL 0)
    message(FATAL_ERROR "Building install-tree consumer failed with ${build_result}")
endif()

execute_process(
    COMMAND "${LD2026_CONSUMER_BINARY_DIR}/ld_settings_consumer"
    RESULT_VARIABLE run_result
)
if(NOT run_result EQUAL 0)
    message(FATAL_ERROR "Running install-tree consumer failed with ${run_result}")
endif()

execute_process(
    COMMAND "${LD2026_CONSUMER_BINARY_DIR}/ld_paths_c_consumer"
    RESULT_VARIABLE run_paths_c_result
)
if(NOT run_paths_c_result EQUAL 0)
    message(FATAL_ERROR "Running C ld_paths install-tree consumer failed with ${run_paths_c_result}")
endif()
