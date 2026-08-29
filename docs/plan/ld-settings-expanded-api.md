# Expanded `ld_settings` API Plan

Status: superseded for module ownership by ADR 0012. This document remains an
inventory of prototype behavior and survey-derived API pressure. It is not the
controlling plan for what `ld_settings` should own at ship-candidate status.

This plan captures the older post-grilling prototype shape that temporarily
expanded `ld_settings` beyond a standard-path resolver.

ADR 0012 narrows `ld_settings` to toolkit-neutral settings/config behavior.
Generic path policy moves to `ld_paths`; desktop integration effects move to
the planned `ld_desktop`; migration planning/execution and app-settings Registry
migration compatibility move to the planned `ld_migration`.

Even in that broader prototype, `ld_settings` did not parse every
application's settings payload. Applications kept ownership of XML, JSON, INI,
TOML, database, and domain-specific validation unless they opted into the
prototype Registry value API.

## Shippable Scope

Under ADR 0012, `ld_settings` can become a ship candidate only when the
settings/config items below are implemented and verified. Registry, desktop
effects, and migration items are extraction inventory for `ld_desktop` and
`ld_migration`, not stable `ld_settings` responsibilities.

- fixed standard roots,
- named roots,
- component roots,
- portable levels,
- all config layers: `defaults`, `global`, `user`, `local`, `portable`, `managed`, `enforced`,
- default precedence with enforced values non-overridable,
- hydration of default/model config files,
- atomic file writes with validation and backups,
- Windows Known Folder resolution,
- no new C ABI expansion before release-candidate status; existing C ABI entry points are kept compatible where practical,
- examples from Notepad++, ShareX, WinSCP, KeePassXC, and PortableApps-style workflows,
- Windows CI or equivalent automated Windows verification for Known Folders, C ABI, and atomic writes,
- manual Windows verification transcript before release.

Extraction inventory:

- migration plans with dry-run default and explicit execution move to `ld_migration`,
- file/directory copy and move execution moves to `ld_migration`,
- rollback and before/after reporting moves to `ld_migration`,
- app-settings Registry JSON and `.reg` snapshot/import/export compatibility moves to `ld_migration`,
- full practical Registry-equivalent desktop/system behavior moves to `ld_desktop`,
- autostart effect implementation on Windows and Linux moves to `ld_desktop`,
- desktop entries, icons, MIME/file associations, default applications, URL protocol handlers, shell-equivalent behavior, desktop database updates, and managed/enforced policy move to `ld_desktop`.

No half-finished product: documentation-only placeholders are acceptable during development, but not for the first declared shippable release.

## Historical Prototype Namespaces

```cpp
namespace linuxdesktop::settings;
namespace linuxdesktop::settings::registry;
namespace linuxdesktop::settings::effects;
```

Rationale:

- root/config lifecycle remains easy to discover,
- raw Registry operations were kept out of the root API during the prototype,
- effect APIs modeled autostart and policy during the prototype.

ADR 0012 changes the target ownership: future stable desktop-effect APIs belong
under `ld_desktop`, and future stable migration APIs belong under
`ld_migration`.

## Core Types

### Portable Level

```cpp
enum class portable_level {
    off,
    settings_only,
    profile,
    clean
};
```

### Root Purpose

```cpp
enum class root_purpose {
    resources,
    config,
    data,
    state,
    cache,
    runtime,
    session,
    plugin_config,
    logs,
    profiles,
    backup,
    temp,
    component_config,
    component_data,
    component_state,
    managed_config,
    enforced_config,
    custom
};
```

### Persistence Class

```cpp
enum class persistence_class {
    roaming,
    machine_local,
    portable,
    ephemeral,
    managed,
    enforced
};
```

### Named Root

