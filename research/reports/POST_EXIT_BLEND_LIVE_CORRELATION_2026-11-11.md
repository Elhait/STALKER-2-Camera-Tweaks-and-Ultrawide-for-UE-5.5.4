# Post-EXIT blend: supplied live correlation

Date: 2026-11-11. Read-only analysis; no source changes, build, tests, game/debugger launch or installation. The runtime record reproduces the previously identified writer-output discontinuities. This analysis does not independently establish their appearance on screen.

## Identity and preserved evidence

- ASI SHA-256: `9DAF593D9B852EA21307A08BBDC4E6FBF71C9F6938C1158160C7C1C3BD3C277E` — exactly the new diagnostic candidate.
- Game SHA-256: `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479` — matches the independently decoded EXE.
- Configuration: RuntimeCombine, Cinematics Auto/GameplayHorPlus, Dialogue Reduced, Diagnostics enabled, aspect 3.55556.
- [Snapshot](../evidence/post-exit-blend-live-2026-11-11-231304/STALKER2CameraTweaks.log): 332,742 bytes, 1,076 lines; SHA-256 `7C95D76E4F051C702C6563F50C7AF52C0DEA56C1998D88B3F28220A195237092`.
- [Parsed observation records](../evidence/post-exit-blend-live-2026-11-11-231304/records.json).
- [Regression input with source-line provenance](../evidence/post-exit-blend-live-2026-11-11-231304/regression-fixture.json) and [integrity result](../evidence/post-exit-blend-live-2026-11-11-231304/fixture-integrity-result.json). This is observed input, not an approved future-output oracle.
- `POST_EXIT_BLEND_CAPABILITY layout=validated` at snapshot line 17.

## 1. The problematic stream now has a receiver/value correlation

Second generation:

- ENTER 23:16:31.098 selects baseline 70 and cinematic output 108.941 (line 756).
- Cinematic summary: all 1,417 observed writer samples have input 70 and result 108.941, all corrected; no output transitions (line 782).
- EXIT 23:16:49.089 has native target 90.3115.
- First post-EXIT writer, 23:16:49.099: input/output 70, `RECOVERY_SAMPLE_UNCLASSIFIED` (line 789).
- Blend context T `0x2B36B41B948` has start 70, target 90.3115, receiver camera `0x2B13E9240C0`, thread 22972. Eight sampled blend records span 23:16:49.100–23:16:50.085.

Seven sampled same-camera/thread writer correlations establish the following equalities at their respective observation boundaries:

| Writer time | Raw XMM6 = raw XMM1 = final setter argument = writer-time field = writer input | Writer callback output |
|---|---:|---:|
| 23:16:49.114 | 70.0071 | 70.0071 |
| 23:16:49.125 | 70.0323 | 70.0323 |
| 23:16:49.151 | 70.1416 | 70.1416 |
| 23:16:49.198 | 70.4519 | 70.4519 |
| 23:16:49.303 | 71.7608 | 71.7608 |
| 23:16:49.511 | 77.0988 | 77.0988 |
| 23:16:49.941 | 89.3775 | 89.3775 |

At 23:16:50.084, normal HorPlus resumes: `90.3084 -> 127.116` (line 975). At 23:16:50.085 the native blend sample reaches exact target 90.3115, with remaining time -0.00440266 (line 987). Mod recovery completion therefore precedes the logged exact native target; it is not evidence of native cleanup.

The immediately preceding logged RuntimeCombine output is 90.293 at 23:16:50.071 (line 973). The adjacent logged outputs therefore differ by 36.823 degrees over 13 ms. The 89.3775 row above is an earlier sparse control sample, not the last pre-handoff output. Neither relation independently measures rendered pixels or excludes unlogged native calls.

This is more than target/time similarity: current receiver identity, same thread, pending sample serials and seven equal value chains are recorded. It strongly supports this component as the observed stream's producer. Exclusive write provenance, successful return from each setter and which caller armed the component remain unobserved; the diagnostic's `unknown` labels are not overridden.

## 2. The receiver selects the base setter, not the derived override

The logged native consumer address and independently decoded consumer RVA `0x6AFFF7C` give runtime image base `0x7FF717840000`. All sampled receiver vtables are `0x7FF71FE988A0`, hence RVA `0x86588A0`.

The previous [current-image data-flow analysis](POST_EXIT_NATIVE_BLEND_DATAFLOW_2026-11-11.md) establishes this table through its constructor assignment. Its slot +0x608 selects RVA `0x1F88EDC`:

