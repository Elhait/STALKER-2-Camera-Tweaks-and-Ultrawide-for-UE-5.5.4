#pragma once

#include "../camera/presentation_state.hpp"
#include "../camera/gameplay_aspect_restoration.hpp"
#include "../camera/gameplay_baseline.hpp"
#include "../camera/fov_observation.hpp"
#include "../camera/fov_semantics.hpp"
#include "../camera/camera_state_publication.hpp"
#include "../cinematics/cinematic_fov.hpp"
#include "../cinematics/cinematic_recovery_retirement.hpp"
#include "../dialogue/dialogue_state.hpp"
#include "camera_correction_gate.hpp"
#include "runtime_settings.hpp"
#include "../gameplay/horplus_gameplay.hpp"
#include "../gameplay/aspect_policy.hpp"
#include "../gameplay/gameplay_state.hpp"
#include "../gameplay/gameplay_camera.hpp"

#include <atomic>
#include <cstdint>
#include <limits>
#include <utility>

namespace plugin
{
    enum class CameraSettingFreshnessChange : std::uint8_t
    {
        GameplayEnabled,
        GameplayMode,
        CinematicsPolicy,
        DialoguePolicy,
    };

    inline void MarkCameraSettingFactsHistorical(
        camera::CameraStateSnapshot& snapshot, bool snapshotExists,
        CameraSettingFreshnessChange change) noexcept
    {
        using Group = camera::CameraStateFreshnessGroup;
        switch (change) {
        case CameraSettingFreshnessChange::GameplayEnabled:
            camera::MarkCameraStateGroupHistorical(snapshot, snapshotExists,
                Group::TransformedResult);
            camera::MarkCameraStateGroupHistorical(snapshot, snapshotExists,
                Group::GameplayFov);
            break;
        case CameraSettingFreshnessChange::GameplayMode:
            camera::MarkCameraStateGroupHistorical(snapshot, snapshotExists,
                Group::GameplayMode);
            camera::MarkCameraStateGroupHistorical(snapshot, snapshotExists,
                Group::TransformedResult);
            camera::MarkCameraStateGroupHistorical(snapshot, snapshotExists,
                Group::GameplayFov);
            break;
        case CameraSettingFreshnessChange::CinematicsPolicy:
            camera::MarkCameraStateGroupHistorical(snapshot, snapshotExists,
                Group::TransformedResult);
            break;
        case CameraSettingFreshnessChange::DialoguePolicy:
            camera::MarkCameraStateGroupHistorical(snapshot, snapshotExists,
                Group::Dialogue);
            camera::MarkCameraStateGroupHistorical(snapshot, snapshotExists,
                Group::TransformedResult);
            camera::MarkCameraStateGroupHistorical(snapshot, snapshotExists,
                Group::GameplayFov);
            break;
        }
    }

    struct CameraSettingStateView
    {
        std::atomic<config::GameplayMode>& gameplayMode;
        std::atomic<bool>& gameplayEnabled;
        std::atomic<std::uint64_t>& gameplayEpoch;
        std::atomic<bool>& gameplayEnablePending;
        std::atomic<bool>& gameplayDisablePending;
        std::atomic<bool>& gameplayTransitionPending;
        std::atomic<bool>& gameplayTransitionDeferralLogged;
        std::atomic<bool>& cinematicsEnabled;
        std::atomic<std::uint64_t>& cinematicsEpoch;
        std::atomic<bool>& activeCinematicsEnabled;
        std::atomic<std::uint64_t>& activeCinematicsEpoch;
        std::atomic<bool>& cinematicFovApplied;
        std::atomic<float>& cinematicTransformedFov;
        std::atomic<float>& exitCinematicTransformedFov;
        std::atomic<bool>& cinematicContinuationPending;
        std::atomic<bool>& cinematicContinuationCarryPending;
        std::atomic<bool>& cinematicContinuationInterpolationActive;
        std::atomic<std::uintptr_t>& cinematicContinuationSource;
        std::atomic<bool>& dialogueEnabled;
        std::atomic<config::DialogueZoomPolicy>& dialoguePolicy;
        std::atomic<std::uint64_t>& dialogueEpoch;
        std::atomic<std::uint64_t>& activeDialogueEpoch;
        bool dialogueNonNativeCapabilityAvailable{};
        std::atomic<config::CinematicAspectPolicy>& cinematicAspectPolicy;
        std::atomic<config::CinematicFovMode>& cinematicFovMode;
        dialogue::CorrectionActivation& dialogueCorrectionActivation;
        void* gameplayModeContext{};
        void (*selectGameplayMode)(void*, config::GameplayMode){};
        void* cameraStateFreshnessContext{};
        void (*cameraSettingChanged)(void*, CameraSettingFreshnessChange){};
    };

    struct CameraSettingTransactionResult
    {
        bool handled{};
        RuntimeMutationResult mutation{};
        bool flushCinematicTrace{};
    };

    // Shared Enabled publication transaction used by the production settings
    // API and deterministic tests. Caller supplies the same RuntimeState-owned
    // fields; non-camera settings remain on their existing path.
    template<typename FlushCinematicTrace>
    CameraSettingTransactionResult ApplyCameraSetting(
        CameraCorrectionGate& gate, CameraSettingStateView state,
        const RuntimeSettingMutation& mutation, bool gameplayHookAvailable,
        FlushCinematicTrace&& flushCinematicTrace)
    {
        const bool cameraSetting =
            mutation.kind == RuntimeSettingKind::GameplayEnabled ||
            mutation.kind == RuntimeSettingKind::GameplayMode ||
            mutation.kind == RuntimeSettingKind::CinematicsEnabled ||
            mutation.kind == RuntimeSettingKind::CinematicAspectPolicy ||
            mutation.kind == RuntimeSettingKind::CinematicFovMode ||
            mutation.kind == RuntimeSettingKind::DialogueEnabled ||
            mutation.kind == RuntimeSettingKind::DialogueZoomPolicy;
        if (!cameraSetting) return {};

        CameraSettingTransactionResult result{};
        result.handled = true;
        {
            auto correctionLock = gate.Acquire();
            const auto publishFreshnessChange = [&] {
                if (!result.mutation.changed || state.cameraSettingChanged == nullptr)
                    return;
                CameraSettingFreshnessChange change{};
                switch (mutation.kind) {
                case RuntimeSettingKind::GameplayEnabled:
                    change = CameraSettingFreshnessChange::GameplayEnabled;
                    break;
                case RuntimeSettingKind::GameplayMode:
                    change = CameraSettingFreshnessChange::GameplayMode;
                    break;
                case RuntimeSettingKind::CinematicsEnabled:
                case RuntimeSettingKind::CinematicAspectPolicy:
                case RuntimeSettingKind::CinematicFovMode:
                    change = CameraSettingFreshnessChange::CinematicsPolicy;
                    break;
                case RuntimeSettingKind::DialogueEnabled:
                case RuntimeSettingKind::DialogueZoomPolicy:
                    change = CameraSettingFreshnessChange::DialoguePolicy;
                    break;
                default:
                    return;
                }
                state.cameraSettingChanged(state.cameraStateFreshnessContext, change);
            };
            switch (mutation.kind) {
            case RuntimeSettingKind::GameplayEnabled: {
                const auto* value = std::get_if<bool>(&mutation.value);
                if (!value || (*value && !gameplayHookAvailable)) return result;
                const bool old = state.gameplayEnabled.load(std::memory_order_acquire);
                if (old == *value) {
                    result.mutation = {true, false};
                    return result;
                }
                state.gameplayEpoch.fetch_add(1, std::memory_order_acq_rel);
                state.gameplayTransitionDeferralLogged.store(false,
                    std::memory_order_release);
                if (*value) {
                    state.gameplayDisablePending.store(false, std::memory_order_release);
                    state.gameplayEnablePending.store(true, std::memory_order_release);
                    state.gameplayEnabled.store(true, std::memory_order_release);
                } else {
                    state.gameplayEnablePending.store(false, std::memory_order_release);
                    state.gameplayTransitionPending.store(false, std::memory_order_release);
                    state.gameplayTransitionDeferralLogged.store(false,
                        std::memory_order_release);
                    state.gameplayDisablePending.store(true, std::memory_order_release);
                    state.gameplayEnabled.store(false, std::memory_order_release);
                }
                result.mutation = {true, true};
                publishFreshnessChange();
                return result;
            }
            case RuntimeSettingKind::GameplayMode: {
                const auto* value = std::get_if<config::GameplayMode>(&mutation.value);
                if (!value || !state.selectGameplayMode) return result;
                const auto old = state.gameplayMode.load(std::memory_order_acquire);
                state.selectGameplayMode(state.gameplayModeContext, *value);
                const auto current = state.gameplayMode.load(std::memory_order_acquire);
                result.mutation = {true, old != current};
                publishFreshnessChange();
                return result;
            }
            case RuntimeSettingKind::CinematicsEnabled: {
                const auto* value = std::get_if<bool>(&mutation.value);
                if (!value) return result;
                const bool old = state.cinematicsEnabled.exchange(*value,
                    std::memory_order_acq_rel);
                if (old == *value) {
                    result.mutation = {true, false};
                    return result;
                }
                state.cinematicsEpoch.fetch_add(1, std::memory_order_acq_rel);
                if (!*value) {
                    state.activeCinematicsEnabled.store(false, std::memory_order_release);
                    state.activeCinematicsEpoch.store(0, std::memory_order_release);
                    state.cinematicFovApplied.store(false, std::memory_order_release);
                    state.cinematicTransformedFov.store(
                        std::numeric_limits<float>::quiet_NaN(), std::memory_order_release);
                    state.exitCinematicTransformedFov.store(
                        std::numeric_limits<float>::quiet_NaN(), std::memory_order_release);
                    state.cinematicContinuationPending.store(false,
                        std::memory_order_release);
                    state.cinematicContinuationCarryPending.store(false,
                        std::memory_order_release);
                    result.flushCinematicTrace = true;
                    std::forward<FlushCinematicTrace>(flushCinematicTrace)();
                    state.cinematicContinuationInterpolationActive.store(false,
                        std::memory_order_release);
                    state.cinematicContinuationSource.store(0, std::memory_order_release);
                }
                result.mutation = {true, true};
                publishFreshnessChange();
                return result;
            }
            case RuntimeSettingKind::DialogueEnabled: {
                const auto* value = std::get_if<bool>(&mutation.value);
                if (!value) return result;
                const bool old = state.dialogueEnabled.exchange(*value,
                    std::memory_order_acq_rel);
                if (old != *value)
                    state.dialogueEpoch.fetch_add(1, std::memory_order_acq_rel);
                state.dialogueCorrectionActivation.ModuleEnabledChanged(*value);
                if (!*value)
                    state.activeDialogueEpoch.store(0, std::memory_order_release);
                result.mutation = {true, old != *value};
                publishFreshnessChange();
                return result;
            }
            case RuntimeSettingKind::CinematicAspectPolicy: {
                const auto* value = std::get_if<config::CinematicAspectPolicy>(&mutation.value);
                if (!value) return result;
                const auto old = state.cinematicAspectPolicy.exchange(*value,
                    std::memory_order_acq_rel);
                result.mutation = {true, old != *value};
                publishFreshnessChange();
                return result;
            }
            case RuntimeSettingKind::CinematicFovMode: {
                const auto* value = std::get_if<config::CinematicFovMode>(&mutation.value);
                if (!value) return result;
                const auto old = state.cinematicFovMode.exchange(*value,
                    std::memory_order_acq_rel);
                result.mutation = {true, old != *value};
                publishFreshnessChange();
                return result;
            }
            case RuntimeSettingKind::DialogueZoomPolicy: {
                const auto* value = std::get_if<config::DialogueZoomPolicy>(&mutation.value);
                if (!value || (*value != config::DialogueZoomPolicy::Native &&
                    !state.dialogueNonNativeCapabilityAvailable)) return result;
                const auto old = state.dialoguePolicy.exchange(*value,
                    std::memory_order_acq_rel);
                result.mutation = {true, old != *value};
                publishFreshnessChange();
                return result;
            }
            default:
                break;
            }
        }
        return result;
    }

