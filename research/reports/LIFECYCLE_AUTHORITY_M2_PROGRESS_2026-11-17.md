# M2 implementation progress — native effects and Cinematic boundaries

2026-11-17. **M2 resumed after the user requested continuation.**
The results below distinguish the previous checkpoint from the resumed work.
Runtime callbacks still use their existing owner. The Authority ports run in
offline fixtures.

## Implemented and exercised

- Authority owns Gameplay physical value state: restoration, replay, source and
  pending enable/mode transitions. Existing ProcessGameplayHorPlusWriter and
  ProcessGameplayAspectRecalculationWriter execute on private working data.
  Native memory callbacks receive no mutable lifecycle view. RuntimeCombine keeps
  write/readback confirmation before HorPlus; write failure, a success response
  without the requested memory change, unreadability and retry are covered.
  Restoration observation sequences remain monotonic across private reductions;
  scratch helper reference stores do not become public authoritative baseline
  publishers. Exported reference data comes from the final Authority state.
- Cinematic activation capture and writer progress use the existing
  CinematicEnterActivation mapper, including a captured Dialogue corrected start.
  First-Cinematic and Dialogue-to-Cinematic trajectories compare actual production
  transactions at weights 0, .25, .5, .75 and 1 in Diagnostics OFF/ON. The latter
  outputs 149.071 -> 150.286 -> 151.501 -> 152.716 -> 153.931 degrees.
- New typed Cinematic port accepts ENTER before a receiver is available. It
  retains the admitted Gameplay numeric reference without assigning its source
  address as the Cinematic receiver. Native armer and aspect-store facts establish
  their own receiver/selection relation. The active record transfers from pending
  selection to its bound scope; it is not mirrored as two owners. Aspect source
  and armer receiver remain separate facts. A frozen reference retains its own
  original source/observation identity.
- Actual native ENTER is an explicit transfer from the preceding primary
  Dialogue scope. Its start witness is captured before its permissions are
  retired. Ordinary setter callbacks during the confirmed native Cinematic
  selection cannot turn an interactive choice into Dialogue ownership.
- Writer-empty EXIT has a terminal path without a fabricated recovery owner or
  Gameplay admission. Retained activation-start evidence is checked through
  RetireForReentry; it is not retained as lifecycle ownership. An unvalidated
  caller or incompatible observed call thread cannot replace the activation.
  A validated new blend capture does not overwrite its independent aspect-policy
  source. New arming clears the
  prior invocation's current publication facts, while old readbacks can remain
  separately historical.
- Cinematic Native aspect policy does not invoke hidden HorPlus. Disabling and
  re-enabling the module cannot rearm an already revoked activation. Configured
  aspect/FOV selections do not increment the enable epoch (matching the current
  production setting transaction); accepted activation policy remains latched.
- Writer argument finalization distinguishes a correction permission from an
  unchanged argument observation. Both can provide a PRE boundary fact; neither
  claims a native FOV store/readback. A prepared Cinematic endpoint that production
  passes through no longer receives a spurious mutation permission.
- Invalid authored FOV disables its numeric effect without discarding the
  independently validated ENTER lifecycle or aspect effect. An invalid EXIT
  target cannot leave a recovery owner with an unreachable numeric endpoint;
  retained corrected-start provenance remains independent of recovery authority.

The existing aspect policy function now forwards to a value-input overload with
the same policy switch. The optional auto resolver is still called only for Auto.
This small production leaf refactor supplies captured viewport values to the
private reducer; it does not change ratios or algorithms. Its own harness passes.

## Verification of current sources

Evidence directory: ../evidence/lifecycle-authority-m2-2026-11-17/.

| Check | Result | Boundary |
| --- | --- | --- |
| M1 default | 111 assertions PASS | Isolated core/characterization |
| Shadow replay | 31 PASS | Preserved limited fixtures; not live ABI |
| Six original OFF/ON sequences | 0 divergences | Same behavioral field comparators |
| Production transaction harness | PASS | Includes all three new M2 fixture groups |
| Gameplay physical parity | PASS OFF/ON | Three modes, pending transitions, failures/retry |
| Cinematic progress parity | PASS OFF/ON | Production activation mapper and transactions |
| Typed Cinematic port | PASS OFF/ON | Scripted captured facts, real serialization gate |
| Existing cinematic aspect harness | PASS | Shared aspect policy/field application |
| Final focused CTest | 3/3 PASS | M1, six-scenario parity, M2 production transactions |
| Registry audit | PASS | 60 targets / 64 invocations / 0 missing inputs |

