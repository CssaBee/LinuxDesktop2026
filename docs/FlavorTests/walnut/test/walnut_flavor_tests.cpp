#include "walnut_flavor.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

struct test_failure : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

void require(bool condition, const std::string& message)
{
    if (!condition) {
        throw test_failure(message);
    }
}

std::filesystem::path test_root()
{
    auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-walnut-flavor";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    if (ec) {
        throw test_failure("failed to create test root: " + ec.message());
    }
    return root;
}

flavor_tests::walnut::RuntimeEnvironment default_environment(const std::filesystem::path& root)
{
    flavor_tests::walnut::RuntimeEnvironment environment;
    environment.executable_directory = root / "bin";
    environment.current_working_directory = root / "workspace";
    environment.home_directory = root / "home";
    environment.environment["XDG_CONFIG_HOME"] = (root / "xdg-config").string();
    environment.environment["APPDATA"] = (root / "appdata" / "roaming").string();
    environment.environment["LOCALAPPDATA"] = (root / "appdata" / "local").string();
    environment.environment["VULKAN_SDK"] = (root / "vulkan-sdk").string();
    environment.gpus = {
        {"integrated", false},
        {"discrete", true},
    };
    std::filesystem::create_directories(environment.executable_directory);
    std::filesystem::create_directories(environment.current_working_directory);
    std::filesystem::create_directories(*environment.home_directory);
    return environment;
}

std::filesystem::path expected_default_config_root(const std::filesystem::path& root)
{
#if defined(_WIN32)
    return root / "appdata" / "roaming" / "Walnut" / "WalnutSandbox";
#else
    return root / "xdg-config" / "Walnut" / "WalnutSandbox";
#endif
}

flavor_tests::walnut::BootstrapPlan prepare_default(
    const std::filesystem::path& root,
    flavor_tests::walnut::RuntimeEnvironment environment = {})
{
    if (environment.executable_directory.empty()) {
        environment = default_environment(root);
    }

    flavor_tests::walnut::ApplicationSpecification specification;
    specification.name = "WalnutSandbox";
    specification.width = 1280;
    specification.height = 720;

    flavor_tests::walnut::LaunchOptions launch;
    return flavor_tests::walnut::ApplicationBootstrap().prepare(specification, environment, launch);
}

void default_bootstrap_preserves_window_and_resolves_executable_resources()
{
    const auto root = test_root();
    const auto environment = default_environment(root);

    const auto plan = prepare_default(root, environment);

    require(plan.should_start, "default Walnut bootstrap should continue startup");
    require(plan.window_title == "WalnutSandbox", "window title should remain Walnut-owned");
    require(plan.width == 1280, "window width should remain Walnut-owned");
    require(plan.height == 720, "window height should remain Walnut-owned");
    require(plan.executable_root == environment.executable_directory,
        "executable root should come from the executable directory");
    require(plan.resource_root == environment.executable_directory,
        "resource root should default beside the executable");
    require(plan.config_root == expected_default_config_root(root),
        "config root should still be resolved through platform user paths");
    require(plan.diagnostics.empty(), "default bootstrap should not expose path diagnostics");
}

void vulkan_sdk_is_recorded_without_becoming_build_system_policy()
{
    const auto root = test_root();
    const auto environment = default_environment(root);

    const auto plan = prepare_default(root, environment);

    require(plan.vulkan_sdk.has_value(), "VULKAN_SDK should be recorded as a capability input");
    require(*plan.vulkan_sdk == root / "vulkan-sdk", "VULKAN_SDK should preserve the environment value");
    require(plan.should_start, "VULKAN_SDK presence should not control startup by itself");
}

void capability_failures_become_walnut_startup_diagnostics()
{
    const auto root = test_root();
    auto environment = default_environment(root);
    environment.glfw_initializes = false;
    environment.vulkan_supported = false;
    environment.required_instance_extensions_available = false;
    environment.wsi_supported = false;

    const auto plan = prepare_default(root, environment);

    require(!plan.should_start, "capability failures should stop startup cleanly");
    require(plan.diagnostics.size() == 4, "all render bootstrap failures should be reported");
    require(plan.diagnostics[0].code == flavor_tests::walnut::StartupDiagnosticCode::GlfwUnavailable,
        "GLFW failure should stay in Walnut vocabulary");
    require(plan.diagnostics[1].code == flavor_tests::walnut::StartupDiagnosticCode::VulkanUnavailable,
        "Vulkan failure should stay in Walnut vocabulary");
    require(plan.diagnostics[2].code == flavor_tests::walnut::StartupDiagnosticCode::RequiredExtensionsMissing,
        "extension failure should stay in Walnut vocabulary");
    require(plan.diagnostics[3].code == flavor_tests::walnut::StartupDiagnosticCode::WsiUnavailable,
        "WSI failure should stay in Walnut vocabulary");
}

void gpu_selection_prefers_discrete_and_falls_back_to_first()
{
    const auto root = test_root();
    auto environment = default_environment(root);

    auto plan = prepare_default(root, environment);
    require(plan.selected_gpu.has_value(), "a GPU should be selected when inventory is present");
    require(plan.selected_gpu->name == "discrete", "discrete GPU should win when available");

    environment.gpus = {
        {"software", false},
        {"integrated", false},
    };
    plan = prepare_default(root, environment);
    require(plan.selected_gpu.has_value(), "non-discrete inventory should still select a GPU");
    require(plan.selected_gpu->name == "software", "GPU selection should fall back to first adapter");
}

