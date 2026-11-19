# Multi-save Cinematic target fallback — 2026-11-17

Status: runtime/source analysis; no production change or new validation run.
User observed one cinematic narrowing to framing equivalent to game FOV 90.
Follow-up: user confirms the failing scene belonged to the second loaded save
in this process. The load itself is user evidence; the log exposes the receiver
replacement. Broader audit: research/reports/RUNTIME_CAMERA_CONTEXT_ISOLATION_AUDIT_2026-11-17.md.

## Evidence identity

All six supplied files preserved alongside manifest.json. Canonical logs belong
to PID 24512 (startup-only in this capture). The actual camera session is PID
13472 in the literal .PID.log files, starting 01:25:36.288. Both report mod
SHA-256 7CEC2406B080A67C96D12F9214D68D1380BA95613829F295F4ED9E3F5ECE738A
and game SHA-256
2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479.
Selections: RuntimeCombine, GameplayHorPlus, Reduced, Diagnostics ON, 32:9.
The initial 120 sample is not treated as proof of loaded Gameplay FOV.

## Confirmed causal chain

- Generations 1–5 select GameplayNativeNumericBaseline, native 130.311 and
  target 153.931. Generations 3 and 4 include repeated short reentry; generation
  4 retains the intended reference after the writer-empty generation 3.
- After the real provider Dialogue recovery, owner returns to Gameplay at
  01:31:00.825. At .827 an ordinary callback creates a new unconfirmed Candidate
  with incoming/native target 130.311, callback source 0x22c9fdf0c08.
- At .841 Candidate observation remains pending. There is no later setter
  callback for this candidate before the next cinematic activation. The source
  trace explicitly reports dialoguePhase=Candidate on subsequent Gameplay writes.
- At 01:31:22.508 receiver/output change from
  0x22c91546700 / 0x22b91003690 to 0x22d268dd9c0 / 0x22b8bc30360;
  writer identity epoch changes 1 to 2. The old baseline is not eligible for reuse.
- New receiver input is 120 initially and 130.311 by 01:31:37.346. RuntimeCombine
  produces 153.931 from the latter. However, writerContext stays
  TransitionAmbiguous, so no fresh Gameplay baseline is admitted for this source.
- At 01:31:48.927 generation 6 ENTER has baseline_valid=1 but baseline_usable=0;
  stored source remains the old receiver, stored native=130.308. The selector
  chooses AuthoredEnter 90 -> 126.87 rather than a Gameplay-derived target.
- Activation mapping starts from a correctly computed 153.931 and follows the
  selected 126.87 target. Generation 6 summary reports first_input=130.311,
  result_max=153.931 and last_result=result_min=126.87. POST_PROCESS records
  confirm actualAfter=126.87, MatchedAtCorrelatedSite, helper inactive.
- Generations 7–9 inherit the already reduced target through the same authored
  fallback. Thus there are four affected technical generations, not necessarily
  four distinct user-visible scenes.

## Current-source mechanism

src/plugin/dialogue_boundary_transaction.hpp::ClassifyDialogueWriterContext
(line 190 onward) makes every non-Inactive phase ambiguous unless it can prove
active Dialogue ownership. This includes an unconfirmed pending Candidate and
even a writer whose receiver differs from the candidate's receiver.

Candidate progression/cancellation in ProcessDialogueBoundary depends on further
setter callbacks. No further callback means the Candidate can remain pending
while primary Gameplay writers continue on a new receiver. The writer does not
retire this pending candidate through this path.

src/camera/gameplay_baseline.cpp::GameplayBaselineStore::Project correctly
rejects a foreign baseline and also makes a same-source ambiguous baseline
non-current. IsEligibleObservation rejects TransitionAmbiguous. Combined with
the sticky Candidate, this prevents subsequent valid-source baseline admission.
The Cinematic selector then executes its existing authored fallback correctly
for the data it was given. The numerical HorPlus formula is not the defect.

## Repair boundary / next regression

The missing contract is pending Dialogue candidate lifetime across continued
Gameplay writer activity and primary receiver replacement. Do not fix this by
reusing the old receiver's baseline, by treating any source mismatch as confirmed
Gameplay, or by weakening active/unknown Dialogue ownership checks.

An integration regression should reproduce: provider Dialogue recovery -> extra
stable callback creates Candidate -> no more setter callbacks -> receiver A/B
replacement -> actual writer 130.311 -> Cinematic ENTER. It must prove legitimate
new Gameplay admission before expecting 153.931, and preserve rejection of old
receiver baseline. Controls must retain true pending/active Dialogue protection,
settings/generation conflicts, real provider handoff and embedded cinematic
choice semantics. A scoped trust contract is needed before production edits;
the previous writer-empty-generation contract alone does not resolve this case.

## Limits

15,609 writer transactions/post-stores/post-helper correlations; final periodic
summary has no readback mismatch, queue drop, correlation mismatch or pending
events. This is not a final worker-shutdown record. No universal coverage or
rendered FOV claim is inferred. No performance measurement or extra process was
started. The new evidence confirms the user-visible narrowing numerically but
does not itself prove a safe new admission rule.
