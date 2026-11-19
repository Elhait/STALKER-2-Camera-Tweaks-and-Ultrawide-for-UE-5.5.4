# Research archive

This archive preserves the reverse-engineering path behind the unified v0.4.0
release. Plans are organized by the result of the bounded investigation, not
only by whether implementation work was performed.

## Completed

Established findings, validated implementation phases and completed supporting
work are in [`completed/`](completed/).

## Rejected

These branches produced negative or unsafe evidence and should not be repeated
without new contradictory evidence: [`rejected/`](rejected/).

## Deferred

Open questions or deliberately paused investigations are in
[`deferred/`](deferred/).

The overlay window-discovery and cinematic aspect issue is deferred until the
current EXIT1 → Dialogue → ENTER2 correction is visually validated. Its task
plan is [`OVERLAY_DISCOVERY_CINEMATIC_ASPECT_TASK_PLAN_2026-11-13.md`](deferred/OVERLAY_DISCOVERY_CINEMATIC_ASPECT_TASK_PLAN_2026-11-13.md),
with the current startup/runtime/overlay logs preserved under
[`evidence/overlay-cinematic-aspect-2026-11-13/`](evidence/overlay-cinematic-aspect-2026-11-13/).

The reported rendered FOV jump on Cinematic → Dialogue is deliberately deferred
while the current U2 stage verifies primary-writer/post-helper diagnostics. Its
evidence boundary and revisit condition are recorded in
[`U2_CINEMATIC_TO_DIALOGUE_RENDERED_FOV_JUMP_2026-11-15.md`](deferred/U2_CINEMATIC_TO_DIALOGUE_RENDERED_FOV_JUMP_2026-11-15.md).

## Reports and evidence

Durable technical reports belong in [`reports/`](reports/), and reusable
evidence that should stand independently of a task plan belongs in
[`evidence/`](evidence/). The reports include the [2026-09-27 Overlay/DXGI
startup crash investigation and repair history](reports/OVERLAY_DXGI_STARTUP_CRASH_INVESTIGATION_2026-09-27.md)
and the [adversarial Overlay crash-safety audit](reports/OVERLAY_ADVERSARIAL_CRASH_SAFETY_AUDIT.md).
The [final production safety/performance audit](reports/OVERLAY_FINAL_SAFETY_PERFORMANCE_AUDIT_2026-09-27.md)
records the remaining replacement, pending-resize, factory-coverage and exception-boundary findings with reproducible offline probes.

For the concise current state, see
[`TESTING_AND_RESEARCH.md`](../TESTING_AND_RESEARCH.md).

The current U2 Dialogue → Cinematic corrected-start repair is tracked in [its task plan](plans/U2_DIALOGUE_TO_CINEMATIC_START_HANDOFF_2026-11-14.md), with the runtime retest and Release candidate evidence under [u2-dialogue-cinematic-retest-2026-11-14](evidence/u2-dialogue-cinematic-retest-2026-11-14/).

The 2026-11-16 P0/U2 runtime capture identified a rejected provider Dialogue
handoff after recovery and same-receiver Gameplay baseline retirement before
Cinematic 2. Its analysis and raw log are preserved under
[`p0-u2-runtime-2026-11-16`](evidence/p0-u2-runtime-2026-11-16/); the bounded
offline repair and remaining runtime gate are recorded in the
[P0/U2 integration plan](plans/P0_U2_NATIVE_REFERENCE_INTEGRATION_2026-11-15.md).
The follow-up capture in the same evidence folder confirms the Cinematic 2
baseline repair, exposes a stale Cinematic EXIT setter interleaved after the
Dialogue provider armer, and records the standalone post-cinematic Dialogue.
That setter interleaving now has an offline regression and bounded fix; runtime
confirmation of the interstitial Dialogue handoff remains pending.
The subsequent Dialogue report confirms the expected setting loaded in the
ordinary candidate, but the log saturated at 16 MiB before any Dialogue or
Cinematic lifecycle records appeared. See
[`assessment-dialogue-followup.md`](evidence/p0-u2-runtime-2026-11-16/assessment-dialogue-followup.md);
the transition failure location remains unknown until a focused capture is
armed before Cinematic 1.

The log saturation cause was repeated identical Gameplay `POST_PROCESS` rows reaching the 16 MiB cap. The ordinary logger now coalesces adjacent bit-identical post-process payloads while retaining their full sample and exact event/transaction ranges. The focused regression and Release build pass; runtime confirmation is pending. This only repairs capture coverage and does not change Dialogue policy.

The 2026-11-16 runtime capture no longer saturated and exposed the Dialogue defect: a matched provider setter was still blocked by the global post-cinematic exclusion, leaving `Reduced` unapplied. A provider-only admission now requires the exact callback source, receiver, generation, settings epoch and Gameplay coordinator; interleaved unrelated callbacks are ignored without dropping ownership. Focused Dialogue, camera-writer and cinematic-lifecycle harnesses pass; runtime confirmation is pending. See the [follow-up assessment](evidence/p0-u2-runtime-2026-11-16/assessment-dialogue-followup.md).

The next capture confirms that the provider armer and setter did match, but the
writer trajectory arrived after native recovery had completed. That path still
fell through to Gameplay HorPlus (`70° → 108.941°`) instead of applying the
captured Dialogue `Reduced` destination. A bounded completed-owner provider
mapping fix now passes its pre-fix failing production-transaction regression;
runtime confirmation is pending. See the [assessment and preserved capture](evidence/dialogue-provider-after-recovery-2026-11-16/assessment.md).

The Lifecycle Authority migration's isolated M1 phase now has a private C++
relation, focused existing-harness integration and offline native binding
records. Its 45-assertion harness and test-registry audit pass. It is not wired
to live callbacks; no ASI or game behavior changed. M2 remains NO-GO pending
native Gameplay-role, Dialogue-terminal and arming-episode causal binding
evidence. See the [M1 result](reports/LIFECYCLE_AUTHORITY_M1_ISOLATED_CORE_2026-11-17.md),
[migration plan](plans/LIFECYCLE_AUTHORITY_MIGRATION_2026-11-17.md) and
[binding sheet](evidence/lifecycle-authority-m1-2026-11-17/native-binding-sheet.md).

The recurring report that both FOV correction and Overlay stop working after
an ASI update is deferred for later analysis. The current capture confirms the
ASI and hooks initialized, but the Overlay never attached to an eligible game
window and no camera transactions were recorded before a playable pawn was
available. It therefore confirms an Overlay startup failure while leaving the
reported FOV failure and the reboot-only recovery cause unresolved. See the
[analysis plan](plans/ASI_UPDATE_RUNTIME_FAILURE_ANALYSIS_2026-11-16.md) and
[preserved runtime logs](evidence/asi-restart-issue-2026-11-16/).
