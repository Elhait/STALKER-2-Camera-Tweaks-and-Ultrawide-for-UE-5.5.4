# U2 transition policy boundaries

Scope approved in the current task: repair repeated Dialogue admission first;
inspect cinematic initial establishment separately. Preserve current targets,
U1-A ownership checks and ordinary Dialogue behavior. No timer or global cooldown.

## Batch 1: completed provider activation

Evidence: runtime 18:03:39.744–.774 records writer output
108.969 → 99.2615 → 108.941 after Reduced AcceptedStart.

Retire writer recovery as before, retaining only the handled activation identity
in PostExitProviderHandoff. At the existing Dialogue boundary, suppress admission
only for matching component, receiver, thread, generation, settings epoch and
native endpoints. A new observed initializer/rearm clears the record, including
initializations not admitted for recovery. Existing ENTER/EXIT/settings/reset
paths clear it as before. No new hook or native write.

Files: post_exit_provider_handoff.hpp, dialogue_boundary_transaction.hpp,
runtime.cpp, dialogue_boundary_transaction_harness.cpp.

Validation: focused Dialogue and camera writer harnesses; compile production
adapter; whitespace check on changed hunks. No game/debugger launch or automatic
installation. Runtime acceptance remains the user's visual test of both control
dialogues and Cinematic1 → Dialogue → Cinematic2 → Gameplay.

Risk: native initialization bypassing the existing initialization observation
cannot distinguish identical reuse of component/receiver/endpoints. A pointer
or numeric match alone is never sufficient. Failed native reads do not suppress
admission. A completed record authorizes no FOV correction.

## Batch 2: cinematic interpolation assessment

Evidence: ENTER2 writer output126.87 falls to raw70.1055 when initial establishment
ends, then native blend reaches126.87. Inspect the existing continuation policy
for intermediate samples before selecting any behavior change. Do not extend
retention by time, flatten the native blend, or treat an arbitrary raw interval
as transition provenance. Batch 1 does not close this second defect.

## Current result

Batch 1 implemented. Dialogue boundary and camera writer transaction harnesses
PASS on the current source; Release single ASI compiled. Changed tracked files
pass git diff --check. Build/test process cleanup verified. The initial combined
build supervision timed out; its owned process tree was cleaned, and a bounded
build recheck returned zero. It is not reported as a passing first invocation.
Evidence and artifact SHA-256: ../evidence/u2-transition-policy-2026-11-13/.
Visual acceptance remains untested. The game ASI was not replaced.

Batch 2 assessment complete; behavior unchanged. ResolveCinematicWriterSample
in cinematic_fov.cpp does not map the observed raw70→corrected126.87 trajectory:
RetainContinuationTarget returns the final target, while the ordinary
PassThroughInterpolation interval begins at authored90. The actual early samples
70.1055…89.1544 lie outside that interval. Simply enabling continuation or
extending initial establishment would retain the final target and flatten the
native transition. ObserveCinematicEnter initializes continuation from previous
recovery ownership, not from the current successful native blend activation.
ProcessCameraWriter receives no current ENTER activation/setter provenance for
this path; existing postExitProviderEvidence is restricted to CinematicExiting.

The next behavior change must connect the current ENTER activation and its
setter samples to the writer, distinguish native-start and transformed-start
domains, and preserve corrected output across the first sample and retention
release. This is a separate integration decision, not an interval/tolerance
adjustment. Required sequence assertions: first output, all intermediate outputs,
retention-release boundary, endpoint, camera-cut cancellation, new generation,
foreign source/output, rearm and missing activation evidence. No new runtime
capture or hooks were required for this assessment.

## Runtime 18:20:50

Archived as ../evidence/u2-transition-policy-2026-11-13/runtime-182050.log.
Loaded artifact matches F07C4FC1F44783A46379D059F84BD9C4A5B077AA51CD877EE488D56CE5849CFD.
At EXIT1 the endpoint handoff publishes108.976, then Gameplay writer publishes
109.005→108.949→108.941. The callback records HandledProviderReset at18:23:38.091;
no AcceptedStart is recorded in this capture. The previous99.2615 Reduced dip
is absent from these recorded writer samples. This confirms routing in the
exercised path, not visual acceptance or the independent control dialogues.
ENTER2 still records126.87→70.1061 after authored_baseline_departure; Batch 2
remains unresolved. EXIT2 recovery completes at18:24:05.892. User supplied no
visual outcome at the time of the initial log analysis.

### Subsequent user visual result

