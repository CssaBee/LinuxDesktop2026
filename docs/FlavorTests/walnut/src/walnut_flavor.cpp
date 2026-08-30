#include "walnut_flavor.hpp"

#include "walnut/generated/platform_path_defaults.hpp"

#include "linuxdesktop/paths.hpp"

#include <algorithm>

namespace flavor_tests::walnut {

namespace ldp = linuxdesktop::paths;

namespace {

std::optional<std::filesystem::path> lookup_path(
    const std::map<std::string, std::string>& environment,
    const std::string& name)
{
    const auto found = environment.find(name);
    if (found == environment.end() || found->second.empty()) {
        return std::nullopt;
    }
    return std::filesystem::path(found->second);
}

std::optional<Gpu> select_gpu(const std::vector<Gpu>& gpus)
{
    const auto discrete = std::find_if(gpus.begin(), gpus.end(), [](const Gpu& gpu) {
        return gpu.discrete;
    });
    if (discrete != gpus.end()) {
        return *discrete;
    }
    if (!gpus.empty()) {
        return gpus.front();
    }
    return std::nullopt;
}

StartupDiagnostic diagnostic(
    StartupDiagnosticCode code,
    std::string message,
    bool fatal,
    std::filesystem::path path = {})
{
    return {code, std::move(message), std::move(path), fatal};
}

std::vector<StartupDiagnostic> translate_path_diagnostics(const ldp::resolver_report& report)
{
    std::vector<StartupDiagnostic> diagnostics;
    for (const auto& item : report.diagnostics) {
        diagnostics.push_back(diagnostic(
            StartupDiagnosticCode::PathResolutionWarning,
            item.message.empty() ? item.code : item.message,
            item.level == linuxdesktop::severity::error,
            item.path));
    }
    return diagnostics;
}

bool has_fatal_diagnostic(const std::vector<StartupDiagnostic>& diagnostics)
{
    return std::any_of(diagnostics.begin(), diagnostics.end(), [](const StartupDiagnostic& item) {
        return item.fatal;
    });
}

} // namespace

BootstrapPlan ApplicationBootstrap::prepare(
    const ApplicationSpecification& specification,
    const RuntimeEnvironment& environment,
    const LaunchOptions& launch) const
{
    ldp::app_identity identity;
    identity.organization = "Walnut";
    identity.application = specification.name;

    ldp::resolver_options options;
    options.executable_path = environment.executable_directory / specification.name;
    options.resource_root = launch.resource_root_override.value_or(environment.executable_directory);
    options.home_directory = environment.home_directory;
    options.environment = environment.environment;
    options.use_process_environment = false;
    if (environment.home_directory) {
        options.platform_defaults = linuxdesktop2026::generated::platform_path_defaults_for_home(
            *environment.home_directory,
            environment.runtime_directory);
    }

    const auto paths = ldp::resolve_app_paths(identity, options);

    BootstrapPlan plan;
    plan.window_title = specification.name;
    plan.width = specification.width;
    plan.height = specification.height;
    plan.entry_point = launch.distribution_mode ? EntryPointKind::WindowsMain : EntryPointKind::ConsoleMain;
    plan.executable_root = paths.selected.at(ldp::path_family::executable_directory);
    plan.resource_root = paths.selected.at(ldp::path_family::resources);
    plan.config_root = paths.selected.at(ldp::path_family::config);
    plan.vulkan_sdk = lookup_path(environment.environment, "VULKAN_SDK");
    plan.required_instance_extensions = environment.required_instance_extensions;
    plan.selected_gpu = select_gpu(environment.gpus);
    plan.diagnostics = translate_path_diagnostics(paths);

    if (launch.headless_test_mode) {
        plan.diagnostics.push_back(diagnostic(
            StartupDiagnosticCode::RenderingSkipped,
            "headless test launch skips GLFW and Vulkan startup",
            false));
        plan.should_start = !has_fatal_diagnostic(plan.diagnostics);
        return plan;
    }

    if (!environment.glfw_initializes) {
        plan.diagnostics.push_back(diagnostic(
            StartupDiagnosticCode::GlfwUnavailable,
            "GLFW initialization failed",
            true));
    }
    if (!environment.vulkan_supported) {
        plan.diagnostics.push_back(diagnostic(
            StartupDiagnosticCode::VulkanUnavailable,
            "Vulkan support is unavailable",
            true));
    }
    if (!environment.required_instance_extensions_available) {
        plan.diagnostics.push_back(diagnostic(
            StartupDiagnosticCode::RequiredExtensionsMissing,
            "required Vulkan instance extensions are unavailable",
            true));
    }
    if (!environment.wsi_supported) {
        plan.diagnostics.push_back(diagnostic(
            StartupDiagnosticCode::WsiUnavailable,
            "Vulkan window-system integration is unavailable",
            true));
    }

    plan.should_start = !has_fatal_diagnostic(plan.diagnostics);
    return plan;
}

ImagePathResult ResourceLocator::resolveImagePath(std::string_view path_or_name, const BootstrapPlan& plan) const
{
    std::filesystem::path requested(path_or_name);
    if (requested.is_absolute()) {
        const bool exists = std::filesystem::exists(requested);
        return {
            exists,
            requested,
            exists ? std::vector<StartupDiagnostic>{}
                   : std::vector<StartupDiagnostic>{diagnostic(
                         StartupDiagnosticCode::ImageMissing,
                         "image asset is missing",
                         true,
                         requested)},
        };
    }

    if (plan.resource_root.empty()) {
        return {
            false,
            requested,
            {diagnostic(
                StartupDiagnosticCode::ResourceRootUnsupported,
                "resource root is unavailable for relative image lookup",
                true)},
        };
    }

    const auto resolved = plan.resource_root / requested;
    const bool exists = std::filesystem::exists(resolved);
    return {
        exists,
        resolved,
        exists ? std::vector<StartupDiagnostic>{}
               : std::vector<StartupDiagnostic>{diagnostic(
                     StartupDiagnosticCode::ImageMissing,
                     "image asset is missing",
                     true,
                     resolved)},
    };
}

void ApplicationLifecycle::requestClose()
{
    running_ = false;
}

void ApplicationLifecycle::shutdown()
{
    running_ = false;
}

} // namespace flavor_tests::walnut
