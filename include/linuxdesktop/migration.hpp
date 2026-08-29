#pragma once

#include "linuxdesktop/core.hpp"
#include "linuxdesktop/paths.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace linuxdesktop::migration {

inline constexpr int version_major = 0;
inline constexpr int version_minor = 1;
inline constexpr int version_patch = 0;

using ::linuxdesktop::diagnostic;
using ::linuxdesktop::severity;
using ::linuxdesktop::to_string;

enum class migration_action_kind {
    copy_file,
    move_file,
    copy_directory,
    move_directory,
    import_registry,
    export_registry,
    write_registry_value,
    delete_registry_key
};

enum class migration_action_state {
    planned,
    executed,
    skipped,
    blocked,
    unsupported,
    partially_executed,
    rollback_missing,
    rollback_failed
};

struct migration_action {
    migration_action_kind kind = migration_action_kind::copy_file;
    std::string name;
    std::filesystem::path source_path;
    std::filesystem::path target_path;
    bool dangerous = false;
    bool requires_elevation = false;
};

struct options {
    bool dry_run = true;
    bool allow_dangerous = false;
    bool allow_elevation = false;
    bool create_parent_directories = true;
    bool overwrite_existing = false;
};

struct migration_plan {
    std::vector<migration_action> actions;
    bool dry_run = true;
    std::vector<diagnostic> diagnostics;
};

migration_plan plan_migration(std::vector<migration_action> actions, const options& options = {});

namespace detail {

inline migration_plan plan_path_inference(
    std::filesystem::path source_path,
    std::filesystem::path target_path,
    const options& options,
    migration_action_kind file_kind,
    migration_action_kind directory_kind,
    bool dangerous,
    std::string_view helper_name)
{
    migration_plan plan;
    plan.dry_run = true;

    if (source_path.empty()) {
        plan.diagnostics.push_back(diagnostic{
            severity::error,
            "migration-helper-source-empty",
            std::string(helper_name) + " requires a source path",
            {}});
        return plan;
    }

    std::error_code ec;
    if (!std::filesystem::exists(source_path, ec)) {
        plan.diagnostics.push_back(diagnostic{
            severity::error,
            "migration-source-missing",
            "Migration source does not exist",
            source_path});
        return plan;
    }

    const bool directory = std::filesystem::is_directory(source_path, ec);
    const bool regular_file = std::filesystem::is_regular_file(source_path, ec);
    if (directory == regular_file) {
        plan.diagnostics.push_back(diagnostic{
            severity::error,
            "migration-source-kind-ambiguous",
            "Migration source kind is ambiguous",
            source_path});
        return plan;
    }

    migration_action action;
    action.kind = directory ? directory_kind : file_kind;
    action.source_path = std::move(source_path);
    action.target_path = std::move(target_path);
    action.dangerous = dangerous;
    return plan_migration({std::move(action)}, options);
}

} // namespace detail

struct migration_action_result {
    migration_action action;
    migration_action_state state = migration_action_state::planned;
    bool planned = false;
    bool executed = false;
    bool skipped = false;
    bool source_existed_before = false;
    bool target_existed_before = false;
    bool source_exists_after = false;
    bool target_exists_after = false;
    bool rollback_available = false;
    bool rollback_attempted = false;
    bool rollback_succeeded = false;
    std::vector<diagnostic> diagnostics;
};

struct rooted_path_request {
    linuxdesktop::paths::app_identity identity;
    linuxdesktop::paths::resolver_options resolver_options;
    linuxdesktop::paths::path_family family = linuxdesktop::paths::path_family::config;
    std::filesystem::path relative_path;
};

struct rooted_path_report {
    std::filesystem::path path;
    std::vector<diagnostic> diagnostics;
};

struct migration_execution_report {
    bool ok = false;
    bool dry_run = true;
    std::vector<migration_action_result> actions;
    std::vector<diagnostic> diagnostics;
};

std::string_view to_string(migration_action_kind value);
std::string_view to_string(migration_action_state value);

inline migration_plan plan_copy_file(
    std::filesystem::path source_path,
    std::filesystem::path target_path,
    const options& options = {})
{
    migration_action action;
    action.kind = migration_action_kind::copy_file;
    action.source_path = std::move(source_path);
    action.target_path = std::move(target_path);
    return plan_migration({std::move(action)}, options);
}

inline migration_plan plan_copy_directory(
    std::filesystem::path source_path,
    std::filesystem::path target_path,
    const options& options = {})
{
    migration_action action;
    action.kind = migration_action_kind::copy_directory;
    action.source_path = std::move(source_path);
    action.target_path = std::move(target_path);
    return plan_migration({std::move(action)}, options);
}