The user reports a visible FOV jump at both boundaries; at Dialogue→Cinematic2
the symptom also appeared as a reduction in FOV. EXIT1→Dialogue therefore FAILS
visual acceptance despite the removed99.2615 writer dip and confirmed admission
suppression. Do not describe Batch 1 as resolving the first visible defect.
Dialogue→Cinematic2 also remains visually unresolved; the recorded return of
writer values to126.87 does not establish a matching rendered-camera return.

Both transition acceptance criteria remain OPEN. Harness PASS and boundary logs
only establish their exercised logic. For the first boundary, a further causal
claim needs camera/view identity and consumption order at the visible jump,
including the earlier portion of the transition rather than just its endpoint.
For the second, any implementation must preserve intermediate outputs through
retention release with verified sample provenance; target retention alone is
not a substitute for interpolation. No policy change is justified solely by
this visual report.

## Product transition requirement clarified by the user

Stable FOV in each mode is insufficient. Cinematic↔Dialogue must preserve a
continuous corrected trajectory, with the destination selected by effective
Dialogue Zoom and Cinematic mode. Different destination values are expected;
temporary rollback to native, double jumps or a foreign policy are failures.
Native easing must remain native-owned. Ownership changes must not insert an
uncorrected invocation. Camera cuts, source replacement, rearm and invalid
provenance must cancel stale transition authority rather than smooth a cut.
Writer continuity is necessary; rendered-view FOV/aspect continuity is a
separate required visual acceptance condition.

### Source findings against this requirement

1. TryMapPostExitProviderDialogueSample in camera_writer_transaction.hpp gates
   on Dialogue enable/policy, but its corrected endpoint is always
   EvaluateHorPlus(activation.nativeTarget,...). It does not evaluate
   AdaptiveTarget, ReducedTarget or Disabled's baseline endpoint. Thus the
   existing U2 mapping does not implement settings-dependent Dialogue endpoints.
   This is a confirmed semantic limitation, not proof of the remaining jump.
2. TracePostExitBlendStartCapture reads receiver+0x230 into receiverFov and
   passes that value as continuityStart to ObserveArmer. This is a native camera
   field observation, not an observed corrected-writer output or rendered start.
   The input-field and output-space endpoints must remain explicitly distinct.
3. ResolveCinematicWriterSample's RetainContinuationTarget always returns the
   final target. Reusing it for the observed ENTER native70→126.87 activation
   would flatten rather than map that transition. Its pass-through path likewise
   does not provide corrected intermediate output below authored90.
4. The working Cinematic→Gameplay recovery derives a mapped output between a
   cached cinematic endpoint and a Gameplay policy endpoint. It is a reference
   for handoff continuity, not proof of active-view identity or a generic
   Cinematic↔Dialogue mapping. Its numerical classifier must not be generalized
   to unfamiliar samples.

### Bounded next implementation design

Keep repeated-admission suppression unchanged. Resolve the two boundaries
independently, documenting: native activation/start/target and sample domain;
authorized source/generation/receiver; corrected input boundary and aspect;
destination policy/reference; mapping of the complete admitted sample sequence;
handoff endpoint and post-handoff policy equality; interruption/cut/rearm/settings
cancellation. Do not derive a Dialogue reference from transformed receiverFov,
nor assume the latest writer snapshot is a rendered-frame observation.

Sequence validation must cover each effective Dialogue Zoom and Cinematic mode,
aspect changes, source/output changes, same-address rearm, missing provenance,
native and transformed starts, equal endpoints, and the first writer after
handoff. Inspect intermediate outputs and policy changes, not just endpoint
equality. A new runtime observation is justified only by a named missing
camera/view consumption fact. No new policy mutation or hooks were made during
this requirement assessment.

Endpoint prerequisite assessment is recorded in
../reports/U2_POLICY_ENDPOINTS_2026-11-13.md. It distinguishes observed native
values from computed policy endpoints, establishes Disabled as zoom suppression
with independent Gameplay correction, and records GameplayHorPlus migration to
canonical NativeHorPlus. Policy-qualified Dialogue baseline and corrected/view
start remain explicit unknowns; the table does not silently authorize a mapping
from transformed native fields. No production edits/build/runtime in this phase.

The separately requested bounded adapter design is recorded in
[U2_TRANSITION_ADAPTER_DESIGN_2026-11-13.md](U2_TRANSITION_ADAPTER_DESIGN_2026-11-13.md).
It defines two local adapters with per-invocation mutation ownership, explicit
missing-evidence fallback, destination policy delivery and cancellation rules.
Design completion does not close production feasibility or visual acceptance.

