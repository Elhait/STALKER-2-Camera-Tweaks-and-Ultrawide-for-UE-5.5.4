# v1.0.0 HorPlus 21:9 Eligibility — Task Plan

## Objective

Extend the diagnostic-only HorPlus proof-of-concept to the observed `21:9`
state for both `flags=0x4` and `flags=0x5`.

## Established evidence and current state

- The telemetry correction now records aspect/flags transitions before the
  existing HorPlus eligibility filter.
- Runtime evidence observed `2.33333 / 0x4` and `2.33333 / 0x5` states.
- Current HorPlus application is limited to `32:9 / 0x4`.
- `AspectRecalculation` production behavior remains validated and unchanged.

## Approved scope

- Add a diagnostic-only `21:9` aspect constant.
- Allow HorPlus diagnostic transformation for `21:9 / 0x4` and
  `21:9 / 0x5`.
- Preserve the existing `32:9 / 0x4` diagnostic behavior.
- Leave non-eligible states observable but unmodified.

## Explicit non-goals

- No production `Gameplay.Mode` or HorPlus configuration.
- No changes to AspectRecalculation, Legacy behavior, resolver logic, hook
  ownership, aspect/flags writes, replay or coordinator semantics.
- No `16:9` HorPlus transformation.
- No mathematical redesign or cache/performance work.
- No game launch by the agent.

## Expected files and areas

- `src/plugin/runtime.cpp`: diagnostic constant and eligibility predicate only.

## Batches

1. Add the bounded diagnostic eligibility extension.
2. Build the diagnostic artifact and run static diff checks.
3. Perform read-only Git review; runtime validation remains user-run and
   optional.

## Validation

- Diagnostic build with the existing observer/HorPlus macros.
- `git diff --check`.
- Confirm the transformation formula and XMM0 rewrite are unchanged.
- Confirm non-eligible states still leave XMM0 untouched.

## Risks and rollback / safe failure

- Risk: accidentally broadening eligibility beyond the two approved 21:9
  states.
- Safe failure: keep the predicate explicit and compile-time diagnostic-only.
- Rollback: revert only the constant/predicate hunk.

## Stop conditions and phase gates

- Stop if the change requires production config, a new hook or resolver work.
- Stop after build and read-only review; do not infer runtime behavior without a
  user-run test.

## Expected final Git review

- Confirm only the approved diagnostic source area and this plan changed.
- Report build/diff results and mark runtime validation as pending or complete.
