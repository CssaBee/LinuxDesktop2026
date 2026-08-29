#include "obs_flavor.hpp"

#include "linuxdesktop/paths.hpp"

#include <cstring>
#include <utility>

namespace flavor_tests::obs {

namespace ldp = linuxdesktop::paths;
namespace lds = linuxdesktop::settings;

namespace {

int copy_path(char* dst, std::size_t size, const std::filesystem::path& path)
{
    const auto text = path.string();
    if (!dst || size == 0 || text.size() + 1 > size) {
        return -1;
    }
    std::memcpy(dst, text.c_str(), text.size() + 1);
    return static_cast<int>(text.size());
}

bool config_has_section(const std::filesystem::path&, std::string& message)
{
    message.clear();
    return true;
}

} // namespace

Platform::Platform(RuntimeEnvironment environment)
    : environment_(std::move(environment))
{
}

int Platform::os_get_config_path(char* dst, std::size_t size, const char* name) const
{
    auto path = resolve_config_root();
    if (name && *name) {
        path /= name;
    }
    return copy_path(dst, size, path);
}

std::string Platform::obs_module_get_config_path(const std::string& module, const std::string& file) const
{
    return (resolve_config_root() / "plugin_config" / module / file).string();
}

int Platform::config_save_safe(const std::filesystem::path& path, const std::string& content) const
{
    lds::write_options write;
    write.target = path;
    write.content = content;
    write.keep_backup = true;
    write.atomic_replace = true;
    write.durable_write = true;
    return lds::write_with_backup(write, config_has_section).ok ? 0 : -1;
}

std::filesystem::path Platform::resolve_config_root() const
{
    ldp::app_identity identity;
    identity.organization = "obsproject";
    identity.application = "obs-studio";

    ldp::resolver_options options;
    options.home_directory = environment_.home_directory;
    options.environment = environment_.variables;
    options.use_process_environment = false;
    if (const auto config_home = environment_.variables.find("XDG_CONFIG_HOME");
        config_home != environment_.variables.end()) {
        options.config_override = std::filesystem::path(config_home->second) / "obs-studio";
    }
    return ldp::resolve_app_paths(identity, options).selected.at(ldp::path_family::config);
}

} // namespace flavor_tests::obs
