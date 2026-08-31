#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace flavor_tests::ctrlrx {

enum class InstanceKind {
    Standalone,
    PluginExport
};

enum class PluginFormat {
    Vst3,
    AudioUnit,
    Aax
};

struct RuntimeEnvironment {
    std::optional<std::filesystem::path> home_directory;
    std::optional<std::filesystem::path> runtime_directory;
    std::filesystem::path executable_directory;
    std::map<std::string, std::string> environment;
};

struct Preferences {
    bool auto_save = true;
    int auto_save_interval_minutes = 5;
    std::string default_look_and_feel = "V4";
};

struct PreferencesSaveResult {
    bool saved = false;
    std::optional<std::filesystem::path> backup_file;
    std::filesystem::path preferences_file;
};

struct ResourceReloadPlan {
    std::filesystem::path resource_root;
    std::vector<std::filesystem::path> panel_search_roots;
    std::vector<std::filesystem::path> lua_search_roots;
};

struct PluginExportPlan {
    PluginFormat format = PluginFormat::Vst3;
    std::filesystem::path output_root;
    std::filesystem::path intermediate_project;
    bool replace_with_panel_ids = true;
};

class CtrlrSettings {
public:
    PreferencesSaveResult savePreferences(
        const RuntimeEnvironment& environment,
        InstanceKind kind,
        const Preferences& preferences) const;

    ResourceReloadPlan reloadResources(const RuntimeEnvironment& environment) const;
    PluginExportPlan planPluginExport(const RuntimeEnvironment& environment, PluginFormat format) const;
};

} // namespace flavor_tests::ctrlrx
