#include "linuxdesktop/settings.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <system_error>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <ShlObj.h>
#include <objbase.h>
#include <windows.h>
#endif

namespace linuxdesktop::settings {
namespace {

diagnostic make_diagnostic(severity level, std::string code, std::string message, std::filesystem::path path = {})
{
    return diagnostic{level, std::move(code), std::move(message), std::move(path)};
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

std::filesystem::path home_directory()
{
#if defined(_WIN32)
    if (const char* user_profile = std::getenv("USERPROFILE")) {
        return user_profile;
    }
    const char* drive = std::getenv("HOMEDRIVE");
    const char* path = std::getenv("HOMEPATH");
    if (drive && path) {
        return std::string(drive) + std::string(path);
    }
#else
    if (const char* home = std::getenv("HOME")) {
        return home;
    }
#endif
    return {};
}

std::optional<std::filesystem::path> absolute_env_path(const char* name, std::vector<diagnostic>& diagnostics)
{
    const char* value = std::getenv(name);
    if (!value || !value[0]) {
        return std::nullopt;
    }

    std::filesystem::path path(value);
    if (!path.is_absolute()) {
        diagnostics.push_back(make_diagnostic(
            severity::warning,
            "relative-env-ignored",
            std::string(name) + " is relative and was ignored",
            path));
        return std::nullopt;
    }
    return path;
}

#if defined(_WIN32)
std::optional<std::filesystem::path> known_folder(REFKNOWNFOLDERID folder_id, std::vector<diagnostic>& diagnostics, const char* code)
{
    PWSTR path = nullptr;
    const HRESULT result = SHGetKnownFolderPath(folder_id, KF_FLAG_DEFAULT, nullptr, &path);
    if (FAILED(result) || !path) {
        diagnostics.push_back(make_diagnostic(
            severity::warning,
            code,
            "Windows known-folder lookup failed"));
        return std::nullopt;
    }

    std::filesystem::path value(path);
    CoTaskMemFree(path);
    return value;
}
#endif

std::filesystem::path current_directory(std::vector<diagnostic>& diagnostics)
{
    std::error_code ec;
    auto path = std::filesystem::current_path(ec);
    if (ec) {
        diagnostics.push_back(make_diagnostic(
            severity::warning,
            "current-directory-failed",
            ec.message()));
        return {};
    }
    return path;
}

std::filesystem::path executable_resource_guess(std::vector<diagnostic>& diagnostics)
{
#if defined(_WIN32)
    std::wstring buffer(MAX_PATH, L'\0');
    DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    while (size == buffer.size() && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        buffer.resize(buffer.size() * 2);
        size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    }
    if (size > 0) {
        buffer.resize(size);
        return std::filesystem::path(buffer).parent_path();
    }
#else
    std::error_code ec;
    auto exe = std::filesystem::read_symlink("/proc/self/exe", ec);
    if (!ec) {
        return exe.parent_path();
    }
#endif
    diagnostics.push_back(make_diagnostic(
        severity::warning,
        "resource-root-guessed",
        "Could not locate executable directory; using current directory as resource root"));
    return current_directory(diagnostics);
}

void create_directory_if_needed(const std::filesystem::path& path, std::vector<diagnostic>& diagnostics)
{
    if (path.empty()) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "empty-directory",
            "Cannot create an empty directory path"));
        return;
    }

    std::error_code ec;
    if (std::filesystem::exists(path, ec)) {
        if (!std::filesystem::is_directory(path, ec)) {
            diagnostics.push_back(make_diagnostic(
                severity::error,
                "path-not-directory",
                "Path exists but is not a directory",
                path));
        }
        return;
    }

    if (std::filesystem::create_directories(path, ec)) {
        diagnostics.push_back(make_diagnostic(
            severity::info,
            "directory-created",
            "Created directory",
            path));
        return;
    }

    if (ec) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "directory-create-failed",
            ec.message(),
            path));
    }
}

bool has_error(const std::vector<diagnostic>& diagnostics)
{
    for (const auto& item : diagnostics) {
        if (item.level == severity::error) {
            return true;
        }
    }
    return false;
}