Additional user-reported Native FOV display behavior audited read-only in
[NATIVE_FOV_OVERLAY_PROVENANCE_AUDIT_2026-11-13.md](../reports/NATIVE_FOV_OVERLAY_PROVENANCE_AUDIT_2026-11-13.md).
Overlay fact is current transaction input, not immutable Gameplay reference;
upstream mod effects can re-enter through native camera state. No direct
Gameplay input/result assignment error was found. Stable Gameplay toggle
causality and actual active-view consumption remain unestablished. Production,
UI and policies unchanged; consumption experiment not started.

User's stronger original-Native-FOV invariant audited in
[NATIVE_FOV_IMMUTABILITY_INVARIANT_AUDIT_2026-11-13.md](../reports/NATIVE_FOV_IMMUTABILITY_INVARIANT_AUDIT_2026-11-13.md).
Saved pre-Batch1 evidence confirms Dialogue output60.9227 admitted as Gameplay
baseline.native60.9227. Current baseline tags do not enforce original game-origin
domain. This semantic prerequisite must be resolved before dependent adapter
implementation; no code or runtime changes were made in the audit.

## Runtime retest — 2026-11-14

The user-loaded artifact hash matches the P0 endpoint-fix Release. The bounded
P0 quarantine passed its intended endpoint sequence; full records and detail
are in `../evidence/p0-u2-runtime-2026-11-13/runtime-retest-assessment-2026-11-14.md`.

For Dialogue→Cinematic2, generation2 ENTER observes native input90 and target
126.87, while the pre-enter writer input is70. Its first CinematicActive writer
passes70 unchanged; `HORPLUS_CINEMATIC_WRITER` reports `applied=NO`. The
native blend activation candidate has start70 and target126.87, but shadow
records report missing activation/context, unknown sample space, unobserved
branch, mapping unverified and rendered start unobserved. This is runtime
evidence of the raw writer path, not proof of an active-view jump. The native
interpolation must not be remapped from an assumed weight or a receiver field.

Both cinematic consumer observers reached `InitializerObservationReached`,
with `native_start_commit=unverified`. The log contains no rendered-view FOV.
The production transition mapping remains unchanged. The new capture does not
settle corrected-start, policy-qualified destination, active-view consumption,
or native eased-weight provenance.

## User visual confirmation — 2026-11-14

After reviewing the 2026-11-14 runtime retest, the user confirmed visible FOV
jumps at both `Cinematic 1 → Dialogue` and `Dialogue → Cinematic 2`. This is
direct visual evidence for the integrated run, but the user did not identify the
frame, direction, magnitude, or whether the displayed FOV increased/decreased
at each boundary. Therefore both visual acceptance criteria are confirmed
FAIL; neither log writer continuity nor endpoint telemetry overrides this
result.

Keep the boundaries as separate defects. For `Dialogue → Cinematic 2`, ENTER2
still shows native blend samples and no verified corrected start or eased-weight
mapping. For `Cinematic 1 → Dialogue`, the prior admission suppression removed
the logged reduced-baseline contamination but did not resolve the visible jump.
The next runtime capture, if needed, must target synchronized active-view or
projection consumption at the reported jumps; do not repeat the P0 admission
checks already settled by the current log.

## Offline implementation and Release candidates — 2026-11-14

### Cinematic 1 → Dialogue / completed provider handoff

Implemented a source-matched, one-shot continuation from a completed post-exit
provider activation into the Dialogue lifecycle when the previously admitted
Gameplay baseline is still valid. Dialogue seeds its own policy baseline from
that trusted Gameplay reference; the approximately70-degree provider endpoint
is not admitted as a Gameplay baseline. Subsequent Dialogue writer callbacks
remain lifecycle-owned until the natural recovery sequence completes. Gameplay
HorPlus evaluation remains independent of whether a FOV sample is admitted to
the baseline. Missing or mismatched source/baseline evidence keeps the prior
fail-closed behavior. This closes the offline baseline-contamination contract;
it does not yet establish rendered continuity at the visible boundary.

The `Dialogue → Cinematic 2` production mapping is not changed: available
evidence still does not establish the corrected start domain or a generally
valid native easing weight for this branch. The diagnostic build now shadows
the ENTER2 transition using an eligible pre-ENTER writer result as C0 only
when source, receiver, generation, aspect and cinematic target provenance
match. Shadow output is observational and does not mutate the production
writer. It records destination and missing-evidence reasons to distinguish
whether later runtime data supports a mapping.

