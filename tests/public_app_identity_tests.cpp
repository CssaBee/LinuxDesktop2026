#include "linuxdesktop/migration.hpp"
#include "linuxdesktop/paths.hpp"
#include "linuxdesktop/root.hpp"
#include "linuxdesktop/settings.hpp"

#include <type_traits>

static_assert(std::is_same_v<linuxdesktop::app_identity, linuxdesktop::paths::app_identity>);
static_assert(std::is_same_v<linuxdesktop::app_identity, linuxdesktop::root::app_identity>);
static_assert(std::is_same_v<linuxdesktop::app_identity, linuxdesktop::settings::app_identity>);
static_assert(std::is_same_v<
    decltype(linuxdesktop::migration::rooted_path_request::identity),
    linuxdesktop::app_identity>);

int main()
{
    linuxdesktop::app_identity identity;
    identity.organization = "LinuxDesktop2026";
    identity.application = "public-app-identity";

    linuxdesktop::paths::resolver_options path_options;
    path_options.use_process_environment = false;

    (void)linuxdesktop::paths::resolve_app_paths(identity, path_options);

    linuxdesktop::root::request_builder root_builder(identity);
    (void)root_builder;

    linuxdesktop::settings::root_builder settings_builder(identity);
    (void)settings_builder;

    return 0;
}