std::string read_text(const std::filesystem::path& path, std::error_code& ec)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        ec = std::make_error_code(std::errc::no_such_file_or_directory);
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (!input.good() && !input.eof()) {
        ec = std::make_error_code(std::errc::io_error);
        return {};
    }

    ec.clear();
    return buffer.str();
}

} // namespace

std::string to_string(severity value)
{
    switch (value) {
    case severity::info:
        return "info";
    case severity::warning:
        return "warning";
    case severity::error:
        return "error";
    }
    return "unknown";
}

root_report resolve_app_roots(const app_identity& identity, const root_options& options)
{
    root_report report;
    const auto organization = sanitize_segment(identity.organization);
    const auto application = sanitize_segment(identity.application, "application");
    const auto app_leaf = organization.empty() ? application : organization + "/" + application;

    report.roots.resources = options.resource_root.value_or(executable_resource_guess(report.diagnostics));

    if (options.settings_override) {
        if (options.settings_override->is_absolute()) {
            report.settings_override_active = true;
            report.roots.config = *options.settings_override;
            report.roots.data = *options.settings_override;
            report.roots.state = *options.settings_override;
            report.roots.cache = *options.settings_override / "cache";
            report.roots.runtime = std::filesystem::path{};
        } else {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "settings-override-relative",
                "Settings override must be absolute",
                *options.settings_override));
        }
    }

    report.portable_requested = options.portable_marker.has_value();
    if (!report.settings_override_active && options.portable_marker) {
        const auto marker = *options.portable_marker;
        std::error_code ec;
        if (std::filesystem::exists(marker, ec)) {
            if (options.allow_portable_root) {
                report.portable_active = true;
                auto portable_root = marker.parent_path();
                report.roots.config = portable_root;
                report.roots.data = portable_root;
                report.roots.state = portable_root;
                report.roots.cache = portable_root / "cache";
                report.roots.runtime = std::filesystem::path{};
            } else {
                report.diagnostics.push_back(make_diagnostic(
                    severity::warning,
                    "portable-denied",
                    "Portable marker exists, but portable roots are disabled",
                    marker));
            }
        } else {
            report.diagnostics.push_back(make_diagnostic(
                severity::info,
                "portable-marker-missing",
                "Portable marker was requested but does not exist",
                marker));
        }
    }

    if (report.roots.config.empty() && !has_error(report.diagnostics)) {
#if defined(_WIN32)
        auto roaming = known_folder(FOLDERID_RoamingAppData, report.diagnostics, "known-folder-roaming-failed");
        auto local = known_folder(FOLDERID_LocalAppData, report.diagnostics, "known-folder-local-failed");
        const auto fallback_home = home_directory();

        if (!roaming && !fallback_home.empty()) {
            roaming = fallback_home / "AppData" / "Roaming";
        }
        if (!local && !fallback_home.empty()) {
            local = fallback_home / "AppData" / "Local";
        }

        report.roots.config = roaming.value_or(current_directory(report.diagnostics)) / app_leaf;
        report.roots.data = report.roots.config;
        report.roots.state = local.value_or(report.roots.config) / app_leaf / "state";
        report.roots.cache = local.value_or(report.roots.config) / app_leaf / "cache";
        report.roots.runtime = std::filesystem::path{};
#else
        const auto home = home_directory();
        auto config_home = absolute_env_path("XDG_CONFIG_HOME", report.diagnostics).value_or(home / ".config");
        auto data_home = absolute_env_path("XDG_DATA_HOME", report.diagnostics).value_or(home / ".local" / "share");
        auto state_home = absolute_env_path("XDG_STATE_HOME", report.diagnostics).value_or(home / ".local" / "state");
        auto cache_home = absolute_env_path("XDG_CACHE_HOME", report.diagnostics).value_or(home / ".cache");
        auto runtime_dir = absolute_env_path("XDG_RUNTIME_DIR", report.diagnostics).value_or(std::filesystem::path{});

        report.roots.config = config_home / app_leaf;
        report.roots.data = data_home / app_leaf;
        report.roots.state = state_home / app_leaf;
        report.roots.cache = cache_home / app_leaf;
        report.roots.runtime = runtime_dir.empty() ? runtime_dir : runtime_dir / application;
#endif
    }

    report.roots.session = report.roots.state / "sessions";
    report.roots.plugin_config = report.roots.config / "plugins" / "Config";

    if (options.create_directories && !has_error(report.diagnostics)) {
        create_directory_if_needed(report.roots.config, report.diagnostics);
        create_directory_if_needed(report.roots.data, report.diagnostics);
        create_directory_if_needed(report.roots.state, report.diagnostics);
        create_directory_if_needed(report.roots.cache, report.diagnostics);
        create_directory_if_needed(report.roots.session, report.diagnostics);
        create_directory_if_needed(report.roots.plugin_config, report.diagnostics);
        if (!report.roots.runtime.empty()) {
            create_directory_if_needed(report.roots.runtime, report.diagnostics);
        }
    }

    return report;
}