    // Non-owning view over the RuntimeState fields participating in writer
    // source observation and F4 recovery retirement. The runtime remains the
    // sole owner; scenario tests provide the same production state types.
    struct CameraWriterRecoveryStateView
    {
        std::atomic<camera::CoordinatorState>& coordinator;
        cinematics::CinematicWriterAssociation& association;
        cinematics::CinematicRecoveryOwner& recoveryOwner;
        dialogue::PostCinematicRecoveryExclusion& postCinematicExclusion;
        std::atomic<bool>& atomicExitHandoffPending;
        std::atomic<std::uintptr_t>& atomicExitHandoffSource;
        std::atomic<float>& atomicExitHandoffPreviousFov;
        std::atomic<bool>& continuationPending;
        std::atomic<bool>& continuationCarryPending;
        std::atomic<std::uintptr_t>& continuationSource;
        std::atomic<float>& continuationExitNativeTarget;
        std::atomic<float>& continuationPreviousTarget;
        std::atomic<bool>& continuationInterpolationActive;
        std::atomic<float>& continuationStartFov;
        std::atomic<std::uintptr_t>& gameplayBaselineContinuationSource;
        std::atomic<float>& exitTargetFov;
        std::atomic<float>& exitCinematicTransformedFov;
    };

    struct CameraWriterInput
    {
        std::uintptr_t sourceIdentity{};
        std::uintptr_t outputObjectIdentity{};
        float incomingFov{};
        float aspect{};
        bool aspectValid{};
        std::uint8_t flags{};
        bool flagsValid{};
        float recoveryEndpointTolerance{0.02f};
        bool gameplayEnabled{true};
        config::GameplayMode gameplayMode{config::GameplayMode::HorPlus};
        float nativeAspect{16.0f / 9.0f};
        float dialogueContextFovJump{5.0f};
        float atomicHandoffFovEpsilon{0.01f};
        float recoveryInterpolationRangeEpsilon{0.01f};
        // Independent window observation; never a Gameplay HorPlus authority.
        float currentViewportAspect{std::numeric_limits<float>::quiet_NaN()};
        bool currentViewportAspectValid{};
    };

    // Runtime-owned Cinematic state consulted by the writer transaction. It
    // contains policy state only; hook registers and engine memory stay in the
    // adapter's validated input snapshot.
    struct CameraWriterCinematicStateView
    {
        std::atomic<bool>& activeCorrectionEnabled;
        std::atomic<bool>& selectionValid;
        std::atomic<std::uint64_t>& generation;
        std::atomic<bool>& targetEstablished;
        std::atomic<float>& authoredEnterFov;
        std::atomic<float>& transformedTargetFov;
        std::atomic<std::uintptr_t>& continuationSource;
        std::atomic<float>& continuationStartFov;
        std::atomic<bool>& continuationInterpolationActive;
        cinematics::CinematicWriterAssociation& association;
    };

    struct CameraWriterFreshAspectFlags
    {
        float aspect{std::numeric_limits<float>::quiet_NaN()};
        std::uint8_t flags{};
        bool aspectReadable{};
        bool flagsReadable{};
    };

    // Narrow engine boundary for validated camera-field operations. Production
    // callbacks use the existing SafeRead/writable-memory helpers; scenario
    // tests record requests and return explicit outcomes without emulating UE.
    struct CameraWriterEngineOperations
    {
        void* context{};
        CameraWriterFreshAspectFlags (*read)(void*, std::uintptr_t, bool){};
        bool (*writeAspectOnly)(void*, std::uintptr_t, float){};
        bool (*writeAspectAndFlags)(void*, std::uintptr_t, float, std::uint8_t){};
    };

    inline bool IsValidCameraWriterAspect(float aspect) noexcept
    {
        return std::isfinite(aspect) && aspect > 0.0f;
    }

    inline bool IsValidCameraWriterFov(float fov) noexcept
    {
        return std::isfinite(fov) && fov > 1.0f && fov < 179.0f;
    }

    inline CameraWriterFreshAspectFlags ReadCameraWriterAspectFlags(
        const CameraWriterEngineOperations& operations, std::uintptr_t source,
        bool includeFlags)
    {
        return operations.read
            ? operations.read(operations.context, source, includeFlags)
            : CameraWriterFreshAspectFlags{};
    }

    inline bool RequestCameraWriterAspectOnly(
        const CameraWriterEngineOperations& operations, std::uintptr_t source,
        float aspect)
    {
        return operations.writeAspectOnly &&
            operations.writeAspectOnly(operations.context, source, aspect);
    }

    inline bool RequestCameraWriterAspectAndFlags(
        const CameraWriterEngineOperations& operations, std::uintptr_t source,
        float aspect, std::uint8_t flags)
    {
        return operations.writeAspectAndFlags &&
            operations.writeAspectAndFlags(operations.context, source, aspect, flags);
    }

    struct RuntimeCombineAspectSync
    {
        CameraWriterFreshAspectFlags fields{};
        float targetAspect{std::numeric_limits<float>::quiet_NaN()};
        bool targetValid{};
        bool mismatch{};
        bool writeAttempted{};
        bool writeSucceeded{};
        bool readbackValid{};
        bool confirmed{};
    };

    // RuntimeCombine's aspect-only synchronization is shared by ordinary
    // Gameplay and post-Cinematic recovery. HorPlus is never evaluated from
    // this target until the writer field is confirmed by read-back.
    inline RuntimeCombineAspectSync SynchronizeRuntimeCombineAspect(
        const CameraWriterEngineOperations& operations,
        std::uintptr_t source, float targetAspect,
        CameraWriterFreshAspectFlags fields)
    {
        RuntimeCombineAspectSync result{};
        result.fields = fields;
        result.targetAspect = targetAspect;
        result.targetValid = IsValidCameraWriterAspect(targetAspect);
        result.readbackValid = fields.aspectReadable && fields.flagsReadable;
        if (source == 0 || !result.targetValid || !result.readbackValid ||
            !IsValidCameraWriterAspect(fields.aspect))
            return result;

        result.mismatch = std::fabs(fields.aspect - targetAspect) > 0.001f;
        if (result.mismatch) {
            result.writeAttempted = true;
            result.writeSucceeded = RequestCameraWriterAspectOnly(
                operations, source, targetAspect);
            result.fields = ReadCameraWriterAspectFlags(operations, source, true);
            result.readbackValid = result.fields.aspectReadable &&
                result.fields.flagsReadable;
            result.confirmed = result.writeSucceeded && result.readbackValid &&
                IsValidCameraWriterAspect(result.fields.aspect) &&
                std::fabs(result.fields.aspect - targetAspect) <= 0.001f;
        } else {
            result.confirmed = std::fabs(fields.aspect - targetAspect) <= 0.001f;
        }
        return result;
    }

    struct CameraWriterDialogueResetOperation
    {
        void* context{};
        void (*reset)(void*){};
        bool (*isActive)(void*){};
    };

    enum class AtomicExitHandoffEvent : std::uint8_t
    {
        None,
        CancelledCoordinator,
        CancelledDialogue,
        NoOwner,
        ForeignSource,
        SourceMismatch,
        InvalidFov,
        FirstSample,
        CameraUnreadable,
        NotDownward,
        WriteFailed,
        Applied,
    };

    struct AtomicExitHandoffResult
    {
        AtomicExitHandoffEvent event{AtomicExitHandoffEvent::None};
        bool handled{};
        bool writeSucceeded{};
        float previousAspect{std::numeric_limits<float>::quiet_NaN()};
        std::uint8_t previousFlags{};
    };

    struct CameraWriterGameplayStateView
    {
        std::atomic<bool>& enableApplyPending;
        std::atomic<bool>& disableRestorePending;
        std::atomic<bool>& modeTransitionPending;
        std::atomic<bool>& enableTransitionDeferralLogged;
        std::atomic<bool>& modeTransitionDeferralLogged;
        std::atomic<config::GameplayMode>& modeTransitionTarget;
        std::atomic<gameplay::ReplayState>& replayState;
        std::atomic<std::uintptr_t>& lastAutoRestoreSource;
        std::atomic<std::uintptr_t>& lastGameplayCameraSource;
        std::atomic<float>& lastGameplayCameraFov;
        camera::GameplayAspectRestorationStore& aspectRestoration;
        camera::CameraFovObservationStore& fovObservation;
        camera::GameplayBaselineStore& baseline;
    };

    struct CameraWriterGameplayOperations
    {
        void* context{};
        bool (*resetDialogueIfActive)(void*, std::uintptr_t, float,
            std::uintptr_t, float){};
    };

    enum class GameplayModeTransitionEvent : std::uint8_t
    {
        None,
        DeferredUnreadable,
        ConsumedNoWrite,
        DeferredNativeNotEstablished,
        RestoreWriteFailed,
        RestoreApplied,
    };

    struct GameplayModeTransitionOutcome
    {
        GameplayModeTransitionEvent event{GameplayModeTransitionEvent::None};
        camera::GameplayAspectRestorationDecision decision{
            camera::GameplayAspectRestorationDecision::NoAction};
        float currentAspect{std::numeric_limits<float>::quiet_NaN()};
        float restorationAspect{std::numeric_limits<float>::quiet_NaN()};
        std::uint8_t flags{};
        bool cameraReadable{};
        bool writeSucceeded{};
        bool shouldLog{};
    };

    enum class GameplayEnabledTransitionEvent : std::uint8_t
    {
        None,
        Deferred,
        RestoreWriteFailed,
        RestoreApplied,
        AlreadyNative,
        AlreadyApplied,
    };

