# Gameplay Baseline Trust Across Confirmed Dialogue

**Status:** Offline fix validated; Release ASI installed; runtime confirmation pending.

## Goal

Specify when a previously admitted Gameplay FOV baseline remains trustworthy
across a confirmed post-cinematic Dialogue, then capture the reported
`Gameplay → Cinematic 1 → Dialogue Reduced → Cinematic 2` sequence in the
existing production-transaction harness. The regression must fail on the
current behavior before any production change is considered.

## State contract

Keep three questions independent:

- **Valid:** Was the numeric native reference admitted from a confirmed
  `GameplayEligible` camera-writer sample with native-register provenance,
  valid aspect, writer source, and publication sequence?
- **Current:** Has evidence contradicted the Gameplay context that owns that
  admitted reference? A non-Gameplay writer sample is not, by itself, evidence
  that Gameplay FOV changed.
- **Usable:** Does this particular consumer have enough current identity and
  lifecycle evidence to use the reference now? A valid/current baseline may be
  temporarily unusable for one operation without being erased.

Dialogue samples must never be admitted as Gameplay baseline updates. A
confirmed provider Dialogue with the same bound writer source, accepted
provider activation, and matching Dialogue lifecycle identity may preserve the
previous Gameplay reference while applying the activation's captured Dialogue
policy. A change to the Dialogue policy alone (for example, Reduced to
Adaptive) does not establish a Gameplay FOV change.

Do not conflate identity namespaces. Cinematic selection generation identifies
a Cinematic activation; provider generation/settings epoch bind a Dialogue
activation; neither is automatically the lifetime generation of the native
Gameplay camera. Before using any epoch to age the Gameplay reference, identify
its owner and the events that advance it.

Evidence that must prevent use of an old baseline includes a confirmed writer
receiver replacement, a native-input provenance failure, or loss/mismatch of
the ownership evidence required by the consumer. The safe consequence may be
temporary non-use or invalidation depending on whether the evidence contradicts
the stored Gameplay context; the distinction must be asserted explicitly.
Fresh Gameplay admission is required after evidence that truly retires that
context.

## Positive integrated regression

Use one receiver and 32:9 aspect with independently fixed expected values:

1. Admit native Gameplay FOV `130.3115°`; its Gameplay HorPlus output is
   `153.931°`.
2. Cinematic 1 selects the Gameplay-native reference and targets `153.931°`.
3. A confirmed provider Dialogue uses captured `Reduced` policy. Its writer
   output is checked against the expected reduced destination (approximately
   `149.071°` for the recorded scenario).
4. Dialogue writer observations do not update the Gameplay baseline. While
   the confirmed Dialogue remains bound to the same source and activation, the
   baseline remains valid/current and is usable by Cinematic 2.
5. Cinematic 2 reports `GameplayNativeNumericBaseline` as its reference origin
   and selects `153.931°`. Assert the selection provenance and trust state,
   not only the resulting number.

The harness must exercise the existing production baseline projection,
Dialogue/provider transaction, camera-writer transaction and Cinematic
baseline selector. It must not replace their decisions with test-only logic.

## Negative controls and existing coverage

Retain or reuse checks for:

- receiver replacement, including unchanged numeric FOV, makes the old
  receiver's baseline unavailable until fresh Gameplay admission;
- transformed/mod-derived input cannot be admitted as native baseline;
- provider generation, receiver, thread, callback source, and settings-epoch
  mismatches cannot authorize the confirmed provider path;
- unbound/ambiguous Dialogue context is not promoted to confirmed provider
  ownership;
- Diagnostics ON/OFF does not change the production decision.

Audit existing harness coverage before duplicating these cases. The new
positive sequence must complement those negative controls; it must not weaken
receiver or provenance guards.

## Current evidence and gap

- Runtime log `research/evidence/p0-u2-runtime-2026-11-16/STALKER2CameraTweaks.log`
  records the provider Dialogue handoff and the following Cinematic ENTER.
- The log shows `DialogueLifecycleObserved` for the same writer source, then
  `baseline_valid=1`, `baseline_usable=0`, `reference_origin=AuthoredEnter`,
  and target `126.87°` at Cinematic 2 ENTER.
- `GameplayBaseline` currently stores native/derived FOV, aspect, source,
  observation sequence, `valid`, and `current`; it does not own a native
  receiver-lifetime generation or a settings epoch.
