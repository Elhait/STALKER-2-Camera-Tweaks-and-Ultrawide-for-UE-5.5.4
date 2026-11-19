# P0/U2 — Native FOV Reference Integration

Status: `+0x234` integration remains gated on per-mode authority evidence.
P0 baseline-admission and U2 fresh-generation repairs are offline-validated;
runtime retest remains pending. Neither repair changes output-FOV transforms.

Scope clarification (2026-11-15): this gate applies only to new production use
of camera `+0x234`. The existing Cinematic `GameplayHorPlus` mode is restored
independently with its prior Gameplay-baseline behavior; its availability does
not claim that `+0x234` is authoritative. The evidence gate below remains in
force for any later `+0x234` integration.

## Objective

Use the native reference at camera `+0x234` to derive corrected output where
its meaning is confirmed for that camera mode, while preserving the existing
writer input at `+0x230` for live native changes such as blend progress and
zoom. Keep current algorithms on modes or samples where `+0x234` is invalid,
stale, or not established as the intended reference.

This continues P0 baseline admission and U2 transition work; it does not add a
new coordinator or replace the primary writer hook.

## Current evidence

- The primary writer reads camera `+0x230` into its native input and writes
  the result to a distinct output object at `+0x30`.
- The pinned-image constructor initializes both camera `+0x230` and `+0x234`
  to `90.0`. The inspected native evaluator does not read `+0x234`.
- Existing runtime logs report `+0x234 = 90` in observed Gameplay, Dialogue,
  CinematicActive and CinematicExiting samples. In those samples `+0x230`
  varies, including Dialogue and Cinematic transitions.
- These captures did not vary the game's own FOV setting. A constant `+0x234`
  value does not prove that the field tracks the current user setting or is an
  authoritative correction reference in any mode.
- The 2026-11-15 loaded-save capture changed the game's FOV from 90 to 100
  after reloading. On the newly observed receiver, `+0x230` reached `100.311`
  while `+0x234` remained `90`. This rejects `+0x234` as an established
  current-setting reference for this tested path; it does not establish its
  semantics in other paths.
- The same capture shows `PostCinematicRecoveryExclusion` active while writer
  transactions continued to report `GameplayEligible`; baseline projections
  then updated from about 90.30 down to 70. The Dialogue boundary correctly
  rejected callbacks under the exclusion, but that suppression was not
  propagated to the Gameplay baseline admission decision.
- At the next cinematic ENTER, `GameplayHorPlus` selected the newly admitted
  70 baseline. After save reload and the FOV setting change, receiver identity
  epoch changed, the prior baseline was retained but unusable, and ENTER fell
  back to authored FOV 90. These are separate observed outcomes; the receiver
  ambiguity after reload is not cleared based on this log alone.
- Capture record: [2026-11-15 runtime assessment](../evidence/p0-u2-native-reference-2026-11-15/assessment.md)
  and archived [raw log](../evidence/p0-u2-native-reference-2026-11-15/STALKER2CameraTweaks.log),
  SHA-256 `E55E7FEBED76524A7C79EA99E6A7C263AA09FDB70B0218054F92FD616375726A`.
  The source log ends at `LOG_LIMIT_REACHED`; its final session summary and later
  events are unavailable.
- The bounded scalar-store inventory found an unclassified candidate that
  writes a float at `+0x230` and a byte at `+0x234`; its owning type is unknown.
  This candidate cannot be attributed to the observed camera object.
- Production currently reads `+0x234` only in diagnostics. `CameraWriterInput`
  carries no validated `+0x234` sample.

## Evidence gate before production integration

1. Use the ordinary ASI and existing Diagnostics setting; do not create a
   diagnostic-only ASI or add a new hook for this check.
2. In a loaded save, change the game's actual FOV setting to a distinguishable
   value (for example `100`), then observe the same camera receiver through
   ordinary Gameplay, Dialogue, Cinematic and recovery. Restore the original
   setting afterward.
3. Check whether `+0x234` follows that setting, remains receiver-bound and
   stable through each mode, while `+0x230` continues to represent the live
   native sample. Keep menu observations separate from loaded-save Gameplay.
4. Classify each mode independently. A result for one mode does not authorize
   use in another. If the setting change does not produce a mode-bound,
   receiver-bound reference, retain the existing policy for that mode.

## Conditional implementation contract

