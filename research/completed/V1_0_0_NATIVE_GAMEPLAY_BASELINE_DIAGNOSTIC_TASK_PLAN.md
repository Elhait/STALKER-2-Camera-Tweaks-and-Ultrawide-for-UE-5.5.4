# Native Gameplay Baseline Diagnostic — Task Plan

Status: In progress

## Objective

Capture one read-only steady-state native gameplay baseline at user FOV=90 with
`Gameplay.Enabled=false`, without entering a cinematic or activating ADS.

## Established evidence and current state

- The validated gameplay-writer resolver can be used by a diagnostic-only
  observer while production Gameplay remains disabled.
- The existing change-driven recorder already captures FOV, aspect, flags,
  selector, output fields, and runtime states after an armed boundary.
- R1 requires the same recorder during ordinary native gameplay, before any
  cinematic transition.

## Approved scope

- Diagnostic build only, using the existing validated writer observer.
- Arm the existing change-driven recorder at diagnostic startup when
  `Gameplay.Enabled=false`.
- Capture ordinary stable gameplay telemetry only.

## Explicit non-goals

- No cinematic, ADS, sprint, interaction, viewmodel or modifier testing in this
  task.
- No FOV/aspect/flags writes or gameplay correction.
- No new hook, resolver, setting, or production lifecycle change.
- Do not enable the production Gameplay feature.
- Do not implement Hor+ or New.
- Do not launch the game.

## Expected files/areas

- `src/plugin/runtime.cpp`
- No production build-script changes.

## Validation

- Build a diagnostic artifact with the existing read-only observer macro and the
  R1 baseline macro.
- Run `git diff --check`.
- User runs one FOV=90 ordinary gameplay session and returns the log.

## Stop conditions

- Stop after build and read-only diff review.
- R2 mapping and R3 modifier research remain blocked until R1 evidence is
  classified.
