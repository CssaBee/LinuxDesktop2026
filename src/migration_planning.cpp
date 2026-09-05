#include "migration_internal.hpp"

namespace linuxdesktop::migration {

migration_plan plan_migration(std::vector<migration_action> actions, const options& options)
{
    migration_plan planned;
    planned.actions = std::move(actions);
    planned.dry_run = true;

    if (!options.dry_run) {
        planned.diagnostics.push_back(internal::make_diagnostic(
            severity::warning,
            "migration-plan-forced-dry-run",
            "Migration plans are always created as dry-run objects; call execute_migration_plan to apply them"));
    }

    for (const auto& action : planned.actions) {
        internal::append_action_gate_diagnostics(action, options, planned.diagnostics);
        internal::append_file_action_diagnostics(action, options, planned.diagnostics);
    }

    return planned;
}

} // namespace linuxdesktop::migration
