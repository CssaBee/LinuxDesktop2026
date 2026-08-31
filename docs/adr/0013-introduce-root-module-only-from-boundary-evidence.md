# Introduce root module only from boundary evidence

The next settings hardening lane treats `ld_root` as a proposed public module, not an internal `ld_settings` file split. `ld_root` may own reusable application root topology across user-owned and app-owned roots only if the design proves that this is more than a rename of `ld_paths` or a cleanup of `settings.cpp`; otherwise the code should be split privately while public APIs stay in their current modules.

## Consequences

The extraction order is documentation and evidence first, then internal seams, then public API. `ld_settings` should not keep growing merely because root resolution is convenient there, but `ld_paths` should also not absorb settings-specific root topology unless repeated FlavorTest or cross-port evidence shows it is truly generic path policy.