inline migration_plan plan_copy(
    std::filesystem::path source_path,
    std::filesystem::path target_path,
    const options& options = {})
{
    return detail::plan_path_inference(
        std::move(source_path),
        std::move(target_path),
        options,
        migration_action_kind::copy_file,
        migration_action_kind::copy_directory,
        false,
        "plan_copy");
}

inline migration_plan copy_path(
    std::filesystem::path source_path,
    std::filesystem::path target_path,
    const options& options = {})
{
    return plan_copy(std::move(source_path), std::move(target_path), options);
}

inline migration_plan plan_move_file(
    std::filesystem::path source_path,
    std::filesystem::path target_path,
    const options& options = {})
{
    migration_action action;
    action.kind = migration_action_kind::move_file;
    action.source_path = std::move(source_path);
    action.target_path = std::move(target_path);
    action.dangerous = true;
    return plan_migration({std::move(action)}, options);
}

inline migration_plan plan_move_directory(
    std::filesystem::path source_path,
    std::filesystem::path target_path,
    const options& options = {})
{
    migration_action action;
    action.kind = migration_action_kind::move_directory;
    action.source_path = std::move(source_path);
    action.target_path = std::move(target_path);
    action.dangerous = true;
    return plan_migration({std::move(action)}, options);
}

inline migration_plan plan_move(
    std::filesystem::path source_path,
    std::filesystem::path target_path,
    const options& options = {})
{
    return detail::plan_path_inference(
        std::move(source_path),
        std::move(target_path),
        options,
        migration_action_kind::move_file,
        migration_action_kind::move_directory,
        true,
        "plan_move");
}

inline migration_plan move_path(
    std::filesystem::path source_path,
    std::filesystem::path target_path,
    const options& options = {})
{
    return plan_move(std::move(source_path), std::move(target_path), options);
}

inline migration_plan plan_delete_registry_key(std::string name = {}, const options& options = {})
{
    migration_action action;
    action.kind = migration_action_kind::delete_registry_key;
    action.name = std::move(name);
    action.dangerous = true;
    return plan_migration({std::move(action)}, options);
}

migration_execution_report execute_migration_plan(const migration_plan& plan, const options& options = {});

rooted_path_report resolve_rooted_path(const rooted_path_request& request);

// App-settings Registry snapshot/import/export compatibility data. This is not
// a general Registry abstraction; it exists to move application state.
namespace registry {

enum class hive {
    current_user,
    local_machine,
    classes_root,
    users,
    current_config
};

enum class view {
    native,
    registry_32,
    registry_64
};

enum class value_type {
    none,
    string,
    expandable_string,
    multi_string,
    dword,
    qword,
    binary,
    unknown
};

struct key {
    hive root = hive::current_user;
    std::string subkey;
    view registry_view = view::native;
};

struct value {
    std::string name;
    value_type type = value_type::none;
    std::vector<std::byte> bytes;
};

struct options {
    bool allow_hklm_write = false;
    bool allow_policy_write = false;
    bool allow_recursive_delete = false;
    bool allow_import = false;
    bool dry_run = true;
};

struct operation_report {
    bool ok = false;
    bool dry_run = false;
    std::vector<diagnostic> diagnostics;
};

struct value_report {
    bool ok = false;
    std::optional<value> item;
    std::vector<diagnostic> diagnostics;
};

struct values_report {
    bool ok = false;
    std::vector<value> values;
    std::vector<diagnostic> diagnostics;
};

struct subkeys_report {
    bool ok = false;
    std::vector<std::string> names;
    std::vector<diagnostic> diagnostics;
};

struct snapshot_value {
    std::string key_path;
    value item;
};

struct snapshot {
    key root;
    std::vector<snapshot_value> values;
};

struct snapshot_report {
    bool ok = false;
    std::optional<snapshot> item;
    std::vector<diagnostic> diagnostics;
};

struct format_report {
    bool ok = false;
    std::string content;
    std::vector<diagnostic> diagnostics;
};

std::string_view to_string(hive value);
std::string_view to_string(view value);
std::string_view to_string(value_type value);

value_report read_value(const key& key, const std::string& name);
operation_report write_value(const key& key, const value& value, const options& options = {});
operation_report delete_value(const key& key, const std::string& name, const options& options = {});
operation_report delete_key(const key& key, const options& options = {});
values_report enumerate_values(const key& key);
subkeys_report enumerate_subkeys(const key& key);
format_report serialize_snapshot_json(const snapshot& snapshot);
snapshot_report parse_snapshot_json(std::string_view content);
format_report serialize_snapshot_reg(const snapshot& snapshot);
snapshot_report parse_snapshot_reg(std::string_view content);
format_report export_tree_json(const key& key);
operation_report import_tree_json(const key& key, std::string_view content, const options& options = {});
format_report export_tree_reg(const key& key);
operation_report import_tree_reg(const key& key, std::string_view content, const options& options = {});

} // namespace registry

} // namespace linuxdesktop::migration