void image_lookup_keeps_absolute_paths_and_resolves_relative_resources()
{
    const auto root = test_root();
    const auto environment = default_environment(root);
    const auto plan = prepare_default(root, environment);
    const auto absolute_image = root / "image.png";
    const auto relative_image = environment.executable_directory / "assets" / "logo.png";
    std::ofstream(absolute_image) << "png";
    std::filesystem::create_directories(relative_image.parent_path());
    std::ofstream(relative_image) << "png";

    flavor_tests::walnut::ResourceLocator locator;
    const auto absolute = locator.resolveImagePath(absolute_image.string(), plan);
    const auto relative = locator.resolveImagePath("assets/logo.png", plan);

    require(absolute.ok, "absolute image lookup should succeed");
    require(absolute.path == absolute_image, "absolute image path should be preserved");
    require(relative.ok, "relative image lookup should succeed");
    require(relative.path == relative_image, "relative image path should resolve under resource root");
}

void image_lookup_reports_missing_assets_in_walnut_terms()
{
    const auto root = test_root();
    const auto plan = prepare_default(root);

    const auto result = flavor_tests::walnut::ResourceLocator().resolveImagePath("missing.png", plan);

    require(!result.ok, "missing image lookup should fail");
    require(result.diagnostics.size() == 1, "missing image lookup should explain the failure");
    require(result.diagnostics[0].code == flavor_tests::walnut::StartupDiagnosticCode::ImageMissing,
        "missing image diagnostic should stay in Walnut vocabulary");
}

void lifecycle_close_and_shutdown_stop_entrypoint_recreation()
{
    flavor_tests::walnut::ApplicationLifecycle lifecycle;

    require(lifecycle.running(), "application should start in the running state");
    require(lifecycle.shouldRestartFromMainLoop(), "entrypoint loop should run while application is active");
    lifecycle.requestClose();
    require(!lifecycle.running(), "Close should stop the application");
    require(!lifecycle.shouldRestartFromMainLoop(), "normal close should prevent entrypoint recreation");

    flavor_tests::walnut::ApplicationLifecycle shutdown_lifecycle;
    shutdown_lifecycle.shutdown();
    require(!shutdown_lifecycle.running(), "Shutdown should stop the application");
    require(!shutdown_lifecycle.shouldRestartFromMainLoop(), "shutdown should prevent entrypoint recreation");
}

void distribution_mode_records_entrypoint_semantics()
{
    const auto root = test_root();
    const auto environment = default_environment(root);
    flavor_tests::walnut::ApplicationSpecification specification;
    specification.name = "WalnutSandbox";

    flavor_tests::walnut::LaunchOptions launch;
    auto plan = flavor_tests::walnut::ApplicationBootstrap().prepare(specification, environment, launch);
    require(plan.entry_point == flavor_tests::walnut::EntryPointKind::ConsoleMain,
        "normal Walnut apps should use main-style semantics");

    launch.distribution_mode = true;
    plan = flavor_tests::walnut::ApplicationBootstrap().prepare(specification, environment, launch);
    require(plan.entry_point == flavor_tests::walnut::EntryPointKind::WindowsMain,
        "distribution mode should record WinMain-style semantics without compiling Windows entrypoint code");
}

void headless_launch_skips_rendering_but_keeps_root_resolution()
{
    const auto root = test_root();
    auto environment = default_environment(root);
    environment.glfw_initializes = false;
    environment.vulkan_supported = false;

    flavor_tests::walnut::ApplicationSpecification specification;
    specification.name = "WalnutHeadless";
    flavor_tests::walnut::LaunchOptions launch;
    launch.headless_test_mode = true;

    const auto plan = flavor_tests::walnut::ApplicationBootstrap().prepare(specification, environment, launch);

    require(plan.should_start, "headless test mode should not fail on missing rendering capabilities");
    require(plan.resource_root == environment.executable_directory,
        "headless launch should still resolve resource roots");
    require(plan.diagnostics.size() == 1, "headless launch should record why rendering was skipped");
    require(plan.diagnostics[0].code == flavor_tests::walnut::StartupDiagnosticCode::RenderingSkipped,
        "headless diagnostic should stay in Walnut vocabulary");
}

void resource_root_override_wins_over_executable_adjacent_default()
{
    const auto root = test_root();
    const auto environment = default_environment(root);
    const auto resource_override = root / "assets";

    flavor_tests::walnut::ApplicationSpecification specification;
    specification.name = "WalnutSandbox";
    flavor_tests::walnut::LaunchOptions launch;
    launch.resource_root_override = resource_override;

    const auto plan = flavor_tests::walnut::ApplicationBootstrap().prepare(specification, environment, launch);

    require(plan.should_start, "resource override should not stop startup");
    require(plan.resource_root == resource_override, "resource override should win over executable-adjacent default");
}

} // namespace

int main()
{
    try {
        default_bootstrap_preserves_window_and_resolves_executable_resources();
        vulkan_sdk_is_recorded_without_becoming_build_system_policy();
        capability_failures_become_walnut_startup_diagnostics();
        gpu_selection_prefers_discrete_and_falls_back_to_first();
        image_lookup_keeps_absolute_paths_and_resolves_relative_resources();
        image_lookup_reports_missing_assets_in_walnut_terms();
        lifecycle_close_and_shutdown_stop_entrypoint_recreation();
        distribution_mode_records_entrypoint_semantics();
        headless_launch_skips_rendering_but_keeps_root_resolution();
        resource_root_override_wins_over_executable_adjacent_default();
    } catch (const std::exception& failure) {
        std::cerr << failure.what() << '\n';
        return 1;
    }

    return 0;
}
