# Camera State semantics — observed runtime validation

Observed run: PID 26108, 2026-11-16 21:38:55–21:44:56 local log timestamps.
ASI SHA-256: 55AA5082A3A9B5A4BFB7BFB267B081994061686CC47B89A5DD9770A51DB78DB8.
Game SHA-256: 2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479.
Configuration: RuntimeCombine, Cinematic Auto/GameplayHorPlus, Dialogue Reduced,
Diagnostics ON; viewport 5120x1440 / 32:9. User reports ideal visuals and overlay.

The canonical logs contain this new run. The fixed .PID.log files still contain
PID 21296 / older ASI 2E93B844..., 19:39–19:43; retained only as provenance and
excluded from current-run counts. Startup and Overlay canonical logs both identify
PID 26108 and confirm renderer activation / first committed frame / input activation.
The later no-eligible-window line alone does not overturn the observed working overlay.

## Five states (user screenshot order, stable values corroborated in current log)

| State | Original boundary capture shown in UI | Writer input | Planned / readback |
| --- | --- | --- | --- |
| Cinematic 1 | authored ENTER 90, Historical | 130.311 | 153.931 |
| Dialogue between cinematics | setter input 70, Historical | 122.087 | 149.071 |
| Cinematic 2 | authored ENTER 90, Historical | 153.931 | 153.931 |
| Gameplay after cinematic | captured 130.31, Historical in screenshot | 130.311 | 153.931 |
| Standalone Dialogue | setter input 70, Historical | 121.882 | 148.946 |

These are sampled stable states, not a claim that transitions remain constant.
Captured ENTER 90 is a boundary reference, not per-frame native camera FOV.
Cinematic 2 correctly keeps prepared 153.931 in Writer Input rather than publishing
it as a Current pristine Native value. The two Dialogue outputs are independently
observed values for their own admissions, not evidence that both must be numerically
identical. No interpolation or inverse transform was used to invent an original FOV.

At both ENTERs (21:40:47.850 and 21:42:33.376), baseline_valid=1,
baseline_usable=1, native reference=130.311, HorPlus=153.931, same admitted source
0x1c5de279a00 and publication sequence 2278. Both selectors choose
GameplayNativeNumericBaseline. The confirmed provider handoff at 21:41:59.904
chooses Reduced and dialogue_target=149.071. Cinematic 2 therefore preserves the
same authoritative Gameplay reference through that Dialogue.

## Readback and diagnostic integrity

Latest periodic summary: 9520 transactions, 9520 postStores and 9520
postProcessCorrelated; zero identity mismatches, unavailable readbacks, readback
mismatches, queue-full drops, contended drops or events without a worker. Pending=0.
The file contains 631 coalesced POST_PROCESS records representing exactly 9520
transactions; the summary reports 8889 coalesced transactions. These are combined
records, not lost events. All 631 records retain matched store status, exact correlated
helper site, and equal planned/actual values; helper input/output is unchanged.
AcceptedEvents=38080 (4 stages x 9520). This remains a periodic summary; no final
worker-shutdown/join claim is inferred from pending=0.

## Freshness on screenshots 4 and 5

Historical is real production projection, not missing numeric data. After recovery,
the writer frequently carries TransitionAmbiguous; the ordinary Dialogue carries
DialogueLifecycleObserved. GameplayBaselineStore::Project explicitly retires currency
for these unconfirmed same-source contexts, while retaining the number and continuing
independent writer correction. ConfirmedProviderDialogueLifecycleObserved has a separate
preservation contract and was observed between the two cinematics. The UI must not
convert the ordinary/ambiguous contexts to Current simply because coordinator=Gameplay
or correction is active. The log also records real new GameplayEligible admissions
(e.g. sequence 5786 -> 130.265, sequence 6238 -> 130.307), separate from these contexts.
No new publication defect is established by the Historical labels in this capture.
This validation does not authorize changing baseline trust or Dialogue admission.

## Disposition

Camera State field separation, original-boundary naming, retained inter-cinematic
Gameplay reference and matching Effective output are confirmed in this observed
combined run plus the user's standalone Dialogue control. User visual report confirms
smooth behavior for this run. Diagnostics OFF remains covered offline, not exercised
in this game run. No global writer coverage, rendered projection measurement, native
receiver epoch or frame-time claim is added. No new game run or production change is
needed for this validation. No ASI was built, installed or replaced in this analysis.
