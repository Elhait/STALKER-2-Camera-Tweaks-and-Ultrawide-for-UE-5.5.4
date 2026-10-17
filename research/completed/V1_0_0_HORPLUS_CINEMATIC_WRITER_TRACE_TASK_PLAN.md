# v1.0.0 HorPlus Cinematic Writer Trace — Task Plan

## Objective

Localize the confirmed `HorPlus`/cinematic interaction by recording the
validated gameplay writer boundary during `CinematicActive`, without changing
production behavior or cinematic logic.

## Established evidence and current state

- With Gameplay disabled, cinematic FOV is visually correct.
- With `Gameplay.Mode=HorPlus`, cinematic ENTER logs `90 -> 126.87`, but the
  cinematic visual FOV is incorrect; post-EXIT gameplay and ADS are correct.
- The gameplay writer hook is the only relevant A/B difference currently
  established, but its exact mechanism is not established.
- `SafetyHook` mid-hook return resumes through the trampoline; hook presence
  alone is not proof of an overwrite.

## Approved scope

- Add diagnostic-only tracing at the existing gameplay writer callback.
- Record relevant hits during `CinematicActive` independently of HorPlus
  eligibility:
  timestamp/sequence, coordinator, XMM0 before/after, eligibility/applied,
  camera FOV fields, aspect, flags, output FOV/aspect.
- Keep normal production behavior unchanged when the diagnostic macro is off.
- Prepare one diagnostic ASI for a user-run test.

## Explicit non-goals

- Do not repair the conflict.
- Do not modify Cinematics, HorPlus math, AspectRecalculation, Dialogue,
  coordinator semantics, hook locations, or native camera state.
- Do not add a projection-owner hook, new RE anchor, polling, timer or cache.
- Do not place logging inside the eligibility branch only.
- Do not launch the game.

## Expected files or areas

- `src/plugin/runtime.cpp`
- Diagnostic build invocation/output only; no production config change.

## Implementation and validation batches

1. Add compile-time-gated before/after writer tracing with bounded,
   change-driven logging and no context mutation.
2. Build the diagnostic ASI with the macro enabled, run static diff checks,
   and verify the production source path is unchanged when the macro is off.

## Risks and safe failure

- Logging must not alter `SafetyHookContext`; the trace is observational only.
- Bound output to state changes and relevant cinematic-active hits to avoid
  recreating per-hit diagnostic overhead.
- If the diagnostic path cannot read a field safely, log an unavailable value
  rather than changing callback behavior.

## Stop conditions and phase gates

- Stop after diagnostic build and static checks.
- Do not infer the conflict mechanism before the user supplies the trace.
- Do not implement a repair in this task.

## Expected final Git review

- Confirm only diagnostic instrumentation was added.
- Confirm no production behavior/config/cinematic logic changed.
- Report the diagnostic ASI as ready for one runtime test, not as a fix.
