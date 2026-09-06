#include "linuxdesktop/desktop.hpp"

#include "durable_file_write.hpp"
#include "linuxdesktop/paths.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <system_error>
#include <utility>

namespace linuxdesktop::desktop {
namespace {

namespace ld_paths = linuxdesktop::paths;

diagnostic make_diagnostic(severity level, std::string code, std::string message, std::filesystem::path path = {})
{
    return diagnostic{level, std::move(code), std::move(message), std::move(path)};
}

bool has_error(const std::vector<diagnostic>& diagnostics)
{
    return std::any_of(diagnostics.begin(), diagnostics.end(), [](const diagnostic& item) {
        return item.level == severity::error;
    });
}

std::string sanitize_segment(std::string value, std::string fallback = {})
{
    for (char& ch : value) {
        if (ch == '/' || ch == '\\' || ch == ':' || ch == '\0') {
            ch = '-';
        }
    }
    return value.empty() ? fallback : value;
}

bool write_file_content(
    const std::filesystem::path& target,
    const std::string& content,
    std::vector<diagnostic>& diagnostics)
{
    ::linuxdesktop::detail::durable_file_write_options options;
    options.target = target;
    options.content = content;
    options.keep_backup = false;
    options.atomic_replace = true;
    options.durable_write = true;

    auto report = ::linuxdesktop::detail::write_durable_file(options);
    diagnostics.insert(diagnostics.end(), report.diagnostics.begin(), report.diagnostics.end());
    return report.ok;
}

std::string read_text(const std::filesystem::path& path, std::error_code& ec)
{
    return ::linuxdesktop::detail::read_text_file(path, ec);
}

std::filesystem::path config_base_directory(std::vector<diagnostic>& diagnostics)
{
    ld_paths::app_identity identity;
    identity.application = "linuxdesktop2026-desktop-base";
    const auto report = ld_paths::resolve_app_paths(identity);
    diagnostics.insert(diagnostics.end(), report.diagnostics.begin(), report.diagnostics.end());
    const auto selected = report.selected.find(ld_paths::path_family::config);
    if (selected == report.selected.end() || selected->second.empty()) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "desktop.config-base-unavailable",
            "Cannot resolve desktop integration config base through ld_paths"));
        return {};
    }
    return selected->second.parent_path();
}

std::filesystem::path data_base_directory(std::vector<diagnostic>& diagnostics)
{
    ld_paths::app_identity identity;
    identity.application = "linuxdesktop2026-desktop-base";
    const auto report = ld_paths::resolve_app_paths(identity);
    diagnostics.insert(diagnostics.end(), report.diagnostics.begin(), report.diagnostics.end());
    const auto selected = report.selected.find(ld_paths::path_family::data);
    if (selected == report.selected.end() || selected->second.empty()) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "desktop.data-base-unavailable",
            "Cannot resolve desktop integration data base through ld_paths"));
        return {};
    }
    return selected->second.parent_path();
}

bool require_absolute_directory_override(
    const std::optional<std::filesystem::path>& override_path,
    std::string code,
    std::string message,
    std::vector<diagnostic>& diagnostics)
{
    if (!override_path || override_path->is_absolute()) {
        return true;
    }
    diagnostics.push_back(make_diagnostic(severity::error, std::move(code), std::move(message), *override_path));
    return false;
}

bool is_safe_desktop_id(std::string_view value)
{
    if (value.empty() || value.front() == '.') {
        return false;
    }
    for (const char ch : value) {
        const auto uch = static_cast<unsigned char>(ch);
        if (!std::isalnum(uch) && ch != '.' && ch != '-' && ch != '_') {
            return false;
        }
    }
    return true;
}

bool is_safe_token(std::string_view value)
{
    if (value.empty() || value.front() == '.' || value.find("..") != std::string_view::npos) {
        return false;
    }
    for (const char ch : value) {
        const auto uch = static_cast<unsigned char>(ch);
        if (!std::isalnum(uch) && ch != '.' && ch != '-' && ch != '_') {
            return false;
        }
    }
    return true;
}

bool is_safe_mime_name(std::string_view value)
{
    const auto slash = value.find('/');
    if (slash == std::string_view::npos || slash == 0 || slash + 1 == value.size()) {
        return false;
    }
    const auto is_mime_component = [](std::string_view component) {
        if (component.empty() || component.front() == '.' || component.find("..") != std::string_view::npos) {
            return false;
        }
        for (const char ch : component) {
            const auto uch = static_cast<unsigned char>(ch);
            if (!std::isalnum(uch) && ch != '.' && ch != '-' && ch != '_' && ch != '+') {
                return false;
            }
        }
        return true;
    };
    return is_mime_component(value.substr(0, slash)) && is_mime_component(value.substr(slash + 1));
}

bool is_safe_scheme(std::string_view value)
{
    if (value.empty() || !std::isalpha(static_cast<unsigned char>(value.front()))) {
        return false;
    }
    for (const char ch : value) {
        const auto uch = static_cast<unsigned char>(ch);
        if (!std::isalnum(uch) && ch != '+' && ch != '-' && ch != '.') {
            return false;
        }
    }
    return true;
}

std::string ensure_desktop_suffix(const std::string& id)
{
    constexpr std::string_view suffix = ".desktop";
    if (id.size() >= suffix.size() && std::string_view(id).substr(id.size() - suffix.size()) == suffix) {
        return id;
    }
    return id + std::string(suffix);
}

std::string desktop_escape(const std::string& value);

std::string join_desktop_list(const std::vector<std::string>& values)
{
    std::string result;
    for (const auto& value : values) {
        if (value.empty()) {
            continue;
        }
        result += desktop_escape(value);
        result += ';';
    }
    return result;
}

std::filesystem::path applications_directory(bool user_scope, const apply_options& options, std::vector<diagnostic>& diagnostics)
{
    if (!require_absolute_directory_override(
            options.applications_directory_override,
            "desktop-entry-directory-relative",
            "Desktop entry applications directory override must be absolute",
            diagnostics)) {
        return {};
    }
    if (options.applications_directory_override) {
        return *options.applications_directory_override;
    }
    if (!user_scope) {
        return std::filesystem::path("/usr/local/share/applications");
    }
    const auto data_base = data_base_directory(diagnostics);
    return data_base.empty() ? std::filesystem::path{} : data_base / "applications";
}

std::filesystem::path icons_directory(bool user_scope, const apply_options& options, std::vector<diagnostic>& diagnostics)
{
    if (!require_absolute_directory_override(
            options.icons_directory_override,
            "icon-directory-relative",
            "Icon directory override must be absolute",
            diagnostics)) {
        return {};
    }
    if (options.icons_directory_override) {
        return *options.icons_directory_override;
    }
    if (!user_scope) {
        return std::filesystem::path("/usr/local/share/icons");
    }
    const auto data_base = data_base_directory(diagnostics);
    return data_base.empty() ? std::filesystem::path{} : data_base / "icons";
}

std::filesystem::path mime_packages_directory(bool user_scope, const apply_options& options, std::vector<diagnostic>& diagnostics)
{
    if (!require_absolute_directory_override(
            options.mime_packages_directory_override,
            "mime-package-directory-relative",
            "MIME package directory override must be absolute",
            diagnostics)) {
        return {};
    }
    if (options.mime_packages_directory_override) {
        return *options.mime_packages_directory_override;
    }
    if (!user_scope) {
        return std::filesystem::path("/usr/local/share/mime/packages");
    }
    const auto data_base = data_base_directory(diagnostics);
    return data_base.empty() ? std::filesystem::path{} : data_base / "mime" / "packages";
}

std::filesystem::path mimeapps_file(bool user_scope, const apply_options& options, std::vector<diagnostic>& diagnostics)
{
    if (!require_absolute_directory_override(
            options.mimeapps_file_override,
            "mimeapps-file-relative",
            "mimeapps.list override must be absolute",
            diagnostics)) {
        return {};
    }
    if (options.mimeapps_file_override) {
        return *options.mimeapps_file_override;
    }
    if (!user_scope) {
        return std::filesystem::path("/usr/local/share/applications/mimeapps.list");
    }
    const auto config_base = config_base_directory(diagnostics);
    return config_base.empty() ? std::filesystem::path{} : config_base / "mimeapps.list";
}

activation_step activation_required(activation_step_kind kind, std::string command, std::string code, std::string message)
{
    activation_step step;
    step.kind = kind;
    step.required = true;
    step.can_run = false;
    step.command_preview = std::move(command);
    step.diagnostics.push_back(make_diagnostic(severity::warning, std::move(code), std::move(message)));
    return step;
}

std::string sanitize_autostart_id(const std::string& value, std::vector<diagnostic>& diagnostics)
{
    auto sanitized = sanitize_segment(value, "application");
    if (sanitized != value) {
        diagnostics.push_back(make_diagnostic(
            severity::warning,
            "autostart-id-sanitized",
            "Autostart id contained filename separators and was sanitized"));
    }
    return sanitized + ".desktop";
}

std::string desktop_escape(const std::string& value)
{
    std::string escaped;
    for (const char ch : value) {
        switch (ch) {
        case '\\':
            escaped += "\\\\";
            break;
        case '\n':
            escaped += "\\n";
            break;
        default:
            escaped.push_back(ch);
            break;
        }
    }
    return escaped;
}

std::string shell_quote(const std::string& value)
{
    if (value.empty()) {
        return "''";
    }
    bool simple = true;
    for (const char ch : value) {
        if (!std::isalnum(static_cast<unsigned char>(ch)) && ch != '/' && ch != '.' && ch != '_' && ch != '-' && ch != ':') {
            simple = false;
            break;
        }
    }
    if (simple) {
        return value;
    }
    std::string quoted = "'";
    for (const char ch : value) {
        if (ch == '\'') {
            quoted += "'\\''";
        } else {
            quoted.push_back(ch);
        }
    }
    quoted += "'";
    return quoted;
}

std::string autostart_command(const autostart_entry& entry)
{
    std::string command = shell_quote(entry.executable.string());
    for (const auto& argument : entry.arguments) {
        command += " ";
        command += shell_quote(argument);
    }
    return command;
}

void append_autostart_validation(const autostart_entry& entry, std::vector<diagnostic>& diagnostics)
{
    if (entry.id.empty()) {
        diagnostics.push_back(make_diagnostic(severity::error, "autostart-id-empty", "Autostart entry requires a stable id"));
    }
    if (entry.display_name.empty()) {
        diagnostics.push_back(make_diagnostic(severity::error, "autostart-display-name-empty", "Autostart entry requires a display name"));
    }
    if (entry.executable.empty()) {
        diagnostics.push_back(make_diagnostic(severity::error, "autostart-executable-empty", "Autostart entry requires an executable path"));
    }
}

std::filesystem::path user_autostart_directory(std::vector<diagnostic>& diagnostics)
{
    const auto config_base = config_base_directory(diagnostics);
    if (config_base.empty()) {
        return {};
    }
    return config_base / "autostart";
}

std::filesystem::path autostart_directory(const autostart_entry& entry, const apply_options& options, std::vector<diagnostic>& diagnostics)
{
    if (options.autostart_directory_override.has_value()) {
        if (!options.autostart_directory_override->is_absolute()) {
            diagnostics.push_back(make_diagnostic(
                severity::error,
                "autostart-directory-relative",
                "Autostart directory override must be absolute",
                *options.autostart_directory_override));
            return {};
        }
        return *options.autostart_directory_override;
    }
    if (!entry.user_scope) {
        return std::filesystem::path("/etc/xdg/autostart");
    }
    return user_autostart_directory(diagnostics);
}

