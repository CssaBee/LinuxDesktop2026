#pragma once

#if !defined(LINUXDESKTOP2026_WATCH_ENABLE_TEST_HOOKS)
#define LINUXDESKTOP2026_WATCH_ENABLE_TEST_HOOKS
#endif

#include "linuxdesktop/watch.hpp"

#include <optional>

namespace linuxdesktop::watch::detail {

class watch_backend {
public:
    virtual ~watch_backend() = default;

    virtual start_report add_watch(watch_id id, const watch_options& options) = 0;
    virtual bool remove_watch(watch_id id) = 0;
    virtual void stop() = 0;
    virtual std::optional<watch_event> wait_event() = 0;
    virtual capability_report capabilities() const = 0;
};

std::shared_ptr<watch_backend> make_native_backend();
std::shared_ptr<watch_backend> make_libuv_backend();
watcher make_watcher_for_backend(std::shared_ptr<watch_backend> backend);

} // namespace linuxdesktop::watch::detail
