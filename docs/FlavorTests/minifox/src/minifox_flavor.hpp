#pragma once

#include "linuxdesktop/paths.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace flavor_tests::minifox {

enum class DiagnosticCode {
    PathResolutionWarning,
    MissingComfyUiMain,
    MissingPythonExecutable
};

enum class RuntimeKind {
    Cuda,
    Rocm,
    HipSdk,
    Zluda
};

struct PortablePackageEnvironment {
    std::filesystem::path executable_directory;
    std::optional<std::filesystem::path> home_directory;
    std::optional<std::filesystem::path> runtime_directory;
    std::map<std::string, std::string> environment;
    bool cuda_available = false;
    bool rocm_available = false;
    bool amd_gpu_with_cuda_torch = false;
    bool bundled_zluda_available = false;
};

struct LaunchProfile {
    std::string name = "default";
    std::optional<std::filesystem::path> comfyui_path;
    std::optional<std::filesystem::path> python_path;
    std::vector<std::string> arguments;
    bool request_zluda = false;
};

struct ToolCandidate {
    std::string name;
    linuxdesktop::paths::candidate_source source = linuxdesktop::paths::candidate_source::executable_relative;
    std::filesystem::path path;
    bool selected = false;
};

struct LaunchDiagnostic {
    DiagnosticCode code = DiagnosticCode::PathResolutionWarning;
    std::string message;
    std::filesystem::path path;
    bool fatal = false;
};

struct RuntimeDiagnostic {
    RuntimeKind kind = RuntimeKind::Cuda;
    std::string message;
    bool eligible = false;
};

struct ReversibleRuntimeAction {
    std::string name;
    std::filesystem::path source;
    std::filesystem::path target;
    std::filesystem::path restore_from;
};

struct LaunchPlan {
    std::filesystem::path working_directory;
    std::filesystem::path application_settings_file;
    std::filesystem::path profile_settings_file;
    std::filesystem::path package_root;
    std::filesystem::path runtime_root;
    std::filesystem::path zluda_cache_root;
    std::filesystem::path triton_cache_root;
    std::filesystem::path torchinductor_cache_root;
    std::vector<ToolCandidate> tool_candidates;
    std::filesystem::path selected_comfyui;
    std::filesystem::path selected_python;
    std::vector<std::string> arguments;
    std::map<std::string, std::string> process_environment;
    std::vector<LaunchDiagnostic> diagnostics;
    std::vector<RuntimeDiagnostic> runtime_diagnostics;
    std::vector<ReversibleRuntimeAction> reversible_runtime_actions;
};

class RuntimeDetector {
public:
    std::vector<RuntimeDiagnostic> inspect(const PortablePackageEnvironment& environment) const;
    std::vector<ReversibleRuntimeAction> planZludaActions(
        const PortablePackageEnvironment& environment,
        const LaunchProfile& profile) const;
};

class LauncherPlanner {
public:
    LaunchPlan plan(const PortablePackageEnvironment& environment, const LaunchProfile& profile) const;
};

} // namespace flavor_tests::minifox
