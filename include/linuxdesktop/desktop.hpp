#pragma once

#include "linuxdesktop/core.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace linuxdesktop::desktop {

inline constexpr int version_major = 0;
inline constexpr int version_minor = 1;
inline constexpr int version_patch = 0;

enum class effect_kind {
    autostart,
    desktop_entry,
    icon,
    mime_association,
    default_application,
    url_protocol_handler,
    shell_integration,
    desktop_database,
    managed_policy
};

enum class capability_state {
    supported,
    unsupported,
    backend_missing,
    backend_limited,
    sandbox_limited,
    permission_denied
};

enum class activation_step_kind {
    refresh_desktop_database,
    refresh_mime_database,
    refresh_icon_cache,
    refresh_dconf_database,
    windows_shell_notify,
    windows_default_apps_ui
};

enum class registration_scope {
    user,
    global
};

enum class registration_status {
    unknown,
    planned,
    staged,
    present,
    missing,
    unsupported,
    failed
};

struct activation_step {
    activation_step_kind kind = activation_step_kind::refresh_desktop_database;
    bool required = false;
    bool can_run = false;
    std::string command_preview;
    std::vector<diagnostic> diagnostics;
};

struct capability {
    effect_kind kind = effect_kind::autostart;
    capability_state state = capability_state::unsupported;
    bool can_query = false;
    bool can_dry_run = false;
    bool can_write_user = false;
    bool can_write_global = false;
    std::vector<diagnostic> diagnostics;
};

struct capability_report {
    std::vector<capability> effects;
    std::vector<diagnostic> diagnostics;
};

struct autostart_entry {
    std::string id;
    std::string display_name;
    std::filesystem::path executable;
    std::vector<std::string> arguments;
    std::filesystem::path working_directory;
    bool enabled = true;
    bool user_scope = true;
};

struct desktop_entry {
    std::string id;
    std::string display_name;
    std::string generic_name;
    std::string comment;
    std::filesystem::path executable;
    std::vector<std::string> arguments;
    std::filesystem::path working_directory;
    std::vector<std::string> categories;
    std::vector<std::string> keywords;
    std::vector<std::string> mime_types;
    bool terminal = false;
    bool user_scope = true;
};

struct desktop_entry_metadata {
    std::string id;
    std::string display_name;
    std::string generic_name;
    std::string comment;
    std::filesystem::path executable;
    std::vector<std::string> arguments;
    std::filesystem::path working_directory;
    std::vector<std::string> categories;
    std::vector<std::string> keywords;
    bool terminal = false;
};

struct icon_entry {
    std::string name;
    std::filesystem::path source_path;
    std::string theme = "hicolor";
    int size = 0;
    bool user_scope = true;
};

struct icon_reference {
    std::string name;
    std::filesystem::path source_path;
    std::string theme = "hicolor";
    std::vector<int> sizes;
};

struct mime_declaration {
    std::string name;
    std::string comment;
    std::vector<std::string> glob_patterns;
    bool user_scope = true;
};

struct mime_association {
    std::string mime_type;
    std::vector<std::string> desktop_entry_ids;
    bool user_scope = true;
};

struct default_application_intent {
    std::string mime_type_or_scheme;
    std::string desktop_entry_id;
    bool make_default = false;
    bool user_scope = true;
};

struct url_scheme_handler {
    std::string scheme;
    std::string desktop_entry_id;
    bool user_scope = true;
};

struct apply_options {
    bool dry_run = true;
    bool allow_global_write = false;
    bool allow_desktop_integration_write = false;
    bool allow_policy_write = false;
    std::optional<std::filesystem::path> autostart_directory_override;
    std::optional<std::filesystem::path> applications_directory_override;
    std::optional<std::filesystem::path> icons_directory_override;
    std::optional<std::filesystem::path> mime_packages_directory_override;
    std::optional<std::filesystem::path> mimeapps_file_override;
    std::optional<std::filesystem::path> policy_defaults_directory_override;
    std::optional<std::filesystem::path> policy_locks_directory_override;
};

struct effect_report {
    effect_kind kind = effect_kind::autostart;
    registration_status status = registration_status::unknown;
    bool ok = false;
    bool dry_run = false;
    bool present = false;
    bool enabled = false;
    std::optional<std::filesystem::path> path;
    std::vector<activation_step> activation_plan;
    std::vector<diagnostic> diagnostics;
};