hydrate_report hydrate_config_bundle(const hydrate_options& options)
{
    hydrate_report report;
    if (options.create_target_root) {
        create_directory_if_needed(options.target_root, report.diagnostics);
    }

    if (has_error(report.diagnostics)) {
        return report;
    }

    for (const auto& file : options.files) {
        const auto target = options.target_root / file.name;
        const auto model = options.model_root / file.model_name;

        std::error_code ec;
        if (std::filesystem::exists(target, ec)) {
            report.skipped_existing.push_back(target);
            continue;
        }

        if (!std::filesystem::exists(model, ec)) {
            report.diagnostics.push_back(make_diagnostic(
                file.required ? severity::error : severity::warning,
                file.required ? "required-model-missing" : "optional-model-missing",
                "Model file does not exist",
                model));
            continue;
        }

        std::filesystem::copy_file(model, target, std::filesystem::copy_options::none, ec);
        if (ec) {
            report.diagnostics.push_back(make_diagnostic(
                file.required ? severity::error : severity::warning,
                file.required ? "required-copy-failed" : "optional-copy-failed",
                ec.message(),
                target));
            continue;
        }

        report.copied.push_back(target);
        report.diagnostics.push_back(make_diagnostic(
            severity::info,
            "config-hydrated",
            "Copied model file into config bundle",
            target));
    }

    return report;
}

write_report write_with_backup(const write_options& options, validation_callback validate)
{
    write_report report;
    create_directory_if_needed(options.target.parent_path(), report.diagnostics);
    if (has_error(report.diagnostics)) {
        return report;
    }

    std::error_code ec;
    const auto backup = options.target.string() + ".bak";
    if (options.keep_backup && std::filesystem::exists(options.target, ec)) {
        std::filesystem::copy_file(options.target, backup, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "backup-copy-failed",
                ec.message(),
                backup));
            return report;
        }
        report.backup_path = backup;
    }

    {
        std::ofstream output(options.target, std::ios::binary | std::ios::trunc);
        if (!output) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "write-open-failed",
                "Could not open target for writing",
                options.target));
            return report;
        }
        output.write(options.content.data(), static_cast<std::streamsize>(options.content.size()));
        if (!output) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "write-failed",
                "Could not write target content",
                options.target));
            return report;
        }
    }

    if (validate) {
        std::string validation_message;
        if (!validate(options.target, validation_message)) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "validation-failed",
                validation_message.empty() ? "Written file failed validation" : validation_message,
                options.target));

            if (report.backup_path) {
                std::filesystem::copy_file(*report.backup_path, options.target, std::filesystem::copy_options::overwrite_existing, ec);
                if (ec) {
                    report.diagnostics.push_back(make_diagnostic(
                        severity::error,
                        "backup-restore-failed",
                        ec.message(),
                        options.target));
                } else {
                    report.diagnostics.push_back(make_diagnostic(
                        severity::warning,
                        "backup-restored",
                        "Restored previous target from backup",
                        options.target));
                }
            }
            return report;
        }
    }

    std::error_code read_ec;
    static_cast<void>(read_text(options.target, read_ec));
    if (read_ec) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "write-readback-failed",
            read_ec.message(),
            options.target));
        return report;
    }

    report.ok = true;
    report.diagnostics.push_back(make_diagnostic(
        severity::info,
        "write-ok",
        "Wrote target file",
        options.target));
    return report;
}

} // namespace linuxdesktop::settings