If the evidence gate confirms a mode, capture `+0x234` once from the same
validated native receiver for the current writer invocation and pass the
value plus validity/provenance through the existing `CameraWriterInput`.
Preserve `+0x230` as the live native input and interpolation/zoom sample.
Use `+0x234` only for the confirmed mode's native correction reference or
baseline projection. An invalid sample, identity mismatch, unvalidated mode,
or conflicting source must retain the existing behavior. Never infer
authority from numeric proximity, a constant value, or a coordinator label.

Add regression coverage for both use and fallback paths, including a changing
`+0x234` reference, a live `+0x230` change with stable `+0x234`, mode changes,
invalid reads and source mismatch. Preserve the current Gameplay A/B and
Dialogue/Cinematic policy behavior unless the confirmed reference contract
specifically requires a changed target.

## Non-goals and validation

- No change to Dialogue/Cinematic formulas, Gameplay RuntimeCombine formula,
  recovery interpolation or ownership ordering before the evidence gate.
- No new hooks, coordinator type, cooldown, numeric blacklist, or diagnostic
  ASI.
- No release packaging or user installation in this evidence step.
- After an authorized implementation: run the focused writer/baseline and
  transition harnesses, then the canonical full `test.cmd`; distinguish those
  results from the required later visual runtime confirmation.

## P0 admission follow-up (2026-11-15)

Confirmed defect: while post-cinematic exclusion returns `Suppressed`, the
Gameplay writer path still receives `GameplayEligible` and admits transient
recovery values into `GameplayBaselineStore`. The bounded repair is to pass an
effective `TransitionAmbiguous` context into Gameplay observation/admission
only for those suppressed samples. The existing Gameplay FOV transform remains
active. The endpoint sample that reports `Recovered` and later eligible samples
may resume admission. No cooldown or numeric blacklist is introduced.

Acceptance: deterministic writer regression proves (1) intermediate recovery
values cannot replace a valid Gameplay baseline, (2) Gameplay HorPlus still
transforms the same samples, and (3) admission resumes after the matched
endpoint. This repair does not establish complete native writer coverage or
close the runtime provenance requirement. The `+0x234` evidence gate remains
open, and the FOV setting result above does not authorize integration.

## Offline implementation and validation (2026-11-15)

- `ProcessCameraWriter` now supplies `TransitionAmbiguous` to Gameplay
  observation/admission only while the post-cinematic exclusion reports
  `Suppressed`. The same input FOV still reaches the existing Gameplay
  transform, and a `Recovered` endpoint can resume admission.
- A carried empty cinematic generation is consumed by the next accepted
  ENTER and prevents its stale Gameplay baseline from selecting the new target;
  the mode uses its authored-reference fallback until a fresh eligible
  Gameplay sample exists. GameplayHorPlus remains available with its existing
  baseline-relative behavior when a current baseline is available.
- The runtime adapter fixture's expected target was corrected to match the
  established GameplayHorPlus contract: valid Gameplay baseline when present,
  authored reference for NativeHorPlus or missing/invalid baseline.
- Focused writer harness passed. Full `test.cmd -NonGraphics` passed: registry
  audit (59 targets), build, exit-code contract, and 62/62 harness invocations;
  runtime adapter exercised 228 fresh child cases with Diagnostics OFF/ON.
  Process-tree cleanup was verified. Graphics/WARP cases were omitted by the
  non-graphics profile.
- Ordinary production Release ASI candidate was built at
  `build-artifacts/release-candidate/STALKER2CameraTweaks.asi`, SHA-256
  `C71BA37145E4385E55D1A0AA8F95A914BD77BA6D485C2BCC9D97E941513B8A85`.
  It was not installed or tested in-game. Diagnostics continue to use the
  ordinary ASI's existing INI setting.

These results validate deterministic baseline admission and generation
ownership only. They do not establish rendered continuity, all native writer
coverage, or `+0x234` authority. A bounded runtime retest with this candidate is
still needed before claiming P0/U2 runtime behavior is resolved.

## Runtime capture and follow-up repair (2026-11-16)

The 2026-11-16 runtime capture exposed two separate failures in the tested
`Cinematic → Dialogue → Cinematic` sequence. The full log and assessment are
preserved under [`evidence/p0-u2-runtime-2026-11-16/`](../evidence/p0-u2-runtime-2026-11-16/).

