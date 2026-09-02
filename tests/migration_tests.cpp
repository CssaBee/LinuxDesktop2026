#include "linuxdesktop/migration.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace {

namespace ld = linuxdesktop::migration;

struct test_failure {
    std::string message;
};

[[noreturn]] void fail(std::string message)
{
    throw test_failure{std::move(message)};
}

void require(bool condition, const std::string& message)
{
    if (!condition) {
        fail(message);
    }
}

bool has_diagnostic(const std::vector<ld::diagnostic>& diagnostics, const std::string& code)
{
    for (const auto& item : diagnostics) {
        if (item.code == code) {
            return true;
        }
    }
    return false;
}

bool has_error_diagnostic(const std::vector<ld::diagnostic>& diagnostics)
{
    for (const auto& item : diagnostics) {
        if (item.level == ld::severity::error) {
            return true;
        }
    }
    return false;
}

std::filesystem::path test_root()
{
    auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-migration-tests";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    if (ec) {
        fail("failed to create test root: " + ec.message());
    }
    return root;
}

std::string read_file(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void plan_is_dry_run_first()
{
    const auto root = test_root();
    const auto source = root / "old" / "config.xml";
    const auto target = root / "new" / "config.xml";
    std::filesystem::create_directories(source.parent_path());
    {
        std::ofstream file(source);
        file << "<Config />\n";
    }

    ld::options options;
    const auto plan = ld::plan_copy_file(source, target, options);
    require(plan.dry_run, "migration plans should be dry-run objects");
    require(plan.actions.size() == 1, "migration plan should keep actions");
    require(!has_error_diagnostic(plan.diagnostics), "valid migration plan should not have errors");
    require(ld::to_string(plan.actions[0].kind) == "copy_file", "migration action kind should stringify");

    const auto dry_run = ld::execute_migration_plan(plan, options);
    require(dry_run.ok, "dry-run execution should succeed for valid file action");
    require(dry_run.dry_run, "dry-run execution should report dry_run");
    require(dry_run.actions.size() == 1, "dry-run execution should report each action");
    require(dry_run.actions[0].planned, "dry-run action should be planned");
    require(!dry_run.actions[0].executed, "dry-run action should not execute");
    require(dry_run.actions[0].state == ld::migration_action_state::skipped,
        "dry-run action should report skipped state");
    require(!std::filesystem::exists(target), "dry-run should not create target");
}

void execute_copies_file()
{
    const auto root = test_root();
    const auto source = root / "old" / "config.xml";
    const auto target = root / "new" / "config.xml";
    std::filesystem::create_directories(source.parent_path());
    {
        std::ofstream file(source);
        file << "<Config copied=\"true\" />\n";
    }

    const auto plan = ld::plan_copy_file(source, target);

    ld::options execute_options;
    execute_options.dry_run = false;
    const auto report = ld::execute_migration_plan(plan, execute_options);

    require(report.ok, "migration execution should succeed");
    require(!report.dry_run, "migration execution should report non-dry-run");
    require(report.actions[0].executed, "copy action should execute");
    require(report.actions[0].state == ld::migration_action_state::executed,
        "copy action should report executed state");
    require(report.actions[0].source_existed_before, "copy action should report source existed before execution");
    require(!report.actions[0].target_existed_before, "copy action should report missing target before execution");
    require(report.actions[0].source_exists_after, "copy action should report source still exists after copy");
    require(report.actions[0].target_exists_after, "copy action should report target exists after copy");
    require(read_file(target).find("copied") != std::string::npos, "target should contain copied file");
}

void execute_overwrite_records_before_and_after_state()
{
    const auto root = test_root();
    const auto source = root / "old" / "config.xml";
    const auto target = root / "new" / "config.xml";
    std::filesystem::create_directories(source.parent_path());
    std::filesystem::create_directories(target.parent_path());
    {
        std::ofstream file(source);
        file << "<Config copied=\"new\" />\n";
    }
    {
        std::ofstream file(target);
        file << "<Config copied=\"old\" />\n";
    }

    ld::options plan_options;
    plan_options.overwrite_existing = true;
    const auto plan = ld::plan_copy_file(source, target, plan_options);

    ld::options execute_options;
    execute_options.dry_run = false;
    execute_options.overwrite_existing = true;
    const auto report = ld::execute_migration_plan(plan, execute_options);

    require(report.ok, "migration overwrite execution should succeed when overwrite_existing is true");
    require(report.actions.size() == 1, "migration overwrite execution should report one action");
    require(report.actions[0].target_existed_before, "migration overwrite should record existing target before execution");
    require(report.actions[0].source_existed_before, "migration overwrite should record existing source before execution");
    require(report.actions[0].source_exists_after, "migration overwrite should leave copy source in place");
    require(report.actions[0].target_exists_after, "migration overwrite should keep target after execution");
    require(report.actions[0].executed, "migration overwrite should mark action executed");
    require(read_file(target).find("new") != std::string::npos, "migration overwrite should replace target content");
}

void execute_blocks_destination_collision_without_overwrite()
{
    const auto root = test_root();
    const auto source = root / "old" / "config.xml";
    const auto target = root / "new" / "config.xml";
    std::filesystem::create_directories(source.parent_path());
    std::filesystem::create_directories(target.parent_path());
    {
        std::ofstream file(source);
        file << "<Config copied=\"new\" />\n";
    }
    {
        std::ofstream file(target);
        file << "<Config copied=\"old\" />\n";
    }

    const auto plan = ld::plan_copy_file(source, target);
    require(has_diagnostic(plan.diagnostics, "migration-target-exists"),
        "migration plan should diagnose destination collisions");

    ld::options execute_options;
    execute_options.dry_run = false;
    const auto report = ld::execute_migration_plan(plan, execute_options);

    require(!report.ok, "migration execution should reject invalid collision plans");
    require(report.actions.size() == 1, "migration collision execution should still report the action");
    require(report.actions[0].state == ld::migration_action_state::blocked,
        "migration collision execution should mark the action blocked");
    require(report.actions[0].source_existed_before, "migration collision should record source before state");
    require(report.actions[0].target_existed_before, "migration collision should record target before state");
    require(report.actions[0].source_exists_after, "migration collision should preserve source");
    require(report.actions[0].target_exists_after, "migration collision should preserve target");
    require(read_file(target).find("old") != std::string::npos,
        "migration collision should not overwrite target without permission");
}

void move_execution_requires_dangerous_permission()
{
    const auto root = test_root();
    const auto source = root / "old" / "config.xml";
    const auto target = root / "new" / "config.xml";
    std::filesystem::create_directories(source.parent_path());
    {
        std::ofstream file(source);
        file << "<Config />\n";
    }

    ld::options plan_options;
    plan_options.allow_dangerous = true;
    const auto plan = ld::plan_move_file(source, target, plan_options);

    ld::options execute_options;
    execute_options.dry_run = false;
    const auto report = ld::execute_migration_plan(plan, execute_options);

    require(!report.ok, "move execution should be denied by default");
    require(report.actions.size() == 1, "move denial should report the action");
    require(report.actions[0].state == ld::migration_action_state::blocked,
        "move denial should report blocked state");
    require(has_diagnostic(report.actions[0].diagnostics, "migration-dangerous-action-denied"),
        "move denial should require allow_dangerous");
    require(std::filesystem::exists(source), "denied move should keep source");
    require(!std::filesystem::exists(target), "denied move should not create target");
}

void execute_copies_directory()
{
    const auto root = test_root();
    const auto source = root / "old-profile";
    const auto target = root / "new-profile";
    std::filesystem::create_directories(source / "subdir");
    {
        std::ofstream file(source / "subdir" / "settings.ini");
        file << "copied=true\n";
    }

    const auto plan = ld::plan_copy_directory(source, target);
    ld::options execute_options;
    execute_options.dry_run = false;
    const auto report = ld::execute_migration_plan(plan, execute_options);

    require(report.ok, "directory copy should succeed");
    require(report.actions[0].state == ld::migration_action_state::executed,
        "directory copy should report executed state");
    require(std::filesystem::exists(source / "subdir" / "settings.ini"),
        "directory copy should keep source");
    require(read_file(target / "subdir" / "settings.ini").find("copied=true") != std::string::npos,
        "directory copy should copy nested files");
}

void execute_moves_file_with_permission()
{
    const auto root = test_root();
    const auto source = root / "old" / "state.bin";
    const auto target = root / "new" / "state.bin";
    std::filesystem::create_directories(source.parent_path());
    {
        std::ofstream file(source);
        file << "state";
    }

    ld::options plan_options;
    plan_options.allow_dangerous = true;
    const auto plan = ld::plan_move_file(source, target, plan_options);
    ld::options execute_options;
    execute_options.dry_run = false;
    execute_options.allow_dangerous = true;
    const auto report = ld::execute_migration_plan(plan, execute_options);

    require(report.ok, "file move should succeed with explicit dangerous permission");
    require(report.actions[0].state == ld::migration_action_state::executed,
        "file move should report executed state");
    require(!std::filesystem::exists(source), "file move should remove source");
    require(read_file(target) == "state", "file move should preserve content");
}

void rooted_paths_resolve_through_ld_paths()
{
    const auto root = test_root();

    ld::rooted_path_request request;
    request.identity.application = "migration-tests";
    request.resolver_options.home_directory = root / "home";
    request.resolver_options.use_process_environment = false;
    request.family = linuxdesktop::paths::path_family::state;
    request.relative_path = "registry-snapshot.json";

#if defined(_WIN32)
    request.resolver_options.environment = {{"LOCALAPPDATA", (root / "state").string()}};
    const auto expected = root / "state" / "migration-tests" / "state" / "registry-snapshot.json";
#else
    request.resolver_options.environment = {{"XDG_STATE_HOME", (root / "state").string()}};
    const auto expected = root / "state" / "migration-tests" / "registry-snapshot.json";
#endif

    const auto resolved = ld::resolve_rooted_path(request);
    require(resolved.path == expected,
        "rooted migration path should be resolved through ld_paths");
    require(!has_error_diagnostic(resolved.diagnostics), "rooted path should not report errors");

    request.relative_path = root / "absolute";
    const auto rejected = ld::resolve_rooted_path(request);
    require(has_diagnostic(rejected.diagnostics, "migration-rooted-path-relative-required"),
        "rooted migration path should reject absolute tails");
}

void plan_copy_infers_source_kind()
{
    const auto root = test_root();
    const auto file_source = root / "old" / "config.xml";
    const auto file_target = root / "new" / "config.xml";
    const auto directory_source = root / "old-profile";
    const auto directory_target = root / "new-profile";
    std::filesystem::create_directories(file_source.parent_path());
    std::filesystem::create_directories(directory_source / "subdir");
    {
        std::ofstream file(file_source);
        file << "<Config />\n";
    }
    {
        std::ofstream file(directory_source / "subdir" / "settings.ini");
        file << "copied=true\n";
    }

    const auto file_plan = ld::plan_copy(file_source, file_target);
    require(file_plan.actions.size() == 1, "copy helper should infer file source kind");
    require(ld::to_string(file_plan.actions[0].kind) == "copy_file",
        "copy helper should infer file actions");
    require(!has_error_diagnostic(file_plan.diagnostics), "copy helper should not warn for existing files");

    const auto directory_plan = ld::plan_copy(directory_source, directory_target);
    require(directory_plan.actions.size() == 1, "copy helper should infer directory source kind");
    require(ld::to_string(directory_plan.actions[0].kind) == "copy_directory",
        "copy helper should infer directory actions");
    require(!has_error_diagnostic(directory_plan.diagnostics),
        "copy helper should not warn for existing directories");
}

void plan_copy_missing_source_reports_diagnostic()
{
    const auto root = test_root();
    const auto source = root / "missing" / "config.xml";
    const auto target = root / "new" / "config.xml";

    const auto plan = ld::plan_copy(source, target);
    require(plan.actions.empty(), "missing source should not guess a copy action");
    require(has_diagnostic(plan.diagnostics, "migration-source-missing"),
        "missing source should report a diagnostic");
}

void dangerous_actions_are_denied_by_default()
{
    const auto plan = ld::plan_delete_registry_key();
    require(has_diagnostic(plan.diagnostics, "migration-dangerous-action-denied"),
        "dangerous migration action should require explicit permission");
}

void registry_json_snapshot_round_trips()
{
    namespace reg = linuxdesktop::migration::registry;

    reg::snapshot snapshot;
    snapshot.root.root = reg::hive::current_user;
    snapshot.root.subkey = "Software\\LinuxDesktop2026\\migration-tests";
    snapshot.root.registry_view = reg::view::native;

    reg::snapshot_value value;
    value.key_path = "Profiles";
    value.item.name = "Name";
    value.item.type = reg::value_type::string;
    value.item.bytes = {
        std::byte{'A'},
        std::byte{'l'},
        std::byte{'i'},
        std::byte{'c'},
        std::byte{'e'},
    };
    snapshot.values.push_back(value);

    const auto serialized = reg::serialize_snapshot_json(snapshot);
    require(serialized.ok, "Registry JSON snapshot serialization should succeed");
    require(serialized.content.find("linuxdesktop.settings.registry.snapshot.v1") != std::string::npos,
        "Registry JSON snapshot should include format marker");

    const auto parsed = reg::parse_snapshot_json(serialized.content);
    require(parsed.ok, "Registry JSON snapshot parsing should succeed");
    require(parsed.item.has_value(), "Registry JSON snapshot parsing should return a snapshot");
    require(parsed.item->root.subkey == snapshot.root.subkey, "Registry JSON root subkey should round-trip");
    require(parsed.item->values.size() == 1, "Registry JSON values should round-trip");
    require(parsed.item->values[0].key_path == "Profiles", "Registry JSON key path should round-trip");
    require(parsed.item->values[0].item.name == "Name", "Registry JSON value name should round-trip");
    require(parsed.item->values[0].item.bytes == value.item.bytes, "Registry JSON bytes should round-trip");
}

void registry_reg_snapshot_round_trips()
{
    namespace reg = linuxdesktop::migration::registry;

    reg::snapshot snapshot;
    snapshot.root.root = reg::hive::current_user;
    snapshot.root.subkey = "Software\\LinuxDesktop2026\\migration-tests";

    reg::snapshot_value string_value;
    string_value.item.name = "DisplayName";
    string_value.item.type = reg::value_type::string;
    string_value.item.bytes = {
        std::byte{'L'},
        std::byte{'D'},
        std::byte{'2'},
        std::byte{'0'},
        std::byte{'2'},
        std::byte{'6'},
    };
    snapshot.values.push_back(string_value);

    reg::snapshot_value binary_value;
    binary_value.key_path = "Binary";
    binary_value.item.name = "Blob";
    binary_value.item.type = reg::value_type::binary;
    binary_value.item.bytes = {
        std::byte{0x01},
        std::byte{0x02},
        std::byte{0xff},
    };
    snapshot.values.push_back(binary_value);

    reg::snapshot_value dword_value;
    dword_value.item.name = "Flags";
    dword_value.item.type = reg::value_type::dword;
    dword_value.item.bytes = {
        std::byte{0x01},
        std::byte{0x00},
        std::byte{0x00},
        std::byte{0x00},
    };
    snapshot.values.push_back(dword_value);

    const auto serialized = reg::serialize_snapshot_reg(snapshot);
    require(serialized.ok, ".reg snapshot serialization should succeed");
    require(serialized.content.find("Windows Registry Editor Version 5.00") != std::string::npos,
        ".reg snapshot should include header");
    require(serialized.content.find("[HKEY_CURRENT_USER\\Software\\LinuxDesktop2026\\migration-tests]") != std::string::npos,
        ".reg snapshot should include root key");

    const auto parsed = reg::parse_snapshot_reg(serialized.content);
    require(parsed.ok, ".reg snapshot parsing should succeed");
    require(parsed.item.has_value(), ".reg snapshot parsing should return a snapshot");
    require(parsed.item->root.root == reg::hive::current_user, ".reg hive should round-trip");
    require(parsed.item->root.subkey == snapshot.root.subkey, ".reg root key should round-trip");
    require(serialized.content.find("\"Flags\"=dword:00000001") != std::string::npos,
        ".reg DWORD should use standard eight-digit text");
    require(parsed.item->values.size() == 3, ".reg values should round-trip");
    require(parsed.item->values[0].item.name == "DisplayName", ".reg string value name should round-trip");
    require(parsed.item->values[1].key_path == "Binary", ".reg child key should be relative to root");
    require(parsed.item->values[1].item.bytes == binary_value.item.bytes, ".reg binary bytes should round-trip");
    require(parsed.item->values[2].item.bytes == dword_value.item.bytes, ".reg DWORD bytes should round-trip");
}

void registry_import_requires_explicit_permission()
{
    namespace reg = linuxdesktop::migration::registry;

    reg::snapshot snapshot;
    snapshot.root.root = reg::hive::current_user;
    snapshot.root.subkey = "Software\\LinuxDesktop2026\\migration-tests";
    reg::snapshot_value value;
    value.item.name = "Name";
    value.item.type = reg::value_type::string;
    value.item.bytes = {std::byte{'A'}};
    snapshot.values.push_back(value);

    const auto serialized = reg::serialize_snapshot_json(snapshot);
    reg::key destination;
    destination.subkey = snapshot.root.subkey;
    const auto imported = reg::import_tree_json(destination, serialized.content);

    require(!imported.ok, "Registry JSON import should be denied by default");
    require(has_diagnostic(imported.diagnostics, "registry-import-denied"),
        "Registry JSON import should require allow_import");
}

void registry_json_rejects_hostile_import_shapes()
{
    namespace reg = linuxdesktop::migration::registry;

    const std::string root =
        "\"root\":{\"hive\":\"current_user\",\"subkey\":\"Software\\\\LinuxDesktop2026\\\\migration-tests\",\"view\":\"native\"}";

    const auto missing_format = reg::parse_snapshot_json("{" + root + ",\"values\":[]}");
    require(!missing_format.ok, "Registry JSON parser should reject missing format markers");
    require(has_diagnostic(missing_format.diagnostics, "registry-json-format-invalid"),
        "Registry JSON parser should diagnose missing format markers");

    const auto malformed_values = reg::parse_snapshot_json(
        "{\"format\":\"linuxdesktop.settings.registry.snapshot.v1\"," + root + ",\"values\":{}}");
    require(!malformed_values.ok, "Registry JSON parser should reject non-array values");
    require(has_diagnostic(malformed_values.diagnostics, "registry-json-values-invalid"),
        "Registry JSON parser should diagnose malformed values arrays");

    const auto truncated_values = reg::parse_snapshot_json(
        "{\"format\":\"linuxdesktop.settings.registry.snapshot.v1\"," + root +
        ",\"values\":[{\"key_path\":\"Profiles\"}");
    require(!truncated_values.ok, "Registry JSON parser should reject truncated values arrays");
    require(has_diagnostic(truncated_values.diagnostics, "registry-json-values-invalid"),
        "Registry JSON parser should diagnose truncated values arrays");

    const auto invalid_hex = reg::parse_snapshot_json(
        "{\"format\":\"linuxdesktop.settings.registry.snapshot.v1\"," + root +
        ",\"values\":[{\"key_path\":\"Profiles\",\"name\":\"Name\",\"type\":\"string\",\"data_hex\":\"abc\"}]}");
    require(!invalid_hex.ok, "Registry JSON parser should reject odd-length hex payloads");
    require(has_diagnostic(invalid_hex.diagnostics, "registry-json-value-invalid"),
        "Registry JSON parser should diagnose invalid value payloads");

    reg::key destination;
    destination.subkey = "Software\\LinuxDesktop2026\\migration-tests";
    const auto imported = reg::import_tree_json(destination,
        "{\"format\":\"linuxdesktop.settings.registry.snapshot.v1\"," + root + ",\"values\":{}}");
    require(!imported.ok, "Registry JSON import should reject malformed snapshots before permission checks");
    require(has_diagnostic(imported.diagnostics, "registry-json-values-invalid"),
        "Registry JSON import should propagate parse diagnostics");
}

void registry_reg_rejects_hostile_import_shapes()
{
    namespace reg = linuxdesktop::migration::registry;

    const auto value_before_key = reg::parse_snapshot_reg("\"Name\"=\"Alice\"\n");
    require(!value_before_key.ok, ".reg parser should reject values before a key");
    require(has_diagnostic(value_before_key.diagnostics, "registry-reg-value-invalid"),
        ".reg parser should diagnose values before a key");

    const auto unknown_hive = reg::parse_snapshot_reg(
        "Windows Registry Editor Version 5.00\n\n[HKEY_NOT_REAL\\Software\\LinuxDesktop2026]\n");
    require(!unknown_hive.ok, ".reg parser should reject unknown hives");
    require(has_diagnostic(unknown_hive.diagnostics, "registry-reg-hive-invalid"),
        ".reg parser should diagnose unknown hives");

    const auto multiple_hives = reg::parse_snapshot_reg(
        "Windows Registry Editor Version 5.00\n\n"
        "[HKEY_CURRENT_USER\\Software\\LinuxDesktop2026]\n"
        "\"Name\"=\"Alice\"\n\n"
        "[HKEY_LOCAL_MACHINE\\Software\\LinuxDesktop2026]\n"
        "\"Name\"=\"Bob\"\n");
    require(!multiple_hives.ok, ".reg parser should reject multiple hive snapshots");
    require(has_diagnostic(multiple_hives.diagnostics, "registry-reg-multiple-hives"),
        ".reg parser should diagnose multiple hives");

    const auto outside_root = reg::parse_snapshot_reg(
        "Windows Registry Editor Version 5.00\n\n"
        "[HKEY_CURRENT_USER\\Software\\LinuxDesktop2026]\n"
        "\"Name\"=\"Alice\"\n\n"
        "[HKEY_CURRENT_USER\\Software\\OtherProduct]\n"
        "\"Name\"=\"Mallory\"\n");
    require(!outside_root.ok, ".reg parser should reject keys outside the first root");
    require(has_diagnostic(outside_root.diagnostics, "registry-reg-outside-root"),
        ".reg parser should diagnose keys outside the first root");

    const auto invalid_dword = reg::parse_snapshot_reg(
        "Windows Registry Editor Version 5.00\n\n"
        "[HKEY_CURRENT_USER\\Software\\LinuxDesktop2026]\n"
        "\"Flags\"=dword:zzzzzzzz\n");
    require(!invalid_dword.ok, ".reg parser should reject malformed DWORD data");
    require(has_diagnostic(invalid_dword.diagnostics, "registry-reg-value-data-invalid"),
        ".reg parser should diagnose malformed DWORD data");

    reg::key destination;
    destination.subkey = "Software\\LinuxDesktop2026";
    const auto imported = reg::import_tree_reg(destination,
        "Windows Registry Editor Version 5.00\n\n"
        "[HKEY_CURRENT_USER\\Software\\LinuxDesktop2026]\n"
        "\"Flags\"=dword:zzzzzzzz\n");
    require(!imported.ok, ".reg import should reject malformed snapshots before permission checks");
    require(has_diagnostic(imported.diagnostics, "registry-reg-value-data-invalid"),
        ".reg import should propagate parse diagnostics");
}

} // namespace

