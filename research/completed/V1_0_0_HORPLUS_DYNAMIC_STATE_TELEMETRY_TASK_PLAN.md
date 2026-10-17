# v1.0.0 HorPlus Dynamic State Telemetry — Task Plan

## Objective

Make the HorPlus diagnostic observer record validated aspect/flags changes on
every diagnostic gameplay-writer callback before any HorPlus eligibility or
early-return path can hide them.

## Established evidence and current state

- HorPlus fixed `32:9 / flags=0x4` idle and ADS behavior was visually validated.
- The latest aspect/flags experiment changed runtime policy, but the emitted
  `HORPLUS_DIAG` records remained limited to the eligible state.
- Absence of a diagnostic record must not be treated as absence of a runtime
  transition.
- The current artifact is diagnostic-only; production Gameplay behavior is
  disabled/unchanged.

## Approved scope

- Reorder or minimally split diagnostic observation so aspect and flags are
  read and change-tracked before HorPlus eligibility decisions.
- Preserve the existing HorPlus transformation and XMM0 rewrite for the
  currently eligible diagnostic state.
- Log non-eligible aspect/flags transitions without applying HorPlus.
- Keep all telemetry diagnostic-only and compile-time gated.

## Explicit non-goals

- No production HorPlus mode or config setting.
- No change to AspectRecalculation, Legacy production behavior, resolver or
  hook ownership.
- No new hook, state machine, cache, performance optimization or target-aspect
  reverse engineering.
- No change to aspect/flags values, replay/recovery, coordinator semantics or
  Dialogue behavior.
- No game launch by the agent.

## Expected files and areas

- `src/plugin/runtime.cpp`: diagnostic snapshot/logging order only.
- This task plan and, if warranted by completed implementation evidence,
  bounded research/task notes.

## Batches

1. Inspect current diagnostic callback and isolate unconditional state-change
   telemetry from HorPlus eligibility/application.
2. Apply the minimal source patch and build a diagnostic artifact with the
   existing diagnostic macros.
3. Perform read-only diff/path review and verify production conditionals and
   transformation semantics are unchanged.

## Validation

- Diagnostic build with the existing observer/HorPlus macros.
- `git diff --check`.
- Read-only diff review against this plan.
- Runtime test is optional and user-run only after static validation confirms
  that non-eligible transitions are observable; no game launch in this task.

## Risks and rollback / safe failure

- Risk: moving telemetry could accidentally alter HorPlus application order.
- Safe failure: keep the existing transformation branch unchanged and log only
  after validated reads; if the patch would require production behavior or a
  new hook, stop without implementation.
- Rollback: revert only the bounded diagnostic source hunk; preserve all other
  user changes in the dirty worktree.

## Stop conditions and phase gates

- Stop if current source does not contain the expected diagnostic path.
- Stop if aspect/flags cannot be read using the already validated observer
  inputs without a new hook or behavior change.
- Stop after build and diff review; do not request a runtime test until the
  observer is statically shown to cover eligible and non-eligible states.

## Expected final Git review

- Confirm only the approved diagnostic source area and this plan changed.
- Confirm production source/config behavior is unchanged.
- Report completed, remaining, deferred and not-runtime-validated items.
