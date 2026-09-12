#pragma once

#include "linuxdesktop/watch.hpp"

#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace flavor_tests::nextcloud {

enum class FileSignalKind {
    Created,
    Modified,
    Removed,
    Renamed,
    Metadata,
    Overflow
};

struct FilesystemSignal {
    std::filesystem::path sync_root;
    std::filesystem::path relative_path;
    FileSignalKind kind = FileSignalKind::Modified;
    bool rescan_recommended = false;
};

struct FileStateSnapshot {
    bool exists = true;
    std::uintmax_t size = 0;
    std::string version;
};

struct ValidationDecision {
    std::filesystem::path relative_path;
    bool needs_sync = false;
    std::string reason;
};

struct ValidationBatch {
    std::size_t raw_events_observed = 0;
    std::size_t candidates_seen = 0;
    std::size_t candidates_validated = 0;
    std::size_t sync_work_items = 0;
    std::size_t ignored_spurious_candidates = 0;
    std::size_t summary_log_lines = 0;
    bool full_rescan_required = false;
    std::vector<ValidationDecision> decisions;
};

class SyncTreeSnapshot {
public:
    void remember(std::filesystem::path relative_path, FileStateSnapshot state);
    void setCurrent(std::filesystem::path relative_path, std::optional<FileStateSnapshot> state);

    ValidationDecision validate(const std::filesystem::path& relative_path) const;

private:
    std::map<std::filesystem::path, FileStateSnapshot> known_;
    std::map<std::filesystem::path, std::optional<FileStateSnapshot>> current_;
};

class WatcherOverloadAdapter {
public:
    explicit WatcherOverloadAdapter(std::size_t max_pending_candidates = 512);

    void ingest(const FilesystemSignal& signal);
    void ingest(const linuxdesktop::watch::watch_event& event);
    ValidationBatch flush(const SyncTreeSnapshot& snapshot);

    std::size_t pendingCandidates() const;
    bool fullRescanRequired() const;

private:
    std::size_t max_pending_candidates_;
    std::size_t raw_events_observed_ = 0;
    bool full_rescan_required_ = false;
    std::map<std::filesystem::path, FilesystemSignal> pending_;
};

} // namespace flavor_tests::nextcloud