std::filesystem::path autostart_path(const autostart_entry& entry, const apply_options& options, std::vector<diagnostic>& diagnostics)
{
    const auto directory = autostart_directory(entry, options, diagnostics);
    if (directory.empty()) {
        return {};
    }
    return directory / sanitize_autostart_id(entry.id, diagnostics);
}

std::string desktop_file_content(const autostart_entry& entry)
{
    std::ostringstream output;
    output << "[Desktop Entry]\n";
    output << "Type=Application\n";
    output << "Name=" << desktop_escape(entry.display_name) << "\n";
    output << "Exec=" << desktop_escape(autostart_command(entry)) << "\n";
    if (!entry.working_directory.empty()) {
        output << "Path=" << desktop_escape(entry.working_directory.string()) << "\n";
    }
    output << "Terminal=false\n";
    if (!entry.enabled) {
        output << "Hidden=true\n";
    }
    output << "X-LinuxDesktop2026-Autostart=true\n";
    return output.str();
}

bool desktop_file_hidden(const std::string& content)
{
    std::istringstream input(content);
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line == "Hidden=true" || line == "Hidden=True" || line == "Hidden=1") {
            return true;
        }
    }
    return false;
}

std::string sanitize_policy_file_id(const std::string& value, std::vector<diagnostic>& diagnostics)
{
    auto sanitized = sanitize_segment(value, "policy");
    if (sanitized != value) {
        diagnostics.push_back(make_diagnostic(
            severity::warning,
            "policy-id-sanitized",
            "Policy id contained filename separators and was sanitized"));
    }
    return sanitized + ".conf";
}

std::string policy_group_name(const policy_entry& entry)
{
    if (!entry.group.empty()) {
        return entry.group;
    }
    return entry.schema_id;
}

void append_policy_validation(const policy_entry& entry, bool require_value, std::vector<diagnostic>& diagnostics)
{
    if (entry.id.empty()) {
        diagnostics.push_back(make_diagnostic(severity::error, "policy-id-empty", "Policy entry requires a stable id"));
    }
    if (entry.schema_id.empty()) {
        diagnostics.push_back(make_diagnostic(severity::error, "policy-schema-empty", "Policy entry requires a schema id"));
    }
    if (policy_group_name(entry).empty()) {
        diagnostics.push_back(make_diagnostic(severity::error, "policy-group-empty", "Policy entry requires a dconf/GSettings group"));
    }
    if (entry.key.empty()) {
        diagnostics.push_back(make_diagnostic(severity::error, "policy-key-empty", "Policy entry requires a key"));
    }
    if (require_value && entry.value.empty()) {
        diagnostics.push_back(make_diagnostic(severity::error, "policy-value-empty", "Policy entry requires a backend-ready value literal"));
    }
}

std::string dconf_group_from_policy(const policy_entry& entry)
{
    auto group = policy_group_name(entry);
    std::replace(group.begin(), group.end(), '.', '/');
    while (!group.empty() && group.front() == '/') {
        group.erase(group.begin());
    }
    while (!group.empty() && group.back() == '/') {
        group.pop_back();
    }
    return group;
}

std::string dconf_policy_content(const policy_entry& entry)
{
    std::ostringstream output;
    output << "[" << dconf_group_from_policy(entry) << "]\n";
    output << entry.key << "=" << entry.value << "\n";
    output << "# Generated by LinuxDesktop2026 ld_desktop.\n";
    return output.str();
}

std::string dconf_lock_content(const policy_entry& entry)
{
    return "/" + dconf_group_from_policy(entry) + "/" + entry.key + "\n";
}

std::filesystem::path user_policy_base_directory(std::vector<diagnostic>& diagnostics)
{
    const auto config_base = config_base_directory(diagnostics);
    if (config_base.empty()) {
        return {};
    }
    return config_base / "linuxdesktop2026" / "dconf";
}

std::filesystem::path policy_defaults_directory(const policy_entry& entry, const apply_options& options, std::vector<diagnostic>& diagnostics)
{
    if (options.policy_defaults_directory_override.has_value()) {
        if (!options.policy_defaults_directory_override->is_absolute()) {
            diagnostics.push_back(make_diagnostic(
                severity::error,
                "policy-defaults-directory-relative",
                "Policy defaults directory override must be absolute",
                *options.policy_defaults_directory_override));
            return {};
        }
        return *options.policy_defaults_directory_override;
    }
    if (!entry.user_scope) {
        return std::filesystem::path("/etc/dconf/db/local.d");
    }
    diagnostics.push_back(make_diagnostic(
        severity::warning,
        "policy-user-scope-dconf-not-system",
        "User-scope dconf-compatible policy files are generated for inspection; desktop enforcement may require system dconf installation"));
    return user_policy_base_directory(diagnostics) / "defaults";
}

std::filesystem::path policy_locks_directory(const policy_entry& entry, const apply_options& options, std::vector<diagnostic>& diagnostics)
{
    if (options.policy_locks_directory_override.has_value()) {
        if (!options.policy_locks_directory_override->is_absolute()) {
            diagnostics.push_back(make_diagnostic(
                severity::error,
                "policy-locks-directory-relative",
                "Policy locks directory override must be absolute",
                *options.policy_locks_directory_override));
            return {};
        }
        return *options.policy_locks_directory_override;
    }
    if (!entry.user_scope) {
        return std::filesystem::path("/etc/dconf/db/local.d/locks");
    }
    return user_policy_base_directory(diagnostics) / "locks";
}

std::filesystem::path policy_defaults_path(const policy_entry& entry, const apply_options& options, std::vector<diagnostic>& diagnostics)
{
    const auto directory = policy_defaults_directory(entry, options, diagnostics);
    if (directory.empty()) {
        return {};
    }
    return directory / sanitize_policy_file_id(entry.id, diagnostics);
}

std::filesystem::path policy_lock_path(const policy_entry& entry, const apply_options& options, std::vector<diagnostic>& diagnostics)
{
    const auto directory = policy_locks_directory(entry, options, diagnostics);
    if (directory.empty()) {
        return {};
    }
    return directory / sanitize_policy_file_id(entry.id, diagnostics);
}

bool has_policy_lock(const std::string& content, const policy_entry& entry)
{
    std::istringstream input(content);
    const auto expected = dconf_lock_content(entry);
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line + "\n" == expected) {
            return true;
        }
    }
    return false;
}

std::optional<std::string> read_policy_value_from_keyfile(const std::string& content, const policy_entry& entry)
{
    std::istringstream input(content);
    const auto expected_group = dconf_group_from_policy(entry);
    bool in_group = false;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty() || line.front() == '#') {
            continue;
        }
        if (line.front() == '[' && line.back() == ']') {
            in_group = line.substr(1, line.size() - 2) == expected_group;
            continue;
        }
        const auto separator = line.find('=');
        if (in_group && separator != std::string::npos && line.substr(0, separator) == entry.key) {
            return line.substr(separator + 1);
        }
    }
    return std::nullopt;
}

diagnostic unsupported_backend_diagnostic(effect_kind kind)
{
    return make_diagnostic(
        severity::warning,
        std::string("desktop.") + std::string(to_string(kind)) + ".backend-missing",
        "This desktop integration backend is not implemented on this platform yet");
}

#if defined(_WIN32)
diagnostic windows_registration_limited_diagnostic(effect_kind kind)
{
    switch (kind) {
    case effect_kind::autostart:
        return make_diagnostic(
            severity::warning,
            "desktop.windows.autostart.registry-layer-required",
            "Windows autostart maps to the per-user Run registration contract; ld_desktop does not mutate it until a shared Registry/system layer owns those writes");
    case effect_kind::desktop_entry:
        return make_diagnostic(
            severity::warning,
            "desktop.windows.app-identity.registry-layer-required",
            "Windows app identity maps to shell App Paths and registered application metadata; ld_desktop does not mutate those Registry artifacts until a shared Registry/system layer owns those writes");
    case effect_kind::icon:
        return make_diagnostic(
            severity::warning,
            "desktop.windows.icon.app-identity-required",
            "Windows icon metadata is part of registered application, association, or shortcut artifacts rather than a standalone XDG-style icon cache");
    case effect_kind::mime_association:
        return make_diagnostic(
            severity::warning,
            "desktop.windows.file-association.registry-layer-required",
            "Windows file associations map to registered application and file-association Registry artifacts; ld_desktop does not mutate them until a shared Registry/system layer owns those writes");
    case effect_kind::default_application:
        return make_diagnostic(
            severity::warning,
            "desktop.windows.default-apps.user-choice-required",
            "Windows default apps are user-choice and policy mediated; ld_desktop can guide the user to Default Apps settings but must not silently force defaults");
    case effect_kind::url_protocol_handler:
        return make_diagnostic(
            severity::warning,
            "desktop.windows.url-protocol.registry-layer-required",
            "Windows URL protocol handlers map to registered protocol Registry artifacts; ld_desktop does not mutate them until a shared Registry/system layer owns those writes");
    case effect_kind::desktop_database:
        return make_diagnostic(
            severity::warning,
            "desktop.windows.shell-notify-required",
            "Windows has no XDG desktop database; shell registration changes require ShellExecute/SHChangeNotify-style activation outside the current staged artifact API");
    case effect_kind::managed_policy:
        return make_diagnostic(
            severity::warning,
            "desktop.windows.policy.registry-layer-required",
            "Windows managed policy maps to HKCU/HKLM policy Registry artifacts; ld_desktop does not mutate those artifacts until a shared Registry/system layer owns those writes");
    case effect_kind::shell_integration:
        return unsupported_backend_diagnostic(kind);
    }
    return unsupported_backend_diagnostic(kind);
}

void append_windows_activation_plan(effect_report& report, effect_kind kind)
{
    if (kind == effect_kind::default_application) {
        report.activation_plan.push_back(activation_required(
            activation_step_kind::windows_default_apps_ui,
            "ms-settings:defaultapps",
            "windows-default-apps-ui-required",
            "Open Windows Default Apps settings so the user can choose the default application; ld_desktop must not force UserChoice defaults"));
        return;
    }
    if (kind == effect_kind::desktop_database ||
        kind == effect_kind::desktop_entry ||
        kind == effect_kind::mime_association ||
        kind == effect_kind::url_protocol_handler) {
        report.activation_plan.push_back(activation_required(
            activation_step_kind::windows_shell_notify,
            "SHChangeNotify(SHCNE_ASSOCCHANGED)",
            "windows-shell-notify-required",
            "Notify the Windows shell after registration artifacts change; ld_desktop only reports this activation step today"));
    }
}
#endif

diagnostic dconf_activation_required_diagnostic()
{
    return make_diagnostic(
        severity::warning,
        "policy-dconf-activation-required",
        "dconf-compatible policy source files were generated, but ld_desktop does not activate the dconf database; install them into a system dconf profile and run dconf update before treating policy as active");
}

} // namespace

std::string_view to_string(effect_kind value)
{
    switch (value) {
    case effect_kind::autostart:
        return "autostart";
    case effect_kind::desktop_entry:
        return "desktop_entry";
    case effect_kind::icon:
        return "icon";
    case effect_kind::mime_association:
        return "mime_association";
    case effect_kind::default_application:
        return "default_application";
    case effect_kind::url_protocol_handler:
        return "url_protocol_handler";
    case effect_kind::shell_integration:
        return "shell_integration";
    case effect_kind::desktop_database:
        return "desktop_database";
    case effect_kind::managed_policy:
        return "managed_policy";
    }
    return "unknown";
}

