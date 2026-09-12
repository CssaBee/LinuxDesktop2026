# 121 - Add Nextcloud Watcher Overload FlavorTest Probe

**What to build:** Add a non-blocking `ld_watch` FlavorTest probe shaped by
Nextcloud Desktop watcher overload evidence, without claiming upstream fit
before the probe measures the seam.

**Blocked by:** None.

**Status:** implemented

- [x] Record the source anchor as Nextcloud Desktop issue `#7873`, including
  the reported high-file-count Linux watcher overload and per-event filtering
  pressure.
- [x] Add a Nextcloud-shaped watcher adapter under `docs/FlavorTests/` or an
  adjacent proof location if the normal FlavorTest harness is too heavy for the
  first pass.
- [x] Model a product-owned "sync candidate" boundary: native watcher events
  enter `ld_watch`, while Nextcloud-style validation decides whether the final
  path state requires sync work.
- [x] Keep the first probe synthetic and hermetic. Do not fetch, vendor, or copy
  Nextcloud source code into the repository.
- [x] Mark qBittorrent `#24444` as a watch item only, not an `ld_watch` ticket,
  unless later source evidence shows watcher involvement instead of
  `ld_desktop` reveal-folder behavior.

## Review Anchor

The September 7, 2026 scan identified Nextcloud Desktop issue `#7873` as a
strong candidate for `ld_watch`: Linux watcher noise over a large tree can
turn raw backend notifications into expensive product work and logging. The
same scan classified qBittorrent `#24444` as a weaker watch item because its
"open containing folder" symptom currently fits `ld_desktop` more than
watching.

## Evidence Fit

FlavorTest evidence should catch this: the risk is whether `ld_watch` still
looks useful when a real product seam must translate noisy native events into
bounded product-owned validation work.

## Release Gate

Does not block `0.2.1`. Promote only if the probe exposes a correctness,
boundedness, or public-claim problem in the existing watcher contract.
