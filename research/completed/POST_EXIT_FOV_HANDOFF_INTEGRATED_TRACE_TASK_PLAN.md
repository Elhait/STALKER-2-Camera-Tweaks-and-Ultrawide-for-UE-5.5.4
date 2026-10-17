# POST_EXIT_FOV_HANDOFF_INTEGRATED_TRACE

## Objective

Add bounded observation-only telemetry to the production `STALKER2CameraTweaks`
source so the released cinematic transform can be correlated with the first
post-EXIT FOV samples without loading a second ASI on the same hooks.

## Established evidence and current state

- Steam 2.0.5 executable identity is SHA-256
  `E7B481A97C02D80581FAB0BECE940214A88EBE30211088A00129845A039F9293`.
- The current indexed cinematic EXIT boundary resolves uniquely in static
  analysis at RVA `0x366FA2D` and is contained by `FUN_14366F9AA`.
- The standalone read-only tracer is safe after moving its hook from the
  internal CALL to the beginning of the boundary, but its native 16:9 control
  run did not reproduce the production `126.x -> 90` handoff.
- Production `STALKER2CameraTweaks.asi` already has the validated gameplay,
  cinematic and dialogue behavior and must remain the behavior control.

## Approved scope

- Modify only `src/experimental_cinematic_21_9_combined_fix_204.cpp`.
- Add observation-only post-EXIT correlation inside existing production hooks.
- Record EXIT timestamp/sequence and at most 32 subsequent writer invocations.
- Record existing input/source fields, aspect/flags/selector, coordinator state,
  cinematic state and dialogue phase where already available.
- Preserve all existing production transforms, replay, recovery and dialogue
  behavior byte-for-byte in intent.

## Explicit non-goals

- No new FOV/aspect/flag writes.
- No timer guard, clamp, interpolation override or frame-based behavior.
- No new hook target or second ASI.
- No gameplay, dialogue or cinematic policy changes.
- No release archive or version change.

## Batches and validation

1. **Instrumentation patch** — add bounded post-EXIT telemetry using the
   existing validated EXIT and camera-writer hooks. Review the diff for no
   writes or policy changes.
2. **Build validation** — compile the normal production ASI and record its
   SHA-256. A successful build proves compilation only.
3. **Runtime validation** — user runs the production ASI in the known 32:9
   cinematic scenario. Confirm whether the log captures `126.x -> 90`, whether
   `XMM0` and `+0x230` agree, and whether recovery completes normally.

## Risks and safe failure

- Telemetry must be bounded and read-only. If resolver/hook setup fails, retain
  the existing production fail-safe behavior.
- If telemetry causes any crash or artifact, revert only this instrumentation
  batch and restore the prior production build; do not alter stable logic.
- The trace is diagnostic evidence only and must not be treated as a release
  feature until runtime-validated.

## Stop conditions and phase gates

- Stop after the first integrated runtime run.
- If no `126.x -> 90` stream is captured, classify the observation as
  `NOT REPRODUCED`; do not add more hooks or broaden the trace automatically.
- If a new upstream owner/state is identified, open a separate research plan
  before changing behavior.

## Final review

- Read-only Git status/diff review after implementation and build.
- Report changed paths, build hash, runtime-validation limits and whether the
  release candidate was intentionally untouched.
