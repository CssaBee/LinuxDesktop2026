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
    auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-watch-libuv-tests";
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

    ld::watch_event wait_for_path(const std::filesystem::path& absolute)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        const bool found = cv_.wait_for(lock, std::chrono::seconds(3), [&] {
            for (const auto& event : events_) {
                if (event.path.absolute == absolute) {
                    return true;
                }
            }
            return false;
        });
        if (!found) {
            fail("timed out waiting for path " + absolute.string());
        }
        for (auto it = events_.begin(); it != events_.end(); ++it) {
            if (it->path.absolute == absolute) {
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

void libuv_backend_reports_capability_and_events()
{
    const auto root = test_root();
    event_collector collector;

    ld::watcher watcher;
    watcher.set_callback([&](const ld::watch_event& event) {
        collector.push(event);
    });

    const auto capabilities = watcher.capabilities();
    require(has_diagnostic(capabilities.diagnostics, "watch.backend.libuv"), "preferred backend should be libuv");

    ld::watch_options options;
    options.path = root;
    options.caller_tag = "libuv-smoke";
    const auto report = watcher.add_watch(options);
    require(report.ok, "libuv directory watch should start");

    const auto file = root / "created.txt";
    writes_file(file, "one");
    const auto event = collector.wait_for_path(file);
    require(event.caller_tag == "libuv-smoke", "libuv event should echo caller tag");
    require(event.path.root.value == report.id.value, "libuv path should carry watch id");
}

} // namespace

int main()
{
    try {
        libuv_backend_reports_capability_and_events();
    } catch (const test_failure& failure) {
        std::cerr << failure.message << "\n";
        return EXIT_FAILURE;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
