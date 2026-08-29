#include "linuxdesktop/migration.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace linuxdesktop::migration {
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

void append_action_gate_diagnostics(
    const migration_action& action,
    const options& options,
    std::vector<diagnostic>& diagnostics)
{
    if (action.name.empty()) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "migration-action-name-empty",
            "Migration action requires a non-empty name"));
    }
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

bool is_file_action(migration_action_kind kind)
{
    return kind == migration_action_kind::copy_file ||
        kind == migration_action_kind::move_file ||
        kind == migration_action_kind::copy_directory ||
        kind == migration_action_kind::move_directory;
}

bool is_directory_action(migration_action_kind kind)
{
    return kind == migration_action_kind::copy_directory ||
        kind == migration_action_kind::move_directory;
}

bool is_move_action(migration_action_kind kind)
{
    return kind == migration_action_kind::move_file ||
        kind == migration_action_kind::move_directory;
}

bool action_requires_dangerous_permission(migration_action_kind kind)
{
    return kind == migration_action_kind::move_file ||
        kind == migration_action_kind::move_directory ||
        kind == migration_action_kind::delete_registry_key;
}

bool path_exists_noerror(const std::filesystem::path& path)
{
    if (path.empty()) {
        return false;
    }
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

void record_after_paths(migration_action_result& result)
{
    result.source_exists_after = path_exists_noerror(result.action.source_path);
    result.target_exists_after = path_exists_noerror(result.action.target_path);
}

void append_file_action_diagnostics(
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
    if (!action.target_path.empty() && std::filesystem::exists(action.target_path, ec) && !options.overwrite_existing) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "migration-target-exists",
            "Migration target exists and overwrite_existing is false",
            action.target_path));
    }
}

#if defined(_WIN32)
std::wstring widen_utf8(const std::string& value)
{
    if (value.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (size <= 0) {
        return std::filesystem::path(value).wstring();
    }
    std::wstring result(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size);
    return result;
}

std::string narrow_utf8(const std::wstring& value)
{
    if (value.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return {};
    }
    std::string result(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size, nullptr, nullptr);
    return result;
}

HKEY native_hive(registry::hive value)
{
    switch (value) {
    case registry::hive::current_user:
        return HKEY_CURRENT_USER;
    case registry::hive::local_machine:
        return HKEY_LOCAL_MACHINE;
    case registry::hive::classes_root:
        return HKEY_CLASSES_ROOT;
    case registry::hive::users:
        return HKEY_USERS;
    case registry::hive::current_config:
        return HKEY_CURRENT_CONFIG;
    }
    return HKEY_CURRENT_USER;
}

REGSAM registry_view_flags(registry::view value)
{
    switch (value) {
    case registry::view::registry_32:
        return KEY_WOW64_32KEY;
    case registry::view::registry_64:
        return KEY_WOW64_64KEY;
    case registry::view::native:
        return 0;
    }
    return 0;
}

DWORD registry_type_to_native(registry::value_type value)
{
    switch (value) {
    case registry::value_type::none:
        return REG_NONE;
    case registry::value_type::string:
        return REG_SZ;
    case registry::value_type::expandable_string:
        return REG_EXPAND_SZ;
    case registry::value_type::multi_string:
        return REG_MULTI_SZ;
    case registry::value_type::dword:
        return REG_DWORD;
    case registry::value_type::qword:
        return REG_QWORD;
    case registry::value_type::binary:
        return REG_BINARY;
    case registry::value_type::unknown:
        return REG_NONE;
    }
    return REG_NONE;
}

registry::value_type registry_type_from_native(DWORD value)
{
    switch (value) {
    case REG_NONE:
        return registry::value_type::none;
    case REG_SZ:
        return registry::value_type::string;
    case REG_EXPAND_SZ:
        return registry::value_type::expandable_string;
    case REG_MULTI_SZ:
        return registry::value_type::multi_string;
    case REG_DWORD:
        return registry::value_type::dword;
    case REG_QWORD:
        return registry::value_type::qword;
    case REG_BINARY:
        return registry::value_type::binary;
    default:
        return registry::value_type::unknown;
    }
}

diagnostic win32_diagnostic(std::string code, std::string message, LSTATUS status)
{
    return make_diagnostic(
        severity::error,
        std::move(code),
        std::move(message) + ": Win32 error " + std::to_string(status));
}

bool is_policy_key(const registry::key& key)
{
    return key.subkey.find("Software\\Policies") == 0 || key.subkey.find("Software/Policies") == 0;
}

bool registry_write_allowed(
    const registry::key& key,
    const registry::options& options,
    bool recursive_delete,
    std::vector<diagnostic>& diagnostics)
{
    if (key.root == registry::hive::local_machine && !options.allow_hklm_write) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "registry-hklm-write-denied",
            "HKLM writes require allow_hklm_write"));
    }
    if (is_policy_key(key) && !options.allow_policy_write) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "registry-policy-write-denied",
            "Policy key writes require allow_policy_write"));
    }
    if (recursive_delete && !options.allow_recursive_delete) {
        diagnostics.push_back(make_diagnostic(
            severity::error,
            "registry-recursive-delete-denied",
            "Recursive Registry delete requires allow_recursive_delete"));
    }
    return !has_error(diagnostics);
}
#endif

} // namespace