- Existing tests cover Dialogue non-admission, provider/recovery retention,
  receiver replacement, and Cinematic selection separately. The current suite
  does not assert the complete confirmed Dialogue-to-next-Cinematic positive
  sequence and its trust/provenance explanation.

## Stage boundary

The focused integrated regression was added to the already registered
`camera_writer_transaction_harness`, built, and shown to fail specifically
because the same-receiver baseline loses currency after confirmed provider
Dialogue. The target harness also retained its receiver-replacement negative
control. `gameplay_baseline_harness`, `dialogue_boundary_transaction_harness`,
and `cinematic_fov_harness` passed before the production change.

## Stage 2 — Confirmed ownership preservation

### Scope

- Allow baseline retention only for a confirmed post-EXIT provider Dialogue
  lifecycle bound to the same writer receiver and provider activation.
- Represent this as a distinct writer context. Generic
  `DialogueLifecycleObserved` remains untrusted for baseline retention.
- Validate provider receiver, callback source, thread, Dialogue generation,
  captured activation epoch, and lifecycle binding at the point that grants the
  confirmed context. A policy-settings epoch change does not replace or erase
  this captured identity; the normal Dialogue binding remains epoch strict.
- Treat a Dialogue settings-epoch change by itself as unrelated to native
  Gameplay baseline currency when the same accepted provider/lifecycle identity
  remains established. Keep policy-change semantics and provider mapping rules
  unchanged.
- A receiver replacement, conflicting lifecycle identity, unmatched provider
  generation/thread/source or invalid native provenance must not preserve or
  restore the old baseline. Never resurrect a baseline already made non-current.
- Keep FOV formulas, Dialogue output policy, Cinematic policy and recovery
  algorithms unchanged.

### Acceptance

- The red `Gameplay → Cinematic 1 → Dialogue Reduced → Cinematic 2` regression
  turns green and asserts baseline data, currency, consumer usability,
  `GameplayNativeNumericBaseline` origin, retention witness and `153.931°`.
- A current Dialogue settings epoch change with the same accepted provider
  identity does not retire the Gameplay baseline or replace its captured
  Dialogue policy. The activation's captured epoch must still match the
  accepted lifecycle record.
- Generic/unbound Dialogue does not receive the confirmed provider context.
- Existing receiver-replacement, generation/thread/settings/source mismatch and
  native-provenance controls remain green; no stale baseline is resurrected.
- Run the complete `test.cmd` suite after the focused harnesses pass. Do not
  build/release an ASI or claim rendered-view validation in this stage.

## Implementation result — 2026-11-16

- Added `ConfirmedProviderDialogueLifecycleObserved` as a distinct writer
  context. Only the accepted provider activation with matching receiver,
  callback source, thread, generation, and captured activation identity can
  produce it. Generic `DialogueLifecycleObserved` remains fail-closed.
- Split CandidateTracker's full policy-epoch binding conflict from structural
  receiver/callback/generation identity conflict. A policy epoch change keeps
  the strict ordinary binding invalid while allowing the already accepted
  provider identity and captured Dialogue policy to continue. Structural
  identity mismatches still reject that path.
- `GameplayBaselineStore` retains a same-source baseline for the new confirmed
  context, but does not admit Dialogue values as Gameplay updates. A different
  writer source still retires the old baseline.
- No FOV formula, Cinematic target selection, or recovery formula changed.
- The integrated lifecycle regression and focused negative controls pass.
  Full `test.cmd`: 62/62 invocations PASS, 0 FAIL; the hardlink ownership
  subcase remains an explicit `SKIP` (`error=5`). The changed production
  `runtime.cpp` translation unit compiled independently.
- Release ASI was built with the standard production profile and installed at
  the game's `Binaries/Win64` path. Its SHA-256 is
  `491829BABFC0830498072A53ACAE5944147AB124BD8976750E26CA9FA1A716AE`.
  The prior installed ASI and pre-test log are preserved under
  `build-artifacts/runtime-test-gameplay-baseline-dialogue-2026-11-16/`.
- Runtime/rendered-view confirmation is still pending. The next capture must
  include Cinematic 1 ENTER, confirmed Dialogue Reduced, Cinematic 2 ENTER and
  EXIT, and post-exit Gameplay recovery/ownership.
