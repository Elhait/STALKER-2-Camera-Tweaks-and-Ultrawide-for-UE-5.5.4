# Runtime decomposition — exact move ledger (Batch 0)

Source: src/plugin/runtime.cpp; SHA-256 D4E124A5A1871CE80F736BAF4AD221E2F4AB24931CA0B2DC9BD761E74F00B341. Generated from the audited indices and hash-checked current source. Destinations are proposals, not authorization for B1–B7. Line numbers are baseline-specific.

All RuntimeState fields retain one resident g_runtime owner. Relocating its definition privately in B1 does not relocate ownership or introduce storage. Guards are source conditions, including dormant unsupported profiles; these profiles are not enabled for V0.

## Functions/templates (one entry per audited definition)

| Symbol | Line | Destination | Batch | State aliases referenced in body | Guard / contract |
|---|---:|---|---|---|---|
| GetCinematicContinuationTrace | 357 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_runtime | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| WaitForWorkerStop | 563 | Remain: runtime.cpp | No move in B1-B6 | g_workers | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| StartWorker | 568 | Remain: runtime.cpp | No move in B1-B6 | g_workers | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| StopWorkers | 578 | Remain: runtime.cpp | No move in B1-B6 | g_workers | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| CreateWorkerStopEvent | 587 | Remain: runtime.cpp | No move in B1-B6 | g_workers | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| GetDialogueSourceProvenanceTrace | 592 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch |  | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| ReadDialogueWriterSnapshot | 602 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_cinematicWriterAssociation | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| DialogueSourceStageName | 609 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch |  | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| EmitDialogueSourceEvent | 621 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch |  | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| RecordDialogueSourceCallback | 644 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_coordinator | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| RecordDialogueRecoverySource | 662 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_coordinator | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| RecordDialogueSourceEnd | 673 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_coordinator | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| GetDialogueBoundaryStateView | 686 | Remain: runtime.cpp | B4 complete; approved Remain (2026-10-09) | g_activeDialoguePolicy, g_activeDialogueSettingsEpoch, g_coordinator, g_dialogueBaseline, g_dialogueCandidate, g_dialogueCorrectionActivation, g_dialogueExitIncomingStart, g_dialogueMutex, g_dialoguePhase, g_dialoguePrevious, g_dialogueRecoveryRearm, g_dialogueSettingsEpoch, g_postCinematicDialogueExclusion, g_runtime, g_runtimeDialogueEnabled, g_runtimeDialoguePolicy | References the single RuntimeState; exposing it across TUs would enlarge mutable owner API |
| ResetDialogueRuntimeState | 697 | Remain: runtime.cpp | B4 complete; approved Remain (2026-10-09) | g_activeDialoguePolicy | Caller-owned gate/dialogueMutex and diagnostic-before-reset order must be preserved |
| ResetPostCinematicDialogueExclusion | 705 | Remain: runtime.cpp | B4 complete; approved Remain (2026-10-09) | g_postCinematicDialogueExclusion | Owner-bound lifecycle wrapper |
| ArmPostCinematicDialogueExclusion | 711 | Remain: runtime.cpp | B4 complete; approved Remain (2026-10-09) | g_exitTargetFov, g_postCinematicDialogueExclusion | Combines resident EXIT target and lifecycle operation |
| ObservePostCinematicDialogueRecovery | 718 | Remain: runtime.cpp | B4 complete; approved Remain (2026-10-09) | g_postCinematicDialogueExclusion | Callback input/lifecycle/log ordering |
| ResetAllRuntimeResources | 734 | Remain: runtime.cpp | No move in B1-B6 | g_cameraWriterHookGate, g_cinematicAspectComponentAvailable, g_cinematicEnterHook, g_cinematicExitHook, g_cinematicHookGate, g_cinematicLifecycleObservationAvailable, g_cinematicRecoveryOwner, g_cinematicSelectionValid, g_dialogueBoundaryHook, g_dialogueBoundaryHookAvailable, g_gameplayRecoveryObservationAvailable, g_hook, g_hotkeyRebindCaptureActive, g_hotkeyRebindConsumedKey, g_postExitConsumerHook, g_postExitGameplayObserverHook, g_postExitProducerStoreHook, g_postExitWriteOwnerHook, g_runtimeInitializationComplete, g_stopping, g_zoomInHits, g_zoomInHook, g_zoomLoggedEdges, g_zoomOutHits, g_zoomOutHook | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| IsValidAspect | 822 | Remain: private runtime utility; shared private header only if required | B1/B6 reviewed |  | Always; No new owner; identical native checks/exception boundary |
| TraceGameplayAspectRestorationState | 827 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch |  | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| UpdateGameplayAspectRestorationState | 839 | Remain: runtime.cpp | No move in B1-B6 | g_gameplayAspectRestorationStore | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| InvalidateGameplayAspectRestorationState | 847 | Remain: runtime.cpp | No move in B1-B6 | g_gameplayAspectRestorationStore | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| LogImmediate | 855 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_logger | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| Log | 869 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_cameraCorrectionGate | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| OutputCameraLogProbeDebug | 889 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_logger | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| FlushCameraLogProbe | 910 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_logger | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| DeferCameraLogProbeFlush | 938 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_cameraCorrectionGate | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| CinematicDiagnosticActionName | 997 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch |  | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| LogCompletedCinematicContinuationTrace | 1013 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch |  | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| FlushCinematicContinuationTrace | 1054 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch |  | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| InitialEstablishmentEndReasonName | 1063 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch |  | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| DiagnosticFovChanged | 1078 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch |  | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| LogCinematicDiagnosticEnter | 1085 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_cinematicDiagnosticMutex, g_lastCinematicDiagnosticAspect, g_lastCinematicDiagnosticAspectGeneration, g_lastCinematicDiagnosticAspectTarget, g_lastCinematicDiagnosticAspectValid, g_lastCinematicDiagnosticFlags, g_lastCinematicDiagnosticFlagsValid, g_lastCinematicDiagnosticWriterState | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| LogCinematicDiagnosticAspectStore | 1106 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_cinematicDiagnosticMutex, g_lastCinematicDiagnosticAspect, g_lastCinematicDiagnosticAspectGeneration, g_lastCinematicDiagnosticAspectTarget, g_lastCinematicDiagnosticAspectValid, g_lastCinematicDiagnosticFlags, g_lastCinematicDiagnosticFlagsValid | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| LogCinematicDiagnosticAssociationTransition | 1139 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch |  | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| LogCinematicDiagnosticWriterState | 1161 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_cinematicDiagnosticMutex, g_lastCinematicDiagnosticWriterState | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| LogCinematicDiagnosticExit | 1243 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_coordinator | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| GetCinematicBaselineWriterPublicationState | 1269 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch |  | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| ReadCinematicBaselineWriterPublication | 1277 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch |  | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| CaptureCinematicBaselineWriterPublication | 1285 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_cinematicWriterAssociation | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| CinematicBaselineOriginName | 1306 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch |  | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| LogCinematicBaselineProvenance | 1318 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch |  | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| LogCinematicBaselineSelection | 1365 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch |  | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| SafeMidHookEntry | 1381 | Remain: private runtime utility; shared private header only if required | B1/B6 reviewed |  | Always; No new owner; identical native checks/exception boundary |
| NowSteadyNs | 1392 | Remain: runtime.cpp | No move in B1-B6 |  | #ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| CancelDeferredGameplayReplay | 1398 | Remain: runtime.cpp | No move in B1-B6 | g_deferredGameplayLastFov, g_deferredGameplayReplayPending, g_deferredGameplaySource, g_deferredGameplayStableSamples, g_deferredGameplayStableSinceNs | #ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| ArmDeferredGameplayReplay | 1416 | Remain: runtime.cpp | No move in B1-B6 | g_deferredGameplayLastFov, g_deferredGameplayReplayPending, g_deferredGameplaySource, g_deferredGameplayStableSamples, g_deferredGameplayStableSinceNs | #ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| ResetAtomicExitHandoff | 1429 | runtime_camera_callbacks.cpp | B6 | g_atomicExitHandoffPending, g_atomicExitHandoffPreviousFov, g_atomicExitHandoffSource | Always; Same owner/gate; preserve whole writer/recovery dispatch |
| CameraWriterRecoveryStateView | 1439 | runtime_camera_callbacks.cpp | B6 | g_atomicExitHandoffPending, g_atomicExitHandoffPreviousFov, g_atomicExitHandoffSource, g_cinematicContinuationCarryPending, g_cinematicContinuationExitNativeTarget, g_cinematicContinuationInterpolationActive, g_cinematicContinuationPending, g_cinematicContinuationPreviousTarget, g_cinematicContinuationSource, g_cinematicContinuationStartFov, g_cinematicRecoveryOwner, g_cinematicWriterAssociation, g_coordinator, g_exitCinematicTransformedFov, g_exitTargetFov, g_gameplayBaselineContinuationSource, g_postCinematicDialogueExclusion | Always; Same owner/gate; preserve whole writer/recovery dispatch |
| GetCameraWriterCinematicStateView | 1455 | runtime_camera_callbacks.cpp | B6 | g_activeCinematicsEnabled, g_cinematicContinuationInterpolationActive, g_cinematicContinuationSource, g_cinematicContinuationStartFov, g_cinematicFovApplied, g_cinematicSelectionGeneration, g_cinematicSelectionValid, g_cinematicTransformedFov, g_cinematicWriterAssociation, g_matchGameplayEnterNativeFov | Always; Same owner/gate; preserve whole writer/recovery dispatch |
| GetCameraWriterGameplayStateView | 1467 | runtime_camera_callbacks.cpp | B6 | g_fovObservationStore, g_gameplayAspectRestorationStore, g_gameplayBaselineStore, g_gameplayDisableRestorePending, g_gameplayEnableApplyPending, g_gameplayEnabledTransitionDeferralLogged, g_gameplayModeTransitionDeferralLogged, g_gameplayModeTransitionPending, g_gameplayModeTransitionTarget, g_lastAutoRestoreSource, g_lastGameplayCameraFov, g_lastGameplayCameraSource, g_state | Always; Same owner/gate; preserve whole writer/recovery dispatch |
| GetCinematicLifecycleStateView | 1486 | runtime_cinematic_callbacks.cpp | B5 | g_activeCinematicAspectPolicy, g_activeCinematicFovMode, g_activeCinematicsEnabled, g_activeCinematicSettingsEpoch, g_atomicExitHandoffPending, g_atomicExitHandoffPreviousFov, g_atomicExitHandoffSource, g_cinematicContinuationCarryPending, g_cinematicContinuationExitNativeTarget, g_cinematicContinuationInterpolationActive, g_cinematicContinuationPending, g_cinematicContinuationPreviousTarget, g_cinematicContinuationSource, g_cinematicContinuationStartFov, g_cinematicFovApplied, g_cinematicRecoveryOwner, g_cinematicSelectionGeneration, g_cinematicSelectionValid, g_cinematicTransformedFov, g_cinematicWriterAssociation, g_coordinator, g_exitCinematicTransformedFov, g_exitTargetFov, g_gameplayBaselineContinuationSource, g_matchGameplayEnterNativeFov, g_matchGameplayPreEnterAspect, g_matchGameplayPreEnterHorPlusFov, g_matchGameplayPreEnterNativeFov, g_matchGameplayPreEnterPairValid, g_postCinematicDialogueExclusion | Always; Same owner/gate; native register/store/selection order |
| ClearCinematicRecoveryAuxiliaryState | 1509 | runtime_camera_callbacks.cpp | B6 | g_coordinator, g_dialogueMutex | Always; Same owner/gate; preserve whole writer/recovery dispatch |
| CompleteCinematicRecovery | 1532 | runtime_camera_callbacks.cpp | B6 |  | Always; Same owner/gate; preserve whole writer/recovery dispatch |
| LogForeignRecoverySource | 1545 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch |  | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| ArmAtomicExitHandoff | 1556 | runtime_camera_callbacks.cpp | B6 | g_atomicExitHandoffPending, g_atomicExitHandoffPreviousFov, g_atomicExitHandoffSource | Always; Same owner/gate; preserve whole writer/recovery dispatch |
| ApplyDelayedAspectDiagnostic | 1568 | Remain: runtime.cpp | No move in B1-B6 | g_delayedAspectDiagnosticApplied, g_lastAutoRestoreSource, g_state | #ifdef POST_EXIT_DELAYED_ASPECT_DIAGNOSTIC; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| LogGameplayFovSourceChange | 1597 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_coordinator, g_dialogueMutex, g_dialoguePhase, g_lastLoggedCameraFirstPersonFov, g_lastLoggedCameraWorldFov, g_lastLoggedFovSource, g_lastLoggedWriterInputFov, g_state | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| LogPostExitWriterObservation | 1657 | Remain: runtime.cpp | No move in B1-B6 | g_coordinator, g_dialogueMutex, g_dialoguePhase, g_logger, g_postExitTraceArmed, g_postExitTraceSequence, g_postExitTraceStartNs, g_postExitTraceWriterCount, g_previousPostExitTrace, g_state | #if defined(POST_EXIT_RAW_TRACE_DIAGNOSTIC) &#124;&#124; defined(POST_EXIT_INTERPOLATED_FOV_PRODUCER_TRACE); Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| ObservePostExitRawWriterEntry | 1741 | Remain: runtime.cpp | No move in B1-B6 | g_fovWriteAddress, g_postExitTraceArmed, g_postExitTraceSequence, g_postExitTraceStartNs | #if defined(POST_EXIT_RAW_TRACE_DIAGNOSTIC) &#124;&#124; defined(POST_EXIT_INTERPOLATED_FOV_PRODUCER_TRACE); Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| ObservePostExitProducerStore | 1785 | Remain: runtime.cpp | No move in B1-B6 | g_coordinator, g_executable, g_logger, g_postExitProducerStoreHitCount, g_postExitTraceArmed, g_postExitTraceSequence, g_postExitTraceStartNs | #ifdef POST_EXIT_FOV_PRODUCER_STORE_TRACE; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| InstallPostExitProducerStoreTrace | 1812 | Remain: runtime.cpp | No move in B1-B6 | g_executable, g_postExitProducerStoreHook | #ifdef POST_EXIT_FOV_PRODUCER_STORE_TRACE; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| ObservePostExitFovWriteOwner | 1839 | Remain: runtime.cpp | No move in B1-B6 | g_coordinator, g_executable, g_logger, g_postExitTraceArmed, g_postExitTraceSequence, g_postExitTraceStartNs, g_postExitWriteOwnerHitCount | #ifdef POST_EXIT_CAMERA_FOV_WRITE_OWNER_TRACE; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| InstallPostExitFovWriteOwnerTrace | 1871 | Remain: runtime.cpp | No move in B1-B6 | g_executable, g_postExitWriteOwnerHook | #ifdef POST_EXIT_CAMERA_FOV_WRITE_OWNER_TRACE; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| ObservePostExitFovConsumer | 1898 | Remain: runtime.cpp | No move in B1-B6 | g_coordinator, g_logger, g_postExitTraceArmed, g_postExitTraceSequence, g_postExitTraceStartNs | #ifdef POST_EXIT_FOV_STATE_CONSUMER_TRACE; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| InstallPostExitFovConsumerTrace | 1934 | Remain: runtime.cpp | No move in B1-B6 | g_executable, g_postExitConsumerHook | #ifdef POST_EXIT_FOV_STATE_CONSUMER_TRACE; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| LogGameplayFovChange | 1962 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_coordinator, g_lastLoggedGameplayFov, g_lastLoggedGameplayFovSource, g_state | #ifdef FOV_SETTINGS_TRACE_DIAGNOSTIC; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| ComputeSha256 | 1979 | Remain: runtime.cpp | No move in B1-B6 |  | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| CinematicAspectOverrideEnabled | 1984 | Remain: runtime.cpp | No move in B1-B6 | g_runtimeCinematicPolicy | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| IsWritable | 1989 | Remain: private runtime utility; shared private header only if required | B1/B6 reviewed |  | Always; No new owner; identical native checks/exception boundary |
| IsValidFovValue | 1994 | Remain: private runtime utility; shared private header only if required | B1/B6 reviewed |  | Always; No new owner; identical native checks/exception boundary |
| SafeRead | 2000 | Remain: private runtime utility; shared private header only if required | B1/B6 reviewed |  | Always; No new owner; identical native checks/exception boundary |
| LogEnterWriterObservation | 2046 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch |  | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| BeginEnterWriterCapture | 2064 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_enterWriterCapture | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| CompleteEnterWriterCapture | 2106 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_enterWriterCapture | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| RecordEnterWriterObservation | 2165 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_enterWriterCapture | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| ReadClientViewportAspect | 2215 | Remain: runtime.cpp | No move in B1-B6 |  | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| ResolveAutoAspect | 2220 | runtime_cinematic_callbacks.cpp | B5 |  | Always; Same owner/gate; native register/store/selection order |
| ReadAutoAspectForCinematicStoreLog | 2231 | runtime_cinematic_callbacks.cpp | B5 |  | Always; Same owner/gate; native register/store/selection order |
| ResolveAutoAspectForCinematicStore | 2238 | runtime_cinematic_callbacks.cpp | B5 |  | Always; Same owner/gate; native register/store/selection order |
| ResolveCinematicAspect | 2245 | runtime_cinematic_callbacks.cpp | B5 | g_activeCinematicAspectPolicy | Always; Same owner/gate; native register/store/selection order |
| ResolveCinematicAspect | 2251 | runtime_cinematic_callbacks.cpp | B5 |  | Always; Same owner/gate; native register/store/selection order |
| WriteAspectAndFlags | 2257 | Remain: private runtime utility; shared private header only if required | B1/B6 reviewed |  | Always; No new owner; identical native checks/exception boundary |
| WriteAspectOnly | 2263 | Remain: private runtime utility; shared private header only if required | B1/B6 reviewed |  | Always; No new owner; identical native checks/exception boundary |
| GetCameraWriterEngineOperations | 2268 | runtime_camera_callbacks.cpp | B6 |  | Always; Same owner/gate; preserve whole writer/recovery dispatch |
| ApplyPendingGameplayModeTransition | 2288 | runtime_camera_callbacks.cpp | B6 | g_coordinator, g_runtimeGameplayEnabled, g_runtimeGameplayMode | Always; Same owner/gate; preserve whole writer/recovery dispatch |
| ApplyGameplayAspectFixAtomic | 2302 | runtime_camera_callbacks.cpp | B6 | g_coordinator, g_gameplayEnableApplyPending, g_gameplayEnabledTransitionDeferralLogged, g_lastAutoRestoreSource, g_runtimeGameplayEnabled, g_runtimeGameplayMode, g_state | Always; Same owner/gate; preserve whole writer/recovery dispatch |
| ApplyCinematicAspectStore | 2329 | runtime_cinematic_callbacks.cpp | B5 | g_activeCinematicsEnabled, g_cameraCorrectionGate, g_cinematicHookGate, g_cinematicSelectionValid, g_runtimeCinematicsEnabled, g_stopping | Always; Same owner/gate; native register/store/selection order |
| LogCameraModeChange | 2391 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_lastCameraMode, g_lastCombinedOutput, g_logCombinedOutputForInvocation | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| WriteFlagsOnly | 2465 | Remain: runtime.cpp | No move in B1-B6 |  | #ifdef GAMEPLAY_ONE_SHOT_CINEMATIC_TRIGGER; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| OneShotTriggerLoop | 2472 | Remain: runtime.cpp | No move in B1-B6 | g_lastCameraSource, g_oneShotCinematicTriggerArmed, g_readyForOneShot, g_state, g_suppressRearmAfterManualRestore | #ifdef GAMEPLAY_ONE_SHOT_CINEMATIC_TRIGGER; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| LogTransitionSnapshot | 2535 | Remain: runtime.cpp | No move in B1-B6 | g_state | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| LogResolutionChange | 2552 | Remain: runtime.cpp | No move in B1-B6 | g_lastResolutionSnapshot | #ifdef COMBINED_GAMEPLAY_DIAGNOSTIC; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| ResolutionMonitorLoop | 2584 | Remain: runtime.cpp | No move in B1-B6 |  | #ifdef COMBINED_GAMEPLAY_DIAGNOSTIC; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| LogCombinedOutputPost | 2591 | Remain: runtime.cpp | No move in B1-B6 |  | #ifdef COMBINED_GAMEPLAY_DIAGNOSTIC; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| ReplayManualTransitionOriginal | 2616 | Remain: runtime.cpp | No move in B1-B6 | g_coordinator, g_dialogueMutex, g_dialoguePhase, g_fovObservationStore, g_gameplayBaselineStore, g_lastAutoRestoreSource, g_lastCameraSource, g_lastGameplayCameraFov, g_lastGameplayCameraSource, g_logCombinedOutputForInvocation, g_oneShotCinematicTriggerArmed, g_readyForOneShot, g_readyMarkerLogged, g_runtimeGameplayMode, g_state, g_suppressRearmAfterManualRestore, g_transitionTraceSequence | #ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| TryDeferredGameplayReplay | 2771 | Remain: runtime.cpp | No move in B1-B6 | g_coordinator, g_deferredGameplayLastFov, g_deferredGameplayReplayPending, g_deferredGameplaySource, g_deferredGameplayStableSamples, g_deferredGameplayStableSinceNs, g_dialogueMutex, g_dialoguePhase, g_exitTargetFov, g_lastAutoRestoreSource, g_postExitTraceStartNs, g_state | #ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| ValidateEnterBoundary | 2873 | runtime_hook_installation.cpp | B7 unapproved | g_executable | Always; Same HookSet owner; disabled/commit/rollback order |
| ValidateExitBoundary | 2891 | runtime_hook_installation.cpp | B7 unapproved | g_executable | Always; Same HookSet owner; disabled/commit/rollback order |
| ValidateIndexedExitBoundary | 2910 | runtime_hook_installation.cpp | B7 unapproved | g_executable | Always; Same HookSet owner; disabled/commit/rollback order |
| ResolveCinematicAspectStore | 2934 | runtime_hook_installation.cpp | B7 unapproved | g_executable | Always; Same HookSet owner; disabled/commit/rollback order |
| ResolveCinematicFovCallsites | 2952 | runtime_hook_installation.cpp | B7 unapproved | g_cinematicExitIndexed, g_executable | Always; Same HookSet owner; disabled/commit/rollback order |
| InstallCinematicAspect | 3011 | runtime_hook_installation.cpp | B7 unapproved | g_cinematicAspectPatched, g_cinematicAspectStore, g_cinematicAspectStoreHook, g_cinematicOriginalImmediate | Always; Same HookSet owner; disabled/commit/rollback order |
| RestoreCinematicAspect | 3029 | runtime_hook_installation.cpp | B7 unapproved | g_cinematicAspectPatched, g_cinematicAspectStoreHook | Always; Same HookSet owner; disabled/commit/rollback order |
| TraceCinematicEnter | 3036 | runtime_cinematic_callbacks.cpp | B5 | g_cameraCorrectionGate, g_cinematicContinuationExitNativeTarget, g_cinematicContinuationPending, g_cinematicContinuationPreviousTarget, g_cinematicContinuationSource, g_cinematicEnterAspect, g_cinematicHookGate, g_cinematicSelectionGeneration, g_cinematicSelectionValid, g_cinematicsSettingsEpoch, g_cinematicWriterAssociation, g_coordinatorState, g_fovObservationStore, g_gameplayBaselineStore, g_postCinematicDialogueExclusion, g_runtime, g_runtimeCinematicFovMode, g_runtimeCinematicPolicy, g_runtimeCinematicsEnabled, g_runtimeGameplayEnabled, g_runtimeGameplayMode, g_stopping | Always; Same owner/gate; native register/store/selection order |
| TraceCinematicExit | 3251 | runtime_cinematic_callbacks.cpp | B5 | g_cameraCorrectionGate, g_cinematicEnterAspect, g_cinematicExitIndexed, g_cinematicExitLogProbeGeneration, g_cinematicHookGate, g_cinematicSelectionGeneration, g_cinematicSelectionValid, g_cinematicTransformedFov, g_cinematicWriterAssociation, g_coordinator, g_delayedAspectDiagnosticApplied, g_dialogueMutex, g_gameplayBaselineContinuationSource, g_gameplayRecoveryObservationAvailable, g_postExitTraceArmed, g_postExitTraceSequence, g_postExitTraceStartNs, g_postExitTraceWriterCount, g_runtimeGameplayEnabled, g_runtimeGameplayMode, g_stopping | Always; Same owner/gate; native register/store/selection order |
| InstallCinematicAspectComponent | 3458 | runtime_hook_installation.cpp | B7 unapproved | g_config | Always; Same HookSet owner; disabled/commit/rollback order |
| InstallCinematicFovComponent | 3468 | runtime_hook_installation.cpp | B7 unapproved | g_cinematicEnterHook, g_cinematicExitHook | Always; Same HookSet owner; disabled/commit/rollback order |
| CommitCinematicComponents | 3484 | runtime_hook_installation.cpp | B7 unapproved | g_cinematicAspectStoreHook, g_cinematicEnterHook, g_cinematicExitHook, g_cinematicHookGate | Always; Same HookSet owner; disabled/commit/rollback order |
| RollbackCinematicAspectComponent | 3497 | runtime_hook_installation.cpp | B7 unapproved |  | Always; Same HookSet owner; disabled/commit/rollback order |
| RollbackCinematicFovComponent | 3502 | runtime_hook_installation.cpp | B7 unapproved | g_cinematicEnterHook, g_cinematicExitHook, g_cinematicHookGate | Always; Same HookSet owner; disabled/commit/rollback order |
| LogDialogueDiagnosticReturn | 3519 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_dialogueDiagnosticReturn1, g_dialogueDiagnosticReturn2, g_dialogueDiagnosticReturn3, g_dialogueDiagnosticReturn4, g_dialogueDiagnosticReturn5 | #ifdef DIALOGUE_BOUNDARY_DIAGNOSTIC; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| LogDialogueDiagnosticSummary | 3535 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_dialogueDiagnosticCallbacks, g_dialogueDiagnosticReturn1, g_dialogueDiagnosticReturn2, g_dialogueDiagnosticReturn3, g_dialogueDiagnosticReturn4, g_dialogueDiagnosticReturn5 | #ifdef DIALOGUE_BOUNDARY_DIAGNOSTIC; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| ArmDialogueRecoveryEndpointTrace | 3570 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_dialogueRecoveryEndpointTrace | #ifdef DIALOGUE_RECOVERY_ENDPOINT_DIAGNOSTIC; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| ObserveDialogueRecoveryEndpointTrace | 3579 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_dialogueRecoveryEndpointTrace | #ifdef DIALOGUE_RECOVERY_ENDPOINT_DIAGNOSTIC; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| TraceDialogueBoundary | 3646 | Remain: runtime.cpp | B4 complete; approved Remain (2026-10-09) | g_activeDialoguePolicy, g_cameraCorrectionGate, g_coordinator, g_dialogueBaseline, g_dialogueCorrectionActivation, g_dialogueDiagnosticCallbacks, g_dialogueMutex, g_dialoguePhase, g_dialoguePrevious, g_dialogueRecoveryRearm | Native XMM6/RSI adapter ordering spans XMM1 write, snapshot publication/wake, diagnostics and gate release |
| InstallDialogueBoundary | 3793 | runtime_hook_installation.cpp | B7 unapproved | g_dialogueBoundaryHook, g_dialogueBoundaryHookAvailable, g_executable, g_runtimeDialoguePolicy | Always; Same HookSet owner; disabled/commit/rollback order |
| ReadHorPlusCinematicWriterState | 3843 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_cinematicEnterAspect, g_cinematicTransformedFov, g_coordinator | #ifdef HORPLUS_CINEMATIC_WRITER_TRACE_DIAGNOSTIC; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| HorPlusCinematicWriterSnapshotChanged | 3871 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_previousHorPlusCinematicWriter | #ifdef HORPLUS_CINEMATIC_WRITER_TRACE_DIAGNOSTIC; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| LogHorPlusCinematicWriterTrace | 3893 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_coordinator, g_matchGameplayEnterNativeFov, g_matchGameplayPreEnterAspect, g_matchGameplayPreEnterHorPlusFov, g_matchGameplayPreEnterNativeFov, g_matchGameplayPreEnterPairValid | #ifdef HORPLUS_CINEMATIC_WRITER_TRACE_DIAGNOSTIC; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| TraceHorPlusCinematicWriterBefore | 3944 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_coordinator, g_logHorPlusCinematicWriterInvocation | #ifdef HORPLUS_CINEMATIC_WRITER_TRACE_DIAGNOSTIC; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| TraceHorPlusCinematicWriterAfter | 3958 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_coordinator, g_logHorPlusCinematicWriterInvocation, g_previousHorPlusCinematicWriter | #ifdef HORPLUS_CINEMATIC_WRITER_TRACE_DIAGNOSTIC; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| FovTraceValueChanged | 3995 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch |  | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| TraceGameplayFovSourceSample | 4003 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch |  | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| TraceHorPlusRecoverySample | 4075 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_runtimeGameplayEnabled | Always; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| MarkCameraStateFreshness | 4106 | runtime_camera_publication.cpp | B3 | g_cameraStateMutex, g_cameraStateSnapshot, g_cameraStateSnapshotValid | Always; Same owner; preserve correction gate then cameraStateMutex and wake order |
| MarkCameraPresentationTransition | 4125 | runtime_camera_publication.cpp | B3 | g_cameraStateMutex, g_cameraStateSnapshot, g_cameraStateSnapshotValid | Always; Same owner; preserve correction gate then cameraStateMutex and wake order |
| MarkCameraStateForSetting | 4145 | runtime_camera_publication.cpp | B3 | g_cameraStateMutex, g_cameraStateSnapshot, g_cameraStateSnapshotValid | Always; Same owner; preserve correction gate then cameraStateMutex and wake order |
| PublishCameraStateSnapshot | 4163 | runtime_camera_publication.cpp | B3 | g_activeDialoguePolicy, g_cameraStateMutex, g_cameraStateSnapshot, g_cameraStateSnapshotValid, g_coordinator, g_dialogueMutex, g_dialoguePhase, g_gameplayModeTransitionPending, g_neutralGameplayFovCandidate, g_neutralGameplayFovCandidateSource, g_neutralGameplayFovSamples, g_postCinematicDialogueExclusion, g_runtime, g_runtimeGameplayMode, g_snapshotZoomActive, g_snapshotZoomHold | Always; Same owner; preserve correction gate then cameraStateMutex and wake order |
| PublishCameraFovTransaction | 4255 | runtime_camera_publication.cpp | B3 | g_cameraStateMutex, g_cameraStateSnapshot, g_cameraStateSnapshotValid, g_runtime | Always; Same owner; preserve correction gate then cameraStateMutex and wake order |
| HorPlusTelemetryOwnerName | 4325 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch |  | #ifdef HORPLUS_FOV_STATE_DIAGNOSTIC; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| ResolveHorPlusTelemetryOwner | 4337 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_coordinator, g_dialogueMutex, g_dialoguePhase, g_postCinematicDialogueExclusion | #ifdef HORPLUS_FOV_STATE_DIAGNOSTIC; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| TraceHorPlusFovState | 4355 | runtime_diagnostics.cpp (coherent domain subsets if necessary) | B3 separate reviewed sub-batch | g_coordinator, g_dialogueMutex, g_dialoguePhase, g_horPlusFovTelemetryMutex, g_postCinematicDialogueExclusion, g_runtime, g_runtimeGameplayMode | #ifdef HORPLUS_FOV_STATE_DIAGNOSTIC; Original owner/TLS/lazy storage; existing guards; deferred emission after gate |
| ReplayManualTransition | 4499 | runtime_camera_callbacks.cpp | B6 | g_cameraCorrectionGate, g_cameraWriterHookGate, g_cinematicExitLogProbeGeneration, g_cinematicFovApplied, g_cinematicTransformedFov, g_coordinator, g_dialogueMutex, g_logCombinedOutputForInvocation, g_postExitTraceArmed, g_postExitTraceStartNs, g_postExitTraceWriterCount, g_runtimeGameplayEnabled, g_runtimeGameplayMode, g_stopping, g_transitionTraceSequence | Always; Same owner/gate; preserve whole writer/recovery dispatch |
| ZoomTransitionFloatChanged | 5204 | Remain: runtime.cpp | B7 optional |  | Always; Zoom affinity/TLS unchanged; no extraction until justified |
| TraceZoomTransition | 5211 | Remain: runtime.cpp | B7 optional | g_coordinator, g_neutralGameplayFovSamples, g_previousZoomIn, g_previousZoomOut, g_runtime, g_snapshotZoomActive, g_snapshotZoomHold, g_state, g_zoomInHits, g_zoomLoggedEdges, g_zoomOutHits | Always; Zoom affinity/TLS unchanged; no extraction until justified |
| TraceZoomIn | 5279 | Remain: runtime.cpp | B7 optional |  | Always; Zoom affinity/TLS unchanged; no extraction until justified |
| TraceZoomOut | 5280 | Remain: runtime.cpp | B7 optional |  | Always; Zoom affinity/TLS unchanged; no extraction until justified |
| InstallZoomTransitionObservation | 5282 | runtime_hook_installation.cpp | B7 unapproved | g_executable, g_zoomInHook, g_zoomOutHook | Always; Same HookSet owner; disabled/commit/rollback order |
| VerifyExecutableAndInstruction | 5327 | runtime_hook_installation.cpp | B7 unapproved | g_executable, g_fovWriteAddress | Always; Same HookSet owner; disabled/commit/rollback order |
| ApplyHorPlusGameplay | 5343 | Remain: runtime.cpp | No move in B1-B6 | g_coordinator, g_dialogueMutex, g_dialoguePhase, g_fovObservationStore, g_gameplayBaselineStore, g_gameplayModeTransitionPending, g_lastGameplayCameraFov, g_lastGameplayCameraSource, g_runtimeGameplayEnabled, g_runtimeGameplayMode | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| ApplyHorPlusGameplayDiagnostic | 5474 | Remain: runtime.cpp | No move in B1-B6 | g_coordinator, g_horPlusDiagnosticApplications, g_horPlusDiagnosticChanges, g_horPlusDiagnosticWriterCallbacks, g_previousHorPlusDiagnostic | #ifdef POST_EXIT_GAMEPLAY_OBSERVER_DIAGNOSTIC; #ifdef HORPLUS_GAMEPLAY_DIAGNOSTIC; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| ObservePostExitDiagnosticWriter | 5533 | Remain: runtime.cpp | No move in B1-B6 |  | #ifdef POST_EXIT_GAMEPLAY_OBSERVER_DIAGNOSTIC; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| InstallPostExitGameplayObserverDiagnostic | 5546 | Remain: runtime.cpp | No move in B1-B6 | g_fovWriteAddress, g_postExitGameplayObserverHook | #ifdef POST_EXIT_GAMEPLAY_OBSERVER_DIAGNOSTIC; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| DialogueDiscoveryNowNs | 5590 | Remain: runtime.cpp | No move in B1-B6 |  | #ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| LogDialogueDiscoveryEnd | 5596 | Remain: runtime.cpp | No move in B1-B6 | g_dialogueDiscoveryActive, g_dialogueDiscoveryChanges, g_dialogueDiscoverySamples, g_dialogueDiscoveryStartNs, g_runtime | #ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| LogDialogueDiscoveryChanges | 5608 | Remain: runtime.cpp | No move in B1-B6 |  | #ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| TraceDialogueDiscovery | 5664 | Remain: runtime.cpp | No move in B1-B6 | g_activeDialoguePolicy, g_coordinator, g_dialogueDiscoveryActive, g_dialogueDiscoveryChanges, g_dialogueDiscoverySamples, g_dialogueDiscoverySeenResetGeneration, g_dialogueDiscoveryStartNs, g_dialogueMutex, g_dialoguePhase, g_previousDialogueDiscovery, g_runtime, g_runtimeDialoguePolicy, g_state | #ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| DialogueDiscoveryLoop | 5734 | Remain: runtime.cpp | No move in B1-B6 | g_dialogueDiscoveryActive, g_dialogueDiscoveryChanges, g_dialogueDiscoverySamples, g_dialogueDiscoveryStartNs, g_previousDialogueDiscovery | #ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| SelectGameplayMode | 5758 | runtime_settings.cpp | B2 | g_coordinator, g_gameplayBaselineStore, g_gameplayModeTransitionDeferralLogged, g_gameplayModeTransitionPending, g_gameplayModeTransitionTarget, g_horPlusFovTelemetryMutex, g_lastAutoRestoreSource, g_lastGameplayCameraFov, g_lastGameplayCameraSource, g_runtime, g_runtimeGameplayEnabled, g_runtimeGameplayMode, g_state | Always; Same owner; settings under correction gate; persistence outside gate |
| RuntimeHotkeyBinding | 5817 | runtime_settings.cpp | B2 |  | Always; Same owner; settings under correction gate; persistence outside gate |
| ApplyGameplayModeSetting | 5835 | runtime_settings.cpp | B2 |  | Always; Same owner; settings under correction gate; persistence outside gate |
| ApplyRuntimeSetting | 5840 | runtime_settings.cpp | B2 | g_activeCinematicsEnabled, g_activeCinematicSettingsEpoch, g_activeDialogueSettingsEpoch, g_cameraCorrectionGate, g_cinematicContinuationCarryPending, g_cinematicContinuationInterpolationActive, g_cinematicContinuationPending, g_cinematicContinuationSource, g_cinematicFovApplied, g_cinematicsSettingsEpoch, g_cinematicTransformedFov, g_config, g_dialogueCorrectionActivation, g_dialogueNonNativeCapabilityAvailable, g_dialogueSettingsEpoch, g_exitCinematicTransformedFov, g_gameplayDisableRestorePending, g_gameplayEnableApplyPending, g_gameplayEnabledTransitionDeferralLogged, g_gameplayModeTransitionPending, g_gameplayRecoveryObservationAvailable, g_gameplaySettingsEpoch, g_hotkeyConfigMutex, g_runtimeCinematicFovMode, g_runtimeCinematicPolicy, g_runtimeCinematicsEnabled, g_runtimeDialogueEnabled, g_runtimeDialoguePolicy, g_runtimeGameplayEnabled, g_runtimeGameplayMode, g_runtimeOverlayAutoLocaleSynchronized, g_runtimeOverlayFontSize, g_runtimeOverlayLocale, g_runtimeOverlayLocaleAuto | Always; Same owner; settings under correction gate; persistence outside gate |
| ReadRuntimeSettings | 5948 | runtime_settings.cpp | B2 | g_cameraCorrectionGate, g_config, g_hotkeyConfigMutex, g_runtimeCinematicFovMode, g_runtimeCinematicPolicy, g_runtimeCinematicsEnabled, g_runtimeDialogueEnabled, g_runtimeDialoguePolicy, g_runtimeGameplayEnabled, g_runtimeGameplayMode, g_runtimeOverlayAutoLocaleSynchronized, g_runtimeOverlayFontSize, g_runtimeOverlayLocale, g_runtimeOverlayLocaleAuto | Always; Same owner; settings under correction gate; persistence outside gate |
| ReadOverlaySemanticSnapshot | 5977 | runtime_camera_publication.cpp | B3 | g_activeCinematicAspectPolicy, g_activeCinematicFovMode, g_activeCinematicsEnabled, g_activeDialoguePolicy, g_cameraCorrectionGate, g_cameraStateMutex, g_cameraStateSnapshot, g_cameraStateSnapshotValid, g_cameraWriterHookGate, g_cinematicAspectComponentAvailable, g_cinematicLifecycleObservationAvailable, g_cinematicSelectionValid, g_coordinatorState, g_dialogueBoundaryHookAvailable, g_dialogueCorrectionActivation, g_dialogueMutex, g_dialogueNonNativeCapabilityAvailable, g_dialoguePhase, g_gameplayBaselineStore, g_gameplayDisableRestorePending, g_gameplayEnableApplyPending, g_gameplayModeTransitionPending, g_runtime, g_runtimeCinematicFovMode, g_runtimeCinematicPolicy, g_runtimeCinematicsEnabled, g_runtimeDialogueEnabled, g_runtimeDialoguePolicy, g_runtimeGameplayEnabled, g_runtimeGameplayMode, g_runtimeInitializationComplete, g_stopping | Always; Same owner; preserve correction gate then cameraStateMutex and wake order |
| PersistHotkeyActionValue | 6159 | runtime_settings.cpp | B2 | g_configPath | Always; Same owner; settings under correction gate; persistence outside gate |
| PersistRuntimeSetting | 6166 | runtime_settings.cpp | B2 | g_configPath, g_runtimeOverlayLocale | Always; Same owner; settings under correction gate; persistence outside gate |
| HotkeyLoop | 6256 | Remain: runtime.cpp | No move in B1-B6 | g_activeDialoguePolicy, g_dialogueMutex, g_gameplayModeTransitionPending, g_hotkeyRebindCaptureActive, g_hotkeyRebindConsumedKey, g_runtimeCinematicFovMode, g_runtimeCinematicPolicy, g_runtimeDialoguePolicy, g_runtimeGameplayMode, g_runtimeSettings | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| Initialize | 6406 | Remain: runtime.cpp | No move in B1-B6 | g_cameraWriterHookGate, g_cinematicAspectComponentAvailable, g_cinematicLifecycleObservationAvailable, g_config, g_configPath, g_dialogueBoundaryHook, g_dialogueNonNativeCapabilityAvailable, g_fovWriteAddress, g_gameplayRecoveryObservationAvailable, g_hook, g_logger, g_module, g_postExitTraceArmed, g_postExitTraceSequence, g_postExitTraceStartNs, g_postExitTraceWriterCount, g_runtime, g_runtimeCinematicFovMode, g_runtimeCinematicPolicy, g_runtimeCinematicsEnabled, g_runtimeDialogueEnabled, g_runtimeDialoguePolicy, g_runtimeGameplayEnabled, g_runtimeGameplayMode, g_runtimeInitializationComplete, g_runtimeOverlayFontSize, g_runtimeOverlayLocale, g_runtimeOverlayLocaleAuto, g_runtimeSettings, g_stopping | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| PublishOverlayNotification | 6671 | Remain: runtime.cpp | No move in B1-B6 | g_config, g_nextNotificationId, g_notificationMutex, g_pendingNotifications | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| DrainOverlayNotifications | 6690 | Remain: runtime.cpp | No move in B1-B6 | g_notificationMutex, g_pendingNotifications | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| SetHotkeyRebindCaptureActive | 6703 | Remain: runtime.cpp | No move in B1-B6 | g_hotkeyRebindCaptureActive | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| MarkHotkeyRebindKeyConsumed | 6708 | Remain: runtime.cpp | No move in B1-B6 | g_hotkeyRebindConsumedKey | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| GetRuntimeSettingsApi | 6713 | Remain: runtime.cpp | No move in B1-B6 | g_runtimeSettings | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| SetDetectedOverlayLocale | 6718 | Remain: runtime.cpp | No move in B1-B6 | g_runtimeOverlayAutoLocaleSynchronized, g_runtimeOverlayFontSize, g_runtimeOverlayLocale, g_runtimeOverlayLocaleAuto | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| GetRuntimeSettingsSnapshot | 6740 | Remain: runtime.cpp | No move in B1-B6 |  | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| GetOverlaySemanticSnapshot | 6748 | Remain: runtime.cpp | No move in B1-B6 | g_runtimeSettings | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| GetRuntimeViewportAspect | 6753 | Remain: runtime.cpp | No move in B1-B6 |  | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| SetModuleHandle | 6758 | Remain: runtime.cpp | No move in B1-B6 | g_module | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| InitializeThread | 6763 | Remain: runtime.cpp | No move in B1-B6 | g_config, g_logger, g_runtime | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| Shutdown | 6786 | Remain: runtime.cpp | No move in B1-B6 | g_logger | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |
| NotifyProcessDetach | 6792 | Remain: runtime.cpp | No move in B1-B6 | g_cameraWriterHookGate, g_cinematicHookGate, g_stopping, g_workers | Always; Facade/startup/worker or dormant experimental helper; no cleanup; B7 requires exact subset approval |