### Offline validation

- `dialogue_boundary_transaction_harness`: PASS, including one-shot provider
  admission, Dialogue baseline quarantine through the reduced endpoint,
  ordinary callback processing, complete natural Dialogue recovery and return
  to `GameplayEligible`.
- Canonical `test.cmd` harness suite: PASS on this working tree, all62 ordered
  harness invocations; includes diagnostics OFF/ON in the runtime-native
  adapter and the Dialogue/provider regression cases. This is deterministic
  offline coverage, not a game callback or visual result.
- Production Release profile (timeline OFF, discovery OFF): PASS. Artifact:
  `build-artifacts/p0-u2-fix-2026-11-14/production/STALKER2CameraTweaks.asi`,
  3,880,960 bytes, SHA-256
  `691001D158A99143854AC91232E27A3F4300CA23094BEBB9EEAC7CE8EC01504C`.
- Runtime uses one Production Release ASI. The repository's separate
  `CAMERA_TWEAKS_PROFILE=diagnostic` setting only selects a diagnostic output
  filename (with optional, separately gated Dialogue discovery); it is not
  required to enable the bounded observations in this task. Those observations
  are part of the Production binary and are enabled by
  `[Diagnostics] Enabled=true` in the INI. The alternate diagnostic-profile
  build was checked but is not a runtime candidate for this test.
- Both builds were rechecked under the project process-tree supervisor; exit
  codes were0 and process cleanup verified. No game process was running. Neither
  artifact was installed over the user's game copy. Build manifests and exact
  process/build records are alongside the candidates in the same dated folder.

### Status and next runtime acceptance

P0 baseline admission is offline- and runtime-validated for the exercised
Dialogue path; final native `camera+0x230` writer provenance remains pending.
Do not repeat that admission scenario absent regression. The two U2 visual
defects remain OPEN. No visual correction is
claimed from these builds. For the next bounded runtime pass, use the single
Production candidate with Diagnostics enabled in the INI, and capture only
Cinematic1 → Dialogue and Dialogue → Cinematic2, including overlay FOV/aspect
immediately before and after each. Keep effective Gameplay, Dialogue and
Cinematic settings unchanged; verify the startup capabilities listed in the
operator runbook. The latest runtime log confirms this candidate was loaded;
the installed file's state after that run is unknown.

## Runtime result on the Production candidate — 2026-11-14 00:55–00:59

Archived the user's full log at
`../evidence/p0-u2-runtime-2026-11-13/runtime-2026-11-14-005523.log`
(SHA-256 `4A79098A21F176D1451FE99A75D194402C8FDB78D2DB08BB86B22AF6BF9AC5E1`).
Its loaded mod hash matches the Production Release candidate
`691001D158A99143854AC91232E27A3F4300CA23094BEBB9EEAC7CE8EC01504C`; game
image hash is `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`.

- **P0 provider Dialogue admission:** before Cinematic1 the Gameplay baseline
  was90.3115. At00:58:09 the provider Dialogue callback started with input
  70.1099 and produced80.2024 under Reduced policy, while its Gameplay
  baseline source remained90.3115. Projection records retain rather than
  admit the DialogueLifecycleObserved sample. This is runtime evidence that
  the endpoint no longer contaminates the baseline in this exercised path;
  native final-writer provenance at `camera+0x230` remains unknown.
- **Dialogue without reloading a save:** after Cinematic2 recovery,
  `U2_CONTROL_BEGIN phase=1` records Gameplay coordinator and inactive
  exclusion. Baseline admission updates to90.2954 with
  `writerContext=GameplayEligible` before the separate Dialogue callback.
  That Dialogue reaches `RecoveryCompleted` and the coordinator returns to
  Gameplay in the same process. No second control phase or save reload is
  needed for this check. A post-recovery eligible baseline update was not
  recorded after the final recovery event, so that specific runtime fact is
  not established by this bounded log.
- **Dialogue → Cinematic2:** ENTER2 begins with pre-enter writer input70;
  matched native activation reports start70 and target126.87. The first
  post-ENTER writer still receives70. The shadow reports
  `missing-activation-or-context`, no corrected start, unknown easing branch,
  and unobserved rendered start. It therefore did not establish a safe
  production mapping. Production behavior remains unchanged at this
  boundary.
- This log contains no rendered-view measurement. The user's earlier report
  of jumps at both boundaries remains the visual acceptance result. Keep both
  boundaries OPEN. Do not request a save reload to repeat the Dialogue
  control already captured in this same-session log.

