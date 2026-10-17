# v1.0.0 HorPlus Cinematic Double-Transform — Task Plan

## Objective

Prevent HorPlus from transforming a cinematic FOV value twice when the
validated cinematic ENTER hook has already produced the aspect-aware value.

## Established evidence and current state

- Runtime trace shows cinematic ENTER computes `90 -> 126.87`.
- During `CinematicActive`, the gameplay writer receives `XMM0=126.87` and
  the current HorPlus path applies the transform again, producing `151.927`.
- The user reports the resulting cinematic FOV is excessively wide.
- The same production path is correct when the writer receives native `90`.

## Approved scope

- Track the current cinematic ENTER transformed FOV within existing runtime
  ownership state.
- During `CinematicActive`, bypass HorPlus only when the writer input matches
  that already-transformed cinematic value.
- Preserve HorPlus transformation for native cinematic writer input and for
  normal Gameplay/ADS input.
- Add bounded harness coverage for the pure idempotence decision if feasible.

## Explicit non-goals

- Do not change HorPlus mathematics or cinematic FOV mathematics.
- Do not change AspectRecalculation, aspect/flags writes, hook locations,
  resolver behavior, Dialogue or coordinator transitions.
- Do not add another hook, timer, cache of gameplay FOV, or new RE anchor.
- Do not treat `Auto` 32:9 behavior as a configuration error.

## Expected files or areas

- `src/plugin/runtime.cpp`
- Existing diagnostic trace may remain until the user confirms this repair.

## Implementation and validation batches

1. Add lifecycle-scoped cinematic transformed-FOV tracking and the narrow
   duplicate-input bypass.
2. Run harnesses, production and diagnostic builds, then `git diff --check`.
3. User performs one 32:9 `Auto` HorPlus cinematic/EXIT/ADS test.

## Risks and safe failure

- Risk: stale transformed-FOV state. Reset it at cinematic EXIT and only use it
  while `CinematicActive`.
- If the tracked value is unavailable or does not match, retain the current
  native-input HorPlus behavior rather than suppressing transformation.

## Stop conditions and phase gates

- Stop after build/static validation; do not add further heuristics.
- Runtime success is pending user confirmation.

## Expected final Git review

- Confirm only duplicate-transform prevention changed.
- Confirm the validated native-input HorPlus and AspectRecalculation paths are
  otherwise unchanged.
