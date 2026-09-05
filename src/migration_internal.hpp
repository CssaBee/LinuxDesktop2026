#pragma once

#include "linuxdesktop/migration.hpp"

#include <algorithm>
#include <filesystem>
#include <system_error>
#include <utility>
#include <vector>

namespace linuxdesktop::migration::internal {

inline diagnostic make_diagnostic(
    severity level,
    std::string code,
    std::string message,
    std::filesystem::path path = {})
{
    return diagnostic{level, std::move(code), std::move(message), std::move(path)};
}

inline bool has_error(const std::vector<diagnostic>& diagnostics)
{
    return std::any_of(diagnostics.begin(), diagnostics.end(), [](const diagnostic& item) {
        return item.level == severity::error;
    });
}

inline bool is_file_action(migration_action_kind kind)
{
    return kind == migration_action_kind::copy_file ||
        kind == migration_action_kind::rename_file ||
        kind == migration_action_kind::copy_directory ||
        kind == migration_action_kind::move_directory;
}

inline bool is_directory_action(migration_action_kind kind)
{
    return kind == migration_action_kind::copy_directory ||
        kind == migration_action_kind::move_directory;
}

inline bool is_move_action(migration_action_kind kind)
{
    return kind == migration_action_kind::rename_file ||
        kind == migration_action_kind::move_directory;
}

inline bool action_requires_dangerous_permission(migration_action_kind kind)
{
    return kind == migration_action_kind::rename_file ||
        kind == migration_action_kind::move_directory ||
        kind == migration_action_kind::delete_registry_key;
}

inline bool path_exists_noerror(const std::filesystem::path& path)
{
    if (path.empty()) {
        return false;
    }
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

inline void append_action_gate_diagnostics(
    const migration_action& action,
    const options& options,
    std::vector<diagnostic>& diagnostics)
{
    if (action.dangerous && !options.allow_dangerous) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "migration-dangerous-action-denied",
            "Dangerous migration action requires allow_dangerous"));
    }
    if (action.requires_elevation && !options.allow_elevation) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "migration-elevation-denied",
            "Migration action requires elevation permission"));
    }
}

inline void append_regular_file_metadata_note(
    const std::filesystem::path& path,
    std::vector<diagnostic>& diagnostics)
{
    std::error_code ec;
    const auto count = std::filesystem::hard_link_count(path, ec);
    if (!ec && count > 1) {
        diagnostics.push_back(make_diagnostic(
            severity::warning,
            "migration-hard-link-topology-not-preserved",
            "Migration copies this regular file as independent app settings data; hard-link topology is not preserved",
            path));
    }
}

inline void append_unsupported_object_diagnostic(
    const std::filesystem::path& path,
    std::vector<diagnostic>& diagnostics)
{
    diagnostics.push_back(make_diagnostic(
        severity::error,
        "migration-source-unsupported-object",
        "Migration supports only regular files and directories containing regular files or subdirectories",
        path));
}

inline void append_source_object_model_diagnostics(
    const migration_action& action,
    std::vector<diagnostic>& diagnostics)
{
    if (!is_file_action(action.kind) || action.source_path.empty()) {
        return;
    }

    std::error_code ec;
    if (!std::filesystem::exists(action.source_path, ec)) {
        return;
    }

    const auto root_status = std::filesystem::symlink_status(action.source_path, ec);
    if (ec) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "migration-source-status-failed",
            ec.message(),
            action.source_path));
        return;
    }
    if (std::filesystem::is_symlink(root_status)) {
        append_unsupported_object_diagnostic(action.source_path, diagnostics);
        return;
    }
    if (std::filesystem::is_regular_file(root_status)) {
        append_regular_file_metadata_note(action.source_path, diagnostics);
        return;
    }
    if (!std::filesystem::is_directory(root_status)) {
        append_unsupported_object_diagnostic(action.source_path, diagnostics);
        return;
    }

    if (!is_directory_action(action.kind)) {
        return;
    }

    auto iterator = std::filesystem::recursive_directory_iterator(action.source_path, std::filesystem::directory_options::none, ec);
    if (ec) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "migration-source-scan-failed",
            ec.message(),
            action.source_path));
        return;
    }

    for (const auto end = std::filesystem::recursive_directory_iterator(); iterator != end; iterator.increment(ec)) {
        if (ec) {
            diagnostics.push_back(make_diagnostic(
                severity::error,
                "migration-source-scan-failed",
                ec.message(),
                action.source_path));
            return;
        }

        const auto entry_status = iterator->symlink_status(ec);
        if (ec) {
            diagnostics.push_back(make_diagnostic(
                severity::error,
                "migration-source-status-failed",
                ec.message(),
                iterator->path()));
            return;
        }
        if (std::filesystem::is_symlink(entry_status)) {
            append_unsupported_object_diagnostic(iterator->path(), diagnostics);
            return;
        }
        if (std::filesystem::is_regular_file(entry_status)) {
            append_regular_file_metadata_note(iterator->path(), diagnostics);
            continue;
        }
        if (!std::filesystem::is_directory(entry_status)) {
            append_unsupported_object_diagnostic(iterator->path(), diagnostics);
            return;
        }
    }
}

inline void record_after_paths(migration_action_result& result)
{
    result.source_exists_after = path_exists_noerror(result.action.source_path);
    result.target_exists_after = path_exists_noerror(result.action.target_path);
}

inline void append_file_action_diagnostics(
    const migration_action& action,
    const options& options,
    std::vector<diagnostic>& diagnostics)
{
    if (!is_file_action(action.kind)) {
        diagnostics.push_back(make_diagnostic(
            severity::warning,
            "migration-action-not-executable-yet",
            "This migration action kind is planned but does not have an executor yet"));
        return;
    }

    if (action.source_path.empty()) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "migration-source-empty",
            "File migration action requires a source path"));
    }
    if (action.target_path.empty()) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "migration-target-empty",
            "File migration action requires a target path"));
    }

    std::error_code ec;
    if (!action.source_path.empty() && !std::filesystem::exists(action.source_path, ec)) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "migration-source-missing",
            "Migration source does not exist",
            action.source_path));
    }
    if (!action.source_path.empty() && std::filesystem::exists(action.source_path, ec)) {
        const auto directory = std::filesystem::is_directory(action.source_path, ec);
        if (is_directory_action(action.kind) != directory) {
            diagnostics.push_back(make_diagnostic(
                severity::error,
                "migration-source-kind-mismatch",
                "Migration source kind does not match the action",
                action.source_path));
        }
    }
    append_source_object_model_diagnostics(action, diagnostics);
    if (action.kind == migration_action_kind::rename_file) {
        diagnostics.push_back(make_diagnostic(
            severity::info,
            "migration-file-rename-atomic-only",
            "File rename actions use atomic rename only; cross-device copy/remove fallback is not supported",
            action.source_path));
    }
    if (action.kind == migration_action_kind::move_directory) {
        diagnostics.push_back(make_diagnostic(
            severity::warning,
            "migration-directory-move-best-effort",
            "Directory moves copy supported entries, then remove the source tree; concurrent mutation and full rollback are not guaranteed",
            action.source_path));
    }
    if (!action.target_path.empty() && std::filesystem::exists(action.target_path, ec) && !options.overwrite_existing) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "migration-target-exists",
            "Migration target exists and overwrite_existing is false",
            action.target_path));
    }
}

} // namespace linuxdesktop::migration::internal