std::string_view to_string(migration_action_kind value)
{
    switch (value) {
    case migration_action_kind::copy_file:
        return "copy_file";
    case migration_action_kind::move_file:
        return "move_file";
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

migration_plan plan_migration(std::vector<migration_action> actions, const options& options)
{
    migration_plan planned;
    planned.actions = std::move(actions);
    planned.dry_run = true;

    if (!options.dry_run) {
        planned.diagnostics.push_back(make_diagnostic(
            severity::warning,
            "migration-plan-forced-dry-run",
            "Migration plans are always created as dry-run objects; call execute_migration_plan to apply them"));
    }

    for (const auto& action : planned.actions) {
        append_action_gate_diagnostics(action, options, planned.diagnostics);
        append_file_action_diagnostics(action, options, planned.diagnostics);
    }

    return planned;
}

migration_execution_report execute_migration_plan(const migration_plan& plan, const options& options)
{
    migration_execution_report report;
    report.dry_run = options.dry_run;

    if (has_error(plan.diagnostics)) {
        report.diagnostics.push_back(make_diagnostic(
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
            result.source_existed_before = path_exists_noerror(action.source_path);
            result.target_existed_before = path_exists_noerror(action.target_path);
            append_action_gate_diagnostics(action, options, result.diagnostics);
            append_file_action_diagnostics(action, options, result.diagnostics);
            record_after_paths(result);
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
        result.source_existed_before = path_exists_noerror(action.source_path);
        result.target_existed_before = path_exists_noerror(action.target_path);
        append_action_gate_diagnostics(action, options, result.diagnostics);
        append_file_action_diagnostics(action, options, result.diagnostics);

        if (has_error(result.diagnostics)) {
            result.state = migration_action_state::blocked;
            result.skipped = true;
            report.ok = false;
            record_after_paths(result);
            report.actions.push_back(std::move(result));
            continue;
        }

        if (options.dry_run) {
            result.state = migration_action_state::skipped;
            result.skipped = true;
            result.diagnostics.push_back(make_diagnostic(
                severity::info,
                "migration-dry-run",
                "Migration action was planned but not executed because dry_run is true"));
            record_after_paths(result);
            report.actions.push_back(std::move(result));
            continue;
        }

        if (!is_file_action(action.kind)) {
            result.state = migration_action_state::unsupported;
            result.skipped = true;
            report.ok = false;
            result.diagnostics.push_back(make_diagnostic(
                severity::error,
                "migration-action-not-executable-yet",
                "This migration action kind does not have an executor yet"));
            record_after_paths(result);
            report.actions.push_back(std::move(result));
            continue;
        }

        if (!options.allow_dangerous && action_requires_dangerous_permission(action.kind)) {
            result.state = migration_action_state::blocked;
            result.skipped = true;
            report.ok = false;
            result.diagnostics.push_back(make_diagnostic(
                severity::error,
                "migration-dangerous-action-denied",
                "Destructive migration action requires allow_dangerous"));
            record_after_paths(result);
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
                result.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "migration-create-parent-failed",
                    ec.message(),
                    action.target_path.parent_path()));
                record_after_paths(result);
                report.actions.push_back(std::move(result));
                continue;
            }
        }

        const auto copy_options = options.overwrite_existing
            ? std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing
            : std::filesystem::copy_options::recursive;

        if (is_directory_action(action.kind) || action.kind == migration_action_kind::copy_file) {
            if (is_directory_action(action.kind)) {
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
                result.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "migration-copy-failed",
                    ec.message(),
                    action.target_path));
                record_after_paths(result);
                report.actions.push_back(std::move(result));
                continue;
            }
        }

        if (is_move_action(action.kind)) {
            if (!is_directory_action(action.kind)) {
                std::filesystem::rename(action.source_path, action.target_path, ec);
            } else {
                std::filesystem::remove_all(action.source_path, ec);
            }
            if (ec) {
                report.ok = false;
                result.rollback_available = path_exists_noerror(action.target_path) && !result.target_existed_before;
                result.state = result.rollback_available
                    ? migration_action_state::partially_executed
                    : migration_action_state::rollback_missing;
                if (result.rollback_available) {
                    result.rollback_attempted = true;
                    std::error_code rollback_ec;
                    std::filesystem::remove_all(action.target_path, rollback_ec);
                    result.rollback_succeeded = !rollback_ec;
                    if (!result.rollback_succeeded) {
                        result.state = migration_action_state::rollback_failed;
                    }
                    result.diagnostics.push_back(make_diagnostic(
                        result.rollback_succeeded ? severity::warning : severity::error,
                        result.rollback_succeeded ? "migration-rollback-succeeded" : "migration-rollback-failed",
                        result.rollback_succeeded ? "Removed copied target after move cleanup failed" : rollback_ec.message(),
                        action.target_path));
                }
                result.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "migration-move-cleanup-failed",
                    ec.message(),
                    action.source_path));
                record_after_paths(result);
                report.actions.push_back(std::move(result));
                continue;
            }
        }

        result.state = migration_action_state::executed;
        result.executed = true;
        record_after_paths(result);
        report.actions.push_back(std::move(result));
    }

    return report;
}

rooted_path_report resolve_rooted_path(const rooted_path_request& request)
{
    rooted_path_report report;
    if (!request.relative_path.empty() && request.relative_path.is_absolute()) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "migration-rooted-path-relative-required",
            "Rooted migration paths require a relative path",
            request.relative_path));
        return report;
    }

    const auto paths = ld_paths::resolve_app_paths(request.identity, request.resolver_options);
    report.diagnostics.insert(report.diagnostics.end(), paths.diagnostics.begin(), paths.diagnostics.end());
    const auto selected = paths.selected.find(request.family);
    if (selected == paths.selected.end() || selected->second.empty()) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "migration-rooted-path-family-missing",
            "Could not resolve requested migration path family"));
        return report;
    }

    report.path = selected->second / request.relative_path;
    return report;
}

