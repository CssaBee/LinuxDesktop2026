#include "linuxdesktop/root.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace {

namespace ld = linuxdesktop::root;
namespace ld_paths = linuxdesktop::paths;

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

bool has_diagnostic(const std::vector<ld::diagnostic>& diagnostics, const std::string& code)
{
    for (const auto& item : diagnostics) {
        if (item.code == code) {
            return true;
        }
    }
    return false;
}

std::filesystem::path test_root()
{
    auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-root-tests";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    if (ec) {
        fail("failed to create test root: " + ec.message());
    }
    return root;
}

ld::app_identity identity()
{
    ld::app_identity value;
    value.organization = "LinuxDesktop2026";
    value.application = "root-tests";
    return value;
}

void resolves_named_roots()
{
    const auto root = test_root() / "app";

    ld::options options;
    options.app_root_override = root;
    options.named_roots = {
        ld::make_log_root_request("logs", ld::ownership_kind::user_local, "Logs"),
        ld::make_profiles_root_request("profiles", ld::ownership_kind::user_roaming, "Profiles"),
    };

    const auto report = ld::resolve_app_roots(identity(), options);

    require(report.named_roots.size() == 2, "two named roots should be resolved");
    require(report.named_roots[0].name == "logs", "first named root should preserve name");
    require(report.named_roots[0].path == report.roots.state / "Logs", "user-local logs should live under state");
    require(report.named_roots[1].path == report.roots.config / "Profiles", "user-roaming profiles should live under config");
    const auto* logs = ld::find_named_root(report, "logs");
    require(logs != nullptr, "C++ helper should find named roots by name");
    require(logs->path == report.roots.state / "Logs", "C++ helper should return the resolved named root");
}

void resolves_component_roots()
{
    const auto root = test_root() / "app";

    const auto plugin = ld::make_component_root_request(
        "compare-plugin",
        ld::component_kind::plugin,
        {
            ld::make_component_config_root_request("config", ld::ownership_kind::user_roaming, "Config"),
            ld::make_component_state_root_request("state", ld::ownership_kind::user_local, "State"),
        });

    ld::options options;
    options.app_root_override = root;
    options.component_roots = {plugin};

    const auto report = ld::resolve_app_roots(identity(), options);

    require(report.component_roots.size() == 1, "one component root group should be resolved");
    require(report.component_roots[0].name == "compare-plugin", "component name should be preserved");
    require(report.component_roots[0].kind == ld::component_kind::plugin, "component kind should be preserved");
    require(report.component_roots[0].roots.size() == 2, "component should expose requested roots");
    require(report.component_roots[0].roots[0].path == root / "components" / "compare-plugin" / "Config",
        "component config should live below component namespace");
    require(report.component_roots[0].roots[1].path == root / "components" / "compare-plugin" / "State",
        "component state should live below component namespace");
    const auto* plugin_roots = ld::find_component_roots(report, "compare-plugin");
    require(plugin_roots != nullptr, "C++ helper should find component root groups");
    const auto* plugin_state = ld::find_component_named_root(*plugin_roots, "state");
    require(plugin_state != nullptr, "C++ helper should find roots inside component groups");
    require(plugin_state->path == root / "components" / "compare-plugin" / "State",
        "component root helper should return resolved component path");
}

void builder_preserves_options()
{
    const auto root = test_root();
    const auto install = root / "Application";
    const auto app = root / "app-root";
    const auto user_config = root / "user-config";

    ld::portable_root_request portable;
    portable.marker = install / "local.marker";
    portable.level = ld::portable_root_level::profile;
    portable.allow_user_config_override = true;

    const auto report = ld::request_builder()
        .app("BuilderOrg", "BuilderApp")
        .resource_root(install)
        .home_directory(root / "home")
        .environment({{"XDG_CONFIG_HOME", (root / "xdg-config").string()}})
        .use_process_environment(false)
        .app_root_override(app)
        .user_config_override(user_config)
        .portable_root(portable)
        .create_directories(false)
        .named_root(ld::make_log_root_request("logs", ld::ownership_kind::user_local, "Logs"))
        .resolve();

    require(report.app_root_override_active, "builder should preserve app root override");
    require(!report.user_config_override_active, "builder should preserve app-over-user-config precedence");
    require(report.roots.resources == install, "builder should preserve resource root");
    require(report.roots.config == app, "builder should preserve config root");
    require(report.roots.state == app, "builder should preserve state root");
    const auto* logs = ld::find_named_root(report, "logs");
    require(logs != nullptr, "builder should preserve named roots");
    require(logs->path == app / "Logs", "builder named roots should resolve like explicit options");
    require(!std::filesystem::exists(app), "builder should preserve create-directories policy");
}

void helper_factories_match_explicit_request_shapes()
{
    const auto config = ld::make_config_root_request("config", ld::ownership_kind::user_roaming, "Config");
    require(config.name == "config", "config helper should preserve name");
    require(config.purpose == ld::purpose_kind::config, "config helper should set config purpose");
    require(config.ownership == ld::ownership_kind::user_roaming, "config helper should preserve ownership");
    require(config.relative_path == "Config", "config helper should preserve relative path");
    require(config.create, "config helper should keep create enabled by default");

    const auto cache = ld::make_cache_root_request("cache", ld::ownership_kind::ephemeral);
    require(cache.purpose == ld::purpose_kind::cache, "cache helper should set cache purpose");
    require(cache.ownership == ld::ownership_kind::ephemeral, "cache helper should preserve ownership");

    const auto session = ld::make_session_root_request("session", ld::ownership_kind::user_local, "Sessions");
    require(session.purpose == ld::purpose_kind::session, "session helper should set session purpose");
    require(session.relative_path == "Sessions", "session helper should preserve relative path");

    const auto component = ld::make_component_root_request(
        "compare-plugin",
        ld::component_kind::plugin,
        {
            ld::make_component_config_root_request("config", ld::ownership_kind::user_roaming, "Config"),
            ld::make_component_state_root_request("state", ld::ownership_kind::user_local, "State"),
        });
    require(component.name == "compare-plugin", "component helper should preserve component name");
    require(component.kind == ld::component_kind::plugin, "component helper should preserve component kind");
    require(component.roots.size() == 2, "component helper should preserve root count");
    require(component.roots[0].purpose == ld::purpose_kind::component_config,
        "component helper should preserve child root purpose");
    require(component.roots[1].ownership == ld::ownership_kind::user_local,
        "component helper should preserve child root ownership");
}

