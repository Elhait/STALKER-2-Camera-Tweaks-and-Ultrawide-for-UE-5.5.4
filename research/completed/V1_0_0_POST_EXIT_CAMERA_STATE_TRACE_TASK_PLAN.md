# Post-EXIT Camera-State Trace — Task Plan

Status: In progress

## Objective

Prepare a diagnostic ASI that records the complete change-driven camera-state
evolution after cinematic EXIT for two user-run A/B scenarios:

- `Gameplay.Enabled=false`, `Cinematics.AspectRatio=Auto`, `Dialogue.Zoom=Disabled`;
- the same scenario with `Gameplay.Enabled=true`.

The trace must cover native recovery, stable post-EXIT gameplay, subsequent
camera/FOV recalculation, and the state after the user returns to gameplay.

## Established evidence and current state

- The existing post-EXIT writer observation is already attached to a validated
  gameplay-writer callback.
- The current observation stops after 32 writer invocations and logs every
  observed invocation while armed.
- Runtime A/B evidence shows different persistence after a later camera rebuild
  between Gameplay disabled and Legacy gameplay correction paths.
- No validated ADS-specific hook is available or required for this diagnostic.

## Approved scope

- Replace the bounded 32-invocation post-EXIT observation with a change-driven
  post-EXIT camera-state trace.
- Record timestamp/sequence, FOV inputs and outputs, aspect, flags, writer and
  output identities, coordinator/replay/dialogue state, and changed fields.
- Keep the trace armed for the post-EXIT interval until the next cinematic EXIT
  resets its baseline.
- Preserve existing hook locations, callback behavior, and production writes.

## Explicit non-goals

- No ADS-specific hook or input detection.
- No new gameplay, cinematic, dialogue, or configuration behavior.
- No Legacy state-machine changes.
- No gameplay/aspect/FOV writes from the diagnostic.
- No resolver, scanner, hook-ordering, or coordinator changes.
- No game launch by the agent.

## Expected files/areas

- `src/plugin/runtime.cpp`
- `src/plugin/runtime.hpp` only if the existing runtime-owned diagnostic state
  requires a declaration change.

## Batches

1. Add a runtime-owned change snapshot and replace the current post-EXIT
   32-sample logger with change-driven logging.
2. Build the diagnostic ASI and inspect the read-only diff.

## Validation

- Build from the repository root with `build.cmd`.
- Run `git diff --check`.
- Confirm no production callback writes or state-machine branches changed.
- Runtime validation is user-owned and will consist of identical OFF/Legacy
  post-EXIT A/B runs. The agent must not launch the game.

## Risks and safe failure

- Diagnostic logging can add hot-path overhead; change filtering and the
  existing logger are retained, with no per-frame log for unchanged state.
- If snapshot comparison cannot be made safe without changing runtime behavior,
  stop before implementation and report the boundary.
- If build or diff validation fails, do not prepare a runtime artifact claim.

## Stop conditions and phase gate

- Stop after build, diff review, and factual report.
- Do not implement a fix based on the trace.
- Do not remove the diagnostic until the user completes both A/B runs and the
  resulting evidence is classified.

## Final Git review

Review status, affected paths, diff summary, and recent history read-only.
Separate diagnostic completion from runtime validation, which remains pending.