namespace registry {
namespace {

std::string json_escape(std::string_view value)
{
    std::string output;
    for (const char ch : value) {
        switch (ch) {
        case '\\':
            output += "\\\\";
            break;
        case '"':
            output += "\\\"";
            break;
        case '\n':
            output += "\\n";
            break;
        case '\r':
            output += "\\r";
            break;
        case '\t':
            output += "\\t";
            break;
        default:
            output += ch;
            break;
        }
    }
    return output;
}

std::string json_unescape(std::string_view value)
{
    std::string output;
    for (size_t index = 0; index != value.size(); ++index) {
        const char ch = value[index];
        if (ch != '\\' || index + 1 == value.size()) {
            output += ch;
            continue;
        }
        const char escaped = value[++index];
        switch (escaped) {
        case 'n':
            output += '\n';
            break;
        case 'r':
            output += '\r';
            break;
        case 't':
            output += '\t';
            break;
        default:
            output += escaped;
            break;
        }
    }
    return output;
}

char hex_digit(unsigned value)
{
    return static_cast<char>(value < 10 ? '0' + value : 'a' + (value - 10));
}

std::string bytes_to_hex(const std::vector<std::byte>& bytes)
{
    std::string output;
    output.reserve(bytes.size() * 2);
    for (const auto byte : bytes) {
        const auto value = static_cast<unsigned>(byte);
        output.push_back(hex_digit((value >> 4) & 0x0f));
        output.push_back(hex_digit(value & 0x0f));
    }
    return output;
}

std::optional<unsigned> parse_hex_digit(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return static_cast<unsigned>(ch - '0');
    }
    if (ch >= 'a' && ch <= 'f') {
        return static_cast<unsigned>(ch - 'a' + 10);
    }
    if (ch >= 'A' && ch <= 'F') {
        return static_cast<unsigned>(ch - 'A' + 10);
    }
    return std::nullopt;
}

std::optional<std::vector<std::byte>> hex_to_bytes(std::string_view hex)
{
    std::string compact;
    for (const char ch : hex) {
        if (ch == ',' || std::isspace(static_cast<unsigned char>(ch))) {
            continue;
        }
        compact.push_back(ch);
    }
    if (compact.size() % 2 != 0) {
        return std::nullopt;
    }

    std::vector<std::byte> output;
    output.reserve(compact.size() / 2);
    for (size_t index = 0; index != compact.size(); index += 2) {
        const auto high = parse_hex_digit(compact[index]);
        const auto low = parse_hex_digit(compact[index + 1]);
        if (!high || !low) {
            return std::nullopt;
        }
        output.push_back(static_cast<std::byte>((*high << 4) | *low));
    }
    return output;
}

std::string trim(std::string_view value)
{
    size_t begin = 0;
    while (begin != value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) {
        ++begin;
    }
    size_t end = value.size();
    while (end != begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return std::string(value.substr(begin, end - begin));
}

std::optional<std::string> json_string_field(std::string_view object, std::string_view field)
{
    const auto needle = "\"" + std::string(field) + "\"";
    auto pos = object.find(needle);
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }
    pos = object.find(':', pos + needle.size());
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }
    pos = object.find('"', pos + 1);
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }
    ++pos;

    std::string raw;
    bool escaped = false;
    for (; pos != object.size(); ++pos) {
        const char ch = object[pos];
        if (escaped) {
            raw.push_back('\\');
            raw.push_back(ch);
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == '"') {
            return json_unescape(raw);
        }
        raw.push_back(ch);
    }
    return std::nullopt;
}

std::optional<std::string_view> json_object_field(std::string_view object, std::string_view field)
{
    const auto needle = "\"" + std::string(field) + "\"";
    auto pos = object.find(needle);
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }
    pos = object.find('{', pos + needle.size());
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }

    size_t depth = 0;
    bool in_string = false;
    bool escaped = false;
    for (size_t index = pos; index != object.size(); ++index) {
        const char ch = object[index];
        if (escaped) {
            escaped = false;
            continue;
        }
        if (in_string && ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == '"') {
            in_string = !in_string;
            continue;
        }
        if (in_string) {
            continue;
        }
        if (ch == '{') {
            ++depth;
        } else if (ch == '}') {
            --depth;
            if (depth == 0) {
                return object.substr(pos, index - pos + 1);
            }
        }
    }
    return std::nullopt;
}

std::vector<std::string_view> json_array_objects(std::string_view object, std::string_view field)
{
    std::vector<std::string_view> objects;
    const auto needle = "\"" + std::string(field) + "\"";
    auto pos = object.find(needle);
    if (pos == std::string_view::npos) {
        return objects;
    }
    pos = object.find('[', pos + needle.size());
    if (pos == std::string_view::npos) {
        return objects;
    }

    size_t depth = 0;
    size_t object_begin = std::string_view::npos;
    bool in_string = false;
    bool escaped = false;
    for (size_t index = pos + 1; index != object.size(); ++index) {
        const char ch = object[index];
        if (escaped) {
            escaped = false;
            continue;
        }
        if (in_string && ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == '"') {
            in_string = !in_string;
            continue;
        }
        if (in_string) {
            continue;
        }
        if (ch == '{') {
            if (depth == 0) {
                object_begin = index;
            }
            ++depth;
        } else if (ch == '}') {
            --depth;
            if (depth == 0 && object_begin != std::string_view::npos) {
                objects.push_back(object.substr(object_begin, index - object_begin + 1));
                object_begin = std::string_view::npos;
            }
        } else if (ch == ']' && depth == 0) {
            break;
        }
    }
    return objects;
}

std::optional<hive> hive_from_string(std::string_view value)
{
    if (value == "current_user") {
        return hive::current_user;
    }
    if (value == "local_machine") {
        return hive::local_machine;
    }
    if (value == "classes_root") {
        return hive::classes_root;
    }
    if (value == "users") {
        return hive::users;
    }
    if (value == "current_config") {
        return hive::current_config;
    }
    return std::nullopt;
}

std::optional<view> view_from_string(std::string_view value)
{
    if (value == "native") {
        return view::native;
    }
    if (value == "registry_32") {
        return view::registry_32;
    }
    if (value == "registry_64") {
        return view::registry_64;
    }
    return std::nullopt;
}

std::optional<value_type> value_type_from_string(std::string_view value)
{
    if (value == "none") {
        return value_type::none;
    }
    if (value == "string") {
        return value_type::string;
    }
    if (value == "expandable_string") {
        return value_type::expandable_string;
    }
    if (value == "multi_string") {
        return value_type::multi_string;
    }
    if (value == "dword") {
        return value_type::dword;
    }
    if (value == "qword") {
        return value_type::qword;
    }
    if (value == "binary") {
        return value_type::binary;
    }
    if (value == "unknown") {
        return value_type::unknown;
    }
    return std::nullopt;
}

