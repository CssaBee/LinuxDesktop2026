# 111 - Fix Desktop Exec Field Quoting

**What to build:** Generate freedesktop Desktop Entry `Exec=` fields with the
desktop-entry argument grammar instead of POSIX shell quoting.

**Blocked by:** None.

**Status:** implemented

- [x] Remove `shell_quote()` from the desktop-file `Exec=` generation path.
- [x] Add a dedicated desktop-entry argument quoting helper that uses double
  quotes and the freedesktop escaping rules for quotes, backslashes, dollar
  signs, backticks, and field-code percent characters.
- [x] Add table-driven tests for spaces, single quotes, double quotes,
  backslashes, `$`, backticks, literal `%`, separators, Unicode paths, empty
  arguments, and executable paths with spaces.
- [x] Replace tests that currently certify shell-style single-quote output.
- [x] Where practical, validate generated desktop entries with an installed
  freedesktop parser or desktop-file validation tool in Linux integration CI.

## Implementation Notes

- Replaced POSIX shell quoting in `ld_desktop` with a Desktop Entry `Exec=`
  token formatter that double-quotes protected argv items and escapes `"`, `\`,
  `$`, `` ` ``, and literal `%` according to the desktop-entry command-line
  grammar.
- Stopped applying generic desktop string escaping to the completed `Exec=`
  command line, so argument backslashes are not double-escaped after token
  formatting.
- Updated `ld_desktop` and `ld_settings` autostart expectations away from
  shell-style single quotes.
- Added adversarial coverage for spaced executable paths, spaces, apostrophes,
  quotes, backslashes, dollar signs, backticks, literal percent signs,
  separators, Unicode argv text, and empty arguments.

Verification: built `ld_desktop_tests` and `ld_settings_tests`, ran
`ctest --test-dir build -R '^ld_desktop_tests$' --output-on-failure` and
`ctest --test-dir build -R '^ld_settings_tests$' --output-on-failure`, and
validated the generated adversarial autostart file with
`desktop-file-validate`.

## Review Anchor

The September 6 clean re-review found that `ld_desktop` emits shell-style
single-quoted arguments into `.desktop` `Exec=` keys, even though `Exec=` is not
parsed by a shell. That can make generated autostart or desktop-entry commands
invalid or differently interpreted by desktop implementations.

## Evidence Fit

Specification-backed tests plus source review should catch this: the risk is a
standards integration bug hidden by hermetic tests that assert the wrong
grammar.

## Release Gate

Resolved for `0.2.1`.