The original six behavioral comparisons were retained. Their optional readback
checks now also cover pass-through arguments: native MOVSS still occurs when
the mod does not change its input. This exposed premature observation-window
abandonment in PrepareWriterArgument. Absence of an execution grant no longer
destroys that window; port finalization or receipt destruction decides its
closure. The two red comparison failures are preserved in
passthrough-observation-red.json and pass after repair.

A second red control seeded an existing provider claim at the eventual
Cinematic policy source. Transferring a provisional armer-bound activation to
that first aspect-store source overwrote the old scope but initially left its
arming record live. Both OFF/ON controls failed. The transfer now explicitly
revokes dependent intents and retires that claim before committing. Evidence:
provider-scope-transfer-red.json; the green control is in production-final.json.
This is a regression over permitted Authority states, not a claim that this
particular native sequence has been observed in-game.

Final focused builds/runs use the existing finite process-tree supervisor and
verified cleanup. No graphics, ETW, game, full suite, ASI build or installation
is part of this batch. Native port fixtures assert captured-boundary validation;
they do not independently verify actual hook/register extraction in the game.

The earlier prepared-argument characterization failed because it demanded a
mutation permission even for the production pass-through action. Its old result
is preserved as prepared-argument-characterization-red.json. The assertion now
requires no permission, retained Cinematic policy ownership and the unchanged
153.931-degree argument; the FOV expectation was not relaxed. The six original
production/Authority comparisons remain green.

## M2 acceptance disposition

Complete in the current implementation batch: ordinary Gameplay physical
effects/pending transitions; activation mapping/progress; typed unbound ENTER,
armer/aspect/EXIT port; PRE argument/publication semantics; focused regressions.

Incomplete for M2: full recovery/atomic exit-handoff physical routing, native
provider and ordinary Dialogue adapter binding/finalization, complete runtime
settings/startup/reset and read-only publication migration, live callback wiring,
removal of old writable lifecycle aliases/views and the full native adapter
validation. The new scoped model must receive the actual facts at those paths;
calling only its FOV scalar API would still omit required behavior.

Next implementation work remains within the approved M2 map: finish those
event/effect paths, replace all old mutation call sites together, then validate
the cutover. These are implementation deliverables, not a new native-proof study
or an additional approval request. There is no claim that the unchanged live
runtime already uses Authority, that all T01–T36 are migrated, or that an ASI is
ready for in-game verification.

## Resume record

The previous checkpoint was saved when the user requested a pause. Its source and evidence were preserved in
the working tree and stopped-source-checkpoint.zip in the evidence directory.
The checkpoint is a snapshot of the named task files, not a Git commit or a
claim that unrelated staged/unstaged workspace changes belong to this batch.
No ASI or game file was replaced. Runtime callback source and R1 hashes remain
equal to the recorded pre-M2 identities.

Before live cutover, also verify the remaining cross-feature Writer gates against
production: Dialogue disabled with Gameplay HorPlus enabled; Gameplay disabled
or AspectRecalculation selected while Dialogue remains active; Dialogue EXIT and
rearm output. Source concern, not a reproduced finding: ResolveCorrectionOwner,
FeatureEnabled and EvaluateProductionPolicy currently select one numeric owner,
while production ProcessGameplayHorPlusWriter separately gates Gameplay output
and Dialogue input mapping. The original six parity sequences do not cover these
toggle combinations. Do not interpret their green result as closing that item.

## Resumed implementation

The ordinary Dialogue writer differential reproduced lost Gameplay effects in
all three Gameplay modes. In HorPlus and RuntimeCombine, the legacy argument
was 153.931 degrees while Authority forwarded 130.311. In AspectRecalculation
and during disable cleanup, the numeric comparison alone missed absent physical
operations. See gates-red.json and gates-green.json.

The engine-backed Writer now runs the Gameplay physical pipeline during
ordinary Dialogue, EXIT and rearm phases. It retains Dialogue writer context
for baseline exclusion. A confirmed provider continuation uses the shared
physical helper with its distinct input-mapping inputs. Gameplay disable and
AspectRecalculation gate writer effects separately from Dialogue ownership.
Dialogue disable revokes the correction latch and prepared setter witness.
Pending mode restoration continues to block the FOV argument mutation.