std::string reg_hive_name(hive value)
{
    switch (value) {
    case hive::current_user:
        return "HKEY_CURRENT_USER";
    case hive::local_machine:
        return "HKEY_LOCAL_MACHINE";
    case hive::classes_root:
        return "HKEY_CLASSES_ROOT";
    case hive::users:
        return "HKEY_USERS";
    case hive::current_config:
        return "HKEY_CURRENT_CONFIG";
    }
    return "HKEY_CURRENT_USER";
}

std::optional<hive> reg_hive_from_name(std::string_view value)
{
    if (value == "HKEY_CURRENT_USER" || value == "HKCU") {
        return hive::current_user;
    }
    if (value == "HKEY_LOCAL_MACHINE" || value == "HKLM") {
        return hive::local_machine;
    }
    if (value == "HKEY_CLASSES_ROOT" || value == "HKCR") {
        return hive::classes_root;
    }
    if (value == "HKEY_USERS" || value == "HKU") {
        return hive::users;
    }
    if (value == "HKEY_CURRENT_CONFIG" || value == "HKCC") {
        return hive::current_config;
    }
    return std::nullopt;
}

std::string reg_escape(std::string_view value)
{
    std::string output;
    for (const char ch : value) {
        if (ch == '\\' || ch == '"') {
            output.push_back('\\');
        }
        output.push_back(ch);
    }
    return output;
}

std::string reg_unescape(std::string_view value)
{
    std::string output;
    bool escaped = false;
    for (const char ch : value) {
        if (escaped) {
            output.push_back(ch);
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        output.push_back(ch);
    }
    return output;
}

std::string bytes_to_reg_hex(const std::vector<std::byte>& bytes)
{
    std::string output;
    for (size_t index = 0; index != bytes.size(); ++index) {
        if (index != 0) {
            output.push_back(',');
        }
        const auto value = static_cast<unsigned>(bytes[index]);
        output.push_back(hex_digit((value >> 4) & 0x0f));
        output.push_back(hex_digit(value & 0x0f));
    }
    return output;
}

std::string bytes_to_lossy_string(const std::vector<std::byte>& bytes)
{
    std::string output;
    output.reserve(bytes.size());
    for (const auto byte : bytes) {
        const auto ch = static_cast<char>(byte);
        if (ch == '\0') {
            break;
        }
        output.push_back(ch);
    }
    return output;
}

std::vector<std::byte> string_to_bytes(std::string_view value)
{
    std::vector<std::byte> bytes;
    bytes.reserve(value.size());
    for (const char ch : value) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
}

std::string dword_bytes_to_reg_text(const std::vector<std::byte>& bytes)
{
    unsigned value = 0;
    for (size_t index = 0; index != bytes.size() && index != 4; ++index) {
        value |= static_cast<unsigned>(bytes[index]) << (index * 8);
    }

    std::string output(8, '0');
    for (size_t index = 0; index != output.size(); ++index) {
        const auto shift = static_cast<unsigned>((output.size() - index - 1) * 4);
        output[index] = hex_digit((value >> shift) & 0x0f);
    }
    return output;
}

std::optional<std::vector<std::byte>> dword_reg_text_to_bytes(std::string_view text)
{
    if (text.size() != 8) {
        return std::nullopt;
    }

    unsigned value = 0;
    for (const char ch : text) {
        const auto digit = parse_hex_digit(ch);
        if (!digit) {
            return std::nullopt;
        }
        value = (value << 4) | *digit;
    }

    return std::vector<std::byte>{
        static_cast<std::byte>(value & 0xff),
        static_cast<std::byte>((value >> 8) & 0xff),
        static_cast<std::byte>((value >> 16) & 0xff),
        static_cast<std::byte>((value >> 24) & 0xff),
    };
}

std::string combine_key_path(const key& root, const std::string& relative)
{
    if (relative.empty()) {
        return root.subkey;
    }
    if (root.subkey.empty()) {
        return relative;
    }
    return root.subkey + "\\" + relative;
}

} // namespace

std::string_view to_string(hive value)
{
    switch (value) {
    case hive::current_user:
        return "current_user";
    case hive::local_machine:
        return "local_machine";
    case hive::classes_root:
        return "classes_root";
    case hive::users:
        return "users";
    case hive::current_config:
        return "current_config";
    }
    return "unknown";
}

std::string_view to_string(view value)
{
    switch (value) {
    case view::native:
        return "native";
    case view::registry_32:
        return "registry_32";
    case view::registry_64:
        return "registry_64";
    }
    return "unknown";
}

std::string_view to_string(value_type value)
{
    switch (value) {
    case value_type::none:
        return "none";
    case value_type::string:
        return "string";
    case value_type::expandable_string:
        return "expandable_string";
    case value_type::multi_string:
        return "multi_string";
    case value_type::dword:
        return "dword";
    case value_type::qword:
        return "qword";
    case value_type::binary:
        return "binary";
    case value_type::unknown:
        return "unknown";
    }
    return "unknown";
}

format_report serialize_snapshot_json(const snapshot& snapshot)
{
    format_report report;
    std::ostringstream output;
    output << "{\n";
    output << "  \"format\": \"linuxdesktop.settings.registry.snapshot.v1\",\n";
    output << "  \"root\": {\n";
    output << "    \"hive\": \"" << to_string(snapshot.root.root) << "\",\n";
    output << "    \"subkey\": \"" << json_escape(snapshot.root.subkey) << "\",\n";
    output << "    \"view\": \"" << to_string(snapshot.root.registry_view) << "\"\n";
    output << "  },\n";
    output << "  \"values\": [\n";
    for (size_t index = 0; index != snapshot.values.size(); ++index) {
        const auto& item = snapshot.values[index];
        output << "    {\n";
        output << "      \"key_path\": \"" << json_escape(item.key_path) << "\",\n";
        output << "      \"name\": \"" << json_escape(item.item.name) << "\",\n";
        output << "      \"type\": \"" << to_string(item.item.type) << "\",\n";
        output << "      \"data_hex\": \"" << bytes_to_hex(item.item.bytes) << "\"\n";
        output << "    }";
        if (index + 1 != snapshot.values.size()) {
            output << ",";
        }
        output << "\n";
    }
    output << "  ]\n";
    output << "}\n";
    report.content = output.str();
    report.ok = true;
    return report;
}