## Focused source correlation — 2026-11-14

The user's runtime/log correlation identifies a concrete Cinematic1 writer
failure path. At00:57:48.239 the admitted writer source/output is corrected to
126.87; at00:57:48.334 the same source/output has ended initial establishment
and the dispatcher returns `PassThroughUnclassified` for raw91.0413 while the
native blend is still active. In current `ResolveCinematicWriterSample`, the
missing/mismatched `continuationSource` check returns before any later interval
handling. This is a confirmed writer-policy defect for the logged path, not a
rendered-view causal proof. A raw interval check cannot repair it because it
would not map the native easing trajectory.

The bounded `POST_EXIT_BLEND_SAMPLE` records correlate the sampled native
weight with `(raw-start)/(target-start)` at low error for this ENTER activation.
That establishes a promising affine native-progress observation for those
samples, not a general easing contract or production authority. A safe mutation
still requires a non-diagnostic activation epoch paired to each exact admitted
writer source/output and an evidence-qualified corrected start/endpoint. The
same-run C2 activation starts at70 with target126.87, but no corrected start is
available; the saved Gameplay baseline90.3115/127.119 is historical and was
explicitly unusable as current camera state. Do not substitute it.

P0 and U2 remain separate: P0's control Dialogue already ran in-session and
returned through `RecoveryCompleted`; no reload or repeat is requested. U2
writer discontinuity at C1 and the C2 start-domain gap are now narrowed to
their respective seams. No production behavior was changed in this
correlation pass. Next implementation must pair ENTER activation with writer
samples and fail closed when corrected start provenance is absent; it must not
add a cooldown or infer rendered continuity from writer values. Both reported
visual boundaries remain open pending an artifact that implements the
supported mapping and an integrated visual retest.

## Approved next batch — production ENTER activation ownership

Scope: add a bounded, production-owned ENTER activation record independent of
Diagnostics; correlate only exact generation/settings/component/receiver and
admitted writer source/output transactions. Capture the native start/target
from the validated initializer seam already present in the Production ASI.
Accept a corrected start only when a current pair matches the activation's
native start and source. Apply the conditional affine mapping only to admitted
writer samples bound to that activation and only when this corrected start is
valid. Missing corrected start,
foreign receiver/source/output, replaced generation, changed settings, cut or
out-of-range samples remain fail-closed with explicit reasons. This batch
establishes and applies the supported production mapping for the qualified C1
path; it does not claim rendered continuity or change Dialogue policy
endpoints. C2 with missing corrected-start provenance remains fail-closed.

Validation: deterministic activation lifecycle/identity and complete writer
sequence tests; focused runtime-native adapter checks; full ordered
`test.cmd` harness suite; one Production Release ASI build. Keep one ASI and
Diagnostics INI toggle. No save reload or repeated P0 control Dialogue.

### Implementation result — 2026-11-14

`CinematicEnterActivation` now belongs to production `RuntimeState`, is armed
for each accepted ENTER, and captures only the existing validated
initializer's exact CinematicEnter caller. It is not gated by Diagnostics.
Capture carries generation/settings epoch, component/receiver/thread, native
start/target, and a corrected start candidate only when the currently usable
Gameplay native/transform pair matches the activation receiver and native
start. The per-writer transaction then requires existing exact invocation
admission for source/output, current settings epoch, receiver match, valid
native segment and matching native/corrected target endpoint. Output addresses
are checked per invocation and may change between calls. The affine sample
mapping is applied only under those conditions; it retires at endpoint or on
stale generation/settings, foreign admitted source, invalid sample or leaving
the activation segment. Diagnostics emits at most the first16 samples plus a
terminal status. The C2 `70°` start cannot borrow the old90.3115° Gameplay
baseline, so it remains unmodified with `missing-corrected-start` provenance.

The camera writer transaction regression confirms mapped midpoint and exact
per-call admission; the cinematic harness covers source/output, generation,
settings, missing start, endpoint and retirement. The final
`test.cmd -NonGraphics` invocation configured/built the current harnesses and
passed registry audit/runner exit-contract checks, but the canonical
62-invocation run stopped at the unrelated `config_persistence_harness`
hard-link fixture (`CreateHardLinkW` error5 in the process temp directory). A
filtered run on the immediately preceding source passed the remaining61
invocations, including the runtime-native adapter's228 child scenarios with
Diagnostics OFF/ON. After the last C2 endpoint-retirement change, the three
focused harnesses were rebuilt and passed, including those228 runtime adapter
scenarios. Thus the full canonical suite is not reported as PASS; the
environment-failing config persistence invocation remains unvalidated on the
final source.

