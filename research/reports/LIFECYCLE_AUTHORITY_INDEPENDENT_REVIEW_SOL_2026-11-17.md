# Independent Lifecycle Authority review — Sol

2026-11-17. Static review of the actual isolated C++ core, harness, production
classification/provider helper, preserved generation-5 log and native decodes.
No production, oracle, harness, ASI or game changes. No tests were executed.
Existing 45/29 assertion results are historical reported results, not a fresh run.

## Decision

M2 live cutover remains NO-GO, but new-receiver native Gameplay proof is not
the sole blocker. M1 is an isolated transition prototype, not yet a complete
production-equivalent authority. Centralizing existing admission semantics does
not inherently require a stronger native proof than production has today.
Replacing those semantics with positive-proof-only admission is a separate
behavioral change. It cannot silently disable supported correction paths.

## Concrete source findings

1. `src/plugin/lifecycle_authority_core.hpp`, DialogueCandidate (465) and
   DialogueTerminated (491): there is no Candidate cancellation/retirement event.
   Terminated accepts Active only. An ordinary Candidate can survive indefinitely;
   GameplaySample may overwrite its receiver/baseline without clearing the
   Candidate. A later DialogueActivated can then activate that old hypothesis.
   Provider Candidate additionally leaves activeArmingId in IsQuarantined, so
   ordinary GameplaySample cannot retire it. The production stationary-candidate
   retirement path is not represented by this public event contract.

2. PrepareEffect (542) resolves ownership but does not validate the supplied
   generation against the current Cinematic generation. Its invalid-baseline
   guard rejects effects for the whole matching scope even when the resolved
   owner is Cinematic or Dialogue. Concrete sequence: admit Gameplay baseline;
   change Gameplay policy (invalidates baseline); accept CinematicEnter with
   compatible aspect; PrepareEffect is rejected due to invalid Gameplay baseline.
   A missing Gameplay reference must select an explicit mode fallback, not
   automatically veto an independently authorized correction owner.

3. Supersession and effect dependencies are incomplete. CinematicEnter,
   SourceReplaced, provider rearming and Dialogue termination do not revoke old
   effect intents. EffectObserved compares the submitted dependency revision
   against the stored intent, not current lifecycle dependencies. Thus an old
   intent/result can still be accepted after a newer Cinematic generation.
   Historical readback can legitimately remain observable, but the interface
   must distinguish that from permission to execute and from current projection.
   No effect execution/consumption interface currently establishes this boundary.

4. Projection maps have no bounded retirement. Each accepted PrepareEffect adds
   a permanent map entry; retired arming records also accumulate. Accept copies
   the entire Projection for every event. This is not suitable for per-frame
   production traffic without a retention contract; no measured runtime cost
   claim is made here.

5. Shadow replay is a selected fixture projection, not numeric differential
   execution of production transactions. The generation-5 test invokes the
   production matching helper, then manually supplies causalArmingJoinProven=true
   to Authority. It checks policy/ownership, while output=153.925/target=149.071
   are printed literals. PID7372 similarly prints target=153.931 without executing
   the production Cinematic selector/mapping. These are useful tests, but cannot
   establish output equivalence or completeness of native adapter binding.

## Native evidence and the caller boundary

The preserved image decodes identify SHA-256
2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479.
They are stored under `02-Research/evidence/post-exit-dataflow-2026-11-11/`
in the workspace. This review inspected those decodes, not a newly decoded EXE.

`writer-caller-state-type.txt` shows 0x53540F6 receives camera RCX and output R8,
preserves them, calls evaluator preparation 0x3236604 and then writer 0x9CD16C.
This preserves receiver identity; it does not choose the camera's Gameplay role.
`derived-vtable.txt` places this function at slot +0x628. The stored direct-call
reference list for the writer does not enumerate incoming virtual dispatches to
that slot. Therefore the upstream activation/selection question has NOT been
exhausted by the prior audit. Absence of a witness there is not established.

There is a concrete second lead already visible in target arming: 0x30DBCD2
calls component virtual +0x198, helper 0x69496A, then 0x30DBDF8; the resulting
camera is weakly stored at S+0x40 and its FOV read at +0x230. 0x30DBDF8 calls
0x255B976 and virtual +0x740 to resolve the camera. This is a native owner-to-
camera relation worth tracing. It is not yet proven to distinguish Gameplay
from Cinematic, which can use that camera too. No new hook follows from it yet.

The generation-5 preserved PID log independently confirms armer at line 1315,
setter at 1317 and applied handoff at 1321 with output 153.925. It confirms the
observed bounded order, not uniqueness of activation after identical rearming.

## Architectural alternatives

- Positive Gameplay binding: strongest native role claim, if selection/owner
  path supplies it. Cost/coverage and lifecycle expiry remain to be established.
- Explicit compatibility admission inside Authority: preserve the supported
  residual writer policy using captured native inputs plus accepted history.
  Name it as a policy decision, not nativeProof. All decisions live in one owner;
  no old coordinator remains as an independent source of truth. This requires
  explicit contract approval because the current migration plan forbids it.
- Separate player Gameplay reference from source-local measured baseline:
  retain configuration/reference identity across cameras only when its own
  provenance supports that continuity. Current FOV field is not automatically
  a user setting, so this cannot justify blindly transferring A's numeric sample
  to B. This option needs additional evidence and is not a ready fix.

Preferred route: preserve the actual behavioral contract in one Authority,
represent admission evidence honestly, and strengthen native bindings where
concrete owner/selection paths justify it. Do not make universal semantic proof
a prerequisite for an ownership refactor, and do not erase known uncertainty.

## Next bounded work

First reconcile the admission contract and close the isolated-core defects with
counterexample sequences. In parallel within the same bounded preparation,
trace the incoming +0x628 selection and +0x740 owner-camera resolution paths.
Then exercise full production selector/mapping outputs for receiver replacement,
Cine1/Dialogue/Cine2 and same-source recovery through an offline adapter.
Compare decisions and numeric results; do not satisfy binding by test booleans.
Only after that review the single-owner adapter cutover. Universal Dialogue END,
global engine episode identity and exhaustive Unreal state proof are not required;
supported terminal/supersession paths and honest unknown behavior are required.
