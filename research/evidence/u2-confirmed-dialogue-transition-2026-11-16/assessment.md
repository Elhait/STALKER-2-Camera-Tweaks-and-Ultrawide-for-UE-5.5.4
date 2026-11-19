# Confirmed Dialogue: two output continuity defects

Artifact: 0E6F2A6943F9531E31DDFD3D05A269C80C05BB8D958F6C01353E8C40F599C2C7.
Active process: 5552, using PID-qualified logs. Startup reaches renderer first
frame and input activation. There are 5,596 primary/post-store/post-process
transactions, no readback mismatches, one contended queue event dropped. Do not
claim a completely lossless capture. User confirms the mod now works.

## Confirmed writer-level discontinuities

| Boundary | Evidence |
| --- | --- |
| Cinematic 1 | GameplayNativeNumericBaseline 130.311 -> 153.931 |
| Provider transition | 19:03:40.199: smooth mapped output starts 153.927 toward Reduced 149.071 |
| Dialogue admission | transaction 2964: input 122.092 -> output/readback 149.074 |
| Next Dialogue writer | transaction 2965: input 122.088 -> output/readback 152.900 |
| Cinematic 2 ENTER | 19:03:50.017: baseline remains usable; correct GameplayNativeNumericBaseline target 153.931 |
| Corrected start | 19:03:50.018: native_start 122.087, corrected_start_valid=0 |
| Subsequent writer | 19:03:50.080: 152.900 -> 122.087, CINEMATIC_PASS_THROUGH_UNCLASSIFIED; subsequent raw interpolation rises toward target |

## Source causes

1. Runtime arms `ArmDialogueWriterStart` only for ProviderDialogueStarted.
   `ConsumeDialogueWriterStart` suppresses writer-side Dialogue mapping for
   that one sample. Later Dialogue boundary callbacks keep mutating XMM1 to
   the Reduced projection, but the writer continuation maps their corrected
   inputs again as though they were native provider samples. This explains
   the 149.074 -> 152.900 jump and wrong stable Dialogue output. Repair must
   correlate setter-corrected samples/ownership, not infer provenance from
   numeric equality or broadly disable provider mapping.
2. `MakeDialogueWriterResultCandidate` requires exactly
   `DialogueLifecycleObserved`. Baseline-trust repair introduced
   `ConfirmedProviderDialogueLifecycleObserved`, which is now present in the
   actual writer observation, but this consumer was not updated. It rejects
   the authoritative prior corrected output; activation has no corrected start
   and routes native interpolation through pass-through. This explains the
   large second boundary jump. Existing receiver/thread/epoch/generation and
   observation guards must remain when recognizing the confirmed context.

Baseline trust preservation is working in this capture. Neither a baseline
fallback nor the native post-helper explains these recorded discontinuities.
Rendered continuity still needs user confirmation after repair; these are
writer/readback causal findings correlated with the reported visual jumps.

## Repair scope and regression requirements

Use existing transaction harnesses. First reproduce repeated setter-corrected
provider Dialogue samples (not only its one-shot seed), then use that actual
confirmed-context observation for Cinematic 2 corrected-start selection and
the entire native eased sample sequence. Assert no double Dialogue transform,
Reduced destination ~149.071, and continuous Cinematic mapping to 153.931.
Retain raw provider trajectory mapping, foreign identity and stale generation
negative controls. Preserve GameplayHorPlus, baseline trust and FOV formulas.
No new runtime capture is needed to localize these two defects. Production
repair and red/green regression runs have not yet been performed in this stage.
