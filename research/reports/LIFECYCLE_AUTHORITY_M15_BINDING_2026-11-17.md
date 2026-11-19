# M1.5 Adapter Binding Verification

2026-11-17. Verdict: verification found concrete binding incompatibilities;
M2/M3 NO-GO. This is not a blanket PASS for adapter readiness. Core, production,
R1, ASI and installed game unchanged. M1 hardening acceptance is preserved.

## Executed counterexamples

The existing `camera_writer_transaction_harness --authority-bindings` runs
the real ProcessCameraWriter through its existing RuntimeCombine fixture and
compares its first original Gameplay sample with isolated Authority admission
and effect preparation. It does not import GameplayEligible as Authority proof.
Original Gameplay is an explicit fixture premise, not a newly observed engine
fact. The underlying production fixture also exercises existing provider,
Dialogue/Cinematic and Diagnostics sequence regressions.

Observed output:

```
M15_DIVERGENCE diagnostics_off production_corrects=1 transaction_id=0 authority_prepare=0
M15_PROPOSAL adapter_owned_id_prepare=1
M15_DIVERGENCE single_writer_admission_revision=2 prepare_revision=3
M15_WINDOW_FIXTURE diagnostics=0 closed=512 pending=0 live_binding=unverified
M15_WINDOW_FIXTURE diagnostics=1 closed=512 pending=0 live_binding=unverified
M15_DIVERGENCE no_terminal diagnostics=0 accepted=128 next_correction=refused
M15_DIVERGENCE no_terminal diagnostics=1 accepted=128 next_correction=refused
```

The test PASS means these counterexamples and fixture contracts were reproduced.
It does not mean the divergences are fixed. An independent callback invocation
ID solves the demonstrated identity incompatibility; that proposal was not wired
to runtime. Admission and PrepareEffect remain separate commits. A live wrapper
could serialize them, but the present interface does not itself provide the
specified single working state and final commit for the whole Writer event.

The observation-window fixture uses an explicit finalizer over the *whole future
window*, including early return, exception, no readback and partial post-store.
It closes all 512 windows in each Diagnostics mode. This proves the isolated API
can release capacity when closure is supplied. It does not prove where a native
window ends or how current callbacks supply closure. It must not be installed as
a PRE-callback destructor that claims the later native store already happened.

## Actual binding boundaries

| Boundary | Captured facts / current path | Binding disposition |
| --- | --- | --- |
| Writer | runtime.cpp ReplayManualTransition, source RSI/output RBX, incoming XMM0, safe aspect/flags, thread | Raw capture exists. Original/prepared provenance and source policy must be decided in Authority; do not feed ClassifyDialogueWriterContext's verdict back as native proof. |
| Writer identity | runtime.cpp assigns writerTransactionId from diagnostic frame only when diagnostics active, otherwise 0 | Confirmed incompatibility with PrepareEffect's nonzero-ID requirement. Allocate an invocation identity independently of diagnostic state. |
| Recovery completion | ProcessCameraWriter can complete source-owned recovery and admit the same sample; current context classification begins before that transaction | Authority needs a composite Writer reduction over final W. Calling RecoveryComplete, GameplaySample and PrepareEffect as separate externally committed events is not the accepted contract. |
| Setter / Candidate terminal | runtime TraceDialogueBoundary; RetireStationaryDialogueCandidateOnGameplayWriter consumes bound candidate, current reference, generation/epoch and stationary endpoints | Raw setter identity is available. M1 EventFacts lacks that complete candidate evidence payload; copying the terminal helper's boolean into dialogueTerminalProven would leave the decision outside Authority. Transfer the predicate and required facts to its private reduction. |
| ENTER/EXIT | AcceptedCinematicEnterInput and actual lifecycle transaction carry selection, aspect, baseline, continuity, targets and settings | Native entry points exist; current simple EventFacts is not a lossless equivalent of these inputs/results. Existing source association and corrected-start provenance must survive projection. |
| Provider | actual armer/setter/writer helper and generation-5 captured order | Supported bounded matching exists. The future adapter must capture identities/trajectory facts; causalArmingJoinProven=true cannot replace matcher ownership migration or resolve indistinguishable rearming. |
| Settings | ApplyCameraSetting and existing settings transactions own effective modes/epochs and pending effects | SettingsChanged can represent enable/policy values, but pending aspect restoration and activation-latched policy binding are not fully integrated. Keep decisions private to Authority. |
| Physical FOV store | ReplayManualTransition changes context.xmm0, then returns to the native MOVSS | Argument prepared is not memory store/readback. PRE callback completion cannot certify Applied. |
| Post-store/helper | runtime ObserveCameraWriterPostStore/PostProcess, diagnostic frames and named readbacks | Hook installation and callback processing are gated by Diagnostics. They are not an always-present terminal mechanism. |
| Shutdown/early rejection | callback admission, gate checks, stopping and unavailable memory | A rejected callback before intent creation needs no close. After intent creation, every accepted abort/supersession path must retire execution permission and explicitly dispose of observation tracking. Current runtime does not own Authority windows. |

Source anchors: runtime.cpp approximately 6880–7020 (capture/classification),
7139–7165 (post-transaction bookkeeping), 5916/6016/6089/6190 (trace/readback),
9185–9260 (diagnostic hook installation); cinematic_lifecycle_transaction.hpp
AcceptedCinematicEnterInput; camera_writer_transaction.hpp ProcessCameraWriter;
lifecycle_authority_core.hpp GameplaySample/PrepareEffect/EffectClosed.

## Capacity and finalization decision

Neither the successful fixture finalizer nor the 128-record cap guarantees live
correction availability. With Diagnostics OFF, current hooks do not provide the
proposed observation terminal; with readback errors a pending record can also
lack a normal positive completion. Simply keeping all records pending can deny
the 129th correction. Adding another hook is not justified by this finding.

The next isolated contract change should separate short-lived execution
permission from optional observation retention. A prepared register argument
consumes/releases its execution permission at the defined executor boundary,
without claiming a native memory write. Named readback correlation remains a
separate bounded observation record, with explicit unavailable/abandoned closure.
Failure to retain diagnostics must not become a condition for FOV correction.
Existing post-store/helper sites may be reused when available; supporting an
unavailable site must not require an invented readback or disable the policy.
This is a proposed binding design, not a production change or approved fallback.

## Coverage and remaining work

- Production transaction harness default: PASS on the modified test artifact.
- Opt-in binding counterexamples/1024 observation windows: PASS, divergences
  explicitly printed and retained; readiness remains NO-GO.
- Build and each execution supervised sequentially with finite timeout/output
  caps and verified owned process-tree cleanup. No graphics/game/full suite.
- Current generation-5/PID7372 numeric shadow checks remain the M1 results;
  this stage does not claim full Authority parity for those runtime sequences.
- Receiver replacement and Cine/Dialogue/Cine are covered by the existing
  production harness; full differential Authority execution is blocked by the
  composite-event and raw-evidence binding gaps above, not declared equivalent.
- R1/core/runtime review hashes preserved in this stage's evidence directory.

Next work is a bounded test-only composite Writer binding and execution-versus-
observation lifecycle contract, driven by these counterexamples, followed by
full source-replacement and recovery/provider sequence differential tests.
Do not start live cutover or a general native evidence search to resolve these
source-level interface incompatibilities.
