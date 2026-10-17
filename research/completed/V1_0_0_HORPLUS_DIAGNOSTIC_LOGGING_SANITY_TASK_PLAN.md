# HorPlus Diagnostic Logging Sanity Task Plan

## Objective

Isolate the cost of per-hit diagnostic logging from the already validated
HorPlus diagnostic callback and `XMM0` rewrite.

## Established evidence and current state

- HorPlus Gate 1 passed at 32:9/FOV 90.
- Idle, ADS, ADS transition and release framing were visually correct.
- The diagnostic observer applies HorPlus on each validated camera-writer hit.
- The current diagnostic path logs one `HORPLUS_DIAG` line per application,
  which can materially distort performance.

## Approved scope

- Remove per-hit `spdlog` from the HorPlus diagnostic hot path.
- Keep cumulative diagnostic counters for total writer callbacks, total
  HorPlus applications and distinct change-driven transitions.
- Emit a log entry only when input FOV, transformed FOV, aspect, flags or
  source changes.
- Preserve the existing HorPlus formula, hook point, XMM0 rewrite and
  Gameplay.Enabled=false diagnostic observer semantics.

## Explicit non-goals

- No HorPlus mathematical change.
- No cache or new optimization logic.
- No hook-point change.
- No aspect/flags writes, replay/recovery, production configuration or
  `Gameplay.Mode` setting.
- No changes to AspectRecalculation, cinematic, dialogue or scanner behavior.

## Expected files or areas

- `src/plugin/runtime.cpp` only, plus this task plan.

## Implementation batch

1. Add diagnostic-only counters and change-detection state.
2. Replace per-hit HorPlus log with change-driven cumulative telemetry.

## Validation

- Build diagnostic artifact with the existing observer and HorPlus macros.
- Run `git diff --check`.
- Inspect the diff for scope compliance.
- User performs the same 32:9/FOV 90 idle → ADS hold → ADS release test.

## Risks and safe-failure behavior

- Counters and logging are diagnostic-only and must be compiled out of the
  normal build.
- The transformation and register rewrite must remain unchanged.
- If the build fails or the diff touches production configuration or another
  hook, stop without expanding the task.

## Stop conditions and phase gates

- Stop after diagnostic build and read-only Git review.
- Do not add caching, profiling, new hooks or production optimization in this
  batch.
- Runtime performance comparison remains user-owned and not agent-validated.

## Expected final Git review

- Confirm only the approved diagnostic source and plan paths changed.
- Report build/diff results separately from user runtime evidence.
- Keep HorPlus production readiness open until the no-log-spam runtime result.