snapshot_report parse_snapshot_json(std::string_view content)
{
    snapshot_report report;
    const auto root_object = json_object_field(content, "root");
    if (!root_object) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "registry-json-root-missing",
            "Registry JSON snapshot is missing the root object"));
        return report;
    }

    const auto hive_text = json_string_field(*root_object, "hive");
    const auto subkey = json_string_field(*root_object, "subkey");
    const auto view_text = json_string_field(*root_object, "view");
    if (!hive_text || !subkey || !view_text) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "registry-json-root-invalid",
            "Registry JSON snapshot root requires hive, subkey, and view"));
        return report;
    }

    const auto parsed_hive = hive_from_string(*hive_text);
    const auto parsed_view = view_from_string(*view_text);
    if (!parsed_hive || !parsed_view) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "registry-json-root-invalid",
            "Registry JSON snapshot root has an unknown hive or view"));
        return report;
    }

    snapshot parsed;
    parsed.root.root = *parsed_hive;
    parsed.root.subkey = *subkey;
    parsed.root.registry_view = *parsed_view;

    for (const auto value_object : json_array_objects(content, "values")) {
        const auto key_path = json_string_field(value_object, "key_path");
        const auto name = json_string_field(value_object, "name");
        const auto type_text = json_string_field(value_object, "type");
        const auto data_hex = json_string_field(value_object, "data_hex");
        if (!key_path || !name || !type_text || !data_hex) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "registry-json-value-invalid",
                "Registry JSON value requires key_path, name, type, and data_hex"));
            return report;
        }

        const auto parsed_type = value_type_from_string(*type_text);
        const auto bytes = hex_to_bytes(*data_hex);
        if (!parsed_type || !bytes) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "registry-json-value-invalid",
                "Registry JSON value has an unknown type or invalid hex data"));
            return report;
        }

        snapshot_value value;
        value.key_path = *key_path;
        value.item.name = *name;
        value.item.type = *parsed_type;
        value.item.bytes = *bytes;
        parsed.values.push_back(std::move(value));
    }

    report.ok = true;
    report.item = std::move(parsed);
    return report;
}

format_report serialize_snapshot_reg(const snapshot& snapshot)
{
    format_report report;
    std::ostringstream output;
    output << "Windows Registry Editor Version 5.00\n\n";

    std::string current_key;
    for (const auto& item : snapshot.values) {
        const auto full_key = reg_hive_name(snapshot.root.root) + "\\" + combine_key_path(snapshot.root, item.key_path);
        if (full_key != current_key) {
            if (!current_key.empty()) {
                output << "\n";
            }
            current_key = full_key;
            output << "[" << current_key << "]\n";
        }

        if (item.item.name.empty()) {
            output << "@=";
        } else {
            output << "\"" << reg_escape(item.item.name) << "\"=";
        }

        switch (item.item.type) {
        case value_type::string:
            output << "\"" << reg_escape(bytes_to_lossy_string(item.item.bytes)) << "\"";
            break;
        case value_type::expandable_string:
            output << "hex(2):" << bytes_to_reg_hex(item.item.bytes);
            break;
        case value_type::multi_string:
            output << "hex(7):" << bytes_to_reg_hex(item.item.bytes);
            break;
        case value_type::dword:
            output << "dword:" << dword_bytes_to_reg_text(item.item.bytes);
            break;
        case value_type::qword:
            output << "hex(b):" << bytes_to_reg_hex(item.item.bytes);
            break;
        case value_type::binary:
        case value_type::none:
        case value_type::unknown:
            output << "hex:" << bytes_to_reg_hex(item.item.bytes);
            break;
        }
        output << "\n";
    }

    report.content = output.str();
    report.ok = true;
    return report;
}

