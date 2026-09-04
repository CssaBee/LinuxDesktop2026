# 70 - Make Dconf Policy Activation Honest

**What to build:** Make managed/enforced policy capability reporting match what
`ld_desktop` actually activates.

**Blocked by:** 69 - Share Durable File Write Primitive With Desktop.

**Status:** implemented

- [x] Decide whether `ld_desktop` invokes `dconf update`, reports manual
  activation as required, or marks managed policy as backend-limited.
- [x] Leave the activation process-execution branch unimplemented because this
  fix chooses explicit backend-limited reporting instead.
- [x] If activation is not implemented, adjust `query_capabilities()` and apply
  diagnostics so callers cannot mistake file generation for active policy.
- [x] Add tests for capability reporting and apply diagnostics.

Implementation note: activation is not implemented. Linux managed/enforced
policy remains a dconf-compatible source-file generation backend, and reports
`backend_limited` plus `policy-dconf-activation-required` diagnostics.
The activation-specific process-execution branch is intentionally not added.

## Review Anchor

Both reviews point at dconf policy state: the implementation writes source
files but does not run `dconf update`, while `query_capabilities()` currently
reports managed policy as supported on non-Windows platforms.
