# F2 Cinematic Continuation — Final Semantic Evidence Review

**Date:** 2026-10-02
**Disposition:** Persistent trace investigation complete for the available natural runtime evidence. No production policy change is made or authorized by this report.

## Evidence provenance and scope

Reviewed the user-provided runtime log at:

`E:\Steam\steamapps\common\S.T.A.L.K.E.R. 2 Heart of Chornobyl\Stalker2\Binaries\Win64\STALKER2CameraTweaks.log`

SHA-256: `B2C39729BD8721F9B994BBB2FABA1A07AF523EF35928674BC6238217F2775334` (159,569 bytes; last write 2026-10-02 16:17:06 UTC).

The user identified the run as experimental build B with out-of-line F2 trace storage. The log itself does not contain a cryptographic ASI identity; build attribution is user-provided. Scenario: a cinematic with dialogue, immediately followed by another cinematic which transitions to dialogue that remains active until exit. This is one natural capture, not a sample of all game cinematics.

## Complete trace-block chronology

Three `CINEMATIC_CONTINUATION_TRACE` blocks are present. Each reports `overflow=0` and `clear_reason=cinematic_exit`.

| Generation | Samples | Source and raw trajectory | Actions / terminal evidence |
|---|---:|---|---|
| 2 | 36 | All 36 samples match expected RSI `0x221f4956700`; raw rises from `90.6557` to `125.892`. Direction is 35 `up` after the first `unknown`; no reversal, target-region entry, crossing or overshoot marker. Last raw is 1.5008° below target `127.393`, outside the ±1° region. | 33 `RetainContinuationTarget`, 3 `RetainTransformedTarget`; trace ends at cinematic EXIT before target convergence. No post-convergence request is tested. |
| 3 | 0 | No writer sample is recorded before the lifecycle ends. | Empty trace emitted at EXIT; no interpolation/completion evidence. |
| 4 | 223 | All samples match expected RSI `0x221f4956700`; no source mismatch. Raw starts at `126.316`, rises toward `127.393`, then remains exactly `127.393` through sample 223. | All 223 outputs are `127.393` with `RetainContinuationTarget`; no overflow. The first sample is in production range but 1.07623° below target, outside the semantic target region. |

Generation 4 is preceded by accepted ENTER generation 4 and an association set where writer RSI equals the aspect-store `targetObject`. Its first post-ENTER writer is sequence 2807, raw `126.316`, output `127.393`. The trace records the same expected/source RSI for all 223 samples. It does not record RBX per sample, so stable RBX across the complete trace is not established here.

## Generation 4 quantitative result

- First ±1° target-region entry: sample 10, raw `126.403`, target distance `0.990181°`.
- Samples 10–32 (23 samples inclusive) remain in the region and continue moving upward. Region entry alone therefore did not mark the end of this observed interpolation.
- Last pre-target sample: sample 32, raw `127.391`, distance `0.002°`.
- First exact target input: sample 33, raw `127.393`. Samples 33–223 are 191 consecutive exact-target inputs, including 190 later invocations after the first exact sample.
- Direction counts: 32 `up`, 190 `flat`, and the initial sample `unknown`. No reversal, target-region exit, or overshoot event is recorded; raw never exceeds the target.
- The `target_crossing` event at sample 33 is emitted for comparison-side transition `below → equal`. It is an arrival at equality, not evidence of strict overshoot beyond target.
- Maximum target distance after first region entry: `0.990181°`; after first exact target: `0°`. Output remains `127.393` throughout.
- Samples have no individual timestamp/QPC. This proves a 191-invocation plateau, not its elapsed duration. The first post-ENTER sample is timestamped 19:14:36.720 and EXIT/trace flush at 19:14:42.182, but those timestamps do not locate each plateau invocation in time.

The observed raw sequence is a monotonic approach followed by an exact flat plateau. For this generation, raw writer FOV reached target and remained there while production continued retaining the target. This is the clearest observable completion-shaped boundary in the capture, but it does not establish that every cinematic producer behaves the same way or exclude a later legitimate same-source request.

## Adversarial reading of candidate completion boundaries

- **First entry into ±1°:** contradicted as a completion point for this trace: 23 in-region samples continue moving toward target after entry. Disarming at region entry would release the remainder of the observed ramp.
- **Exact target arrival after directed approach:** the narrowest candidate supported by this trace. At sample 33 raw reaches equality from below, then remains there. A separate production-design task may evaluate disarming continuation after retaining that arrival sample, allowing a later in-range same-source input to pass through. This remains a candidate, not an adopted or generally proven rule.
- **Stable plateau:** 191 exact samples strongly corroborate settling retrospectively. But the trace exposes no producer completion event; making a forward-looking rule from “repeated” samples would require an arbitrary count or dwell-time threshold. This report does not endorse such a threshold.
- Overshoot, post-target departure, non-monotonic return, and a later legitimate same-source request (for example, 110 before EXIT) are not observed. The evidence cannot distinguish such a request from a resumed producer using only current source/FOV/lifecycle signals, nor establish how often those sequences occur.

## Post-EXIT recovery is separate

