#include "linuxdesktop/root.hpp"

#include <filesystem>
#include <iostream>
#include <string_view>
#include <utility>

int main(int argc, char** argv)
{
    linuxdesktop::root::options options;
    options.create_directories = false;
    if (argc == 3 && std::string_view{argv[1]} == "--portable-root") {
        linuxdesktop::root::portable_root_request portable;
        portable.root = std::filesystem::path{argv[2]};
        portable.requested = true;
        options.portable_root = std::move(portable);
    }
    options.named_roots = {
        linuxdesktop::root::make_log_root_request(
            "logs",
            linuxdesktop::root::ownership_kind::user_local,
            "logs"),
    };

    const auto report = linuxdesktop::root::resolve_app_roots(
        {"LinuxDesktop2026", "root-demo"},
        options);

    std::cout << "config: " << report.roots.config << "\n";
    std::cout << "portable: " << (report.portable_root_active ? "active" : "inactive") << "\n";
    if (const auto* logs = linuxdesktop::root::find_named_root(report, "logs")) {
        std::cout << "logs: " << logs->path << "\n";
    }
    return report.roots.config.empty() ? 1 : 0;
}
