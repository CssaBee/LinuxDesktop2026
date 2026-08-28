#include "linuxdesktop/paths.hpp"

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <fstream>
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

bool has_path_list_diagnostic(const ld::path_list_report& report, std::string_view code)
{
    return has_diagnostic(report.diagnostics, code);
}

bool has_plugin_diagnostic(const ld::plugin_path_report& report, std::string_view code)
{
    return has_diagnostic(report.diagnostics, code);
}

const ld::plugin_path_set& plugin_set(const ld::plugin_path_report& report, std::string_view name)
{
    for (const auto& set : report.sets) {
        if (set.name == name) {
            return set;
        }
    }
    fail(std::string("missing plugin path set: ") + std::string(name));
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
    require(ld::to_string(ld::directory_action::would_create) == "would_create", "directory action should stringify");
    require(ld::to_string(ld::plugin_path_kind::vst3) == "vst3", "plugin path kind should stringify");
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

void ensures_directories_only_when_requested()
{
    const auto base = std::filesystem::temp_directory_path() / "linuxdesktop2026-paths-ensure-tests";
    std::error_code ec;
    std::filesystem::remove_all(base, ec);

    const auto target = base / "parent" / "leaf";

    auto dry_run = ld::ensure_directory(target);
    require(dry_run.action == ld::directory_action::would_create, "default directory ensure should be dry-run only");
    require(!std::filesystem::exists(target), "dry-run should not create directories");

    ld::ensure_directory_options no_parents;
    no_parents.dry_run = false;
    no_parents.create_parents = false;
    auto missing_parent = ld::ensure_directory(target, no_parents);
    require(missing_parent.action == ld::directory_action::failed,
        "directory ensure should fail when parent creation is disabled");
    require(has_diagnostic(missing_parent.diagnostics, ld::diagnostic_code::directory_parent_missing),
        "missing parent should be diagnosed");

    ld::ensure_directory_options create;
    create.dry_run = false;
    auto created = ld::ensure_directory(target, create);
    require(created.action == ld::directory_action::created, "directory ensure should create when requested");
    require(std::filesystem::is_directory(target), "requested directory should exist");

    auto exists = ld::ensure_directory(target, create);
    require(exists.action == ld::directory_action::already_exists, "existing directory should be reported");

    const auto file_path = base / "file";
    {
        std::ofstream file(file_path);
        file << "not a directory";
    }
    auto file = ld::ensure_directory(file_path, create);
    require(file.action == ld::directory_action::failed, "file path should not be treated as directory");
    require(has_diagnostic(file.diagnostics, ld::diagnostic_code::directory_exists_as_file),
        "existing file should be diagnosed");

    std::filesystem::remove_all(base, ec);
}

void ensures_directory_from_resolver_family()
{
    ld::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-tests";
    const auto resolved = ld::resolve_app_paths(identity, deterministic_options());

    auto report = ld::ensure_directory(resolved, ld::path_family::config);
    require(report.action == ld::directory_action::would_create,
        "resolver-family directory ensure should use selected path");
    require(report.path == "/home/tester/.config/LinuxDesktop2026/paths-tests",
        "resolver-family directory ensure should preserve selected path");

    ld::resolver_report empty;
    auto missing = ld::ensure_directory(empty, ld::path_family::config);
    require(missing.action == ld::directory_action::failed, "unresolved family should fail");
    require(has_diagnostic(missing.diagnostics, "paths.directory.family_unresolved"),
        "unresolved family should be diagnosed");
}

void parses_and_joins_path_lists_with_diagnostics()
{
    ld::path_list_options options;
    options.separator = ':';

    const auto report = ld::parse_path_list("/one:/two/../two:relative::/one", options);

    require(report.paths.size() == 2, "path list should keep absolute unique entries");
    require(report.paths[0] == "/one", "first path should be preserved");
    require(report.paths[1] == "/two", "second path should be normalized");
    require(has_path_list_diagnostic(report, ld::diagnostic_code::path_list_relative_ignored),
        "relative path-list entries should be diagnosed");
    require(has_path_list_diagnostic(report, ld::diagnostic_code::path_list_empty_entry_ignored),
        "empty path-list entries should be diagnosed");
    require(has_path_list_diagnostic(report, ld::diagnostic_code::path_list_duplicate_ignored),
        "duplicate path-list entries should be diagnosed");
    require(ld::join_path_list(report.paths, options) == "/one:/two",
        "path-list join should use the selected separator");
}

void resolves_typed_plugin_path_sets()
{
    ld::plugin_path_options options;
    options.use_process_environment = false;
    options.home_directory = "/home/tester";
    options.kinds = {ld::plugin_path_kind::vst3, ld::plugin_path_kind::lv2};
    options.environment["VST3_PATH"] = "/vendor/vst3:/vendor/vst3/../vst3:relative";

    const auto report = ld::resolve_plugin_path_sets(options);

    const auto& vst3 = plugin_set(report, "vst3");
    require(vst3.kind && *vst3.kind == ld::plugin_path_kind::vst3,
        "typed plugin set should preserve its kind");
    require(vst3.paths.size() == 4, "VST3 should include env path and Linux defaults");
    require(vst3.paths[0] == "/vendor/vst3", "environment plugin path should win ordering");
    require(vst3.paths[1] == "/home/tester/.vst3", "VST3 should include home default");
    require(has_plugin_diagnostic(report, ld::diagnostic_code::path_list_relative_ignored),
        "relative plugin environment entries should be diagnosed");
    require(has_plugin_diagnostic(report, ld::diagnostic_code::path_list_duplicate_ignored),
        "duplicate plugin entries should be diagnosed");

    const auto& lv2 = plugin_set(report, "lv2");
    require(lv2.paths[0] == "/home/tester/.lv2", "LV2 should include home default");
    require(lv2.paths[1] == "/usr/local/lib/lv2", "LV2 should include local system default");
}

void resolves_wine_and_custom_plugin_path_sets()
{
    ld::plugin_path_options options;
    options.use_process_environment = false;
    options.home_directory = "/home/tester";
    options.wine_prefix = "/wine/prefix";
    options.include_wine_prefix_defaults = true;
    options.kinds = {ld::plugin_path_kind::vst2, ld::plugin_path_kind::clap};

    ld::custom_plugin_path_set custom;
    custom.name = "sampler-bank";
    custom.environment_variable = "SAMPLER_BANK_PATH";
    custom.defaults = {"/opt/sampler/banks"};
    options.custom_sets = {custom};
    options.environment["SAMPLER_BANK_PATH"] = "/library/banks";

    const auto report = ld::resolve_plugin_path_sets(options);

    const auto& vst2 = plugin_set(report, "vst2");
    require(std::find(vst2.paths.begin(), vst2.paths.end(), "/wine/prefix/drive_c/Program Files/VstPlugins") != vst2.paths.end(),
        "VST2 should include Wine-prefix default when requested");

    const auto& clap = plugin_set(report, "clap");
    require(std::find(clap.paths.begin(), clap.paths.end(), "/wine/prefix/drive_c/Program Files/Common Files/CLAP") != clap.paths.end(),
        "CLAP should include Wine-prefix default when requested");

    const auto& sampler = plugin_set(report, "sampler-bank");
    require(!sampler.kind, "custom plugin set should not claim a built-in kind");
    require(sampler.paths.size() == 2, "custom plugin set should include environment and defaults");
    require(sampler.paths[0] == "/library/banks", "custom environment path should be first");
    require(sampler.paths[1] == "/opt/sampler/banks", "custom default path should follow");
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
        ensures_directories_only_when_requested();
        ensures_directory_from_resolver_family();
        parses_and_joins_path_lists_with_diagnostics();
        resolves_typed_plugin_path_sets();
        resolves_wine_and_custom_plugin_path_sets();
    } catch (const test_failure& failure) {
        std::cerr << failure.message << "\n";
        return EXIT_FAILURE;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
