#include "minifox_flavor.hpp"

#include "minifox/generated/platform_path_defaults.hpp"

#include "linuxdesktop/root.hpp"

#include <cctype>
#include <utility>

namespace flavor_tests::minifox {

namespace {

LaunchDiagnostic diagnostic(
    DiagnosticCode code,
    std::string message,
    bool fatal,
    std::filesystem::path path = {})
{
    return {code, std::move(message), std::move(path), fatal};
}

std::string profile_file_stem(const std::string& name)
{
    std::string stem;
    for (const auto ch : name) {
        const auto value = static_cast<unsigned char>(ch);
        if (std::isalnum(value) || ch == '-' || ch == '_') {
            stem.push_back(static_cast<char>(std::tolower(value)));
        } else if (!stem.empty() && stem.back() != '-') {
            stem.push_back('-');
        }
    }
    while (!stem.empty() && stem.back() == '-') {
        stem.pop_back();
    }
    return stem.empty() ? "default" : stem;
}

ToolCandidate make_candidate(
    std::string name,
    linuxdesktop::paths::candidate_source source,
    std::filesystem::path path,
    bool selected)
{
    return {std::move(name), source, std::move(path), selected};
}

void append_path_diagnostics(
    const linuxdesktop::root::report& report,
    std::vector<LaunchDiagnostic>& diagnostics)
{
    for (const auto& item : report.diagnostics) {
        diagnostics.push_back(diagnostic(
            DiagnosticCode::PathResolutionWarning,
            item.message.empty() ? item.code : item.message,
            item.level == linuxdesktop::severity::error,
            item.path));
    }
}

} // namespace

std::vector<RuntimeDiagnostic> RuntimeDetector::inspect(const PortablePackageEnvironment& environment) const
{
    std::vector<RuntimeDiagnostic> diagnostics;
    diagnostics.push_back({
        RuntimeKind::Cuda,
        environment.cuda_available ? "CUDA runtime will be treated as a Minifox runtime input" :
                                     "CUDA runtime was not detected by Minifox inputs",
        environment.cuda_available});
    diagnostics.push_back({
        RuntimeKind::Rocm,
        environment.rocm_available ? "ROCm runtime will be treated as a Minifox runtime input" :
                                     "ROCm runtime was not detected by Minifox inputs",
        environment.rocm_available});

    const auto hip = environment.environment.find("HIP_PATH");
    diagnostics.push_back({
        RuntimeKind::HipSdk,
        hip != environment.environment.end() && !hip->second.empty() ?
            "HIP SDK path is a Minifox runtime input" :
            "HIP SDK path was not provided to Minifox",
        hip != environment.environment.end() && !hip->second.empty()});

    diagnostics.push_back({
        RuntimeKind::Zluda,
        environment.amd_gpu_with_cuda_torch && environment.bundled_zluda_available ?
            "ZLUDA is eligible for product-owned reversible runtime work" :
            "ZLUDA is not eligible from the provided Minifox inputs",
        environment.amd_gpu_with_cuda_torch && environment.bundled_zluda_available});
    return diagnostics;
}

std::vector<ReversibleRuntimeAction> RuntimeDetector::planZludaActions(
    const PortablePackageEnvironment& environment,
    const LaunchProfile& profile) const
{
    if (!profile.request_zluda ||
        !environment.amd_gpu_with_cuda_torch ||
        !environment.bundled_zluda_available) {
        return {};
    }

    const auto cache_root = environment.executable_directory / ".cache" / "zluda";
    return {{
        "temporarily replace CUDA runtime shim",
        environment.executable_directory / ".minifox" / "packages" / "zluda" / "nvcuda.dll",
        cache_root / "active" / "nvcuda.dll",
        cache_root / "restore" / "nvcuda.dll",
    }};
}

LaunchPlan LauncherPlanner::plan(
    const PortablePackageEnvironment& environment,
    const LaunchProfile& profile) const
{
    linuxdesktop::root::portable_root_request portable_root;
    portable_root.root = environment.executable_directory;
    portable_root.requested = true;
    portable_root.level = linuxdesktop::root::portable_root_level::profile;

    const auto data_root = linuxdesktop::root::make_named_root_handle(
        linuxdesktop::root::make_named_root_request(
            "minifox-data",
            linuxdesktop::root::purpose_kind::data,
            linuxdesktop::root::ownership_kind::app_local,
            ".minifox"));
    const auto cache_root = linuxdesktop::root::make_named_root_handle(
        linuxdesktop::root::make_named_root_request(
            "minifox-cache",
            linuxdesktop::root::purpose_kind::cache,
            linuxdesktop::root::ownership_kind::app_local,
            ".cache"));
    const auto runtime_root = linuxdesktop::root::make_named_root_handle(
        linuxdesktop::root::make_named_root_request(
            "minifox-runtime",
            linuxdesktop::root::purpose_kind::runtime,
            linuxdesktop::root::ownership_kind::app_local,
            ".minifox/runtime"));

    auto builder = linuxdesktop::root::request_builder()
        .app("Minifox", "ComfyUI Launcher")
        .resource_root(environment.executable_directory)
        .home_directory(environment.home_directory)
        .environment(environment.environment)
        .use_process_environment(false)
        .portable_root(portable_root)
        .named_root(data_root)
        .named_root(cache_root)
        .named_root(runtime_root);

    if (environment.home_directory) {
        builder.platform_defaults(linuxdesktop2026::generated::platform_path_defaults_for_home(
            *environment.home_directory,
            environment.runtime_directory));
    }

    const auto report = builder.resolve();
    const auto portable_comfyui = environment.executable_directory / "ComfyUI";
    const auto portable_python = environment.executable_directory / "python" / "python.exe";

    LaunchPlan plan;
    plan.working_directory = portable_comfyui;
    plan.package_root = environment.executable_directory;
    plan.application_settings_file = environment.executable_directory / ".minifox" / "application-settings.json";
    plan.profile_settings_file =
        environment.executable_directory / ".minifox" / "profiles" / (profile_file_stem(profile.name) + ".json");
    plan.runtime_root = environment.executable_directory / ".minifox" / "runtime";
    plan.zluda_cache_root = environment.executable_directory / ".cache" / "zluda";
    plan.triton_cache_root = environment.executable_directory / ".cache" / "triton";
    plan.torchinductor_cache_root = environment.executable_directory / ".cache" / "torchinductor";

    if (const auto* data = linuxdesktop::root::find_named_root(report, data_root)) {
        plan.application_settings_file = data->path / "application-settings.json";
        plan.profile_settings_file = data->path / "profiles" / (profile_file_stem(profile.name) + ".json");
    }
    if (const auto* cache = linuxdesktop::root::find_named_root(report, cache_root)) {
        plan.zluda_cache_root = cache->path / "zluda";
        plan.triton_cache_root = cache->path / "triton";
        plan.torchinductor_cache_root = cache->path / "torchinductor";
    }
    if (const auto* runtime = linuxdesktop::root::find_named_root(report, runtime_root)) {
        plan.runtime_root = runtime->path;
    }

    plan.selected_comfyui = profile.comfyui_path.value_or(portable_comfyui);
    plan.selected_python = profile.python_path.value_or(portable_python);
    plan.tool_candidates.push_back(make_candidate(
        "ComfyUI",
        profile.comfyui_path ? linuxdesktop::paths::candidate_source::explicit_option :
                                linuxdesktop::paths::candidate_source::executable_relative,
        plan.selected_comfyui,
        true));
    plan.tool_candidates.push_back(make_candidate(
        "python",
        profile.python_path ? linuxdesktop::paths::candidate_source::explicit_option :
                               linuxdesktop::paths::candidate_source::executable_relative,
        plan.selected_python,
        true));

    plan.arguments = profile.arguments;
    plan.process_environment = {
        {"MINIFOX_PROFILE", profile.name},
        {"TRITON_CACHE_DIR", plan.triton_cache_root.string()},
        {"TORCHINDUCTOR_CACHE_DIR", plan.torchinductor_cache_root.string()},
    };
    if (profile.request_zluda) {
        plan.process_environment["ZLUDA_CACHE_DIR"] = plan.zluda_cache_root.string();
    }

    append_path_diagnostics(report, plan.diagnostics);
    if (!std::filesystem::exists(plan.selected_comfyui / "main.py")) {
        plan.diagnostics.push_back(diagnostic(
            DiagnosticCode::MissingComfyUiMain,
            "ComfyUI main.py is required before Minifox can launch this profile",
            true,
            plan.selected_comfyui / "main.py"));
    }
    if (!std::filesystem::exists(plan.selected_python)) {
        plan.diagnostics.push_back(diagnostic(
            DiagnosticCode::MissingPythonExecutable,
            "Python executable is required before Minifox can launch this profile",
            true,
            plan.selected_python));
    }

    RuntimeDetector detector;
    plan.runtime_diagnostics = detector.inspect(environment);
    plan.reversible_runtime_actions = detector.planZludaActions(environment, profile);
    return plan;
}

} // namespace flavor_tests::minifox
