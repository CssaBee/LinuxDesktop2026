#include "minifox_flavor.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace minifox = flavor_tests::minifox;

namespace {

int failures = 0;

void expect(bool condition, const std::string& name)
{
    if (condition) {
        std::cout << "ok " << name << '\n';
    } else {
        std::cout << "not ok " << name << '\n';
        ++failures;
    }
}

minifox::PortablePackageEnvironment default_env(const std::string& name)
{
    const auto root = std::filesystem::temp_directory_path() / ("linuxdesktop2026-" + name);
    std::filesystem::remove_all(root);

    minifox::PortablePackageEnvironment env;
    env.executable_directory = root / "ComfyUI-Package";
    env.home_directory = root / "home" / "alice";
    env.runtime_directory = root / "run" / "user" / "1000";
    return env;
}

void create_portable_package(const minifox::PortablePackageEnvironment& env)
{
    std::filesystem::create_directories(env.executable_directory / "ComfyUI");
    std::filesystem::create_directories(env.executable_directory / "python");
    std::ofstream(env.executable_directory / "ComfyUI" / "main.py") << "print('ComfyUI')\n";
    std::ofstream(env.executable_directory / "python" / "python.exe") << "python\n";
}

bool has_runtime(const minifox::LaunchPlan& plan, minifox::RuntimeKind kind, bool eligible)
{
    for (const auto& item : plan.runtime_diagnostics) {
        if (item.kind == kind && item.eligible == eligible) {
            return true;
        }
    }
    return false;
}

bool has_diagnostic(const minifox::LaunchPlan& plan, minifox::DiagnosticCode code)
{
    for (const auto& item : plan.diagnostics) {
        if (item.code == code) {
            return true;
        }
    }
    return false;
}

void default_portable_layout_uses_executable_adjacent_roots()
{
    auto env = default_env("minifox-portable");
    create_portable_package(env);

    minifox::LaunchProfile profile;
    profile.name = "NVIDIA default";
    profile.arguments = {"--listen", "127.0.0.1"};

    const auto plan = minifox::LauncherPlanner{}.plan(env, profile);

    expect(plan.working_directory == env.executable_directory / "ComfyUI",
        "ComfyUI working directory is executable-adjacent");
    expect(plan.selected_python == env.executable_directory / "python" / "python.exe",
        "Python candidate is executable-adjacent");
    expect(plan.application_settings_file == env.executable_directory / ".minifox" / "application-settings.json",
        "application settings stay beside the launcher");
    expect(plan.profile_settings_file == env.executable_directory / ".minifox" / "profiles" / "nvidia-default.json",
        "profile settings stay under portable data");
    expect(plan.zluda_cache_root == env.executable_directory / ".cache" / "zluda",
        "ZLUDA cache is executable-adjacent");
    expect(plan.diagnostics.empty(), "valid portable package has no launch diagnostics");
}

void manual_profile_paths_override_auto_candidates()
{
    auto env = default_env("minifox-manual");
    create_portable_package(env);
    const auto manual_comfyui = *env.home_directory / "ComfyUI-dev";
    const auto manual_python = *env.home_directory / "venv" / "Scripts" / "python.exe";
    std::filesystem::create_directories(manual_comfyui);
    std::filesystem::create_directories(manual_python.parent_path());
    std::ofstream(manual_comfyui / "main.py") << "print('manual')\n";
    std::ofstream(manual_python) << "python\n";

    minifox::LaunchProfile profile;
    profile.name = "manual";
    profile.comfyui_path = manual_comfyui;
    profile.python_path = manual_python;

    const auto plan = minifox::LauncherPlanner{}.plan(env, profile);

    expect(plan.selected_comfyui == manual_comfyui, "manual ComfyUI path is selected");
    expect(plan.selected_python == manual_python, "manual Python path is selected");
    expect(plan.tool_candidates.size() == 2, "manual profile still reports both tool candidates");
    expect(plan.tool_candidates[0].source == linuxdesktop::paths::candidate_source::explicit_option,
        "ComfyUI candidate reports manual profile source");
    expect(plan.tool_candidates[1].source == linuxdesktop::paths::candidate_source::explicit_option,
        "Python candidate reports manual profile source");
}

void missing_launch_tools_are_product_diagnostics_without_root_creation()
{
    const auto env = default_env("minifox-missing");

    const auto plan = minifox::LauncherPlanner{}.plan(env, {});

    expect(has_diagnostic(plan, minifox::DiagnosticCode::MissingComfyUiMain),
        "missing ComfyUI main is diagnosed");
    expect(has_diagnostic(plan, minifox::DiagnosticCode::MissingPythonExecutable),
        "missing Python executable is diagnosed");
    expect(!std::filesystem::exists(env.executable_directory / ".minifox"),
        "planning does not create Minifox data root");
    expect(!std::filesystem::exists(env.executable_directory / ".cache"),
        "planning does not create Minifox cache root");
}

void multiple_profiles_share_cache_policy_but_not_settings_files()
{
    auto env = default_env("minifox-profiles");
    create_portable_package(env);

    minifox::LaunchProfile low_vram;
    low_vram.name = "Low VRAM";
    minifox::LaunchProfile high_vram;
    high_vram.name = "High VRAM";

    const auto low = minifox::LauncherPlanner{}.plan(env, low_vram);
    const auto high = minifox::LauncherPlanner{}.plan(env, high_vram);

    expect(low.profile_settings_file != high.profile_settings_file,
        "profiles have separate settings files");
    expect(low.triton_cache_root == high.triton_cache_root,
        "profiles share portable Triton cache policy");
    expect(low.torchinductor_cache_root == high.torchinductor_cache_root,
        "profiles share portable TorchInductor cache policy");
}

void runtime_inputs_remain_minifox_diagnostics_and_actions()
{
    auto env = default_env("minifox-runtime");
    create_portable_package(env);
    env.cuda_available = true;
    env.rocm_available = true;
    env.amd_gpu_with_cuda_torch = true;
    env.bundled_zluda_available = true;
    env.environment["HIP_PATH"] = (env.executable_directory / ".minifox" / "packages" / "hip").string();

    minifox::LaunchProfile profile;
    profile.name = "AMD ZLUDA";
    profile.request_zluda = true;

    const auto plan = minifox::LauncherPlanner{}.plan(env, profile);

    expect(has_runtime(plan, minifox::RuntimeKind::Cuda, true), "CUDA is recorded as a runtime input");
    expect(has_runtime(plan, minifox::RuntimeKind::Rocm, true), "ROCm is recorded as a runtime input");
    expect(has_runtime(plan, minifox::RuntimeKind::HipSdk, true), "HIP SDK is recorded as a runtime input");
    expect(has_runtime(plan, minifox::RuntimeKind::Zluda, true), "ZLUDA eligibility is recorded as runtime input");
    expect(plan.reversible_runtime_actions.size() == 1,
        "ZLUDA replacement is product-owned reversible runtime work");
    expect(plan.process_environment.count("ZLUDA_CACHE_DIR") == 1,
        "ZLUDA cache environment is part of the launch plan");
}

} // namespace

int main()
{
    default_portable_layout_uses_executable_adjacent_roots();
    manual_profile_paths_override_auto_candidates();
    missing_launch_tools_are_product_diagnostics_without_root_creation();
    multiple_profiles_share_cache_policy_but_not_settings_files();
    runtime_inputs_remain_minifox_diagnostics_and_actions();
    return failures == 0 ? 0 : 1;
}
