#include "linuxdesktop/desktop.hpp"

#include "durable_file_write.hpp"
#include "linuxdesktop/paths.hpp"

#include <algorithm>
#include <cctype>
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
    case capability_state::sandbox_limited:
        return "sandbox_limited";
    case capability_state::permission_denied:
        return "permission_denied";
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
        if (kind == effect_kind::autostart || kind == effect_kind::managed_policy) {
            item.state = capability_state::supported;
            item.can_query = true;
            item.can_write_user =
                kind == effect_kind::managed_policy ? options.allow_policy_write : options.allow_desktop_integration_write;
            item.can_write_global = item.can_write_user && options.allow_global_write;
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
    return report;
#endif
}

} // namespace linuxdesktop::desktop
