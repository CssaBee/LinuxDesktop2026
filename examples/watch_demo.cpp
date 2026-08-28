#include "linuxdesktop/watch.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

struct cli_options {
    std::filesystem::path directory;
    std::optional<std::filesystem::path> file;
    int seconds = 10;
};

void print_usage(const char* program)
{
    std::cout
        << "Usage: " << program << " --dir PATH [--file PATH] [--seconds N]\n"
        << "\n"
        << "Runs the LinuxDesktop2026 file watcher prototype demo.\n";
}

cli_options parse_args(int argc, char** argv)
{
    cli_options options;
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
        if (arg == "--dir") {
            options.directory = require_value("--dir");
            continue;
        }
        if (arg == "--file") {
            options.file = require_value("--file");
            continue;
        }
        if (arg == "--seconds") {
            options.seconds = std::stoi(require_value("--seconds"));
            continue;
        }
        throw std::runtime_error("Unknown argument: " + arg);
    }
    if (options.directory.empty()) {
        throw std::runtime_error("--dir is required");
    }
    return options;
}

void print_diagnostics(const std::vector<linuxdesktop::diagnostic>& diagnostics)
{
    for (const auto& item : diagnostics) {
        std::cout << "  diagnostic " << linuxdesktop::to_string(item.level)
                  << " " << item.code << ": " << item.message;
        if (!item.path.empty()) {
            std::cout << " [" << item.path.string() << "]";
        }
        std::cout << "\n";
    }
}

void print_event(const linuxdesktop::watch::watch_event& event)
{
    namespace ld = linuxdesktop::watch;
    std::cout << ld::to_string(event.kind) << " "
              << ld::to_string(event.path.type) << " "
              << event.path.absolute.string();
    if (event.path.root_relative) {
        std::cout << " relative=" << event.path.root_relative->string();
    }
    if (event.old_path) {
        std::cout << " old=" << event.old_path->absolute.string();
    }
    if (!event.caller_tag.empty()) {
        std::cout << " tag=" << event.caller_tag;
    }
    if (event.rescan_recommended) {
        std::cout << " rescan=recommended";
    }
    std::cout << " state=" << ld::to_string(event.state) << "\n";
    print_diagnostics(event.diagnostics);
}

} // namespace

int main(int argc, char** argv)
{
    try {
        namespace ld = linuxdesktop::watch;
        const auto cli = parse_args(argc, argv);

        ld::watcher watcher;
        watcher.set_callback(print_event);

        ld::watch_options directory;
        directory.path = cli.directory;
        directory.caller_tag = "directory";
        directory.recursive = ld::recursive_policy::none;
        const auto directory_report = watcher.add_watch(directory);
        std::cout << "directory watch: " << (directory_report.ok ? "ok" : "failed") << "\n";
        print_diagnostics(directory_report.diagnostics);

        if (cli.file) {
            ld::watch_options file;
            file.path = *cli.file;
            file.caller_tag = "file";
            file.settle = ld::settle_options{
                std::chrono::milliseconds{100},
                std::chrono::milliseconds{250},
                std::chrono::milliseconds{50}};
            const auto file_report = watcher.add_watch(file);
            std::cout << "file watch: " << (file_report.ok ? "ok" : "failed") << "\n";
            print_diagnostics(file_report.diagnostics);
        }

        std::this_thread::sleep_for(std::chrono::seconds(cli.seconds));
        watcher.stop();
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