snapshot_report parse_snapshot_reg(std::string_view content)
{
    snapshot_report report;
    snapshot parsed;
    bool have_root = false;
    std::string current_relative_key;

    std::istringstream input{std::string(content)};
    std::string line;
    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty() || line[0] == ';') {
            continue;
        }
        if (line == "Windows Registry Editor Version 5.00" || line == "REGEDIT4") {
            continue;
        }
        if (line.front() == '[' && line.back() == ']') {
            const auto full_key = line.substr(1, line.size() - 2);
            const auto slash = full_key.find('\\');
            const auto hive_text = slash == std::string::npos ? full_key : full_key.substr(0, slash);
            const auto hive = reg_hive_from_name(hive_text);
            if (!hive) {
                report.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "registry-reg-hive-invalid",
                    "Registry file contains an unknown hive"));
                return report;
            }

            const auto subkey = slash == std::string::npos ? std::string{} : full_key.substr(slash + 1);
            if (!have_root) {
                parsed.root.root = *hive;
                parsed.root.subkey = subkey;
                parsed.root.registry_view = view::native;
                have_root = true;
                current_relative_key.clear();
            } else if (*hive != parsed.root.root) {
                report.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "registry-reg-multiple-hives",
                    "Registry file snapshots must contain one hive"));
                return report;
            } else if (subkey == parsed.root.subkey) {
                current_relative_key.clear();
            } else if (subkey.find(parsed.root.subkey + "\\") == 0) {
                current_relative_key = subkey.substr(parsed.root.subkey.size() + 1);
            } else {
                report.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "registry-reg-outside-root",
                    "Registry file contains a key outside the snapshot root"));
                return report;
            }
            continue;
        }

        const auto equals = line.find('=');
        if (equals == std::string::npos || !have_root) {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "registry-reg-value-invalid",
                "Registry value line is malformed or appears before a key"));
            return report;
        }

        const auto name_text = trim(std::string_view(line).substr(0, equals));
        const auto value_text = trim(std::string_view(line).substr(equals + 1));

        snapshot_value value;
        value.key_path = current_relative_key;
        if (name_text == "@") {
            value.item.name.clear();
        } else if (name_text.size() >= 2 && name_text.front() == '"' && name_text.back() == '"') {
            value.item.name = reg_unescape(std::string_view(name_text).substr(1, name_text.size() - 2));
        } else {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "registry-reg-value-name-invalid",
                "Registry value name is malformed"));
            return report;
        }

        if (value_text.size() >= 2 && value_text.front() == '"' && value_text.back() == '"') {
            value.item.type = value_type::string;
            value.item.bytes = string_to_bytes(reg_unescape(std::string_view(value_text).substr(1, value_text.size() - 2)));
        } else if (value_text.find("dword:") == 0) {
            value.item.type = value_type::dword;
            const auto bytes = dword_reg_text_to_bytes(std::string_view(value_text).substr(6));
            if (!bytes || bytes->empty()) {
                report.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "registry-reg-value-data-invalid",
                    "Registry DWORD value contains invalid hex data"));
                return report;
            }
            value.item.bytes = *bytes;
        } else if (value_text.find("hex(b):") == 0) {
            value.item.type = value_type::qword;
            const auto bytes = hex_to_bytes(std::string_view(value_text).substr(7));
            if (!bytes) {
                report.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "registry-reg-value-data-invalid",
                    "Registry QWORD value contains invalid hex data"));
                return report;
            }
            value.item.bytes = *bytes;
        } else if (value_text.find("hex(2):") == 0) {
            value.item.type = value_type::expandable_string;
            const auto bytes = hex_to_bytes(std::string_view(value_text).substr(7));
            if (!bytes) {
                report.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "registry-reg-value-data-invalid",
                    "Registry expandable string contains invalid hex data"));
                return report;
            }
            value.item.bytes = *bytes;
        } else if (value_text.find("hex(7):") == 0) {
            value.item.type = value_type::multi_string;
            const auto bytes = hex_to_bytes(std::string_view(value_text).substr(7));
            if (!bytes) {
                report.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "registry-reg-value-data-invalid",
                    "Registry multi-string contains invalid hex data"));
                return report;
            }
            value.item.bytes = *bytes;
        } else if (value_text.find("hex:") == 0) {
            value.item.type = value_type::binary;
            const auto bytes = hex_to_bytes(std::string_view(value_text).substr(4));
            if (!bytes) {
                report.diagnostics.push_back(make_diagnostic(
                    severity::error,
                    "registry-reg-value-data-invalid",
                    "Registry binary value contains invalid hex data"));
                return report;
            }
            value.item.bytes = *bytes;
        } else {
            report.diagnostics.push_back(make_diagnostic(
                severity::error,
                "registry-reg-value-data-invalid",
                "Registry value data is not a supported .reg form"));
            return report;
        }

        parsed.values.push_back(std::move(value));
    }

    if (!have_root) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "registry-reg-root-missing",
            "Registry file does not contain a key section"));
        return report;
    }

    report.ok = true;
    report.item = std::move(parsed);
    return report;
}

#if defined(_WIN32)
value_report read_value(const key& key, const std::string& name)
{
    value_report report;
    HKEY handle = nullptr;
    const auto subkey = widen_utf8(key.subkey);
    const LSTATUS open_status = RegOpenKeyExW(
        native_hive(key.root),
        subkey.c_str(),
        0,
        KEY_QUERY_VALUE | registry_view_flags(key.registry_view),
        &handle);
    if (open_status != ERROR_SUCCESS) {
        report.diagnostics.push_back(win32_diagnostic(
            "registry-open-failed",
            "Failed to open Registry key for read",
            open_status));
        return report;
    }

    DWORD type = REG_NONE;
    DWORD size = 0;
    const auto value_name = widen_utf8(name);
    LSTATUS query_status = RegQueryValueExW(
        handle,
        value_name.c_str(),
        nullptr,
        &type,
        nullptr,
        &size);
    if (query_status != ERROR_SUCCESS) {
        RegCloseKey(handle);
        report.diagnostics.push_back(win32_diagnostic(
            "registry-query-failed",
            "Failed to query Registry value size",
            query_status));
        return report;
    }

    value item;
    item.name = name;
    item.type = registry_type_from_native(type);
    item.bytes.resize(size);
    query_status = RegQueryValueExW(
        handle,
        value_name.c_str(),
        nullptr,
        &type,
        reinterpret_cast<LPBYTE>(item.bytes.data()),
        &size);
    RegCloseKey(handle);
    if (query_status != ERROR_SUCCESS) {
        report.diagnostics.push_back(win32_diagnostic(
            "registry-query-failed",
            "Failed to query Registry value",
            query_status));
        return report;
    }
    item.bytes.resize(size);
    report.ok = true;
    report.item = std::move(item);
    return report;
}

operation_report write_value(const key& key, const value& value, const options& options)
{
    operation_report report;
    report.dry_run = options.dry_run;
    if (!registry_write_allowed(key, options, false, report.diagnostics)) {
        return report;
    }
    if (options.dry_run) {
        report.ok = true;
        report.diagnostics.push_back(make_diagnostic(
            severity::info,
            "registry-dry-run",
            "Registry value write was planned but not executed"));
        return report;
    }

    HKEY handle = nullptr;
    const auto subkey = widen_utf8(key.subkey);
    const LSTATUS create_status = RegCreateKeyExW(
        native_hive(key.root),
        subkey.c_str(),
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE | registry_view_flags(key.registry_view),
        nullptr,
        &handle,
        nullptr);
    if (create_status != ERROR_SUCCESS) {
        report.diagnostics.push_back(win32_diagnostic(
            "registry-create-failed",
            "Failed to create/open Registry key for write",
            create_status));
        return report;
    }

    const auto name = widen_utf8(value.name);
    const auto type = registry_type_to_native(value.type);
    const LSTATUS set_status = RegSetValueExW(
        handle,
        name.c_str(),
        0,
        type,
        reinterpret_cast<const BYTE*>(value.bytes.data()),
        static_cast<DWORD>(value.bytes.size()));
    RegCloseKey(handle);
    if (set_status != ERROR_SUCCESS) {
        report.diagnostics.push_back(win32_diagnostic(
            "registry-write-failed",
            "Failed to write Registry value",
            set_status));
        return report;
    }
    report.ok = true;
    return report;
}