Production Release ASI rebuilt with startup timeline/discovery OFF and
Diagnostics controlled by the INI. Candidate:
`../build-artifacts/p0-u2-enter-activation-2026-11-14/production/STALKER2CameraTweaks.asi`,
3,905,024 bytes, SHA-256
`B1F2D825A01D61EBCA0C72D9114553E515D5C5D3DE94F9C68CB3D21C2E66EAF0`.
Source and packaged candidate hashes match; manifest is
`../build-artifacts/p0-u2-enter-activation-2026-11-14/candidate-manifest.json`.
The build emitted CMake long-object-path warnings but completed the link with
exit0 and verified process-tree cleanup.

No in-game run has loaded this new hash. Writer mapping is now implemented for
qualified activations; rendered-view continuity at both user-reported
boundaries remains unverified, and C2 deliberately remains fail-closed until a
current Dialogue-corrected start is present. P0's same-session control
Dialogue is already complete and must not be repeated.

## Runtime result — 2026-11-14

The candidate Release was loaded in-game; runtime identity confirms ASI SHA-256
`B1F2D825A01D61EBCA0C72D9114553E515D5C5D3DE94F9C68CB3D21C2E66EAF0` and game
image SHA-256 `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`.
The full log and focused assessment are archived under
`../research/evidence/u2-enter-activation-runtime-2026-11-14/`.

Generation 1 mapped 64 admitted writer samples from native `90.3115` to
`126.87` using corrected start `127.119`, ending with
`production_mutation=1`. Despite that writer continuity, the user observed a
visible Cinematic 1 → Dialogue jump. At the next validated provider handoff,
raw/continuity start was `120.46`, matched setter sample `120.434`, and applied
output `120.451` toward Dialogue target `108.941`; this is about `6.42°` below
the prior cinematic writer endpoint. It is the concrete remaining writer seam
candidate, while rendered ordering/value remains unobserved.

Generation 2 began at native `70` with no corrected start; samples remained
`missing-corrected-start` and `production_mutation=0`. The user observed the
Dialogue → Cinematic 2 jump. Following recovery, the baseline returned to
`GameplayEligible` and updated at sequence 2273 (`90.3071 / 127.115`), so P0
admission recovered in this session.

The user also tested another cutscene after loading a save. Generation 3 began
at native `90.3115` and target `126.87`, but the current receiver
`0x2948da1f340` differed from the retained Gameplay baseline source
`0x294850b4d80`. The activation therefore had no current corrected start and
all sampled writer records remained fail-closed through sample 188 near the
native endpoint. This is consistent with the reported ENTER jump. Do not weaken
source identity or borrow a historical baseline; investigate a current
receiver-bound transform/provenance source.

Next bounded implementation work: establish whether Dialogue's current
corrected output can seed C2 activation and whether the cinematic EXIT
provider handoff should continue from the prior corrected writer endpoint
rather than its lower raw provider start. Preserve fail-closed behavior when
current provenance is absent. The three reported visible jumps remain open;
logs establish writer behavior only, not rendered-view acceptance.

## Approved follow-up — corrected post-EXIT provider continuity start

New runtime evidence isolates a writer discontinuity on generation 1: the
existing Gameplay recovery maps native `120.46` to about `126.914`, but the
provider Dialogue handoff seeds its output mapping with the raw receiver field
`120.46`, producing `120.451` on the first paired sample. Implement a narrow
change in the existing writer transaction: derive the provider mapping's
output start from the current generation/source-bound HorPlus recovery mapping
at that provider activation's native start. Reuse the existing recovery
contract and current aspect/flags; do not use a historical Gameplay baseline
as the corrected start. If recovery provenance or current fields are invalid,
retain current fail-closed/pass-through behavior. Preserve native activation
weight and Dialogue destination policy. Add a transaction regression proving
continuity at activation start and endpoint. C2 ENTER corrected-start admission
and other receiver identities remain outside this follow-up and fail-closed.

Validation: focused provider handoff transaction and relevant recovery/dialogue
harnesses, then canonical non-graphics harness subject to its known hard-link
fixture limitation; rebuild one Production Release candidate. Runtime and
rendered continuity require a later in-game check and are not claimed here.

## Follow-up result — corrected provider continuity start