```cpp
struct named_root_request {
    std::string name;
    root_purpose purpose = root_purpose::custom;
    persistence_class persistence = persistence_class::roaming;
    std::filesystem::path relative_path;
    bool create = true;
};

struct named_root {
    std::string name;
    root_purpose purpose = root_purpose::custom;
    persistence_class persistence = persistence_class::roaming;
    std::filesystem::path path;
    bool created = false;
    std::vector<diagnostic> diagnostics;
};
```

Why vector, not map:

- C ABI can expose arrays naturally,
- ordering remains deterministic,
- metadata stays attached to each root,
- duplicate names can be diagnosed explicitly.

### Component Roots

```cpp
enum class component_kind {
    plugin,
    embedded_tool,
    profile,
    language_pack,
    extension,
    custom
};

struct component_root_request {
    std::string name;
    component_kind kind = component_kind::custom;
    std::vector<named_root_request> roots;
};

struct component_root_group {
    std::string name;
    component_kind kind = component_kind::custom;
    std::vector<named_root> roots;
    std::vector<diagnostic> diagnostics;
};
```

Rule: component roots resolve paths only. Hydration, writes, migration, and Registry import/export operate through separate APIs.

## Config Layers

```cpp
enum class config_layer_kind {
    defaults,
    global,
    user,
    local,
    portable,
    managed,
    enforced
};

enum class storage_backend {
    file,
    registry,
    null_backend,
    override_values,
    app_callback
};

struct config_layer {
    config_layer_kind kind;
    storage_backend backend;
    std::string name;
    std::filesystem::path path;
    bool writable = false;
    bool required = false;
    bool enforced = false;
    int precedence = 0;
};

struct layer_report {
    std::vector<config_layer> candidates;
    std::vector<config_layer> active_read_order;
    std::optional<config_layer> active_write_layer;
    std::vector<diagnostic> diagnostics;
};
```

Default precedence:

```text
defaults < global < user < local < portable < managed < enforced
```

The report should include all candidates because migrations often need to explain why a different store won.

## Merge Boundary

`ld_settings` should not become a universal parser.

```cpp
using merge_callback = std::function<bool(
    const std::vector<config_layer>& read_order,
    const std::filesystem::path& output_path,
    std::string& error)>;
```

Rules:

- app-owned XML remains app-owned,
- app-owned JSON/INI/TOML remains app-owned unless the app opts into helper adapters later,
- Registry value import/export was owned by `ld_settings::registry` in the
  prototype; ADR 0012 moves app-settings Registry migration compatibility to
  `ld_migration`,
- merge callbacks let the app decide content semantics.

## Migration Plans Prototype

Hydration copies missing defaults. Migration moves or transforms existing user state. Keep them separate.

ADR 0012 moves migration planning/execution to the planned `ld_migration`
module. `ld_settings` may describe settings bundles and participate in
migrations, but it should not own the stable migration engine.

```cpp
enum class migration_action_kind {
    copy_file,
    move_file,
    copy_directory,
    move_directory,
    import_registry,
    export_registry,
    write_registry_value,
    delete_registry_key,
    write_autostart,
    write_policy
};

struct migration_action {
    migration_action_kind kind;
    std::string name;
    std::filesystem::path source_path;
    std::filesystem::path target_path;
    bool dangerous = false;
    bool requires_elevation = false;
};

struct migration_plan {
    std::vector<migration_action> actions;
    bool dry_run = true;
    std::vector<diagnostic> diagnostics;
};
```

Execution rules:

- dry-run first by default,
- destructive and global operations require explicit flags,
- every executed action reports before/after state where practical,
- rollback support is required for portable run-scoped registry/file moves before release.

## Registry API Prototype

ADR 0012 splits Registry-shaped behavior by meaning. App-settings Registry
snapshot/import/export compatibility belongs to `ld_migration` when used to move
application state. Registry-equivalent behavior whose purpose is desktop,
startup, shell, policy, or session integration belongs to `ld_desktop`.

### Registry Types

