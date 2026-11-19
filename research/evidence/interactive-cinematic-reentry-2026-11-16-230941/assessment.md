# Interactive cinematic: rapid empty-generation reentry loses GameplayHorPlus

Status: confirmed runtime defect; source cause localized; no production edits.
User scenario: one cinematic with an embedded reply choice, which must remain
under Cinematic policy. FOV decreased after that interaction. Game FOV setting
130 is the user's preferred diagnostic control for further scene tests.

Current evidence: canonical log, PID 3120, start 2026-11-16 23:09:41.
ASI SHA256 55AA5082A3A9B5A4BFB7BFB267B081994061686CC47B89A5DD9770A51DB78DB8.
Game SHA256 2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479.
RuntimeCombine / Cinematic Auto + GameplayHorPlus / Dialogue Reduced / diagnostics ON.
The .PID.log is the older PID 21296 and is excluded from current conclusions.
Pre-load 120 is not used as Gameplay evidence. Post-load admission is 130.311.

## Confirmed causal sequence

- 23:11:16.300: generation 1 selects GameplayNativeNumericBaseline 130.311 -> 153.931.
- 23:11:57.548/.553: internal EXIT/ENTER creates generation 2 with the same correct target.
- 23:12:01.257/.262: EXIT generation 2, ENTER generation 3, same correct target.
- 23:12:01.267: EXIT generation 3 occurs without any writer sample in that generation.
  Association has the aspect target but no admitted writer. recovery_owner_captured=0.
- 23:12:01.272: generation 4 still reports baseline_valid=1 and baseline_usable=1,
  same source 0x16a38baf340 and admission sequence 981. Nonetheless selection switches
  to AuthoredEnter, witness_available=0, target=126.87 instead of 153.931.
- 23:12:01.273: activation capture treats native_start=153.931 as a current native
  sample and supplies corrected_start=166.795 (HorPlus applied again).
- First generation-4 writer at 23:12:01.280 maps 153.931 -> 166.795, then the native
  blend plus mapping decreases toward 126.87. At 23:12:03.067 the output passes
  through 126.87 while CinematicActive.

No confirmed Dialogue ownership/handoff, Dialogue writer context or mutated
Dialogue callback is recorded in this run (all four checks count zero).
Thus the embedded interaction was not corrected with Reduced. Internal native
EXIT/ENTER events are observed even though the user regards the whole sequence
as one cinematic; deleting/ignoring EXIT globally would be an unsafe response.

## Source localization

src/plugin/cinematic_lifecycle_transaction.hpp::ProcessAcceptedCinematicEnter
consumes continuationCarryPending into inheritedEmptyGenerationContinuation and
includes !inheritedEmptyGenerationContinuation in baselineUsable. It passes an
empty baseline to SelectCinematicBaseline despite the store still being usable.
That exact branch explains the reported provenance/witness/target discrepancy.

src/plugin/runtime.cpp::CaptureCurrentNativeEnterStartCandidate checks numeric
nativeStart/receiverFov agreement and source aspect/flags, then EvaluateHorPlus
without proving that the current field is not a previous cinematic-corrected
value. CinematicEnterActivation accepts this current-start candidate. The recorded
153.931 -> 166.795 matches that path. This is an additional start-provenance defect,
not an argument to unconditionally bypass writer correction.

## Existing evidence limits and next bounded regression

Latest periodic summary: 4983 transactions/stores/correlated post-process events,
zero readback mismatch, unavailable readback, correlation failures or queue losses;
pending=0. This confirms output-store observations, not final rendered FOV or
worker shutdown. User supplies the visual reduction report.

Reproduce the exact production lifecycle: admitted Gameplay 130.311 -> owned
Cinematic writer at 153.931 -> EXIT/ENTER -> EXIT/ENTER with zero writer calls
in the intervening generation -> first writer of the successor generation.
Assert both baseline origin/target and the complete writer trajectory, including
absence of fabricated double-HorPlus start. Keep source/settings/generation
conflict controls, repeated empty generations, changed FOV/aspect, true provider
Dialogue and the already validated Dialogue-to-Cinematic path.

No blind removal of continuationCarryPending or numeric equality ownership rule.
Preserve existing GameplayHorPlus modes, baseline admission, native interpolation
and the distinction between confirmed Dialogue and cinematic reply UI. No new
hooks/timers are needed merely to reproduce the recorded production events.
Production fix follows an executable red regression and a reviewed continuity
contract. Heavy builds/game/graphics tests were not started while the user is
continuing scene testing. This report does not claim the defect fixed.