std::string_view to_string(capability_state value)
{
    switch (value) {
    case capability_state::supported:
        return "supported";
    case capability_state::unsupported:
        return "unsupported";
    case capability_state::backend_missing:
        return "backend_missing";
    case capability_state::backend_limited:
        return "backend_limited";
    case capability_state::sandbox_limited:
        return "sandbox_limited";
    case capability_state::permission_denied:
        return "permission_denied";
    }
    return "unknown";
}

std::string_view to_string(activation_step_kind value)
{
    switch (value) {
    case activation_step_kind::refresh_desktop_database:
        return "refresh_desktop_database";
    case activation_step_kind::refresh_mime_database:
        return "refresh_mime_database";
    case activation_step_kind::refresh_icon_cache:
        return "refresh_icon_cache";
    case activation_step_kind::refresh_dconf_database:
        return "refresh_dconf_database";
    case activation_step_kind::windows_shell_notify:
        return "windows_shell_notify";
    case activation_step_kind::windows_default_apps_ui:
        return "windows_default_apps_ui";
    }
    return "unknown";
}

std::string_view to_string(registration_scope value)
{
    switch (value) {
    case registration_scope::user:
        return "user";
    case registration_scope::global:
        return "global";
    }
    return "unknown";
}

std::string_view to_string(registration_status value)
{
    switch (value) {
    case registration_status::unknown:
        return "unknown";
    case registration_status::planned:
        return "planned";
    case registration_status::staged:
        return "staged";
    case registration_status::present:
        return "present";
    case registration_status::missing:
        return "missing";
    case registration_status::unsupported:
        return "unsupported";
    case registration_status::failed:
        return "failed";
    }
    return "unknown";
}

std::string_view to_string(cleanup_status value)
{
    switch (value) {
    case cleanup_status::planned:
        return "planned";
    case cleanup_status::removed:
        return "removed";
    case cleanup_status::already_absent:
        return "already_absent";
    case cleanup_status::skipped_duplicate:
        return "skipped_duplicate";
    case cleanup_status::parent_removed:
        return "parent_removed";
    case cleanup_status::parent_not_empty:
        return "parent_not_empty";
    case cleanup_status::manual_follow_up:
        return "manual_follow_up";
    case cleanup_status::failed:
        return "failed";
    }
    return "unknown";
}

capability_report query_capabilities(const apply_options& options)
{
    capability_report report;
    const effect_kind kinds[] = {
        effect_kind::autostart,
        effect_kind::desktop_entry,
        effect_kind::icon,
        effect_kind::mime_association,
        effect_kind::default_application,
        effect_kind::url_protocol_handler,
        effect_kind::shell_integration,
        effect_kind::desktop_database,
        effect_kind::managed_policy,
    };

    for (const auto kind : kinds) {
        capability item;
        item.kind = kind;
        item.can_dry_run = true;
#if defined(_WIN32)
        if (kind == effect_kind::shell_integration) {
            item.state = capability_state::backend_missing;
            item.diagnostics.push_back(unsupported_backend_diagnostic(kind));
        } else {
            item.state = capability_state::backend_limited;
            item.diagnostics.push_back(windows_registration_limited_diagnostic(kind));
        }
#else
        if (kind == effect_kind::autostart ||
            kind == effect_kind::desktop_entry ||
            kind == effect_kind::icon ||
            kind == effect_kind::mime_association ||
            kind == effect_kind::default_application ||
            kind == effect_kind::url_protocol_handler ||
            kind == effect_kind::desktop_database) {
            item.state = capability_state::supported;
            item.can_query = true;
            item.can_write_user = options.allow_desktop_integration_write;
            item.can_write_global = item.can_write_user && options.allow_global_write;
        } else if (kind == effect_kind::managed_policy) {
            item.state = capability_state::backend_limited;
            item.can_query = true;
            item.can_write_user = options.allow_policy_write;
            item.can_write_global = item.can_write_user && options.allow_global_write;
            item.diagnostics.push_back(dconf_activation_required_diagnostic());
        } else {
            item.state = capability_state::backend_missing;
            item.diagnostics.push_back(unsupported_backend_diagnostic(kind));
        }
#endif
        report.effects.push_back(std::move(item));
    }
    return report;
}

effect_report apply_autostart(const autostart_entry& entry, const apply_options& options)
{
    effect_report report;
    report.dry_run = options.dry_run;
    report.enabled = entry.enabled;
    append_autostart_validation(entry, report.diagnostics);
    if (!entry.user_scope && !options.allow_global_write) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "autostart-global-write-denied",
            "Machine-wide autostart changes require allow_global_write"));
    }
    if (!options.allow_desktop_integration_write) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "autostart-write-denied",
            "Autostart changes require allow_desktop_integration_write"));
    }
    if (has_error(report.diagnostics)) {
        return report;
    }

#if defined(_WIN32)
    report.diagnostics.push_back(windows_registration_limited_diagnostic(effect_kind::autostart));
    if (options.dry_run) {
        report.ok = true;
        report.diagnostics.push_back(make_diagnostic(
            severity::info,
            "autostart-dry-run",
            "Windows autostart registration was planned but not written"));
    }
    return report;
#else
    report.path = autostart_path(entry, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    if (options.dry_run) {
        report.ok = true;
        report.diagnostics.push_back(make_diagnostic(
            severity::info,
            "autostart-dry-run",
            "Autostart desktop entry was planned but not written",
            *report.path));
        return report;
    }

    std::error_code ec;
    std::filesystem::create_directories(report.path->parent_path(), ec);
    if (ec) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "autostart-create-directory-failed", ec.message(), report.path->parent_path()));
        return report;
    }
    if (!write_file_content(*report.path, desktop_file_content(entry), report.diagnostics)) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "autostart-write-failed", "Failed to write XDG autostart desktop entry", *report.path));
        return report;
    }
    report.ok = true;
    return report;
#endif
}

effect_report remove_autostart(const autostart_entry& entry, const apply_options& options)
{
    effect_report report;
    report.dry_run = options.dry_run;
    append_autostart_validation(entry, report.diagnostics);
    if (!entry.user_scope && !options.allow_global_write) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "autostart-global-write-denied", "Machine-wide autostart changes require allow_global_write"));
    }
    if (!options.allow_desktop_integration_write) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "autostart-write-denied", "Autostart changes require allow_desktop_integration_write"));
    }
    if (has_error(report.diagnostics)) {
        return report;
    }

#if defined(_WIN32)
    report.diagnostics.push_back(windows_registration_limited_diagnostic(effect_kind::autostart));
    if (options.dry_run) {
        report.ok = true;
        report.diagnostics.push_back(make_diagnostic(
            severity::info,
            "autostart-dry-run",
            "Windows autostart registration removal was planned but not applied"));
    }
    return report;
#else
    report.path = autostart_path(entry, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    if (options.dry_run) {
        report.ok = true;
        report.diagnostics.push_back(make_diagnostic(severity::info, "autostart-dry-run", "Autostart desktop entry removal was planned but not applied", *report.path));
        return report;
    }

    std::error_code ec;
    std::filesystem::remove(*report.path, ec);
    if (ec) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "autostart-remove-failed", ec.message(), *report.path));
        return report;
    }
    report.ok = true;
    return report;
#endif
}

effect_report query_autostart(const autostart_entry& entry, const apply_options& options)
{
    effect_report report;
    append_autostart_validation(entry, report.diagnostics);
    if (has_error(report.diagnostics)) {
        return report;
    }

#if defined(_WIN32)
    report.diagnostics.push_back(windows_registration_limited_diagnostic(effect_kind::autostart));
    return report;
#else
    report.path = autostart_path(entry, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    std::error_code ec;
    if (!std::filesystem::exists(*report.path, ec)) {
        report.ok = true;
        report.enabled = false;
        return report;
    }
    std::error_code read_ec;
    const auto content = read_text(*report.path, read_ec);
    if (read_ec) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "autostart-read-failed", read_ec.message(), *report.path));
        return report;
    }
    report.ok = true;
    report.present = true;
    report.enabled = !desktop_file_hidden(content);
    return report;
#endif
}

void append_desktop_write_permission(bool user_scope, const apply_options& options, const std::string& prefix, std::vector<diagnostic>& diagnostics)
{
    if (!user_scope && !options.allow_global_write) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            prefix + "-global-write-denied",
            "Machine-wide desktop integration changes require allow_global_write"));
    }
    if (!options.allow_desktop_integration_write) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            prefix + "-write-denied",
            "Desktop integration changes require allow_desktop_integration_write"));
    }
}

void append_desktop_entry_validation(const desktop_entry& entry, std::vector<diagnostic>& diagnostics)
{
    if (!is_safe_desktop_id(entry.id)) {
        diagnostics.push_back(make_diagnostic(severity::error, "desktop-entry-id-invalid", "Desktop entry id must be a stable filename token"));
    }
    if (entry.display_name.empty()) {
        diagnostics.push_back(make_diagnostic(severity::error, "desktop-entry-display-name-empty", "Desktop entry requires a display name"));
    }
    if (entry.executable.empty() || !entry.executable.is_absolute()) {
        diagnostics.push_back(make_diagnostic(severity::error, "desktop-entry-executable-invalid", "Desktop entry executable must be an absolute path", entry.executable));
    }
    if (!entry.working_directory.empty() && !entry.working_directory.is_absolute()) {
        diagnostics.push_back(make_diagnostic(severity::error, "desktop-entry-working-directory-relative", "Desktop entry working directory must be absolute", entry.working_directory));
    }
    for (const auto& item : entry.categories) {
        if (!is_safe_token(item)) {
            diagnostics.push_back(make_diagnostic(severity::error, "desktop-entry-category-invalid", "Desktop entry category must be a safe token"));
        }
    }
    for (const auto& item : entry.mime_types) {
        if (!is_safe_mime_name(item) && item.rfind("x-scheme-handler/", 0) != 0) {
            diagnostics.push_back(make_diagnostic(severity::error, "desktop-entry-mime-type-invalid", "Desktop entry MimeType item must be a valid MIME name or x-scheme-handler entry"));
        }
    }
}

std::filesystem::path desktop_entry_path(const desktop_entry& entry, const apply_options& options, std::vector<diagnostic>& diagnostics)
{
    const auto directory = applications_directory(entry.user_scope, options, diagnostics);
    return directory.empty() ? std::filesystem::path{} : directory / ensure_desktop_suffix(entry.id);
}

std::string desktop_entry_content(const desktop_entry& entry)
{
    std::ostringstream output;
    output << "[Desktop Entry]\n";
    output << "Type=Application\n";
    output << "Name=" << desktop_escape(entry.display_name) << "\n";
    if (!entry.generic_name.empty()) {
        output << "GenericName=" << desktop_escape(entry.generic_name) << "\n";
    }
    if (!entry.comment.empty()) {
        output << "Comment=" << desktop_escape(entry.comment) << "\n";
    }
    output << "Exec=" << desktop_escape(autostart_command({entry.id, entry.display_name, entry.executable, entry.arguments, entry.working_directory, true, entry.user_scope})) << "\n";
    if (!entry.working_directory.empty()) {
        output << "Path=" << desktop_escape(entry.working_directory.string()) << "\n";
    }
    output << "Terminal=" << (entry.terminal ? "true" : "false") << "\n";
    if (!entry.categories.empty()) {
        output << "Categories=" << join_desktop_list(entry.categories) << "\n";
    }
    if (!entry.keywords.empty()) {
        output << "Keywords=" << join_desktop_list(entry.keywords) << "\n";
    }
    if (!entry.mime_types.empty()) {
        output << "MimeType=" << join_desktop_list(entry.mime_types) << "\n";
    }
    output << "X-LinuxDesktop2026-DesktopEntry=true\n";
    return output.str();
}

