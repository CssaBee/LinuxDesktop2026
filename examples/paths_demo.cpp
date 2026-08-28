#include "linuxdesktop/paths.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void print_usage(const char* program)
{
    std::cout
        << "Usage: " << program << " [--org NAME] [--app NAME]\n"
        << "\n"
        << "Prints the LinuxDesktop2026 path resolver prototype report.\n";
}

linuxdesktop::paths::app_identity parse_args(int argc, char** argv)
{
    linuxdesktop::paths::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "paths-demo";

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto require_value = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error(std::string("Missing value for ") + name);
            }
            return argv[++i];
        };

        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            std::exit(0);
        }
        if (arg == "--org") {
            identity.organization = require_value("--org");
            continue;
        }
        if (arg == "--app") {
            identity.application = require_value("--app");
            continue;
        }
        throw std::runtime_error("Unknown argument: " + arg);
    }

    return identity;
}

void print_diagnostics(const std::vector<linuxdesktop::diagnostic>& diagnostics)
{
    for (const auto& item : diagnostics) {
        std::cout << "diagnostic " << linuxdesktop::to_string(item.level)
                  << " " << item.code << ": " << item.message;
        if (!item.path.empty()) {
            std::cout << " [" << item.path.string() << "]";
        }
        std::cout << "\n";
    }
}

void print_report(const linuxdesktop::paths::resolver_report& report)
{
    namespace ld = linuxdesktop::paths;

    std::cout << "selected paths: " << report.selected.size() << "\n";
    for (const auto& item : report.selected) {
        std::cout << "  " << ld::to_string(item.first) << ": " << item.second.string() << "\n";
    }

    std::cout << "candidates: " << report.candidates.size() << "\n";
    for (const auto& candidate : report.candidates) {
        std::cout << "  " << (candidate.selected ? "*" : "-")
                  << " " << ld::to_string(candidate.family)
                  << " via " << ld::to_string(candidate.source);
        if (!candidate.path.empty()) {
            std::cout << ": " << candidate.path.string();
        }
        std::cout << "\n";
        print_diagnostics(candidate.diagnostics);
    }
}

void print_plugin_sets(const linuxdesktop::paths::plugin_path_report& report)
{
    namespace ld = linuxdesktop::paths;

    std::cout << "plugin path sets: " << report.sets.size() << "\n";
    for (const auto& set : report.sets) {
        std::cout << "  " << set.name << ": " << set.paths.size() << " roots\n";
        for (const auto& path : set.paths) {
            std::cout << "    " << path.string() << "\n";
        }
    }

    std::cout << "plugin candidates: " << report.candidates.size() << "\n";
    for (const auto& candidate : report.candidates) {
        std::cout << "  " << (candidate.selected ? "*" : "-")
                  << " " << ld::to_string(candidate.source);
        if (!candidate.path.empty()) {
            std::cout << ": " << candidate.path.string();
        }
        std::cout << "\n";
    }
    print_diagnostics(report.diagnostics);
}

} // namespace

int main(int argc, char** argv)
{
    try {
        namespace ld = linuxdesktop::paths;
        const auto identity = parse_args(argc, argv);
        const auto report = ld::resolve_app_paths(identity);

        std::cout << "ld_paths report for "
                  << identity.organization << "/" << identity.application << "\n";
        print_report(report);
        print_diagnostics(report.diagnostics);

        ld::plugin_path_options plugin_options;
        plugin_options.kinds = {ld::plugin_path_kind::lv2, ld::plugin_path_kind::vst3, ld::plugin_path_kind::clap};
        plugin_options.include_wine_prefix_defaults = true;
        const auto plugins = ld::resolve_plugin_path_sets(plugin_options);
        print_plugin_sets(plugins);
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
