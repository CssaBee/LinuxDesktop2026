# `ld_desktop` Extraction Requirements

Status: initial C++ extraction in progress; full ship-candidate coverage still
required.

`ld_desktop` owns platform actions that register an application with the
desktop, shell, session, or managed-policy environment. The current C++
autostart and managed/enforced policy implementation lives in
`linuxdesktop::desktop`; callers should use `ld_desktop` directly for those
effects. Registry-equivalent desktop/system behavior belongs in `ld_desktop`.
The matching C ABI lives in `linuxdesktop/desktop_c.h` and the
`ld_desktop` C library surface. `ld_settings` does not publicly depend on
`ld_desktop`.

## Scope

`ld_desktop` supports desktop integration by standards-backed registration
artifacts rather than by separate GNOME, KDE, Xfce, bare-window-manager, or
Windows-shell public APIs. GNOME, KDE Plasma, Xfce, bare window-manager
sessions, and the Windows shell are Desktop Flavors for validation, not separate
platform promises.

The extracted module must cover these responsibility groups before
`ld_desktop` can be treated as the stable desktop integration surface:

- autostart entries,
- desktop entries,
- icon installation and lookup metadata,
- MIME and file associations,
- default applications,
- URL protocol handlers,
- desktop database update and activation plans,
- uninstall cleanup reporting,
- managed and enforced desktop or application policy,
- Registry-equivalent behavior whose purpose is shell, startup, policy,
  session, or desktop integration.

Runtime shell actions such as opening a path or URL, revealing a file in a file
manager, and forwarding a later invocation to an already-running process are
outside this expansion. They should be handled by a later process/shell or IPC
design pass unless repeated consumer evidence proves they belong here.

The preferred C++ surface for app registration is a coherent desktop bundle
that can plan, dry-run, apply, query, and remove the normal registration set
together. Individual effect calls remain useful for advanced callers,
tests, and partial integrations, but they should not create per-desktop public
APIs.

## Desktop Bundle API

The current C++ surface exposes `desktop_bundle` as the preferred path for
ordinary application registration. The bundle is a product-owned description of
the artifacts and intents an installer, first-run setup, or migration tool
wants to stage:

```cpp
namespace linuxdesktop::desktop {

enum class registration_scope {
    user,
    global
};

enum class activation_step_kind {
    refresh_desktop_database,
    refresh_mime_database,
    refresh_icon_cache,
    refresh_dconf_database,
    windows_shell_notify,
    windows_default_apps_ui
};

struct desktop_entry_metadata {
    std::string id;
    std::string display_name;
    std::string generic_name;
    std::string comment;
    std::filesystem::path executable;
    std::vector<std::string> arguments;
    std::filesystem::path working_directory;
    std::vector<std::string> categories;
    std::vector<std::string> keywords;
    bool terminal = false;
};

struct icon_reference {
    std::string name;
    std::filesystem::path source_path;
    std::string theme = "hicolor";
    std::vector<int> sizes;
};

struct mime_declaration {
    std::string name;
    std::string comment;
    std::vector<std::string> glob_patterns;
};

struct mime_association {
    std::string mime_type;
    std::vector<std::string> desktop_entry_ids;
};

struct default_application_intent {
    std::string mime_type_or_scheme;
    std::string desktop_entry_id;
    bool make_default = false;
};

struct url_scheme_handler {
    std::string scheme;
    std::string desktop_entry_id;
};

struct cleanup_rule {
    std::filesystem::path path;
    bool remove_empty_parent = false;
};

struct desktop_bundle {
    registration_scope scope = registration_scope::user;
    std::optional<autostart_entry> autostart;
    std::optional<desktop_entry_metadata> entry;
    std::vector<icon_reference> icons;
    std::vector<mime_declaration> mime_declarations;
    std::vector<mime_association> mime_associations;
    std::vector<default_application_intent> default_applications;
    std::vector<url_scheme_handler> url_scheme_handlers;
    std::vector<policy_entry> policies;
    std::vector<cleanup_rule> cleanup;
};

struct activation_step {
    activation_step_kind kind;
    bool required = false;
    bool can_run = false;
    std::string command_preview;
    std::vector<diagnostic> diagnostics;
};

struct desktop_bundle_report {
    bool ok = false;
    bool dry_run = true;
    std::vector<effect_report> artifact_reports;
    std::vector<policy_report> policy_reports;
    std::vector<activation_step> activation_plan;
    std::vector<cleanup_rule> cleanup_plan;
    std::vector<diagnostic> diagnostics;
};

desktop_bundle_report plan_bundle(const desktop_bundle&, const apply_options& = {});
desktop_bundle_report apply_bundle(const desktop_bundle&, const apply_options& = {});
desktop_bundle_report query_bundle(const desktop_bundle&, const apply_options& = {});
desktop_bundle_report remove_bundle(const desktop_bundle&, const apply_options& = {});

} // namespace linuxdesktop::desktop
```

The first implementation intentionally keeps live activation as an explicit
plan. A successful bundle write means the expected files or Registry artifacts
were staged. It does not mean the desktop database, MIME database, icon cache,
dconf database, or Windows default-app state has already consumed those
artifacts.

Bundle reports should preserve per-artifact diagnostics instead of collapsing
everything into a single success flag. A bundle can partially plan successfully
while still requiring user-facing follow-up, such as opening Windows default
apps settings, running `update-desktop-database`, or retrying a global write
with elevated privileges.

## Individual Effect Calls

These effect calls should remain public after the bundle API exists:

- autostart apply, query, and remove;
- managed/enforced policy apply, query, and remove;
- desktop entry stage, query, and remove;
- icon stage, query, and remove;
- MIME declaration stage, query, and remove;
- MIME association stage, query, and remove;
- default-application intent stage, query, and remove;
- URL protocol handler stage, query, and remove;
- activation-plan query for desktop databases, MIME databases, icon caches,
  dconf, and Windows shell/default-app follow-up;
