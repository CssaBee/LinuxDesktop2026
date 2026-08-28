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

std::string_view to_string(path_family value);
std::string_view to_string(candidate_source value);

} // namespace linuxdesktop::paths
