#include "ctrlrx_flavor.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace ctrlrx = flavor_tests::ctrlrx;

namespace {

int failures = 0;

void expect(bool condition, const std::string& name)
{
    if (condition) {
        std::cout << "ok " << name << '\n';
    } else {
        std::cout << "not ok " << name << '\n';
        ++failures;
    }
}

ctrlrx::RuntimeEnvironment default_env()
{
    const auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-ctrlrx-flavor";
    std::filesystem::remove_all(root);
    ctrlrx::RuntimeEnvironment env;
    env.home_directory = root / "home" / "alice";
    env.runtime_directory = root / "run" / "user" / "1000";
    env.executable_directory = root / "opt" / "CtrlrX";
    std::filesystem::create_directories(env.executable_directory / "Resources" / "Panels");
    std::filesystem::create_directories(env.executable_directory / "Resources" / "Lua");
    return env;
}

void standalone_preferences_are_written_to_user_config()
{
    const auto env = default_env();
    ctrlrx::Preferences preferences;
    preferences.auto_save_interval_minutes = 9;

    const auto saved = ctrlrx::CtrlrSettings{}.savePreferences(
        env,
        ctrlrx::InstanceKind::Standalone,
        preferences);

    expect(saved.saved, "standalone preferences are saved");
    expect(saved.preferences_file.filename() == "Ctrlr.settings", "preferences file keeps Ctrlr settings name");
    expect(std::filesystem::exists(saved.preferences_file), "preferences file exists after save");
}

void plugin_instance_does_not_write_global_preferences()
{
    const auto env = default_env();

    const auto saved = ctrlrx::CtrlrSettings{}.savePreferences(
        env,
        ctrlrx::InstanceKind::PluginExport,
        {});

    expect(!saved.saved, "plugin instance does not update standalone preferences");
    expect(!std::filesystem::exists(saved.preferences_file), "plugin instance leaves global settings untouched");
}

void resource_reload_keeps_juce_resource_policy_in_product_code()
{
    const auto env = default_env();

    const auto plan = ctrlrx::CtrlrSettings{}.reloadResources(env);

    expect(plan.resource_root == env.executable_directory / "Resources", "resources come from app resource root");
    expect(plan.panel_search_roots.front() == env.executable_directory / "Resources" / "Panels",
        "panel reload checks shipped panels first");
    expect(plan.lua_search_roots.front() == env.executable_directory / "Resources" / "Lua",
        "lua reload checks shipped scripts first");
}

void plugin_exports_use_plugin_paths_without_owning_audio_formats()
{
    auto env = default_env();
    const auto vst3_root = *env.home_directory / "custom-vst3";
    env.environment["VST3_PATH"] = vst3_root.string();

    const auto plan = ctrlrx::CtrlrSettings{}.planPluginExport(env, ctrlrx::PluginFormat::Vst3);

    expect(plan.output_root == vst3_root, "VST3 export root follows plugin path environment");
    expect(plan.intermediate_project.filename() == "CtrlrX.jucer", "plugin export keeps jucer intermediate");
    expect(plan.replace_with_panel_ids, "panel id replacement remains CtrlrX export policy");
}

} // namespace

int main()
{
    standalone_preferences_are_written_to_user_config();
    plugin_instance_does_not_write_global_preferences();
    resource_reload_keeps_juce_resource_policy_in_product_code();
    plugin_exports_use_plugin_paths_without_owning_audio_formats();
    return failures == 0 ? 0 : 1;
}
