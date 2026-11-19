# Dialogue Follow-up Log Assessment — 2026-11-16

## Capture identity

- Source: `STALKER2CameraTweaks-dialogue-followup.log`.
- SHA-256: `A714ED64B78540328009EC137EC80C5D3EE4D3BA8A120619AFA63B8F9BA0D4EF`.
- Runtime identity reports mod SHA-256 `6AAB96F0BF8263AEF2BA9C819A00DA98C7E463652859DECFC0FFB2089BB4A659`, matching the ordinary Release candidate from this task, and the same game image SHA-256 as the prior capture.
- Startup configuration: Gameplay `RuntimeCombine`, Cinematic `Auto` / `GameplayHorPlus`, Dialogue enabled with `Reduced`, diagnostics enabled.

## What this log establishes

- The intended Dialogue setting was loaded by this ASI at startup.
- Before log saturation, every emitted `CAMERA_STATE_DIAGNOSTIC event=POST_PROCESS` record had `coordinator=Gameplay` and `writerContext=GameplayEligible`. No `DIALOGUE_BOUNDARY_PROVENANCE_ARM`, Dialogue boundary provenance, cinematic writer ownership, or post-EXIT provider handoff records appear in the preserved portion.
- The file ends with `LOG_LIMIT_REACHED`; the logger's configured maximum is 16 MiB. Thus it is unknown whether the user's cinematic/dialogue sequence occurred after that point, and runtime events after saturation could not be recorded.
- The last periodic diagnostic summary before saturation reports 12,454 transactions, 12,454 post-stores, 12,454 correlated post-process readbacks, zero mismatches/unavailable readbacks, four contended event drops, and no pending events. This summary does not cover possible activity after the log limit.
- The capture reports `camera+0x230=130.311°` and `camera+0x234=90°` in a Gameplay source-change observation. This does not establish either field's semantics for the missing Dialogue interval.

## Assessment

The user's visual report is that Dialogue between cinematics still does not use the configured Dialogue behavior. This is accepted as the runtime observation. The log confirms the configuration loaded but cannot locate the transition defect because it contains no Dialogue/Cinematic lifecycle evidence before its 16 MiB limit and suppresses all later records. No production behavior change is justified from this capture alone; the previous setter-interleaving repair remains an offline-tested hypothesis until its runtime handoff is observed.

## Next capture

Use the same ordinary ASI and existing INI Diagnostics setting. Arm the built-in one-shot FOV capture after the game starts and immediately before the first Cinematic ENTER, then perform `Cinematic 1 → Dialogue → Cinematic 2` without reloading the save. The bounded capture is tied to the next accepted Cinematic ENTER, while the normal log should then include the EXIT-armed Dialogue boundary trace. Inspect `DIALOGUE_BOUNDARY_PROVENANCE_ARM`, `POST_EXIT_PROVIDER_DIALOGUE_CANDIDATE`, setter reject/match and writer-context records before making another behavioral change.

## Log saturation repair

The user correctly redirected the task to the capture failure: the log filled before the cinematic, so the missing Dialogue records could not locate the Dialogue policy defect. The cause was repeated full `POST_PROCESS` rows for unchanged Gameplay writer payloads exhausting the existing 16 MiB file cap.

The ordinary ASI now coalesces adjacent post-process records only when every payload field is bit-identical (including receiver, output object, context, settings epochs, decision, FOV values and validity flags). The emitted row retains the full representative payload, count, first event/transaction IDs, and exact last event/transaction IDs. Any changed payload closes the run and is written as a separate row. Runs are flushed at least once per second and before other event types and shutdown. The cap itself and camera/FOV behavior were not changed.

Validation: `camera_writer_identity_harness` PASS for coalescing, exact ranges, payload-change boundaries, settings/context separation and flush. The ordinary Release ASI built and was published as `build-artifacts/release-candidate/STALKER2CameraTweaks.asi` (SHA-256 `A796D33364DA7C036B110FC776A144A7E481B49DF4E8740E25C8DFEDBDE4DF68`). Build and focused-harness process trees were cleaned and verified. This is offline validation only; the next runtime log must confirm the cap is no longer reached before the Cinematic/Dialogue sequence. Dialogue policy behavior remains unresolved and was not changed by this repair.

## Dialogue Reduced root cause and repair — 2026-11-16

The next runtime log uses the ordinary ASI hash `A796D33364DA7C036B110FC776A144A7E481B49DF4E8740E25C8DFEDBDE4DF68`, has the expected `Dialogue.Enabled=true` / `Dialogue.Mode=Reduced` configuration, and is only 1,235,515 bytes; `LOG_LIMIT_REACHED` is absent. Thus log coalescing fixed the capture-capacity problem and exposed the transition.

At Cinematic generation 1 EXIT (`02:29:22.176`), the provider candidate was captured with start `130.311` and target `70`. Recovery completed at `02:29:22.187`. The provider setter sample matched at `02:29:22.286` for the same component/receiver/thread/endpoints. However, the corresponding Dialogue boundary callback was still returned as `ExclusionReset`, `guard_reason=post_cinematic_exclusion_active`, and `selected_policy=Native`; subsequent target-70 callbacks were rejected the same way. The camera writer consequently reached native `70` and the Cinematic 2 activation recorded `corrected_start=108.941`, showing that the Dialogue Reduced boundary transform never took ownership. This is the reproduced cause of the user's report.

The fix lets only the one-shot, identity-validated provider activation pass the post-cinematic Dialogue gate. It seeds Dialogue from the admitted Gameplay baseline and the policy frozen on that provider activation, without admitting the provider's native endpoint to the Gameplay baseline. The resulting Dialogue ownership remains valid only for the exact provider callback source, receiver, lifecycle generation, settings epoch and Gameplay coordinator. Interleaved callback sources are ignored without clearing provider ownership; receiver/lifecycle changes fail closed and reset it. Ordinary unconfirmed callbacks remain blocked by the exclusion.

Focused validation after the final source change:

- `dialogue_boundary_transaction_harness`: PASS, including active-exclusion provider admission, frozen policy, interleaved unrelated callback, receiver replacement reset, and the existing unconfirmed-exclusion rejection.
- `camera_writer_transaction_harness`: PASS.
- `cinematic_lifecycle_transaction_harness`: PASS.
- Ordinary Release ASI build and publish: PASS; SHA-256 `05A303F48493DBA389C1F9EC2D9A6F9FC50CE62B311744B9C08E4F13558C6429`.
- All supervised harness/build/publish process trees reported cleanup verified. The full `test.cmd -NonGraphics` registry was not completed in this turn, so no full-suite claim is made.

Runtime confirmation of the repaired Dialogue value is still pending. The exact ordinary Release artifact is at `build-artifacts/release-candidate/STALKER2CameraTweaks.asi`; no separate diagnostic ASI or INI change is required.

Archived follow-up runtime evidence: `dialogue-policy-before-fix-runtime.log`, 1,235,515 bytes, SHA-256 `36CCE346FE12119FC27B6CC344C01F2AB9DFF01EF0B6E44BD56D67A35DECC126`.
