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
set(ld2026_configure_args
    "${CMAKE_COMMAND}"
    -S "${LD2026_CONSUMER_SOURCE_DIR}"
    -B "${LD2026_CONSUMER_BINARY_DIR}"
    "${ld2026_package_dir_arg}"
)
if(DEFINED LD2026_CONSUMER_GENERATOR AND NOT LD2026_CONSUMER_GENERATOR STREQUAL "")
    list(APPEND ld2026_configure_args -G "${LD2026_CONSUMER_GENERATOR}")
endif()
if(DEFINED LD2026_CONSUMER_C_COMPILER AND NOT LD2026_CONSUMER_C_COMPILER STREQUAL "")
    list(APPEND ld2026_configure_args "-DCMAKE_C_COMPILER=${LD2026_CONSUMER_C_COMPILER}")
endif()
if(DEFINED LD2026_CONSUMER_CXX_COMPILER AND NOT LD2026_CONSUMER_CXX_COMPILER STREQUAL "")
    list(APPEND ld2026_configure_args "-DCMAKE_CXX_COMPILER=${LD2026_CONSUMER_CXX_COMPILER}")
endif()
if(DEFINED LD2026_CONSUMER_EXE_LINKER_FLAGS AND NOT LD2026_CONSUMER_EXE_LINKER_FLAGS STREQUAL "")
    list(APPEND ld2026_configure_args "-DCMAKE_EXE_LINKER_FLAGS=${LD2026_CONSUMER_EXE_LINKER_FLAGS}")
endif()

execute_process(
    COMMAND ${ld2026_configure_args}
    RESULT_VARIABLE configure_result
)
if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR "Configuring install-tree consumer failed with ${configure_result}")
endif()

set(ld2026_build_args "${CMAKE_COMMAND}" --build "${LD2026_CONSUMER_BINARY_DIR}")
if(DEFINED LD2026_CONSUMER_BUILD_CONFIG AND NOT LD2026_CONSUMER_BUILD_CONFIG STREQUAL "")
    list(APPEND ld2026_build_args --config "${LD2026_CONSUMER_BUILD_CONFIG}")
endif()
execute_process(
    COMMAND ${ld2026_build_args}
    RESULT_VARIABLE build_result
)
if(NOT build_result EQUAL 0)
    message(FATAL_ERROR "Building install-tree consumer failed with ${build_result}")
endif()

set(ld2026_consumer_runtime_dir "${LD2026_CONSUMER_BINARY_DIR}")
if(DEFINED LD2026_CONSUMER_BUILD_CONFIG AND NOT LD2026_CONSUMER_BUILD_CONFIG STREQUAL "")
    set(ld2026_candidate_runtime_dir "${LD2026_CONSUMER_BINARY_DIR}/${LD2026_CONSUMER_BUILD_CONFIG}")
    if(EXISTS "${ld2026_candidate_runtime_dir}")
        set(ld2026_consumer_runtime_dir "${ld2026_candidate_runtime_dir}")
    endif()
endif()

set(ld2026_executable_suffix "${CMAKE_EXECUTABLE_SUFFIX}")

execute_process(
    COMMAND "${ld2026_consumer_runtime_dir}/ld_settings_consumer${ld2026_executable_suffix}"
    RESULT_VARIABLE run_result
)
if(NOT run_result EQUAL 0)
    message(FATAL_ERROR "Running install-tree consumer failed with ${run_result}")
endif()

execute_process(
    COMMAND "${ld2026_consumer_runtime_dir}/ld_paths_c_consumer${ld2026_executable_suffix}"
    RESULT_VARIABLE run_paths_c_result
)
if(NOT run_paths_c_result EQUAL 0)
    message(FATAL_ERROR "Running C ld_paths install-tree consumer failed with ${run_paths_c_result}")
endif()
