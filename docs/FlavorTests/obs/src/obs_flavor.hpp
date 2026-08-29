#pragma once

#include "linuxdesktop/paths.hpp"
#include "linuxdesktop/settings.hpp"

#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>
#include <string>

namespace flavor_tests::obs {

struct RuntimeEnvironment {
    std::optional<std::filesystem::path> home_directory;
    std::map<std::string, std::string> variables;
};

class Platform {
public:
    explicit Platform(RuntimeEnvironment environment);

    int os_get_config_path(char* dst, std::size_t size, const char* name) const;
    std::string obs_module_get_config_path(const std::string& module, const std::string& file) const;
    int config_save_safe(const std::filesystem::path& path, const std::string& content) const;

private:
    linuxdesktop::paths::resolver_report resolve() const;

    RuntimeEnvironment environment_;
};

} // namespace flavor_tests::obs
