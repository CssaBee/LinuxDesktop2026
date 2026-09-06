#include "endless_sky_flavor.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace endless_sky = flavor_tests::endless_sky;

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

endless_sky::RuntimeEnvironment default_env(const std::string& name)
{
    const auto root = std::filesystem::temp_directory_path() / ("linuxdesktop2026-" + name);
    std::filesystem::remove_all(root);
    endless_sky::RuntimeEnvironment env;
    env.home_directory = root / "home" / "alice";
    env.runtime_directory = root / "run" / "user" / "1000";
    env.executable_directory = root / "usr" / "games";
    env.resource_directory = root / "usr" / "share" / "games" / "endless-sky";
    return env;
}

void owned_plugin_roots_keep_bundled_and_user_plugins_separate()
{
    const auto env = default_env("endless-sky-plugins");

    const auto paths = endless_sky::Files{}.initialize(env);

    expect(paths.plugin_roots.size() == 2, "two plugin roots are exposed to the scanner");
    expect(paths.plugin_roots[0].name == "bundled", "bundled plugin root keeps product name");
    expect(paths.plugin_roots[0].path == env.resource_directory / "plugins",
        "bundled plugins stay resource-owned");
    expect(!paths.plugin_roots[0].user_writable, "bundled plugins are not user writable");
    expect(paths.plugin_roots[1].name == "local", "local plugin root keeps product name");
    expect(paths.plugin_roots[1].path == *env.home_directory / ".local" / "share" / "endless-sky" / "endless-sky" / "plugins",
        "local plugins resolve under the user data root");
    expect(paths.plugin_roots[1].user_writable, "local plugins are user writable");
}

void downloadable_plugins_use_the_owned_user_plugin_root()
{
    const auto env = default_env("endless-sky-downloads");

    const auto paths = endless_sky::Files{}.initialize(env);

    expect(paths.downloadable_plugin_root == paths.plugin_roots[1].path,
        "plugin downloads target the user-owned plugin root");
    expect(paths.plugin_errors_file == paths.downloadable_plugin_root / "errors.txt",
        "plugin diagnostics stay beside the user-owned plugin root");
}

void saves_and_preferences_stay_in_endless_sky_data_vocabulary()
{
    const auto env = default_env("endless-sky-saves");

    const auto paths = endless_sky::Files{}.initialize(env);

    expect(paths.config_root == *env.home_directory / ".local" / "share" / "endless-sky" / "endless-sky",
        "config root follows Endless Sky user data location");
    expect(paths.save_root == paths.config_root / "saves", "save root stays a product data child");
    expect(paths.preferences_file == paths.config_root / "preferences.txt",
        "preferences file keeps Endless Sky file name");
}

} // namespace

int main()
{
    owned_plugin_roots_keep_bundled_and_user_plugins_separate();
    downloadable_plugins_use_the_owned_user_plugin_root();
    saves_and_preferences_stay_in_endless_sky_data_vocabulary();
    return failures == 0 ? 0 : 1;
}