For same-source EXIT recovery, Authority now derives endpoint decisions from
fresh engine fields. RuntimeCombine requires confirmed aspect readback before
resuming Gameplay, with the cached Cinematic target held for eligible samples
while confirmation fails. Native recovery completion and the depart/return
exclusion progress inside the composite Writer event. AspectRecalculation's
descending handoff consumes a single aspect/flags attempt, with replay success
recorded only on a successful write. Tests exercise both outcomes and the
subsequent native endpoint. These recovery tests exercise the typed ports
offline. They do not establish foreign-source retirement or provider handoff
parity across the new physical path.

Final focused CTest passed 3/3 on resumed sources, including the original
OFF/ON differential and the expanded physical/Cinematic transaction fixtures.
The supervisor verified process cleanup. See resumed-ctest.json.

The remaining cutover work includes source replacement during recovery,
provider/Dialogue native binding including atomic handoff integration, the
settings/reset/publication paths and replacement of live callback mutation
sites as one migration.

## Receiver replacement and Dialogue setter boundary

The engine-backed Writer now uses the preceding observed receiver and the
production recovery retirement predicate. A verified change away from the
recovery owner retires its scope and invalidates its reference before the same
invocation enters the Gameplay pipeline. The new receiver still needs separate
baseline admission. A foreign writer without an owner departure updates writer
history only. Retained capabilities and the baseline stay with the old receiver.
Both cases initially failed with Diagnostics OFF and ON and pass after the
change. See receiver-red.json and receiver-green.json.

The gated Dialogue setter port now exercises the same Writer parity fixtures.
The read sample (XMM6) and the argument to mutate (XMM1) are separate captured
inputs. The port preserves XMM1 on pass-through, refuses a mismatched capture
without a state commit and compares mutation intent with production. Provider
cases still use replayed causal evidence. The live armer/setter/writer join and
native callback routing remain to be bound, together with settings, reset and
publication ownership. Focused validation is recorded in
receiver-setter-ctest.json.

## Provider correlation and runtime-facing data boundaries

Authority now runs the existing PostExitProviderHandoff tracker on the
engine-backed path. The provider armer is admitted against the selected recovery
scope. The tracker joins the captured setter arguments to the Writer sample and
checks progression. A successful mapped endpoint completes recovery and opens
one provider setter admission. Continuation checks whether the camera still
holds the corrected setter argument before deciding whether to map again.
Production and Authority now call the same numeric provider mapping helper.
The offline sequence uses captured fields through the ports without a synthetic
arming ID or caller-supplied causal join verdict and runs in both diagnostics modes.

Authority also accepts existing FeatureConfig and typed RuntimeSettingMutation
values under CameraCorrectionGate. Shutdown clears provider claims, baseline
and pending physical operations. ReadCameraEvidence projects the committed
baseline, current writer argument and correlated readbacks. A cinematic boundary
retires the prior Writer transaction for publication.

Focused CTest passed 3/3 after the last change and cleanup was verified. Evidence
is in provider-settings-ctest.json. Production still routes its native callbacks
through the existing runtime owner. The remaining M2 integration must replace
those mutation calls and migrate their settings and snapshot consumers as one
cutover. The read-only projection presently covers camera FOV evidence, while
the overlay, zoom and telemetry consumers still need their production bindings.

The native Writer port now binds the scalar, receiver, output, aspect, flags
and thread without consuming adapter-side ownership classification or
Gameplay enable decisions. PostStore and PostHelper use a separate readback
entry. Authority compares the sample with its prepared argument and validates
the full observation identity before publishing. The overlay lifecycle
projection combines settings, coordinator, selected policy and FOV evidence
under the camera gate. The focused native-shaped provider sequence runs with
Diagnostics OFF/ON and rejects a duplicate readback or reuse of a closed window.
The final focused CTest was 3/3 with verified process cleanup
(fact-boundaries-ctest.json).

Live cutover remains incomplete. A call-site check still finds direct mutable
views in Cinematic and Dialogue, provider armer/setter/Writer calls on the old
handoff tracker, baseline Project/Invalidate calls and Runtime's existing
settings transaction. No live mutation paths were removed in this batch.
The new overlay projection is exercised by fixtures. Production overlay,
zoom and telemetry have not been switched to that projection.

## Native capture and Cinematic reentry

The live provider callbacks reuse one capture of their native FOV fields. The
Dialogue setter checks address arithmetic before reading its target field.
Removed three unreferenced legacy mutation wrappers from runtime.cpp:
ClearCinematicRecoveryAuxiliaryState, CompleteCinematicRecovery and
ArmAtomicExitHandoff. Their former callers were already absent.

