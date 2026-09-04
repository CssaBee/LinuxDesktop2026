#include "linuxdesktop/watch.hpp"

#if defined(LINUXDESKTOP2026_WATCH_ENABLE_TEST_HOOKS)
#error "watch test hooks must not leak through the public ld_watch target"
#endif

int main()
{
    linuxdesktop::watch::watcher watcher;
    (void)watcher.capabilities();
    return 0;
}