```cpp
namespace linuxdesktop::settings::registry {

enum class hive {
    current_user,
    local_machine,
    classes_root,
    users,
    current_config
};

enum class view {
    native,
    registry_32,
    registry_64
};

enum class value_type {
    none,
    string,
    expandable_string,
    multi_string,
    dword,
    qword,
    binary,
    unknown
};

struct key {
    hive root;
    std::string subkey;
    view registry_view = view::native;
};

struct value {
    std::string name;
    value_type type;
    std::vector<std::byte> bytes;
};

struct options {
    bool allow_hklm_write = false;
    bool allow_policy_write = false;
    bool allow_recursive_delete = false;
    bool allow_import = false;
    bool dry_run = true;
};

}
```

### Registry Operations

The prototype inventory includes:

- `read_value`,
- `write_value`,
- `delete_value`,
- `delete_key`,
- `enumerate_values`,
- `enumerate_subkeys`,
- `export_tree_json`,
- `import_tree_json`,
- `export_tree_reg`,
- `import_tree_reg`.

Safety rules while this remains in `ld_settings`:

- default access is read-only,
- writes to HKLM require explicit permission and real platform capability,
- recursive deletion requires explicit permission,
- imports require explicit permission,
- policy writes require explicit permission,
- Linux builds return unsupported diagnostics for raw Registry operations unless reading/writing a portable Registry snapshot file.

## Effects API Prototype

ADR 0012 moves desktop integration effects to the planned `ld_desktop` module.
The current `linuxdesktop::settings::effects` implementation is temporary
prototype evidence.

Only these effects are first-scope:

- autostart,
- managed preference,
- enforced preference.

These effects are explicitly unsupported in first `ld_settings` and should become GitHub issue requests:

- file associations,
- default applications,
- protocol handlers,
- shell context menus,
- recent documents,
- jump lists,
- services,
- COM/shell extension registration.

### Autostart

```cpp
namespace linuxdesktop::settings::effects {

struct autostart_entry {
    std::string id;
    std::string display_name;
    std::filesystem::path executable;
    std::vector<std::string> arguments;
    bool enabled = true;
    bool user_scope = true;
};

struct apply_options {
    bool dry_run = true;
    bool allow_global_write = false;
    bool allow_desktop_integration_write = false;
    bool allow_policy_write = false;
};

}
```

Windows backend:

- write/remove/check `CurrentVersion\Run` entries,
- support per-user first,
- require explicit global flag for machine-wide startup.

Linux backend:

- write/remove/check XDG Autostart `.desktop` files,
- support `Hidden=true` for disable,
- validate desktop entry fields before writing.

### Managed And Enforced Preferences

First C++ API shape:

```cpp
namespace linuxdesktop::settings::effects {

struct policy_entry {
    std::string id;
    std::string schema_id;
    std::string group;
    std::string key;
    std::string value;
    bool enforced = false;
    bool user_scope = false;
};

policy_report apply_policy(const policy_entry&, const apply_options& = {});
policy_report remove_policy(const policy_entry&, const apply_options& = {});
policy_report query_policy(const policy_entry&, const apply_options& = {});

}
```

Windows backend:

- Registry policy keys and app-owned HKLM/HKCU layers.
- `Software\Policies\<schema/group>` values are written through the raw Registry backend.
- global writes require both `allow_policy_write` and `allow_global_write`.

Linux backend:

- dconf/GSettings-compatible defaults and lock files where schemas exist, without linking GLib.
- defaults are emitted as keyfiles; enforced settings also emit lock files.
- the initial implementation supports override directories for safe tests and staged package generation.
- Return diagnostics when no schema/backend exists.

## C ABI Posture

The existing C ABI exposes named roots and config-layer reports, plus the
prototype surfaces already added for mutation, migration, Registry snapshot
formats, autostart effects, and policy effects. Keep that surface compatible
where practical, but do not add new C ABI families until release-candidate
status.

When C ABI design resumes, the shape should remain:

