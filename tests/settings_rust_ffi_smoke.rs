#![no_std]

use core::ffi::{c_char, c_int};
use core::ptr;

#[repr(C)]
struct LdSettingsRootOptions {
    organization: *const c_char,
    application: *const c_char,
    resource_root: *const c_char,
    settings_override: *const c_char,
    sync_config_override: *const c_char,
    portable_marker: *const c_char,
    privileged_install_roots: *const *const c_char,
    privileged_install_root_count: usize,
    allow_portable_root: c_int,
    deny_portable_root_in_privileged_install: c_int,
    allow_sync_config_for_portable_root: c_int,
    create_directories: c_int,
    portable_level: c_int,
    named_roots: *const core::ffi::c_void,
    named_root_count: usize,
    component_roots: *const core::ffi::c_void,
    component_root_count: usize,
}

#[repr(C)]
struct LdSettingsRootReport {
    resources: *mut c_char,
    config: *mut c_char,
    data: *mut c_char,
    state: *mut c_char,
    cache: *mut c_char,
    runtime: *mut c_char,
    session: *mut c_char,
    plugin_config: *mut c_char,
    portable_requested: c_int,
    portable_active: c_int,
    settings_override_active: c_int,
    sync_config_override_active: c_int,
    portable_level: c_int,
    named_roots: *mut core::ffi::c_void,
    named_root_count: usize,
    component_roots: *mut core::ffi::c_void,
    component_root_count: usize,
    config_layers: *mut core::ffi::c_void,
    config_layer_count: usize,
    active_read_order: *mut core::ffi::c_void,
    active_read_order_count: usize,
    active_write_layer: *mut core::ffi::c_void,
    diagnostics: *mut core::ffi::c_void,
    diagnostic_count: usize,
}

unsafe extern "C" {
    fn ld_settings_root_options_init(options: *mut LdSettingsRootOptions);
    fn ld_settings_resolve_app_roots(
        options: *const LdSettingsRootOptions,
        report: *mut LdSettingsRootReport,
    ) -> c_int;
    fn ld_settings_free_root_report(report: *mut LdSettingsRootReport);
    fn ld_settings_version_major() -> c_int;
}

#[no_mangle]
pub extern "C" fn ld_settings_rust_ffi_smoke() -> c_int {
    let mut options = LdSettingsRootOptions {
        organization: ptr::null(),
        application: ptr::null(),
        resource_root: ptr::null(),
        settings_override: ptr::null(),
        sync_config_override: ptr::null(),
        portable_marker: ptr::null(),
        privileged_install_roots: ptr::null(),
        privileged_install_root_count: 0,
        allow_portable_root: 0,
        deny_portable_root_in_privileged_install: 0,
        allow_sync_config_for_portable_root: 0,
        create_directories: 0,
        portable_level: 0,
        named_roots: ptr::null(),
        named_root_count: 0,
        component_roots: ptr::null(),
        component_root_count: 0,
    };
    let mut report = LdSettingsRootReport {
        resources: ptr::null_mut(),
        config: ptr::null_mut(),
        data: ptr::null_mut(),
        state: ptr::null_mut(),
        cache: ptr::null_mut(),
        runtime: ptr::null_mut(),
        session: ptr::null_mut(),
        plugin_config: ptr::null_mut(),
        portable_requested: 0,
        portable_active: 0,
        settings_override_active: 0,
        sync_config_override_active: 0,
        portable_level: 0,
        named_roots: ptr::null_mut(),
        named_root_count: 0,
        component_roots: ptr::null_mut(),
        component_root_count: 0,
        config_layers: ptr::null_mut(),
        config_layer_count: 0,
        active_read_order: ptr::null_mut(),
        active_read_order_count: 0,
        active_write_layer: ptr::null_mut(),
        diagnostics: ptr::null_mut(),
        diagnostic_count: 0,
    };

    unsafe {
        ld_settings_root_options_init(&mut options);
        options.organization = b"LinuxDesktop2026\0".as_ptr() as *const c_char;
        options.application = b"rust-ffi-smoke\0".as_ptr() as *const c_char;
        #[cfg(windows)]
        {
            options.settings_override =
                b"C:\\Temp\\linuxdesktop2026-rust-ffi-smoke\0".as_ptr() as *const c_char;
        }
        #[cfg(not(windows))]
        {
        options.settings_override = b"/tmp/linuxdesktop2026-rust-ffi-smoke\0".as_ptr() as *const c_char;
        }
        if ld_settings_version_major() != 0 {
            return 1;
        }
        if ld_settings_resolve_app_roots(&options, &mut report) == 0 {
            return 2;
        }
        if report.config.is_null() || report.settings_override_active != 1 {
            ld_settings_free_root_report(&mut report);
            return 3;
        }
        ld_settings_free_root_report(&mut report);
        if !report.config.is_null() {
            return 4;
        }
    }

    0
}

#[panic_handler]
fn panic(_: &core::panic::PanicInfo<'_>) -> ! {
    loop {}
}