A narrow production repair now seeds the post-EXIT provider-to-Dialogue
projection mapping with the current recovery transform evaluated at that
provider activation's native start. It uses the active recovery generation,
receiver, current camera aspect/flags, EXIT native target, and cached
transformed cinematic endpoint. Raw `camera+0x230` remains explicitly named
`nativeContinuityStart` and is not treated as corrected output. Invalid or
missing recovery state keeps the previous fail-closed/pass-through behavior.
The new transaction regression exercises the observed `120.46°` native start
against recovery endpoints `90.3115° / 126.87°`; the first mapped output is the
recovery-corrected start (~`126.914°`), continuing toward Dialogue's current
policy target. Diagnostics now reports native and corrected starts separately.

Final-source validation: `test.cmd -NonGraphics` audit, harness build, and
runner exit contract passed; canonical execution again stopped at
`config_persistence_harness` invocation 10/62 on Windows `CreateHardLinkW`
error 5 (`occupied=1`), with cleanup verified. Focused camera writer,
Dialogue-boundary, cinematic FOV and runtime-native adapter harnesses passed;
the adapter passed 228 fresh OFF/ON processes. Production Release linked
successfully with supervised cleanup.

Candidate is
`../build-artifacts/u2-provider-continuity-fix-2026-11-14/production/STALKER2CameraTweaks.asi`,
3,906,048 bytes, SHA-256
`A439FC24FFD46907A91F5056A0B9AF0BFEA8D629CAADA5F867119E24CDE74BEE`.
It is one Production ASI with Diagnostics controlled by the existing INI; its
manifest verifies matching source/package hashes. No runtime has loaded this
new candidate. C2 still has no current Dialogue-policy corrected start and
remains fail-closed; the additional post-load camera receiver case also remains
unmapped. Rendered-view continuity at all reported boundaries is still open.

## Runtime retest — provider handoff and ENTER2 — 2026-11-14

Archived the user-supplied run under `../evidence/u2-provider-continuity-runtime-2026-11-14/`.
It loaded the A439FC24 provider-continuity candidate against game image
2BD3F115. Generation1 ENTER writer mapping reaches126.87 from corrected start
127.119. The first exact post-EXIT setter/writer pair is continuous at
126.004→126.866 from corrected start126.876. This confirms only that paired
sample: subsequent native samples descend125.836→70.1896 while the log marks
Gameplay HorPlus `applied=false reason=POST_CINEMATIC_RECOVERY_HOLD`; Dialogue
callbacks return their incoming sample with `result_mutated=0` while exclusion
is active. The complete provider trajectory therefore remains uncorrected by
this candidate even though the derived `CAMERA_FOV_STATE.output` is smooth.
That derived field is not mutation or rendered-view evidence.

Generation2 ENTER starts70→126.87 with `corrected_start_valid=0`; sampled
writer records remain `missing-corrected-start`, `production_mutation=0`. The
Gameplay baseline90.3115/127.119 is retained through provider endpoint and
becomes eligible again after recovery; no new P0 baseline contamination is
observed. The user reports visible widening then narrowing at transitions and
a jump at another cutscene after save load. Those are visual reports; the log
supports the uncorrected-writer-path diagnosis but cannot identify active-view
consumption order or establish rendered causality.

Next U2 implementation must cover the complete activation-bound native sample
sequence, use the effective Dialogue destination policy and a current
receiver-bound corrected start, and cancel on activation/source/thread/
generation/settings/cut/rearm/segment mismatch. Do not infer these endpoints
from a historical Gameplay baseline or from `CAMERA_FOV_STATE.output`. P0
remains runtime-confirmed only for baseline retention/recovery paths exercised
here. Rendered transition acceptance remains open. No production change or
repeat runtime pass was made during this evidence review.

## Offline U2 implementation — activation ownership and full writer mapping — 2026-11-14

Production now keeps one activation record for an accepted Cinematic ENTER and
qualifies writer mapping by that activation's receiver/source, thread,
generation, settings epoch and native segment. The corrected start can come
from a current receiver-bound Gameplay sample or from the current Dialogue
policy while Dialogue is active/exiting. If Dialogue provenance is active but
unavailable or stale, the implementation does not fall back to historical
Gameplay baseline. Missing corrected start remains pass-through. Mapping uses
the native activation's sample weight and one admitted writer transaction.

The post-EXIT provider mapping continues across its monotonic same-activation
writer trajectory after the initial exact setter/writer pairing and evaluates
the destination against the effective Dialogue policy. Invalid/reversed or
identity-mismatched samples retire the mapping. This is writer-transaction
continuity; it does not establish active-view consumption or rendered
continuity.