```text
MOVSS [RCX+0x230], XMM1
RET
```

Thus the virtual setter selected by these logged calls is the base direct-write setter. The derived optical-parameter override is not selected by this receiver vtable. This removes that particular alternative for these calls; it does not exclude independent writes or other direct-call paths elsewhere.

## 3. Important additional finding: overlapping native blends

After the first EXIT, two distinct tick contexts operate on the same receiver/thread in an overlapping interval:

| Context T | Start | Target | Sampled interval |
|---|---:|---:|---|
| `0x2B10CB59728` | 90.3115 | 90.3115 | 23:16:14.904–23:16:15.721 |
| `0x2B3570F42C8` | 90.3115 | 70 | 23:16:14.983–23:16:15.971 |

The constant-target context sees a changing receiver field before its setter call:

- 23:16:15.009: field 90.305, its own output 90.3115;
- 23:16:15.111: field 89.615, its own output 90.3115;
- 23:16:15.305: field 86.0053, its own output 90.3115;
- 23:16:15.721: field 72.7277, its own output 90.3115.

The descending context's sampled arguments then match subsequent writer fields/inputs, with HorPlus applied by normal Gameplay. These are interleaved native updates on one camera, not two threads racing in the recorded samples and not evidence that mod Dialogue correction is fighting Cinematics.

No semantic weapon/dialogue name is assigned to either native context. The second context's target 70 is evidence, not proof of why the game chose it. The constant context's target matching EXIT is not sufficient to prove its arming caller.

Architectural consequence: a single camera address, EXIT generation or equal target cannot establish an exclusive active blend. A future observer/contract must account for multiple contexts and the particular setter invocation that precedes a writer, cancellation/supersession, and unobserved intervening writes. Do not pin the first observed blend as the sole owner.

## 4. What is established about FOV space

For the sampled problematic transition, raw blend output reaches the setter argument and writer input numerically unchanged; mod Dialogue does not transform those samples. All 24 emitted writer-correlations across the log report `input_minus_setter=0` at logged precision. Dialogue has candidate capture/cancellation records, but no accepted/active correction in this capture.

Together with the cinematic writer's persistent input 70/output 108.941 and the blend's stored start 70, this supports the native-start hypothesis for this particular stream. It does not discover a universal native-space tag or prove that an arbitrary future captured camera start is unmodified. Other mods, older transformed-start scenarios and unobserved earlier writes are not globally excluded.

The old `106.688 -> 90` fixture still has no instance/receiver/progress capture in this current-game session. Its assumed equivalence to the new native blend mechanism remains a hypothesis. This log provides two additional current-image scenarios (constant target and descending to 70), not a runtime validation of that fixture.

## 5. Diagnostic cost and remaining acceptance gaps

- 58 new-marker records total, approximately 23,078 bytes including CRLF; 30 blend samples, 24 writer correlations, two arm records, one summary and one startup capability record.
- All sampled field masks are 31: the five requested reads succeeded.
- Generation-one summary reports 1,425 observed callbacks, zero pair rejections and 27 session records at that point. The global 192-record budget was not exhausted.
- Whole log size is about 325 KiB. Its full size includes pre-existing camera/Dialogue diagnostics; it must not be attributed solely to the new observer.
- Last record is a Camera State snapshot at 23:16:59.714, not a shutdown confirmation. Absence of a final summary does not prove a crash or failed cleanup.

Remaining unknowns: actual component arming caller/EXIT link, provider/easing state, direct post-setter readback, native completion callback for the sampled component, and universal initial-FOV provenance. The same-camera/value link is now observed; those other questions remain separate.

## Next bounded decision

Keep production recovery unchanged. Use this capture to refine the transition contract before another artifact:

1. Preserve per-instance/per-setter evidence and support overlapping contexts on one receiver; do not assume one owner from camera/generation alone.
2. Decide whether the desired output follows transformed native samples or maps confirmed native eased progress between rendered endpoints. Those curves are not generally equivalent. The old transformed-start case must remain a regression input.
3. Establish the arming/progress and start-space evidence actually needed by the chosen contract, without requiring an unrelated global camera-mode reconstruction or using numeric FOV equality as causal proof.

No repair is claimed from the successful diagnostic capture.

The follow-up [recovery contract proposal](../plans/POST_EXIT_RECOVERY_CONTRACT_PROPOSAL_2026-11-11.md) separates native blend completion from mod handoff, compares the two mathematical alternatives and records their conditional evidence requirements. It does not supersede production invariants or select a new output curve.
