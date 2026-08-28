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
    auto root = std::filesystem::temp_directory_path() / "linuxdesktop2026-watch-inotify-tests";
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

    ld::watch_event wait_for_kind(ld::event_kind kind)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        const bool found = cv_.wait_for(lock, std::chrono::seconds(3), [&] {
            for (const auto& event : events_) {
                if (event.kind == kind) {
                    return true;
                }
            }
            return false;
        });
        if (!found) {
            fail("timed out waiting for event kind " + std::string(ld::to_string(kind)));
        }
        for (auto it = events_.begin(); it != events_.end(); ++it) {
            if (it->kind == kind) {
                auto matched = *it;
                events_.erase(it);
                return matched;
            }
        }
        fail("event kind disappeared");
    }

    ld::watch_event wait_for_path(ld::event_kind kind, const std::filesystem::path& absolute)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        const bool found = cv_.wait_for(lock, std::chrono::seconds(3), [&] {
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

    ld::watch_event wait_for_diagnostic(const std::string& code)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        const bool found = cv_.wait_for(lock, std::chrono::seconds(3), [&] {
            for (const auto& event : events_) {
                if (has_diagnostic(event.diagnostics, code)) {
                    return true;
                }
            }
            return false;
        });
        if (!found) {
            fail("timed out waiting for diagnostic " + code);
        }
        for (auto it = events_.begin(); it != events_.end(); ++it) {
            if (has_diagnostic(it->diagnostics, code)) {
                auto matched = *it;
                events_.erase(it);
                return matched;
            }
        }
        fail("event diagnostic disappeared");
    }

    ld::watch_event wait_for_path_and_tag(
        ld::event_kind kind,
        const std::filesystem::path& absolute,
        const std::string& caller_tag)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        const bool found = cv_.wait_for(lock, std::chrono::seconds(3), [&] {
            for (const auto& event : events_) {
                if (event.kind == kind && event.path.absolute == absolute && event.caller_tag == caller_tag) {
                    return true;
                }
            }
            return false;
        });
        if (!found) {
            fail("timed out waiting for path " + absolute.string() + " and tag " + caller_tag);
        }
        for (auto it = events_.begin(); it != events_.end(); ++it) {
            if (it->kind == kind && it->path.absolute == absolute && it->caller_tag == caller_tag) {
                auto matched = *it;
                events_.erase(it);
                return matched;
            }
        }
        fail("event path/tag disappeared");
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

void native_directory_watch_reports_basic_events()
{
    const auto root = test_root();
    event_collector collector;
    ld::watcher watcher;
    watcher.set_callback([&](const ld::watch_event& event) {
        collector.push(event);
    });

    ld::watch_options options;
    options.path = root;
    options.caller_tag = "native-dir";
    const auto report = watcher.add_watch(options);
    require(report.ok, "native directory watch should start");

    const auto file = root / "created.txt";
    writes_file(file, "one");
    auto event = collector.wait_for_path(ld::event_kind::created, file);
    require(event.caller_tag == "native-dir", "native event should echo caller tag");
    require(event.path.absolute == file, "native create should preserve absolute path");

    writes_file(file, "two");
    event = collector.wait_for_kind(ld::event_kind::modified);
    require(event.path.absolute == file, "native modify should preserve absolute path");

    std::filesystem::remove(file);
    event = collector.wait_for_kind(ld::event_kind::removed);
    require(event.path.absolute == file, "native remove should preserve absolute path");
}

void native_rename_pairs_when_cookie_matches()
{
    const auto root = test_root();
    event_collector collector;
    ld::watcher watcher;
    watcher.set_callback([&](const ld::watch_event& event) {
        collector.push(event);
    });

    ld::watch_options options;
    options.path = root;
    const auto report = watcher.add_watch(options);
    require(report.ok, "native rename watch should start");

    const auto before = root / "before.txt";
    const auto after = root / "after.txt";
    writes_file(before, "rename");
    collector.wait_for_kind(ld::event_kind::created);
    std::filesystem::rename(before, after);

    const auto event = collector.wait_for_kind(ld::event_kind::renamed_new);
    require(event.path.absolute == after, "rename destination should be reported");
    require(event.paired_rename, "rename destination should be paired when inotify cookie matches");
    require(event.old_path.has_value(), "rename destination should carry old path");
    require(event.old_path->absolute == before, "rename source should be reported");
}

void native_single_file_watch_handles_save_by_replace()
{
    const auto root = test_root();
    const auto file = root / "watched.txt";
    writes_file(file, "old");

    event_collector collector;
    ld::watcher watcher;
    watcher.set_callback([&](const ld::watch_event& event) {
        collector.push(event);
    });

    ld::watch_options options;
    options.path = file;
    options.caller_tag = "single-file";
    const auto report = watcher.add_watch(options);
    require(report.ok, "native single-file watch should start");

    const auto temp = root / "watched.txt.tmp";
    writes_file(temp, "new");
    std::filesystem::rename(temp, file);

    const auto event = collector.wait_for_kind(ld::event_kind::renamed_new);
    require(event.caller_tag == "single-file", "single-file event should echo caller tag");
    require(event.path.absolute == file, "single-file replace should report target file");
}

void native_recursive_policy_is_honest()
{
    const auto root = test_root();
    ld::watcher watcher;

    ld::watch_options options;
    options.path = root;
    options.recursive = ld::recursive_policy::native_if_supported;
    const auto report = watcher.add_watch(options);
    require(!report.ok, "inotify native recursive request should fail");
    require(has_diagnostic(report.diagnostics, "watch.recursive.unsupported"),
        "inotify native recursive request should diagnose unsupported policy");
}

void native_recursive_emulation_expands_new_directories()
{
    const auto root = test_root();
    event_collector collector;
    ld::watcher watcher;
    watcher.set_callback([&](const ld::watch_event& event) {
        collector.push(event);
    });

    ld::watch_options options;
    options.path = root;
    options.recursive = ld::recursive_policy::emulate;
    const auto report = watcher.add_watch(options);
    require(report.ok, "native recursive emulation should start");
    require(has_diagnostic(report.diagnostics, "watch.recursive.emulated"),
        "native recursive emulation should be explicit");

    const auto child = root / "child";
    std::filesystem::create_directory(child);
    auto event = collector.wait_for_path(ld::event_kind::created, child);
    require(event.path.absolute == child, "new recursive child directory should be reported");
    require(event.path.root_relative == std::filesystem::path("child"), "child directory should be root-relative");

    const auto nested = child / "nested.txt";
    writes_file(nested, "inside");
    event = collector.wait_for_path(ld::event_kind::created, nested);
    require(event.path.absolute == nested, "new file under discovered directory should be reported");
    require(event.path.root_relative == std::filesystem::path("child/nested.txt"),
        "new file under discovered directory should be root-relative");
}

void native_recursive_emulation_skips_symlinked_directories()
{
    const auto root = test_root();
    const auto target = root / "target";
    std::filesystem::create_directory(target);
    const auto linked = root / "linked";
    std::error_code ec;
    std::filesystem::create_directory_symlink(target, linked, ec);
    if (ec) {
        return;
    }

    ld::watcher watcher;
    ld::watch_options options;
    options.path = root;
    options.recursive = ld::recursive_policy::emulate;
    const auto report = watcher.add_watch(options);
    require(report.ok, "recursive emulation should start with a symlinked directory present");
    require(has_diagnostic(report.diagnostics, "watch.recursive.symlink_skipped"),
        "recursive emulation should diagnose skipped symlinked directories");
}

void native_recursive_emulation_skips_duplicate_existing_directories()
{
    const auto root = test_root();
    const auto child = root / "child";
    std::filesystem::create_directory(child);

    ld::watcher watcher;
    ld::watch_options options;
    options.path = root;
    options.recursive = ld::recursive_policy::emulate;
    const auto report = watcher.add_watch(options);
    require(report.ok, "recursive emulation should start with existing child directories");
    require(!has_diagnostic(report.diagnostics, "watch.recursive.duplicate_skipped"),
        "initial recursive scan should not report duplicate directories");
}

void native_same_directory_watches_fan_out_and_remove_independently()
{
    const auto root = test_root();
    event_collector collector;
    ld::watcher watcher;
    watcher.set_callback([&](const ld::watch_event& event) {
        collector.push(event);
    });

    ld::watch_options first;
    first.path = root;
    first.caller_tag = "first";
    const auto first_report = watcher.add_watch(first);
    require(first_report.ok, "first directory watch should start");

    ld::watch_options second;
    second.path = root;
    second.caller_tag = "second";
    const auto second_report = watcher.add_watch(second);
    require(second_report.ok, "second directory watch should start");

    const auto shared = root / "shared.txt";
    writes_file(shared, "one");
    collector.wait_for_path_and_tag(ld::event_kind::created, shared, "first");
    collector.wait_for_path_and_tag(ld::event_kind::created, shared, "second");

    require(watcher.remove_watch(first_report.id), "removing first watch should succeed");
    const auto after_remove = root / "after-remove.txt";
    writes_file(after_remove, "two");
    collector.wait_for_path_and_tag(ld::event_kind::created, after_remove, "second");
}

} // namespace

int main()
{
#if defined(__linux__)
    try {
        native_directory_watch_reports_basic_events();
        native_rename_pairs_when_cookie_matches();
        native_single_file_watch_handles_save_by_replace();
        native_recursive_policy_is_honest();
        native_recursive_emulation_expands_new_directories();
        native_recursive_emulation_skips_symlinked_directories();
        native_recursive_emulation_skips_duplicate_existing_directories();
        native_same_directory_watches_fan_out_and_remove_independently();
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
