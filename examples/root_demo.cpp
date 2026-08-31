#include "linuxdesktop/root.hpp"

#include <filesystem>
#include <iostream>

int main()
{
    linuxdesktop::root::options options;
    options.create_directories = false;
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
    if (const auto* logs = linuxdesktop::root::find_named_root(report, "logs")) {
        std::cout << "logs: " << logs->path << "\n";
    }
    return report.roots.config.empty() ? 1 : 0;
}