effect_report apply_desktop_entry(const desktop_entry& entry, const apply_options& options)
{
    effect_report report;
    report.dry_run = options.dry_run;
    report.present = true;
    append_desktop_entry_validation(entry, report.diagnostics);
    append_desktop_write_permission(entry.user_scope, options, "desktop-entry", report.diagnostics);
    if (has_error(report.diagnostics)) {
        return report;
    }
#if defined(_WIN32)
    report.diagnostics.push_back(windows_registration_limited_diagnostic(effect_kind::desktop_entry));
    append_windows_activation_plan(report, effect_kind::desktop_entry);
    if (options.dry_run) {
        report.ok = true;
    }
    return report;
#else
    report.path = desktop_entry_path(entry, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    report.activation_plan.push_back(activation_required(
        activation_step_kind::refresh_desktop_database,
        "update-desktop-database " + report.path->parent_path().string(),
        "desktop-database-refresh-required",
        "Desktop entry artifacts were staged; refresh the desktop database before treating them as active"));
    if (options.dry_run) {
        report.ok = true;
        report.diagnostics.push_back(make_diagnostic(severity::info, "desktop-entry-dry-run", "Desktop entry was planned but not written", *report.path));
        return report;
    }
    std::error_code ec;
    std::filesystem::create_directories(report.path->parent_path(), ec);
    if (ec) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "desktop-entry-create-directory-failed", ec.message(), report.path->parent_path()));
        return report;
    }
    if (!write_file_content(*report.path, desktop_entry_content(entry), report.diagnostics)) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "desktop-entry-write-failed", "Failed to write XDG desktop entry", *report.path));
        return report;
    }
    report.ok = true;
    return report;
#endif
}

effect_report remove_desktop_entry(const desktop_entry& entry, const apply_options& options)
{
    effect_report report;
    report.dry_run = options.dry_run;
    append_desktop_entry_validation(entry, report.diagnostics);
    append_desktop_write_permission(entry.user_scope, options, "desktop-entry", report.diagnostics);
#if defined(_WIN32)
    if (has_error(report.diagnostics)) {
        return report;
    }
    report.diagnostics.push_back(windows_registration_limited_diagnostic(effect_kind::desktop_entry));
    append_windows_activation_plan(report, effect_kind::desktop_entry);
    if (options.dry_run) {
        report.ok = true;
    }
    return report;
#else
    report.path = desktop_entry_path(entry, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    report.activation_plan.push_back(activation_required(activation_step_kind::refresh_desktop_database, "update-desktop-database " + report.path->parent_path().string(), "desktop-database-refresh-required", "Refresh the desktop database after removing the desktop entry artifact"));
    if (options.dry_run) {
        report.ok = true;
        return report;
    }
    std::error_code ec;
    report.present = std::filesystem::exists(*report.path, ec);
    if (ec) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "desktop-entry-remove-probe-failed", ec.message(), *report.path));
        return report;
    }
    std::filesystem::remove(*report.path, ec);
    if (ec) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "desktop-entry-remove-failed", ec.message(), *report.path));
        return report;
    }
    report.ok = true;
    return report;
#endif
}

effect_report query_desktop_entry(const desktop_entry& entry, const apply_options& options)
{
    effect_report report;
    append_desktop_entry_validation(entry, report.diagnostics);
#if defined(_WIN32)
    if (has_error(report.diagnostics)) {
        return report;
    }
    report.diagnostics.push_back(windows_registration_limited_diagnostic(effect_kind::desktop_entry));
    append_windows_activation_plan(report, effect_kind::desktop_entry);
    return report;
#else
    report.path = desktop_entry_path(entry, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    std::error_code ec;
    report.present = std::filesystem::exists(*report.path, ec);
    report.ok = !ec;
    return report;
#endif
}

void append_icon_validation(const icon_entry& entry, std::vector<diagnostic>& diagnostics)
{
    if (!is_safe_token(entry.name)) {
        diagnostics.push_back(make_diagnostic(severity::error, "icon-name-invalid", "Icon name must be a safe token"));
    }
    if (entry.source_path.empty() || !entry.source_path.is_absolute()) {
        diagnostics.push_back(make_diagnostic(severity::error, "icon-source-path-invalid", "Icon source path must be absolute", entry.source_path));
    }
    if (entry.size < 0) {
        diagnostics.push_back(make_diagnostic(severity::error, "icon-size-invalid", "Icon size must be zero for scalable icons or a positive pixel size"));
    }
    if (!is_safe_token(entry.theme)) {
        diagnostics.push_back(make_diagnostic(severity::error, "icon-theme-invalid", "Icon theme must be a safe token"));
    }
}

std::filesystem::path icon_path(const icon_entry& entry, const apply_options& options, std::vector<diagnostic>& diagnostics)
{
    const auto base = icons_directory(entry.user_scope, options, diagnostics);
    if (base.empty()) {
        return {};
    }
    const auto bucket = entry.size == 0 ? std::filesystem::path("scalable") : std::filesystem::path(std::to_string(entry.size) + "x" + std::to_string(entry.size));
    auto extension = entry.source_path.extension();
    if (extension.empty()) {
        extension = entry.size == 0 ? ".svg" : ".png";
    }
    return base / entry.theme / bucket / "apps" / (entry.name + extension.string());
}

effect_report apply_icon(const icon_entry& entry, const apply_options& options)
{
    effect_report report;
    report.dry_run = options.dry_run;
    report.present = true;
    append_icon_validation(entry, report.diagnostics);
    append_desktop_write_permission(entry.user_scope, options, "icon", report.diagnostics);
#if defined(_WIN32)
    if (has_error(report.diagnostics)) {
        return report;
    }
    report.diagnostics.push_back(windows_registration_limited_diagnostic(effect_kind::icon));
    if (options.dry_run) {
        report.ok = true;
    }
    return report;
#else
    report.path = icon_path(entry, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    report.activation_plan.push_back(activation_required(activation_step_kind::refresh_icon_cache, "gtk-update-icon-cache " + (report.path->parent_path().parent_path().parent_path()).string(), "icon-cache-refresh-required", "Icon artifact was staged; refresh the icon cache before treating it as active"));
    if (options.dry_run) {
        report.ok = true;
        return report;
    }
    std::error_code ec;
    if (!std::filesystem::is_regular_file(entry.source_path, ec)) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "icon-source-path-not-file", "Icon source path must name a readable file", entry.source_path));
        return report;
    }
    std::filesystem::create_directories(report.path->parent_path(), ec);
    if (ec) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "icon-create-directory-failed", ec.message(), report.path->parent_path()));
        return report;
    }
    std::filesystem::copy_file(entry.source_path, *report.path, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "icon-copy-failed", ec.message(), *report.path));
        return report;
    }
    report.ok = true;
    return report;
#endif
}

effect_report remove_icon(const icon_entry& entry, const apply_options& options)
{
    effect_report report;
    report.dry_run = options.dry_run;
    append_icon_validation(entry, report.diagnostics);
    append_desktop_write_permission(entry.user_scope, options, "icon", report.diagnostics);
#if defined(_WIN32)
    if (has_error(report.diagnostics)) {
        return report;
    }
    report.diagnostics.push_back(windows_registration_limited_diagnostic(effect_kind::icon));
    if (options.dry_run) {
        report.ok = true;
    }
    return report;
#else
    report.path = icon_path(entry, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    report.activation_plan.push_back(activation_required(activation_step_kind::refresh_icon_cache, "gtk-update-icon-cache " + (report.path->parent_path().parent_path().parent_path()).string(), "icon-cache-refresh-required", "Refresh the icon cache after removing the icon artifact"));
    if (options.dry_run) {
        report.ok = true;
        return report;
    }
    std::error_code ec;
    report.present = std::filesystem::exists(*report.path, ec);
    if (ec) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "icon-remove-probe-failed", ec.message(), *report.path));
        return report;
    }
    std::filesystem::remove(*report.path, ec);
    if (ec) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "icon-remove-failed", ec.message(), *report.path));
        return report;
    }
    report.ok = true;
    return report;
#endif
}

effect_report query_icon(const icon_entry& entry, const apply_options& options)
{
    effect_report report;
    append_icon_validation(entry, report.diagnostics);
#if defined(_WIN32)
    if (has_error(report.diagnostics)) {
        return report;
    }
    report.diagnostics.push_back(windows_registration_limited_diagnostic(effect_kind::icon));
    return report;
#else
    report.path = icon_path(entry, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    std::error_code ec;
    report.present = std::filesystem::exists(*report.path, ec);
    report.ok = !ec;
    return report;
#endif
}

void append_mime_declaration_validation(const mime_declaration& declaration, std::vector<diagnostic>& diagnostics)
{
    if (!is_safe_mime_name(declaration.name)) {
        diagnostics.push_back(make_diagnostic(severity::error, "mime-name-invalid", "MIME declaration name must be type/subtype with safe tokens"));
    }
    if (declaration.comment.empty()) {
        diagnostics.push_back(make_diagnostic(severity::error, "mime-comment-empty", "MIME declaration requires a comment"));
    }
    for (const auto& pattern : declaration.glob_patterns) {
        if (pattern.empty() || pattern.find('/') != std::string::npos || pattern.find('\\') != std::string::npos) {
            diagnostics.push_back(make_diagnostic(severity::error, "mime-glob-invalid", "MIME glob pattern must be a filename pattern, not a path"));
        }
    }
}

std::filesystem::path mime_declaration_path(const mime_declaration& declaration, const apply_options& options, std::vector<diagnostic>& diagnostics)
{
    const auto directory = mime_packages_directory(declaration.user_scope, options, diagnostics);
    auto file_name = sanitize_segment(declaration.name, "application");
    return directory.empty() ? std::filesystem::path{} : directory / (file_name + ".xml");
}

std::string xml_escape(const std::string& value)
{
    std::string result;
    for (const char ch : value) {
        switch (ch) {
        case '&': result += "&amp;"; break;
        case '<': result += "&lt;"; break;
        case '>': result += "&gt;"; break;
        case '"': result += "&quot;"; break;
        default: result.push_back(ch); break;
        }
    }
    return result;
}

std::string mime_declaration_content(const mime_declaration& declaration)
{
    std::ostringstream output;
    output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    output << "<mime-info xmlns=\"http://www.freedesktop.org/standards/shared-mime-info\">\n";
    output << "  <mime-type type=\"" << xml_escape(declaration.name) << "\">\n";
    output << "    <comment>" << xml_escape(declaration.comment) << "</comment>\n";
    for (const auto& pattern : declaration.glob_patterns) {
        output << "    <glob pattern=\"" << xml_escape(pattern) << "\"/>\n";
    }
    output << "  </mime-type>\n";
    output << "</mime-info>\n";
    return output.str();
}

effect_report apply_mime_declaration(const mime_declaration& declaration, const apply_options& options)
{
    effect_report report;
    report.dry_run = options.dry_run;
    report.present = true;
    append_mime_declaration_validation(declaration, report.diagnostics);
    append_desktop_write_permission(declaration.user_scope, options, "mime-declaration", report.diagnostics);
#if defined(_WIN32)
    if (has_error(report.diagnostics)) {
        return report;
    }
    report.diagnostics.push_back(windows_registration_limited_diagnostic(effect_kind::mime_association));
    if (options.dry_run) {
        report.ok = true;
    }
    return report;
#else
    report.path = mime_declaration_path(declaration, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    report.activation_plan.push_back(activation_required(activation_step_kind::refresh_mime_database, "update-mime-database " + report.path->parent_path().parent_path().string(), "mime-database-refresh-required", "MIME declaration was staged; refresh the MIME database before treating it as active"));
    if (options.dry_run) {
        report.ok = true;
        return report;
    }
    std::error_code ec;
    std::filesystem::create_directories(report.path->parent_path(), ec);
    if (ec) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "mime-declaration-create-directory-failed", ec.message(), report.path->parent_path()));
        return report;
    }
    if (!write_file_content(*report.path, mime_declaration_content(declaration), report.diagnostics)) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "mime-declaration-write-failed", "Failed to write shared-mime-info package file", *report.path));
        return report;
    }
    report.ok = true;
    return report;