## RuntimeState members (one entry per field)

| Member | Line | Declaration/initializer first line | Owner / destination / batch | Source guard |
|---|---:|---|---|---|
| module | 158 | HMODULE module{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| executable | 159 | HMODULE executable = GetModuleHandle(nullptr); | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| workers | 160 | plugin::WorkerLifecycle workers; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| fovWriteAddress | 161 | std::uint8_t* fovWriteAddress{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| hooks | 162 | hooks::HookSet hooks; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| logger | 163 | std::shared_ptr<spdlog::logger> logger; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| config | 164 | config::FeatureConfig config{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| hotkeyConfigMutex | 165 | std::mutex hotkeyConfigMutex; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cameraCorrectionGate | 168 | plugin::CameraCorrectionGate cameraCorrectionGate; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| gameplaySettingsEpoch | 169 | std::atomic<std::uint64_t> gameplaySettingsEpoch{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cinematicsSettingsEpoch | 170 | std::atomic<std::uint64_t> cinematicsSettingsEpoch{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| dialogueSettingsEpoch | 171 | std::atomic<std::uint64_t> dialogueSettingsEpoch{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| activeCinematicSettingsEpoch | 172 | std::atomic<std::uint64_t> activeCinematicSettingsEpoch{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| activeDialogueSettingsEpoch | 173 | std::atomic<std::uint64_t> activeDialogueSettingsEpoch{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| hotkeyRebindCaptureActive | 174 | std::atomic<bool> hotkeyRebindCaptureActive{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| hotkeyRebindConsumedKey | 175 | std::atomic<int> hotkeyRebindConsumedKey{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| gameplayRecoveryObservationAvailable | 176 | std::atomic<bool> gameplayRecoveryObservationAvailable{false}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| runtimeInitializationComplete | 177 | std::atomic<bool> runtimeInitializationComplete{false}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| overlayStartupSucceeded | 178 | std::atomic<bool> overlayStartupSucceeded{false}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| runtimeGameplayEnabled | 179 | std::atomic<bool> runtimeGameplayEnabled{true}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| gameplayEnableApplyPending | 180 | std::atomic<bool> gameplayEnableApplyPending{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| gameplayDisableRestorePending | 181 | std::atomic<bool> gameplayDisableRestorePending{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| gameplayEnabledTransitionDeferralLogged | 182 | std::atomic<bool> gameplayEnabledTransitionDeferralLogged{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| runtimeGameplayMode | 183 | std::atomic<config::GameplayMode> runtimeGameplayMode{ | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| runtimeCinematicsEnabled | 185 | std::atomic<bool> runtimeCinematicsEnabled{true}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| activeCinematicsEnabled | 186 | std::atomic<bool> activeCinematicsEnabled{true}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| gameplayModeTransitionPending | 187 | std::atomic<bool> gameplayModeTransitionPending{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| gameplayModeTransitionDeferralLogged | 188 | std::atomic<bool> gameplayModeTransitionDeferralLogged{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| gameplayModeTransitionTarget | 189 | std::atomic<config::GameplayMode> gameplayModeTransitionTarget{ | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| runtimeCinematicPolicy | 191 | std::atomic<config::CinematicAspectPolicy> runtimeCinematicPolicy{ | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| runtimeCinematicFovMode | 193 | std::atomic<config::CinematicFovMode> runtimeCinematicFovMode{ | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| activeCinematicAspectPolicy | 195 | std::atomic<config::CinematicAspectPolicy> activeCinematicAspectPolicy{ | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| activeCinematicFovMode | 197 | std::atomic<config::CinematicFovMode> activeCinematicFovMode{ | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cinematicSelectionGeneration | 199 | std::atomic<std::uint64_t> cinematicSelectionGeneration{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cinematicSelectionValid | 200 | std::atomic<bool> cinematicSelectionValid{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cinematicExitIndexed | 201 | std::atomic<bool> cinematicExitIndexed{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cinematicExitLogProbeGeneration | 202 | std::atomic<std::uint64_t> cinematicExitLogProbeGeneration{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cameraWriterHookGate | 203 | std::atomic<bool> cameraWriterHookGate{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cinematicHookGate | 204 | std::atomic<bool> cinematicHookGate{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| stopping | 205 | std::atomic<bool> stopping{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| runtimeDialoguePolicy | 206 | std::atomic<config::DialogueZoomPolicy> runtimeDialoguePolicy{ | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| runtimeDialogueEnabled | 208 | std::atomic<bool> runtimeDialogueEnabled{true}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| runtimeOverlayLocale | 209 | std::atomic<const localization::LocaleDescriptor*> runtimeOverlayLocale{ | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| runtimeOverlayLocaleAuto | 211 | std::atomic<bool> runtimeOverlayLocaleAuto{true}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| runtimeOverlayAutoLocaleSynchronized | 212 | std::atomic<bool> runtimeOverlayAutoLocaleSynchronized{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| runtimeOverlayFontSize | 213 | std::atomic<int> runtimeOverlayFontSize{config::OverlayFontSizeDefault}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cinematicLifecycleObservationAvailable | 214 | std::atomic<bool> cinematicLifecycleObservationAvailable{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cinematicAspectComponentAvailable | 215 | std::atomic<bool> cinematicAspectComponentAvailable{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| dialogueBoundaryHookAvailable | 216 | std::atomic<bool> dialogueBoundaryHookAvailable{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| dialogueNonNativeCapabilityAvailable | 217 | std::atomic<bool> dialogueNonNativeCapabilityAvailable{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| configPath | 218 | std::filesystem::path configPath; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| state | 219 | std::atomic<ReplayState> state{ReplayState::WaitingForAutomaticUpdate}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| lastCameraMode | 220 | std::atomic<std::uint32_t> lastCameraMode{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| transitionTraceSequence | 221 | std::atomic<std::uint64_t> transitionTraceSequence{0}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| coordinator | 222 | std::atomic<CoordinatorState> coordinator{CoordinatorState::Gameplay}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| fovObservationStore | 223 | camera::CameraFovObservationStore fovObservationStore{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cameraTransitionBoundary | 224 | camera::CameraTransitionBoundaryObservation cameraTransitionBoundary{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| gameplayBaselineStore | 225 | camera::GameplayBaselineStore gameplayBaselineStore{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cinematicWriterAssociation | 226 | cinematics::CinematicWriterAssociation cinematicWriterAssociation{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cinematicRecoveryOwner | 227 | cinematics::CinematicRecoveryOwner cinematicRecoveryOwner{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cinematicContinuationTrace | 228 | std::unique_ptr<diagnostics::CinematicContinuationTrace> cinematicContinuationTrace{ | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| gameplayAspectRestorationStore | 230 | camera::GameplayAspectRestorationStore gameplayAspectRestorationStore{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cameraStateMutex | 231 | std::mutex cameraStateMutex; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cameraStateSnapshot | 232 | camera::CameraStateSnapshot cameraStateSnapshot{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cameraStateSnapshotValid | 233 | bool cameraStateSnapshotValid{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| neutralGameplayFovCandidate | 234 | float neutralGameplayFovCandidate{std::numeric_limits<float>::quiet_NaN()}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| neutralGameplayFovSamples | 235 | std::uint32_t neutralGameplayFovSamples{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| neutralGameplayFovCandidateSource | 236 | std::uintptr_t neutralGameplayFovCandidateSource{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cinematicFovApplied | 237 | std::atomic<bool> cinematicFovApplied{false}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cinematicTransformedFov | 238 | std::atomic<float> cinematicTransformedFov{std::numeric_limits<float>::quiet_NaN()}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| exitCinematicTransformedFov | 239 | std::atomic<float> exitCinematicTransformedFov{std::numeric_limits<float>::quiet_NaN()}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| matchGameplayEnterNativeFov | 240 | std::atomic<float> matchGameplayEnterNativeFov{std::numeric_limits<float>::quiet_NaN()}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| matchGameplayPreEnterNativeFov | 241 | std::atomic<float> matchGameplayPreEnterNativeFov{std::numeric_limits<float>::quiet_NaN()}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| matchGameplayPreEnterHorPlusFov | 242 | std::atomic<float> matchGameplayPreEnterHorPlusFov{std::numeric_limits<float>::quiet_NaN()}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| matchGameplayPreEnterAspect | 243 | std::atomic<float> matchGameplayPreEnterAspect{std::numeric_limits<float>::quiet_NaN()}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| gameplayBaselineContinuationSource | 247 | std::atomic<std::uintptr_t> gameplayBaselineContinuationSource{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cinematicContinuationPending | 248 | std::atomic<bool> cinematicContinuationPending{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cinematicContinuationCarryPending | 249 | std::atomic<bool> cinematicContinuationCarryPending{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cinematicContinuationSource | 250 | std::atomic<std::uintptr_t> cinematicContinuationSource{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cinematicContinuationExitNativeTarget | 251 | std::atomic<float> cinematicContinuationExitNativeTarget{ | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cinematicContinuationPreviousTarget | 253 | std::atomic<float> cinematicContinuationPreviousTarget{ | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cinematicContinuationInterpolationActive | 255 | std::atomic<bool> cinematicContinuationInterpolationActive{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cinematicContinuationStartFov | 256 | std::atomic<float> cinematicContinuationStartFov{ | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| matchGameplayPreEnterPairValid | 258 | std::atomic<bool> matchGameplayPreEnterPairValid{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cinematicEnterAspect | 260 | std::atomic<float> cinematicEnterAspect{std::numeric_limits<float>::quiet_NaN()}; | g_runtime only; resident runtime.cpp / private definition B1 | #ifdef HORPLUS_CINEMATIC_WRITER_TRACE_DIAGNOSTIC |
| exitTargetFov | 262 | std::atomic<float> exitTargetFov{0.0f}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| postCinematicDialogueExclusion | 263 | dialogue::PostCinematicRecoveryExclusion postCinematicDialogueExclusion{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| postExitTraceArmed | 265 | std::atomic<bool> postExitTraceArmed{false}; | g_runtime only; resident runtime.cpp / private definition B1 | #ifdef POST_EXIT_TRACE_STATE |
| postExitTraceWriterCount | 266 | std::atomic<std::uint32_t> postExitTraceWriterCount{0}; | g_runtime only; resident runtime.cpp / private definition B1 | #ifdef POST_EXIT_TRACE_STATE |
| postExitTraceStartNs | 267 | std::atomic<std::int64_t> postExitTraceStartNs{0}; | g_runtime only; resident runtime.cpp / private definition B1 | #ifdef POST_EXIT_TRACE_STATE |
| postExitTraceSequence | 268 | std::atomic<std::uint64_t> postExitTraceSequence{0}; | g_runtime only; resident runtime.cpp / private definition B1 | #ifdef POST_EXIT_TRACE_STATE |
| deferredGameplayReplayPending | 271 | std::atomic<bool> deferredGameplayReplayPending{false}; | g_runtime only; resident runtime.cpp / private definition B1 | #ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST |
| deferredGameplaySource | 272 | std::atomic<std::uintptr_t> deferredGameplaySource{}; | g_runtime only; resident runtime.cpp / private definition B1 | #ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST |
| deferredGameplayStableSamples | 273 | std::atomic<std::uint32_t> deferredGameplayStableSamples{0}; | g_runtime only; resident runtime.cpp / private definition B1 | #ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST |
| deferredGameplayLastFov | 274 | std::atomic<float> deferredGameplayLastFov{std::numeric_limits<float>::quiet_NaN()}; | g_runtime only; resident runtime.cpp / private definition B1 | #ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST |
| deferredGameplayStableSinceNs | 275 | std::atomic<std::int64_t> deferredGameplayStableSinceNs{0}; | g_runtime only; resident runtime.cpp / private definition B1 | #ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST |
| atomicExitHandoffPending | 277 | std::atomic<bool> atomicExitHandoffPending{false}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| atomicExitHandoffSource | 278 | std::atomic<std::uintptr_t> atomicExitHandoffSource{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| atomicExitHandoffPreviousFov | 279 | std::atomic<float> atomicExitHandoffPreviousFov{std::numeric_limits<float>::quiet_NaN()}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| delayedAspectDiagnosticApplied | 281 | std::atomic<bool> delayedAspectDiagnosticApplied{false}; | g_runtime only; resident runtime.cpp / private definition B1 | #ifdef POST_EXIT_DELAYED_ASPECT_DIAGNOSTIC |
| horPlusDiagnosticWriterCallbacks | 284 | std::atomic<std::uint64_t> horPlusDiagnosticWriterCallbacks{0}; | g_runtime only; resident runtime.cpp / private definition B1 | #ifdef HORPLUS_GAMEPLAY_DIAGNOSTIC |
| horPlusDiagnosticApplications | 285 | std::atomic<std::uint64_t> horPlusDiagnosticApplications{0}; | g_runtime only; resident runtime.cpp / private definition B1 | #ifdef HORPLUS_GAMEPLAY_DIAGNOSTIC |
| horPlusDiagnosticChanges | 286 | std::atomic<std::uint64_t> horPlusDiagnosticChanges{0}; | g_runtime only; resident runtime.cpp / private definition B1 | #ifdef HORPLUS_GAMEPLAY_DIAGNOSTIC |
| dialogueDiscoveryActive | 289 | std::atomic<bool> dialogueDiscoveryActive{false}; | g_runtime only; resident runtime.cpp / private definition B1 | #ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC |
| dialogueDiscoveryStartNs | 290 | std::atomic<std::int64_t> dialogueDiscoveryStartNs{0}; | g_runtime only; resident runtime.cpp / private definition B1 | #ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC |
| dialogueDiscoverySamples | 291 | std::atomic<std::uint64_t> dialogueDiscoverySamples{0}; | g_runtime only; resident runtime.cpp / private definition B1 | #ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC |
| dialogueDiscoveryChanges | 292 | std::atomic<std::uint64_t> dialogueDiscoveryChanges{0}; | g_runtime only; resident runtime.cpp / private definition B1 | #ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC |
| dialogueDiscoveryResetGeneration | 293 | std::atomic<std::uint64_t> dialogueDiscoveryResetGeneration{0}; | g_runtime only; resident runtime.cpp / private definition B1 | #ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC |
| zoomInHook | 295 | SafetyHookMid zoomInHook; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| zoomOutHook | 296 | SafetyHookMid zoomOutHook; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| zoomInHits | 297 | std::atomic<std::uint64_t> zoomInHits{0}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| zoomOutHits | 298 | std::atomic<std::uint64_t> zoomOutHits{0}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| zoomLoggedEdges | 299 | std::atomic<std::uint64_t> zoomLoggedEdges{0}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| snapshotZoomDirection | 300 | std::atomic<std::uint8_t> snapshotZoomDirection{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| snapshotZoomSequence | 301 | std::atomic<std::uint64_t> snapshotZoomSequence{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| snapshotZoomSource | 302 | std::atomic<std::uintptr_t> snapshotZoomSource{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| snapshotZoomPrimary | 303 | std::atomic<float> snapshotZoomPrimary{std::numeric_limits<float>::quiet_NaN()}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| snapshotZoomSecondary | 304 | std::atomic<float> snapshotZoomSecondary{std::numeric_limits<float>::quiet_NaN()}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| snapshotZoomActive | 305 | std::atomic<bool> snapshotZoomActive{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| snapshotZoomObservationValid | 306 | std::atomic<bool> snapshotZoomObservationValid{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| snapshotZoomHold | 307 | std::atomic<bool> snapshotZoomHold{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| snapshotEventSequence | 308 | std::atomic<std::uint64_t> snapshotEventSequence{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| snapshotDialogueSource | 309 | std::atomic<std::uintptr_t> snapshotDialogueSource{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| snapshotDialogueTarget | 310 | std::atomic<float> snapshotDialogueTarget{std::numeric_limits<float>::quiet_NaN()}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| snapshotDialogueTargetValid | 311 | std::atomic<bool> snapshotDialogueTargetValid{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| horPlusFovTelemetryMutex | 312 | std::mutex horPlusFovTelemetryMutex; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| horPlusFovTelemetryValid | 313 | bool horPlusFovTelemetryValid{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| horPlusGameplayCacheValid | 314 | bool horPlusGameplayCacheValid{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| stableObservationSource | 315 | std::uintptr_t stableObservationSource{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| stableObservationFov | 316 | float stableObservationFov{std::numeric_limits<float>::quiet_NaN()}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| stableObservationSamples | 317 | std::uint32_t stableObservationSamples{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| stableObservationReported | 318 | bool stableObservationReported{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| horPlusTelemetryNativeFov | 319 | float horPlusTelemetryNativeFov{std::numeric_limits<float>::quiet_NaN()}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| horPlusTelemetryAspect | 320 | float horPlusTelemetryAspect{std::numeric_limits<float>::quiet_NaN()}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| horPlusTelemetryResult | 321 | float horPlusTelemetryResult{std::numeric_limits<float>::quiet_NaN()}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| horPlusTelemetryFlags | 322 | std::uint8_t horPlusTelemetryFlags{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| horPlusTelemetryMode | 323 | config::GameplayMode horPlusTelemetryMode{config::GameplayMode::AspectRecalculation}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| horPlusTelemetryOwner | 324 | std::uint8_t horPlusTelemetryOwner{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| postExitWriteOwnerHitCount | 326 | std::atomic<std::uint32_t> postExitWriteOwnerHitCount{0}; | g_runtime only; resident runtime.cpp / private definition B1 | #ifdef POST_EXIT_CAMERA_FOV_WRITE_OWNER_TRACE |
| postExitProducerStoreHitCount | 329 | std::atomic<std::uint32_t> postExitProducerStoreHitCount{0}; | g_runtime only; resident runtime.cpp / private definition B1 | #ifdef POST_EXIT_FOV_PRODUCER_STORE_TRACE |
| cinematicAspectStore | 331 | std::uint8_t* cinematicAspectStore{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cinematicOriginalImmediate | 332 | std::uint8_t cinematicOriginalImmediate[sizeof(kCinematicOriginalImmediate)]{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| cinematicAspectPatched | 333 | bool cinematicAspectPatched{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| lastAutoRestoreSource | 334 | std::atomic<std::uintptr_t> lastAutoRestoreSource{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| lastGameplayCameraSource | 335 | std::atomic<std::uintptr_t> lastGameplayCameraSource{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| lastGameplayCameraFov | 336 | std::atomic<float> lastGameplayCameraFov{std::numeric_limits<float>::quiet_NaN()}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| lastLoggedFovSource | 337 | std::uintptr_t lastLoggedFovSource{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| lastLoggedWriterInputFov | 338 | float lastLoggedWriterInputFov = std::numeric_limits<float>::quiet_NaN(); | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| lastLoggedCameraWorldFov | 339 | float lastLoggedCameraWorldFov = std::numeric_limits<float>::quiet_NaN(); | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| lastLoggedCameraFirstPersonFov | 340 | float lastLoggedCameraFirstPersonFov = std::numeric_limits<float>::quiet_NaN(); | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| dialogueMutex | 341 | std::mutex dialogueMutex; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| dialoguePhase | 342 | Phase dialoguePhase{Phase::Inactive}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| activeDialoguePolicy | 343 | dialogue::PolicySnapshot activeDialoguePolicy{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| dialogueCorrectionActivation | 344 | dialogue::CorrectionActivation dialogueCorrectionActivation{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| dialogueBaseline | 345 | float dialogueBaseline = std::numeric_limits<float>::quiet_NaN(); | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| dialoguePrevious | 346 | float dialoguePrevious = std::numeric_limits<float>::quiet_NaN(); | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| dialogueExitIncomingStart | 347 | float dialogueExitIncomingStart{std::numeric_limits<float>::quiet_NaN()}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| dialogueCandidate | 348 | dialogue::CandidateTracker dialogueCandidate{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |
| dialogueRecoveryRearm | 349 | dialogue::RecoveryRearm dialogueRecoveryRearm{}; | g_runtime only; resident runtime.cpp / private definition B1 | Always |

## Aliases, data shapes, constants and static/TLS storage

Alias bindings remain references to the original owner or HookSet submember, not independent state. Their eventual private visibility follows each consumer; initial B1 keeps a single binding site. Non-owner storage stays at its current lifetime and scope until its owning integration component moves. Local/TLS storage must never become a header-static copy. This table preserves exact declarations and conditions rather than inferring ownership from names.

| Line | Declaration | Source guard | Initial disposition |
|---:|---|---|---|
| 104 | plugin::RuntimeSettingsApi g_runtimeSettings; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 105 | std::mutex g_notificationMutex; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 106 | std::deque<plugin::OverlayNotification> g_pendingNotifications; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 107 | std::uint64_t g_nextNotificationId{}; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 131 | constexpr auto DialoguePhaseName = dialogue::PhaseName; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 134 | constexpr std::uintptr_t kAspectOffset = 0x254; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 135 | constexpr std::uintptr_t kFlagsOffset = 0x259; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 136 | constexpr std::uintptr_t kCameraWorldFovOffset = 0x230; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 137 | constexpr float kWideAspect = 32.0f / 9.0f; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 138 | constexpr float kNativeAspect = 16.0f / 9.0f; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 139 | constexpr std::uint8_t kCinematicStorePrefix[] = { 0xC7, 0x80, 0x54, 0x02, 0x00, 0x00 }; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 140 | constexpr std::uint8_t kCinematicOriginalImmediate[] = { 0x39, 0x8E, 0xE3, 0x3F }; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 141 | constexpr std::size_t kCinematicImmediateOffset = 6; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 142 | constexpr std::size_t kCinematicStoreInstructionLength = 10; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 143 | constexpr float kRecoveryEndpointToleranceDegrees = 0.01f; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 144 | constexpr float kRecoveryInterpolationRangeEpsilon = 0.01f; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 145 | constexpr std::size_t kDialogueBoundaryHookOffset = 9; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 146 | constexpr float kDialogueTransformEpsilon = 0.01f; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 147 | constexpr std::uint32_t kDialogueRecoveryStableSamples = 2; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 148 | constexpr std::uint32_t kDialogueCandidateStableSamples = 3; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 149 | constexpr float kDialogueContextFovJump = 5.0f; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 156 | struct RuntimeState | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 355 | RuntimeState& g_runtime = *new RuntimeState{}; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 362 | auto& g_module = g_runtime.module; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 363 | auto& g_executable = g_runtime.executable; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 364 | auto& g_workers = g_runtime.workers; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 365 | auto& g_fovWriteAddress = g_runtime.fovWriteAddress; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 366 | auto& g_hooks = g_runtime.hooks; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 367 | auto& g_hook = g_hooks.gameplay; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 368 | auto& g_logger = g_runtime.logger; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 369 | auto& g_config = g_runtime.config; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 370 | auto& g_hotkeyConfigMutex = g_runtime.hotkeyConfigMutex; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 371 | auto& g_cameraCorrectionGate = g_runtime.cameraCorrectionGate; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 372 | auto& g_gameplaySettingsEpoch = g_runtime.gameplaySettingsEpoch; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 373 | auto& g_cinematicsSettingsEpoch = g_runtime.cinematicsSettingsEpoch; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 374 | auto& g_dialogueSettingsEpoch = g_runtime.dialogueSettingsEpoch; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 375 | auto& g_activeCinematicSettingsEpoch = g_runtime.activeCinematicSettingsEpoch; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 376 | auto& g_activeDialogueSettingsEpoch = g_runtime.activeDialogueSettingsEpoch; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 377 | auto& g_hotkeyRebindCaptureActive = g_runtime.hotkeyRebindCaptureActive; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 378 | auto& g_hotkeyRebindConsumedKey = g_runtime.hotkeyRebindConsumedKey; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 379 | auto& g_gameplayRecoveryObservationAvailable = g_runtime.gameplayRecoveryObservationAvailable; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 381 | auto& g_runtimeGameplayEnabled = g_runtime.runtimeGameplayEnabled; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 382 | auto& g_gameplayEnableApplyPending = g_runtime.gameplayEnableApplyPending; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 383 | auto& g_gameplayDisableRestorePending = g_runtime.gameplayDisableRestorePending; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 384 | auto& g_gameplayEnabledTransitionDeferralLogged = g_runtime.gameplayEnabledTransitionDeferralLogged; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 386 | auto& g_runtimeGameplayMode = g_runtime.runtimeGameplayMode; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 387 | auto& g_runtimeCinematicsEnabled = g_runtime.runtimeCinematicsEnabled; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 388 | auto& g_activeCinematicsEnabled = g_runtime.activeCinematicsEnabled; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 389 | auto& g_gameplayModeTransitionPending = g_runtime.gameplayModeTransitionPending; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 390 | auto& g_gameplayModeTransitionDeferralLogged = g_runtime.gameplayModeTransitionDeferralLogged; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 391 | auto& g_gameplayModeTransitionTarget = g_runtime.gameplayModeTransitionTarget; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 392 | auto& g_runtimeCinematicPolicy = g_runtime.runtimeCinematicPolicy; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 393 | auto& g_runtimeCinematicFovMode = g_runtime.runtimeCinematicFovMode; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 394 | auto& g_activeCinematicAspectPolicy = g_runtime.activeCinematicAspectPolicy; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 395 | auto& g_activeCinematicFovMode = g_runtime.activeCinematicFovMode; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 396 | auto& g_cinematicSelectionGeneration = g_runtime.cinematicSelectionGeneration; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 397 | auto& g_cinematicSelectionValid = g_runtime.cinematicSelectionValid; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 398 | auto& g_cinematicExitIndexed = g_runtime.cinematicExitIndexed; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 399 | auto& g_cinematicExitLogProbeGeneration = g_runtime.cinematicExitLogProbeGeneration; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 401 | auto& g_cameraWriterHookGate = g_runtime.cameraWriterHookGate; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 402 | auto& g_cinematicHookGate = g_runtime.cinematicHookGate; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 403 | auto& g_stopping = g_runtime.stopping; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 404 | auto& g_runtimeDialoguePolicy = g_runtime.runtimeDialoguePolicy; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 405 | auto& g_runtimeDialogueEnabled = g_runtime.runtimeDialogueEnabled; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 406 | auto& g_runtimeOverlayLocale = g_runtime.runtimeOverlayLocale; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 407 | auto& g_runtimeOverlayLocaleAuto = g_runtime.runtimeOverlayLocaleAuto; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 408 | auto& g_runtimeOverlayAutoLocaleSynchronized = g_runtime.runtimeOverlayAutoLocaleSynchronized; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 410 | auto& g_runtimeOverlayFontSize = g_runtime.runtimeOverlayFontSize; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 411 | auto& g_cinematicLifecycleObservationAvailable = g_runtime.cinematicLifecycleObservationAvailable; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 413 | auto& g_cinematicAspectComponentAvailable = g_runtime.cinematicAspectComponentAvailable; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 415 | auto& g_dialogueBoundaryHookAvailable = g_runtime.dialogueBoundaryHookAvailable; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 417 | auto& g_runtimeInitializationComplete = g_runtime.runtimeInitializationComplete; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 418 | auto& g_dialogueNonNativeCapabilityAvailable = g_runtime.dialogueNonNativeCapabilityAvailable; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 420 | auto& g_horPlusFovTelemetryMutex = g_runtime.horPlusFovTelemetryMutex; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 421 | auto& g_configPath = g_runtime.configPath; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 422 | auto& g_state = g_runtime.state; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 423 | auto& g_lastCameraMode = g_runtime.lastCameraMode; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 424 | auto& g_transitionTraceSequence = g_runtime.transitionTraceSequence; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 425 | auto& g_coordinator = g_runtime.coordinator; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 426 | auto& g_coordinatorState = g_runtime.coordinator; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 427 | auto& g_fovObservationStore = g_runtime.fovObservationStore; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 428 | auto& g_gameplayBaselineStore = g_runtime.gameplayBaselineStore; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 429 | auto& g_cinematicWriterAssociation = g_runtime.cinematicWriterAssociation; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 430 | auto& g_cinematicRecoveryOwner = g_runtime.cinematicRecoveryOwner; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 431 | auto& g_gameplayAspectRestorationStore = g_runtime.gameplayAspectRestorationStore; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 432 | auto& g_cameraStateMutex = g_runtime.cameraStateMutex; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 433 | auto& g_cameraStateSnapshot = g_runtime.cameraStateSnapshot; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 434 | auto& g_cameraStateSnapshotValid = g_runtime.cameraStateSnapshotValid; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 435 | auto& g_neutralGameplayFovCandidate = g_runtime.neutralGameplayFovCandidate; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 436 | auto& g_neutralGameplayFovSamples = g_runtime.neutralGameplayFovSamples; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 437 | auto& g_neutralGameplayFovCandidateSource = g_runtime.neutralGameplayFovCandidateSource; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 438 | auto& g_cinematicFovApplied = g_runtime.cinematicFovApplied; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 439 | auto& g_cinematicTransformedFov = g_runtime.cinematicTransformedFov; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 440 | auto& g_exitCinematicTransformedFov = g_runtime.exitCinematicTransformedFov; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 442 | auto& g_cinematicEnterAspect = g_runtime.cinematicEnterAspect; | #ifdef HORPLUS_CINEMATIC_WRITER_TRACE_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 444 | auto& g_matchGameplayEnterNativeFov = g_runtime.matchGameplayEnterNativeFov; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 445 | auto& g_matchGameplayPreEnterNativeFov = g_runtime.matchGameplayPreEnterNativeFov; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 446 | auto& g_matchGameplayPreEnterHorPlusFov = g_runtime.matchGameplayPreEnterHorPlusFov; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 447 | auto& g_matchGameplayPreEnterAspect = g_runtime.matchGameplayPreEnterAspect; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 448 | auto& g_gameplayBaselineContinuationSource = g_runtime.gameplayBaselineContinuationSource; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 450 | auto& g_cinematicContinuationPending = g_runtime.cinematicContinuationPending; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 451 | auto& g_cinematicContinuationCarryPending = g_runtime.cinematicContinuationCarryPending; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 453 | auto& g_cinematicContinuationSource = g_runtime.cinematicContinuationSource; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 454 | auto& g_cinematicContinuationExitNativeTarget = g_runtime.cinematicContinuationExitNativeTarget; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 456 | auto& g_cinematicContinuationPreviousTarget = g_runtime.cinematicContinuationPreviousTarget; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 458 | auto& g_cinematicContinuationInterpolationActive = g_runtime.cinematicContinuationInterpolationActive; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 460 | auto& g_cinematicContinuationStartFov = g_runtime.cinematicContinuationStartFov; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 461 | auto& g_matchGameplayPreEnterPairValid = g_runtime.matchGameplayPreEnterPairValid; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 462 | auto& g_exitTargetFov = g_runtime.exitTargetFov; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 463 | auto& g_postCinematicDialogueExclusion = g_runtime.postCinematicDialogueExclusion; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 465 | auto& g_postExitTraceArmed = g_runtime.postExitTraceArmed; | #ifdef POST_EXIT_TRACE_STATE | Remain at original lifetime; bind same owner on approved consumer move |
| 466 | auto& g_postExitTraceWriterCount = g_runtime.postExitTraceWriterCount; | #ifdef POST_EXIT_TRACE_STATE | Remain at original lifetime; bind same owner on approved consumer move |
| 467 | auto& g_postExitTraceStartNs = g_runtime.postExitTraceStartNs; | #ifdef POST_EXIT_TRACE_STATE | Remain at original lifetime; bind same owner on approved consumer move |
| 468 | auto& g_postExitTraceSequence = g_runtime.postExitTraceSequence; | #ifdef POST_EXIT_TRACE_STATE | Remain at original lifetime; bind same owner on approved consumer move |
| 474 | constexpr std::uint32_t kDeferredGameplayStableSamples = 3; | #ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST | Remain at original lifetime; bind same owner on approved consumer move |
| 475 | constexpr std::int64_t kDeferredGameplayDelayNs = | #ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST | Remain at original lifetime; bind same owner on approved consumer move |
| 477 | auto& g_deferredGameplayReplayPending = g_runtime.deferredGameplayReplayPending; | #ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST | Remain at original lifetime; bind same owner on approved consumer move |
| 478 | auto& g_deferredGameplaySource = g_runtime.deferredGameplaySource; | #ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST | Remain at original lifetime; bind same owner on approved consumer move |
| 479 | auto& g_deferredGameplayStableSamples = g_runtime.deferredGameplayStableSamples; | #ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST | Remain at original lifetime; bind same owner on approved consumer move |
| 480 | auto& g_deferredGameplayLastFov = g_runtime.deferredGameplayLastFov; | #ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST | Remain at original lifetime; bind same owner on approved consumer move |
| 481 | auto& g_deferredGameplayStableSinceNs = g_runtime.deferredGameplayStableSinceNs; | #ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST | Remain at original lifetime; bind same owner on approved consumer move |
| 483 | auto& g_atomicExitHandoffPending = g_runtime.atomicExitHandoffPending; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 484 | auto& g_atomicExitHandoffSource = g_runtime.atomicExitHandoffSource; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 485 | auto& g_atomicExitHandoffPreviousFov = g_runtime.atomicExitHandoffPreviousFov; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 487 | auto& g_delayedAspectDiagnosticApplied = g_runtime.delayedAspectDiagnosticApplied; | #ifdef POST_EXIT_DELAYED_ASPECT_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 490 | auto& g_horPlusDiagnosticWriterCallbacks = g_runtime.horPlusDiagnosticWriterCallbacks; | #ifdef HORPLUS_GAMEPLAY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 491 | auto& g_horPlusDiagnosticApplications = g_runtime.horPlusDiagnosticApplications; | #ifdef HORPLUS_GAMEPLAY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 492 | auto& g_horPlusDiagnosticChanges = g_runtime.horPlusDiagnosticChanges; | #ifdef HORPLUS_GAMEPLAY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 495 | auto& g_dialogueDiscoveryActive = g_runtime.dialogueDiscoveryActive; | #ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 496 | auto& g_dialogueDiscoveryStartNs = g_runtime.dialogueDiscoveryStartNs; | #ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 497 | auto& g_dialogueDiscoverySamples = g_runtime.dialogueDiscoverySamples; | #ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 498 | auto& g_dialogueDiscoveryChanges = g_runtime.dialogueDiscoveryChanges; | #ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 499 | constexpr std::int64_t kDialogueDiscoveryDurationNs = 10'000'000'000LL; | #ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 501 | auto& g_zoomInHook = g_runtime.zoomInHook; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 502 | auto& g_zoomOutHook = g_runtime.zoomOutHook; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 503 | auto& g_zoomInHits = g_runtime.zoomInHits; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 504 | auto& g_zoomOutHits = g_runtime.zoomOutHits; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 505 | auto& g_zoomLoggedEdges = g_runtime.zoomLoggedEdges; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 506 | auto& g_snapshotZoomActive = g_runtime.snapshotZoomActive; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 507 | auto& g_snapshotZoomHold = g_runtime.snapshotZoomHold; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 509 | auto& g_postExitWriteOwnerHitCount = g_runtime.postExitWriteOwnerHitCount; | #ifdef POST_EXIT_CAMERA_FOV_WRITE_OWNER_TRACE | Remain at original lifetime; bind same owner on approved consumer move |
| 512 | auto& g_postExitProducerStoreHitCount = g_runtime.postExitProducerStoreHitCount; | #ifdef POST_EXIT_FOV_PRODUCER_STORE_TRACE | Remain at original lifetime; bind same owner on approved consumer move |
| 514 | auto& g_cinematicEnterHook = g_hooks.cinematicEnter; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 515 | auto& g_cinematicExitHook = g_hooks.cinematicExit; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 516 | auto& g_cinematicAspectStore = g_runtime.cinematicAspectStore; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 517 | auto& g_cinematicOriginalImmediate = g_runtime.cinematicOriginalImmediate; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 518 | auto& g_cinematicAspectPatched = g_runtime.cinematicAspectPatched; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 519 | auto& g_cinematicAspectStoreHook = g_hooks.cinematicAspectStore; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 520 | auto& g_dialogueBoundaryHook = g_hooks.dialogueBoundary; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 522 | auto& g_postExitGameplayObserverHook = g_hooks.postExitGameplayObserver; | #ifdef POST_EXIT_GAMEPLAY_OBSERVER_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 525 | auto& g_postExitConsumerHook = g_hooks.postExitConsumer; | #ifdef POST_EXIT_FOV_STATE_CONSUMER_TRACE | Remain at original lifetime; bind same owner on approved consumer move |
| 526 | constexpr std::uintptr_t kPostExitConsumerRva = 0x318DCD4; | #ifdef POST_EXIT_FOV_STATE_CONSUMER_TRACE | Remain at original lifetime; bind same owner on approved consumer move |
| 527 | constexpr char kPostExitConsumerGameSha256[] = | #ifdef POST_EXIT_FOV_STATE_CONSUMER_TRACE | Remain at original lifetime; bind same owner on approved consumer move |
| 531 | auto& g_postExitWriteOwnerHook = g_hooks.postExitWriteOwner; | #ifdef POST_EXIT_CAMERA_FOV_WRITE_OWNER_TRACE | Remain at original lifetime; bind same owner on approved consumer move |
| 532 | constexpr std::uintptr_t kPostExitWriteOwnerRva = 0x3DB2CE7; | #ifdef POST_EXIT_CAMERA_FOV_WRITE_OWNER_TRACE | Remain at original lifetime; bind same owner on approved consumer move |
| 533 | constexpr char kPostExitWriteOwnerGameSha256[] = | #ifdef POST_EXIT_CAMERA_FOV_WRITE_OWNER_TRACE | Remain at original lifetime; bind same owner on approved consumer move |
| 538 | auto& g_postExitProducerStoreHook = g_hooks.postExitProducerStore; | #ifdef POST_EXIT_FOV_PRODUCER_STORE_TRACE | Remain at original lifetime; bind same owner on approved consumer move |
| 539 | constexpr std::uintptr_t kPostExitProducerStoreRva = 0x32B779D; | #ifdef POST_EXIT_FOV_PRODUCER_STORE_TRACE | Remain at original lifetime; bind same owner on approved consumer move |
| 540 | constexpr char kPostExitProducerStoreGameSha256[] = | #ifdef POST_EXIT_FOV_PRODUCER_STORE_TRACE | Remain at original lifetime; bind same owner on approved consumer move |
| 543 | auto& g_lastAutoRestoreSource = g_runtime.lastAutoRestoreSource; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 544 | auto& g_lastGameplayCameraSource = g_runtime.lastGameplayCameraSource; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 545 | auto& g_lastGameplayCameraFov = g_runtime.lastGameplayCameraFov; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 546 | auto& g_lastLoggedFovSource = g_runtime.lastLoggedFovSource; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 547 | auto& g_lastLoggedWriterInputFov = g_runtime.lastLoggedWriterInputFov; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 548 | auto& g_lastLoggedCameraWorldFov = g_runtime.lastLoggedCameraWorldFov; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 549 | auto& g_lastLoggedCameraFirstPersonFov = g_runtime.lastLoggedCameraFirstPersonFov; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 550 | auto& g_dialogueMutex = g_runtime.dialogueMutex; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 551 | auto& g_dialoguePhase = g_runtime.dialoguePhase; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 552 | auto& g_activeDialoguePolicy = g_runtime.activeDialoguePolicy; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 553 | auto& g_dialogueCorrectionActivation = g_runtime.dialogueCorrectionActivation; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 554 | auto& g_dialogueBaseline = g_runtime.dialogueBaseline; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 555 | auto& g_dialoguePrevious = g_runtime.dialoguePrevious; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 556 | auto& g_dialogueExitIncomingStart = g_runtime.dialogueExitIncomingStart; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 557 | auto& g_dialogueCandidate = g_runtime.dialogueCandidate; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 558 | auto& g_dialogueRecoveryRearm = g_runtime.dialogueRecoveryRearm; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 597 | static auto* trace = new (std::nothrow) | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 782 | std::atomic_bool g_oneShotCinematicTriggerArmed{false}; | #ifdef GAMEPLAY_ONE_SHOT_CINEMATIC_TRIGGER | Remain at original lifetime; bind same owner on approved consumer move |
| 785 | std::atomic<std::uintptr_t> g_lastCameraSource{}; | #ifdef COMBINED_GAMEPLAY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 786 | std::atomic_bool g_suppressRearmAfterManualRestore{false}; | #ifdef COMBINED_GAMEPLAY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 787 | std::atomic_bool g_readyForOneShot{}; | #ifdef COMBINED_GAMEPLAY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 788 | std::atomic_bool g_readyMarkerLogged{}; | #ifdef COMBINED_GAMEPLAY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 789 | thread_local bool g_logCombinedOutputForInvocation{}; | #ifdef COMBINED_GAMEPLAY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 791 | struct CombinedOutputSnapshot | #ifdef COMBINED_GAMEPLAY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 803 | thread_local CombinedOutputSnapshot g_lastCombinedOutput{}; | #ifdef COMBINED_GAMEPLAY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 805 | struct ResolutionSnapshot | #ifdef COMBINED_GAMEPLAY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 818 | float g_lastLoggedGameplayFov = std::numeric_limits<float>::quiet_NaN(); | #ifdef FOV_SETTINGS_TRACE_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 963 | struct CinematicDiagnosticWriterState | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 988 | std::mutex g_cinematicDiagnosticMutex; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 992 | float g_lastCinematicDiagnosticAspect{std::numeric_limits<float>::quiet_NaN()}; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 994 | bool g_lastCinematicDiagnosticAspectValid{}; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 995 | bool g_lastCinematicDiagnosticFlagsValid{}; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 1263 | struct CinematicBaselineWriterPublicationState | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 1273 | static auto* state = new (std::nothrow) CinematicBaselineWriterPublicationState{}; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 1636 | struct PostExitTraceSnapshot | #if defined(POST_EXIT_RAW_TRACE_DIAGNOSTIC) &#124;&#124; defined(POST_EXIT_INTERPOLATED_FOV_PRODUCER_TRACE) | Remain at original lifetime; bind same owner on approved consumer move |
| 1655 | thread_local PostExitTraceSnapshot g_previousPostExitTrace{}; | #if defined(POST_EXIT_RAW_TRACE_DIAGNOSTIC) &#124;&#124; defined(POST_EXIT_INTERPOLATED_FOV_PRODUCER_TRACE) | Remain at original lifetime; bind same owner on approved consumer move |
| 1820 | constexpr std::uint8_t expected[] = { | #ifdef POST_EXIT_FOV_PRODUCER_STORE_TRACE | Remain at original lifetime; bind same owner on approved consumer move |
| 1879 | constexpr std::uint8_t expected[] = { | #ifdef POST_EXIT_CAMERA_FOV_WRITE_OWNER_TRACE | Remain at original lifetime; bind same owner on approved consumer move |
| 1942 | constexpr std::uint8_t expected[] = { | #ifdef POST_EXIT_FOV_STATE_CONSUMER_TRACE | Remain at original lifetime; bind same owner on approved consumer move |
| 2005 | struct EnterWriterRawObservation | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 2020 | struct EnterWriterCaptureState | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 2914 | constexpr std::size_t kMovssOffset = 4; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 2915 | constexpr std::size_t kCallOffset = 10; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 3512 | std::atomic<std::uint64_t> g_dialogueDiagnosticCallbacks{}; | #ifdef DIALOGUE_BOUNDARY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 3513 | std::atomic<std::uint64_t> g_dialogueDiagnosticReturn1{}; | #ifdef DIALOGUE_BOUNDARY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 3514 | std::atomic<std::uint64_t> g_dialogueDiagnosticReturn2{}; | #ifdef DIALOGUE_BOUNDARY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 3515 | std::atomic<std::uint64_t> g_dialogueDiagnosticReturn3{}; | #ifdef DIALOGUE_BOUNDARY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 3516 | std::atomic<std::uint64_t> g_dialogueDiagnosticReturn4{}; | #ifdef DIALOGUE_BOUNDARY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 3517 | std::atomic<std::uint64_t> g_dialogueDiagnosticReturn5{}; | #ifdef DIALOGUE_BOUNDARY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 3551 | constexpr std::uint32_t kDialogueRecoveryTraceMaxSamples = 256; | #ifdef DIALOGUE_RECOVERY_ENDPOINT_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 3553 | struct DialogueRecoveryEndpointTrace | #ifdef DIALOGUE_RECOVERY_ENDPOINT_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 3568 | thread_local DialogueRecoveryEndpointTrace g_dialogueRecoveryEndpointTrace{}; | #ifdef DIALOGUE_RECOVERY_ENDPOINT_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 3801 | constexpr std::uint8_t expected[] = { 0xFF, 0x90, 0x08, 0x06, 0x00, 0x00 }; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 3821 | struct HorPlusCinematicWriterTraceSnapshot | #ifdef HORPLUS_CINEMATIC_WRITER_TRACE_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 3840 | thread_local HorPlusCinematicWriterTraceSnapshot g_previousHorPlusCinematicWriter{}; | #ifdef HORPLUS_CINEMATIC_WRITER_TRACE_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 3841 | thread_local bool g_logHorPlusCinematicWriterInvocation{}; | #ifdef HORPLUS_CINEMATIC_WRITER_TRACE_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 3974 | struct GameplayFovSourceTraceSnapshot | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 4010 | thread_local GameplayFovSourceTraceSnapshot previous{}; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 4011 | static std::atomic<std::uint64_t> nextTraceSequence{}; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 4017 | constexpr float candidateThreshold = 0.01f; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 4018 | constexpr float cameraFovEventThreshold = 1.0f; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 4019 | constexpr float aspectEventThreshold = 0.001f; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 4316 | enum class HorPlusTelemetryOwner : std::uint8_t | #ifdef HORPLUS_FOV_STATE_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 4617 | struct RuntimeCombineDiagnosticSnapshot | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 4635 | static thread_local RuntimeCombineDiagnosticSnapshot previous{}; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 5182 | constexpr char kZoomTransitionGameSha256[] = | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 5184 | constexpr std::size_t kZoomTransitionPostLoadOffset = 13; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 5186 | struct ZoomTransitionSnapshot | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 5201 | thread_local ZoomTransitionSnapshot g_previousZoomIn{}; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 5202 | thread_local ZoomTransitionSnapshot g_previousZoomOut{}; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 5296 | constexpr std::uint8_t expected[] = { 0x0F, 0x2E, 0xC8 }; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 5460 | struct HorPlusDiagnosticSnapshot | #ifdef POST_EXIT_GAMEPLAY_OBSERVER_DIAGNOSTIC; #ifdef HORPLUS_GAMEPLAY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 5472 | thread_local HorPlusDiagnosticSnapshot g_previousHorPlusDiagnostic{}; | #ifdef POST_EXIT_GAMEPLAY_OBSERVER_DIAGNOSTIC; #ifdef HORPLUS_GAMEPLAY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 5565 | struct DialogueDiscoverySnapshot | #ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 5587 | thread_local DialogueDiscoverySnapshot g_previousDialogueDiscovery{}; | #ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 5588 | thread_local std::uint64_t g_dialogueDiscoverySeenResetGeneration{}; | #ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC | Remain at original lifetime; bind same owner on approved consumer move |
| 6506 | constexpr bool cinematicRequested = true; | Always | Remain at original lifetime; bind same owner on approved consumer move |
| 6507 | constexpr bool dialogueRequested = true; | Always | Remain at original lifetime; bind same owner on approved consumer move |

Inventory: 172 function/template definitions, 159 RuntimeState members. Full declarations and initializers, transitive calls and hook/member layout are preserved in the recoverable baseline source; alias lists above are direct references, not an exhaustive call graph.

## B4 approved boundary disposition — 2026-10-09

The user approved the review dispositions on 2026-10-09. B4 is **Complete —
with documented Remain decisions**. This addendum supersedes the original B4
destination proposals for the functions listed below without changing their
baseline source or line inventory; no production code was moved or changed.

| Responsibility / symbol | B4 disposition | Boundary note |
|---|---|---|
| `TraceDialogueBoundary` | `Remain` in `runtime.cpp` | Native XMM6/RSI capture, target read, gate/dialogue lock, XMM1 mutation, publication/wake and deferred logging order are integrated. |
| `GetDialogueBoundaryStateView` | `Remain` in `runtime.cpp` | References the single RuntimeState's Dialogue/lifecycle fields; exporting the view would expose mutable owner state across TUs. |
| `ResetDialogueRuntimeState` | `Remain` in `runtime.cpp` | Caller owns synchronization; diagnostic end/release precedes the already-extracted reset operation. Preserve each caller's lock and publication ordering. |
| `ResetPostCinematicDialogueExclusion`, `ArmPostCinematicDialogueExclusion`, `ObservePostCinematicDialogueRecovery` | `Remain` in `runtime.cpp` | Lifecycle wrappers bind the owner-held exclusion state and EXIT/recovery call sites. |
| `DialogueBoundaryStateView`, `ProcessDialogueBoundary`, `ResetDialogueBoundaryState`, `DialogueBoundaryIsActive` | `Already extracted` | Existing shared contract in `src/plugin/dialogue_boundary_transaction.hpp`; no further move. |
| Dialogue phase/candidate/rearm/exclusion policy helpers | `Already extracted` | Existing domain state and transforms in `src/dialogue/dialogue_state.*` and `dialogue_fov.*`. |
| `DialogueSourceProvenanceTrace` event/state model | `Already extracted` | Existing diagnostic-domain implementation in `src/diagnostics/dialogue_source_provenance.*`. |
| `DialogueDiscoveryNowNs`, `LogDialogueDiscoveryEnd`, `LogDialogueDiscoveryChanges`, `TraceDialogueDiscovery` | `Remain` | Optional diagnostic observer uses runtime timer/state/logger and the production callback context; no independent B4 seam. |
| `DialogueDiscoveryLoop` | `Remain` (B7) | Optional worker lifecycle is outside B4; no worker movement approved. |
| `DialogueLifecycleCapabilityAvailable` | `Already extracted` | Existing feature-capability predicate in `src/plugin/feature_status.*`. |
| `LogDialogueDiagnosticReturn`, `LogDialogueDiagnosticSummary`, `ArmDialogueRecoveryEndpointTrace`, `ObserveDialogueRecoveryEndpointTrace` | `Remain` | Compile-time diagnostic state/logging is owner- and callback-lifecycle-bound; reset summary remains with B7 orchestration. |
| `InstallDialogueBoundary` and global reset/install orchestration | `Remain` (B7) | Outside B4; B7 remains unapproved. |

V0-B evidence remains partial: named actual-adapter lifecycle, disabled-in-
flight, source-cancellation, unreadable-target, XMM1/publication-wake and
serialization cases are recorded in
`research/reports/RUNTIME_CPP_DECOMPOSITION_B0_V0_RESULTS_2026-10-08.md`.
Full native identity/invalid-input permutations, all reset call-site
interleavings and live trampoline/game behavior are not proven by that suite.
These gaps remain explicitly open and are not represented as completed tests
or full Dialogue runtime validation. They do not block the approved B4
`Remain` decision. B5 boundary review is the only authorized next step;
B5–B7 implementation remains unapproved. If B5 finds no useful independent
boundary, pause for the architecture checkpoint in the implementation plan
before considering B6–B7.

## B5 Cinematic boundary review — recommended dispositions (2026-10-10)

Read-only review: see
`B5_CINEMATIC_CALLBACK_BOUNDARY_REVIEW_2026-10-10.md`. This addendum records
recommendations only; the user has not yet approved B5 closure. No production
source was moved or changed.

| Responsibility / symbol | Recommended disposition | Boundary note |
|---|---|---|
| `GetCinematicLifecycleStateView` | `Remain` | Factory binds the existing transaction view to the sole RuntimeState and its coordinator, generation, association, recovery, continuation, and Dialogue-exclusion state. |
| `TraceCinematicEnter` / ENTER runtime adapter | `Remain` | Native low-XMM0 input/output, correction gate, accepted selection/generation, cross-domain owner state, publication and deferred diagnostics form one callback boundary. |
| `TraceCinematicExit` / EXIT runtime adapter | `Remain` | Native legacy/indexed load context, accepted/rejected dispatch, recovery-owner capture, Dialogue reset, handoff, publication and deferred logging are ordered together. |
| `ApplyCinematicAspectStore` | `Remain` | RAX target and exact RIP continuation, association/lifecycle observation, validated memory store and fail-closed behavior remain at the native hook boundary. |
| `ResolveCinematicAspect` and viewport adapters | `Remain` (adapter); pure aspect policy `Already extracted` | Resolver uses runtime viewport and accepted selection; `cinematics::ResolveAspect` and store application helpers are already extracted. |
| Selection capture/generation and baseline/transform decisions | `Already extracted` | Existing `cinematic_selection.*` and `cinematic_lifecycle_transaction.hpp`; generation is committed through the owner-bound state view. |
| Accepted ENTER/EXIT lifecycle transactions and exit-authority decision | `Already extracted` | Existing `ProcessAcceptedCinematicEnter`, `ProcessAcceptedCinematicExit`, and `cinematic_exit_authority.*`; runtime callback integration remains in place. |
| Source association, recovery-owner and continuation policy | `Already extracted` (domain); runtime wiring `Remain` | Cinematic domain stores/predicates are separate; lifecycle is jointly consumed by ENTER/EXIT and the Gameplay writer. |
| Gameplay recovery, rapid-chain and generation-continuation orchestration | `Remain` | Crosses B5 callbacks, B6 Gameplay writer and settings/lifecycle paths; no duplicate coordinator or continuation store. |
| Camera publication/freshness and Overlay wake | `Remain` | Preserve the approved B3 disposition and its shared lock/wake ordering. |
| Continuation trace model | `Already extracted`; begin/record/flush adapters `Remain` | Trace state model is in diagnostics; runtime call sites share generation lifecycle and gate-aware logger ordering. |
| `ResolveCinematicFovCallsites`, `InstallCinematicAspectComponent`, `InstallCinematicFovComponent`, `CommitCinematicComponents`, `RollbackCinematicAspectComponent`, `RollbackCinematicFovComponent` | `Remain` | Production installers share the resident HookSet, startup order and component gate; `cinematic_initialization.hpp` tests establish helper transaction behavior only, not these functions or full Initialize/install/reset orchestration. |
| `cinematics::ResolveAspectStore`, `ResolveAspect`, `ApplyAspectStore` | `Already extracted` | Bounded resolver/aspect application helpers; live signature binding and target lifetime remain unverified. |

Evidence limitations remain explicit and are not treated as completed checks:
original legacy/indexed EXIT loads and CALL relocation at the actual game site;
live aspect target binding/lifetime; complete production Initialize/install/
partial rollback/reset; arbitrary engine callback quiescence; and replay
provenance. N01/N02 are `Fixed` in the remediation report within the tested
offline scope; N03 remains **Partially mitigated**. The preceding live smoke
logs identify ASI `A7122D7439F9EDDE8601F234CB99673742992E1F7890FBE3786C71DB6F5C7771`,
not the report's final production artifact, so they are not exact-artifact
runtime evidence for that remediation. N04–N06 remain separate verification
gates. No tests/builds were rerun for this documentary review.

The B1–B5 architecture checkpoint finds that the reviewed remaining
`runtime.cpp` cinematic/dialogue/publication code is predominantly composition
root/native integration, while corresponding pure domain helpers are already
separate. This conclusion does not pre-classify B6 or B7. B6 may merit a
separately authorized boundary review of the shared Gameplay writer; B7 has no
demonstrated review target yet and is not recommended absent a concrete
maintenance need. Neither review nor implementation is authorized by this
addendum. B5 closure remains pending user approval.

## Superseding approval and B6 review addendum — 2026-10-10

This dated addendum supersedes the earlier B5 paragraph immediately above
that said closure was pending and B6 was unauthorized. The user approved B5
closure as **Complete — with documented Remain decisions**. B5 native
ENTER/EXIT/aspect adapters, cross-generation/source/recovery orchestration,
publication/wake, and production installer commit/rollback remain with the
runtime owner; cinematic domain selection/aspect/lifecycle/recovery helpers
are already extracted. See
`B5_CINEMATIC_CALLBACK_BOUNDARY_REVIEW_2026-10-10.md`.

The user authorized only the B6 Gameplay Writer boundary review. The review
report is `B6_GAMEPLAY_WRITER_BOUNDARY_REVIEW_2026-10-10.md`; classifications
below are recommendations and do not close B6 without user disposition.

| B6 responsibility | Recommended disposition | Boundary / evidence note |
|---|---|---|
| `ReplayManualTransition` native callback and displaced writer-store contract | `Remain` | Binds RSI/RBX/XMM0 context to the sole owner/gate and native trampoline; adapter tests do not execute the original `[RBX+0x30]` store. |
| Gameplay/Cinematic/recovery RuntimeState view factories | `Remain` | Reference the single owner and cross B2–B5 state; moving widens mutable owner API. |
| `GetCameraWriterEngineOperations` native memory/viewport adapters | `Remain` | Production safe-read/writeability/readback operations are native integration, not reusable policy. |
| `ProcessCameraWriter` transaction routing | `Already extracted` | Existing transaction helper coordinates owner views and injected engine operations. |
| HorPlus, AspectRecalculation, RuntimeCombine policy and readback-gated transform | `Already extracted` (policy); runtime input/I/O `Remain` | Existing transaction/domain helpers encode these decisions; viewport and actual memory operations remain bound to runtime. |
| Cinematic recovery/continuation and generation/source policy | `Already extracted` (domain); cross-callback wiring `Remain` | Recovery owner is created by B5 and consumed by B6; do not split this lifecycle or duplicate coordinator state. |
| RSI/RBX identity and ADS/source transition policy | `Already extracted` (domain); runtime observation/association `Remain` | Selected fixture coverage exists, not full live source churn. |
| Dialogue reset/activity operations and mutex binding | `Remain` (adapter); Dialogue state/domain helpers `Already extracted` | Preserve `CameraCorrectionGate → dialogueMutex` ordering and B4 boundary. |
| Publication/freshness/Overlay wake and deferred diagnostics | `Remain` (runtime adapters/call ordering); pure helpers/events `Already extracted` | Preserve B3 disposition and post-gate deferred logging. |
| Hook resolver | `Already extracted` (resolver); installation is outside B6 | Installation remains outside this review; B7 is not approved or classified. |

Recommendation: no B6 physical extraction. The remaining native callback is
composition/orchestration over one runtime owner; moving it would add
cross-TU dependencies without reducing conceptual coupling. B6 closure awaits
user disposition; B6 implementation is not approved. B7 review and
implementation remain unapproved and unreviewed.

Evidence status is separate from decomposition: N01/N02 are `Fixed` within
the remediation report's tested offline scope; N03 is `Partially mitigated`;
N04–N06 remain open verification gates. The previous live-smoke loaded ASI
SHA-256 `A7122D7439F9EDDE8601F234CB99673742992E1F7890FBE3786C71DB6F5C7771`
does not match final remediation artifact
`E349E29B855FC30C0093D2BE84882DF4F4C7C61977C35BB111EC266B1A37DC81`; exact-
artifact live validation remains open. No tests, builds, source changes, or
game runtime were performed for this review.

## Final B6 approval and B1–B6 checkpoint — 2026-10-10

The user approved the B6 classifications. B6 is now **Complete — with
documented Remain decisions**. The detailed B6 table immediately above is
final: native Gameplay writer callback, owner-bound state/engine-operation
adapters, and publication/diagnostic call ordering are `Remain`;
already-separated transaction, mode, synchronization, source/recovery policy
helpers are `Already extracted`. No B6 production extraction was performed.

**B1–B6 are complete according to their individual outcomes.** The concise
architecture checkpoint is
`RUNTIME_CPP_DECOMPOSITION_FINAL_ARCHITECTURE_CHECKPOINT_2026-10-10.md`.
Approved `Remain` decisions are final scope dispositions, not pending move
tasks. The single resident owner, correction gate, native callback contracts,
lock ordering, and centralized startup/install orchestration are preserved.

Independent evidence gates remain unchanged: N03 is `Partially mitigated`;
N04–N06 remain open; live validation using the final remediation ASI
`E349E29B855FC30C0093D2BE84882DF4F4C7C61977C35BB111EC266B1A37DC81` is
pending because the earlier live-smoke logs used a different ASI hash.
B7 closure and the B1–B7 campaign completion are recorded in the user-approved
final closure addendum below. No production, vendor, test, build, release, or
configuration files were changed; no tests, builds, or game runtime were run.

## B7 final boundary review — 2026-10-10

This addendum supersedes B7's earlier proposed destinations (for example,
`runtime_hook_installation.cpp`) as current recommendations. They remain in
the historical inventory; no such component was created. The full review is
`B7_STARTUP_INSTALL_WORKER_RESET_BOUNDARY_REVIEW_2026-10-10.md`.

| B7 responsibility / inventory | Final review classification | Ownership and ordering disposition |
|---|---|---|
| `DllMain` attach/detach entry | `Already extracted` | Already in `src/plugin/dll_entry.cpp`; starts initialization asynchronously and only signals stop on non-terminating detach. |
| `InitializeThread`, `Initialize`, startup sequencing and exception boundary | `Remain` | RuntimeState, logger/config, hook set, gates, worker owner and Overlay readiness are coordinated in one initialization transaction. N03 remains partial. |
| `CloseInitializationReadiness`, `TryInitializationFailureCleanup` | `Remain` | Bound to runtime flags, one-shot cleanup marker and reset routine; readiness closes before guarded cleanup. |
| `ResetAllRuntimeResources`, `Shutdown`, `NotifyProcessDetach` | `Remain` | Preserve stop/readiness/gate closure → worker join → hook reset/patch restoration → capability reset; failed joins retain hook resources. Loader-lock detach remains signal-only. |
| `WorkerLifecycle` implementation | `Already extracted` | Existing `worker_lifecycle.hpp/.cpp`; one instance remains owned by RuntimeState and lifecycle calls are externally serialized. |
| Runtime worker wrappers and subsystem worker loops | `Remain` | Thin wrappers bind logging and the owner-held WorkerLifecycle; loops access hotkey, Dialogue, resolution and diagnostic owner state. Overlay owns its own discovery/presenter workers. |
| `VerifyExecutableAndInstruction`, site validators, Gameplay/Cinematic resolvers | `Already extracted` (pure resolver rules); runtime image/HookSet binding `Remain` | Resolver/helper boundaries already exist; production scan and hook-site binding are not equivalent to helper tests. |
| Gameplay, Cinematic, Dialogue, Zoom and optional diagnostic hook installers | `Remain` | Callback addresses, SafetyHook members, capability flags and hook gates share the runtime owner. No new installation TU is recommended. |
| Cinematic interdependent install/commit/rollback (`InstallCinematic*Component`, `CommitCinematicComponents`, `Rollback*`, `RestoreCinematicAspect`) | `Remain` (installer adapters); `Already extracted` (transaction decision helper) | Keep hooks disabled until component commit and publish the gate only after success; failure/rollback order remains owner-bound. |
| `SafeMidHookEntry` | `Remain` | Private wrapper remains beside local hook registrations; N02 Zoom containment is `Fixed`. Moving only this template would add no current boundary. |
| SafetyHook executable patch-state/lifetime | `Already extracted` (existing vendor component; N01 remediation) | N01 `Fixed` within recorded offline tests; uncertain executable references are retained to process termination, which is not successful teardown. |
| `HotkeyLoop` | `Remain` (loop/action orchestration); pure hotkey dispatch/mapping/persistence `Already extracted` | The loop mutates owner settings/state and shares rebind flags/mutex; preserve existing B2 decisions. |
| `TryStartConfiguredOverlay` and Overlay discovery/presenter | `Already extracted` | Optional-start helper and Overlay subsystem already separate; plugin-owned readiness result and core-ready handshake remain in `Initialize`/`InitializeThread`. |
| B7 diagnostic/experimental worker and hook call sites | `Remain` | Profile-gated call sites bind runtime owner state; no cleanup or behavior changes are included in this review. |

Recommendation: B7 has no additional extraction slice with proven net
architectural benefit. Close B7 as **Complete — with documented `Remain`
decisions** and end the decomposition campaign at B1–B7 if approved. B7
implementation is not authorized. The new
`RUNTIME_CPP_DECOMPOSITION_FINAL_ARCHITECTURE_CHECKPOINT_B1_B7_2026-10-10.md`
summarizes the overall architecture and independent verification gates.

Open evidence is unchanged: N03 `Partially mitigated`; N04–N06 open; live
validation of final remediation ASI
`E349E29B855FC30C0093D2BE84882DF4F4C7C61977C35BB111EC266B1A37DC81` pending.
Prior live-smoke ASI hash mismatch remains material. No code, tests, builds,
release/configuration artifacts, ASI installation, or game runtime were
changed or executed for this review.

## User-approved B7 and campaign closure — 2026-10-10

The user approved B7 as **Complete — with documented `Remain` decisions** and
approved completion of `RUNTIME_CPP_DECOMPOSITION` across B1–B7. The B7
responsibility table above records its final classifications; this addendum
supersedes only the earlier recommendation/approval-pending wording. All
individual B1–B6 dispositions and their historical evidence remain unchanged.

The campaign is closed with no additional source splits. Approved `Remain`
decisions are final scope dispositions, not unfinished move tasks. This
architectural closure does not imply full runtime verification or change any
remediation status:

- N03 remains `Partially mitigated`.
- N04–N06 remain open verification gates.
- Live validation of final remediation ASI SHA-256
  `E349E29B855FC30C0093D2BE84882DF4F4C7C61977C35BB111EC266B1A37DC81` remains
  pending; prior live-smoke evidence used a different ASI hash.

No production/vendor code, tests, configuration, release artifacts, or other
non-documentation files were changed for this closure. No tests, builds, or
game were run.
