#pragma once

#include <filesystem>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

namespace linuxdesktop {

enum class severity {
    info,
    warning,
    error
};

struct diagnostic {
    severity level = severity::info;
    std::string code;
    std::string message;
    std::filesystem::path path;
};

enum class diagnostic_disposition {
    log,
    status,
    prompt
};

struct diagnostic_handling {
    bool log = true;
    bool status_visible = false;
    bool prompt_user = false;
};

inline std::string to_string(severity value)
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

inline std::string to_string(diagnostic_disposition value)
{
    switch (value) {
    case diagnostic_disposition::log:
        return "log";
    case diagnostic_disposition::status:
        return "status";
    case diagnostic_disposition::prompt:
        return "prompt";
    }
    return "unknown";
}

inline diagnostic_disposition default_diagnostic_disposition(severity value)
{
    switch (value) {
    case severity::info:
        return diagnostic_disposition::log;
    case severity::warning:
        return diagnostic_disposition::status;
    case severity::error:
        return diagnostic_disposition::prompt;
    }
    return diagnostic_disposition::prompt;
}

inline bool diagnostic_code_is_one_of(std::string_view code, std::initializer_list<std::string_view> values)
{
    for (const auto value : values) {
        if (code == value) {
            return true;
        }
    }
    return false;
}

inline diagnostic_disposition disposition_for_diagnostic_code(std::string_view code, severity fallback_level)
{
    if (diagnostic_code_is_one_of(code, {
            "directory-created",
            "config-hydrated",
            "write-ok",
            "write-ok-durable",
            "temp-cleaned",
            "paths.path_list.empty_entry_ignored",
            "paths.path_list.duplicate_ignored",
            "paths.legacy.relative_ignored",
            "paths.xdg_user_dir.malformed",
            "paths.xdg_user_dir.relative_ignored",
            "paths.xdg_user_dir.unreadable",
            "watch.backend.inotify",
            "watch.backend.windows",
            "watch.backend.libuv",
            "watch.recursive.native",
            "watch.recursive.emulated",
            "watch.recursive.symlink_skipped",
            "watch.recursive.duplicate_skipped",
            "watch.recursive.discovered",
            "watch.rename.unpaired",
            "watch.settle.ready",
        })) {
        return diagnostic_disposition::log;
    }

    if (diagnostic_code_is_one_of(code, {
            "app-root-override-relative",
            "user-config-override-relative",
            "user-config-override-ignored",
            "user-config-override-ignored-app-local",
            "user-config-override-ignored-portable",
            "settings-override-relative",
            "sync-config-override-relative",
            "sync-config-override-ignored",
            "sync-config-override-ignored-portable",
            "app_local-marker-missing",
            "portable-marker-missing",
            "portable-root-missing",
            "backup-restored",
            "backup-restored-after-readback-failure",
            "migration-source-missing",
            "paths.environment.relative_ignored",
            "paths.override.relative_ignored",
            "paths.platform_default.relative_ignored",
            "paths.executable.unavailable",
            "paths.temp.unavailable",
            "paths.path_list.relative_ignored",
            "watch.overflow",
            "watch.rescan_recommended",
            "watch.settle.timeout",
        })) {
        return diagnostic_disposition::status;
    }

    if (diagnostic_code_is_one_of(code, {
            "app_local-denied-privileged-install",
            "portable-denied-privileged-install",
            "app_local-denied",
            "portable-denied",
            "portable-root-relative",
            "empty-directory",
            "path-not-directory",
            "directory-create-failed",
            "named-root-name-empty",
            "named-root-relative-path-absolute",
            "named-root-base-empty",
            "named-root-duplicate",
            "component-root-name-empty",
            "required-model-missing",
            "required-copy-failed",
            "backup-copy-failed",
            "temp-open-failed",
            "temp-write-failed",
            "temp-close-failed",
            "temp-cleanup-failed",
            "backup-restore-failed",
            "write-failed",
            "write-readback-failed",
            "write-readback-mismatch",
            "validation-failed",
            "close-failed",
            "durable-flush-failed",
            "parent-flush-failed",
            "atomic-replace-failed",
            "migration-helper-source-empty",
            "migration-source-empty",
            "migration-target-empty",
            "migration-source-kind-ambiguous",
            "migration-source-kind-mismatch",
            "migration-target-exists",
            "migration-dangerous-action-denied",
            "migration-elevation-denied",
            "migration-action-not-executable-yet",
            "registry-hklm-write-denied",
            "registry-policy-write-denied",
            "registry-recursive-delete-denied",
            "paths.identity.application_missing",
            "paths.home.missing",
            "paths.directory.exists_as_file",
            "paths.directory.parent_missing",
            "paths.directory.create_failed",
            "watch.backend.unavailable",
            "watch.backend.error",
            "watch.path.not_found",
            "watch.path.unsupported_type",
            "watch.path.access_denied",
            "watch.recursive.unsupported",
            "watch.callback.exception",
            "watch.queue.overflow",
            "watch.resource.limit",
        })) {
        return diagnostic_disposition::prompt;
    }

    return default_diagnostic_disposition(fallback_level);
}

inline diagnostic_disposition disposition_for_diagnostic(const diagnostic& source)
{
    return disposition_for_diagnostic_code(source.code, source.level);
}

inline diagnostic_handling handling_for_diagnostic_disposition(diagnostic_disposition value)
{
    switch (value) {
    case diagnostic_disposition::log:
        return {true, false, false};
    case diagnostic_disposition::status:
        return {true, true, false};
    case diagnostic_disposition::prompt:
        return {true, true, true};
    }
    return {true, true, true};
}

inline diagnostic_handling handling_for_diagnostic(const diagnostic& source)
{
    return handling_for_diagnostic_disposition(disposition_for_diagnostic(source));
}

template <typename ProductSeverity>
struct product_diagnostic_severity_map {
    ProductSeverity info;
    ProductSeverity warning;
    ProductSeverity error;
};

struct product_diagnostic_options {
    std::string code_prefix;
};

template <typename ProductSeverity>
ProductSeverity to_product_severity(
    severity value,
    const product_diagnostic_severity_map<ProductSeverity>& mapping)
{
    switch (value) {
    case severity::info:
        return mapping.info;
    case severity::warning:
        return mapping.warning;
    case severity::error:
        return mapping.error;
    }
    return mapping.error;
}

template <typename ProductDiagnostic, typename ProductSeverity, typename Factory>
ProductDiagnostic make_product_diagnostic(
    const diagnostic& source,
    const product_diagnostic_severity_map<ProductSeverity>& mapping,
    Factory make,
    const product_diagnostic_options& options = {})
{
    return make(
        to_product_severity(source.level, mapping),
        options.code_prefix + source.code,
        source.message,
        source.path);
}

template <
    typename ProductDiagnostic,
    typename ProductSeverity,
    typename Factory>
ProductDiagnostic make_classified_product_diagnostic(
    const diagnostic& source,
    const product_diagnostic_severity_map<ProductSeverity>& severity_mapping,
    Factory make,
    const product_diagnostic_options& options = {})
{
    return make(
        to_product_severity(source.level, severity_mapping),
        options.code_prefix + source.code,
        source.message,
        source.path,
        handling_for_diagnostic(source));
}

template <typename ProductDiagnostic, typename ProductSeverity, typename Factory>
std::vector<ProductDiagnostic> make_product_diagnostics(
    const std::vector<diagnostic>& source,
    const product_diagnostic_severity_map<ProductSeverity>& mapping,
    Factory make,
    const product_diagnostic_options& options = {})
{
    std::vector<ProductDiagnostic> result;
    result.reserve(source.size());
    for (const auto& item : source) {
        result.push_back(make_product_diagnostic<ProductDiagnostic>(item, mapping, make, options));
    }
    return result;
}

template <
    typename ProductDiagnostic,
    typename ProductSeverity,
    typename Factory>
std::vector<ProductDiagnostic> make_classified_product_diagnostics(
    const std::vector<diagnostic>& source,
    const product_diagnostic_severity_map<ProductSeverity>& severity_mapping,
    Factory make,
    const product_diagnostic_options& options = {})
{
    std::vector<ProductDiagnostic> result;
    result.reserve(source.size());
    for (const auto& item : source) {
        result.push_back(make_classified_product_diagnostic<ProductDiagnostic>(
            item,
            severity_mapping,
            make,
            options));
    }
    return result;
}

} // namespace linuxdesktop
