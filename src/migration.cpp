#include "migration_internal.hpp"

namespace linuxdesktop::migration {

std::string_view to_string(migration_action_kind value)
{
    switch (value) {
    case migration_action_kind::copy_file:
        return "copy_file";
    case migration_action_kind::rename_file:
        return "rename_file";
    case migration_action_kind::copy_directory:
        return "copy_directory";
    case migration_action_kind::move_directory:
        return "move_directory";
    case migration_action_kind::import_registry:
        return "import_registry";
    case migration_action_kind::export_registry:
        return "export_registry";
    case migration_action_kind::write_registry_value:
        return "write_registry_value";
    case migration_action_kind::delete_registry_key:
        return "delete_registry_key";
    }
    return "unknown";
}

std::string_view to_string(migration_action_state value)
{
    switch (value) {
    case migration_action_state::planned:
        return "planned";
    case migration_action_state::executed:
        return "executed";
    case migration_action_state::skipped:
        return "skipped";
    case migration_action_state::blocked:
        return "blocked";
    case migration_action_state::unsupported:
        return "unsupported";
    case migration_action_state::partially_executed:
        return "partially_executed";
    case migration_action_state::rollback_missing:
        return "rollback_missing";
    case migration_action_state::rollback_failed:
        return "rollback_failed";
    }
    return "unknown";
}

rooted_path_report resolve_rooted_path(const rooted_path_request& request)
{
    rooted_path_report report;
    if (!request.relative_path.empty() && request.relative_path.is_absolute()) {
        report.diagnostics.push_back(internal::make_diagnostic(
            severity::error,
            "migration-rooted-path-relative-required",
            "Rooted migration paths require a relative path",
            request.relative_path));
        return report;
    }

    const auto paths = paths::resolve_app_paths(request.identity, request.resolver_options);
    report.diagnostics.insert(report.diagnostics.end(), paths.diagnostics.begin(), paths.diagnostics.end());
    const auto selected = paths.selected.find(request.family);
    if (selected == paths.selected.end() || selected->second.empty()) {
        report.diagnostics.push_back(internal::make_diagnostic(
            severity::error,
            "migration-rooted-path-family-missing",
            "Could not resolve requested migration path family"));
        return report;
    }

    report.path = selected->second / request.relative_path;
    return report;
}

} // namespace linuxdesktop::migration
