#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace flavor_tests::walnut {

struct ApplicationSpecification {
    std::string name = "Walnut App";
    int width = 1600;
    int height = 900;
};

struct Gpu {
    std::string name;
    bool discrete = false;
};

struct RuntimeEnvironment {
    std::filesystem::path executable_directory;
    std::filesystem::path current_working_directory;
    std::optional<std::filesystem::path> home_directory;
    std::map<std::string, std::string> environment;
    bool glfw_initializes = true;
    bool vulkan_supported = true;
    std::vector<std::string> required_instance_extensions = {"VK_KHR_surface", "VK_KHR_xcb_surface"};
    bool required_instance_extensions_available = true;
    bool wsi_supported = true;
    std::vector<Gpu> gpus;
};

enum class EntryPointKind {
    ConsoleMain,
    WindowsMain
};

struct LaunchOptions {
    std::vector<std::string> arguments;
    bool distribution_mode = false;
    bool headless_test_mode = false;
    std::optional<std::filesystem::path> resource_root_override;
};

enum class StartupDiagnosticCode {
    PathResolutionWarning,
    GlfwUnavailable,
    VulkanUnavailable,
    RequiredExtensionsMissing,
    WsiUnavailable,
    RenderingSkipped,
    ResourceRootUnsupported,
    ImageMissing
};

struct StartupDiagnostic {
    StartupDiagnosticCode code = StartupDiagnosticCode::PathResolutionWarning;
    std::string message;
    std::filesystem::path path;
    bool fatal = false;
};

struct BootstrapPlan {
    std::string window_title;
    int width = 0;
    int height = 0;
    std::filesystem::path executable_root;
    std::filesystem::path resource_root;
    std::filesystem::path config_root;
    std::optional<std::filesystem::path> vulkan_sdk;
    std::vector<std::string> required_instance_extensions;
    std::optional<Gpu> selected_gpu;
    EntryPointKind entry_point = EntryPointKind::ConsoleMain;
    std::vector<StartupDiagnostic> diagnostics;
    bool should_start = false;
};

struct ImagePathResult {
    bool ok = false;
    std::filesystem::path path;
    std::vector<StartupDiagnostic> diagnostics;
};

class ApplicationBootstrap {
public:
    BootstrapPlan prepare(
        const ApplicationSpecification& specification,
        const RuntimeEnvironment& environment,
        const LaunchOptions& launch) const;
};

class ResourceLocator {
public:
    ImagePathResult resolveImagePath(std::string_view path_or_name, const BootstrapPlan& plan) const;
};

class ApplicationLifecycle {
public:
    bool running() const { return running_; }
    bool shouldRestartFromMainLoop() const { return running_; }

    void requestClose();
    void shutdown();

private:
    bool running_ = true;
};

} // namespace flavor_tests::walnut
