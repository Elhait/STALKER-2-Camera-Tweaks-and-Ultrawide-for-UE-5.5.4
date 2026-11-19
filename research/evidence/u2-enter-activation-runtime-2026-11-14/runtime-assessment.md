# U2 ENTER activation runtime assessment — 2026-11-14

## Artifact and input

- Runtime identity in log: ASI SHA-256 `B1F2D825A01D61EBCA0C72D9114553E515D5C5D3DE94F9C68CB3D21C2E66EAF0`; game image SHA-256 `2BD3F115B959A8EEE7E5C4B42B0142DD0025BD437E1014F92DFC60B9AFEE5479`.
- Diagnostics enabled. Configuration logged `Gameplay.Mode=RuntimeCombine`, `Cinematics.Mode=Auto`, `Cinematics.FovMode=NativeHorPlus`, `Dialogue.Mode=Reduced`.
- Preserved full log: `STALKER2CameraTweaks.log`, SHA-256 `EB915DF48228F20DC11C9A4E79F13B5F0260B71A7A7AFBA7FE65930837ED47B0`.

## Observed results

1. **Cinematic 1 ENTER writer continuity:** generation 1 captured start `90.3115`, target `126.87`, corrected start `127.119`. Sixty-four admitted writer samples were mapped, ending at `126.87` (`completed-endpoint`, `production_mutation=1`). This confirms the implemented writer mapping ran through the sampled blend. It does not prove rendered-view continuity.
2. **Cinematic 1 → Dialogue seam:** at EXIT generation 1, the later validated provider activation reported raw start/continuity start `120.46`, matched Dialogue setter sample `120.434`, and applied output `120.451` toward Dialogue target `108.941`. The last ENTER mapping endpoint was `126.87`. This is a concrete writer-side handoff domain difference of about `6.42°` between the cinematic endpoint and provider continuity start. It is consistent with the user's observed jump and is the strongest current seam candidate; the exact rendered frame/order remains unobserved.
3. **Dialogue → Cinematic 2:** generation 2 began at native `70`, target `126.87`; corrected start was invalid and repeated samples remained `missing-corrected-start`, `production_mutation=0`. The record confirms fail-closed behavior, not a correction. The historical Gameplay pair `90.3115/127.119` was retained but correctly not used to map from `70`.
4. **Post-recovery Gameplay admission:** after Dialogue recovery, writer context returned to `GameplayEligible` and baseline updated at sequence 2273 to native `90.3071` / HorPlus `127.115`. Thus this runtime did not leave Gameplay baseline admission stuck in `TransitionAmbiguous`.
5. **Additional cinematic after save load:** generation 3 started at native `90.3115`, target `126.87`, but receiver changed from prior baseline source `0x294850b4d80` to `0x2948da1f340`; no current corrected-start provenance existed. All observed writer samples were `missing-corrected-start` with `production_mutation=0` through sample 188 near native endpoint (`126.825`). This matches the reported jump and demonstrates the strict receiver match can reject a numerically similar but identity-distinct current camera. It does not establish whether that new receiver shares the same transform history.

## Assessment and next bounded work

- P0 baseline admission is runtime-confirmed for the exercised recovery: Dialogue/ambiguous samples were retained and a later GameplayEligible sample updated the baseline.
- U2 writer mapping fixes the C1 ENTER blend path, but the user still saw both original jumps. The remaining likely causal path for C1→Dialogue is the `126.87 → 120.451` writer handoff; verify the supported endpoint/continuity contract before changing it.
- C2 and generation 3 remain unmapped by design. Do not loosen receiver identity or reuse historical baseline values. The next implementation needs a current corrected-start provenance source applicable to the activation (Dialogue policy output for C2; current camera's valid transformed state for the new receiver), or should remain fail-closed with telemetry.
- The added cutscene is a distinct runtime case after save load, not evidence that the same camera receiver or activation provenance is shared with the earlier scene.
- Rendered-view acceptance remains unverified by logs; user reported visible jumps at C1→Dialogue, Dialogue→C2, and the additional post-load cinematic ENTER.

## Follow-up implementation and validation — corrected provider start

The production provider handoff now derives its corrected output start by
running `ResolveHorPlusRecoveryInterpolationFov` at the provider activation's
native start, using that same camera source/recovery owner, current aspect and
flags, active EXIT native target, and cached transformed cinematic endpoint.
The mapper then preserves the native provider sample's projection-space
progress from this corrected start to the current Dialogue-policy destination.
The raw receiver field is named `nativeContinuityStart` and remains provenance;
it is no longer treated as a corrected output. Missing/invalid recovery
provenance still fails closed. Diagnostics now prints both native and corrected
continuity starts.

Regression `EXIT-to-provider mapping preserves corrected recovery output at the
native activation start` uses native activation `120.46`, active EXIT recovery
`90.3115 / 126.87`, and verifies the first mapped output equals the corrected
recovery result (about `126.914`), rather than raw `120.46`, while retaining the
Dialogue destination at HorPlus(70).

Validation on the final source: canonical `test.cmd -NonGraphics` configured
and built the current harnesses, passed registry audit and runner exit-code
contract, then stopped at invocation 10/62 because the existing hard-link
fixture returned Windows error 5 (`CreateHardLinkW`, `occupied=1`). Cleanup was
verified. The focused camera writer, Dialogue boundary, cinematic FOV, and
runtime native adapter harnesses passed; the adapter passed 228 fresh
Diagnostics OFF/ON processes. The final Production Release build linked with
exit 0 and verified process-tree cleanup.

Candidate: `../build-artifacts/u2-provider-continuity-fix-2026-11-14/production/STALKER2CameraTweaks.asi`,
3,906,048 bytes, SHA-256
`A439FC24FFD46907A91F5056A0B9AF0BFEA8D629CAADA5F867119E24CDE74BEE`.
Manifest confirms one Production ASI, startup timeline/discovery OFF, and
source/package hashes match. This hash has not been loaded in-game. C2 still
needs a current Dialogue-policy corrected start, and the post-load receiver
change case remains fail-closed. The report does not claim rendered continuity.