- cleanup-plan query and remove for artifacts previously generated by
  `ld_desktop`.

These calls exist for advanced callers, tests, partial installers, and Desktop
Flavor fixtures. They should share the same validation, path selection,
diagnostics, activation-step vocabulary, and durable write primitive used by
`desktop_bundle`.

Do not add public GNOME, KDE, Xfce, freedesktop.org session, Explorer, or
Registry-class-specific APIs for this tranche. Those are backend capabilities
and validation flavors, not public product concepts.

## C ABI Posture

Do not mirror `desktop_bundle` into the C ABI during this design step. The
current C ABI remains limited to the existing autostart and policy calls until
release-candidate API policy explicitly allows expansion. When it does expand,
prefer opaque handles or builder-style allocation over copying the whole C++
object graph into nested C structs; registration bundles contain repeated
strings, paths, associations, diagnostics, activation steps, and cleanup rules,
which are easy to make painful for C callers if exposed prematurely.

## Required API Posture

- Use explicit capability reports for every backend.
- Keep filesystem and Registry mutation behind explicit permission flags.
- Default mutating operations to dry-run where practical.
- Separate staged file/Registry artifact writes from live activation commands
  such as `update-desktop-database`, `update-mime-database`, icon-cache
  refreshes, `dconf update`, or Windows system-default updates.
- Report unsupported, sandbox-limited, permission-denied, backend-missing, and
  externally-updated-database states as diagnostics.
- Keep desktop/session concepts separate from settings payload concepts.
- Route generic path selection through `ld_paths` instead of embedding XDG,
  Known Folder, or home-directory policy.
- Treat Windows `CurrentVersion\Run`, `Software\Policies`, shell classes, and
  protocol handler equivalents as desktop/system effects, not settings storage.

## Current Implementation

- `include/linuxdesktop/desktop.hpp` exposes the C++ `ld_desktop` API,
  including the first `desktop_bundle` request/report vocabulary and
  `plan_bundle`, `apply_bundle`, `query_bundle`, and `remove_bundle`.
- `include/linuxdesktop/desktop_c.h` exposes the C ABI `ld_desktop` surface.
- `ld_desktop` reports capabilities for autostart, Linux/XDG desktop entries,
  icons, MIME/file associations, default applications, URL protocol handlers,
  shell-equivalent integration, desktop database updates, and managed policy.
  Managed policy is reported as backend-limited on Linux because the current
  implementation generates dconf-compatible source files but does not run
  `dconf update` or otherwise verify active dconf database state.
- Linux autostart, desktop entries, icons, shared-mime-info package XML,
  `mimeapps.list` associations, default applications, URL scheme handlers, and
  managed/enforced policy use dry-run-first staged artifact behavior.
  Successful reports mean the expected artifact was staged, not that the
  running desktop, MIME database, icon cache, or dconf database consumed it.
- Linux/XDG registration reports return activation plans for
  `update-desktop-database`, `update-mime-database`, icon-cache refreshes, and
  dconf activation instead of running those commands by default.
- Windows registration effects report native mappings without overpromising
  mutation. Autostart maps to per-user Run registration; application identity,
  file associations, URL protocols, and policy map to Registry-backed shell or
  policy artifacts; default-app selection is user-choice mediated and reports a
  Default Apps settings activation step instead of forcing `UserChoice`.
  Registry-backed Windows writes remain backend-limited until a shared
  Registry/system layer owns HKCU/HKLM mutation, and Windows shell activation is
  reported as a follow-up plan rather than executed by staged artifact calls.
- Windows 10 and Windows 11 share the same public posture: registration
  artifacts and diagnostics are modeled once, while OS-version differences in
  Settings UI or shell consumption are validation notes unless a stable Windows
  contract requires a public branch.
- Desktop Flavor variance is currently covered by hermetic capability, XDG
  path, and staged artifact tests. Live desktop-session consumption is not yet
  release evidence.
- New C++ callers should include `linuxdesktop/desktop.hpp` and link
  `LinuxDesktop2026::ld_desktop`.
- New C callers should include `linuxdesktop/desktop_c.h` and link
  `LinuxDesktop2026::ld_desktop`.

## Validation Required

Before `ld_desktop` is a ship candidate, tests and examples must cover:

- Linux XDG Autostart write, query, disable, and remove paths,
- Linux `.desktop` field escaping and invalid-field rejection,
- Linux MIME/default-app/protocol registration as dry-run and staged file
  generation before any live database update,
- Desktop Flavor scenarios for `xdg_full_gnome`, `xdg_full_kde`,
  `xdg_light_xfce`, `xdg_minimal_bare_wm`, and `windows_registration`, with
  assertions focused on honest capability reporting and correct staged
  artifacts rather than live desktop consumption,
- managed/enforced Linux policy diagnostics for missing schemas, user-vs-global
  scope, and lock/default file behavior,
- Windows-shaped autostart and policy diagnostics, even when CI cannot mutate
  machine-wide state,
- hostile input for desktop entry IDs, command arguments, paths, MIME names,
  protocol names, and policy values,
- permission denial for global writes,
- rollback or uninstall reporting for generated files where practical,
- at least one real consumer integration that exercises desktop effects.

## Extraction Rule

When this module grows, keep C++ and C desktop-effect entry points under the
`ld_desktop` headers and library target. Do not leave callers believing that
`ld_settings` owns desktop integration.

The old `ld_settings` desktop-effect facade has been removed; there are no
`ld_settings_*` desktop-effect C ABI entry points in the current code.