The Authority Writer and Dialogue setter ports accept native-shaped inputs.
Writer admission and live selection generation come from Authority; the
setter port does not supply a claimed generation or provider admission.
An accepted Cinematic ENTER revokes the preceding scoped provider claim
even if a foreign receiver supplied the last writer. When the predecessor
actually supplied the last writer, corrected-start evidence is captured
before retiring the provider.

An engine-backed fixture reproduced a second Cinematic activation losing
target retention without an armer. The production and Authority ENTER paths
now use the same continuation predicate. The writer needs the admitted
receiver/output and a validated preceding EXIT. The fixture's negative
control submits a sample outside the authored retention tolerance.

Remaining live paths include ProcessCameraWriter, ProcessDialogueBoundary,
ProcessAcceptedCinematicEnter/Exit, the runtime provider tracker, settings
and reset, Gameplay baseline writes, and reads in zoom, overlay and telemetry.
The production callback owner is still the pre-cutover runtime. This remains
an implementation checkpoint.

Verification: focused CTest 3/3 (continuation-verified-ctest.json), 18
Cinematic continuation native-adapter invocations in Diagnostics OFF/ON
(continuation-green-*.json), and 16 paired native-adapter invocations on the
preceding fact-boundary checkpoint (runtime-capture-*.json). Test and build
supervisors reported exit 0 and verified process cleanup for the green runs.

## Production cutover, 2026-11-18

The ordinary production ASI runs all active camera callbacks through the
process-resident Authority under CameraCorrectionGate. Native leaves capture
registers, validated caller evidence and camera fields, then execute only the
prepared argument or requested aspect operation. Settings and shutdown use the
same serialized owner. Stop closes callback admission and drains callbacks and
workers before retiring Authority.

| Shipping boundary | Authority binding | Superseded runtime path |
| --- | --- | --- |
| Primary Writer and recovery | WriterBoundary.ProcessNative, Observe, Close | ProcessCameraWriter, CameraWriterRecoveryStateView, GetCameraWriterGameplayStateView, GetCameraWriterCinematicStateView, writable coordinator, restoration and Gameplay baseline stores |
| Dialogue setter and provider armer | DialogueBoundary.ProcessNative and Arm | GetDialogueBoundaryStateView, ProcessDialogueBoundary, RuntimeState PostExitProviderHandoff, independent candidate and provider tracking |
| Cinematic ENTER, aspect, armer and EXIT | CinematicBoundary.Enter, Aspect, Armer and Exit | GetCinematicLifecycleStateView, ProcessAcceptedCinematicEnter/Exit, ObserveCinematicAspectStore, writable association, activation, continuation and recovery fields |
| Settings, startup and reset | Authority.Initialize, ApplySetting, ReadSettings and Shutdown | ApplyCameraSetting, SelectGameplayMode, independent mode/enable epochs and direct baseline invalidation |
| Overlay, zoom and telemetry | ReadOverlayLifecycle, ReadLifecycle and ReadCameraEvidence | Reading ownership, settings, provider and Gameplay reference from independent runtime stores |

The previous mutable owner and compatibility adapters compile only in
characterization fixtures. The shipping-source ownership audit found no
superseded mutation symbols and all six required Authority binding surfaces.
Zoom, transition-boundary and writer telemetry caches contain observations.
They supply no lifecycle or native effect permission.

The production ASI linked successfully. SHA-256
A2EBAB8D05E2D3DEB3527D80115F987F79E911DA7285B08C7DDD55124EDAD07C.
Focused CTest passed 5/5. The cutover harness runs the shipping callback
branches with Diagnostics OFF and ON, executes a real SafetyHook trampoline
around the native MOVSS [RBX+30h], XMM0 form, and checks the post-store value,
provider continuation, one aspect write, register preservation, settings
revocation and shutdown. The static installed-image check passed all six
required anchors, including indexed EXIT. The installed file was opened
read-only. This evidence covers the matching executable anchors and the
executed fixture ABI. In-game scenario coverage belongs to M3.

The final canonical non-graphics regression completed with 64 PASS, 0 FAIL,
and one named hardlink ownership skip (Windows error 5). WARP was omitted
by the non-graphics profile. The legacy adapter matrix remains
characterization, alongside the separate shipping cutover fixtures.
Build, focused checks, installed-image scan and the canonical runner all
reported verified process-tree cleanup.

Current evidence: cutover-shipping-build.json, cutover-shipping-ctest.json,
cutover-installed-abi-run.json, cutover-shipping-regressions.json, and
cutover-owner-source-audit.json in the M2 evidence directory.
