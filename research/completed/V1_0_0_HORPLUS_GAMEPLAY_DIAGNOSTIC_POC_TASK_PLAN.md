# HorPlus Gameplay Diagnostic POC Task Plan

## Objective

Build a diagnostic-only proof of concept for a direct aspect-aware gameplay FOV
transformation on the validated camera-writer hook. The POC targets 32:9 with
user FOV 90 and must not modify the production `AspectRecalculation` path or
add a production configuration mode.

## Established evidence and current state

- `AspectRecalculation` is the validated, frozen production gameplay mode.
- Native gameplay keeps the tracked writer/world/first-person FOV at the user's
  selected value across 16:9, 21:9 and 32:9.
- Native post-cinematic framing can look correct until ADS forces a camera/
  projection rebuild, after which vanilla ultrawide framing is exposed.
- The camera-writer resolver and `MOVSS [RBX+0x30], XMM0` hook contract are
  already validated for the current executable identity.
- `cinematics::HorPlus()` is an existing tested mathematical helper.

## Approved scope

- Add a compile-time diagnostic-only `HorPlus` branch.
- Reuse the existing validated camera-writer diagnostic hook when
  `Gameplay.Enabled=false`.
- For ordinary `CoordinatorState::Gameplay`, read the already validated source
  aspect/flags and transform the incoming `XMM0` FOV with the existing
  `cinematics::HorPlus()` helper for the 32:9 POC.
- Do not write the source aspect or flags and do not arm replay/recovery.
- Log the diagnostic input/output FOV and source aspect/flags.

## Explicit non-goals

- No `Gameplay.Mode` setting or `HorPlus` production mode.
- No changes to `AspectRecalculation`.
- No changes to cinematic, dialogue, scanner, resolver or hook ownership logic.
- No ADS-specific hook, timer, polling, new signature or new memory path.
- No production behavior change in normal builds.
- No game launch by the agent.

## Expected files or areas

- `src/plugin/runtime.cpp`: diagnostic-only transformation and logging.
- No other production source expected unless the existing diagnostic hook
  declaration requires it.

## Implementation batch

1. Add the diagnostic branch behind a unique compile-time macro and keep it
   out of the normal build.
2. Reuse `cinematics::HorPlus()` with native 16:9 reference aspect, only for
   finite valid FOV and the validated 32:9/unconstrained source state.
3. Preserve native pass-through for cinematic-active/exiting states and all
   non-matching states.

## Validation

- Build a diagnostic artifact with the new macro and the existing read-only
  gameplay observer macro.
- Run `git diff --check`.
- Inspect the diff and confirm no normal-build production path changed.
- User performs one runtime POC at 32:9/FOV 90 with cinematic/dialogue Native,
  observing idle framing and ADS/release framing.

## Risks and safe-failure behavior

- The POC intentionally changes only the diagnostic build's FOV register path;
  it must never write aspect/flags or run production replay logic.
- Invalid/unreadable source state, non-finite/out-of-range FOV, non-32:9
  aspect, constrained flags, or non-Gameplay coordinator state must pass
  through unchanged.
- If the diagnostic hook cannot be resolved or installed, the POC must report
  refusal and leave production behavior untouched.

## Stop conditions and phase gates

- Stop if implementation requires a new hook, signature, configuration setting,
  production path change, or modification of `AspectRecalculation`.
- Stop after the diagnostic build and static diff review; runtime validation is
  performed by the user.
- A failed POC does not authorize production changes.

## Expected final Git review

- Confirm only the approved diagnostic source/plan paths changed.
- Report build and diff-check results separately from runtime validation.
- Keep `HorPlus` classified as diagnostic POC until the user reports idle and
  ADS results.
