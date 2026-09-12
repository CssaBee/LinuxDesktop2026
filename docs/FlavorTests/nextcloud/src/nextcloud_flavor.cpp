#include "nextcloud_flavor.hpp"

#include <algorithm>
#include <utility>

namespace flavor_tests::nextcloud {

namespace {

std::string reason_for_change(
    const std::optional<FileStateSnapshot>& known,
    const std::optional<FileStateSnapshot>& current)
{
    if (known && !current) {
        return "file disappeared before validation";
    }
    if (!known && current) {
        return "new file exists at validation";
    }
    if (!known && !current) {
        return "path is absent in both snapshots";
    }
    if (known->version != current->version) {
        return "version changed at validation";
    }
    if (known->size != current->size) {
        return "size changed at validation";
    }
    return "final path state matches known metadata";
}

bool differs(
    const std::optional<FileStateSnapshot>& known,
    const std::optional<FileStateSnapshot>& current)
{
    if (known.has_value() != current.has_value()) {
        return true;
    }
    if (!known && !current) {
        return false;
    }
    return known->exists != current->exists ||
        known->size != current->size ||
        known->version != current->version;
}

FileSignalKind to_signal_kind(linuxdesktop::watch::event_kind kind)
{
    switch (kind) {
    case linuxdesktop::watch::event_kind::created:
        return FileSignalKind::Created;
    case linuxdesktop::watch::event_kind::removed:
        return FileSignalKind::Removed;
    case linuxdesktop::watch::event_kind::renamed_old:
    case linuxdesktop::watch::event_kind::renamed_new:
        return FileSignalKind::Renamed;
    case linuxdesktop::watch::event_kind::metadata:
        return FileSignalKind::Metadata;
    case linuxdesktop::watch::event_kind::overflow:
    case linuxdesktop::watch::event_kind::error:
        return FileSignalKind::Overflow;
    case linuxdesktop::watch::event_kind::modified:
    default:
        return FileSignalKind::Modified;
    }
}

FilesystemSignal from_linuxdesktop_event(const linuxdesktop::watch::watch_event& event)
{
    return {
        event.path.absolute.parent_path(),
        event.path.root_relative.value_or(event.path.absolute.filename()),
        to_signal_kind(event.kind),
        event.rescan_recommended,
    };
}

} // namespace

void SyncTreeSnapshot::remember(std::filesystem::path relative_path, FileStateSnapshot state)
{
    known_[std::move(relative_path)] = std::move(state);
}

void SyncTreeSnapshot::setCurrent(
    std::filesystem::path relative_path,
    std::optional<FileStateSnapshot> state)
{
    current_[std::move(relative_path)] = std::move(state);
}

ValidationDecision SyncTreeSnapshot::validate(const std::filesystem::path& relative_path) const
{
    const auto known_it = known_.find(relative_path);
    const auto current_it = current_.find(relative_path);
    const std::optional<FileStateSnapshot> known =
        known_it == known_.end() ? std::optional<FileStateSnapshot>{} : known_it->second;
    const std::optional<FileStateSnapshot> current =
        current_it == current_.end() ? known : current_it->second;

    return {relative_path, differs(known, current), reason_for_change(known, current)};
}

WatcherOverloadAdapter::WatcherOverloadAdapter(std::size_t max_pending_candidates)
    : max_pending_candidates_(std::max<std::size_t>(1, max_pending_candidates))
{
}

void WatcherOverloadAdapter::ingest(const FilesystemSignal& signal)
{
    ++raw_events_observed_;
    if (signal.rescan_recommended || signal.kind == FileSignalKind::Overflow ||
        signal.relative_path.empty()) {
        full_rescan_required_ = true;
        return;
    }

    if (pending_.size() >= max_pending_candidates_ &&
        pending_.find(signal.relative_path) == pending_.end()) {
        full_rescan_required_ = true;
        return;
    }

    pending_[signal.relative_path] = signal;
}

void WatcherOverloadAdapter::ingest(const linuxdesktop::watch::watch_event& event)
{
    ingest(from_linuxdesktop_event(event));
}

ValidationBatch WatcherOverloadAdapter::flush(const SyncTreeSnapshot& snapshot)
{
    ValidationBatch batch;
    batch.raw_events_observed = raw_events_observed_;
    batch.candidates_seen = pending_.size();
    batch.full_rescan_required = full_rescan_required_;

    if (!full_rescan_required_) {
        for (const auto& item : pending_) {
            auto decision = snapshot.validate(item.first);
            if (decision.needs_sync) {
                ++batch.sync_work_items;
            } else {
                ++batch.ignored_spurious_candidates;
            }
            ++batch.candidates_validated;
            batch.decisions.push_back(std::move(decision));
        }
    }

    if (batch.raw_events_observed > 0 || batch.full_rescan_required) {
        batch.summary_log_lines = 1;
    }

    raw_events_observed_ = 0;
    full_rescan_required_ = false;
    pending_.clear();
    return batch;
}

std::size_t WatcherOverloadAdapter::pendingCandidates() const
{
    return pending_.size();
}

bool WatcherOverloadAdapter::fullRescanRequired() const
{
    return full_rescan_required_;
}

} // namespace flavor_tests::nextcloud