    struct GameplayEnabledTransitionOutcome
    {
        GameplayEnabledTransitionEvent event{GameplayEnabledTransitionEvent::None};
        gameplay::GameplayEnabledAction action{gameplay::GameplayEnabledAction::NoAction};
        config::GameplayMode selectedMode{config::GameplayMode::HorPlus};
        gameplay::ReplayState replayState{gameplay::ReplayState::WaitingForAutomaticUpdate};
        camera::GameplayAspectRestorationState restoration{};
        float currentAspect{std::numeric_limits<float>::quiet_NaN()};
        std::uint8_t flags{};
        bool enabled{};
        bool cameraReadable{};
        bool currentAspectValid{};
        bool restorationSourceMatches{};
        bool writeSucceeded{};
        bool stopWriter{};
        bool shouldLog{};
    };

    struct CameraWriterGameplayResult
    {
        std::uintptr_t source{};
        float inputFov{};
        float outputFov{};
        float aspect{std::numeric_limits<float>::quiet_NaN()};
        // Actual aspect passed to Gameplay HorPlus (native writer aspect).
        float policyAspect{std::numeric_limits<float>::quiet_NaN()};
        bool policyAspectValid{};
        float currentViewportAspect{std::numeric_limits<float>::quiet_NaN()};
        bool currentViewportAspectValid{};
        std::uint8_t flags{};
        bool sourceReadable{};
        bool eligible{};
        bool applied{};
        bool gameplayStateEstablished{};
        const char* reason{"NOT_ATTEMPTED"};
        bool observationPublished{};
        camera::CameraFovObservation observation{};
        camera::GameplayBaselineProjectionResult baselineProjection{};
        camera::GameplayAspectRestorationResult aspectRestoration{};
        bool aspectRestorationObserved{};
        bool handled{};
        bool enabledTransitionHandled{};
        bool dialogueContextInvalidated{};
        std::uintptr_t previousSource{};
        float previousFov{std::numeric_limits<float>::quiet_NaN()};
        bool modeTransitionPending{};
        bool enableTransitionApplied{};
        GameplayModeTransitionOutcome modeTransition{};
        GameplayEnabledTransitionOutcome enabledTransition{};
        bool nativeAspectWriteAttempted{};
        bool nativeAspectWriteSucceeded{};
        bool constrainedAspectPass{};
        bool automaticAspectRearmed{};
        bool runtimeCombineTelemetryValid{};
        float runtimeCombineAspectBefore{std::numeric_limits<float>::quiet_NaN()};
        float runtimeCombineTargetAspect{std::numeric_limits<float>::quiet_NaN()};
        float runtimeCombineAspectAfter{std::numeric_limits<float>::quiet_NaN()};
        float runtimeCombineFovInput{std::numeric_limits<float>::quiet_NaN()};
        float runtimeCombineFovOutput{std::numeric_limits<float>::quiet_NaN()};
        std::uint8_t runtimeCombineFlagsBefore{};
        std::uint8_t runtimeCombineFlagsAfter{};
        bool runtimeCombineMismatch{};
        bool runtimeCombineWriteAttempted{};
        bool runtimeCombineWriteSucceeded{};
        bool runtimeCombineReadbackValid{};
        bool runtimeCombineConfirmed{};
        bool runtimeCombineFovApplied{};
    };

    struct CameraWriterRecoveryCompletionResult
    {
        bool completed{};
        bool exclusionActive{};
        cinematics::CinematicRecoveryOwnerSnapshot owner{};
    };

    struct HorPlusRecoveryTransactionResult
    {
        gameplay::HorPlusRecoveryAction action{
            gameplay::HorPlusRecoveryAction::HoldNativePassThrough};
        float outputFov{};
        bool completed{};
        CameraWriterRecoveryCompletionResult completion{};
    };

    inline CameraWriterRecoveryCompletionResult CompleteCameraWriterRecovery(
        CameraWriterRecoveryStateView state, std::uint64_t generation);

    inline HorPlusRecoveryTransactionResult ProcessHorPlusRecovery(
        CameraWriterRecoveryStateView state,
        camera::CoordinatorState coordinator,
        gameplay::HorPlusRecoverySample sample,
        bool nativeEndpoint, float endpointTolerance, float nativeAspect);

    enum class CameraWriterContinuation : std::uint8_t
    {
        ContinueRouting,
        RecoveryWithoutOwner,
        RecoveryForeignSource,
        PostRecoveryForeignSource,
        ReturnCinematic,
    };

    // Whether the adapter must continue the existing downstream writer
    // transaction or can finish this callback using this result. The reason
    // for a final pass-through/mutation remains in `continuation` and the
    // existing policy action; this field is routing only.
    enum class CameraWriterDisposition : std::uint8_t
    {
        ContinueRouting,
        FinalOutput,
    };

    struct CameraWriterResult
    {
        bool processed{};
        cinematics::CinematicWriterAssociationSnapshot previous{};
        cinematics::CinematicWriterAssociationSnapshot current{};
        cinematics::SharedWriterPairTransition pairTransition{};
        cinematics::CinematicRecoveryOwnerSnapshot recoveryOwnerBefore{};
        cinematics::CinematicRecoveryOwnerSnapshot recoveryOwnerAfter{};
        dialogue::PostCinematicRecoveryExclusion::Observation exclusionObservation{
            dialogue::PostCinematicRecoveryExclusion::Observation::Inactive};
        bool recoveryRetired{};
        bool foreignSourceRejected{};
        bool handoffCancelled{};
        bool exclusionCleared{};
        camera::CoordinatorState coordinatorAfter{camera::CoordinatorState::Gameplay};
        float inputFov{std::numeric_limits<float>::quiet_NaN()};
        float outputFov{std::numeric_limits<float>::quiet_NaN()};
        bool mutateFov{};
        CameraWriterDisposition disposition{CameraWriterDisposition::ContinueRouting};
        CameraWriterContinuation continuation{CameraWriterContinuation::ContinueRouting};
        cinematics::CinematicWriterAction cinematicAction{
            cinematics::CinematicWriterAction::PassThroughUnclassified};
        bool cinematicHandled{};
        bool initialEstablishmentAuthorized{};
        bool continuationSourceMatch{};
        float continuationStartFov{std::numeric_limits<float>::quiet_NaN()};
        bool continuationInterpolationActive{};
        cinematics::CinematicWriterAssociationSnapshot associationAfterDecision{};
        HorPlusRecoveryTransactionResult horPlusRecovery{};
        CameraWriterGameplayResult gameplay{};
        bool horPlusRecoveryProcessed{};
        bool recoveryExclusionRearmed{};
        bool noOwnerRecoveryCleanup{};
        bool aspectRecoveryEndpoint{};
        AtomicExitHandoffResult atomicExitHandoff{};
        dialogue::PostCinematicRecoveryExclusion::Observation
            recoveryExclusionObservation{
                dialogue::PostCinematicRecoveryExclusion::Observation::Inactive};
    };

    struct GameplayWriterPolicyResult
    {
        bool dispatched{};
        gameplay::HorPlusTransformResult transform{};
    };

    inline AtomicExitHandoffResult ProcessAtomicExitHandoff(
        CameraWriterRecoveryStateView state, const CameraWriterInput& input,
        camera::CoordinatorState coordinator,
        const cinematics::CinematicRecoveryOwnerSnapshot& owner,
        const CameraWriterEngineOperations& engineOperations,
        CameraWriterDialogueResetOperation dialogueReset,
        const CameraWriterGameplayStateView* gameplayState);

    // Shared enabled/owner/mode gate and HorPlus output calculation used by
    // the production writer path and offline transaction scenarios. It does
    // not mutate RuntimeState or an ABI register; the adapter commits output.
    inline GameplayWriterPolicyResult ResolveGameplayWriterPolicy(
        bool enabled, camera::CoordinatorState coordinator,
        config::GameplayMode mode, float inputFov, float aspect,
        std::uint8_t flags, float nativeAspect)
    {
        GameplayWriterPolicyResult result{};
        result.transform.outputFov = inputFov;
        if (!enabled || coordinator != camera::CoordinatorState::Gameplay ||
            mode != config::GameplayMode::HorPlus)
            return result;
        result.dispatched = true;
        result.transform = gameplay::EvaluateHorPlus(inputFov, aspect, flags,
            nativeAspect);
        return result;
    }

    struct CameraWriterRecoveryCleanupResult
    {
        bool handoffCancelled{};
        bool exclusionCleared{};
    };

    inline CameraWriterRecoveryCleanupResult ClearCameraWriterRecoveryState(
        CameraWriterRecoveryStateView state, bool publishGameplayCoordinator = true)
    {
        CameraWriterRecoveryCleanupResult result{};
        state.recoveryOwner.Clear();
        result.handoffCancelled = state.atomicExitHandoffPending.exchange(false,
            std::memory_order_acq_rel);
        state.atomicExitHandoffSource.store(0, std::memory_order_release);
        state.atomicExitHandoffPreviousFov.store(
            std::numeric_limits<float>::quiet_NaN(), std::memory_order_release);
        result.exclusionCleared = state.postCinematicExclusion.Reset();
        state.continuationPending.store(false, std::memory_order_release);
        state.continuationCarryPending.store(false, std::memory_order_release);
        state.continuationSource.store(0, std::memory_order_release);
        state.continuationExitNativeTarget.store(
            std::numeric_limits<float>::quiet_NaN(), std::memory_order_release);
        state.continuationPreviousTarget.store(
            std::numeric_limits<float>::quiet_NaN(), std::memory_order_release);
        state.continuationInterpolationActive.store(false, std::memory_order_release);
        state.continuationStartFov.store(
            std::numeric_limits<float>::quiet_NaN(), std::memory_order_release);
        state.gameplayBaselineContinuationSource.store(0, std::memory_order_release);
        state.exitTargetFov.store(std::numeric_limits<float>::quiet_NaN(),
            std::memory_order_release);
        state.exitCinematicTransformedFov.store(
            std::numeric_limits<float>::quiet_NaN(), std::memory_order_release);
        if (publishGameplayCoordinator)
            state.coordinator.store(camera::CoordinatorState::Gameplay,
                std::memory_order_release);
        return result;
    }