#endif
}

effect_report remove_mime_declaration(const mime_declaration& declaration, const apply_options& options)
{
    effect_report report;
    report.dry_run = options.dry_run;
    append_mime_declaration_validation(declaration, report.diagnostics);
    append_desktop_write_permission(declaration.user_scope, options, "mime-declaration", report.diagnostics);
#if defined(_WIN32)
    if (has_error(report.diagnostics)) {
        return report;
    }
    report.diagnostics.push_back(windows_registration_limited_diagnostic(effect_kind::mime_association));
    if (options.dry_run) {
        report.ok = true;
    }
    return report;
#else
    report.path = mime_declaration_path(declaration, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    report.activation_plan.push_back(activation_required(activation_step_kind::refresh_mime_database, "update-mime-database " + report.path->parent_path().parent_path().string(), "mime-database-refresh-required", "Refresh the MIME database after removing the MIME package file"));
    if (options.dry_run) {
        report.ok = true;
        return report;
    }
    std::error_code ec;
    report.present = std::filesystem::exists(*report.path, ec);
    if (ec) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "mime-declaration-remove-probe-failed", ec.message(), *report.path));
        return report;
    }
    std::filesystem::remove(*report.path, ec);
    if (ec) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "mime-declaration-remove-failed", ec.message(), *report.path));
        return report;
    }
    report.ok = true;
    return report;
#endif
}

effect_report query_mime_declaration(const mime_declaration& declaration, const apply_options& options)
{
    effect_report report;
    append_mime_declaration_validation(declaration, report.diagnostics);
#if defined(_WIN32)
    if (has_error(report.diagnostics)) {
        return report;
    }
    report.diagnostics.push_back(windows_registration_limited_diagnostic(effect_kind::mime_association));
    return report;
#else
    report.path = mime_declaration_path(declaration, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    std::error_code ec;
    report.present = std::filesystem::exists(*report.path, ec);
    report.ok = !ec;
    return report;
#endif
}

using mimeapps_sections = std::map<std::string, std::map<std::string, std::vector<std::string>>>;

std::vector<std::string> split_desktop_ids(std::string_view value)
{
    std::vector<std::string> result;
    size_t first = 0;
    while (first <= value.size()) {
        const auto last = value.find(';', first);
        const auto part = value.substr(first, last == std::string_view::npos ? value.size() - first : last - first);
        if (!part.empty()) {
            result.emplace_back(part);
        }
        if (last == std::string_view::npos) {
            break;
        }
        first = last + 1;
    }
    return result;
}

mimeapps_sections parse_mimeapps(const std::string& content)
{
    mimeapps_sections sections;
    std::string current = "Added Associations";
    std::istringstream input(content);
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty() || line.front() == '#') {
            continue;
        }
        if (line.front() == '[' && line.back() == ']') {
            current = line.substr(1, line.size() - 2);
            continue;
        }
        const auto separator = line.find('=');
        if (separator != std::string::npos) {
            sections[current][line.substr(0, separator)] = split_desktop_ids(line.substr(separator + 1));
        }
    }
    return sections;
}

std::string render_mimeapps(const mimeapps_sections& sections)
{
    std::ostringstream output;
    for (const auto& section : sections) {
        if (section.second.empty()) {
            continue;
        }
        output << "[" << section.first << "]\n";
        for (const auto& item : section.second) {
            output << item.first << "=" << join_desktop_list(item.second) << "\n";
        }
        output << "\n";
    }
    return output.str();
}

bool update_mimeapps_key(
    const std::filesystem::path& path,
    const std::string& section,
    const std::string& key,
    const std::vector<std::string>& ids,
    bool remove,
    std::vector<diagnostic>& diagnostics)
{
    std::error_code ec;
    const auto content = std::filesystem::exists(path, ec) ? read_text(path, ec) : std::string{};
    if (ec) {
        diagnostics.push_back(make_diagnostic(severity::error, "mimeapps-read-failed", ec.message(), path));
        return false;
    }
    auto sections = parse_mimeapps(content);
    auto& values = sections[section][key];
    if (remove) {
        values.erase(std::remove_if(values.begin(), values.end(), [&](const std::string& value) {
                         return std::find(ids.begin(), ids.end(), value) != ids.end();
                     }),
            values.end());
        if (values.empty()) {
            sections[section].erase(key);
        }
    } else {
        std::set<std::string> seen(values.begin(), values.end());
        for (const auto& id : ids) {
            if (seen.insert(id).second) {
                values.push_back(id);
            }
        }
    }
    return write_file_content(path, render_mimeapps(sections), diagnostics);
}

bool query_mimeapps_key(const std::filesystem::path& path, const std::string& section, const std::string& key, const std::string& id, std::vector<diagnostic>& diagnostics)
{
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        return false;
    }
    const auto content = read_text(path, ec);
    if (ec) {
        diagnostics.push_back(make_diagnostic(severity::error, "mimeapps-read-failed", ec.message(), path));
        return false;
    }
    const auto sections = parse_mimeapps(content);
    const auto section_it = sections.find(section);
    if (section_it == sections.end()) {
        return false;
    }
    const auto key_it = section_it->second.find(key);
    if (key_it == section_it->second.end()) {
        return false;
    }
    return std::find(key_it->second.begin(), key_it->second.end(), id) != key_it->second.end();
}

std::vector<std::string> normalized_desktop_ids(const std::vector<std::string>& ids, std::vector<diagnostic>& diagnostics)
{
    std::vector<std::string> result;
    if (ids.empty()) {
        diagnostics.push_back(make_diagnostic(severity::error, "desktop-entry-id-empty", "At least one desktop entry id is required"));
    }
    for (const auto& id : ids) {
        if (!is_safe_desktop_id(id)) {
            diagnostics.push_back(make_diagnostic(severity::error, "desktop-entry-id-invalid", "Desktop entry id must be a stable filename token"));
        } else {
            result.push_back(ensure_desktop_suffix(id));
        }
    }
    return result;
}

effect_report apply_mime_association(const mime_association& association, const apply_options& options)
{
    effect_report report;
    report.dry_run = options.dry_run;
    report.present = true;
    if (!is_safe_mime_name(association.mime_type)) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "mime-name-invalid", "MIME association requires a valid MIME name"));
    }
    const auto ids = normalized_desktop_ids(association.desktop_entry_ids, report.diagnostics);
    append_desktop_write_permission(association.user_scope, options, "mime-association", report.diagnostics);
#if defined(_WIN32)
    if (has_error(report.diagnostics)) {
        return report;
    }
    report.diagnostics.push_back(windows_registration_limited_diagnostic(effect_kind::mime_association));
    append_windows_activation_plan(report, effect_kind::mime_association);
    if (options.dry_run) {
        report.ok = true;
    }
    return report;
#else
    report.path = mimeapps_file(association.user_scope, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    report.activation_plan.push_back(activation_required(activation_step_kind::refresh_desktop_database, "update-desktop-database " + report.path->parent_path().string(), "desktop-database-refresh-required", "MIME association artifact was staged; refresh the desktop database before treating it as active"));
    if (options.dry_run) {
        report.ok = true;
        return report;
    }
    std::error_code ec;
    std::filesystem::create_directories(report.path->parent_path(), ec);
    if (ec) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "mimeapps-create-directory-failed", ec.message(), report.path->parent_path()));
        return report;
    }
    report.ok = update_mimeapps_key(*report.path, "Added Associations", association.mime_type, ids, false, report.diagnostics);
    if (!report.ok) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "mimeapps-write-failed", "Failed to update mimeapps.list", *report.path));
    }
    return report;
#endif
}

effect_report remove_mime_association(const mime_association& association, const apply_options& options)
{
    effect_report report;
    report.dry_run = options.dry_run;
    if (!is_safe_mime_name(association.mime_type)) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "mime-name-invalid", "MIME association requires a valid MIME name"));
    }
    const auto ids = normalized_desktop_ids(association.desktop_entry_ids, report.diagnostics);
    append_desktop_write_permission(association.user_scope, options, "mime-association", report.diagnostics);
#if defined(_WIN32)
    if (has_error(report.diagnostics)) {
        return report;
    }
    report.diagnostics.push_back(windows_registration_limited_diagnostic(effect_kind::mime_association));
    append_windows_activation_plan(report, effect_kind::mime_association);
    if (options.dry_run) {
        report.ok = true;
    }
    return report;
#else
    report.path = mimeapps_file(association.user_scope, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    if (options.dry_run) {
        report.ok = true;
        return report;
    }
    report.present = !ids.empty() && query_mimeapps_key(*report.path, "Added Associations", association.mime_type, ids.front(), report.diagnostics);
    report.ok = update_mimeapps_key(*report.path, "Added Associations", association.mime_type, ids, true, report.diagnostics);
    if (!report.ok) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "mimeapps-write-failed", "Failed to update mimeapps.list", *report.path));
    }
    return report;
#endif
}

effect_report query_mime_association(const mime_association& association, const apply_options& options)
{
    effect_report report;
    if (!is_safe_mime_name(association.mime_type)) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "mime-name-invalid", "MIME association requires a valid MIME name"));
    }
    const auto ids = normalized_desktop_ids(association.desktop_entry_ids, report.diagnostics);
#if defined(_WIN32)
    if (has_error(report.diagnostics)) {
        return report;
    }
    report.diagnostics.push_back(windows_registration_limited_diagnostic(effect_kind::mime_association));
    append_windows_activation_plan(report, effect_kind::mime_association);
    return report;
#else
    report.path = mimeapps_file(association.user_scope, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    report.present = !ids.empty() && query_mimeapps_key(*report.path, "Added Associations", association.mime_type, ids.front(), report.diagnostics);
    report.ok = !has_error(report.diagnostics);
    return report;
#endif
}

std::string scheme_mime_key(const std::string& scheme)
{
    return "x-scheme-handler/" + scheme;
}

effect_report apply_default_application(const default_application_intent& intent, const apply_options& options)
{
    effect_report report;
    report.dry_run = options.dry_run;
    report.present = intent.make_default;
    if (!is_safe_mime_name(intent.mime_type_or_scheme) && intent.mime_type_or_scheme.rfind("x-scheme-handler/", 0) != 0) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "default-application-key-invalid", "Default application key must be a MIME name or x-scheme-handler entry"));
    }
    const auto ids = normalized_desktop_ids({intent.desktop_entry_id}, report.diagnostics);
    append_desktop_write_permission(intent.user_scope, options, "default-application", report.diagnostics);
#if defined(_WIN32)
    if (has_error(report.diagnostics)) {
        return report;
    }
    report.diagnostics.push_back(windows_registration_limited_diagnostic(effect_kind::default_application));
    append_windows_activation_plan(report, effect_kind::default_application);
    if (options.dry_run) {
        report.ok = true;
    }
    return report;
