# Review Hardening Ticket Order

This order is the active implementation sequence after the Flavor review round.
Ticket numbers remain historical; execution order follows the list below.

## Current Order

1. `28` — Remove `ld_settings` Desktop Compatibility Facade
2. `29` — Remove `ld_settings` Migration Compatibility Facade
3. `21` — Define Flavor API Exposure Budget
4. `22` — Translate Library Reports At Product Boundaries
5. `23` — Add Common Config Write Facade
6. `24` — Add Settings Root Construction Helpers
7. `26` — Add Ergonomic Migration Action Helpers
8. `25` — Add Clear Config Defaults Alias
9. `27` — Record FlavorTest API Friction Notes
10. `14` — Add Adversarial Parser And Filesystem Tests
11. `15` — Validate APIs With One Maintained Consumer Branch
12. `18` — Expand CI Into Portability Evidence
13. `16` — Decide Keep, Wrap, Or Retire Native Watch Backends
14. `17` — Reopen Roadmap Only From Consumer Evidence

## Rationale

The expanded FlavorTests show that `ld_paths`, safe writes, desktop effects,
and migration mechanics remove real repeated platform work. They also show that
`ld_settings` can become too visible around roots, hydration, write reports, and
migration action objects.

The next work therefore reduces observable framework tax at proven product
seams before the project treats the APIs as ready for adversarial hardening or
real maintained consumer branches.
