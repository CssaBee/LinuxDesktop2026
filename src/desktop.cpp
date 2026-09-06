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
        item.state = capability_state::backend_missing;
        item.diagnostics.push_back(unsupported_backend_diagnostic(kind));
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

#if defined(_WIN32)
    report.diagnostics.push_back(unsupported_backend_diagnostic(effect_kind::autostart));
    return report;
#else
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

    report.path = autostart_path(entry, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    if (options.dry_run) {
        report.ok = true;
        report.diagnostics.push_back(make_diagnostic(severity::info, "autostart-dry-run", "Autostart desktop entry removal was planned but not applied", *report.path));
        return report;
    }

#if defined(_WIN32)
    report.diagnostics.push_back(unsupported_backend_diagnostic(effect_kind::autostart));
    return report;
#else
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
    report.diagnostics.push_back(unsupported_backend_diagnostic(effect_kind::autostart));
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
#if defined(_WIN32)
    report.diagnostics.push_back(unsupported_backend_diagnostic(effect_kind::desktop_entry));
    return report;
#else
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
    report.path = desktop_entry_path(entry, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    report.activation_plan.push_back(activation_required(activation_step_kind::refresh_desktop_database, "update-desktop-database " + report.path->parent_path().string(), "desktop-database-refresh-required", "Refresh the desktop database after removing the desktop entry artifact"));
    if (options.dry_run) {
        report.ok = true;
        return report;
    }
#if defined(_WIN32)
    report.diagnostics.push_back(unsupported_backend_diagnostic(effect_kind::desktop_entry));
    return report;
#else
    std::error_code ec;
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
    report.path = desktop_entry_path(entry, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
#if defined(_WIN32)
    report.diagnostics.push_back(unsupported_backend_diagnostic(effect_kind::desktop_entry));
    return report;
#else
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
    report.path = icon_path(entry, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    report.activation_plan.push_back(activation_required(activation_step_kind::refresh_icon_cache, "gtk-update-icon-cache " + (report.path->parent_path().parent_path().parent_path()).string(), "icon-cache-refresh-required", "Icon artifact was staged; refresh the icon cache before treating it as active"));
    if (options.dry_run) {
        report.ok = true;
        return report;
    }
#if defined(_WIN32)
    report.diagnostics.push_back(unsupported_backend_diagnostic(effect_kind::icon));
    return report;
#else
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
    report.path = icon_path(entry, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    report.activation_plan.push_back(activation_required(activation_step_kind::refresh_icon_cache, "gtk-update-icon-cache " + (report.path->parent_path().parent_path().parent_path()).string(), "icon-cache-refresh-required", "Refresh the icon cache after removing the icon artifact"));
    if (options.dry_run) {
        report.ok = true;
        return report;
    }
#if defined(_WIN32)
    report.diagnostics.push_back(unsupported_backend_diagnostic(effect_kind::icon));
    return report;
#else
    std::error_code ec;
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
    report.path = icon_path(entry, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
#if defined(_WIN32)
    report.diagnostics.push_back(unsupported_backend_diagnostic(effect_kind::icon));
    return report;
#else
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
    report.path = mime_declaration_path(declaration, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    report.activation_plan.push_back(activation_required(activation_step_kind::refresh_mime_database, "update-mime-database " + report.path->parent_path().parent_path().string(), "mime-database-refresh-required", "MIME declaration was staged; refresh the MIME database before treating it as active"));
    if (options.dry_run) {
        report.ok = true;
        return report;
    }
#if defined(_WIN32)
    report.diagnostics.push_back(unsupported_backend_diagnostic(effect_kind::mime_association));
    return report;
#else
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
    report.path = mime_declaration_path(declaration, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    report.activation_plan.push_back(activation_required(activation_step_kind::refresh_mime_database, "update-mime-database " + report.path->parent_path().parent_path().string(), "mime-database-refresh-required", "Refresh the MIME database after removing the MIME package file"));
    if (options.dry_run) {
        report.ok = true;
        return report;
    }
#if defined(_WIN32)
    report.diagnostics.push_back(unsupported_backend_diagnostic(effect_kind::mime_association));
    return report;
#else
    std::error_code ec;
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
    report.path = mime_declaration_path(declaration, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
#if defined(_WIN32)
    report.diagnostics.push_back(unsupported_backend_diagnostic(effect_kind::mime_association));
    return report;
#else
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
    report.path = mimeapps_file(association.user_scope, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    report.activation_plan.push_back(activation_required(activation_step_kind::refresh_desktop_database, "update-desktop-database " + report.path->parent_path().string(), "desktop-database-refresh-required", "MIME association artifact was staged; refresh the desktop database before treating it as active"));
    if (options.dry_run) {
        report.ok = true;
        return report;
    }
#if defined(_WIN32)
    report.diagnostics.push_back(unsupported_backend_diagnostic(effect_kind::mime_association));
    return report;
#else
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
    report.path = mimeapps_file(association.user_scope, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
    if (options.dry_run) {
        report.ok = true;
        return report;
    }
#if defined(_WIN32)
    report.diagnostics.push_back(unsupported_backend_diagnostic(effect_kind::mime_association));
    return report;
#else
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
    report.path = mimeapps_file(association.user_scope, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
#if defined(_WIN32)
    report.diagnostics.push_back(unsupported_backend_diagnostic(effect_kind::mime_association));
    return report;
#else
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
#if defined(_WIN32)
    report.diagnostics.push_back(unsupported_backend_diagnostic(effect_kind::default_application));
    return report;
#else
    std::error_code ec;
    std::filesystem::create_directories(report.path->parent_path(), ec);
    if (ec) {
        report.diagnostics.push_back(make_diagnostic(severity::error, "mimeapps-create-directory-failed", ec.message(), report.path->parent_path()));
        return report;
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
    report.path = mimeapps_file(intent.user_scope, options, report.diagnostics);
    if (!report.path || report.path->empty() || has_error(report.diagnostics)) {
        return report;
    }
#if defined(_WIN32)
    report.diagnostics.push_back(unsupported_backend_diagnostic(effect_kind::default_application));
    return report;
#else
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
}

effect_report remove_url_scheme_handler(const url_scheme_handler& handler, const apply_options& options)
{
    if (!is_safe_scheme(handler.scheme)) {
        effect_report report;
        report.diagnostics.push_back(make_diagnostic(severity::error, "url-scheme-invalid", "URL scheme must follow RFC-style scheme token syntax"));
        return report;
    }
    default_application_intent intent;
    intent.mime_type_or_scheme = scheme_mime_key(handler.scheme);
    intent.desktop_entry_id = handler.desktop_entry_id;
    intent.make_default = false;
    intent.user_scope = handler.user_scope;
    return apply_default_application(intent, options);
}

effect_report query_url_scheme_handler(const url_scheme_handler& handler, const apply_options& options)
{
    if (!is_safe_scheme(handler.scheme)) {
        effect_report report;
        report.diagnostics.push_back(make_diagnostic(severity::error, "url-scheme-invalid", "URL scheme must follow RFC-style scheme token syntax"));
        return report;
    }
    default_application_intent intent;
    intent.mime_type_or_scheme = scheme_mime_key(handler.scheme);
    intent.desktop_entry_id = handler.desktop_entry_id;
    intent.user_scope = handler.user_scope;
    return query_default_application(intent, options);
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

#if defined(_WIN32)
    report.diagnostics.push_back(unsupported_backend_diagnostic(effect_kind::managed_policy));
    return report;
#else
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

#if defined(_WIN32)
    report.diagnostics.push_back(unsupported_backend_diagnostic(effect_kind::managed_policy));
    return report;
#else
    std::error_code ec;
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
    report.diagnostics.push_back(unsupported_backend_diagnostic(effect_kind::managed_policy));
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

} // namespace linuxdesktop::desktop