#else
    report.path = mimeapps_file(intent.user_scope, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    report.activation_plan.push_back(activation_required(
        activation_step_kind::refresh_desktop_database,
        "update-desktop-database " + report.path->parent_path().string(),
        "desktop-database-refresh-required",
        "Default application artifact was staged; refresh desktop registration metadata before treating it as active"));
    if (options.dry_run) {
        report.ok = true;
        return report;
    }
    std::error_code ec;
    std::filesystem::create_directories(report.path->parent_path(), ec);
    if (ec) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "mimeapps-create-directory-failed", ec.message(), report.path->parent_path()));
        return report;
    }
    if (!intent.make_default) {
        report.present = !ids.empty() && query_mimeapps_key(*report.path, "Default Applications", intent.mime_type_or_scheme, ids.front(), report.diagnostics);
        if (has_error(report.diagnostics)) {
            return report;
        }
    }
    report.ok = update_mimeapps_key(*report.path, "Default Applications", intent.mime_type_or_scheme, ids, !intent.make_default, report.diagnostics);
    if (!report.ok) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "mimeapps-write-failed", "Failed to update mimeapps.list", *report.path));
    }
    return report;
#endif
}

effect_report remove_default_application(const default_application_intent& intent, const apply_options& options)
{
    auto removed = intent;
    removed.make_default = false;
    return apply_default_application(removed, options);
}

effect_report query_default_application(const default_application_intent& intent, const apply_options& options)
{
    effect_report report;
    if (!is_safe_mime_name(intent.mime_type_or_scheme) && intent.mime_type_or_scheme.rfind("x-scheme-handler/", 0) != 0) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "default-application-key-invalid", "Default application key must be a MIME name or x-scheme-handler entry"));
    }
    const auto ids = normalized_desktop_ids({intent.desktop_entry_id}, report.diagnostics);
#if defined(_WIN32)
    if (has_error(report.diagnostics)) {
        return report;
    }
    report.diagnostics.push_back(windows_registration_limited_diagnostic(effect_kind::default_application));
    append_windows_activation_plan(report, effect_kind::default_application);
    return report;
#else
    report.path = mimeapps_file(intent.user_scope, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    report.present = !ids.empty() && query_mimeapps_key(*report.path, "Default Applications", intent.mime_type_or_scheme, ids.front(), report.diagnostics);
    report.ok = !has_error(report.diagnostics);
    return report;
#endif
}

effect_report apply_url_scheme_handler(const url_scheme_handler& handler, const apply_options& options)
{
    if (!is_safe_scheme(handler.scheme)) {
        effect_report report;
        report.diagnostics.push_back(make_diagnostic(severity::error, "url-scheme-invalid", "URL scheme must follow RFC-style scheme token syntax"));
        return report;
    }
#if defined(_WIN32)
    effect_report report;
    report.dry_run = options.dry_run;
    report.present = true;
    (void)normalized_desktop_ids({handler.desktop_entry_id}, report.diagnostics);
    append_desktop_write_permission(handler.user_scope, options, "url-scheme-handler", report.diagnostics);
    if (has_error(report.diagnostics)) {
        return report;
    }
    report.diagnostics.push_back(windows_registration_limited_diagnostic(effect_kind::url_protocol_handler));
    append_windows_activation_plan(report, effect_kind::url_protocol_handler);
    if (options.dry_run) {
        report.ok = true;
    }
    return report;
#else
    default_application_intent intent;
    intent.mime_type_or_scheme = scheme_mime_key(handler.scheme);
    intent.desktop_entry_id = handler.desktop_entry_id;
    intent.make_default = true;
    intent.user_scope = handler.user_scope;
    auto report = apply_default_application(intent, options);
    if (report.path && !has_error(report.diagnostics)) {
        report.activation_plan.push_back(activation_required(
            activation_step_kind::refresh_desktop_database,
            "update-desktop-database " + report.path->parent_path().string(),
            "desktop-database-refresh-required",
            "URL scheme handler artifact was staged; refresh desktop registration metadata before treating it as active"));
    }
    return report;
#endif
}

effect_report remove_url_scheme_handler(const url_scheme_handler& handler, const apply_options& options)
{
    if (!is_safe_scheme(handler.scheme)) {
        effect_report report;
        report.diagnostics.push_back(make_diagnostic(severity::error, "url-scheme-invalid", "URL scheme must follow RFC-style scheme token syntax"));
        return report;
    }
#if defined(_WIN32)
    effect_report report;
    report.dry_run = options.dry_run;
    (void)normalized_desktop_ids({handler.desktop_entry_id}, report.diagnostics);
    append_desktop_write_permission(handler.user_scope, options, "url-scheme-handler", report.diagnostics);
    if (has_error(report.diagnostics)) {
        return report;
    }
    report.diagnostics.push_back(windows_registration_limited_diagnostic(effect_kind::url_protocol_handler));
    append_windows_activation_plan(report, effect_kind::url_protocol_handler);
    if (options.dry_run) {
        report.ok = true;
    }
    return report;
#else
    default_application_intent intent;
    intent.mime_type_or_scheme = scheme_mime_key(handler.scheme);
    intent.desktop_entry_id = handler.desktop_entry_id;
    intent.make_default = false;
    intent.user_scope = handler.user_scope;
    return apply_default_application(intent, options);
#endif
}

effect_report query_url_scheme_handler(const url_scheme_handler& handler, const apply_options& options)
{
    if (!is_safe_scheme(handler.scheme)) {
        effect_report report;
        report.diagnostics.push_back(make_diagnostic(severity::error, "url-scheme-invalid", "URL scheme must follow RFC-style scheme token syntax"));
        return report;
    }
#if defined(_WIN32)
    effect_report report;
    (void)normalized_desktop_ids({handler.desktop_entry_id}, report.diagnostics);
    if (has_error(report.diagnostics)) {
        return report;
    }
    report.diagnostics.push_back(windows_registration_limited_diagnostic(effect_kind::url_protocol_handler));
    append_windows_activation_plan(report, effect_kind::url_protocol_handler);
    return report;
#else
    default_application_intent intent;
    intent.mime_type_or_scheme = scheme_mime_key(handler.scheme);
    intent.desktop_entry_id = handler.desktop_entry_id;
    intent.user_scope = handler.user_scope;
    return query_default_application(intent, options);
#endif
}

policy_report apply_policy(const policy_entry& entry, const apply_options& options)
{
    policy_report report;
    report.dry_run = options.dry_run;
    report.enforced = entry.enforced;
    append_policy_validation(entry, true, report.diagnostics);
    if (!entry.user_scope && !options.allow_global_write) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "policy-global-write-denied", "Machine-wide policy changes require allow_global_write"));
    }
    if (!options.allow_policy_write) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "policy-write-denied", "Managed/enforced policy changes require allow_policy_write"));
    }
    if (has_error(report.diagnostics)) {
        return report;
    }
    report.present = true;

#if defined(_WIN32)
    report.diagnostics.push_back(windows_registration_limited_diagnostic(effect_kind::managed_policy));
    if (options.dry_run) {
        report.ok = true;
        report.value = entry.value;
        report.diagnostics.push_back(make_diagnostic(
            severity::info,
            "policy-dry-run",
            "Windows managed policy registration was planned but not written"));
    }
    return report;
#else
    report.path = policy_defaults_path(entry, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    std::filesystem::path lock_path;
    if (entry.enforced) {
        lock_path = policy_lock_path(entry, options, report.diagnostics);
        if (lock_path.empty() || has_error(report.diagnostics)) {
            return report;
        }
    }
    if (options.dry_run) {
        report.ok = true;
        report.value = entry.value;
        report.diagnostics.push_back(make_diagnostic(severity::info, "policy-dry-run", "Managed/enforced policy file was planned but not written", *report.path));
        report.diagnostics.push_back(dconf_activation_required_diagnostic());
        return report;
    }

    std::error_code ec;
    std::filesystem::create_directories(report.path->parent_path(), ec);
    if (ec) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "policy-create-directory-failed", ec.message(), report.path->parent_path()));
        return report;
    }
    if (!write_file_content(*report.path, dconf_policy_content(entry), report.diagnostics)) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "policy-write-failed", "Failed to write dconf-compatible policy defaults file", *report.path));
        return report;
    }
    if (entry.enforced) {
        std::filesystem::create_directories(lock_path.parent_path(), ec);
        if (ec) {
            report.diagnostics.push_back(make_diagnostic(severity::error, "policy-lock-create-directory-failed", ec.message(), lock_path.parent_path()));
            return report;
        }
        if (!write_file_content(lock_path, dconf_lock_content(entry), report.diagnostics)) {
            report.diagnostics.push_back(make_diagnostic(severity::error, "policy-lock-write-failed", "Failed to write dconf-compatible policy lock file", lock_path));
            return report;
        }
    }
    report.ok = true;
    report.value = entry.value;
    report.diagnostics.push_back(dconf_activation_required_diagnostic());
    return report;
#endif
}

policy_report remove_policy(const policy_entry& entry, const apply_options& options)
{
    policy_report report;
    report.dry_run = options.dry_run;
    append_policy_validation(entry, false, report.diagnostics);
    if (!entry.user_scope && !options.allow_global_write) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "policy-global-write-denied", "Machine-wide policy changes require allow_global_write"));
    }
    if (!options.allow_policy_write) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "policy-write-denied", "Managed/enforced policy changes require allow_policy_write"));
    }
    if (has_error(report.diagnostics)) {
        return report;
    }

#if defined(_WIN32)
    report.diagnostics.push_back(windows_registration_limited_diagnostic(effect_kind::managed_policy));
    if (options.dry_run) {
        report.ok = true;
        report.diagnostics.push_back(make_diagnostic(
            severity::info,
            "policy-dry-run",
            "Windows managed policy registration removal was planned but not applied"));
    }
    return report;
#else
    report.path = policy_defaults_path(entry, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    if (options.dry_run) {
        report.ok = true;
        report.diagnostics.push_back(make_diagnostic(severity::info, "policy-dry-run", "Managed/enforced policy file removal was planned but not applied", *report.path));
        report.diagnostics.push_back(dconf_activation_required_diagnostic());
        return report;
    }

    std::error_code ec;
    report.present = std::filesystem::exists(*report.path, ec);
    if (ec) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "policy-remove-probe-failed", ec.message(), *report.path));
        return report;
    }
    std::filesystem::remove(*report.path, ec);
    if (ec) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "policy-remove-failed", ec.message(), *report.path));
        return report;
    }
    const auto lock_path = policy_lock_path(entry, options, report.diagnostics);
    if (!lock_path.empty()) {
        std::filesystem::remove(lock_path, ec);
        if (ec) {
            report.diagnostics.push_back(make_diagnostic(severity::error, "policy-lock-remove-failed", ec.message(), lock_path));
            return report;
        }
    }
    report.ok = true;
    report.diagnostics.push_back(dconf_activation_required_diagnostic());
    return report;
#endif
}

