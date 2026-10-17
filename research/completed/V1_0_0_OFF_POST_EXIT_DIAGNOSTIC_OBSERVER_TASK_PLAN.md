# Gameplay-Disabled Post-EXIT Diagnostic Observer — Task Plan

Status: In progress

## Objective

Make the existing validated gameplay-writer post-EXIT telemetry available in a
diagnostic build when `Gameplay.Enabled=false`, without enabling or changing the
production Gameplay feature.

## Established evidence and current state

- The current production path intentionally bypasses the gameplay writer hook
  when `Gameplay.Enabled=false` as required by Design D.
- The previous A/B trace therefore produced `POST_EXIT` telemetry only for the
  Legacy/Gameplay-enabled run.
- The same validated resolver and writer instruction are available regardless of
  the user-facing Gameplay setting; only production hook installation is gated.

## Approved scope

- Add a compile-time diagnostic-only observer hook using the existing validated
  gameplay-writer resolution.
- Install it only in the diagnostic build and only when production Gameplay is
  disabled.
- The observer may call the existing read-only post-EXIT recorder and must then
  leave the original writer instruction and output untouched.
- Preserve the existing change-driven `POST_EXIT` recorder.

## Explicit non-goals

- Do not install or enable the production Gameplay hook.
- Do not set `g_gameplayAvailable` or change FeatureStatus.
- Do not call gameplay correction, replay, aspect/flags writes, or coordinator
  transitions from the diagnostic observer.
- Do not add an ADS hook, input detection, new resolver anchor, setting, or New
  implementation.
- Do not change Legacy behavior or Dialogue behavior.
- Do not launch the game.

## Expected files/areas

- `src/plugin/runtime.cpp`
- `src/hooks/hook_set.hpp`

## Batches

1. Add the compile-time diagnostic observer and its owned hook lifetime.
2. Build a diagnostic ASI with the observer macro enabled and inspect the diff.

## Validation

- Build with `POST_EXIT_GAMEPLAY_OBSERVER_DIAGNOSTIC` enabled only for this
  diagnostic artifact.
- Run `git diff --check`.
- Verify the production build script and production conditional remain unchanged.
- Verify the diagnostic callback has no writes or production state transitions.
- User runs one `Gameplay.Enabled=false` post-EXIT/ADS scenario.

## Risks and safe failure

- If the validated writer resolution or hook creation fails, log refusal and
  leave production behavior unchanged.
- If the observer cannot be made read-only, stop without preparing a runtime
  artifact.
- Diagnostic hook teardown must follow the existing HookSet lifetime boundary.

## Stop conditions and phase gate

- Stop after diagnostic build and read-only Git review.
- Do not implement a gameplay fix from the resulting trace.
- Runtime classification remains pending until the user returns the log.
