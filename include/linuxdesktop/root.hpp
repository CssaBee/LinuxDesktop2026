#pragma once

#include "linuxdesktop/core.hpp"
#include "linuxdesktop/paths.hpp"
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace linuxdesktop::root {

inline constexpr int version_major = 0;
inline constexpr int version_minor = 1;
inline constexpr int version_patch = 0;

struct app_identity {
    std::string organization;
    std::string application;
};

enum class portable_root_level {
    off,
    settings_only,
    profile,
    clean
};

enum class purpose_kind {
    resources,
    config,
    data,
    state,
    cache,
    runtime,
    session,
    plugin_config,
    logs,
    profiles,
    backup,
    temp,
    component_config,
    component_data,
    component_state,
    managed_config,
    enforced_config,
    custom
};

enum class ownership_kind {
    user_roaming,
    user_local,
    app_local,
    ephemeral,
    managed,
    enforced
};

enum class component_kind {
    plugin,
    embedded_tool,
    profile,
    language_pack,
    extension,
    custom
};

struct named_root_request {
    std::string name;
    purpose_kind purpose = purpose_kind::custom;
    ownership_kind ownership = ownership_kind::user_roaming;
    std::filesystem::path relative_path;
    bool create = true;
};

struct named_root {
    std::string name;
    purpose_kind purpose = purpose_kind::custom;
    ownership_kind ownership = ownership_kind::user_roaming;
    std::filesystem::path path;
    bool created = false;
    std::vector<diagnostic> diagnostics;
};

struct component_root_request {
    std::string name;
    component_kind kind = component_kind::custom;
    std::vector<named_root_request> roots;
};

struct component_root_group {
    std::string name;
    component_kind kind = component_kind::custom;
    std::vector<named_root> roots;
    std::vector<diagnostic> diagnostics;
};

struct portable_root_request {
    std::optional<std::filesystem::path> root;
    std::optional<std::filesystem::path> marker;
    bool requested = false;
    portable_root_level level = portable_root_level::profile;
    bool allow = true;
    bool deny_in_privileged_install = false;
    bool allow_user_config_override = false;
    std::vector<std::filesystem::path> privileged_install_roots;
};

inline named_root_request make_named_root_request(
    std::string name,
    purpose_kind purpose,
    ownership_kind ownership = ownership_kind::user_roaming,
    std::filesystem::path relative_path = {},
    bool create = true)
{
    return {std::move(name), purpose, ownership, std::move(relative_path), create};
}

inline named_root_request make_config_root_request(
    std::string name,
    ownership_kind ownership = ownership_kind::user_roaming,
    std::filesystem::path relative_path = {},
    bool create = true)
{
    return make_named_root_request(std::move(name), purpose_kind::config, ownership, std::move(relative_path), create);
}

inline named_root_request make_state_root_request(
    std::string name,
    ownership_kind ownership = ownership_kind::user_roaming,
    std::filesystem::path relative_path = {},
    bool create = true)
{
    return make_named_root_request(std::move(name), purpose_kind::state, ownership, std::move(relative_path), create);
}

inline named_root_request make_cache_root_request(
    std::string name,
    ownership_kind ownership = ownership_kind::user_roaming,
    std::filesystem::path relative_path = {},
    bool create = true)
{
    return make_named_root_request(std::move(name), purpose_kind::cache, ownership, std::move(relative_path), create);
}

inline named_root_request make_session_root_request(
    std::string name,
    ownership_kind ownership = ownership_kind::user_local,
    std::filesystem::path relative_path = {},
    bool create = true)
{
    return make_named_root_request(std::move(name), purpose_kind::session, ownership, std::move(relative_path), create);
}

inline named_root_request make_log_root_request(
    std::string name,
    ownership_kind ownership = ownership_kind::user_local,
    std::filesystem::path relative_path = {},
    bool create = true)
{
    return make_named_root_request(std::move(name), purpose_kind::logs, ownership, std::move(relative_path), create);
}

inline named_root_request make_profiles_root_request(
    std::string name,
    ownership_kind ownership = ownership_kind::user_roaming,
    std::filesystem::path relative_path = {},
    bool create = true)
{
    return make_named_root_request(std::move(name), purpose_kind::profiles, ownership, std::move(relative_path), create);
}

inline named_root_request make_plugin_config_root_request(
    std::string name,
    ownership_kind ownership = ownership_kind::user_roaming,
    std::filesystem::path relative_path = {},
    bool create = true)
{
    return make_named_root_request(std::move(name), purpose_kind::plugin_config, ownership, std::move(relative_path), create);
}

inline named_root_request make_component_config_root_request(
    std::string name,
    ownership_kind ownership = ownership_kind::user_roaming,
    std::filesystem::path relative_path = {},
    bool create = true)
{
    return make_named_root_request(std::move(name), purpose_kind::component_config, ownership, std::move(relative_path), create);
}

inline named_root_request make_component_data_root_request(
    std::string name,
    ownership_kind ownership = ownership_kind::user_roaming,
    std::filesystem::path relative_path = {},
    bool create = true)
{
    return make_named_root_request(std::move(name), purpose_kind::component_data, ownership, std::move(relative_path), create);
}

inline named_root_request make_component_state_root_request(
    std::string name,
    ownership_kind ownership = ownership_kind::user_local,
    std::filesystem::path relative_path = {},
    bool create = true)
{
    return make_named_root_request(std::move(name), purpose_kind::component_state, ownership, std::move(relative_path), create);
}

inline component_root_request make_component_root_request(
    std::string name,
    component_kind kind = component_kind::custom,
    std::vector<named_root_request> roots = {})
{
    return {std::move(name), kind, std::move(roots)};
}

struct options {
    std::optional<std::filesystem::path> resource_root;
    std::optional<std::filesystem::path> home_directory;
    std::optional<linuxdesktop::paths::platform_path_defaults> platform_defaults;
    std::map<std::string, std::string> environment;
    std::optional<std::filesystem::path> app_root_override;
    std::optional<std::filesystem::path> user_config_override;
    std::optional<portable_root_request> portable_root;
    bool create_directories = true;
    bool use_process_environment = true;
    std::vector<named_root_request> named_roots;
    std::vector<component_root_request> component_roots;
};

struct app_roots {
    std::filesystem::path resources;
    std::filesystem::path config;
    std::filesystem::path data;
    std::filesystem::path state;
    std::filesystem::path cache;
    std::filesystem::path runtime;
    std::filesystem::path session;
    std::filesystem::path plugin_config;
};

struct report {
    app_roots roots;
    bool portable_root_requested = false;
    bool portable_root_active = false;
    bool app_root_override_active = false;
    bool user_config_override_active = false;
    portable_root_level portable_root = portable_root_level::off;
    std::vector<named_root> named_roots;
    std::vector<component_root_group> component_roots;
    std::vector<diagnostic> diagnostics;
};

class request_builder {
public:
    request_builder() = default;
    explicit request_builder(app_identity identity) : identity_(std::move(identity)) {}

    request_builder& app(std::string organization, std::string application);
    request_builder& resource_root(std::filesystem::path path);
    request_builder& home_directory(std::optional<std::filesystem::path> path);
    request_builder& platform_defaults(std::optional<linuxdesktop::paths::platform_path_defaults> defaults);
    request_builder& environment(std::map<std::string, std::string> values);
    request_builder& use_process_environment(bool enabled);
    request_builder& app_root_override(std::optional<std::filesystem::path> path);
    request_builder& user_config_override(std::optional<std::filesystem::path> path);
    request_builder& portable_root(portable_root_request request);
    request_builder& create_directories(bool enabled);
    request_builder& named_root(named_root_request request);
    request_builder& component_roots(component_root_request request);

    const app_identity& identity() const { return identity_; }
    const options& current_options() const { return options_; }
    options build() const { return options_; }
    report resolve() const;

private:
    app_identity identity_;
    options options_;
};

std::string_view to_string(portable_root_level value);
std::string_view to_string(purpose_kind value);
std::string_view to_string(ownership_kind value);
std::string_view to_string(component_kind value);

const named_root* find_named_root(const report& report, const std::string& name);
const component_root_group* find_component_roots(const report& report, const std::string& name);
const named_root* find_component_named_root(const component_root_group& component, const std::string& name);

report resolve_app_roots(const app_identity& identity, const options& options = {});

inline report request_builder::resolve() const
{
    return resolve_app_roots(identity_, options_);
}

} // namespace linuxdesktop::root