Regressions cover second trajectory samples, reversal cancellation, policy
endpoints, Dialogue corrected-start provenance, stale Dialogue epoch, current
sample start, and fail-closed suppression of Gameplay-history fallback while
Dialogue context remains active. Focused camera/U2 harnesses and the 228-process
Diagnostics OFF/ON runtime adapter harness pass. `test.cmd -NonGraphics`
registry audit, harness build, and runner-exit contract pass; canonical suite
stops at invocation 10/62 because the unrelated hardlink fixture receives
Windows error 5 (`occupied=1`). The remaining registered invocations were then
run individually. `localization_harness` fails its generated-INI text equality
assertion at `tests/overlay/localization_harness.cpp:271`; all other remaining
invocations, including the final four N06 cases, pass. The localization/config
documentation consistency issue is outside this U2 change and remains open.
Process cleanup was verified for all runs.

One Production Release ASI was prepared for visual/runtime review:
`build-artifacts/p0-u2-runtime-candidate-2026-11-14/STALKER2CameraTweaks.asi`,
SHA-256 `0BB27A9F536436C07F7EDFEF89061A040B775FE52FAFCC702EECC41DAFD21853`,
3,910,144 bytes, startup timeline OFF and dialogue discovery OFF. Diagnostics
remains INI-gated; no separate ASI was produced. The exact single-session
scenario is in
`../evidence/u2-activation-ownership-runtime-2026-11-14/operator-runbook.md`.

Status: offline writer-continuity implementation/build validated; runtime
integration and visual acceptance pending. P0 remains offline-validated and
runtime-pending for final producer/def-use of `camera+0x230` and receiver↔writer
identity. Do not infer rendered causality for either reported jump until the
user's next-run observation is correlated with this exact artifact.

## User-reported provider Dialogue endpoint jump — 2026-11-14

The user reports that the prior repeated FOV rocking is gone, but after the cutscene the FOV moves gradually and then jumps at the end of the Dialogue transition. The user-supplied runtime log confirms the corresponding writer boundary: first provider Dialogue callback output `80.2035°`, first writer output `118.602°`, next native writer input `70.0357°` and output `108.977°`. The generic context detector reset the active Dialogue lifecycle on the intended `90.3115° → 80.2035°` transition. The baseline remained `90.3115° / 127.119°` and was retained.

A source-matched provider Dialogue writer continuation was added. It requires the consumed completed provider activation, receiver/thread/generation/settings identity, active bound Dialogue lifecycle and a one-shot callback-result match for the first writer sample. That initial sample uses the ordinary Gameplay HorPlus output. Later native samples map through the active Dialogue destination policy with the observed native target before Gameplay HorPlus. This removes the witnessed raw-native endpoint falling through the ordinary Gameplay mapping. It does not assert which writer output reached the rendered view.

Offline regression and Release candidate validation are recorded in [`u2-provider-dialogue-endpoint-runtime-2026-11-14`](../evidence/u2-provider-dialogue-endpoint-runtime-2026-11-14/assessment.md) and the artifact manifest at `build-artifacts/u2-provider-dialogue-endpoint-fix-2026-11-14/candidate-manifest.json`. Runtime visual acceptance is still pending. The optional post-recovery control Dialogue was absent from the supplied capture; it is not required as a separate reload or immediate standalone test.

### Dialogue → Cinematic start retest and offline repair — 2026-11-14

The user's latest visual report identifies a returned Dialogue→Cinematic jump.
The retest log shows generation2 writer pass-through because the Dialogue start
candidate was formed from authored ENTER XMM0=90 and did not match the blend
setter start80.1509. The value80.2035 is not treated as pristine original-native
Dialogue FOV. A bounded repair now seeds C0 from a receiver/thread/settings/
aspect/context-qualified Dialogue writer observation and requires its input to
match the later activation start. Details, validation and limits are in
[`U2_DIALOGUE_TO_CINEMATIC_START_HANDOFF_2026-11-14.md`](U2_DIALOGUE_TO_CINEMATIC_START_HANDOFF_2026-11-14.md)
and [`u2-dialogue-cinematic-retest-2026-11-14`](../evidence/u2-dialogue-cinematic-retest-2026-11-14/assessment.md).
Focused offline harnesses pass and one Release candidate is prepared. The
canonical non-graphics registry still stops at invocation10/62 on the unrelated
Windows hardlink fixture error5. Rendered-view acceptance is pending.
