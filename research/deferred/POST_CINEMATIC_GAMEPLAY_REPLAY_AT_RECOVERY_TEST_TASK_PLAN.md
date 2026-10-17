# POST_CINEMATIC_GAMEPLAY_REPLAY_AT_RECOVERY_TEST

## Objective

Build a research-only variant that applies the already validated final gameplay
aspect/flags state in the same writer invocation that detects native cinematic
FOV recovery completion. This tests whether the atomic result can be moved
earlier without reintroducing the FOV handoff conflict.

## Established evidence

- The deferred atomic build removed the visible replay oscillation.
- Its final apply occurred about 194 ms after recovery completion.
- The atomic final state was `aspect=1.77778 / flags=0x4`, with FOV remaining
  `90`.
- The remaining question is whether the same final-state write is safe at the
  recovery-complete boundary itself.

## Approved scope

- Add one compile-time research flag only.
- At the existing condition `coordinator=CinematicExiting` and
  `abs(currentFov-targetFov)<=kRecoveryEpsilon`, write final aspect/flags once
  in that same writer invocation.
- Preserve authored/native FOV and all existing cinematic/dialogue logic.
- Log an explicit `AtomicReplayApplied phase=RecoveryComplete` marker.
- Build a separate 2.0.5 research ASI.

## Explicit non-goals

- No write before native FOV recovery completes.
- No timer, sample counter, clamp, new hook or production change.
- No modification of the released ASI or existing deferred research artifact.

## Validation

- Build through the established VS 2022 toolchain.
- Perform read-only Git review after the build.
- Runtime validation is user-run only and must compare visual transition and
  log ordering against the deferred atomic build.

## Stop conditions

- If the immediate write fails, leave native state untouched.
- If visual artifacts or FOV interference return, reject the earlier timing and
  retain the deferred atomic build as the better research result.
- Do not promote either research variant to production without explicit runtime
  evidence and a separate production decision.
