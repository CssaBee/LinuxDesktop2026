#include "linuxdesktop/watch.hpp"

#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <system_error>
#include <vector>

namespace {

namespace ld = linuxdesktop::watch;

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

bool has_diagnostic(const std::vector<linuxdesktop::diagnostic>& diagnostics, const std::string& code)
{
    for (const auto& item : diagnostics) {
        if (item.code == code) {
            return true;
        }
    }
    return false;
}

std::filesystem::path test_root()
{
    auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-watch-windows-tests";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    if (ec) {
        fail("failed to create test root: " + ec.message());
    }
    return root;
}

class event_collector {
public:
    void push(const ld::watch_event& event)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            events_.push_back(event);
        }
        cv_.notify_all();
    }

    ld::watch_event wait_for_path(ld::event_kind kind, const std::filesystem::path& absolute)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        const bool found = cv_.wait_for(lock, std::chrono::seconds(5), [&] {
            for (const auto& event : events_) {
                if (event.kind == kind && event.path.absolute == absolute) {
                    return true;
                }
            }
            return false;
        });
        if (!found) {
            fail("timed out waiting for path " + absolute.string());
        }
        for (auto it = events_.begin(); it != events_.end(); ++it) {
            if (it->kind == kind && it->path.absolute == absolute) {
                auto matched = *it;
                events_.erase(it);
                return matched;
            }
        }
        fail("event path disappeared");
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<ld::watch_event> events_;
};

void writes_file(const std::filesystem::path& path, const std::string& content)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << content;
}

void windows_directory_watch_reports_basic_events()
{
    const auto root = test_root();
    event_collector collector;
    ld::watcher watcher;
    watcher.set_callback([&](const ld::watch_event& event) {
        collector.push(event);
    });

    ld::watch_options options;
    options.path = root;
    options.caller_tag = "windows-dir";
    const auto report = watcher.add_watch(options);
    require(report.ok, "Windows directory watch should start");
    require(has_diagnostic(report.capabilities.diagnostics, "watch.backend.windows"),
        "Windows backend should report its capability source");

    const auto file = root / "created.txt";
    writes_file(file, "one");
    auto event = collector.wait_for_path(ld::event_kind::created, file);
    require(event.caller_tag == "windows-dir", "Windows event should echo caller tag");
    require(event.path.root.value == report.id.value, "Windows path should carry watch id");

    writes_file(file, "two");
    collector.wait_for_path(ld::event_kind::modified, file);
}

void windows_rename_pairs_adjacent_old_and_new_names()
{
    const auto root = test_root();
    const auto before = root / "before.txt";
    const auto after = root / "after.txt";
    writes_file(before, "rename");

    event_collector collector;
    ld::watcher watcher;
    watcher.set_callback([&](const ld::watch_event& event) {
        collector.push(event);
    });

    ld::watch_options options;
    options.path = root;
    const auto report = watcher.add_watch(options);
    require(report.ok, "Windows rename watch should start");

    std::filesystem::rename(before, after);
    const auto event = collector.wait_for_path(ld::event_kind::renamed_new, after);
    require(event.paired_rename, "Windows rename destination should be paired");
    require(event.old_path.has_value(), "Windows rename destination should carry old path");
    require(event.old_path->absolute == before, "Windows rename source should be reported");
}

void windows_native_recursive_reports_nested_file()
{
    const auto root = test_root();
    event_collector collector;
    ld::watcher watcher;
    watcher.set_callback([&](const ld::watch_event& event) {
        collector.push(event);
    });

    ld::watch_options options;
    options.path = root;
    options.recursive = ld::recursive_policy::native_if_supported;
    const auto report = watcher.add_watch(options);
    require(report.ok, "Windows native recursive watch should start");
    require(report.capabilities.native_recursive, "Windows should report native recursive capability");
    require(has_diagnostic(report.diagnostics, "watch.recursive.native"),
        "Windows recursive watch should diagnose native recursion");

    const auto child = root / "child";
    std::filesystem::create_directory(child);
    const auto nested = child / "nested.txt";
    writes_file(nested, "inside");
    const auto event = collector.wait_for_path(ld::event_kind::created, nested);
    require(event.path.root_relative == std::filesystem::path("child") / "nested.txt",
        "Windows recursive event should be root-relative");
}

} // namespace

int main()
{
#if defined(_WIN32)
    try {
        windows_directory_watch_reports_basic_events();
        windows_rename_pairs_adjacent_old_and_new_names();
        windows_native_recursive_reports_nested_file();
    } catch (const test_failure& failure) {
        std::cerr << failure.message << "\n";
        return EXIT_FAILURE;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return EXIT_FAILURE;
    }
#endif

    return EXIT_SUCCESS;
}