- In loaded-save Gameplay, native `+0x230` was `130.311°`, `+0x234` remained
  `90°`, and RuntimeCombine output was `153.931°`. Cinematic 1 selected the
  Gameplay baseline and targeted `153.931°`.
- After Cinematic 1 recovery completed, the true provider callback for Dialogue
  (`start=130.311°`, `target=70°`) arrived with post-cinematic exclusion still
  active, the same receiver and generation, and coordinator Gameplay. It was
  rejected because the authorization required live recovery authority, which
  ends at `MarkComplete` even though the same owner metadata is retained.
- During that same-receiver provider/recovery transition, generic
  `TransitionAmbiguous` projection marked the previously trusted Gameplay
  baseline non-current. Cinematic 2 therefore fell back to authored 90° and
  targeted `126.87°`, despite the retained numeric baseline `130.311° /
  153.931°`.

The bounded follow-up repair changes only these ownership/admission contracts:

- Provider armer, setter and writer accept completed owner identity only while
  exclusion is active and receiver plus generation still match. This does not
  restore camera recovery authority; all other states remain fail-closed.
- Explicit `ProviderTransitionObserved` and `PostCinematicRecoveryExcluded`
  contexts cannot update Gameplay baseline, but can retain its last trusted
  value for the same receiver. Generic ambiguous samples and receiver changes
  still make that baseline unusable.
- No Gameplay, Dialogue, Cinematic or recovery FOV formula changed. Production
  use of `+0x234` remains gated; this capture again showed it did not follow the
  tested in-game FOV setting of 130.

Focused baseline, writer and Dialogue harnesses passed. The full non-graphics
suite passed 62/62 runner invocations with 0 failures; the hardlink subcase was
`SKIP` (`error=5`) and the WARP case was omitted. The ordinary Release ASI was
built to `build-artifacts/release-candidate/` with SHA-256
`1BB6934CB1EE40802113AF7EDEBFCD4EA0DDD9270FF6B6EDF78C909F4B9F42F5`; it was
not installed or runtime-tested. Process-tree cleanup was verified.

This capture explains why Cinematic 2 selected the 90° fallback and why the
provider handoff did not admit Dialogue Reduced. It does not prove rendered
continuity. Runtime confirmation of the repaired same-session sequence remains
pending; the `+0x234` authority gate also remains open.

### Runtime follow-up and provider-setter interleaving (2026-11-16)

A second capture after the first bounded repair shows Cinematic 2 now retains
the same-receiver Gameplay reference and selects `GameplayNativeNumericBaseline`
(`130.311° / 153.931°`). Its first native writer sample is `70°`; the current
cinematic mapping begins at corrected output `108.941°` and reaches `153.931°`.
The user's visual report says Cinematic transitions improved.

The interstitial Dialogue still did not receive provider handoff. The log shows
the validated provider armer accepted `start=130.311° / target=70°`, then an
older Cinematic EXIT setter for the same receiver but a different component and
old `130.311°` target arrived. That mismatch retired the new provider
activation before its matching setter could pair. No
`ProviderTransitionObserved` writer sample was recorded. A later, separately
started Dialogue after Cinematic 2 did enter the ordinary Dialogue lifecycle,
but it quickly entered recovery and does not validate the interstitial
provider handoff or rendered Reduced behavior there.

The bounded offline repair keeps a provider candidate across a mismatched
setter when the generation, settings epoch, receiver and authorization remain
current. It still requires an exact component/thread/endpoints and callback ↔
setter sample match before writer evidence is accepted; stale authorization or
receiver/generation/settings identity retires the candidate. The regression
replays the observed stale EXIT setter followed by the exact provider setter and
writer sample.

Validation on this revision: `test.cmd -NonGraphics` reported 62/62 runner
invocations PASS, 0 FAIL, with one explicit hardlink SKIP (`error=5`); WARP was
omitted. The final pause was acknowledged to allow the wrapper to exit. The
ordinary Release ASI was built and published to
`build-artifacts/release-candidate/` (SHA-256
`6AAB96F0BF8263AEF2BA9C819A00DA98C7E463652859DECFC0FFB2089BB4A659`). The
build and test process trees were cleaned; the game was not launched or
terminated. Full capture and a separate assessment are stored beside the prior
runtime evidence.

This closes the reproduced setter-interleaving defect offline, not the runtime
or visual validation gate. The new candidate still needs one in-game replay of
the same Cinematic → Dialogue → Cinematic sequence to verify that the provider
handoff now survives the old EXIT setter and that Dialogue Reduced is visible.

