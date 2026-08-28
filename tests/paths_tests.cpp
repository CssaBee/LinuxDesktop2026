#include "linuxdesktop/paths.hpp"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

namespace ld = linuxdesktop::paths;

struct test_failure {
    std::string message;
};

[[noreturn]] void fail(std::string message)
{
    throw test_failure{std::move(message)};
}

void require(bool condition, const std::string& message)
{
    if (!condition) {
        fail(message);
    }
}

bool has_diagnostic(const std::vector<ld::diagnostic>& diagnostics, std::string_view code)
{
    for (const auto& item : diagnostics) {
        if (item.code == code) {
            return true;
        }
    }
    return false;
}

std::filesystem::path selected_path(const ld::resolver_report& report, ld::path_family family)
{
    const auto item = report.selected.find(family);
    if (item == report.selected.end()) {
        fail(std::string("missing selected path for ") + std::string(ld::to_string(family)));
    }
    return item->second;
}

bool has_selected_candidate(const ld::resolver_report& report, ld::path_family family, ld::candidate_source source)
{
    for (const auto& candidate : report.candidates) {
        if (candidate.family == family && candidate.source == source && candidate.selected) {
            return true;
        }
    }
    return false;
}

ld::resolver_options deterministic_options()
{
    ld::resolver_options options;
    options.use_process_environment = false;
    options.home_directory = "/home/tester";
    options.temp_override = "/tmp/linuxdesktop2026-paths-tests";
    options.executable_path = "/opt/linuxdesktop2026/bin/paths-tests";
    return options;
}

void exposes_cpp_version()
{
    require(ld::version_major == 0, "C++ version major should match project version");
    require(ld::version_minor == 1, "C++ version minor should match project version");
    require(ld::version_patch == 0, "C++ version patch should match project version");
}

void paths_diagnostics_use_shared_core_vocabulary()
{
    ld::diagnostic paths_diagnostic;
    paths_diagnostic.level = ld::severity::warning;
    paths_diagnostic.code = "shared-diagnostic";

    linuxdesktop::diagnostic core_diagnostic = paths_diagnostic;
    require(core_diagnostic.code == "shared-diagnostic", "paths diagnostics should alias shared diagnostics");
    require(linuxdesktop::to_string(core_diagnostic.level) == "warning", "shared severity should stringify");
    require(ld::to_string(paths_diagnostic.level) == "warning", "paths severity alias should stringify");
}

void stringifies_public_enums()
{
    require(ld::to_string(ld::path_family::config) == "config", "path family should stringify");
    require(ld::to_string(ld::path_family::public_share) == "public_share", "public share should stringify");
    require(ld::to_string(ld::candidate_source::known_folder) == "known_folder", "candidate source should stringify");
    require(ld::to_string(ld::candidate_source::xdg_base_dir) == "xdg_base_dir", "XDG base dir should stringify");
}

void resolves_linux_xdg_base_directories_from_injected_environment()
{
    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";
    auto options = deterministic_options();
    options.environment["XDG_CONFIG_HOME"] = "/xdg/config";
    options.environment["XDG_DATA_HOME"] = "/xdg/data";
    options.environment["XDG_STATE_HOME"] = "/xdg/state";
    options.environment["XDG_CACHE_HOME"] = "/xdg/cache";

    const auto report = ld::resolve_app_paths(identity, options);

    require(selected_path(report, ld::path_family::config) == "/xdg/config/LinuxDesktop2026/paths-tests",
        "config path should use XDG_CONFIG_HOME");
    require(selected_path(report, ld::path_family::data) == "/xdg/data/LinuxDesktop2026/paths-tests",
        "data path should use XDG_DATA_HOME");
    require(selected_path(report, ld::path_family::state) == "/xdg/state/LinuxDesktop2026/paths-tests",
        "state path should use XDG_STATE_HOME");
    require(selected_path(report, ld::path_family::cache) == "/xdg/cache/LinuxDesktop2026/paths-tests",
        "cache path should use XDG_CACHE_HOME");
    require(has_selected_candidate(report, ld::path_family::config, ld::candidate_source::xdg_base_dir),
        "XDG config candidate should be source-labeled");
}

