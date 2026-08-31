#include "linuxdesktop/settings.hpp"
#include "linuxdesktop/settings_c.h"
#include "linuxdesktop/desktop.hpp"
#include "linuxdesktop/migration.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace {

namespace ld = linuxdesktop::settings;
namespace ld_paths = linuxdesktop::paths;
namespace desk = linuxdesktop::desktop;
namespace mig = linuxdesktop::migration;

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
    auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-settings-tests";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    if (ec) {
        fail("failed to create test root: " + ec.message());
    }
    return root;
}

std::string path_to_utf8_string(const std::filesystem::path& value)
{
    const auto text = value.u8string();
    return std::string(reinterpret_cast<const char*>(text.data()), text.size());
}

std::string read_file(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

ld::app_identity identity()
{
    ld::app_identity value;
    value.organization = "LinuxDesktop2026";
    value.application = "settings-tests";
    return value;
}

void exposes_cpp_version()
{
    require(ld::version_major == 0, "C++ version major should match project version");
    require(ld::version_minor == 1, "C++ version minor should match project version");
    require(ld::version_patch == 0, "C++ version patch should match project version");
}

void settings_diagnostics_use_shared_core_vocabulary()
{
    ld::diagnostic settings_diagnostic;
    settings_diagnostic.level = ld::severity::warning;
    settings_diagnostic.code = "shared-diagnostic";

    linuxdesktop::diagnostic core_diagnostic = settings_diagnostic;
    require(core_diagnostic.code == "shared-diagnostic", "settings diagnostics should alias shared diagnostics");
    require(linuxdesktop::to_string(core_diagnostic.level) == "warning", "shared severity should stringify");
    require(ld::to_string(settings_diagnostic.level) == "warning", "settings severity alias should stringify");
}

void writes_absolute_settings_override()
{
    const auto root = test_root() / "override";

    ld::root_options options;
    options.settings_override = root;

    const auto report = ld::resolve_settings_roots(identity(), options);

    require(report.settings_override_active, "settings override should be active");
    require(report.roots.config == root, "settings override should own config");
    require(report.roots.data == root, "settings override should own data");
    require(report.roots.state == root, "settings override should own state");
    require(report.roots.session == root / "sessions", "settings override should own session");
}

void rejects_relative_settings_override()
{
    ld::root_options options;
    options.settings_override = "relative-settings";

    const auto report = ld::resolve_settings_roots(identity(), options);

    require(!report.settings_override_active, "relative settings override must not activate");
    require(has_diagnostic(report.diagnostics, "settings-override-relative"),
        "relative settings override should report a diagnostic");
}

void denies_portable_under_privileged_install_root()
{
    const auto root = test_root();
    const auto install = root / "Program Files" / "Notepad++";
    const auto marker = install / "doLocalConf.xml";
    std::filesystem::create_directories(install);
    {
        std::ofstream marker_file(marker);
        marker_file << "<localConf />\n";
    }

    ld::root_options options;
    options.resource_root = install;
    options.portable_marker = marker;
    options.privileged_install_roots = {root / "Program Files"};
    options.deny_portable_root_in_privileged_install = true;

    const auto report = ld::resolve_settings_roots(identity(), options);

    require(report.portable_requested, "portable marker should be requested");
    require(!report.portable_active, "portable root should be denied under privileged install root");
    require(has_diagnostic(report.diagnostics, "portable-denied-privileged-install"),
        "privileged portable denial should report a diagnostic");
}

void sync_config_override_keeps_state_local()
{
    const auto root = test_root();
    const auto sync = root / "sync-config";

    ld::root_options options;
    options.sync_config_override = sync;

    const auto report = ld::resolve_settings_roots(identity(), options);

    require(report.sync_config_override_active, "sync config override should be active");
    require(report.roots.config == sync, "sync config override should own config");
    require(report.roots.plugin_config == sync / "plugins" / "Config",
        "plugin config should follow config root");
    require(report.roots.state != sync, "state should stay local when only config is synced");
    require(report.roots.session == report.roots.state / "sessions", "session should follow state");
}

void rejects_relative_sync_config_override()
{
    ld::root_options options;
    options.sync_config_override = "relative-sync";

    const auto report = ld::resolve_settings_roots(identity(), options);

    require(!report.sync_config_override_active, "relative sync config override must not activate");
    require(has_diagnostic(report.diagnostics, "sync-config-override-relative"),
        "relative sync config override should report a diagnostic");
}

void settings_override_wins_over_sync_override()
{
    const auto root = test_root();
    const auto settings = root / "settings";
    const auto sync = root / "sync";

    ld::root_options options;
    options.settings_override = settings;
    options.sync_config_override = sync;

    const auto report = ld::resolve_settings_roots(identity(), options);

    require(report.settings_override_active, "settings override should be active");
    require(!report.sync_config_override_active, "sync override should not activate over settings override");
    require(report.roots.config == settings, "settings override should win config");
    require(report.roots.state == settings, "settings override should win state");
    require(has_diagnostic(report.diagnostics, "sync-config-override-ignored"),
        "ignored sync override should report a diagnostic");
}

void delegates_generic_roots_to_paths_with_injected_environment()
{
    const auto root = test_root();

    ld::root_options options;
    options.create_directories = false;
    options.use_process_environment = false;
    options.home_directory = root / "home";
#if defined(_WIN32)
    options.environment["APPDATA"] = (root / "appdata" / "roaming").string();
    options.environment["LOCALAPPDATA"] = "relative-local";
#else
    options.environment["XDG_CONFIG_HOME"] = "relative-config";
    options.environment["XDG_DATA_HOME"] = (root / "xdg-data").string();
    options.environment["XDG_STATE_HOME"] = (root / "xdg-state").string();
    options.environment["XDG_CACHE_HOME"] = (root / "xdg-cache").string();
    options.environment["XDG_RUNTIME_DIR"] = (root / "xdg-runtime").string();
#endif

    const auto report = ld::resolve_settings_roots(identity(), options);

    require(has_diagnostic(report.diagnostics, "paths.environment.relative_ignored"),
        "settings root resolution should expose ld_paths diagnostics");
#if defined(_WIN32)
    require(report.roots.config == root / "appdata" / "roaming" / "LinuxDesktop2026" / "settings-tests",
        "settings config root should use ld_paths Windows roaming selection");
    require(report.roots.data == root / "appdata" / "roaming" / "LinuxDesktop2026" / "settings-tests",
        "settings data root should use ld_paths Windows roaming selection");
#else
    require(report.roots.config == root / "home" / ".config" / "LinuxDesktop2026" / "settings-tests",
        "settings config root should fall back through ld_paths");
    require(report.roots.data == root / "xdg-data" / "LinuxDesktop2026" / "settings-tests",
        "settings data root should use ld_paths XDG data selection");
    require(report.roots.state == root / "xdg-state" / "LinuxDesktop2026" / "settings-tests",
        "settings state root should use ld_paths XDG state selection");
    require(report.roots.cache == root / "xdg-cache" / "LinuxDesktop2026" / "settings-tests",
        "settings cache root should use ld_paths XDG cache selection");
    require(report.roots.runtime == root / "xdg-runtime" / "settings-tests",
        "settings runtime root should use ld_paths XDG runtime selection");
#endif
}

void delegates_platform_defaults_to_paths()
{
    const auto root = test_root();

    ld::root_options options;
    options.create_directories = false;
    options.use_process_environment = false;
#if defined(_WIN32)
    options.platform_defaults = ld_paths::platform_path_defaults::windows(root / "home");
#else
    options.platform_defaults = ld_paths::platform_path_defaults::xdg(root / "home", root / "runtime");
#endif

    const auto report = ld::resolve_settings_roots(identity(), options);

    require(!has_error_diagnostic(report.diagnostics),
        "settings root resolution should accept absolute platform defaults");
#if defined(_WIN32)
    require(report.roots.config == root / "home" / "AppData" / "Roaming" / "LinuxDesktop2026" / "settings-tests",
        "settings config root should use Windows platform defaults");
    require(report.roots.cache == root / "home" / "AppData" / "Local" / "LinuxDesktop2026" / "settings-tests",
        "settings cache root should use Windows local platform defaults");
#else
    require(report.roots.config == root / "home" / ".config" / "LinuxDesktop2026" / "settings-tests",
        "settings config root should use XDG platform defaults");
    require(report.roots.data == root / "home" / ".local" / "share" / "LinuxDesktop2026" / "settings-tests",
        "settings data root should use XDG platform defaults");
    require(report.roots.runtime == root / "runtime" / "settings-tests",
        "settings runtime root should use XDG runtime platform defaults");
#endif
}

void reports_path_directory_failure_for_generic_root_creation()
{
    const auto root = test_root();
    const auto file_root = root / "not-a-directory";
    {
        std::ofstream file(file_root);
        file << "not a directory\n";
    }

    ld::root_options options;
    options.settings_override = file_root;
    options.use_process_environment = false;
    options.home_directory = root / "home";

    const auto report = ld::resolve_settings_roots(identity(), options);

    require(has_diagnostic(report.diagnostics, "paths.directory.exists_as_file"),
        "generic root creation failures should come from ld_paths diagnostics");
}

void resolves_settings_layers()
{
    const auto root = test_root() / "settings";

    ld::root_options options;
    options.settings_override = root;

    const auto report = ld::resolve_settings_roots(identity(), options);

    require(report.layers.candidates.size() >= 6, "layer report should include default/user/local/managed/enforced candidates");
    require(report.layers.active_write_layer.has_value(), "layer report should include active write layer");
    require(report.layers.active_read_order.front().kind == ld::config_layer_kind::enforced,
        "enforced layer should have highest default precedence");
    const auto* user_layer = ld::find_config_layer(report.layers, ld::config_layer_kind::user);
    require(user_layer != nullptr, "C++ helper should find layer candidates by kind");
    require(user_layer->backend == ld::storage_backend::file || user_layer->backend == ld::storage_backend::registry,
        "user layer should expose its storage backend");
    require(ld::to_string(report.portable) == "settings_only", "portable level should stringify for diagnostics");
    require(ld::to_string(user_layer->kind) == "user", "layer kind should stringify for diagnostics");
    require(ld::to_string(user_layer->backend) == "file" || ld::to_string(user_layer->backend) == "registry",
        "storage backend should stringify for diagnostics");
}

void root_builder_preserves_root_options()
{
    const auto root = test_root();
    const auto install = root / "Application";
    const auto settings = root / "settings";
    const auto sync = root / "sync";

    const auto report = ld::root_builder()
        .app("BuilderOrg", "BuilderApp")
        .resource_root(install)
        .home_directory(root / "home")
        .environment({{"XDG_CONFIG_HOME", (root / "xdg-config").string()}})
        .use_process_environment(false)
        .settings_override(settings)
        .sync_config_override(sync)
        .portable_marker(install / "portable.marker")
        .portable(ld::portable_level::profile)
        .allow_sync_config_for_portable_root(true)
        .create_directories(false)
        .resolve();

    require(report.settings_override_active, "builder should preserve settings override");
    require(!report.sync_config_override_active, "builder should preserve settings-over-sync precedence");
    require(report.roots.resources == install, "builder should preserve resource root");
    require(report.roots.config == settings, "builder should preserve config root");
    require(report.roots.state == settings, "builder should preserve state root");
    require(!std::filesystem::exists(settings), "builder should preserve create-directories policy");
}

#if defined(_WIN32)
void windows_default_roots_are_resolved()
{
    ld::root_options options;
    options.create_directories = false;

    const auto report = ld::resolve_settings_roots(identity(), options);

    require(!report.roots.config.empty(), "Windows config root should be resolved");
    require(!report.roots.data.empty(), "Windows data root should be resolved");
    require(!report.roots.state.empty(), "Windows state root should be resolved");
    require(!report.roots.cache.empty(), "Windows cache root should be resolved");
    require(!report.roots.session.empty(), "Windows session root should be resolved");
}
#endif

void ensure_config_defaults_copies_missing_models()
{
    const auto root = test_root();
    const auto models = root / "models";
    const auto target = root / "config";
    std::filesystem::create_directories(models);
    {
        std::ofstream model(models / "config.model.xml");
        model << "<Config />\n";
    }

    ld::config_file file;
    file.name = "config.xml";
    file.model_name = "config.model.xml";
    file.required = true;

    ld::hydrate_options options;
    options.model_root = models;
    options.target_root = target;
    options.files = {file};

    const auto report = ld::ensure_config_defaults(options);

    require(report.copied.size() == 1, "config defaults should copy one model");
    require(std::filesystem::exists(target / "config.xml"), "config default target should exist");
}

void legacy_hydrate_config_bundle_forwards_to_config_defaults()
{
    const auto root = test_root();
    const auto models = root / "models";
    const auto target = root / "config";
    std::filesystem::create_directories(models);
    {
        std::ofstream model(models / "config.model.xml");
        model << "<Config />\n";
    }

    ld::config_file file;
    file.name = "config.xml";
    file.model_name = "config.model.xml";
    file.required = true;

    ld::hydrate_options options;
    options.model_root = models;
    options.target_root = target;
    options.files = {file};

    const auto report = ld::hydrate_config_bundle(options);

    require(report.copied.size() == 1, "legacy hydration API should keep forwarding");
    require(std::filesystem::exists(target / "config.xml"), "legacy hydration target should exist");
}

void common_config_write_replaces_target_with_backup()
{
    const auto root = test_root();
    const auto target = root / "config.xml";
    std::filesystem::create_directories(root);
    {
        std::ofstream existing(target);
        existing << "<Config saved=\"old\" />\n";
    }

    const auto report = ld::write_common_config({target, "<Config saved=\"new\" />\n", true},
        [](const std::filesystem::path& path, std::string&) {
        return read_file(path).find("new") != std::string::npos;
    });

    require(report.ok, "valid common config write should succeed");
    require(report.backup_path.has_value(), "valid common config write should keep old target as backup");
    require(report.temp_path.has_value(), "common config write should report temp path");
    require(report.durable_write, "common config write should report durable mode when enabled");
    require(!std::filesystem::exists(*report.temp_path), "common config temp file should be replaced away");
    require(read_file(target).find("new") != std::string::npos, "target should contain new content");
    require(read_file(*report.backup_path).find("old") != std::string::npos, "backup should contain old content");
}

void common_config_write_validation_keeps_original_target()
{
    const auto root = test_root();
    const auto target = root / "config.xml";
    std::filesystem::create_directories(root);
    {
        std::ofstream existing(target);
        existing << "<Config saved=\"old\" />\n";
    }

    const auto report = ld::write_common_config({target, "", false}, [](const std::filesystem::path&, std::string& message) {
        message = "empty writes are invalid in this test";
        return false;
    });

    require(!report.ok, "invalid common config write should fail");
    require(!report.backup_path.has_value(), "invalid common config write should not need a backup");
    require(report.temp_path.has_value(), "invalid common config write should report temp path");
    require(!std::filesystem::exists(*report.temp_path), "invalid common config temp file should be cleaned");
    require(has_diagnostic(report.diagnostics, "temp-cleaned"), "invalid common config write should report temp cleanup");
    require(read_file(target).find("old") != std::string::npos, "original target should stay untouched");
}

void direct_write_validation_restores_backup()
{
    const auto root = test_root();
    const auto target = root / "config.xml";
    std::filesystem::create_directories(root);
    {
        std::ofstream existing(target);
        existing << "<Config saved=\"old\" />\n";
    }

    ld::write_options options;
    options.target = target;
    options.content = "";
    options.keep_backup = true;
    options.atomic_replace = false;

    const auto report = ld::write_with_backup(options, [](const std::filesystem::path&, std::string& message) {
        message = "empty writes are invalid in this test";
        return false;
    });

    require(!report.ok, "invalid direct write should fail");
    require(report.backup_path.has_value(), "invalid direct write should have backup");
    require(has_diagnostic(report.diagnostics, "backup-restored"), "invalid direct write should restore backup");
    require(std::filesystem::file_size(target) > 0, "restored target should not be empty");
}

void common_config_write_reports_durable_mode_and_keeps_backup()
{
    const auto root = test_root();
    const auto target = root / "config.xml";
    std::filesystem::create_directories(root);
    {
        std::ofstream existing(target);
        existing << "<Config saved=\"old\" />\n";
    }

    const auto report = ld::write_common_config({target, "<Config saved=\"new\" />\n", true},
        [](const std::filesystem::path& path, std::string&) {
        return read_file(path).find("new") != std::string::npos;
    });

    require(report.ok, "durable common config write should succeed");
    require(report.durable_write, "durable common config write should report durable mode");
    require(report.backup_path.has_value(), "durable common config write should keep backup when requested");
    require(read_file(target).find("new") != std::string::npos, "durable common config write should update target");
    require(read_file(*report.backup_path).find("old") != std::string::npos, "durable common config write backup should contain old content");
}

void common_config_write_restores_backup_after_readback_mismatch()
{
    const auto root = test_root();
    const auto target = root / "config.xml";
    std::filesystem::create_directories(root);
    {
        std::ofstream existing(target);
        existing << "<Config saved=\"old\" />\n";
    }

    const auto report = ld::write_common_config({target, "<Config saved=\"new\" />\n", true},
        [](const std::filesystem::path& path, std::string&) {
        std::ofstream corrupt(path, std::ios::binary | std::ios::trunc);
        corrupt << "<Config saved=\"corrupt\" />\n";
        return true;
    });

    require(!report.ok, "readback mismatch should fail the common config write");
    require(report.durable_write, "readback mismatch should still report durable mode");
    require(has_diagnostic(report.diagnostics, "write-readback-mismatch"), "readback mismatch should be reported");
    require(has_diagnostic(report.diagnostics, "backup-restored-after-readback-failure"), "backup should be restored after readback mismatch");
    require(read_file(target).find("old") != std::string::npos, "readback mismatch should restore original target content");
}

void migration_plan_is_dry_run_first()
{
    const auto root = test_root();
    const auto source = root / "old" / "config.xml";
    const auto target = root / "new" / "config.xml";
    std::filesystem::create_directories(source.parent_path());
    {
        std::ofstream file(source);
        file << "<Config />\n";
    }

    mig::options options;
    const auto plan = mig::plan_copy_file(source, target, options);
    require(plan.dry_run, "migration plans should be dry-run objects");
    require(plan.actions.size() == 1, "migration plan should keep actions");
    require(!has_error_diagnostic(plan.diagnostics), "valid migration plan should not have errors");
    require(mig::to_string(plan.actions[0].kind) == "copy_file", "migration action kind should stringify");

    const auto dry_run = mig::execute_migration_plan(plan, options);
    require(dry_run.ok, "dry-run execution should succeed for valid file action");
    require(dry_run.dry_run, "dry-run execution should report dry_run");
    require(dry_run.actions.size() == 1, "dry-run execution should report each action");
    require(dry_run.actions[0].planned, "dry-run action should be planned");
    require(!dry_run.actions[0].executed, "dry-run action should not execute");
    require(!std::filesystem::exists(target), "dry-run should not create target");
}

void migration_execute_copies_file()
{
    const auto root = test_root();
    const auto source = root / "old" / "config.xml";
    const auto target = root / "new" / "config.xml";
    std::filesystem::create_directories(source.parent_path());
    {
        std::ofstream file(source);
        file << "<Config copied=\"true\" />\n";
    }

    mig::options plan_options;
    const auto plan = mig::plan_copy_file(source, target, plan_options);

    mig::options execute_options;
    execute_options.dry_run = false;
    const auto report = mig::execute_migration_plan(plan, execute_options);

    require(report.ok, "migration execution should succeed");
    require(!report.dry_run, "migration execution should report non-dry-run");
    require(report.actions[0].executed, "copy action should execute");
    require(read_file(target).find("copied") != std::string::npos, "target should contain copied file");
}

void migration_blocks_dangerous_without_permission()
{
    const auto plan = mig::plan_delete_registry_key();
    require(has_diagnostic(plan.diagnostics, "migration-dangerous-action-denied"),
        "dangerous migration action should require explicit permission");
}

void registry_reports_unsupported_on_linux()
{
#if !defined(_WIN32)
    mig::registry::key key;
    key.subkey = "Software/LinuxDesktop2026/settings-tests";

    const auto report = mig::registry::read_value(key, "Example");
    require(!report.ok, "Linux raw Registry read should not succeed");
    require(has_diagnostic(report.diagnostics, "registry-unsupported-platform"),
        "Linux raw Registry read should report unsupported platform");
    require(mig::registry::to_string(key.root) == "current_user",
        "Registry hive should stringify");
    require(mig::registry::to_string(key.registry_view) == "native",
        "Registry view should stringify");
    require(mig::registry::to_string(mig::registry::value_type::dword) == "dword",
        "Registry value type should stringify");
#endif
}

void registry_json_snapshot_round_trips()
{
    namespace reg = mig::registry;

    reg::snapshot snapshot;
    snapshot.root.root = reg::hive::current_user;
    snapshot.root.subkey = "Software\\LinuxDesktop2026\\settings-tests";
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
    namespace reg = mig::registry;

    reg::snapshot snapshot;
    snapshot.root.root = reg::hive::current_user;
    snapshot.root.subkey = "Software\\LinuxDesktop2026\\settings-tests";

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
    require(serialized.content.find("[HKEY_CURRENT_USER\\Software\\LinuxDesktop2026\\settings-tests]") != std::string::npos,
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
    namespace reg = mig::registry;

    reg::snapshot snapshot;
    snapshot.root.root = reg::hive::current_user;
    snapshot.root.subkey = "Software\\LinuxDesktop2026\\settings-tests";
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
    namespace reg = mig::registry;

    const std::string root =
        "\"root\":{\"hive\":\"current_user\",\"subkey\":\"Software\\\\LinuxDesktop2026\\\\settings-tests\",\"view\":\"native\"}";

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
    destination.subkey = "Software\\LinuxDesktop2026\\settings-tests";
    const auto imported = reg::import_tree_json(destination,
        "{\"format\":\"linuxdesktop.settings.registry.snapshot.v1\"," + root + ",\"values\":{}}");
    require(!imported.ok, "Registry JSON import should reject malformed snapshots before permission checks");
    require(has_diagnostic(imported.diagnostics, "registry-json-values-invalid"),
        "Registry JSON import should propagate parse diagnostics");
}

void registry_reg_rejects_hostile_import_shapes()
{
    namespace reg = mig::registry;

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

void common_config_write_rejects_parent_that_is_file()
{
    const auto root = test_root();
    const auto parent = root / "not-a-directory";
    {
        std::ofstream file(parent);
        file << "not a directory\n";
    }

    const auto report = ld::write_common_config({parent / "config.xml", "<Config />\n", true});

    require(!report.ok, "common config write should fail when parent exists as a file");
    require(has_diagnostic(report.diagnostics, "path-not-directory"),
        "common config write should diagnose file-as-directory parents");
}

void common_config_write_cleans_temp_after_backup_copy_failure()
{
    const auto root = test_root();
    const auto target = root / "config.xml";
    std::filesystem::create_directories(root);
    {
        std::ofstream existing(target);
        existing << "<Config saved=\"old\" />\n";
    }
    std::filesystem::create_directories(target.string() + ".bak");

    const auto report = ld::write_common_config({target, "<Config saved=\"new\" />\n", true},
        [](const std::filesystem::path& path, std::string&) {
            return read_file(path).find("new") != std::string::npos;
        });

    require(!report.ok, "common config write should fail when backup path cannot be replaced");
    require(report.temp_path.has_value(), "backup-copy failure should report the temp path");
    require(has_diagnostic(report.diagnostics, "backup-copy-failed"),
        "common config write should diagnose backup-copy failures");
    require(!std::filesystem::exists(*report.temp_path),
        "common config write should clean temporary files after backup-copy failures");
    require(read_file(target).find("old") != std::string::npos,
        "backup-copy failure should keep the original target content");
}

desk::autostart_entry autostart_entry_for_tests()
{
    desk::autostart_entry entry;
    entry.id = "linuxdesktop2026-settings-tests";
    entry.display_name = "LinuxDesktop2026 Settings Tests";
    entry.executable = "/usr/bin/ld-settings-test";
    entry.arguments = {"--profile", "Default User"};
    return entry;
}

void autostart_dry_run_does_not_write()
{
    const auto root = test_root() / "autostart";
    const auto entry = autostart_entry_for_tests();

    desk::apply_options options;
    options.allow_desktop_integration_write = true;
    options.autostart_directory_override = root;

    const auto report = desk::apply_autostart(entry, options);
    require(report.ok, "autostart dry-run should succeed");
    require(report.dry_run, "autostart dry-run should report dry_run");
    require(report.path.has_value(), "autostart dry-run should report target path");
    require(!std::filesystem::exists(*report.path), "autostart dry-run should not write a file");
    require(has_diagnostic(report.diagnostics, "autostart-dry-run"),
        "autostart dry-run should include a dry-run diagnostic");
}

void autostart_linux_writes_queries_and_removes_desktop_file()
{
#if !defined(_WIN32)
    const auto root = test_root() / "autostart";
    const auto entry = autostart_entry_for_tests();

    desk::apply_options options;
    options.dry_run = false;
    options.allow_desktop_integration_write = true;
    options.autostart_directory_override = root;

    const auto applied = desk::apply_autostart(entry, options);
    require(applied.ok, "Linux autostart write should succeed");
    require(applied.path.has_value(), "Linux autostart write should report path");
    const auto content = read_file(*applied.path);
    require(content.find("[Desktop Entry]") != std::string::npos, "autostart file should be a desktop entry");
    require(content.find("Type=Application") != std::string::npos, "autostart file should be an application entry");
    require(content.find("Name=LinuxDesktop2026 Settings Tests") != std::string::npos,
        "autostart file should include display name");
    require(content.find("Exec=/usr/bin/ld-settings-test --profile 'Default User'") != std::string::npos,
        "autostart file should quote arguments in Exec");

    auto queried = desk::query_autostart(entry, options);
    require(queried.ok, "Linux autostart query should succeed");
    require(queried.enabled, "Linux autostart query should report enabled file");

    auto disabled_entry = entry;
    disabled_entry.enabled = false;
    const auto disabled = desk::apply_autostart(disabled_entry, options);
    require(disabled.ok, "Linux disabled autostart write should succeed");
    queried = desk::query_autostart(entry, options);
    require(queried.ok, "Linux disabled autostart query should succeed");
    require(!queried.enabled, "Linux Hidden=true autostart file should query as disabled");

    const auto removed = desk::remove_autostart(entry, options);
    require(removed.ok, "Linux autostart remove should succeed");
    require(!std::filesystem::exists(*applied.path), "Linux autostart remove should delete the desktop file");
#endif
}

void autostart_global_write_requires_permission()
{
    auto entry = autostart_entry_for_tests();
    entry.user_scope = false;

    desk::apply_options options;
    options.allow_desktop_integration_write = true;
    options.autostart_directory_override = test_root() / "autostart";

    const auto report = desk::apply_autostart(entry, options);
    require(!report.ok, "global autostart write should be denied without permission");
    require(has_diagnostic(report.diagnostics, "autostart-global-write-denied"),
        "global autostart write should require allow_global_write");
}

desk::policy_entry policy_entry_for_tests()
{
    desk::policy_entry entry;
    entry.id = "settings-tests-theme";
    entry.schema_id = "org.linuxdesktop2026.settings-tests";
    entry.key = "theme";
    entry.value = "'dark'";
    return entry;
}

void policy_write_requires_explicit_permission()
{
    auto entry = policy_entry_for_tests();

    desk::apply_options options;
    options.policy_defaults_directory_override = test_root() / "dconf" / "defaults";

    const auto report = desk::apply_policy(entry, options);
    require(!report.ok, "policy write should be denied without permission");
    require(has_diagnostic(report.diagnostics, "policy-write-denied"),
        "policy write should require allow_policy_write");
}

void policy_global_write_requires_permission()
{
    auto entry = policy_entry_for_tests();
    entry.user_scope = false;

    desk::apply_options options;
    options.allow_policy_write = true;
    options.policy_defaults_directory_override = test_root() / "dconf" / "defaults";

    const auto report = desk::apply_policy(entry, options);
    require(!report.ok, "global policy write should be denied without permission");
    require(has_diagnostic(report.diagnostics, "policy-global-write-denied"),
        "global policy write should require allow_global_write");
}

void policy_dry_run_does_not_write()
{
    auto entry = policy_entry_for_tests();
    entry.user_scope = true;

    const auto root = test_root() / "dconf" / "defaults";
    desk::apply_options options;
    options.allow_policy_write = true;
    options.policy_defaults_directory_override = root;

    const auto report = desk::apply_policy(entry, options);
    require(report.ok, "policy dry-run should succeed");
    require(report.dry_run, "policy dry-run should report dry_run");
    require(report.present, "policy dry-run should report planned presence");
    require(report.path.has_value(), "policy dry-run should report defaults path");
    require(!std::filesystem::exists(*report.path), "policy dry-run should not write a file");
    require(has_diagnostic(report.diagnostics, "policy-dry-run"),
        "policy dry-run should include a dry-run diagnostic");
}

void policy_linux_writes_queries_and_removes_dconf_files()
{
#if !defined(_WIN32)
    auto entry = policy_entry_for_tests();
    entry.enforced = true;
    entry.user_scope = true;

    const auto root = test_root() / "dconf";
    desk::apply_options options;
    options.dry_run = false;
    options.allow_policy_write = true;
    options.policy_defaults_directory_override = root / "defaults";
    options.policy_locks_directory_override = root / "locks";

    const auto applied = desk::apply_policy(entry, options);
    require(applied.ok, "Linux policy write should succeed");
    require(applied.path.has_value(), "Linux policy write should report defaults path");
    require(read_file(*applied.path).find("[org/linuxdesktop2026/settings-tests]") != std::string::npos,
        "Linux policy file should include dconf group");
    require(read_file(*applied.path).find("theme='dark'") != std::string::npos,
        "Linux policy file should include GVariant-ready value");

    const auto queried = desk::query_policy(entry, options);
    require(queried.ok, "Linux policy query should succeed");
    require(queried.present, "Linux policy query should report present value");
    require(queried.enforced, "Linux policy query should report lock file");
    require(queried.value.has_value() && *queried.value == "'dark'", "Linux policy query should return value literal");

    const auto removed = desk::remove_policy(entry, options);
    require(removed.ok, "Linux policy removal should succeed");
    require(!std::filesystem::exists(*applied.path), "Linux policy removal should remove defaults file");

    const auto queried_after_remove = desk::query_policy(entry, options);
    require(queried_after_remove.ok, "Linux policy query after removal should succeed");
    require(!queried_after_remove.present, "Linux policy query after removal should report absent value");
#endif
}

void c_abi_resolves_settings_override()
{
    const auto root = test_root() / "c-override";

    ld_settings_root_options options = {};
    ld_settings_root_options_init(&options);
    options.organization = "LinuxDesktop2026";
    options.application = "c-settings-tests";
    const auto root_text = path_to_utf8_string(root);
    options.settings_override = root_text.c_str();
    ld_settings_root_report report = {};
    const int ok = ld_settings_resolve_app_roots(&options, &report);

    require(ok == 1, "C ABI root resolution should succeed");
    require(report.settings_override_active == 1, "C ABI settings override should be active");
    require(report.config != nullptr, "C ABI config path should be allocated");
    require(std::filesystem::path(report.config) == root, "C ABI config path should match override");
    require(std::filesystem::path(report.session) == root / "sessions", "C ABI session path should match override");
    require(report.config_layer_count >= 6, "C ABI should expose config layer candidates");
    require(report.active_write_layer != nullptr, "C ABI should expose active write layer");
    require(std::string(ld_settings_severity_name(LD_SETTINGS_SEVERITY_WARNING)) == "warning",
        "C ABI severity names should be stable");
    require(ld_settings_version_major() == LD_SETTINGS_VERSION_MAJOR,
        "C ABI runtime major should match header major");
    require(ld_settings_version_minor() == LD_SETTINGS_VERSION_MINOR,
        "C ABI runtime minor should match header minor");
    require(ld_settings_version_patch() == LD_SETTINGS_VERSION_PATCH,
        "C ABI runtime patch should match header patch");
    require(std::string(ld_settings_version_string()) == "0.1.0",
        "C ABI version string should match project version");

    ld_settings_free_root_report(&report);

    require(report.config == nullptr, "C ABI free should clear report");
    require(report.diagnostic_count == 0, "C ABI free should clear diagnostics");
}

void c_abi_root_resolution_accepts_injected_environment()
{
    const auto root = test_root();
    const auto home = root / "home";
#if defined(_WIN32)
    const auto config = root / "appdata" / "roaming";
    const auto local = root / "appdata" / "local";
    const auto config_text = path_to_utf8_string(config);
    const auto local_text = path_to_utf8_string(local);
#else
    const auto config = root / "xdg-config";
    const auto config_text = path_to_utf8_string(config);
#endif
    const auto home_text = path_to_utf8_string(home);

    ld_settings_environment_entry environment[2] = {};
#if defined(_WIN32)
    environment[0].name = "APPDATA";
    environment[0].value = config_text.c_str();
    environment[1].name = "LOCALAPPDATA";
    environment[1].value = local_text.c_str();
#else
    environment[0].name = "XDG_CONFIG_HOME";
    environment[0].value = config_text.c_str();
#endif

    ld_settings_root_options options = {};
    ld_settings_root_options_init(&options);
    options.organization = "LinuxDesktop2026";
    options.application = "c-settings-tests";
    options.home_directory = home_text.c_str();
    options.environment = environment;
#if defined(_WIN32)
    options.environment_count = 2;
#else
    options.environment_count = 1;
#endif
    options.use_process_environment = 0;
    options.create_directories = 0;

    ld_settings_root_report report = {};
    const int ok = ld_settings_resolve_app_roots(&options, &report);

    require(ok == 1, "C ABI injected environment root resolution should succeed");
    require(report.config != nullptr, "C ABI injected environment should allocate config path");
#if defined(_WIN32)
    require(std::filesystem::path(report.config) == config / "LinuxDesktop2026" / "c-settings-tests",
        "C ABI config path should use injected APPDATA");
#else
    require(std::filesystem::path(report.config) == config / "LinuxDesktop2026" / "c-settings-tests",
        "C ABI config path should use injected XDG_CONFIG_HOME");
#endif

    ld_settings_free_root_report(&report);
}

} // namespace

int main()
{
    const std::vector<std::pair<const char*, void (*)()>> tests = {
        {"exposes_cpp_version", exposes_cpp_version},
        {"settings_diagnostics_use_shared_core_vocabulary", settings_diagnostics_use_shared_core_vocabulary},
        {"writes_absolute_settings_override", writes_absolute_settings_override},
        {"rejects_relative_settings_override", rejects_relative_settings_override},
        {"denies_portable_under_privileged_install_root", denies_portable_under_privileged_install_root},
        {"sync_config_override_keeps_state_local", sync_config_override_keeps_state_local},
        {"rejects_relative_sync_config_override", rejects_relative_sync_config_override},
        {"settings_override_wins_over_sync_override", settings_override_wins_over_sync_override},
        {"delegates_generic_roots_to_paths_with_injected_environment", delegates_generic_roots_to_paths_with_injected_environment},
        {"delegates_platform_defaults_to_paths", delegates_platform_defaults_to_paths},
        {"reports_path_directory_failure_for_generic_root_creation", reports_path_directory_failure_for_generic_root_creation},
        {"resolves_settings_layers", resolves_settings_layers},
        {"root_builder_preserves_root_options", root_builder_preserves_root_options},
#if defined(_WIN32)
        {"windows_default_roots_are_resolved", windows_default_roots_are_resolved},
#endif
        {"ensure_config_defaults_copies_missing_models", ensure_config_defaults_copies_missing_models},
        {"legacy_hydrate_config_bundle_forwards_to_config_defaults", legacy_hydrate_config_bundle_forwards_to_config_defaults},
        {"common_config_write_replaces_target_with_backup", common_config_write_replaces_target_with_backup},
        {"common_config_write_validation_keeps_original_target", common_config_write_validation_keeps_original_target},
        {"direct_write_validation_restores_backup", direct_write_validation_restores_backup},
        {"common_config_write_reports_durable_mode_and_keeps_backup", common_config_write_reports_durable_mode_and_keeps_backup},
        {"common_config_write_restores_backup_after_readback_mismatch", common_config_write_restores_backup_after_readback_mismatch},
        {"migration_plan_is_dry_run_first", migration_plan_is_dry_run_first},
        {"migration_execute_copies_file", migration_execute_copies_file},
        {"migration_blocks_dangerous_without_permission", migration_blocks_dangerous_without_permission},
        {"registry_reports_unsupported_on_linux", registry_reports_unsupported_on_linux},
        {"registry_json_snapshot_round_trips", registry_json_snapshot_round_trips},
        {"registry_reg_snapshot_round_trips", registry_reg_snapshot_round_trips},
        {"registry_import_requires_explicit_permission", registry_import_requires_explicit_permission},
        {"registry_json_rejects_hostile_import_shapes", registry_json_rejects_hostile_import_shapes},
        {"registry_reg_rejects_hostile_import_shapes", registry_reg_rejects_hostile_import_shapes},
        {"common_config_write_rejects_parent_that_is_file", common_config_write_rejects_parent_that_is_file},
        {"common_config_write_cleans_temp_after_backup_copy_failure", common_config_write_cleans_temp_after_backup_copy_failure},
        {"autostart_dry_run_does_not_write", autostart_dry_run_does_not_write},
        {"autostart_linux_writes_queries_and_removes_desktop_file", autostart_linux_writes_queries_and_removes_desktop_file},
        {"autostart_global_write_requires_permission", autostart_global_write_requires_permission},
        {"policy_write_requires_explicit_permission", policy_write_requires_explicit_permission},
        {"policy_global_write_requires_permission", policy_global_write_requires_permission},
        {"policy_dry_run_does_not_write", policy_dry_run_does_not_write},
        {"policy_linux_writes_queries_and_removes_dconf_files", policy_linux_writes_queries_and_removes_dconf_files},
        {"c_abi_resolves_settings_override", c_abi_resolves_settings_override},
        {"c_abi_root_resolution_accepts_injected_environment", c_abi_root_resolution_accepts_injected_environment},
    };

    int failures = 0;
    for (const auto& test : tests) {
        try {
            test.second();
            std::cout << "ok " << test.first << "\n";
        } catch (const test_failure& failure) {
            ++failures;
            std::cout << "not ok " << test.first << ": " << failure.message << "\n";
        } catch (const std::exception& ex) {
            ++failures;
            std::cout << "not ok " << test.first << ": unexpected exception: " << ex.what() << "\n";
        }
    }

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