### Provider trajectory after completed recovery (2026-11-16)

The subsequent runtime capture preserves the accepted provider activation and
an exact setter match for generation 1, receiver and thread, but its writer
trajectory occurs after native Cinematic recovery has completed. The coordinator
is already Gameplay while the matching recovery-owner identity and baseline
exclusion remain active. The writer therefore bypassed the provider mapper and
used ordinary Gameplay HorPlus at the Dialogue endpoint: native `70°` became
`108.941°`; no provider handoff was applied. The captured `Reduced` setting
was not reflected in this writer output.

The production writer now admits the exact provider trajectory in this
completed-recovery state, guarded by `ProviderTransitionObserved`, the retained
completed owner, active exclusion, matching generation/receiver/thread/settings,
valid same-receiver baseline and monotonic provider evidence. It maps before
Gameplay policy dispatch and retires the completed owner/exclusion at the
matched provider endpoint. No FOV formula or user-facing mode was changed.

The production transaction regression failed on both intermediate and endpoint
checks before this change and passes after it. The focused
`camera_writer_transaction_harness` passes with process cleanup verified. The
ordinary Release ASI target also compiled and linked; its local candidate hash
and the bounded compiler-helper cleanup note are recorded in the assessment.
Full test registry and runtime/visual confirmation remain pending. The
separate generation-2 provider attempt after its owner was
already invalid remains fail-closed and is not claimed fixed by this change.
See the [runtime assessment and preserved log](../evidence/dialogue-provider-after-recovery-2026-11-16/assessment.md).

### Dialogue follow-up capture: log saturation (2026-11-16)

The next user-provided log reports the ordinary candidate hash and startup
configuration `Dialogue.Enabled=true`, `Dialogue.Mode=Reduced`. The user reports
that the Dialogue between cinematics still did not reflect the configured
Dialogue behavior. The preserved log contains 12,863 post-process records, all
with `coordinator=Gameplay` / `writerContext=GameplayEligible`; it contains no
Dialogue boundary arm, Dialogue lifecycle, cinematic writer ownership, or
provider-handoff events before `LOG_LIMIT_REACHED`. The 16 MiB session log cap
suppressed all subsequent records, so it is unknown whether the tested sequence
occurred after saturation. The last periodic summary reports 12,454 transactions
and four contended event drops; it cannot characterize later activity.

This capture confirms the setting was loaded and records the user's visual
result, but cannot establish where the tested transition failed. No further
FOV-policy change is made from this evidence. Before another behavioral fix,
arm the ordinary ASI's existing one-shot FOV capture before Cinematic 1 and
inspect the bounded Cinematic ENTER / EXIT / Dialogue handoff records. The raw
log and focused assessment are preserved under
[`evidence/p0-u2-runtime-2026-11-16/`](../evidence/p0-u2-runtime-2026-11-16/).

The saturation cause is now addressed in the ordinary diagnostic logger: adjacent `POST_PROCESS` transactions with bit-identical payloads are emitted as one bounded run summary with the complete representative payload, count, and exact first/last event sequence and transaction IDs. Any payload change starts a distinct record; runs flush at least once per second. The existing 16 MiB cap is unchanged. The focused coalescer regression and Release build pass offline; a runtime capture is still needed to confirm the log now retains the entire Cinematic → Dialogue → Cinematic sequence. This diagnostic change does not itself fix or validate Dialogue policy admission.

The 2026-11-16 follow-up runtime log no longer reached its 16 MiB cap and captured the missing Dialogue handoff. It showed a matched provider setter followed by every Dialogue callback being rejected as `ExclusionReset`, so the frozen `Reduced` policy never acquired Dialogue ownership. The provider-only exclusion bypass is now identity-bound to callback source, receiver, generation, settings epoch and Gameplay coordinator; interleaved callbacks do not clear it, and foreign receiver/lifecycle state still resets it. Focused Dialogue, camera-writer and cinematic-lifecycle harnesses pass, and the ordinary Release candidate was rebuilt. See the appended root-cause analysis in [`assessment-dialogue-followup.md`](../evidence/p0-u2-runtime-2026-11-16/assessment-dialogue-followup.md). Runtime validation of the new Dialogue output remains pending; the full test.cmd registry was not completed for this change.