    inline GameplayModeTransitionOutcome ProcessPendingHorPlusModeTransition(
        CameraWriterGameplayStateView state, const CameraWriterInput& input,
        camera::CoordinatorState coordinator,
        const CameraWriterEngineOperations& engineOperations)
    {
        GameplayModeTransitionOutcome outcome{};
        if (!state.modeTransitionPending.load(std::memory_order_acquire) ||
            state.modeTransitionTarget.load(std::memory_order_acquire) !=
                config::GameplayMode::HorPlus || !input.gameplayEnabled ||
            coordinator != camera::CoordinatorState::Gameplay)
            return outcome;

        const auto fields = ReadCameraWriterAspectFlags(engineOperations,
            input.sourceIdentity, true);
        outcome.currentAspect = fields.aspect;
        outcome.flags = fields.flags;
        outcome.cameraReadable = input.sourceIdentity != 0 &&
            fields.aspectReadable && fields.flagsReadable;
        if (!outcome.cameraReadable) {
            outcome.event = GameplayModeTransitionEvent::DeferredUnreadable;
            outcome.shouldLog = !state.modeTransitionDeferralLogged.exchange(true,
                std::memory_order_acq_rel);
            return outcome;
        }

        const auto restoration = state.aspectRestoration.Read();
        outcome.restorationAspect = restoration.aspect;
        const auto decision = camera::ResolveGameplayAspectRestorationDecision({
            true,
            true,
            input.gameplayEnabled,
            coordinator == camera::CoordinatorState::Gameplay,
            true,
            IsValidCameraWriterAspect(fields.aspect),
            gameplay::IsUltrawideAspect(fields.aspect, input.nativeAspect),
            restoration.valid,
            IsValidCameraWriterAspect(restoration.aspect),
            gameplay::IsUltrawideAspect(restoration.aspect, input.nativeAspect),
        }, restoration.aspect);
        outcome.decision = decision.decision;

        if (decision.decision == camera::GameplayAspectRestorationDecision::ConsumeNoWrite) {
            state.modeTransitionPending.store(false, std::memory_order_release);
            state.modeTransitionDeferralLogged.store(false, std::memory_order_release);
            outcome.event = GameplayModeTransitionEvent::ConsumedNoWrite;
            outcome.currentAspect = fields.aspect;
            outcome.flags = fields.flags;
            outcome.shouldLog = true;
            return outcome;
        }
        if (decision.decision != camera::GameplayAspectRestorationDecision::Restore) {
            outcome.event = GameplayModeTransitionEvent::DeferredNativeNotEstablished;
            outcome.shouldLog = !state.modeTransitionDeferralLogged.exchange(true,
                std::memory_order_acq_rel);
            return outcome;
        }

        outcome.restorationAspect = decision.restorationAspect;
        outcome.writeSucceeded = RequestCameraWriterAspectAndFlags(
            engineOperations, input.sourceIdentity, decision.restorationAspect,
            fields.flags);
        if (!outcome.writeSucceeded) {
            outcome.event = GameplayModeTransitionEvent::RestoreWriteFailed;
            outcome.shouldLog = !state.modeTransitionDeferralLogged.exchange(true,
                std::memory_order_acq_rel);
            return outcome;
        }

        state.modeTransitionPending.store(false, std::memory_order_release);
        state.modeTransitionDeferralLogged.store(false, std::memory_order_release);
        outcome.event = GameplayModeTransitionEvent::RestoreApplied;
        outcome.shouldLog = true;
        return outcome;
    }

    inline GameplayEnabledTransitionOutcome ProcessGameplayEnableTransition(
        CameraWriterGameplayStateView state, const CameraWriterInput& input,
        camera::CoordinatorState coordinator,
        const CameraWriterEngineOperations& engineOperations)
    {
        GameplayEnabledTransitionOutcome outcome{};
        outcome.enabled = input.gameplayEnabled;
        const bool pending = input.gameplayEnabled
            ? state.enableApplyPending.load(std::memory_order_acquire)
            : state.disableRestorePending.load(std::memory_order_acquire);
        if (!pending) return outcome;

        if (input.gameplayEnabled && input.gameplayMode == config::GameplayMode::HorPlus &&
            state.modeTransitionPending.load(std::memory_order_acquire))
            return outcome;

        outcome.selectedMode = input.gameplayEnabled
            ? input.gameplayMode : config::GameplayMode::HorPlus;
        outcome.replayState = state.replayState.load(std::memory_order_acquire);
        const auto fields = ReadCameraWriterAspectFlags(engineOperations,
            input.sourceIdentity, true);
        outcome.currentAspect = fields.aspect;
        outcome.flags = fields.flags;
        outcome.cameraReadable = input.sourceIdentity != 0 &&
            fields.aspectReadable && fields.flagsReadable;
        outcome.currentAspectValid = outcome.cameraReadable &&
            IsValidCameraWriterAspect(fields.aspect);
        outcome.restoration = state.aspectRestoration.Read();
        outcome.restorationSourceMatches = outcome.restoration.source.valid &&
            outcome.restoration.source.value == input.sourceIdentity;
        const bool currentUltrawide = outcome.currentAspectValid &&
            gameplay::IsUltrawideAspect(fields.aspect, input.nativeAspect);
        outcome.action = gameplay::ResolveGameplayEnabledTransition({
            input.gameplayEnabled,
            outcome.selectedMode,
            coordinator,
            outcome.replayState,
            outcome.cameraReadable,
            fields.aspect,
            outcome.currentAspectValid && currentUltrawide && fields.flags == 0x4,
            outcome.restoration.aspect,
            outcome.restorationSourceMatches,
        });

        if (outcome.action == gameplay::GameplayEnabledAction::Defer) {
            outcome.event = GameplayEnabledTransitionEvent::Deferred;
            outcome.shouldLog = !state.enableTransitionDeferralLogged.exchange(true,
                std::memory_order_acq_rel);
            outcome.stopWriter = !input.gameplayEnabled ||
                coordinator == camera::CoordinatorState::Gameplay;
            return outcome;
        }

        if (outcome.action == gameplay::GameplayEnabledAction::RestoreNativeAspect) {
            outcome.writeSucceeded = RequestCameraWriterAspectAndFlags(
                engineOperations, input.sourceIdentity, outcome.restoration.aspect,
                fields.flags);
            if (!outcome.writeSucceeded) {
                outcome.event = GameplayEnabledTransitionEvent::RestoreWriteFailed;
                outcome.shouldLog = !state.enableTransitionDeferralLogged.exchange(true,
                    std::memory_order_acq_rel);
                outcome.stopWriter = true;
                return outcome;
            }
            state.replayState.store(gameplay::ReplayState::WaitingForAutomaticUpdate,
                std::memory_order_release);
            state.lastAutoRestoreSource.store(0, std::memory_order_release);
            state.aspectRestoration.Update(outcome.restoration.aspect,
                {input.sourceIdentity, input.sourceIdentity != 0});
            state.enableTransitionDeferralLogged.store(false, std::memory_order_release);
            if (!input.gameplayEnabled)
                state.disableRestorePending.store(false, std::memory_order_release);
            outcome.event = GameplayEnabledTransitionEvent::RestoreApplied;
            outcome.writeSucceeded = true;
            outcome.shouldLog = true;
            outcome.stopWriter = !input.gameplayEnabled;
            return outcome;
        }

        if (!input.gameplayEnabled) {
            state.disableRestorePending.store(false, std::memory_order_release);
            if (outcome.replayState != gameplay::ReplayState::WaitingForAutomaticUpdate)
                state.replayState.store(gameplay::ReplayState::WaitingForAutomaticUpdate,
                    std::memory_order_release);
            if (outcome.currentAspectValid)
                state.aspectRestoration.Update(fields.aspect,
                    {input.sourceIdentity, input.sourceIdentity != 0});
            state.enableTransitionDeferralLogged.store(false, std::memory_order_release);
            outcome.event = GameplayEnabledTransitionEvent::AlreadyNative;
            outcome.shouldLog = true;
            outcome.stopWriter = true;
            return outcome;
        }

        if (outcome.action == gameplay::GameplayEnabledAction::ApplyHorPlus &&
            outcome.replayState != gameplay::ReplayState::WaitingForAutomaticUpdate &&
            currentUltrawide)
            state.replayState.store(gameplay::ReplayState::WaitingForAutomaticUpdate,
                std::memory_order_release);

        if (outcome.action == gameplay::GameplayEnabledAction::AlreadyApplied) {
            state.enableApplyPending.store(false, std::memory_order_release);
            state.enableTransitionDeferralLogged.store(false, std::memory_order_release);
            outcome.event = GameplayEnabledTransitionEvent::AlreadyApplied;
            outcome.shouldLog = true;
        }
        return outcome;
    }

