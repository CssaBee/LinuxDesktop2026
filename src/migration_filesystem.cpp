#include "migration_internal.hpp"

#include <filesystem>
#include <system_error>

namespace linuxdesktop::migration {

migration_execution_report execute_migration_plan(const migration_plan& plan, const options& options)
{
    migration_execution_report report;
    report.dry_run = options.dry_run;

    if (internal::has_error(plan.diagnostics)) {
        report.diagnostics.push_back(internal::make_diagnostic(
            severity::error,
            "migration-plan-invalid",
            "Migration plan has errors and cannot be executed"));
        report.actions.reserve(plan.actions.size());
        for (const auto& action : plan.actions) {
            migration_action_result result;
            result.action = action;
            result.state = migration_action_state::blocked;
            result.planned = true;
            result.skipped = true;
            result.source_existed_before = internal::path_exists_noerror(action.source_path);
            result.target_existed_before = internal::path_exists_noerror(action.target_path);
            internal::append_action_gate_diagnostics(action, options, result.diagnostics);
            internal::append_file_action_diagnostics(action, options, result.diagnostics);
            internal::record_after_paths(result);
            report.actions.push_back(std::move(result));
        }
        return report;
    }

    report.ok = true;
    report.actions.reserve(plan.actions.size());
    for (const auto& action : plan.actions) {
        migration_action_result result;
        result.action = action;
        result.planned = true;
        result.source_existed_before = internal::path_exists_noerror(action.source_path);
        result.target_existed_before = internal::path_exists_noerror(action.target_path);
        internal::append_action_gate_diagnostics(action, options, result.diagnostics);
        internal::append_file_action_diagnostics(action, options, result.diagnostics);

        if (internal::has_error(result.diagnostics)) {
            result.state = migration_action_state::blocked;
            result.skipped = true;
            report.ok = false;
            internal::record_after_paths(result);
            report.actions.push_back(std::move(result));
            continue;
        }

        if (options.dry_run) {
            result.state = migration_action_state::skipped;
            result.skipped = true;
            result.diagnostics.push_back(internal::make_diagnostic(
                severity::info,
                "migration-dry-run",
                "Migration action was planned but not executed because dry_run is true"));
            internal::record_after_paths(result);
            report.actions.push_back(std::move(result));
            continue;
        }

        if (!internal::is_file_action(action.kind)) {
            result.state = migration_action_state::unsupported;
            result.skipped = true;
            report.ok = false;
            result.diagnostics.push_back(internal::make_diagnostic(
                severity::error,
                "migration-action-not-executable-yet",
                "This migration action kind does not have an executor yet"));
            internal::record_after_paths(result);
            report.actions.push_back(std::move(result));
            continue;
        }

        if (!options.allow_dangerous && internal::action_requires_dangerous_permission(action.kind)) {
            result.state = migration_action_state::blocked;
            result.skipped = true;
            report.ok = false;
            result.diagnostics.push_back(internal::make_diagnostic(
                severity::error,
                "migration-dangerous-action-denied",
                "Destructive migration action requires allow_dangerous"));
            internal::record_after_paths(result);
            report.actions.push_back(std::move(result));
            continue;
        }

        std::error_code ec;
        if (options.create_parent_directories) {
            std::filesystem::create_directories(action.target_path.parent_path(), ec);
            if (ec) {
                result.state = migration_action_state::blocked;
                result.skipped = true;
                report.ok = false;
                result.diagnostics.push_back(internal::make_diagnostic(
                    severity::error,
                    "migration-create-parent-failed",
                    ec.message(),
                    action.target_path.parent_path()));
                internal::record_after_paths(result);
                report.actions.push_back(std::move(result));
                continue;
            }
        }

        const auto copy_options = options.overwrite_existing
            ? std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing
            : std::filesystem::copy_options::recursive;

        if (internal::is_directory_action(action.kind) || action.kind == migration_action_kind::copy_file) {
            if (internal::is_directory_action(action.kind)) {
                std::filesystem::copy(action.source_path, action.target_path, copy_options, ec);
            } else {
                std::filesystem::copy_file(
                    action.source_path,
                    action.target_path,
                    options.overwrite_existing
                        ? std::filesystem::copy_options::overwrite_existing
                        : std::filesystem::copy_options::none,
                    ec);
            }
            if (ec) {
                result.state = migration_action_state::blocked;
                result.skipped = true;
                report.ok = false;
                result.diagnostics.push_back(internal::make_diagnostic(
                    severity::error,
                    "migration-copy-failed",
                    ec.message(),
                    action.target_path));
                internal::record_after_paths(result);
                report.actions.push_back(std::move(result));
                continue;
            }
        }

        if (internal::is_move_action(action.kind)) {
            if (!internal::is_directory_action(action.kind)) {
                std::filesystem::rename(action.source_path, action.target_path, ec);
            } else {
                std::filesystem::remove_all(action.source_path, ec);
            }
            if (ec) {
                report.ok = false;
                result.rollback_available = internal::is_directory_action(action.kind) &&
                    internal::path_exists_noerror(action.target_path) &&
                    !result.target_existed_before;
                result.state = result.rollback_available
                    ? migration_action_state::partially_executed
                    : migration_action_state::rollback_missing;
                if (result.rollback_available) {
                    result.rollback_attempted = true;
                    result.rollback_path = action.target_path;
                    std::error_code rollback_ec;
                    std::filesystem::remove_all(action.target_path, rollback_ec);
                    result.rollback_succeeded = !rollback_ec;
                    if (!result.rollback_succeeded) {
                        result.state = migration_action_state::rollback_failed;
                    }
                    result.diagnostics.push_back(internal::make_diagnostic(
                        result.rollback_succeeded ? severity::warning : severity::error,
                        result.rollback_succeeded ? "migration-rollback-succeeded" : "migration-rollback-failed",
                        result.rollback_succeeded ? "Removed copied target after move cleanup failed" : rollback_ec.message(),
                        action.target_path));
                }
                result.diagnostics.push_back(internal::make_diagnostic(
                    severity::error,
                    internal::is_directory_action(action.kind) ? "migration-move-cleanup-failed" : "migration-file-rename-failed",
                    internal::is_directory_action(action.kind)
                        ? ec.message()
                        : "Atomic file rename failed; cross-device copy/remove fallback is not supported: " + ec.message(),
                    action.source_path));
                internal::record_after_paths(result);
                report.actions.push_back(std::move(result));
                continue;
            }
        }

        result.state = migration_action_state::executed;
        result.executed = true;
        internal::record_after_paths(result);
        report.actions.push_back(std::move(result));
    }

    return report;
}

} // namespace linuxdesktop::migration
