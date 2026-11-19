# P0 — Native reference admission

Priority accepted in current task: P0 origin/reference correctness, P1 overlay
reference readback, P2 transition adapters, P3 visual/consumption acceptance.
P0 offline admission correctness is validated; runtime origin investigation
remains pending. P1/P2/P3 work is gated only by evidence it individually needs,
not by a blanket dependency on complete P0 runtime provenance.

Invariant: raw native register is observation location, not game-origin proof.
Unknown or mod-derived input cannot replace a confirmed native reference.
An independently confirmed new game sample must replace it; no permanent freeze.

First bounded step: encode a failing regression for the existing baseline
admission using the recorded60.9227 mod-result input, alongside an independent
game-change fixture. Preserve canonical characterization tests, expose the
stronger contract through --native-origin-contract. A FAIL here is an open
defect, not acceptance or a production fix. The test only replays the baseline
admission boundary; it does not emulate the native setter.

Production integration prerequisite: CameraWriterInput/CameraFovObservation
currently contain no independent origin evidence. Raw60.9227 from a real game
change and the recorded Dialogue mutation can reach Project with identical
boundary/space/provenance/source/flags. Changes solely inside IsEligibleObservation
cannot distinguish them. Numeric blacklist, current Dialogue active flag (which
the writer may already reset), timing, or declaring every initial raw input
GameOrigin are invalid replacements for origin evidence.

Mutation sites for propagation: accepted ENTER operand; Dialogue XMM1 commit;
native initialization captures and setter return; writer output commit. Existing
source/generation/lifecycle/settings/execution fields must qualify the relation.
Unknown remains unknown through native re-observation. Game-origin admission
must name an actual independent boundary; configuredGameplayFovKnown is false
in current production snapshot. Neither stable samples nor callback absence
alone establishes game-origin.

Validation in this step: sequential supervised baseline harness build, current
characterization run and opt-in contract run. Finite timeout/output budget and
verified owned-tree cleanup. Do not install/build a new ASI or launch the game.
## Result of this bounded step

Changed tests/camera/gameplay_baseline_harness.cpp only in executable code.
Canonical no-argument baseline characterization: PASS. Opt-in
--native-origin-contract: FAIL (exit1), specifically
recorded_mod_result_must_not_replace_native_reference, actual native60.9227.
Positive independent_game_change_must_update_native_reference: PASS.

The negative and positive fixtures intentionally supply identical observation
payloads after the same initial70.0005 reference; only their external causal
annotation differs. Therefore no predicate over the current Project arguments
can satisfy both requirements. This is an API information gap, not a request
to blacklist a numeric FOV or prevent legitimate changes. Positive fixture PASS
is not proof of an independently observed production game-origin boundary.
Native setter execution is not emulated; the negative replays its recorded
output at the baseline admission seam. Full requested native-adapter regression
remains to be implemented with the provenance propagation fix.

Evidence: ../evidence/native-reference-p0-2026-11-13/. Final harness build recheck
PASS; source diff whitespace check PASS. Supervised compile invocations timed
out despite generated artifacts; bounded rechecks returned0. All owned process
tree cleanups verified; no ASI built/installed or game/debugger launched.

The remaining production seam must carry origin evidence from known mutations
and from independent game-origin producers into CameraFovObservation/Project.
The native initialization/setter/writer points currently do not expose that
independent Gameplay origin as a validated fact. In particular snapshot
configuredGameplayFovKnown=false is explicit, and first/raw/stable input cannot
be promoted merely because no preceding mutation was logged.

Simply defaulting all input to Unknown and rejecting it would suppress both
fixtures and prevent baseline establishment, affecting U1-A witness/provider
eligibility. Defaulting raw input to GameOrigin would preserve the defect.
Neither is an acceptable completed fix. Known-modified propagation is necessary,
but it must not be marketed as full game-origin proof when native intervening
writes remain unknown.

Status at that earlier stage: regression requirement encoded and reproduced;
the general native-origin propagation fix and full native-adapter validation
were NOT IMPLEMENTED. P1/P2/P3 remain dependent.

## Runtime-confirmed post-exit endpoint admission — 2026-11-13

The controlled runtime log exposed a narrower admission defect: after the
validated post-EXIT provider handoff reaches its native target, subsequent
same-receiver writer samples near that activation's native target are classified
GameplayEligible and replace the pre-transition baseline (90.3115) with ~70.
The intended offline fix is bounded by the completed activation's receiver,
thread, cinematic generation, Dialogue settings epoch, and native-target
endpoint neighborhood. Such samples remain baseline-ambiguous while normal
Gameplay HorPlus still runs. Admission resumes outside that endpoint
neighborhood; no timer, numeric blacklist, or permanent freeze is introduced.