- arrays with explicit counts,
- UTF-8 strings owned by report objects,
- one matching free function per report family,
- no STL types or exceptions,
- enum values fixed and documented,
- runtime version functions must match header macros.

Registry, migration, and future effect C ABI additions can lag behind C++
during development. At release-candidate status, decide whether the existing
plain-struct reports are still adequate or whether new long-lived surfaces need
opaque handles or size-tagged structs.

## Example Targets

Required examples before the affected modules ship:

`ld_settings`:

- Notepad++ settings root resolution with named plugin/session/log roots.
- Notepad++ model XML hydration and ordered writes.

`ld_migration`:

- ShareX personal path resolution with portable marker, registry source,
  migration from old path, and named roots.
- WinSCP-style Registry/INI/null/override storage selection.
- KeePassXC-style roaming/local split and Linux state migration.
- PortableApps-style registry snapshot/restore dry-run.

`ld_desktop`:

- Autostart enable/disable on Windows and Linux.
- Managed/enforced policy plan on Windows and Linux.

## Historical Implementation Order

1. Update docs and examples to reflect the expanded API.
2. Add named roots and component roots to C++ and the existing C ABI. `(implemented)`
3. Add config layers and precedence reports.
4. Add portable levels.
5. Add migration plans as dry-run objects. `(initial C++ API implemented)`
6. Implement Windows Registry raw operations. `(initial C++ API/backend implemented; Windows verification pending)`
7. Add JSON Registry import/export. `(initial C++ snapshot format and tree wrappers implemented)`
8. Add `.reg` compatibility import/export. `(initial C++ snapshot format and tree wrappers implemented)`
9. Implement autostart Windows and Linux backends. `(initial C++ API/backend implemented; Windows verification pending)`
10. Implement managed/enforced policy Windows and Linux backends. `(initial C++ API/backend implemented; Windows verification pending)`
11. Add Windows CI and manual verification transcript. `(CI exists; transcript remains pending after a real Windows run)`
12. Defer further C ABI expansion until release-candidate status. `(existing surface is maintained where practical)`

## Current Status

`ld_settings` is a working first sample, not a shippable final module. The
current Registry, autostart, policy, and migration APIs are temporary
implementation locations. See `docs/plan/ld-desktop-extraction.md` and
`docs/plan/ld-migration-extraction.md` for the extraction requirements that
must be satisfied before ship-candidate status.

Implemented in the current C++ sample:

- named roots,
- component roots,
- config layer candidates and active read/write ordering,
- portable levels,
- string names for public root/layer enums,
- C++ lookup helpers for named roots, component roots, component-local roots, and config layers,
- existing C ABI exposure for named roots, component roots, config layers, and portable levels,
- dry-run-first migration plans with file/directory copy and move execution,
- raw `ld_settings::registry` C++ API for read, write, delete, and enumeration,
- Windows Registry backend seed using Win32 Registry APIs,
- structured unsupported diagnostics for raw Registry calls on non-Windows platforms,
- canonical JSON Registry snapshot serialization/parsing,
- `.reg` snapshot serialization/parsing for string, expandable string, multi-string, DWORD, QWORD, and binary-shaped values,
- Registry tree JSON/`.reg` import/export wrappers over the raw Registry API,
- autostart effect API with dry-run-first writes,
- Linux XDG Autostart `.desktop` write/query/remove support,
- Windows `CurrentVersion\Run` autostart backend shape over the raw Registry API,
- managed/enforced policy effect API with dry-run-first writes,
- Linux dconf/GSettings-compatible defaults and lock-file generation without GLib,
- Windows `Software\Policies` backend shape over the raw Registry API.

The next code work should harden module boundaries, write safety, and Windows
verification before adding new C ABI families. Per ADR 0012, the Registry,
autostart, policy, and migration behavior listed here must be extracted to
`ld_desktop` and `ld_migration` before those APIs can be considered ship
candidates.
