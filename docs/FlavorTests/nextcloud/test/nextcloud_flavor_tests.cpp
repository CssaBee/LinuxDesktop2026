#include "nextcloud_flavor.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace nextcloud = flavor_tests::nextcloud;

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

nextcloud::FilesystemSignal modified(const std::filesystem::path& path)
{
    return {"/sync", path, nextcloud::FileSignalKind::Modified, false};
}

linuxdesktop::watch::watch_event ld_modified(const std::filesystem::path& path)
{
    linuxdesktop::watch::watch_event event;
    event.kind = linuxdesktop::watch::event_kind::modified;
    event.path.absolute = "/sync";
    event.path.absolute /= path;
    event.path.root_relative = path;
    return event;
}

nextcloud::FileStateSnapshot state(std::uintmax_t size, std::string version)
{
    return {true, size, std::move(version)};
}

void noisy_spurious_events_validate_once_and_log_once()
{
    nextcloud::SyncTreeSnapshot snapshot;
    snapshot.remember("Photos/2026/party.jpg", state(42, "etag-a"));
    snapshot.setCurrent("Photos/2026/party.jpg", state(42, "etag-a"));

    nextcloud::WatcherOverloadAdapter adapter;
    for (int i = 0; i < 1000; ++i) {
        adapter.ingest(modified("Photos/2026/party.jpg"));
    }

    const auto batch = adapter.flush(snapshot);

    expect(batch.raw_events_observed == 1000, "raw noisy events are counted");
    expect(batch.candidates_seen == 1, "repeated path is coalesced to one candidate");
    expect(batch.candidates_validated == 1, "spurious path is validated once");
    expect(batch.sync_work_items == 0, "unchanged final state does not schedule sync work");
    expect(batch.ignored_spurious_candidates == 1, "unchanged candidate is ignored after validation");
    expect(batch.summary_log_lines == 1, "noisy batch emits one summary log line");
}

void changed_final_state_becomes_sync_work()
{
    nextcloud::SyncTreeSnapshot snapshot;
    snapshot.remember("Documents/report.md", state(12, "old"));
    snapshot.setCurrent("Documents/report.md", state(18, "new"));

    nextcloud::WatcherOverloadAdapter adapter;
    adapter.ingest(modified("Documents/report.md"));
    adapter.ingest(ld_modified("Documents/report.md"));
    adapter.ingest({"/sync", "Documents/report.md", nextcloud::FileSignalKind::Metadata, false});

    const auto batch = adapter.flush(snapshot);

    expect(batch.candidates_validated == 1, "changed path is still validated once");
    expect(batch.sync_work_items == 1, "changed final state schedules one sync item");
    expect(batch.decisions[0].reason == "version changed at validation",
        "sync reason stays in product validation vocabulary");
}

void deleted_final_state_becomes_sync_work()
{
    nextcloud::SyncTreeSnapshot snapshot;
    snapshot.remember("Notes/todo.txt", state(7, "seen"));
    snapshot.setCurrent("Notes/todo.txt", std::nullopt);

    nextcloud::WatcherOverloadAdapter adapter;
    adapter.ingest({"/sync", "Notes/todo.txt", nextcloud::FileSignalKind::Removed, false});

    const auto batch = adapter.flush(snapshot);

    expect(batch.sync_work_items == 1, "deleted final state schedules sync work");
    expect(batch.decisions[0].reason == "file disappeared before validation",
        "delete reason is produced by product validation");
}

void overflow_requests_rescan_without_per_path_validation()
{
    nextcloud::SyncTreeSnapshot snapshot;
    nextcloud::WatcherOverloadAdapter adapter;

    adapter.ingest(modified("a.txt"));
    adapter.ingest({"/sync", {}, nextcloud::FileSignalKind::Overflow, true});

    const auto batch = adapter.flush(snapshot);

    expect(batch.full_rescan_required, "overflow asks product to rescan");
    expect(batch.candidates_validated == 0, "overflow skips stale candidate validation");
    expect(batch.summary_log_lines == 1, "overflow emits one summary log line");
    expect(adapter.pendingCandidates() == 0, "flush clears pending candidates");
}

void candidate_limit_degrades_to_rescan()
{
    nextcloud::SyncTreeSnapshot snapshot;
    nextcloud::WatcherOverloadAdapter adapter(2);

    adapter.ingest(modified("one.txt"));
    adapter.ingest(modified("two.txt"));
    adapter.ingest(modified("three.txt"));

    const auto batch = adapter.flush(snapshot);

    expect(batch.full_rescan_required, "candidate limit degrades to full rescan");
    expect(batch.candidates_validated == 0, "degraded batch avoids partial validation claims");
    expect(batch.summary_log_lines == 1, "candidate overload is summarized");
}

} // namespace

int main()
{
    noisy_spurious_events_validate_once_and_log_once();
    changed_final_state_becomes_sync_work();
    deleted_final_state_becomes_sync_work();
    overflow_requests_rescan_without_per_path_validation();
    candidate_limit_degrades_to_rescan();
    return failures == 0 ? 0 : 1;
}