int main()
{
    const std::vector<std::pair<std::string, void (*)()>> tests = {
        {"plan_is_dry_run_first", plan_is_dry_run_first},
        {"execute_copies_file", execute_copies_file},
        {"execute_overwrite_records_before_and_after_state", execute_overwrite_records_before_and_after_state},
        {"execute_blocks_destination_collision_without_overwrite", execute_blocks_destination_collision_without_overwrite},
        {"move_execution_requires_dangerous_permission", move_execution_requires_dangerous_permission},
        {"execute_copies_directory", execute_copies_directory},
        {"execute_moves_file_with_permission", execute_moves_file_with_permission},
        {"rooted_paths_resolve_through_ld_paths", rooted_paths_resolve_through_ld_paths},
        {"plan_copy_infers_source_kind", plan_copy_infers_source_kind},
        {"plan_copy_missing_source_reports_diagnostic", plan_copy_missing_source_reports_diagnostic},
        {"dangerous_actions_are_denied_by_default", dangerous_actions_are_denied_by_default},
        {"registry_json_snapshot_round_trips", registry_json_snapshot_round_trips},
        {"registry_reg_snapshot_round_trips", registry_reg_snapshot_round_trips},
        {"registry_import_requires_explicit_permission", registry_import_requires_explicit_permission},
        {"registry_json_rejects_hostile_import_shapes", registry_json_rejects_hostile_import_shapes},
        {"registry_reg_rejects_hostile_import_shapes", registry_reg_rejects_hostile_import_shapes},
    };

    int failures = 0;
    for (const auto& test : tests) {
        try {
            test.second();
            std::cout << "ok " << test.first << "\n";
        } catch (const test_failure& failure) {
            ++failures;
            std::cerr << "not ok " << test.first << ": " << failure.message << "\n";
        } catch (const std::exception& exception) {
            ++failures;
            std::cerr << "not ok " << test.first << ": unexpected exception: " << exception.what() << "\n";
        }
    }

    return failures == 0 ? 0 : 1;
}
