# Confirmed provider Dialogue FOV continuity

Status: offline and runtime validation complete for the reported PID 25756
scenario; user confirms transitions are ideal.

## Contract and evidence

Use the PID 5552 capture and causal assessment in
`research/evidence/u2-confirmed-dialogue-transition-2026-11-16/`.
Fix two independent defects in one bounded continuity task:

1. A provider Dialogue setter-corrected value must receive Gameplay HorPlus
   once, without a second Dialogue projection. Raw provider samples must retain
   their existing mapping. Provenance requires accepted lifecycle, receiver,
   callback source, thread, generation and captured activation identity; FOV
   numeric proximity by itself is not ownership evidence.
2. Cinematic ENTER must accept the qualified writer result from confirmed
   provider Dialogue, preserve its receiver/thread/epoch/generation guards, and
   map every subsequent native eased sample continuously to GameplayHorPlus.

Preserve Gameplay baseline trust, all policies, existing formulas and raw
provider/recovery behavior. No new hooks or general architecture audit.

## Batches and acceptance

- Add independent red regressions to existing harnesses: repeated corrected
  Dialogue samples must stay ~149.071 rather than 152.900; confirmed-context
  corrected start and the complete Cinematic sample trajectory must start at
  the prior output and end at ~153.931 rather than dropping to raw 122.087.
- Extend existing handoff provenance only as needed to recognize corrected
  setter updates, including stable repeated writer samples. Record/retire
  evidence at actual callback boundaries. Do not label all provider input as
  corrected or use historical baseline to reconstruct the current start.
- Keep raw samples, wrong identity, stale lifecycle and disabled-policy
  negative controls. Use production transactions and independent FOV oracles.
- Apply minimal production changes only after reds reproduce both defects.
- Focused harnesses, full test.cmd (graphics only when PC availability is
  established), then ordinary Release ASI and link. Never install into game.
- Runtime confirmation remains pending until the user's next capture; no
  rendered continuity claim from the offline suite.

Expected surfaces: `post_exit_provider_handoff.hpp`, runtime Dialogue callback
and writer adapter, `cinematic_enter_activation.hpp`, and existing writer/
Dialogue/Cinematic harnesses. Risk: retained corrected provenance could hide
raw updates or outlive its provider; mismatch/retirement must be tested.

## Implementation and focused evidence

- The old one-shot setter flag failed repeated owned writer reads in
  `dialogue_boundary_transaction_harness` before the change.
- The confirmed-context Cinematic sample sequence failed independently in
  `cinematic_fov_harness` before the change.
- Replaced the one-shot start record with a corrected-setter sample record in
  the existing handoff owner. Runtime records every accepted setter correction
  through `RecordProviderDialogueSetterResult`, under the existing locks.
  Receiver/thread/generation/captured epoch and callback-source/lifecycle proof
  authorize the record. Repeated writer reads corroborate its value; they
  cannot create it. A different field value retires it and numeric return alone
  cannot resurrect it. Reset/rearm clear it; ignored foreign callbacks do not
  replace it. Ordinary raw provider inputs retain the mapping path.
- Allowed the confirmed context in the existing Cinematic corrected-start
  selector, preserving all other provenance/identity guards.
- Focused Dialogue, Cinematic FOV and camera-writer harnesses PASS. The latter
  exercises repeated real Dialogue boundary outputs, three writer reads after
  each setter, then corrected-start capture and production writer mapping for
  weights 0, .03, .17, .49, .83, 1. It checks ~149.071 -> ~153.931 and clears
  Dialogue ownership before the new Cinematic samples.
- Full `test.cmd`: 62/62 invocations PASS, 0 FAIL, non_graphics=0,
  WARP_case_omitted=0. One hardlink ownership subcase is explicitly SKIP with
  error=5. Native adapter reports 230 fresh processes with Diagnostics OFF/ON.
  Process-tree supervisor reports cleanup verified for the suite stages.
- Ordinary production Release built successfully and published only at the
  project root. Artifact identity and validation summary are recorded in
  `research/evidence/u2-confirmed-dialogue-transition-2026-11-16/repair-validation.json`.
  The installed game ASI was not changed. Build process-tree cleanup verified.

## Runtime confirmation

Capture preserved in `research/evidence/u2-continuity-runtime-2026-11-16-192205/`.
Artifact SHA matches the validated Release 5919E133…0A85549. Both Cinematics
select GameplayNativeNumericBaseline and target 153.931. Cinematic 2 now
captures corrected_start_valid=1, corrected_start=149.071 from dialogue-policy,
with native_start=122.087 and corrected_target=153.931. The prior writer-level
discontinuities are absent in the captured sequence; user reports ideal visual
transitions. Final counters: 6551 transactions, 6551 postStores and postProcess
correlations, zero readback/identity mismatches, zero queue drops, pending=0.
This confirms the captured path, not all game scenes or parameter combinations.

User separately reports increased startup RAM/pagefile use after agent work
and PID-qualified log accumulation. These are independent follow-ups; do not
reopen the validated FOV formulas or transition repair without new evidence.