    inline CameraWriterGameplayResult ProcessGameplayHorPlusWriter(
        CameraWriterGameplayStateView state,
        const CameraWriterInput& input,
        camera::CoordinatorState coordinator,
        const CameraWriterEngineOperations& engineOperations,
        CameraWriterGameplayOperations operations)
    {
        CameraWriterGameplayResult result{};
        result.source = input.sourceIdentity;
        result.inputFov = input.incomingFov;
        result.outputFov = input.incomingFov;
        result.currentViewportAspect = input.currentViewportAspect;
        result.currentViewportAspectValid = input.currentViewportAspectValid &&
            IsValidCameraWriterAspect(input.currentViewportAspect);
        result.gameplayStateEstablished = coordinator == camera::CoordinatorState::Gameplay;

        result.enabledTransition = ProcessGameplayEnableTransition(state, input,
            coordinator, engineOperations);
        if (result.enabledTransition.stopWriter) {
            result.handled = true;
            result.enabledTransitionHandled = true;
            result.reason = "ENABLED_TRANSITION";
            return result;
        }

        if (!input.gameplayEnabled) {
            // Enabled is terminal for correction policy. This remaining
            // aspect observation is the existing OFF cleanup behavior.
            if (coordinator == camera::CoordinatorState::Gameplay) {
                const auto fields = ReadCameraWriterAspectFlags(engineOperations,
                    input.sourceIdentity, false);
                if (fields.aspectReadable && std::isfinite(fields.aspect) &&
                    fields.aspect > 0.0f) {
                    result.aspectRestoration = state.aspectRestoration.Update(
                        fields.aspect, {input.sourceIdentity,
                            input.sourceIdentity != 0});
                    result.aspectRestorationObserved = true;
                }
            }
            result.handled = true;
            result.reason = "GAMEPLAY_DISABLED";
            return result;
        }

        const bool runtimeCombine = input.gameplayMode ==
            config::GameplayMode::RuntimeCombine;
        if (input.gameplayMode != config::GameplayMode::HorPlus && !runtimeCombine)
            return result;

        // The production hook retries a pending AspectRecalculation ->
        // HorPlus transition before entering the ordinary writer read/policy
        // path. Keep that ordering, including on a non-Gameplay coordinator
        // where the operation itself will defer without mutation.
        if (!runtimeCombine)
            result.modeTransition = ProcessPendingHorPlusModeTransition(state, input,
                coordinator, engineOperations);
        result.modeTransitionPending = state.modeTransitionPending.load(
            std::memory_order_acquire);

        result.handled = true;
        if (coordinator != camera::CoordinatorState::Gameplay) {
            result.reason = "COORDINATOR_GUARD";
            return result;
        }

        CameraWriterFreshAspectFlags fields{};
        fields = ReadCameraWriterAspectFlags(engineOperations,
            input.sourceIdentity, true);
        if (input.sourceIdentity == 0 || !fields.aspectReadable || !fields.flagsReadable) {
            result.reason = "SOURCE_READ_FAILED";
            return result;
        }
        result.sourceReadable = true;
        result.aspect = fields.aspect;
        result.flags = fields.flags;

        if (runtimeCombine) {
            const auto sync = SynchronizeRuntimeCombineAspect(engineOperations,
                input.sourceIdentity, result.currentViewportAspect, fields);
            result.runtimeCombineTelemetryValid = true;
            result.runtimeCombineAspectBefore = fields.aspect;
            result.runtimeCombineTargetAspect = result.currentViewportAspect;
            result.runtimeCombineFlagsBefore = fields.flags;
            result.runtimeCombineFovInput = input.incomingFov;
            result.runtimeCombineAspectAfter = sync.fields.aspect;
            result.runtimeCombineFlagsAfter = sync.fields.flags;
            result.runtimeCombineMismatch = sync.mismatch;
            result.runtimeCombineWriteAttempted = sync.writeAttempted;
            result.runtimeCombineWriteSucceeded = sync.writeSucceeded;
            result.runtimeCombineReadbackValid = sync.readbackValid;
            result.runtimeCombineConfirmed = sync.confirmed;
            fields = sync.fields;
            if (fields.aspectReadable) result.aspect = fields.aspect;
            if (fields.flagsReadable) result.flags = fields.flags;
        }

        result.previousSource = state.lastGameplayCameraSource.exchange(
            input.sourceIdentity, std::memory_order_acq_rel);
        result.previousFov = state.lastGameplayCameraFov.exchange(
            input.incomingFov, std::memory_order_acq_rel);
        const auto contextChange = gameplay::EvaluateGameplayContextChange(
            result.previousSource, result.previousFov, input.sourceIdentity,
            input.incomingFov, input.dialogueContextFovJump);
        if (contextChange.InvalidatesDialogue() && operations.resetDialogueIfActive)
            result.dialogueContextInvalidated = operations.resetDialogueIfActive(
                operations.context, result.previousSource, result.previousFov,
                input.sourceIdentity, input.incomingFov);

        if (std::isfinite(input.incomingFov) && input.incomingFov > 1.0f &&
            input.incomingFov < 179.0f && std::isfinite(fields.aspect) &&
            fields.aspect > 0.0f) {
            result.aspectRestoration = state.aspectRestoration.Update(fields.aspect,
                {input.sourceIdentity, input.sourceIdentity != 0});
            result.aspectRestorationObserved = true;
        }

        // Existing HorPlus retains native-writer-aspect authority. RuntimeCombine
        // is an isolated candidate: it uses the validated viewport target only
        // after an aspect-only write and fresh read-back confirm that target.
        result.policyAspect = runtimeCombine
            ? (result.runtimeCombineConfirmed
                ? result.runtimeCombineTargetAspect
                : std::numeric_limits<float>::quiet_NaN())
            : result.aspect;
        const bool policyAspectValid = result.sourceReadable &&
            IsValidCameraWriterAspect(result.policyAspect) &&
            (!runtimeCombine || result.runtimeCombineConfirmed);
        result.policyAspectValid = policyAspectValid;
        const auto policy = ResolveGameplayWriterPolicy(true, coordinator,
            runtimeCombine ? config::GameplayMode::HorPlus : input.gameplayMode,
            input.incomingFov,
            policyAspectValid ? result.policyAspect
                : std::numeric_limits<float>::quiet_NaN(),
            result.flags, input.nativeAspect);
        result.eligible = policy.transform.eligible;
        result.applied = policy.transform.applied;
        result.outputFov = policy.transform.outputFov;
        result.reason = runtimeCombine && !result.runtimeCombineConfirmed
            ? "RUNTIME_ASPECT_UNCONFIRMED"
            : (result.applied ? "GAMEPLAY_FOV_CHANGED" :
                (result.eligible ? "TRANSFORM_RESULT_REJECTED" : "INPUT_BYPASS"));
        result.runtimeCombineFovOutput = result.outputFov;
        result.runtimeCombineFovApplied = result.applied;

        if (std::isfinite(result.inputFov) && result.inputFov > 1.0f &&
            result.inputFov < 179.0f && std::isfinite(result.outputFov) &&
            result.outputFov > 1.0f && result.outputFov < 179.0f &&
            std::isfinite(result.aspect) && result.aspect > 0.0f) {
            camera::CameraFovObservation observation{};
            observation.boundary = camera::FovObservationBoundary::CameraWriter;
            observation.inputFov = {result.inputFov, camera::FovSpace::Native,
                camera::FovProvenance::NativeRegisterInput, true};
            observation.resultFov = {result.outputFov,
                result.applied ? camera::FovSpace::Transformed : camera::FovSpace::Native,
                result.applied ? camera::FovProvenance::ModTransformResult :
                    camera::FovProvenance::PassThroughResult,
                true};
            observation.aspect = {result.aspect,
                camera::FovProvenance::ResolvedAspect, true};
            observation.policyAspect = {result.policyAspect,
                !policyAspectValid ? camera::FovProvenance::Unavailable
                    : runtimeCombine ? camera::FovProvenance::ClientViewportAspect
                        : camera::FovProvenance::NativeWriterAspect,
                policyAspectValid};
            observation.currentViewportAspect = {input.currentViewportAspect,
                input.currentViewportAspectValid
                    ? camera::FovProvenance::ClientViewportAspect
                    : camera::FovProvenance::Unavailable,
                input.currentViewportAspectValid &&
                    IsValidCameraWriterAspect(input.currentViewportAspect)};
            observation.writerSource = {result.source, result.source != 0};
            observation.writerFlags = result.flags;
            observation.writerFlagsValid = true;
            observation.observedDuringGameplay =
                coordinator == camera::CoordinatorState::Gameplay;
            result.observation = state.fovObservation.Publish(observation);
            result.observationPublished = true;
            result.baselineProjection = state.baseline.Project(
                result.observation, coordinator == camera::CoordinatorState::Gameplay);
        }

        result.modeTransitionPending = state.modeTransitionPending.load(
            std::memory_order_acquire);
        if (!result.modeTransitionPending && state.enableApplyPending.load(
                std::memory_order_acquire) &&
            coordinator == camera::CoordinatorState::Gameplay &&
            result.sourceReadable && std::isfinite(result.aspect) &&
            result.aspect > 0.0f && std::isfinite(result.inputFov) &&
            result.inputFov > 1.0f && result.inputFov < 179.0f) {
            state.enableApplyPending.store(false, std::memory_order_release);
            state.enableTransitionDeferralLogged.store(false, std::memory_order_release);
            result.enableTransitionApplied = true;
        }
        return result;
    }

    inline CameraWriterGameplayResult ProcessGameplayAspectRecalculationWriter(
        CameraWriterGameplayStateView state, const CameraWriterInput& input,
        camera::CoordinatorState coordinator,
        const CameraWriterEngineOperations& engineOperations,
        CameraWriterGameplayOperations operations)
    {
        CameraWriterGameplayResult result{};
        result.source = input.sourceIdentity;
        result.inputFov = input.incomingFov;
        result.outputFov = input.incomingFov;
        result.gameplayStateEstablished = coordinator == camera::CoordinatorState::Gameplay;
        result.enabledTransition = ProcessGameplayEnableTransition(state, input,
            coordinator, engineOperations);
        if (result.enabledTransition.stopWriter) {
            result.handled = true;
            result.enabledTransitionHandled = true;
            result.reason = "ENABLED_TRANSITION";
            return result;
        }
        if (!input.gameplayEnabled) {
            result.handled = true;
            result.reason = "GAMEPLAY_DISABLED";
            if (coordinator == camera::CoordinatorState::Gameplay) {
                const auto fields = ReadCameraWriterAspectFlags(engineOperations,
                    input.sourceIdentity, false);
                if (fields.aspectReadable && IsValidCameraWriterAspect(fields.aspect)) {
                    result.aspectRestoration = state.aspectRestoration.Update(
                        fields.aspect, {input.sourceIdentity,
                            input.sourceIdentity != 0});
                    result.aspectRestorationObserved = true;
                }
            }
            return result;
        }
        if (coordinator != camera::CoordinatorState::Gameplay) {
            result.handled = true;
            result.reason = "COORDINATOR_GUARD";
            return result;
        }

        const auto fields = ReadCameraWriterAspectFlags(engineOperations,
            input.sourceIdentity, true);
        if (input.sourceIdentity == 0 || !fields.aspectReadable || !fields.flagsReadable) {
            result.handled = true;
            result.reason = "SOURCE_READ_FAILED";
            return result;
        }
        result.handled = true;
        result.sourceReadable = true;
        result.aspect = fields.aspect;
        result.flags = fields.flags;
        result.previousSource = state.lastGameplayCameraSource.exchange(
            input.sourceIdentity, std::memory_order_acq_rel);
        result.previousFov = state.lastGameplayCameraFov.exchange(
            input.incomingFov, std::memory_order_acq_rel);
        const auto contextChange = gameplay::EvaluateGameplayContextChange(
            result.previousSource, result.previousFov, input.sourceIdentity,
            input.incomingFov, input.dialogueContextFovJump);
        if (contextChange.InvalidatesDialogue() && operations.resetDialogueIfActive)
            result.dialogueContextInvalidated = operations.resetDialogueIfActive(
                operations.context, result.previousSource, result.previousFov,
                input.sourceIdentity, input.incomingFov);

        const auto replayState = state.replayState.load(std::memory_order_acquire);
        const bool ownAutoRestore = replayState == gameplay::ReplayState::Complete &&
            std::fabs(fields.aspect - input.nativeAspect) <= 0.001f &&
            state.lastAutoRestoreSource.load(std::memory_order_acquire) ==
                input.sourceIdentity;
        const bool pendingOwnAutoRestore = replayState ==
                gameplay::ReplayState::AppliedConstrainPass &&
            std::fabs(fields.aspect - input.nativeAspect) <= 0.001f;
        if (!ownAutoRestore && !pendingOwnAutoRestore &&
            IsValidCameraWriterAspect(fields.aspect)) {
            result.aspectRestoration = state.aspectRestoration.Update(fields.aspect,
                {input.sourceIdentity, input.sourceIdentity != 0});
            result.aspectRestorationObserved = true;
        }

        if (IsValidCameraWriterFov(input.incomingFov) &&
            IsValidCameraWriterAspect(fields.aspect) && !ownAutoRestore &&
            !pendingOwnAutoRestore) {
            camera::CameraFovObservation observation{};
            observation.boundary = camera::FovObservationBoundary::CameraWriter;
            observation.inputFov = {input.incomingFov, camera::FovSpace::Native,
                camera::FovProvenance::NativeRegisterInput, true};
            observation.resultFov = {input.incomingFov, camera::FovSpace::Native,
                camera::FovProvenance::PassThroughResult, true};
            observation.aspect = {fields.aspect,
                camera::FovProvenance::ResolvedAspect, true};
            observation.writerSource = {input.sourceIdentity, true};
            observation.writerFlags = fields.flags;
            observation.writerFlagsValid = true;
            observation.observedDuringGameplay =
                coordinator == camera::CoordinatorState::Gameplay;
            result.observation = state.fovObservation.Publish(observation);
            result.observationPublished = true;
            result.baselineProjection = state.baseline.Project(result.observation, true);
        }

        if (gameplay::IsConstrainedUltrawideAspect(fields.aspect, fields.flags,
                input.nativeAspect)) {
            result.constrainedAspectPass = true;
            result.nativeAspectWriteAttempted = true;
            result.nativeAspectWriteSucceeded = RequestCameraWriterAspectAndFlags(
                engineOperations, input.sourceIdentity, input.nativeAspect, 0x5);
            result.reason = result.nativeAspectWriteSucceeded
                ? "CONSTRAINED_ASPECT_NORMALIZED" : "CONSTRAINED_ASPECT_WRITE_FAILED";
            return result;
        }

        auto currentReplayState = state.replayState.load(std::memory_order_acquire);
        if (currentReplayState == gameplay::ReplayState::Complete &&
            gameplay::IsUltrawideAspect(fields.aspect, input.nativeAspect) &&
            fields.flags == 0x4) {
            if (operations.resetDialogueIfActive)
                static_cast<void>(operations.resetDialogueIfActive(operations.context,
                    result.previousSource, result.previousFov,
                    input.sourceIdentity, input.incomingFov));
            state.replayState.store(gameplay::ReplayState::WaitingForAutomaticUpdate,
                std::memory_order_release);
            currentReplayState = gameplay::ReplayState::WaitingForAutomaticUpdate;
            result.automaticAspectRearmed = true;
        }

        if (currentReplayState == gameplay::ReplayState::WaitingForAutomaticUpdate) {
            if (!gameplay::IsUltrawideAspect(fields.aspect, input.nativeAspect) ||
                fields.flags != 0x4) {
                result.reason = "WAITING_FOR_AUTOMATIC_UPDATE";
                return result;
            }
            result.nativeAspectWriteAttempted = true;
            result.nativeAspectWriteSucceeded = RequestCameraWriterAspectAndFlags(
                engineOperations, input.sourceIdentity, input.nativeAspect, 0x4);
            if (result.nativeAspectWriteSucceeded) {
                state.replayState.store(gameplay::ReplayState::Complete,
                    std::memory_order_release);
                state.lastAutoRestoreSource.store(input.sourceIdentity,
                    std::memory_order_release);
                if (state.enableApplyPending.load(std::memory_order_acquire)) {
                    state.enableApplyPending.store(false, std::memory_order_release);
                    state.enableTransitionDeferralLogged.store(false,
                        std::memory_order_release);
                    result.enableTransitionApplied = true;
                }
                result.reason = "ATOMIC_NATIVE_ASPECT_RESTORED";
            } else {
                result.reason = "ATOMIC_NATIVE_ASPECT_WRITE_FAILED";
            }
            return result;
        }
        result.reason = "ASPECT_RECALCULATION_OBSERVATION";
        return result;
    }