operation_report delete_value(const key& key, const std::string& name, const options& options)
{
    operation_report report;
    report.dry_run = options.dry_run;
    if (!registry_write_allowed(key, options, false, report.diagnostics)) {
        return report;
    }
    if (options.dry_run) {
        report.ok = true;
        report.diagnostics.push_back(make_diagnostic(
            severity::info,
            "registry-dry-run",
            "Registry value delete was planned but not executed"));
        return report;
    }

    HKEY handle = nullptr;
    const auto subkey = widen_utf8(key.subkey);
    const LSTATUS open_status = RegOpenKeyExW(
        native_hive(key.root),
        subkey.c_str(),
        0,
        KEY_SET_VALUE | registry_view_flags(key.registry_view),
        &handle);
    if (open_status != ERROR_SUCCESS) {
        report.diagnostics.push_back(win32_diagnostic(
            "registry-open-failed",
            "Failed to open Registry key for value delete",
            open_status));
        return report;
    }

    const auto value_name = widen_utf8(name);
    const LSTATUS delete_status = RegDeleteValueW(handle, value_name.c_str());
    RegCloseKey(handle);
    if (delete_status != ERROR_SUCCESS) {
        report.diagnostics.push_back(win32_diagnostic(
            "registry-delete-value-failed",
            "Failed to delete Registry value",
            delete_status));
        return report;
    }
    report.ok = true;
    return report;
}

operation_report delete_key(const key& key, const options& options)
{
    operation_report report;
    report.dry_run = options.dry_run;
    if (!registry_write_allowed(key, options, false, report.diagnostics)) {
        return report;
    }
    if (options.dry_run) {
        report.ok = true;
        report.diagnostics.push_back(make_diagnostic(
            severity::info,
            "registry-dry-run",
            "Registry key delete was planned but not executed"));
        return report;
    }

    const auto subkey = widen_utf8(key.subkey);
    const LSTATUS delete_status = RegDeleteKeyExW(
        native_hive(key.root),
        subkey.c_str(),
        registry_view_flags(key.registry_view),
        0);
    if (delete_status != ERROR_SUCCESS) {
        report.diagnostics.push_back(win32_diagnostic(
            "registry-delete-key-failed",
            "Failed to delete Registry key",
            delete_status));
        return report;
    }
    report.ok = true;
    return report;
}

values_report enumerate_values(const key& key)
{
    values_report report;
    HKEY handle = nullptr;
    const auto subkey = widen_utf8(key.subkey);
    const LSTATUS open_status = RegOpenKeyExW(
        native_hive(key.root),
        subkey.c_str(),
        0,
        KEY_QUERY_VALUE | registry_view_flags(key.registry_view),
        &handle);
    if (open_status != ERROR_SUCCESS) {
        report.diagnostics.push_back(win32_diagnostic(
            "registry-open-failed",
            "Failed to open Registry key for value enumeration",
            open_status));
        return report;
    }

    DWORD value_count = 0;
    DWORD max_name_length = 0;
    DWORD max_value_size = 0;
    LSTATUS info_status = RegQueryInfoKeyW(
        handle,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        &value_count,
        &max_name_length,
        &max_value_size,
        nullptr,
        nullptr);
    if (info_status != ERROR_SUCCESS) {
        RegCloseKey(handle);
        report.diagnostics.push_back(win32_diagnostic(
            "registry-query-info-failed",
            "Failed to query Registry key metadata",
            info_status));
        return report;
    }

    for (DWORD index = 0; index != value_count; ++index) {
        std::wstring name(max_name_length + 1, L'\0');
        std::vector<std::byte> data(max_value_size);
        DWORD name_size = static_cast<DWORD>(name.size());
        DWORD data_size = static_cast<DWORD>(data.size());
        DWORD type = REG_NONE;
        const LSTATUS enum_status = RegEnumValueW(
            handle,
            index,
            name.data(),
            &name_size,
            nullptr,
            &type,
            reinterpret_cast<LPBYTE>(data.data()),
            &data_size);
        if (enum_status != ERROR_SUCCESS) {
            RegCloseKey(handle);
            report.diagnostics.push_back(win32_diagnostic(
                "registry-enum-value-failed",
                "Failed to enumerate Registry value",
                enum_status));
            return report;
        }

        name.resize(name_size);
        data.resize(data_size);
        report.values.push_back({narrow_utf8(name), registry_type_from_native(type), std::move(data)});
    }

    RegCloseKey(handle);
    report.ok = true;
    return report;
}

subkeys_report enumerate_subkeys(const key& key)
{
    subkeys_report report;
    HKEY handle = nullptr;
    const auto subkey = widen_utf8(key.subkey);
    const LSTATUS open_status = RegOpenKeyExW(
        native_hive(key.root),
        subkey.c_str(),
        0,
        KEY_ENUMERATE_SUB_KEYS | registry_view_flags(key.registry_view),
        &handle);
    if (open_status != ERROR_SUCCESS) {
        report.diagnostics.push_back(win32_diagnostic(
            "registry-open-failed",
            "Failed to open Registry key for subkey enumeration",
            open_status));
        return report;
    }

    DWORD subkey_count = 0;
    DWORD max_name_length = 0;
    LSTATUS info_status = RegQueryInfoKeyW(
        handle,
        nullptr,
        nullptr,
        nullptr,
        &subkey_count,
        &max_name_length,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr);
    if (info_status != ERROR_SUCCESS) {
        RegCloseKey(handle);
        report.diagnostics.push_back(win32_diagnostic(
            "registry-query-info-failed",
            "Failed to query Registry key metadata",
            info_status));
        return report;
    }

    for (DWORD index = 0; index != subkey_count; ++index) {
        std::wstring name(max_name_length + 1, L'\0');
        DWORD name_size = static_cast<DWORD>(name.size());
        const LSTATUS enum_status = RegEnumKeyExW(
            handle,
            index,
            name.data(),
            &name_size,
            nullptr,
            nullptr,
            nullptr,
            nullptr);
        if (enum_status != ERROR_SUCCESS) {
            RegCloseKey(handle);
            report.diagnostics.push_back(win32_diagnostic(
                "registry-enum-subkey-failed",
                "Failed to enumerate Registry subkey",
                enum_status));
            return report;
        }
        name.resize(name_size);
        report.names.push_back(narrow_utf8(name));
    }

    RegCloseKey(handle);
    report.ok = true;
    return report;
}
#else
value_report read_value(const key&, const std::string&)
{
    value_report report;
    report.diagnostics.push_back(make_diagnostic(
        severity::error,
        "registry-unsupported-platform",
        "Raw Windows Registry operations are not supported on this platform"));
    return report;
}

