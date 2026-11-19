# Concrete production differential parity

2026-11-17. Current verdict: corrective differential parity PASS in the six
supported OFF/ON fixture sequences. The original red result is preserved below
as historical evidence. The user authorized proceeding to M2 after green parity;
M2 implementation has started, but live cutover is not complete. No runtime,
R1, ASI, game or existing production helper implementation was changed.

## Corrective pass result and M2 entry

The four reproduced classes were repaired in the isolated Authority/binding:

1. Cinematic numeric policy uses the actual selector/transform and
   ResolveCinematicWriterSample, accepted activation reference retention,
   source association/conflict and initial-establishment departure. Prepared
   target input is not projected twice. This proves the fixture's retention and
   endpoint behavior, not all native activation-blend samples.
2. Setter now receives raw start/target, source, component, thread and generation.
   ProcessDialogueBoundary and RetireStationaryDialogueCandidateOnGameplayWriter
   run on Authority-private working data. Exclusion and stationary termination
   are decided there, without an adapter-supplied terminal verdict.
3. A matched provider endpoint can be consumed by the next setter after recovery
   already completed. Active Reduced and its exact prepared-setter witness are
   retained; the following writer uses Dialogue ownership, not Gameplay with an
   accidentally equal result. Arming policy/epoch remain latched.
4. A validated ambiguous B writer can receive a source-bound HorPlus argument
   without refreshing a reference. A's baseline stays Invalid; admission remains
   Unknown on this correction-only path. Unknown prepared input or unreadable
   native flags do not gain that permission.

Before repair: 26 field divergences, preserved in corrective-before.json.
After repair: **0 divergences**, 139 OFF and 215 ON comparisons, with identical
results for all 139 common fields. ON additionally checks named optional
observations, foreign output/thread refusal and closure. The six input sequences
and their ownership/reference/provenance comparisons were retained. Setter
argument comparison now checks Authority's actual prepared argument against the
production setter argument, rather than assuming setter pass-through.

Focused validation of current sources:

- M1 default: 111 assertions PASS, including 13 corrective assertions.
- Shadow replay: 31 PASS. Its historical raw baseline-admission divergence is
  still printed; correction-only B permission deliberately does not assert new
  baseline admission or independent native Gameplay proof.
- Production transaction harness default: PASS.
- Differential parity: PASS; source-level Writer port used by the Authority lane.
- CTest M1/parity: 2/2 PASS.
- Canonical registry audit: 60 targets / 64 invocations / 0 missing inputs.
- Runtime/R1 hashes equal the preserved M1.6 identities; no ASI/game operation.
- Finite supervised sequential builds/runs; every final record has verified
  process-tree cleanup. Full suite, graphics and live ABI were not exercised.

The differential invocation is now part of test.cmd's registry and CTest. It
cannot disappear while the registry audit still reports a valid inventory.
All current records are in ../evidence/lifecycle-authority-differential-2026-11-17/;
summary.json is current, initial-red-summary.json preserves the former summary.

M2's first implementation step is lifecycle_authority_writer_port.hpp. The
bound port checks the real CameraCorrectionGate, readable captured fields and
the matching incoming argument. It owns the canonical policy callback and
consumes/destroys WriterInvocation before returning a result. It exposes neither
legacy Accept(EventFacts), owner state nor PrepareWriterArgument. Negative
access checks and supervised contention/mismatched-argument checks pass. No
Applied result is inferred before native MOVSS. Optional readback additionally
matches output object and thread as well as invocation/source/generation.

This source API is not yet connected to runtime.cpp. M2 remains implementation
work, not a completed migration: actual AspectRecalculation/RuntimeCombine
aspect/flags writes and pending enable/mode transitions still live in
ProcessGameplayAspectRecalculationWriter, ProcessGameplayEnableTransition,
SynchronizeRuntimeCombineAspect and ProcessAtomicExitHandoff; full Cinematic
activation progress and all remaining mutable lifecycle entry paths must move
under Authority before the single live cutover. Merely replacing the FOV scalar
call would omit these concrete behaviors and is not an acceptable cutover.
The added AspectRecalculation regression first failed when the new policy tried
to apply direct HorPlus; the guard is fixed and green. It does not claim that
AspectRecalculation's physical aspect effects have already migrated.

No new native proof search, hook, phase-number extension or R1 change was needed.
The older red-stage sections below describe the initial counterexamples and
are superseded by this result where they refer to current readiness.

## Method and coverage

The existing camera_writer_transaction_harness now has --authority-parity.
Its test-only lifecycle_authority_parity_cases.hpp holds independent production
and Authority states. Both receive scripted native boundary values/identities.
The production lane calls actual ProcessCameraWriter, ProcessDialogueBoundary,
PrepareCinematicEnter/ProcessAcceptedCinematicEnter, ObserveCinematicAspectStore,
ProcessAcceptedCinematicExit, provider matching/consumption and ApplyCameraSetting.
Relevant runtime classification, stationary-candidate retirement, provider
bookkeeping and ENTER reset operations are represented by their real helpers.

The Authority lane receives independently constructed event inputs, never
production output FOV, baseline verdict, GameplayEligible or ownership as proof.
PolicyAdmitted is requested from its own source-bound original fixture sample.
Known source relation, boundary validation and bounded provider causal evidence
are fixture premises, not newly discovered native bindings. No fixed PID is
used; generic source identities A/B and relative generations drive the cases.

Six sequences run in both Diagnostics modes: same-source recovery before next
Cinematic; provider arm/setter/writer through endpoint, prepared Dialogue and
next Cinematic; receiver replacement; Gameplay disable/writer; stationary
Candidate termination; Gameplay disable immediately followed by ENTER.