    // Shared production transaction for writer observation, F4 retirement,
    // recovery routing, Cinematic writer policy, and Gameplay HorPlus dispatch.
    // Engine-dependent non-HorPlus effects remain in the runtime adapter for
    // later migration stages.
    inline CameraWriterResult ProcessCameraWriter(
        const CameraCorrectionGate& gate,
        CameraWriterRecoveryStateView state,
        const CameraWriterInput& input,
        CameraWriterCinematicStateView cinematicState,
        CameraWriterEngineOperations engineOperations = {},
        CameraWriterDialogueResetOperation dialogueReset = {},
        const CameraWriterGameplayStateView* gameplayState = nullptr,
        CameraWriterGameplayOperations gameplayOperations = {})
    {
        CameraWriterResult result{};
        result.inputFov = input.incomingFov;
        result.outputFov = input.incomingFov;
        if (!gate.IsHeldByCurrentThread()) return result;
        auto& observation = result;
        observation.processed = true;
        observation.coordinatorAfter = state.coordinator.load(std::memory_order_acquire);
        observation.previous = state.association.Read();
        observation.current = state.association.ObserveWriter(observation.coordinatorAfter,
            input.sourceIdentity, input.outputObjectIdentity, input.incomingFov, input.aspect,
            input.aspectValid, input.flags, input.flagsValid,
            &observation.pairTransition);
        observation.recoveryOwnerBefore = state.recoveryOwner.Read();

        observation.recoveryRetired = cinematics::RetireRecoveryForWriterRsiChange(
            observation.coordinatorAfter, observation.recoveryOwnerBefore,
            observation.pairTransition, [&] {
                const auto cleanup = ClearCameraWriterRecoveryState(state);
                observation.handoffCancelled = cleanup.handoffCancelled;
                observation.exclusionCleared = cleanup.exclusionCleared;
            });

        if (observation.recoveryRetired)
            observation.coordinatorAfter = camera::CoordinatorState::Gameplay;
        else if (cinematics::ShouldRejectPostRecoveryForeignWriter(
            observation.recoveryOwnerBefore, state.postCinematicExclusion.IsActive(),
            input.sourceIdentity))
            observation.foreignSourceRejected = true;

        if (!observation.foreignSourceRejected) {
            observation.exclusionObservation = state.postCinematicExclusion.ObserveValidated(
                input.sourceIdentity, input.incomingFov, input.recoveryEndpointTolerance);
            observation.exclusionCleared = observation.exclusionObservation ==
                    dialogue::PostCinematicRecoveryExclusion::Observation::Cancelled ||
                observation.exclusionObservation ==
                    dialogue::PostCinematicRecoveryExclusion::Observation::Recovered;
            if (observation.recoveryOwnerBefore.valid &&
                observation.recoveryOwnerBefore.recoveryComplete &&
                observation.exclusionObservation !=
                    dialogue::PostCinematicRecoveryExclusion::Observation::Suppressed)
                state.recoveryOwner.Clear();
        }

        observation.recoveryOwnerAfter = state.recoveryOwner.Read();
        observation.coordinatorAfter = state.coordinator.load(std::memory_order_acquire);

        // A foreign source rejected by the existing post-recovery gate remains
        // native and cannot enter Cinematic or Gameplay policy on this sample.
        if (observation.foreignSourceRejected) {
            result.disposition = CameraWriterDisposition::FinalOutput;
            result.continuation = CameraWriterContinuation::PostRecoveryForeignSource;
            return result;
        }

        if (observation.coordinatorAfter == camera::CoordinatorState::CinematicExiting) {
            if (!observation.recoveryOwnerAfter.HasRecoveryAuthority()) {
                const auto cleanup = ClearCameraWriterRecoveryState(state, false);
                observation.handoffCancelled = cleanup.handoffCancelled;
                observation.exclusionCleared = cleanup.exclusionCleared;
                if (dialogueReset.reset)
                    dialogueReset.reset(dialogueReset.context);
                state.coordinator.store(camera::CoordinatorState::Gameplay,
                    std::memory_order_release);
                observation.recoveryOwnerAfter = state.recoveryOwner.Read();
                observation.coordinatorAfter = camera::CoordinatorState::Gameplay;
                observation.noOwnerRecoveryCleanup = true;
                result.disposition = CameraWriterDisposition::FinalOutput;
                result.continuation = CameraWriterContinuation::RecoveryWithoutOwner;
                return result;
            }
            if (observation.recoveryOwnerAfter.writerRsi != input.sourceIdentity) {
                result.disposition = CameraWriterDisposition::FinalOutput;
                result.continuation = CameraWriterContinuation::RecoveryForeignSource;
                return result;
            }

            result.atomicExitHandoff = ProcessAtomicExitHandoff(state, input,
                observation.coordinatorAfter, observation.recoveryOwnerAfter,
                engineOperations, dialogueReset, gameplayState);
            if (result.atomicExitHandoff.handled) {
                result.disposition = CameraWriterDisposition::FinalOutput;
                result.continuation = CameraWriterContinuation::ReturnCinematic;
                return result;
            }

            if (input.gameplayMode == config::GameplayMode::HorPlus) {
                result.horPlusRecoveryProcessed = true;
                gameplay::HorPlusRecoverySample recoverySample{};
                recoverySample.source = input.sourceIdentity;
                recoverySample.validatedSource = observation.recoveryOwnerAfter.writerRsi;
                recoverySample.inputFov = input.incomingFov;
                recoverySample.exitNativeTarget = state.exitTargetFov.load(
                    std::memory_order_acquire);
                recoverySample.cachedCinematicFov = state.exitCinematicTransformedFov.load(
                    std::memory_order_acquire);
                recoverySample.gameplayEnabled = input.gameplayEnabled;
                const auto fields = ReadCameraWriterAspectFlags(engineOperations,
                    input.sourceIdentity, true);
                recoverySample.aspect = fields.aspect;
                recoverySample.policyAspect = fields.aspect;
                recoverySample.policyAspectValid = fields.aspectReadable &&
                    IsValidCameraWriterAspect(fields.aspect);
                recoverySample.flags = fields.flags;
                recoverySample.cameraReadable = fields.aspectReadable &&
                    fields.flagsReadable;
                result.horPlusRecovery = ProcessHorPlusRecovery(state,
                    observation.coordinatorAfter, recoverySample,
                    gameplay::IsNativeHorPlusRecoverySample(recoverySample,
                        input.recoveryEndpointTolerance),
                    input.recoveryEndpointTolerance, input.nativeAspect);
                if (result.horPlusRecovery.action ==
                        gameplay::HorPlusRecoveryAction::TransformRecoveryInterpolation) {
                    result.outputFov = result.horPlusRecovery.outputFov;
                    result.mutateFov = true;
                    result.disposition = CameraWriterDisposition::FinalOutput;
                } else if (result.horPlusRecovery.action ==
                        gameplay::HorPlusRecoveryAction::HoldNativePassThrough) {
                    if (observation.exclusionObservation ==
                            dialogue::PostCinematicRecoveryExclusion::Observation::Cancelled ||
                        observation.exclusionObservation ==
                            dialogue::PostCinematicRecoveryExclusion::Observation::Inactive) {
                        state.postCinematicExclusion.Arm(recoverySample.exitNativeTarget);
                        result.recoveryExclusionRearmed = true;
                        result.recoveryExclusionObservation =
                            state.postCinematicExclusion.ObserveValidated(
                                input.sourceIdentity, input.incomingFov,
                                input.recoveryEndpointTolerance);
                    }
                    result.disposition = CameraWriterDisposition::FinalOutput;
                } else if (result.horPlusRecovery.action ==
                        gameplay::HorPlusRecoveryAction::ResumeGameplay) {
                    result.coordinatorAfter = state.coordinator.load(
                        std::memory_order_acquire);
                }
                result.recoveryOwnerAfter = state.recoveryOwner.Read();
                if (result.horPlusRecovery.action !=
                        gameplay::HorPlusRecoveryAction::ResumeGameplay)
                    return result;
            } else if (input.gameplayMode == config::GameplayMode::RuntimeCombine &&
                input.gameplayEnabled) {
                result.horPlusRecoveryProcessed = true;
                const auto fields = ReadCameraWriterAspectFlags(engineOperations,
                    input.sourceIdentity, true);
                const auto sync = SynchronizeRuntimeCombineAspect(engineOperations,
                    input.sourceIdentity,
                    input.currentViewportAspectValid
                        ? input.currentViewportAspect
                        : std::numeric_limits<float>::quiet_NaN(), fields);
                auto& telemetry = result.gameplay;
                telemetry.source = input.sourceIdentity;
                telemetry.inputFov = input.incomingFov;
                telemetry.outputFov = input.incomingFov;
                telemetry.aspect = sync.fields.aspect;
                telemetry.policyAspect = sync.confirmed
                    ? sync.targetAspect : std::numeric_limits<float>::quiet_NaN();
                telemetry.policyAspectValid = sync.confirmed;
                telemetry.currentViewportAspect = input.currentViewportAspect;
                telemetry.currentViewportAspectValid = sync.targetValid;
                telemetry.flags = sync.fields.flags;
                telemetry.sourceReadable = sync.readbackValid &&
                    IsValidCameraWriterAspect(sync.fields.aspect);
                telemetry.handled = true;
                telemetry.runtimeCombineTelemetryValid = true;
                telemetry.runtimeCombineAspectBefore = fields.aspect;
                telemetry.runtimeCombineTargetAspect = sync.targetAspect;
                telemetry.runtimeCombineAspectAfter = sync.fields.aspect;
                telemetry.runtimeCombineFlagsBefore = fields.flags;
                telemetry.runtimeCombineFlagsAfter = sync.fields.flags;
                telemetry.runtimeCombineMismatch = sync.mismatch;
                telemetry.runtimeCombineWriteAttempted = sync.writeAttempted;
                telemetry.runtimeCombineWriteSucceeded = sync.writeSucceeded;
                telemetry.runtimeCombineReadbackValid = sync.readbackValid;
                telemetry.runtimeCombineConfirmed = sync.confirmed;
                telemetry.runtimeCombineFovInput = input.incomingFov;

                gameplay::HorPlusRecoverySample recoverySample{};
                recoverySample.source = input.sourceIdentity;
                recoverySample.validatedSource = observation.recoveryOwnerAfter.writerRsi;
                recoverySample.inputFov = input.incomingFov;
                recoverySample.exitNativeTarget = state.exitTargetFov.load(
                    std::memory_order_acquire);
                recoverySample.cachedCinematicFov =
                    state.exitCinematicTransformedFov.load(std::memory_order_acquire);
                recoverySample.aspect = sync.fields.aspect;
                recoverySample.policyAspect = telemetry.policyAspect;
                recoverySample.policyAspectValid = sync.confirmed;
                recoverySample.flags = sync.fields.flags;
                recoverySample.cameraReadable = sync.readbackValid &&
                    IsValidCameraWriterAspect(sync.fields.aspect);
                recoverySample.gameplayEnabled = input.gameplayEnabled;
                const bool nativeEndpoint = gameplay::IsNativeHorPlusRecoverySample(
                    recoverySample, input.recoveryEndpointTolerance);
                const auto action = gameplay::ResolveHorPlusRecoveryAction(
                    observation.coordinatorAfter, recoverySample, nativeEndpoint,
                    input.recoveryEndpointTolerance);
                const bool atCachedCinematicEndpoint =
                    recoverySample.cameraReadable &&
                    recoverySample.source != 0 &&
                    recoverySample.source == recoverySample.validatedSource &&
                    recoverySample.flags == 0x04 &&
                    IsValidCameraWriterFov(recoverySample.inputFov) &&
                    IsValidCameraWriterFov(recoverySample.cachedCinematicFov) &&
                    IsValidCameraWriterFov(recoverySample.exitNativeTarget) &&
                    std::fabs(recoverySample.cachedCinematicFov -
                        recoverySample.exitNativeTarget) >
                        input.recoveryInterpolationRangeEpsilon &&
                    std::fabs(recoverySample.inputFov -
                        recoverySample.cachedCinematicFov) <=
                        input.recoveryInterpolationRangeEpsilon;

                if (!sync.confirmed && action !=
                        gameplay::HorPlusRecoveryAction::NotWaiting &&
                    (action != gameplay::HorPlusRecoveryAction::HoldNativePassThrough ||
                        atCachedCinematicEndpoint) &&
                    IsValidCameraWriterFov(recoverySample.cachedCinematicFov)) {
                    // Keep the last already-confirmed Cinematic domain while
                    // Gameplay synchronization is unavailable. Do not derive
                    // a new-domain HorPlus result or complete EXIT recovery.
                    result.horPlusRecovery.action = action;
                    result.horPlusRecovery.outputFov =
                        recoverySample.cachedCinematicFov;
                    result.outputFov = recoverySample.cachedCinematicFov;
                    result.mutateFov = true;
                    result.disposition = CameraWriterDisposition::FinalOutput;
                    telemetry.runtimeCombineFovOutput = result.outputFov;
                    telemetry.runtimeCombineFovApplied = true;
                    telemetry.reason = "RUNTIME_ASPECT_UNCONFIRMED_HOLD_CINEMATIC";
                    result.recoveryOwnerAfter = state.recoveryOwner.Read();
                    return result;
                }

                if (sync.confirmed && atCachedCinematicEndpoint && action ==
                        gameplay::HorPlusRecoveryAction::HoldNativePassThrough) {
                    // The first native callback can differ from the cached
                    // transformed endpoint by float representation alone.
                    // Keep the confirmed endpoint without retiring recovery.
                    result.horPlusRecovery.action =
                        gameplay::HorPlusRecoveryAction::TransformRecoveryInterpolation;
                    result.horPlusRecovery.outputFov =
                        recoverySample.cachedCinematicFov;
                    result.outputFov = recoverySample.cachedCinematicFov;
                    result.mutateFov = true;
                    result.disposition = CameraWriterDisposition::FinalOutput;
                    telemetry.runtimeCombineFovOutput = result.outputFov;
                    telemetry.runtimeCombineFovApplied = true;
                    telemetry.reason = "RUNTIME_COMBINE_RECOVERY_ENDPOINT_HOLD";
                    result.recoveryOwnerAfter = state.recoveryOwner.Read();
                    return result;
                }

                result.horPlusRecovery = ProcessHorPlusRecovery(state,
                    observation.coordinatorAfter, recoverySample, nativeEndpoint,
                    input.recoveryEndpointTolerance, input.nativeAspect);
                if (result.horPlusRecovery.action ==
                        gameplay::HorPlusRecoveryAction::TransformRecoveryInterpolation) {
                    result.outputFov = result.horPlusRecovery.outputFov;
                    result.mutateFov = true;
                    result.disposition = CameraWriterDisposition::FinalOutput;
                    telemetry.runtimeCombineFovOutput = result.outputFov;
                    telemetry.runtimeCombineFovApplied = true;
                    telemetry.reason = "RUNTIME_COMBINE_RECOVERY_INTERPOLATION";
                } else if (result.horPlusRecovery.action ==
                        gameplay::HorPlusRecoveryAction::HoldNativePassThrough) {
                    result.disposition = CameraWriterDisposition::FinalOutput;
                    telemetry.runtimeCombineFovOutput = input.incomingFov;
                    telemetry.reason = sync.confirmed
                        ? "RECOVERY_SAMPLE_UNCLASSIFIED"
                        : "RUNTIME_ASPECT_UNCONFIRMED";
                } else if (result.horPlusRecovery.action ==
                        gameplay::HorPlusRecoveryAction::ResumeGameplay) {
                    result.coordinatorAfter = state.coordinator.load(
                        std::memory_order_acquire);
                }
                result.recoveryOwnerAfter = state.recoveryOwner.Read();
                if (result.horPlusRecovery.action !=
                        gameplay::HorPlusRecoveryAction::ResumeGameplay)
                    return result;
            } else {
                const auto fields = ReadCameraWriterAspectFlags(engineOperations,
                    input.sourceIdentity, true);
                const float targetFov = state.exitTargetFov.load(
                    std::memory_order_acquire);
                const bool endpoint = input.sourceIdentity != 0 &&
                    fields.aspectReadable && fields.flagsReadable &&
                    IsValidCameraWriterAspect(fields.aspect) && fields.flags == 0x04 &&
                    IsValidCameraWriterFov(input.incomingFov) &&
                    IsValidCameraWriterFov(targetFov) &&
                    std::fabs(input.incomingFov - targetFov) <=
                        input.recoveryEndpointTolerance;
                if (endpoint) {
                    state.coordinator.store(camera::CoordinatorState::Gameplay,
                        std::memory_order_release);
                    state.continuationPending.store(false, std::memory_order_release);
                    state.continuationInterpolationActive.store(false,
                        std::memory_order_release);
                    state.continuationSource.store(0, std::memory_order_release);
                    result.horPlusRecovery.completion = CompleteCameraWriterRecovery(
                        state, observation.recoveryOwnerAfter.generation);
                    result.horPlusRecovery.completed =
                        result.horPlusRecovery.completion.completed;
                    result.aspectRecoveryEndpoint = true;
                }
                result.disposition = CameraWriterDisposition::FinalOutput;
                result.continuation = CameraWriterContinuation::ReturnCinematic;
                result.coordinatorAfter = state.coordinator.load(
                    std::memory_order_acquire);
                result.recoveryOwnerAfter = state.recoveryOwner.Read();
                return result;
            }
        }

        if (result.atomicExitHandoff.event == AtomicExitHandoffEvent::None &&
            state.atomicExitHandoffPending.load(std::memory_order_acquire) &&
            observation.coordinatorAfter != camera::CoordinatorState::CinematicActive) {
            result.atomicExitHandoff = ProcessAtomicExitHandoff(state, input,
                observation.coordinatorAfter, observation.recoveryOwnerAfter,
                engineOperations, dialogueReset, gameplayState);
            if (result.atomicExitHandoff.handled) {
                result.disposition = CameraWriterDisposition::FinalOutput;
                result.continuation = CameraWriterContinuation::ReturnCinematic;
                return result;
            }
        }

        if (observation.coordinatorAfter == camera::CoordinatorState::CinematicActive) {
        const bool correctionEnabled = cinematicState.activeCorrectionEnabled.load(
            std::memory_order_acquire);
        const bool targetEstablished = cinematicState.targetEstablished.load(
            std::memory_order_acquire);
        const bool generationValid = cinematicState.selectionValid.load(
            std::memory_order_acquire);
        const auto generation = cinematicState.generation.load(
            std::memory_order_acquire);
        result.initialEstablishmentAuthorized = correctionEnabled &&
            cinematicState.association.AuthorizeInitialEstablishmentRetention(
                observation.coordinatorAfter, true, generationValid,
                targetEstablished, generation, input.sourceIdentity, input.incomingFov);
        result.associationAfterDecision = cinematicState.association.Read();
        auto continuationSource = cinematicState.continuationSource.load(
            std::memory_order_acquire);
        result.continuationStartFov = cinematicState.continuationStartFov.load(
            std::memory_order_acquire);
        result.continuationInterpolationActive =
            cinematicState.continuationInterpolationActive.load(std::memory_order_acquire);
        const auto& association = result.associationAfterDecision;
        // The newly accepted Gameplay-baseline continuation is not a source-only
        // authorization. Every retained sample must still belong to this accepted
        // generation and its confirmed aspect-store / RSI / RBX association.
        const bool gameplayBaselineContinuation = result.continuationInterpolationActive &&
            association.effectiveSettings.fovMode == config::CinematicFovMode::GameplayHorPlus &&
            !camera::AreFovValuesSemanticallyEquivalent(result.continuationStartFov,
                cinematicState.authoredEnterFov.load(std::memory_order_acquire));
        if (gameplayBaselineContinuation &&
            (!generationValid || !association.generationActive ||
             association.generation != generation || !association.effectiveSettings.enabled ||
             !association.associationValid || association.associationConflict ||
             association.aspectStoreTargetObject != input.sourceIdentity ||
             association.associatedWriterRsi != input.sourceIdentity ||
             association.observedWriterRbx != input.outputObjectIdentity ||
             association.gameplayBaselineSource == 0 ||
             association.gameplayBaselineSource != continuationSource ||
             !std::isfinite(association.gameplayBaselineFov) ||
             std::fabs(result.continuationStartFov - association.gameplayBaselineFov) > 0.05f))
            continuationSource = 0;
        result.continuationSourceMatch = continuationSource != 0 &&
            continuationSource == input.sourceIdentity;

        const cinematics::CinematicWriterSample sample{
            input.sourceIdentity,
            continuationSource,
            input.incomingFov,
            cinematicState.authoredEnterFov.load(std::memory_order_acquire),
            cinematicState.transformedTargetFov.load(std::memory_order_acquire),
            result.continuationStartFov,
            targetEstablished,
            result.continuationInterpolationActive,
            result.initialEstablishmentAuthorized,
            result.associationAfterDecision.gameplayBaselineFov
        };
        const auto dispatch = cinematics::DispatchCinematicWriter(
            observation.coordinatorAfter, correctionEnabled, sample);
        result.cinematicHandled = dispatch.handled;
        result.cinematicAction = dispatch.resolution.action;
        if (dispatch.handled &&
            (dispatch.resolution.action ==
                cinematics::CinematicWriterAction::RetainTransformedTarget ||
             dispatch.resolution.action ==
                cinematics::CinematicWriterAction::RetainContinuationTarget)) {
            result.outputFov = dispatch.resolution.outputFov;
            result.mutateFov = true;
        }
        if (dispatch.handled) {
            result.disposition = CameraWriterDisposition::FinalOutput;
            result.continuation = CameraWriterContinuation::ReturnCinematic;
            return result;
        }

        }

        observation.coordinatorAfter = state.coordinator.load(std::memory_order_acquire);
        if (observation.coordinatorAfter == camera::CoordinatorState::CinematicExiting)
            return result;

        if (gameplayState) {
            if (input.gameplayMode == config::GameplayMode::AspectRecalculation)
                result.gameplay = ProcessGameplayAspectRecalculationWriter(
                    *gameplayState, input, observation.coordinatorAfter,
                    engineOperations, gameplayOperations);
            else
                result.gameplay = ProcessGameplayHorPlusWriter(*gameplayState, input,
                    observation.coordinatorAfter, engineOperations,
                    gameplayOperations);
        }
        if (result.gameplay.handled) {
            result.disposition = CameraWriterDisposition::FinalOutput;
            // Preserve the existing mode-transition rule: until the pending
            // aspect transition is resolved, record the HorPlus observation
            // but leave this callback's XMM0 native.
            result.mutateFov = result.gameplay.applied &&
                !result.gameplay.modeTransitionPending;
            result.outputFov = result.mutateFov
                ? result.gameplay.outputFov : input.incomingFov;
        }
        return result;
    }