Root cause: `TryMapPostExitProviderDialogueSample` marks the shared
`recoveryRetired` result when its sample reaches the activation's native target,
sets coordinator-after to Gameplay, and clears recovery state. Runtime logged
that shared result as a writer-RSI retirement regardless of cause, explaining
the equal previous/current RSI/RBX. On following calls, the completed provider
record suppressed repeated Dialogue admission but did not affect Gameplay
writer context, so endpoint values were admitted into the baseline.

Offline fix implemented: the completed activation retains a bounded baseline
quarantine for its native-target neighborhood, qualified by receiver, thread,
cinematic generation and Dialogue settings epoch. Matching samples are
`TransitionAmbiguous`; normal Gameplay HorPlus remains active. Admission resumes
outside the endpoint neighborhood. No timer, numeric blacklist or permanent
freeze is used. Recovery diagnostics now distinguish provider-endpoint
completion from actual writer-RSI retirement.

Regression requirements cover identity mismatches, endpoint bounds, baseline
retention, continued Gameplay HorPlus output and later Gameplay re-admission.
Full canonical `test.cmd` passed on the current tree: all 59 registered targets,
62 serial runner invocations, graphics included and process cleanup verified.
A fresh ordinary Production Release was built, not installed. Artifact and
source hashes are in
`../evidence/p0-u2-runtime-2026-11-13/candidate-baseline-admission-fix.json`.
The user then ran that exact artifact. Runtime identity matched its SHA. The
provider completed at input70.2322/native target70 with
`writer_pair_changed=0`; subsequent matched samples70.109,70.0334 and70.0019
were `TransitionAmbiguous` and retained native baseline90.3115 at baseline
sequence1340. No ~70 baseline update occurred. A later eligible Gameplay update
to90.3069 occurred after control phase1 began, and phase2 also completed in
clean Gameplay. This confirms the bounded endpoint-admission fix on this
runtime path and that eligibility is restored; it does not close the broader
native-versus-mod-origin proof gap or establish the final producer/def-use of
`camera+0x230`.

## Lifecycle-scoped authority follow-up

Requested audit of the known Gameplay→Cinematic1→Dialogue→Cinematic2→Gameplay
boundaries is complete in
[P0_LIFECYCLE_REFERENCE_BOUNDARIES_2026-11-13.md](../reports/P0_LIFECYCLE_REFERENCE_BOUNDARIES_2026-11-13.md).
Positive existing paths: accepted original Cinematic ENTER operand and validated
constant-backed provider request. Scope is their request/reference, not arbitrary
Gameplay or inferred Dialogue lifecycle. Startup/Dialogue ENTER/resume cannot
be promoted from coordinator/raw/stability alone. No source changes or new
tests/build/runtime in this follow-up; P0 admission remains unresolved.

## Перегляд P0: camera context

User уточнив, що штатний Gameplay залишається dynamic incoming→HorPlus без
класифікації ADS/zoom або універсального доказу кожного native producer. Поточне
завдання — класифікація контексту конкретного writer та baseline admission,
окремо від FOV domain. Новий integration path і acceptance matrix:
[P0_CAMERA_CONTEXT_CLASSIFICATION_2026-11-13.md](../reports/P0_CAMERA_CONTEXT_CLASSIFICATION_2026-11-13.md).
У recorded60.9227 Dialogue classifier був Active перед writer reset; semantic
receiver ownership не доведено. Універсальний watchpoint більше не є обов'язковою
передумовою для відхилення підтвердженого context conflict. Він може знадобитися
окремо, якщо потрібно буде встановити domain після справжнього Gameplay handoff.

## P0-A production integration: missing causal event

The requested minimum integration was checked against the current callbacks.
PostExitProviderHandoff::ObserveSetterSample runs at the pre-CALL boundary;
TakeWriterSample matches activation identity and a numeric sample, but does not
observe the setter return or intervening native writes. The existing
TracePostExitBlendSetterReturn is Diagnostics-gated; making it available with
Diagnostics OFF proves the setter returned and can read camera+0x230, but still
does not identify the last producer of the value subsequently loaded by writer.

The concrete missing event is the native write/def-use that supplies the current
writer's input from camera+0x230 after the setter returns, including whether an
intervening writer replaced or recomputed that field. Existing current-image
data-flow identifies a derived GetCameraView path that calls 0x3236604, writes
camera+0x230 and then invokes the camera writer. That path is a counterexample to
assuming setter return is the last write. The recorded receiver's vtable/path is
not established; this is not a claim that this path occurred in that capture.