policy_report query_policy(const policy_entry& entry, const apply_options& options)
{
    policy_report report;
    append_policy_validation(entry, false, report.diagnostics);
    if (has_error(report.diagnostics)) {
        return report;
    }

#if defined(_WIN32)
    report.diagnostics.push_back(windows_registration_limited_diagnostic(effect_kind::managed_policy));
    return report;
#else
    report.path = policy_defaults_path(entry, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    std::error_code ec;
    if (!std::filesystem::exists(*report.path, ec)) {
        report.ok = true;
        report.present = false;
        report.diagnostics.push_back(dconf_activation_required_diagnostic());
        return report;
    }
    std::error_code read_ec;
    const auto content = read_text(*report.path, read_ec);
    if (read_ec) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "policy-read-failed", read_ec.message(), *report.path));
        return report;
    }
    report.ok = true;
    report.value = read_policy_value_from_keyfile(content, entry);
    report.present = report.value.has_value();

    const auto lock_path = policy_lock_path(entry, options, report.diagnostics);
    if (!lock_path.empty() && std::filesystem::exists(lock_path, ec)) {
        const auto lock_content = read_text(lock_path, read_ec);
        if (read_ec) {
            report.diagnostics.push_back(make_diagnostic(severity::error, "policy-lock-read-failed", read_ec.message(), lock_path));
            report.ok = false;
            return report;
        }
        report.enforced = has_policy_lock(lock_content, entry);
    }
    report.diagnostics.push_back(dconf_activation_required_diagnostic());
    return report;
#endif
}

namespace {

bool bundle_user_scope(registration_scope scope)
{
    return scope == registration_scope::user;
}

desktop_entry desktop_entry_from_bundle(const desktop_bundle& bundle)
{
    desktop_entry entry;
    if (!bundle.entry) {
        return entry;
    }

    entry.id = bundle.entry->id;
    entry.display_name = bundle.entry->display_name;
    entry.generic_name = bundle.entry->generic_name;
    entry.comment = bundle.entry->comment;
    entry.executable = bundle.entry->executable;
    entry.arguments = bundle.entry->arguments;
    entry.working_directory = bundle.entry->working_directory;
    entry.categories = bundle.entry->categories;
    entry.keywords = bundle.entry->keywords;
    entry.terminal = bundle.entry->terminal;
    entry.user_scope = bundle_user_scope(bundle.scope);

    for (const auto& association : bundle.mime_associations) {
        entry.mime_types.push_back(association.mime_type);
    }
    for (const auto& intent : bundle.default_applications) {
        if (intent.mime_type_or_scheme.rfind("x-scheme-handler/", 0) == 0) {
            entry.mime_types.push_back(intent.mime_type_or_scheme);
        }
    }
    for (const auto& handler : bundle.url_scheme_handlers) {
        entry.mime_types.push_back("x-scheme-handler/" + handler.scheme);
    }
    std::sort(entry.mime_types.begin(), entry.mime_types.end());
    entry.mime_types.erase(std::unique(entry.mime_types.begin(), entry.mime_types.end()), entry.mime_types.end());
    return entry;
}

std::vector<icon_entry> icon_entries_from_reference(const icon_reference& reference, registration_scope scope)
{
    std::vector<icon_entry> entries;
    if (reference.sizes.empty()) {
        entries.push_back({reference.name, reference.source_path, reference.theme, 0, bundle_user_scope(scope)});
        return entries;
    }
    for (const auto size : reference.sizes) {
        entries.push_back({reference.name, reference.source_path, reference.theme, size, bundle_user_scope(scope)});
    }
    return entries;
}

template <typename Report>
void append_report_diagnostics(const Report& source, desktop_bundle_report& target)
{
    target.diagnostics.insert(target.diagnostics.end(), source.diagnostics.begin(), source.diagnostics.end());
}

void append_activation_plan(const effect_report& source, desktop_bundle_report& target)
{
    target.activation_plan.insert(target.activation_plan.end(), source.activation_plan.begin(), source.activation_plan.end());
}

void append_policy_activation_plan(const policy_entry& entry, desktop_bundle_report& report)
{
#if defined(_WIN32)
    (void)entry;
    report.activation_plan.push_back(activation_required(
        activation_step_kind::windows_shell_notify,
        "SHChangeNotify(SHCNE_ASSOCCHANGED)",
        "windows-shell-notify-required",
        "Notify the Windows shell after policy-backed registration artifacts change; ld_desktop only reports this activation step today"));
#else
    (void)entry;
    report.activation_plan.push_back(activation_required(
        activation_step_kind::refresh_dconf_database,
        "dconf update",
        "policy-dconf-activation-required",
        "dconf-compatible policy source files were staged; refresh the dconf database before treating policy as active"));
#endif
}

enum class bundle_operation {
    plan,
    apply,
    query,
    remove
};

bool report_is_backend_limited(const std::vector<diagnostic>& diagnostics)
{
    return std::any_of(diagnostics.begin(), diagnostics.end(), [](const diagnostic& item) {
        return item.code.find(".backend-missing") != std::string::npos ||
            item.code.find(".registry-layer-required") != std::string::npos ||
            item.code.find(".user-choice-required") != std::string::npos ||
            item.code.find(".app-identity-required") != std::string::npos;
    });
}

registration_status status_for_effect(bundle_operation operation, const effect_report& report)
{
    if (report_is_backend_limited(report.diagnostics)) {
        return registration_status::unsupported;
    }
    if (!report.ok || has_error(report.diagnostics)) {
        return registration_status::failed;
    }
    if (operation == bundle_operation::query) {
        return report.present || report.enabled ? registration_status::present : registration_status::missing;
    }
    if (operation == bundle_operation::remove) {
        return registration_status::missing;
    }
    return report.dry_run ? registration_status::planned : registration_status::staged;
}

registration_status status_for_policy(bundle_operation operation, const policy_report& report)
{
    if (report_is_backend_limited(report.diagnostics)) {
        return registration_status::unsupported;
    }
    if (!report.ok || has_error(report.diagnostics)) {
        return registration_status::failed;
    }
    if (operation == bundle_operation::query) {
        return report.present ? registration_status::present : registration_status::missing;
    }
    if (operation == bundle_operation::remove) {
        return registration_status::missing;
    }
    return report.dry_run ? registration_status::planned : registration_status::staged;
}

std::string cleanup_key(const std::filesystem::path& path)
{
    return path.lexically_normal().string();
}

diagnostic cleanup_permission_diagnostic(const std::string& operation, const std::filesystem::path& path, const std::error_code& ec)
{
    const bool denied = ec == std::errc::permission_denied || ec == std::errc::operation_not_permitted;
    return make_diagnostic(
        severity::error,
        denied ? "cleanup-permission-denied" : "cleanup-" + operation + "-failed",
        ec.message(),
        path);
}

void append_cleanup_diagnostics(const cleanup_report& source, desktop_bundle_report& target)
{
    target.diagnostics.insert(target.diagnostics.end(), source.diagnostics.begin(), source.diagnostics.end());
}

cleanup_report make_cleanup_report(
    bundle_operation operation,
    cleanup_rule rule,
    bool owns_path,
    bool allow_write,
    bool child_ok,
    bool child_present)
{
    cleanup_report report;
    report.rule = std::move(rule);
    report.dry_run = operation == bundle_operation::query ? false : operation == bundle_operation::plan || operation == bundle_operation::apply;
    report.present = child_present;

    if (report.rule.path.empty()) {
        report.status = cleanup_status::failed;
        report.diagnostics.push_back(make_diagnostic(severity::error, "cleanup-path-empty", "Cleanup rule path must not be empty"));
        return report;
    }
    if (!report.rule.path.is_absolute()) {
        report.status = cleanup_status::failed;
        report.diagnostics.push_back(make_diagnostic(severity::error, "cleanup-path-relative", "Cleanup rule path must be absolute", report.rule.path));
        return report;
    }
    if (!allow_write && operation == bundle_operation::remove) {
        report.status = cleanup_status::failed;
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "cleanup-global-write-denied",
            "Global cleanup requires allow_global_write",
            report.rule.path));
        return report;
    }
    if (!owns_path) {
        report.status = cleanup_status::manual_follow_up;
        report.ok = true;
        report.diagnostics.push_back(make_diagnostic(
            severity::warning,
            "cleanup-shared-artifact-manual-follow-up",
            "Shared registration file may contain generated entries, but the whole file is not owned by ld_desktop cleanup",
            report.rule.path));
        return report;
    }
    if (operation == bundle_operation::plan || operation == bundle_operation::apply) {
        report.status = cleanup_status::planned;
        report.ok = true;
        return report;
    }
    if (operation == bundle_operation::query) {
        std::error_code ec;
        report.present = std::filesystem::exists(report.rule.path, ec);
        if (ec) {
            report.status = cleanup_status::failed;
            report.diagnostics.push_back(cleanup_permission_diagnostic("probe", report.rule.path, ec));
            return report;
        }
        report.status = report.present ? cleanup_status::planned : cleanup_status::already_absent;
        report.ok = true;
        return report;
    }
    if (!child_ok) {
        report.status = cleanup_status::failed;
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "cleanup-artifact-remove-failed",
            "Generated artifact cleanup follows a failed remove report",
            report.rule.path));
        return report;
    }
    report.status = child_present ? cleanup_status::removed : cleanup_status::already_absent;
    report.ok = true;
    return report;
}

void append_generated_cleanup_report(
    bundle_operation operation,
    const cleanup_rule& rule,
    bool owns_path,
    bool allow_write,
    bool child_ok,
    bool child_present,
    std::set<std::string>& seen_cleanup_paths,
    desktop_bundle_report& bundle_report)
{
    if (rule.path.empty()) {
        return;
    }

    cleanup_report cleanup;
    const auto key = cleanup_key(rule.path);
    if (!seen_cleanup_paths.insert(key).second) {
        cleanup.rule = rule;
        cleanup.status = cleanup_status::skipped_duplicate;
        cleanup.ok = true;
        cleanup.dry_run = operation == bundle_operation::query ? false : operation == bundle_operation::plan || operation == bundle_operation::apply;
        cleanup.diagnostics.push_back(make_diagnostic(severity::info, "cleanup-duplicate-skipped", "Duplicate cleanup path was reported once and skipped", rule.path));
    } else {
        cleanup = make_cleanup_report(operation, rule, owns_path, allow_write, child_ok, child_present);
    }
    append_cleanup_diagnostics(cleanup, bundle_report);
    bundle_report.cleanup_reports.push_back(std::move(cleanup));
}