    inline CameraWriterRecoveryCompletionResult CompleteCameraWriterRecovery(
        CameraWriterRecoveryStateView state, std::uint64_t generation)
    {
        CameraWriterRecoveryCompletionResult result{};
        result.completed = state.recoveryOwner.MarkComplete(generation);
        if (!result.completed) {
            result.owner = state.recoveryOwner.Read();
            return result;
        }
        result.exclusionActive = state.postCinematicExclusion.IsActive();
        if (!result.exclusionActive) state.recoveryOwner.Clear();
        result.owner = state.recoveryOwner.Read();
        return result;
    }

    inline AtomicExitHandoffResult ProcessAtomicExitHandoff(
        CameraWriterRecoveryStateView state, const CameraWriterInput& input,
        camera::CoordinatorState coordinator,
        const cinematics::CinematicRecoveryOwnerSnapshot& owner,
        const CameraWriterEngineOperations& engineOperations,
        CameraWriterDialogueResetOperation dialogueReset,
        const CameraWriterGameplayStateView* gameplayState)
    {
        AtomicExitHandoffResult result{};
        if (!state.atomicExitHandoffPending.load(std::memory_order_acquire)) return result;
        const auto reset = [&] {
            state.atomicExitHandoffPending.store(false, std::memory_order_release);
            state.atomicExitHandoffSource.store(0, std::memory_order_release);
            state.atomicExitHandoffPreviousFov.store(
                std::numeric_limits<float>::quiet_NaN(), std::memory_order_release);
        };
        if (coordinator != camera::CoordinatorState::CinematicExiting) {
            reset();
            result.event = AtomicExitHandoffEvent::CancelledCoordinator;
            return result;
        }
        if (dialogueReset.isActive && dialogueReset.isActive(dialogueReset.context)) {
            reset();
            result.event = AtomicExitHandoffEvent::CancelledDialogue;
            return result;
        }
        if (!owner.HasRecoveryAuthority()) {
            const auto cleanup = ClearCameraWriterRecoveryState(state);
            result.event = AtomicExitHandoffEvent::NoOwner;
            static_cast<void>(cleanup);
            return result;
        }
        if (owner.writerRsi != input.sourceIdentity) {
            result.event = AtomicExitHandoffEvent::ForeignSource;
            result.handled = true;
            return result;
        }
        const auto expectedSource = state.atomicExitHandoffSource.load(
            std::memory_order_acquire);
        const float target = state.exitTargetFov.load(std::memory_order_acquire);
        if (expectedSource != 0 && expectedSource != input.sourceIdentity) {
            reset();
            result.event = AtomicExitHandoffEvent::SourceMismatch;
            return result;
        }
        if (!std::isfinite(input.incomingFov) || !std::isfinite(target)) {
            reset();
            result.event = AtomicExitHandoffEvent::InvalidFov;
            return result;
        }
        const float previousFov = state.atomicExitHandoffPreviousFov.load(
            std::memory_order_acquire);
        if (expectedSource == 0) {
            state.atomicExitHandoffSource.store(input.sourceIdentity,
                std::memory_order_release);
            state.atomicExitHandoffPreviousFov.store(input.incomingFov,
                std::memory_order_release);
            result.event = AtomicExitHandoffEvent::FirstSample;
            return result;
        }
        state.atomicExitHandoffPreviousFov.store(input.incomingFov,
            std::memory_order_release);
        if (!std::isfinite(previousFov) ||
            input.incomingFov >= previousFov - input.atomicHandoffFovEpsilon ||
            input.incomingFov <= target + input.recoveryInterpolationRangeEpsilon) {
            result.event = AtomicExitHandoffEvent::NotDownward;
            return result;
        }
        const auto fields = ReadCameraWriterAspectFlags(engineOperations,
            input.sourceIdentity, true);
        if (!fields.aspectReadable || !fields.flagsReadable) {
            reset();
            result.event = AtomicExitHandoffEvent::CameraUnreadable;
            return result;
        }
        result.previousAspect = fields.aspect;
        result.previousFlags = fields.flags;
        bool expectedPending = true;
        if (!state.atomicExitHandoffPending.compare_exchange_strong(expectedPending,
                false, std::memory_order_acq_rel))
            return result;
        result.writeSucceeded = RequestCameraWriterAspectAndFlags(
            engineOperations, input.sourceIdentity, input.nativeAspect, 0x4);
        result.event = result.writeSucceeded ? AtomicExitHandoffEvent::Applied
            : AtomicExitHandoffEvent::WriteFailed;
        result.handled = true;
        if (result.writeSucceeded && gameplayState) {
            gameplayState->replayState.store(gameplay::ReplayState::Complete,
                std::memory_order_release);
            gameplayState->lastAutoRestoreSource.store(input.sourceIdentity,
                std::memory_order_release);
            if (gameplayState->enableApplyPending.load(std::memory_order_acquire) &&
                input.gameplayEnabled && input.gameplayMode ==
                    config::GameplayMode::AspectRecalculation &&
                coordinator == camera::CoordinatorState::Gameplay) {
                gameplayState->enableApplyPending.store(false, std::memory_order_release);
                gameplayState->enableTransitionDeferralLogged.store(false,
                    std::memory_order_release);
            }
        }
        return result;
    }