Generation 4's continuation trace ends at cinematic EXIT. The log then separately records `CinematicExiting` recovery on the same gameplay source: raw input descends from the transformed region (first listed recovery sample `127.393`, then `126.309`, `124.638`, onward through `91.5934`). The existing validated native-recovery path clears post-cinematic Dialogue exclusion with `reason=native-recovery-converged`, resumes Gameplay, and observes native `90.6557`.

This supports the observed recovery sequence for this run. It is not continuation-completion evidence: it occurs after lifecycle/ownership EXIT and follows the separate recovery contract.

## Evidence matrix

| Classification | Result |
|---|---|
| **ESTABLISHED BY F2** | In generation 4, one associated RSI supplied a monotonic raw approach from `126.316` to exact target `127.393`, then 191 exact target samples through EXIT. Production retained the target throughout. No overflow, source mismatch, reversal, region exit, or overshoot was recorded. Generation 2 shows a monotonic partial approach ending at EXIT before target-region entry. Generation 3 contains no samples. Separate post-EXIT recovery converged to native Gameplay value in this run. |
| **SUPPORTED BUT NOT PROVEN GENERALLY** | For generation 4, exact target arrival after directed approach is a useful candidate boundary; the subsequent plateau corroborates settling. It is more informative than ±1° region entry alone. |
| **NOT OBSERVED** | Target-region exit, raw overshoot, direction reversal, post-arrival movement away, source mismatch, or a legitimate later same-source request before EXIT in these trace blocks. |
| **UNKNOWN** | Whether exact arrival/plateau is a semantic completion event across other cinematic shapes; whether a later legitimate request can occur on the same source before EXIT; a universally safe online completion predicate; and whether/when production continuation should disarm. |

## Diagnostic-build startup incident

The prior differential audit reports that inline F2 trace storage increased `RuntimeState` from 3,552 B to 36,512 B. Experimental build A crashed on startup on two reported attempts; the reported fault was at module RVA `+0xF6A9C`, with a write into `.pdata` at module offset `+0x1C1870`. Out-of-line experimental build B reports `RuntimeState` 3,560 B; the user reports one cold launch without startup crash, followed by the full natural trace above. This makes B a practical working candidate that preserved useful trace behavior in the observed run.

This A/B does not isolate whether the crash arose from inline size/layout, trace-object lifetime/initialization, or another binary-layout interaction. The exact low-level mechanism remains **UNKNOWN**. Trace logic itself is not a confirmed cause. C and D controls were not run and are not required for this semantic review or the user's selected evidence set.

## Closure verdict

One natural continuation approached target, entered the ±1° region while still moving, reached exact target, then remained exactly there for 190 subsequent invocations while continuation stayed active. We did not learn whether this behavior is universal or whether a later legitimate same-source request can arrive before EXIT.

The best-fitting candidate model for a separate production-design review is **directed approach → exact target arrival → retain that arrival sample, then disarm**, with the observed plateau as corroboration—not a dwell/count heuristic. The evidence is sufficient to open that bounded design task, but not to claim a universal completion rule or implement one without that review. No production code, continuation semantics, lifecycle/source/range gates, or mutation policy changed here. No additional F2 runtime capture is requested.

## Production normalization checkpoint — B out-of-line storage (2026-10-02)

F2 research is closed. Before F3, the B out-of-line storage implementation was canonicalized into the ordinary production source/build path. `RuntimeState` now owns a process-lifetime `std::unique_ptr<CinematicContinuationTrace>`; the fixed-capacity 512-sample trace storage is allocated separately rather than embedded in `RuntimeState`. Temporary A/B/C/D build selectors and conditional branches were removed. Diagnostic Mode still gates detailed sample collection and trace-block emission; all current sample fields/events, repeated-identical-sample capture, explicit overflow, and resolver/output isolation remain. This is storage/build normalization only: **no continuation or camera production policy, completion semantics, lifecycle/source/range gate, FOV output, or mutation behavior was changed.**

The production build's MSVC layout report gives `RuntimeState sizeof=3560`, `alignof=8`; the F2 trace buffer is out-of-line. Offline validation passed: 48/48 harnesses, localization/resource audit across 18 locales, `git diff --check`, and ordinary production build. Required `CINEMATIC_CONTINUATION_TRACE_BEGIN`, `..._SAMPLE`, `..._END`, and event strings were found in the artifact; temporary variant selector strings were absent.

Canonical root artifact, produced by ordinary `build.cmd`: `E:\Work\Slaker2 mods\01-Projects\STALKER-2-Ultrawide-Fix-for-UE-5.5.4\STALKER2CameraTweaks.asi` — SHA-256 `C36DBD812E3A2C917F1088A898082165BA3E6C806C34249F066F9BC16660B5F4`, 2,784,256 bytes. The prior root binary (`564CE70F…`) was preserved at `build-artifacts\pre-f2-canonical-root-backup\STALKER2CameraTweaks-before-canonical-build-564CE70F.asi`. No game was launched. Experimental evidence remains bounded: inline A crashed in 2/2 reported attempts; B had one reported successful cold launch and the natural capture in this report; C/D were not run. The exact startup-crash mechanism remains **UNKNOWN**; B is selected as the canonical lower-footprint implementation, not claimed as a proven root-cause repair. F2 research and this normalization step are complete; F3 may proceed as a separate task. No universal completion rule is inferred or adopted.