void resolves_home_fallbacks_when_xdg_is_unset()
{
    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";
    const auto report = ld::resolve_app_paths(identity, deterministic_options());

    require(selected_path(report, ld::path_family::config) == "/home/tester/.config/LinuxDesktop2026/paths-tests",
        "config path should fall back under HOME");
    require(selected_path(report, ld::path_family::data) == "/home/tester/.local/share/LinuxDesktop2026/paths-tests",
        "data path should fall back under HOME");
    require(selected_path(report, ld::path_family::state) == "/home/tester/.local/state/LinuxDesktop2026/paths-tests",
        "state path should fall back under HOME");
    require(selected_path(report, ld::path_family::cache) == "/home/tester/.cache/LinuxDesktop2026/paths-tests",
        "cache path should fall back under HOME");
    require(selected_path(report, ld::path_family::documents) == "/home/tester/Documents",
        "documents path should use stable HOME fallback until XDG user-dirs parsing lands");
}

void rejects_relative_overrides_and_environment_values()
{
    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";
    auto options = deterministic_options();
    options.config_override = "relative-config";
    options.environment["XDG_DATA_HOME"] = "relative-data";

    const auto report = ld::resolve_app_paths(identity, options);

    require(has_diagnostic(report.diagnostics, ld::diagnostic_code::override_relative_ignored),
        "relative explicit override should be diagnosed");
    require(has_diagnostic(report.diagnostics, ld::diagnostic_code::environment_relative_ignored),
        "relative environment path should be diagnosed");
    require(selected_path(report, ld::path_family::config) == "/home/tester/.config/LinuxDesktop2026/paths-tests",
        "relative config override should be ignored");
    require(selected_path(report, ld::path_family::data) == "/home/tester/.local/share/LinuxDesktop2026/paths-tests",
        "relative data environment should be ignored");
}

void reports_missing_home_without_selecting_user_scoped_fallbacks()
{
    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";
    auto options = deterministic_options();
    options.home_directory.reset();

    const auto report = ld::resolve_app_paths(identity, options);

    require(has_diagnostic(report.diagnostics, ld::diagnostic_code::home_missing),
        "missing home should be diagnosed");
    require(report.selected.find(ld::path_family::config) == report.selected.end(),
        "config should not be guessed without HOME or XDG_CONFIG_HOME");
    require(report.selected.find(ld::path_family::documents) == report.selected.end(),
        "documents should not be guessed without HOME");
}

void resolves_executable_install_resource_and_temp_paths()
{
    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";
    const auto report = ld::resolve_app_paths(identity, deterministic_options());

    require(selected_path(report, ld::path_family::temp) == "/tmp/linuxdesktop2026-paths-tests",
        "temp override should be selected");
    require(selected_path(report, ld::path_family::executable) == "/opt/linuxdesktop2026/bin/paths-tests",
        "injected executable path should be selected");
    require(selected_path(report, ld::path_family::executable_directory) == "/opt/linuxdesktop2026/bin",
        "executable directory should derive from executable path");
    require(selected_path(report, ld::path_family::install_prefix) == "/opt/linuxdesktop2026",
        "install prefix should derive from a bin executable directory");
    require(selected_path(report, ld::path_family::resources) == "/opt/linuxdesktop2026/share/paths-tests",
        "resource root should derive from install prefix");
}

void honors_absolute_explicit_options()
{
    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";
    auto options = deterministic_options();
    options.config_override = "/override/config";
    options.resource_root = "/override/resources";
    options.install_prefix = "/override/prefix";

    const auto report = ld::resolve_app_paths(identity, options);

    require(selected_path(report, ld::path_family::config) == "/override/config",
        "absolute config override should win");
    require(selected_path(report, ld::path_family::resources) == "/override/resources",
        "absolute resource root should win");
    require(selected_path(report, ld::path_family::install_prefix) == "/override/prefix",
        "absolute install prefix should win");
    require(has_selected_candidate(report, ld::path_family::resources, ld::candidate_source::explicit_option),
        "explicit resource candidate should be source-labeled");
}

} // namespace

int main()
{
    try {
        exposes_cpp_version();
        paths_diagnostics_use_shared_core_vocabulary();
        stringifies_public_enums();
        resolves_linux_xdg_base_directories_from_injected_environment();
        resolves_home_fallbacks_when_xdg_is_unset();
        rejects_relative_overrides_and_environment_values();
        reports_missing_home_without_selecting_user_scoped_fallbacks();
        resolves_executable_install_resource_and_temp_paths();
        honors_absolute_explicit_options();
    } catch (const test_failure& failure) {
        std::cerr << failure.message << "\n";
        return EXIT_FAILURE;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