    inline HorPlusRecoveryTransactionResult ProcessHorPlusRecovery(
        CameraWriterRecoveryStateView state,
        camera::CoordinatorState coordinator,
        gameplay::HorPlusRecoverySample sample,
        bool nativeEndpoint, float endpointTolerance, float nativeAspect)
    {
        HorPlusRecoveryTransactionResult result{};
        result.outputFov = sample.inputFov;
        result.action = gameplay::ResolveHorPlusRecoveryAction(
            coordinator, sample, nativeEndpoint, endpointTolerance);
        if (result.action == gameplay::HorPlusRecoveryAction::TransformRecoveryInterpolation) {
            result.outputFov = gameplay::ResolveHorPlusRecoveryInterpolationFov(
                sample, nativeAspect);
            if (!std::isfinite(result.outputFov) || result.outputFov <= 1.0f ||
                result.outputFov >= 179.0f) {
                result.outputFov = sample.inputFov;
                result.action = gameplay::HorPlusRecoveryAction::HoldNativePassThrough;
            }
        } else if (result.action == gameplay::HorPlusRecoveryAction::ResumeGameplay) {
            state.coordinator.store(camera::CoordinatorState::Gameplay,
                std::memory_order_release);
            state.continuationPending.store(false, std::memory_order_release);
            state.continuationInterpolationActive.store(false,
                std::memory_order_release);
            state.continuationSource.store(0, std::memory_order_release);
            result.completion = CompleteCameraWriterRecovery(
                state, state.recoveryOwner.Read().generation);
            result.completed = result.completion.completed;
        }
        return result;
    }
}