struct policy_entry {
    std::string id;
    std::string schema_id;
    std::string group;
    std::string key;
    std::string value;
    bool enforced = false;
    bool user_scope = false;
};

struct policy_report {
    effect_kind kind = effect_kind::managed_policy;
    registration_status status = registration_status::unknown;
    bool ok = false;
    bool dry_run = false;
    bool present = false;
    bool enforced = false;
    std::optional<std::filesystem::path> path;
    std::optional<std::string> value;
    std::vector<diagnostic> diagnostics;
};

struct cleanup_rule {
    std::filesystem::path path;
    bool remove_empty_parent = false;
};

struct desktop_bundle {
    registration_scope scope = registration_scope::user;
    std::optional<autostart_entry> autostart;
    std::optional<desktop_entry_metadata> entry;
    std::vector<icon_reference> icons;
    std::vector<mime_declaration> mime_declarations;
    std::vector<mime_association> mime_associations;
    std::vector<default_application_intent> default_applications;
    std::vector<url_scheme_handler> url_scheme_handlers;
    std::vector<policy_entry> policies;
    std::vector<cleanup_rule> cleanup;
};

struct desktop_bundle_report {
    bool ok = false;
    bool dry_run = true;
    std::vector<effect_report> artifact_reports;
    std::vector<policy_report> policy_reports;
    std::vector<activation_step> activation_plan;
    std::vector<cleanup_rule> cleanup_plan;
    std::vector<diagnostic> diagnostics;
};

std::string_view to_string(effect_kind value);
std::string_view to_string(capability_state value);
std::string_view to_string(activation_step_kind value);
std::string_view to_string(registration_scope value);
std::string_view to_string(registration_status value);

capability_report query_capabilities(const apply_options& options = {});

effect_report apply_autostart(const autostart_entry& entry, const apply_options& options = {});
effect_report remove_autostart(const autostart_entry& entry, const apply_options& options = {});
effect_report query_autostart(const autostart_entry& entry, const apply_options& options = {});

effect_report apply_desktop_entry(const desktop_entry& entry, const apply_options& options = {});
effect_report remove_desktop_entry(const desktop_entry& entry, const apply_options& options = {});
effect_report query_desktop_entry(const desktop_entry& entry, const apply_options& options = {});

effect_report apply_icon(const icon_entry& entry, const apply_options& options = {});
effect_report remove_icon(const icon_entry& entry, const apply_options& options = {});
effect_report query_icon(const icon_entry& entry, const apply_options& options = {});

effect_report apply_mime_declaration(const mime_declaration& declaration, const apply_options& options = {});
effect_report remove_mime_declaration(const mime_declaration& declaration, const apply_options& options = {});
effect_report query_mime_declaration(const mime_declaration& declaration, const apply_options& options = {});

effect_report apply_mime_association(const mime_association& association, const apply_options& options = {});
effect_report remove_mime_association(const mime_association& association, const apply_options& options = {});
effect_report query_mime_association(const mime_association& association, const apply_options& options = {});

effect_report apply_default_application(const default_application_intent& intent, const apply_options& options = {});
effect_report remove_default_application(const default_application_intent& intent, const apply_options& options = {});
effect_report query_default_application(const default_application_intent& intent, const apply_options& options = {});

effect_report apply_url_scheme_handler(const url_scheme_handler& handler, const apply_options& options = {});
effect_report remove_url_scheme_handler(const url_scheme_handler& handler, const apply_options& options = {});
effect_report query_url_scheme_handler(const url_scheme_handler& handler, const apply_options& options = {});

policy_report apply_policy(const policy_entry& entry, const apply_options& options = {});
policy_report remove_policy(const policy_entry& entry, const apply_options& options = {});
policy_report query_policy(const policy_entry& entry, const apply_options& options = {});

desktop_bundle_report plan_bundle(const desktop_bundle& bundle, const apply_options& options = {});
desktop_bundle_report apply_bundle(const desktop_bundle& bundle, const apply_options& options = {});
desktop_bundle_report query_bundle(const desktop_bundle& bundle, const apply_options& options = {});
desktop_bundle_report remove_bundle(const desktop_bundle& bundle, const apply_options& options = {});

} // namespace linuxdesktop::desktop
