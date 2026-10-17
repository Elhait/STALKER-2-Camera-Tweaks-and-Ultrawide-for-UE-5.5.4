# Delayed Aspect Handoff Diagnostic — Task Plan

Status: In progress

## Objective

Run one isolated diagnostic proof-of-concept for the hypothesis that the
validated `3.55556 -> 1.77778 / flags=0x4` handoff is correct in substance but
is applied too early during native cinematic FOV descent.

## Established evidence and current state

- Legacy applies the atomic aspect handoff on the first descending gameplay
  writer sample and produces the known visible EXIT discontinuity.
- Gameplay-disabled native control recovers smoothly but later loses correct
  framing after a camera/FOV rebuild.
- The same validated atomic write is already production-tested.

## Approved scope

- Diagnostic build only, behind `POST_EXIT_DELAYED_ASPECT_DIAGNOSTIC`.
- Suppress the existing early atomic EXIT handoff only in that diagnostic build.
- At the existing confirmed native recovery boundary near the selected gameplay
  FOV, perform exactly one validated atomic aspect/flags write.
- Log point A (stable native recovery), point B before/after the delayed write,
  and the existing change-driven post-EXIT trace for point C.

## Explicit non-goals

- Do not change the production Legacy path or `build.cmd`.
- Do not add timers, ADS detection, polling, new hooks, new resolvers, or new
  configuration settings.
- Do not change the FOV formula, gameplay FOV, dialogue behavior, coordinator
  contract, or successful production semantics.
- Do not create the New implementation from this experiment.
- Do not launch the game.

## Expected files/areas

- `src/plugin/runtime.cpp`
- `src/plugin/hook_set.hpp` is not expected to change.

## Batches

1. Add the compile-time diagnostic branch and one-shot delayed write.
2. Build the diagnostic ASI with the macro enabled and review the diff.

## Validation

- Build with `POST_EXIT_DELAYED_ASPECT_DIAGNOSTIC` enabled only for this
  artifact.
- Run `git diff --check`.
- Verify the ordinary production branch remains textually unchanged except for
  compile-time guarded diagnostic code.
- Runtime validation is user-owned: assess A, B, and C visually and return the
  log.

## Risks and safe failure

- If native recovery state is unreadable or the source aspect is not the
  expected ultrawide value, refuse the diagnostic write and retain native state.
- The diagnostic write is one-shot and uses existing validated write semantics.
- No production correction is reachable when the macro is absent.

## Stop conditions and phase gate

- Stop after diagnostic build and read-only Git review.
- Do not interpret runtime behavior or implement a fix in this task.
- If the delayed write cannot be bounded to the existing recovery boundary,
  stop without preparing a runtime artifact.
