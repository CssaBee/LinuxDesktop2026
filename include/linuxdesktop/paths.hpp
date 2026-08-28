#pragma once

#include "linuxdesktop/core.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace linuxdesktop::paths {

inline constexpr int version_major = 0;
inline constexpr int version_minor = 1;
inline constexpr int version_patch = 0;

using ::linuxdesktop::diagnostic;
using ::linuxdesktop::severity;
using ::linuxdesktop::to_string;

namespace diagnostic_code {
inline constexpr std::string_view application_missing = "paths.identity.application_missing";
inline constexpr std::string_view home_missing = "paths.home.missing";
inline constexpr std::string_view environment_relative_ignored = "paths.environment.relative_ignored";
inline constexpr std::string_view override_relative_ignored = "paths.override.relative_ignored";
inline constexpr std::string_view executable_unavailable = "paths.executable.unavailable";
inline constexpr std::string_view temp_directory_unavailable = "paths.temp.unavailable";
inline constexpr std::string_view directory_exists_as_file = "paths.directory.exists_as_file";
inline constexpr std::string_view directory_parent_missing = "paths.directory.parent_missing";
inline constexpr std::string_view directory_create_failed = "paths.directory.create_failed";
inline constexpr std::string_view path_list_relative_ignored = "paths.path_list.relative_ignored";
inline constexpr std::string_view path_list_empty_entry_ignored = "paths.path_list.empty_entry_ignored";
inline constexpr std::string_view path_list_duplicate_ignored = "paths.path_list.duplicate_ignored";
inline constexpr std::string_view xdg_user_dir_malformed = "paths.xdg_user_dir.malformed";
inline constexpr std::string_view xdg_user_dir_relative_ignored = "paths.xdg_user_dir.relative_ignored";
inline constexpr std::string_view xdg_user_dir_unreadable = "paths.xdg_user_dir.unreadable";
inline constexpr std::string_view legacy_path_relative_ignored = "paths.legacy.relative_ignored";
} // namespace diagnostic_code

struct app_identity {
    std::string organization;
    std::string application;
};

enum class path_family {
    config,
    data,
    state,
    cache,
    temp,
    documents,
    desktop,
    downloads,
    music,
    pictures,
    videos,
    templates,
    public_share,
    executable,
    executable_directory,
    install_prefix,
    resources,
    plugin_search
};

enum class candidate_source {
    explicit_option,
    environment,
    xdg_base_dir,
    xdg_user_dir,
    known_folder,
    executable_relative,
    legacy,
    site_default,
    fallback
};

struct path_candidate {
    path_family family = path_family::config;
    candidate_source source = candidate_source::fallback;
    std::filesystem::path path;
    bool selected = false;
    std::vector<diagnostic> diagnostics;
};

struct resolver_options {
    std::optional<std::filesystem::path> config_override;
    std::optional<std::filesystem::path> data_override;
    std::optional<std::filesystem::path> state_override;
    std::optional<std::filesystem::path> cache_override;
    std::optional<std::filesystem::path> temp_override;
    std::optional<std::filesystem::path> resource_root;
    std::optional<std::filesystem::path> install_prefix;
    std::optional<std::filesystem::path> executable_path;
    std::optional<std::filesystem::path> home_directory;
    std::map<std::string, std::string> environment;
    std::vector<std::filesystem::path> legacy_config_files;
    bool use_process_environment = true;
};

struct resolver_report {
    std::map<path_family, std::filesystem::path> selected;
    std::vector<path_candidate> candidates;
    std::vector<diagnostic> diagnostics;
};

resolver_report resolve_app_paths(const app_identity& identity, const resolver_options& options = {});

enum class directory_action {
    already_exists,
    would_create,
    created,
    failed
};

struct ensure_directory_options {
    bool dry_run = true;
    bool create_parents = true;
};

struct ensure_directory_report {
    std::filesystem::path path;
    directory_action action = directory_action::failed;
    std::vector<diagnostic> diagnostics;
};

ensure_directory_report ensure_directory(
    const std::filesystem::path& path,
    const ensure_directory_options& options = {});

ensure_directory_report ensure_directory(
    const resolver_report& report,
    path_family family,
    const ensure_directory_options& options = {});

struct path_list_options {
    bool require_absolute = true;
    bool drop_duplicates = true;
    std::optional<char> separator;
};

struct path_list_report {
    std::vector<std::filesystem::path> paths;
    std::vector<path_candidate> candidates;
    std::vector<diagnostic> diagnostics;
};

path_list_report parse_path_list(
    std::string_view value,
    const path_list_options& options = {});

std::string join_path_list(
    const std::vector<std::filesystem::path>& paths,
    const path_list_options& options = {});

enum class plugin_path_kind {
    ladspa,
    dssi,
    lv2,
    vst2,
    vst3,
    clap,
    sf2,
    sfz,
    jsfx
};

struct custom_plugin_path_set {
    std::string name;
    std::optional<std::string> environment_variable;
    std::vector<std::filesystem::path> defaults;
};

struct plugin_path_options {
    std::vector<plugin_path_kind> kinds;
    std::vector<custom_plugin_path_set> custom_sets;
    std::map<std::string, std::string> environment;
    std::optional<std::filesystem::path> home_directory;
    std::optional<std::filesystem::path> wine_prefix;
    bool use_process_environment = true;
    bool include_wine_prefix_defaults = false;
    path_list_options list_options;
};

struct plugin_path_set {
    std::string name;
    std::optional<plugin_path_kind> kind;
    std::vector<std::filesystem::path> paths;
};

struct plugin_path_report {
    std::vector<plugin_path_set> sets;
    std::vector<path_candidate> candidates;
    std::vector<diagnostic> diagnostics;
};

plugin_path_report resolve_plugin_path_sets(const plugin_path_options& options = {});

std::string_view to_string(path_family value);
std::string_view to_string(candidate_source value);
std::string_view to_string(directory_action value);
std::string_view to_string(plugin_path_kind value);

} // namespace linuxdesktop::paths