Baseline comparison checks numeric/source identity and consumer usability,
allowing production current retention and Authority Retained to represent the
same usable reference. Both raw trust representations are printed. Provider
Candidate in Authority is compared with the combined production Handoff plus
Dialogue state, not incorrectly with the ordinary Dialogue enum alone.

OFF has 139 field comparisons; ON has those same 139 plus 36 modeled named
readback/closure checks. All common result patterns and FOV traces are equal
between OFF/ON. Optional readback uses a modeled store result from the production
transaction, not actual native MOVSS execution or hook readback. The complete
ABI adapters in runtime_native_adapter_harness were not exercised in this stage.

## Confirmed red-to-green core repair

Before repair the paired trace had 34 field divergences, including Gameplay
disable invalidating Authority's original native reference immediately. Actual
ApplyCameraSetting's GameplayEnabled branch changes the execution gate/epoch
and pending physical transition; it does not invalidate GameplayBaselineStore.
An immediate Cinematic ENTER consequently retained target 153.931 in production
while the candidate Authority binding fell back to authored target 126.87.

Authority now preserves an already Current/Retained original reference across
an enable-gate change and updates its relevant epoch without refreshing sample
provenance. Execution intents remain revoked. Policy selection still invalidates
as before; toggling enable cannot revive Invalid trust. Three focused assertions
cover permission revocation, immediate Cinematic use and no resurrection.
Red evidence: evidence/lifecycle-authority-differential-2026-11-17/red.json;
final paired trace and focused results are in the same directory.

## Remaining divergences: 13 fields per mode, four related classes

1. **Cinematic policy binding is incomplete.** Native writer input 130.311
   produces 153.931 in production but 167.764 with the current provisional
   numeric proposal; prepared Cinematic input 153.931 produces 153.931 versus
   173.864. The candidate proposal reuses simple selector/projection arithmetic,
   not the production writer's source-authorized initial retention and prepared
   activation mapping. This is a demonstrated policy-adapter defect, not proof
   that the core independently computes those wrong numbers. The missing
   association/establishment/provenance inputs must be privately owned and
   supplied to the actual writer resolution policy, not guessed from equality.

2. **Setter and Candidate bindings omit accepted lifecycle dependencies.**
   After same-source recovery, production's post-cinematic exclusion still
   rejects the ordinary neutral setter; an unconditional DialogueCandidate event
   creates a hypothesis in Authority. Separately a stationary Candidate is
   retired by production's source/epoch/generation/reference predicate on the
   next writer, whereas the current composite input supplies no matching raw
   terminal facts and leaves it present. The existing terminal boolean is not a
   lossless binding. Its predicate and required facts belong inside Authority.

3. **Already-completed recovery -> provider Dialogue activation is incomplete.**
   Armer and first matched writer outputs agree (153.924 at the fixture's rounded
   sample); endpoint writer outputs also agree (149.071). At the subsequent
   actual setter, production consumes provider admission and activates Reduced;
   the candidate low-level ProviderSetterObserved binding leaves Candidate and
   has no setter policy output. The following prepared writer gets 149.071 by
   Gameplay HorPlus in Authority while production uses Dialogue. Correct numeric
   output here conceals incorrect ownership. M1.6's pending-recovery transfer
   fixture did not cover this already-completed recovery ordering.

4. **Ambiguous replacement correction and baseline admission remain coupled.**
   On B's first TransitionAmbiguous sample, production preserves Gameplay HorPlus
   (153.931) while invalidating A's reference. Authority rejects baseline admission
   and also leaves output native (130.311). Both reject A's baseline correctly.
   The missing contract is source-scoped permission for the supported correction
   without admitting an ambiguous sample as the new Gameplay reference. This
   cannot be resolved by transferring A's baseline or pretending Gameplay was
   independently native-proven.

Some field failures repeat a class at later transitions; 26 total field failures
are not 26 independent defects. Settings-related comparisons are green after the
bounded repair. No core rewrite or speculative numeric workaround was applied
to conceal the remaining binding failures.

## Serialization and access

The real CameraCorrectionGate was exercised with a competing settings task.
The PRE-shaped scope accepted/prepared/destroyed its invocation while holding
the gate; settings committed only after release. Same-thread nested native
callback admission was rejected. The worker was joined and process-tree cleanup
verified. This supports the serialized order when a future adapter keeps both
operations and receipt lifetime in that scope; it is not live callback proof.

A prospective Writer-only port forwards only composite Accept and argument
preparation. Compile-time negative checks reject legacy EventFacts/PrepareEffect
through that port and copying WriterInvocation. This is a test-only structural
candidate. Production has no Authority wiring yet, so actual prevention of
raw-owner leakage remains a required M2 access gate.

## Validation and disposition

- Both focused targets built sequentially under the existing supervisor.
- Default isolated M1 harness: 98 assertions PASS (three new settings checks).
- Isolated shadow replay: 31 assertions PASS.
- Default production camera transaction harness: PASS.
- Differential invocation: exit 1, 26 field divergences; intentionally red.
- Common OFF/ON checks and FOV traces identical; summary.json records counts.
- Owned build/test trees, including compiler helper and gate worker: cleaned.
- Full suite, graphics, game, release and live cutover were not run.

The remaining work is concrete: bind the complete Cinematic writer resolution;
bind Setter/provider/exclusion/terminal facts as private Authority transitions;
separate correction-only admission from baseline refresh. Re-run these same
paired sequences after each bounded counterexample repair. No general T01–T36
audit, new Gameplay proof search or new hook is justified by these findings.
Full behavioral equivalence and complete native adapter binding remain unmet.