void activates_portable_root_from_install_adjacent_marker()
{
    const auto root = test_root();
    const auto install = root / "install";
    const auto marker = install / "portable.marker";
    std::filesystem::create_directories(install);
    {
        std::ofstream marker_file(marker);
        marker_file << "local\n";
    }

    ld::options options;
    options.resource_root = install;
    ld::portable_root_request portable;
    portable.marker = marker;
    portable.level = ld::portable_root_level::profile;
    options.portable_root = portable;

    const auto report = ld::resolve_app_roots(identity(), options);

    require(report.portable_root_requested, "portable marker should be reported as requested");
    require(report.portable_root_active, "existing portable marker should activate portable roots");
    require(report.roots.resources == install, "resource root should remain install-adjacent");
    require(report.roots.config == install, "portable config should live beside the marker");
    require(report.roots.session == install / "sessions", "session root should live under portable state");
}

void explicit_portable_root_does_not_need_marker_file()
{
    const auto root = test_root();
    const auto install = root / "install";
    std::filesystem::create_directories(install);

    ld::portable_root_request portable;
    portable.root = install;
    portable.marker = install / "portable.marker";
    portable.requested = true;

    ld::options options;
    options.resource_root = install;
    options.portable_root = portable;

    const auto report = ld::resolve_app_roots(identity(), options);

    require(report.portable_root_requested, "explicit portable request should be recorded");
    require(report.portable_root_active, "explicit portable request should activate the supplied root");
    require(report.roots.config == install, "explicit portable root should supply config");
    require(report.roots.data == install, "portable profile should supply data by default");
}

void settings_only_portable_root_keeps_machine_local_roots()
{
    const auto root = test_root();
    const auto install = root / "install";
    const auto marker = install / "portable.marker";
    std::filesystem::create_directories(install);
    {
        std::ofstream marker_file(marker);
        marker_file << "local\n";
    }

    ld::portable_root_request portable;
    portable.marker = marker;
    portable.level = ld::portable_root_level::settings_only;

    ld::options options;
    options.resource_root = install;
    options.home_directory = root / "home";
    options.platform_defaults = ld_paths::platform_path_defaults::xdg(root / "home", root / "run");
    options.use_process_environment = false;
    options.portable_root = portable;

    const auto report = ld::resolve_app_roots(identity(), options);

    require(report.portable_root_active, "settings-only portable request should activate");
    require(report.roots.config == install, "settings-only portable root should move config");
    require(report.roots.data != install, "settings-only portable root should keep data on platform roots");
    require(report.roots.state != install, "settings-only portable root should keep state on platform roots");
}

void reports_root_creation_failures()
{
    const auto root = test_root();
    const auto file_root = root / "not-a-directory";
    {
        std::ofstream file(file_root);
        file << "not a directory\n";
    }

    ld::options options;
    options.app_root_override = file_root;
    options.use_process_environment = false;
    options.home_directory = root / "home";

    const auto report = ld::resolve_app_roots(identity(), options);

    require(has_diagnostic(report.diagnostics, "paths.directory.exists_as_file"),
        "root creation failures should come from ld_paths diagnostics");
}

void stringifies_public_root_vocabulary()
{
    require(ld::to_string(ld::portable_root_level::settings_only) == "settings_only",
        "portable-root level should stringify");
    require(ld::to_string(ld::purpose_kind::plugin_config) == "plugin_config", "root purpose should stringify");
    require(ld::to_string(ld::ownership_kind::user_local) == "user_local", "root ownership should stringify");
    require(ld::to_string(ld::component_kind::plugin) == "plugin", "component kind should stringify");
}

} // namespace

int main()
{
    const std::vector<std::pair<const char*, void (*)()>> tests = {
        {"resolves_named_roots", resolves_named_roots},
        {"resolves_component_roots", resolves_component_roots},
        {"builder_preserves_options", builder_preserves_options},
        {"helper_factories_match_explicit_request_shapes", helper_factories_match_explicit_request_shapes},
        {"activates_portable_root_from_install_adjacent_marker", activates_portable_root_from_install_adjacent_marker},
        {"explicit_portable_root_does_not_need_marker_file", explicit_portable_root_does_not_need_marker_file},
        {"settings_only_portable_root_keeps_machine_local_roots", settings_only_portable_root_keeps_machine_local_roots},
        {"reports_root_creation_failures", reports_root_creation_failures},
        {"stringifies_public_root_vocabulary", stringifies_public_root_vocabulary},
    };

    int failures = 0;
    for (const auto& test : tests) {
        try {
            test.second();
            std::cout << "ok " << test.first << "\n";
        } catch (const test_failure& failure) {
            ++failures;
            std::cout << "not ok " << test.first << ": " << failure.message << "\n";
        } catch (const std::exception& ex) {
            ++failures;
            std::cout << "not ok " << test.first << ": unexpected exception: " << ex.what() << "\n";
        }
    }

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