operation_report write_value(const key&, const value&, const options& options)
{
    operation_report report;
    report.dry_run = options.dry_run;
    report.diagnostics.push_back(make_diagnostic(
        severity::error,
        "registry-unsupported-platform",
        "Raw Windows Registry operations are not supported on this platform"));
    return report;
}

operation_report delete_value(const key&, const std::string&, const options& options)
{
    operation_report report;
    report.dry_run = options.dry_run;
    report.diagnostics.push_back(make_diagnostic(
        severity::error,
        "registry-unsupported-platform",
        "Raw Windows Registry operations are not supported on this platform"));
    return report;
}

operation_report delete_key(const key&, const options& options)
{
    operation_report report;
    report.dry_run = options.dry_run;
    report.diagnostics.push_back(make_diagnostic(
        severity::error,
        "registry-unsupported-platform",
        "Raw Windows Registry operations are not supported on this platform"));
    return report;
}

values_report enumerate_values(const key&)
{
    values_report report;
    report.diagnostics.push_back(make_diagnostic(
        severity::error,
        "registry-unsupported-platform",
        "Raw Windows Registry operations are not supported on this platform"));
    return report;
}

subkeys_report enumerate_subkeys(const key&)
{
    subkeys_report report;
    report.diagnostics.push_back(make_diagnostic(
        severity::error,
        "registry-unsupported-platform",
        "Raw Windows Registry operations are not supported on this platform"));
    return report;
}
#endif

namespace {

bool collect_snapshot_values(const key& root, const std::string& relative, snapshot& output, std::vector<diagnostic>& diagnostics)
{
    key current = root;
    current.subkey = combine_key_path(root, relative);

    const auto values = enumerate_values(current);
    diagnostics.insert(diagnostics.end(), values.diagnostics.begin(), values.diagnostics.end());
    if (!values.ok) {
        return false;
    }
    for (const auto& item : values.values) {
        output.values.push_back({relative, item});
    }

    const auto subkeys = enumerate_subkeys(current);
    diagnostics.insert(diagnostics.end(), subkeys.diagnostics.begin(), subkeys.diagnostics.end());
    if (!subkeys.ok) {
        return false;
    }
    for (const auto& subkey : subkeys.names) {
        const auto child = relative.empty() ? subkey : relative + "\\" + subkey;
        if (!collect_snapshot_values(root, child, output, diagnostics)) {
            return false;
        }
    }

    return true;
}

operation_report import_snapshot(const key& destination, const snapshot& snapshot, const options& options)
{
    operation_report report;
    report.dry_run = options.dry_run;
    if (!options.allow_import) {
        report.diagnostics.push_back(make_diagnostic(
            severity::error,
            "registry-import-denied",
            "Registry import requires allow_import"));
        return report;
    }

    report.ok = true;
    for (const auto& item : snapshot.values) {
        key target = destination;
        target.subkey = combine_key_path(destination, item.key_path);
        auto write_report = write_value(target, item.item, options);
        report.diagnostics.insert(
            report.diagnostics.end(),
            write_report.diagnostics.begin(),
            write_report.diagnostics.end());
        if (!write_report.ok) {
            report.ok = false;
        }
    }

    return report;
}

} // namespace

format_report export_tree_json(const key& key)
{
    format_report report;
    snapshot snapshot;
    snapshot.root = key;
    if (!collect_snapshot_values(key, {}, snapshot, report.diagnostics)) {
        return report;
    }
    return serialize_snapshot_json(snapshot);
}

operation_report import_tree_json(const key& key, std::string_view content, const options& options)
{
    auto parsed = parse_snapshot_json(content);
    operation_report report;
    report.dry_run = options.dry_run;
    report.diagnostics.insert(report.diagnostics.end(), parsed.diagnostics.begin(), parsed.diagnostics.end());
    if (!parsed.ok || !parsed.item) {
        return report;
    }
    auto imported = import_snapshot(key, *parsed.item, options);
    imported.diagnostics.insert(imported.diagnostics.begin(), report.diagnostics.begin(), report.diagnostics.end());
    return imported;
}

format_report export_tree_reg(const key& key)
{
    format_report report;
    snapshot snapshot;
    snapshot.root = key;
    if (!collect_snapshot_values(key, {}, snapshot, report.diagnostics)) {
        return report;
    }
    return serialize_snapshot_reg(snapshot);
}

operation_report import_tree_reg(const key& key, std::string_view content, const options& options)
{
    auto parsed = parse_snapshot_reg(content);
    operation_report report;
    report.dry_run = options.dry_run;
    report.diagnostics.insert(report.diagnostics.end(), parsed.diagnostics.begin(), parsed.diagnostics.end());
    if (!parsed.ok || !parsed.item) {
        return report;
    }
    auto imported = import_snapshot(key, *parsed.item, options);
    imported.diagnostics.insert(imported.diagnostics.begin(), report.diagnostics.begin(), report.diagnostics.end());
    return imported;
}

} // namespace registry

} // namespace linuxdesktop::migration