Minimum evidence-gathering step: for the affected receiver, observe the actual
last write to camera+0x230 between paired setter return and writer read, with the
writer invocation, thread/receiver and input dependencies of that producer.
A bounded hardware write watchpoint can locate candidate instructions in a
controlled debugger investigation; it cannot itself serve as production origin
authority. A production solution would need a structurally resolved boundary
covering the verified final producer and propagation/invalidation for intervening
or foreign writes; a new hook at that producer is a candidate only after that
coverage is demonstrated. No fixed RVA, equality-only test, timestamp, or assumed
exclusive camera ownership qualifies this relation.

P0-A remains incomplete; no provenance mutation, admission change, regression
PASS or Release build is claimed. The already-added opt-in baseline regression
continues to fail for the recorded mod-derived input. This is a specific causal
gap, not evidence that dynamic Gameplay must track all ADS/zoom triggers.

## Offline working-tree follow-up

The current working tree has since added typed `WriterContext` propagation into
Gameplay baseline projection. Focused offline validation now passes:

- `gameplay_baseline_harness` canonical characterization and
  `--native-origin-contract`: PASS. The negative case supplies
  `DialogueLifecycleObserved`; the positive same-value case supplies the
  default `GameplayEligible`. This proves typed-context admission only, not
  automatic game-origin discovery.
- `camera_writer_transaction_harness`: PASS. During an active Dialogue, a
  writer-source mismatch without independent identity evidence now remains
  `TransitionAmbiguous`; Gameplay correction continues while baseline
  projection is deferred.
- `runtime_native_adapter_harness dialogue-baseline-context off/on`: PASS.
  The production adapter fixture seeds70.0005, applies the Reduced Dialogue
  output60.9227, then sends it through a writer that invalidates Dialogue in the
  same invocation. The seed baseline remains retained but stale, and the
  Gameplay HorPlus correction still applies. Diagnostics OFF/ON agree.

These checks establish the current offline context-conflict contract. They do
not establish that the fixture's receiver↔writer identity matches the affected
game path, nor identify an intervening native producer of `camera+0x230`.
The opt-in case's distinct-origin annotation remains supplied by the fixture.
Full native game-origin admission is still NOT IMPLEMENTED. The bounded
hardware watchpoint is deferred while the user requested offline-only work.

## Status split and downstream scope — 2026-11-13

Disposition: **OFFLINE-VALIDATED; RUNTIME-PENDING**. The offline acceptance
criteria for preventing the tested Dialogue result from replacing the
Gameplay baseline, while retaining Gameplay correction, are met for the
named fixtures. This does not close full native-origin investigation: the
affected runtime's final producer for `camera+0x230` and receiver↔writer
relation remain unestablished.

The runtime evidence gap gates only decisions that depend on actual producer
identity or final-write coverage. It is not a blanket gate on independent U2
offline work. U2 source/design analysis and deterministic sequence fixtures
may proceed when their contract is fully represented by current code and do
not claim native invocation provenance, active-view consumption or visual
correctness. Production mapping/application decisions that rely on those
unknowns remain pending. P1/P2/P3 are therefore evaluated by their individual
evidence dependencies rather than automatically blocked by this P0 runtime
item.

The ambiguity-exit regression now exercises natural Dialogue recovery
completion and the first subsequent Gameplay writer. It confirms that the
current source computes `TransitionAmbiguous` only while Dialogue phase is
non-Inactive, and recovery completion resets that phase to Inactive before the
next Gameplay writer.

## Controlled runtime capture prepared — 2026-11-13

User authorized a controlled P0/U2 runtime pass. A fresh ordinary Production
Release was built from the current working tree with the existing bounded
diagnostics, startup timeline OFF and discovery OFF. No FOV policy/admission
logic changed for this capture. The only new source change is diagnostic
correlation: `FOV_OBSERVATION` and `GAMEPLAY_BASELINE_PROJECTION` now include
the writer context and current observation sequence. This is gated by the
existing Diagnostics switch and emitted with the existing FOV change event.

Candidate: `build-artifacts/p0-u2-runtime-2026-11-13/STALKER2CameraTweaks.asi`.
SHA-256 `2D952B287FB9DB55A7FA789F767F5CA9E6395EF1D41F0BB6EDA5BF7C3D23295A`,
3,866,112 bytes, version3.2.2.0. It has not been installed. Build/source
provenance and exact operator steps are in
`../evidence/p0-u2-runtime-2026-11-13/`.

Next: user installs the candidate manually, enables `[Diagnostics] Enabled`,
arms the one-shot capture before Cinematic1, runs Cinematic1→Dialogue→Cinematic2
and recovery, then uses the two bounded controls for post-recovery and ordinary
Gameplay Dialogue. Analyze the full log before deciding whether to run a
separate hardware watchpoint; its address must come from the current run's
receiver/writer evidence, not an old pointer. P0 remains runtime-pending until
the relevant `camera+0x230` final producer/def-use is observed or formally
dispositioned as unavailable.