cleanup_report run_explicit_cleanup_rule(
    bundle_operation operation,
    const cleanup_rule& rule,
    bool allow_write,
    std::set<std::string>& seen_cleanup_paths)
{
    cleanup_report report;
    report.rule = rule;
    report.dry_run = operation == bundle_operation::query ? false : operation == bundle_operation::plan || operation == bundle_operation::apply;

    if (rule.path.empty()) {
        report.status = cleanup_status::failed;
        report.diagnostics.push_back(make_diagnostic(severity::error, "cleanup-path-empty", "Cleanup rule path must not be empty"));
        return report;
    }
    if (!rule.path.is_absolute()) {
        report.status = cleanup_status::failed;
        report.diagnostics.push_back(make_diagnostic(severity::error, "cleanup-path-relative", "Cleanup rule path must be absolute", rule.path));
        return report;
    }

    const auto key = cleanup_key(rule.path);
    if (!seen_cleanup_paths.insert(key).second) {
        report.status = cleanup_status::skipped_duplicate;
        report.ok = true;
        report.diagnostics.push_back(make_diagnostic(severity::info, "cleanup-duplicate-skipped", "Duplicate cleanup path was reported once and skipped", rule.path));
        return report;
    }

    if (!allow_write && operation == bundle_operation::remove) {
        report.status = cleanup_status::failed;
        report.diagnostics.push_back(make_diagnostic(severity::error, "cleanup-global-write-denied", "Global cleanup requires allow_global_write", rule.path));
        return report;
    }

    std::error_code ec;
    const auto status = std::filesystem::symlink_status(rule.path, ec);
    if (ec) {
        if (ec == std::errc::no_such_file_or_directory) {
            report.status = cleanup_status::already_absent;
            report.ok = true;
            report.present = false;
            return report;
        }
        report.status = cleanup_status::failed;
        report.diagnostics.push_back(cleanup_permission_diagnostic("probe", rule.path, ec));
        return report;
    }
    report.present = std::filesystem::exists(status);
    if (!report.present) {
        report.status = cleanup_status::already_absent;
        report.ok = true;
        return report;
    }

    if (operation == bundle_operation::plan || operation == bundle_operation::apply || operation == bundle_operation::query) {
        report.status = cleanup_status::planned;
        report.ok = true;
        return report;
    }

    if (!std::filesystem::is_regular_file(status) && !std::filesystem::is_symlink(status)) {
        report.status = cleanup_status::manual_follow_up;
        report.ok = true;
        report.diagnostics.push_back(make_diagnostic(
            severity::warning,
            "cleanup-manual-follow-up",
            "Cleanup path is not a generated file artifact; remove it manually if appropriate",
            rule.path));
        return report;
    }

    std::filesystem::remove(rule.path, ec);
    if (ec) {
        report.status = cleanup_status::failed;
        report.diagnostics.push_back(cleanup_permission_diagnostic("remove", rule.path, ec));
        return report;
    }
    report.status = cleanup_status::removed;
    report.ok = true;

    if (rule.remove_empty_parent) {
        const auto parent = rule.path.parent_path();
        if (!parent.empty()) {
            report.parent_attempted = true;
            report.parent_path = parent;
            std::filesystem::remove(parent, ec);
            if (ec) {
                if (ec == std::errc::directory_not_empty || ec == std::errc::file_exists) {
                    report.diagnostics.push_back(make_diagnostic(severity::info, "cleanup-parent-not-empty", "Cleanup parent was left in place because it is not empty", parent));
                    return report;
                }
                report.status = cleanup_status::failed;
                report.ok = false;
                report.diagnostics.push_back(cleanup_permission_diagnostic("parent-remove", parent, ec));
                return report;
            }
            if (std::filesystem::exists(parent)) {
                report.diagnostics.push_back(make_diagnostic(severity::info, "cleanup-parent-not-empty", "Cleanup parent was left in place because it is not empty", parent));
            } else {
                report.diagnostics.push_back(make_diagnostic(severity::info, "cleanup-parent-removed", "Cleanup parent directory was empty and removed", parent));
            }
        }
    }
    return report;
}

effect_report run_effect(bundle_operation operation, const autostart_entry& entry, const apply_options& options)
{
    switch (operation) {
    case bundle_operation::plan:
    case bundle_operation::apply:
        return apply_autostart(entry, options);
    case bundle_operation::query:
        return query_autostart(entry, options);
    case bundle_operation::remove:
        return remove_autostart(entry, options);
    }
    return {};
}

effect_report run_effect(bundle_operation operation, const desktop_entry& entry, const apply_options& options)
{
    switch (operation) {
    case bundle_operation::plan:
    case bundle_operation::apply:
        return apply_desktop_entry(entry, options);
    case bundle_operation::query:
        return query_desktop_entry(entry, options);
    case bundle_operation::remove:
        return remove_desktop_entry(entry, options);
    }
    return {};
}

effect_report run_effect(bundle_operation operation, const icon_entry& entry, const apply_options& options)
{
    switch (operation) {
    case bundle_operation::plan:
    case bundle_operation::apply:
        return apply_icon(entry, options);
    case bundle_operation::query:
        return query_icon(entry, options);
    case bundle_operation::remove:
        return remove_icon(entry, options);
    }
    return {};
}

effect_report run_effect(bundle_operation operation, const mime_declaration& declaration, const apply_options& options)
{
    switch (operation) {
    case bundle_operation::plan:
    case bundle_operation::apply:
        return apply_mime_declaration(declaration, options);
    case bundle_operation::query:
        return query_mime_declaration(declaration, options);
    case bundle_operation::remove:
        return remove_mime_declaration(declaration, options);
    }
    return {};
}

effect_report run_effect(bundle_operation operation, const mime_association& association, const apply_options& options)
{
    switch (operation) {
    case bundle_operation::plan:
    case bundle_operation::apply:
        return apply_mime_association(association, options);
    case bundle_operation::query:
        return query_mime_association(association, options);
    case bundle_operation::remove:
        return remove_mime_association(association, options);
    }
    return {};
}

effect_report run_effect(bundle_operation operation, const default_application_intent& intent, const apply_options& options)
{
    switch (operation) {
    case bundle_operation::plan:
    case bundle_operation::apply:
        return apply_default_application(intent, options);
    case bundle_operation::query:
        return query_default_application(intent, options);
    case bundle_operation::remove:
        return remove_default_application(intent, options);
    }
    return {};
}

effect_report run_effect(bundle_operation operation, const url_scheme_handler& handler, const apply_options& options)
{
    switch (operation) {
    case bundle_operation::plan:
    case bundle_operation::apply:
        return apply_url_scheme_handler(handler, options);
    case bundle_operation::query:
        return query_url_scheme_handler(handler, options);
    case bundle_operation::remove:
        return remove_url_scheme_handler(handler, options);
    }
    return {};
}

policy_report run_policy(bundle_operation operation, const policy_entry& entry, const apply_options& options)
{
    switch (operation) {
    case bundle_operation::plan:
    case bundle_operation::apply:
        return apply_policy(entry, options);
    case bundle_operation::query:
        return query_policy(entry, options);
    case bundle_operation::remove:
        return remove_policy(entry, options);
    }
    return {};
}

template <typename Entry>
void append_effect_report(
    bundle_operation operation,
    effect_kind kind,
    const Entry& entry,
    const apply_options& options,
    bool owns_cleanup_path,
    bool allow_cleanup_write,
    std::set<std::string>& seen_cleanup_paths,
    desktop_bundle_report& report)
{
    auto effect = run_effect(operation, entry, options);
    effect.kind = kind;
    effect.status = status_for_effect(operation, effect);
    append_activation_plan(effect, report);
    append_report_diagnostics(effect, report);
    if (effect.path) {
        append_generated_cleanup_report(
            operation,
            {*effect.path, false},
            owns_cleanup_path,
            allow_cleanup_write,
            effect.ok,
            effect.present || effect.enabled,
            seen_cleanup_paths,
            report);
    }
    report.artifact_reports.push_back(std::move(effect));
}

void append_policy_report(
    bundle_operation operation,
    const policy_entry& entry,
    const apply_options& options,
    bool allow_cleanup_write,
    std::set<std::string>& seen_cleanup_paths,
    desktop_bundle_report& report)
{
    auto policy = run_policy(operation, entry, options);
    policy.kind = effect_kind::managed_policy;
    policy.status = status_for_policy(operation, policy);
    if (operation != bundle_operation::query) {
        append_policy_activation_plan(entry, report);
    }
    append_report_diagnostics(policy, report);
    if (policy.path) {
        append_generated_cleanup_report(
            operation,
            {*policy.path, false},
            true,
            allow_cleanup_write,
            policy.ok,
            policy.present,
            seen_cleanup_paths,
            report);
    }
    if (entry.enforced) {
        auto diagnostics = policy.diagnostics;
        const auto lock_path = policy_lock_path(entry, options, diagnostics);
        if (!lock_path.empty()) {
            append_generated_cleanup_report(
                operation,
                {lock_path, false},
                true,
                allow_cleanup_write,
                policy.ok,
                policy.present,
                seen_cleanup_paths,
                report);
        }
    }
    report.policy_reports.push_back(std::move(policy));
}

desktop_bundle_report run_bundle(bundle_operation operation, const desktop_bundle& bundle, apply_options options)
{
    if (operation == bundle_operation::plan) {
        options.dry_run = true;
    }

    desktop_bundle_report report;
    report.dry_run = operation == bundle_operation::query ? false : options.dry_run;
    std::set<std::string> seen_cleanup_paths;
    const bool allow_cleanup_write = bundle.scope == registration_scope::user || options.allow_global_write;

    if (bundle.entry) {
        append_effect_report(operation, effect_kind::desktop_entry, desktop_entry_from_bundle(bundle), options, true, allow_cleanup_write, seen_cleanup_paths, report);
    }
    if (bundle.autostart) {
        auto autostart = *bundle.autostart;
        autostart.user_scope = bundle_user_scope(bundle.scope);
        append_effect_report(operation, effect_kind::autostart, autostart, options, true, allow_cleanup_write, seen_cleanup_paths, report);
    }
    for (const auto& icon : bundle.icons) {
        for (const auto& entry : icon_entries_from_reference(icon, bundle.scope)) {
            append_effect_report(operation, effect_kind::icon, entry, options, true, allow_cleanup_write, seen_cleanup_paths, report);
        }
    }
    for (auto declaration : bundle.mime_declarations) {
        declaration.user_scope = bundle_user_scope(bundle.scope);
        append_effect_report(operation, effect_kind::mime_association, declaration, options, true, allow_cleanup_write, seen_cleanup_paths, report);
    }
    for (auto association : bundle.mime_associations) {
        association.user_scope = bundle_user_scope(bundle.scope);
        append_effect_report(operation, effect_kind::mime_association, association, options, false, allow_cleanup_write, seen_cleanup_paths, report);
    }
    for (auto intent : bundle.default_applications) {
        intent.user_scope = bundle_user_scope(bundle.scope);
        append_effect_report(operation, effect_kind::default_application, intent, options, false, allow_cleanup_write, seen_cleanup_paths, report);
    }
    for (auto handler : bundle.url_scheme_handlers) {
        handler.user_scope = bundle_user_scope(bundle.scope);
        append_effect_report(operation, effect_kind::url_protocol_handler, handler, options, false, allow_cleanup_write, seen_cleanup_paths, report);
    }
    for (auto policy : bundle.policies) {
        policy.user_scope = bundle_user_scope(bundle.scope);
        append_policy_report(operation, policy, options, allow_cleanup_write, seen_cleanup_paths, report);
    }
    for (const auto& rule : bundle.cleanup) {
        auto cleanup = run_explicit_cleanup_rule(operation, rule, allow_cleanup_write, seen_cleanup_paths);
        append_cleanup_diagnostics(cleanup, report);
        report.cleanup_reports.push_back(std::move(cleanup));
    }

    report.ok = !has_error(report.diagnostics) &&
        std::all_of(report.artifact_reports.begin(), report.artifact_reports.end(), [](const effect_report& item) {
            return item.ok;
        }) &&
        std::all_of(report.policy_reports.begin(), report.policy_reports.end(), [](const policy_report& item) {
            return item.ok;
        }) &&
        std::all_of(report.cleanup_reports.begin(), report.cleanup_reports.end(), [](const cleanup_report& item) {
            return item.ok;
        });
    return report;
}

} // namespace

desktop_bundle_report plan_bundle(const desktop_bundle& bundle, const apply_options& options)
{
    return run_bundle(bundle_operation::plan, bundle, options);
}

desktop_bundle_report apply_bundle(const desktop_bundle& bundle, const apply_options& options)
{
    return run_bundle(bundle_operation::apply, bundle, options);
}

desktop_bundle_report query_bundle(const desktop_bundle& bundle, const apply_options& options)
{
    return run_bundle(bundle_operation::query, bundle, options);
}

desktop_bundle_report remove_bundle(const desktop_bundle& bundle, const apply_options& options)
{
    return run_bundle(bundle_operation::remove, bundle, options);
}

} // namespace linuxdesktop::desktop
