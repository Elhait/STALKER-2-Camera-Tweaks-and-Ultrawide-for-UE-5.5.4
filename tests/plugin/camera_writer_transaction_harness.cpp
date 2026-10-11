#include "../../src/plugin/camera_writer_transaction.hpp"
#include "../../src/overlay/presenter/presenter_runtime_policy.hpp"
#include "../../src/plugin/cinematic_lifecycle_transaction.hpp"
#include "../../src/camera/camera_state_publication.hpp"
#include "../../src/gameplay/horplus_gameplay.hpp"
#include "../../src/gameplay/gameplay_state.hpp"
#include "../../src/overlay/presentation/camera_integration.hpp"

#include <array>
#include <cmath>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <string_view>

namespace
{
    struct CameraStateFreshnessSink
    {
        std::mutex* mutex{};
        camera::CameraStateSnapshot* snapshot{};
        bool* valid{};

        static void OnSettingChanged(void* context,
            plugin::CameraSettingFreshnessChange change)
        {
            auto& sink = *static_cast<CameraStateFreshnessSink*>(context);
            std::lock_guard lock(*sink.mutex);
            plugin::MarkCameraSettingFactsHistorical(*sink.snapshot,
                *sink.valid, change);
        }
    };

    void SelectMode(void* context, config::GameplayMode mode)
    {
        static_cast<std::atomic<config::GameplayMode>*>(context)->store(
            mode, std::memory_order_release);
    }

    bool Check(bool value, const char* message)
    {
        if (!value) std::cerr << "FAIL: " << message << '\n';
        return value;
    }

    struct SettingsFixture
    {
        std::atomic<config::GameplayMode> gameplayMode{config::GameplayMode::HorPlus};
        std::atomic<bool> gameplayEnabled{true};
        std::atomic<std::uint64_t> gameplayEpoch{0};
        std::atomic<bool> gameplayEnablePending{};
        std::atomic<bool> gameplayDisablePending{};
        std::atomic<bool> gameplayTransitionPending{};
        std::atomic<bool> gameplayTransitionDeferralLogged{};
        std::atomic<bool> cinematicsEnabled{true};
        std::atomic<std::uint64_t> cinematicsEpoch{0};
        std::atomic<bool> activeCinematicsEnabled{};
        std::atomic<std::uint64_t> activeCinematicsEpoch{};
        std::atomic<bool> cinematicFovApplied{};
        std::atomic<float> cinematicTarget{NAN};
        std::atomic<float> exitTarget{NAN};
        std::atomic<bool> continuationPending{};
        std::atomic<bool> continuationCarryPending{};
        std::atomic<bool> continuationInterpolation{};
        std::atomic<std::uintptr_t> continuationSource{};
        std::atomic<bool> dialogueEnabled{true};
        std::atomic<config::DialogueZoomPolicy> dialoguePolicy{
            config::DialogueZoomPolicy::Adaptive};
        std::atomic<std::uint64_t> dialogueEpoch{0};
        std::atomic<std::uint64_t> activeDialogueEpoch{0};
        bool dialogueNonNativeCapabilityAvailable{true};
        std::atomic<config::CinematicAspectPolicy> cinematicAspectPolicy{
            config::CinematicAspectPolicy::Auto};
        std::atomic<config::CinematicFovMode> cinematicFovMode{
            config::CinematicFovMode::NativeHorPlus};
        dialogue::CorrectionActivation dialogueActivation;
        void* cameraStateFreshnessContext{};
        void (*cameraSettingChanged)(void*, plugin::CameraSettingFreshnessChange){};

        plugin::CameraSettingStateView View()
        {
            return {gameplayMode, gameplayEnabled, gameplayEpoch, gameplayEnablePending,
                gameplayDisablePending, gameplayTransitionPending,
                gameplayTransitionDeferralLogged, cinematicsEnabled,
                cinematicsEpoch, activeCinematicsEnabled, activeCinematicsEpoch,
                cinematicFovApplied, cinematicTarget, exitTarget,
                continuationPending, continuationCarryPending, continuationInterpolation,
                continuationSource, dialogueEnabled, dialoguePolicy, dialogueEpoch,
                activeDialogueEpoch, dialogueNonNativeCapabilityAvailable,
                cinematicAspectPolicy, cinematicFovMode,
                dialogueActivation, &gameplayMode, &SelectMode,
                cameraStateFreshnessContext, cameraSettingChanged};
        }
    };

    struct GameplayWriterFixture
    {
        std::atomic<bool> enableApplyPending{};
        std::atomic<bool> disableRestorePending{};
        std::atomic<bool> modeTransitionPending{};
        std::atomic<bool> enableTransitionDeferralLogged{};
        std::atomic<bool> modeTransitionDeferralLogged{};
        std::atomic<config::GameplayMode> modeTransitionTarget{
            config::GameplayMode::HorPlus};
        std::atomic<gameplay::ReplayState> replayState{
            gameplay::ReplayState::WaitingForAutomaticUpdate};
        std::atomic<std::uintptr_t> lastAutoRestoreSource{};
        std::atomic<std::uintptr_t> lastSource{};
        std::atomic<float> lastFov{NAN};
        camera::GameplayAspectRestorationStore aspectRestoration;
        camera::CameraFovObservationStore fovObservation;
        camera::GameplayBaselineStore baseline;

        plugin::CameraWriterGameplayStateView View()
        {
            return {enableApplyPending, disableRestorePending,
                modeTransitionPending, enableTransitionDeferralLogged,
                modeTransitionDeferralLogged, modeTransitionTarget,
                replayState, lastAutoRestoreSource, lastSource, lastFov,
                aspectRestoration, fovObservation, baseline};
        }
    };

    struct ModeTransitionBarrier
    {
        std::mutex mutex;
        std::condition_variable condition;
        unsigned calls{};
        bool pause{};
        bool entered{};
        bool release{};
    };

    struct FreshFieldsFixture
    {
        std::uintptr_t expectedSource{};
        float aspect{3.55556f};
        std::uint8_t flags{0x04};
        bool readable{true};
        unsigned calls{};
        ModeTransitionBarrier* barrier{};
        bool useAspectSequence{};
        std::array<float, 4> aspectSequence{};
        bool aspectFlagsWriteSucceeds{true};
        unsigned aspectFlagsWriteCalls{};
    };

    plugin::CameraWriterFreshAspectFlags ReadFreshFields(
        void* context, std::uintptr_t source, bool includeFlags)
    {
        auto& fixture = *static_cast<FreshFieldsFixture*>(context);
        const auto callIndex = fixture.calls++;
        if (fixture.barrier && fixture.barrier->pause) {
            std::unique_lock lock(fixture.barrier->mutex);
            ++fixture.barrier->calls;
            if (!fixture.barrier->entered) {
                fixture.barrier->entered = true;
                fixture.barrier->condition.notify_all();
                fixture.barrier->condition.wait(lock, [&] {
                    return fixture.barrier->release;
                });
            }
        }
        plugin::CameraWriterFreshAspectFlags fields{};
        if (source != fixture.expectedSource || !fixture.readable) return fields;
        fields.aspect = fixture.useAspectSequence && callIndex < fixture.aspectSequence.size()
            ? fixture.aspectSequence[callIndex] : fixture.aspect;
        fields.flags = fixture.flags;
        fields.aspectReadable = true;
        fields.flagsReadable = includeFlags;
        return fields;
    }

    bool WriteFreshAspectAndFlags(void* context, std::uintptr_t, float, std::uint8_t)
    {
        auto& fixture = *static_cast<FreshFieldsFixture*>(context);
        ++fixture.aspectFlagsWriteCalls;
        return fixture.aspectFlagsWriteSucceeds;
    }

    struct EngineOperationFixture
    {
        std::array<unsigned, 3> operations{};
        unsigned count{};
        bool aspectOnlyResult{true};
        bool aspectFlagsResult{};
    };

    plugin::CameraWriterFreshAspectFlags RecordEngineRead(
        void* context, std::uintptr_t, bool includeFlags)
    {
        auto& fixture = *static_cast<EngineOperationFixture*>(context);
        fixture.operations[fixture.count++] = includeFlags ? 1u : 2u;
        return {3.5f, 0x04, true, includeFlags};
    }

    bool RecordAspectOnlyWrite(void* context, std::uintptr_t, float)
    {
        auto& fixture = *static_cast<EngineOperationFixture*>(context);
        fixture.operations[fixture.count++] = 3u;
        return fixture.aspectOnlyResult;
    }

    bool RecordAspectFlagsWrite(void* context, std::uintptr_t, float, std::uint8_t)
    {
        auto& fixture = *static_cast<EngineOperationFixture*>(context);
        fixture.operations[fixture.count++] = 4u;
        return fixture.aspectFlagsResult;
    }

    struct RuntimeCombineMemory
    {
        std::uintptr_t source{0xC01};
        float aspect{16.0f / 9.0f};
        std::uint8_t flags{0x04};
        bool readable{true};
        bool writeSucceeds{true};
        bool writeActuallyApplies{true};
        unsigned reads{};
        unsigned aspectWrites{};
        unsigned aspectFlagsWrites{};
    };

    plugin::CameraWriterFreshAspectFlags ReadRuntimeCombineMemory(
        void* context, std::uintptr_t source, bool includeFlags)
    {
        auto& memory = *static_cast<RuntimeCombineMemory*>(context);
        ++memory.reads;
        if (source != memory.source || !memory.readable) return {};
        return {memory.aspect, memory.flags, true, includeFlags};
    }

    bool WriteRuntimeCombineAspect(void* context, std::uintptr_t source, float aspect)
    {
        auto& memory = *static_cast<RuntimeCombineMemory*>(context);
        ++memory.aspectWrites;
        if (source != memory.source || !memory.writeSucceeds) return false;
        if (memory.writeActuallyApplies) memory.aspect = aspect;
        return true;
    }

    bool WriteRuntimeCombineAspectAndFlags(void* context, std::uintptr_t source,
        float aspect, std::uint8_t flags)
    {
        auto& memory = *static_cast<RuntimeCombineMemory*>(context);
        ++memory.aspectFlagsWrites;
        if (source != memory.source || !memory.writeSucceeds) return false;
        memory.aspect = aspect;
        memory.flags = flags;
        return true;
    }

    bool CheckRuntimeCombineProductionTransaction()
    {
        using namespace camera;
        constexpr float native = 16.0f / 9.0f;
        constexpr float arbitraryTarget = 2.75f;
        RuntimeCombineMemory memory{};
        std::atomic<CoordinatorState> coordinator{CoordinatorState::Gameplay};
        cinematics::CinematicWriterAssociation association;
        cinematics::CinematicRecoveryOwner owner;
        dialogue::PostCinematicRecoveryExclusion exclusion;
        std::atomic<bool> handoffPending{};
        std::atomic<std::uintptr_t> handoffSource{};
        std::atomic<float> handoffFov{NAN};
        std::atomic<bool> continuationPending{};
        std::atomic<bool> continuationCarryPending{};
        std::atomic<std::uintptr_t> continuationSource{};
        std::atomic<float> continuationTarget{NAN};
        std::atomic<float> continuationPrevious{NAN};
        std::atomic<bool> continuationInterpolation{};
        std::atomic<float> continuationStart{NAN};
        std::atomic<std::uintptr_t> baselineSource{};
        std::atomic<float> recoveryTarget{NAN};
        std::atomic<float> recoveryCinematicTarget{NAN};
        plugin::CameraWriterRecoveryStateView recoveryState{coordinator, association,
            owner, exclusion, handoffPending, handoffSource, handoffFov,
            continuationPending, continuationCarryPending, continuationSource,
            continuationTarget, continuationPrevious, continuationInterpolation,
            continuationStart, baselineSource, recoveryTarget, recoveryCinematicTarget};
        std::atomic<bool> cinematicCorrection{};
        std::atomic<bool> cinematicSelection{};
        std::atomic<std::uint64_t> cinematicGeneration{};
        std::atomic<bool> cinematicTargetEstablished{};
        std::atomic<float> authoredFov{NAN};
        std::atomic<float> cinematicTargetFov{NAN};
        std::atomic<std::uintptr_t> cinematicContinuationSource{};
        std::atomic<float> cinematicContinuationStart{NAN};
        std::atomic<bool> cinematicContinuationActive{};
        plugin::CameraWriterCinematicStateView cinematicState{cinematicCorrection,
            cinematicSelection, cinematicGeneration, cinematicTargetEstablished,
            authoredFov, cinematicTargetFov, cinematicContinuationSource,
            cinematicContinuationStart, cinematicContinuationActive, association};
        plugin::CameraCorrectionGate gate;
        GameplayWriterFixture gameplayState;
        const plugin::CameraWriterEngineOperations engineOperations{&memory,
            &ReadRuntimeCombineMemory, &WriteRuntimeCombineAspect,
            &WriteRuntimeCombineAspectAndFlags};

        const auto run = [&](float target, bool targetValid, bool enabled = true) {
            plugin::CameraWriterInput input{memory.source, 0xC02, 90.6557f,
                memory.aspect, true, memory.flags, true, 0.02f, enabled,
                config::GameplayMode::RuntimeCombine, native};
            input.currentViewportAspect = target;
            input.currentViewportAspectValid = targetValid;
            auto gameplayView = gameplayState.View();
            auto guard = gate.Acquire();
            return plugin::ProcessCameraWriter(gate, recoveryState, input,
                cinematicState, engineOperations, {}, &gameplayView);
        };

        bool pass = true;
        const auto corrected = run(arbitraryTarget, true);
        pass &= Check(corrected.gameplay.runtimeCombineTelemetryValid &&
            corrected.gameplay.runtimeCombineMismatch &&
            corrected.gameplay.runtimeCombineWriteAttempted &&
            corrected.gameplay.runtimeCombineWriteSucceeded &&
            corrected.gameplay.runtimeCombineConfirmed &&
            std::fabs(corrected.gameplay.runtimeCombineAspectBefore - native) < 0.001f &&
            std::fabs(corrected.gameplay.runtimeCombineTargetAspect - arbitraryTarget) < 0.001f &&
            std::fabs(corrected.gameplay.runtimeCombineAspectAfter - arbitraryTarget) < 0.001f &&
            memory.aspectWrites == 1 && memory.aspectFlagsWrites == 0 &&
            memory.flags == 0x04 && corrected.gameplay.runtimeCombineFlagsBefore ==
                corrected.gameplay.runtimeCombineFlagsAfter && corrected.mutateFov &&
            corrected.gameplay.runtimeCombineFovApplied &&
            std::fabs(corrected.gameplay.policyAspect - arbitraryTarget) < 0.001f &&
            corrected.gameplay.observation.policyAspect.provenance ==
                FovProvenance::ClientViewportAspect &&
            std::fabs(corrected.outputFov - gameplay::EvaluateHorPlus(
                90.6557f, arbitraryTarget, 0x04, native).outputFov) < 0.001f,
            "RuntimeCombine synchronizes an arbitrary target aspect before HorPlus and preserves native flags");

        CameraStateSnapshot publishedSnapshot{};
        bool publishedValid{};
        float neutralCandidate{NAN};
        std::uint32_t neutralSamples{};
        std::uintptr_t neutralSource{};
        std::atomic<bool> zoomHold{};
        std::atomic<std::uint64_t> eventSequence{};
        CameraStatePublicationView publicationState{publishedSnapshot, publishedValid,
            neutralCandidate, neutralSamples, neutralSource, zoomHold, eventSequence};
        CameraStatePublicationInput publication{};
        publication.observationPublished = corrected.gameplay.observationPublished;
        publication.observation = corrected.gameplay.observation;
        publication.source = corrected.gameplay.source;
        publication.applied = corrected.gameplay.applied;
        publication.presentation = PresentationState::Gameplay;
        publication.gameplayMode = static_cast<std::uint8_t>(
            config::GameplayMode::RuntimeCombine);
        publication.runtimeCombineValid = corrected.gameplay.runtimeCombineTelemetryValid;
        publication.runtimeCombineAspectBefore = corrected.gameplay.runtimeCombineAspectBefore;
        publication.runtimeCombineTargetAspect = corrected.gameplay.runtimeCombineTargetAspect;
        publication.runtimeCombineAspectAfter = corrected.gameplay.runtimeCombineAspectAfter;
        publication.runtimeCombineFovInput = corrected.gameplay.runtimeCombineFovInput;
        publication.runtimeCombineFovOutput = corrected.gameplay.runtimeCombineFovOutput;
        publication.runtimeCombineFlagsBefore = corrected.gameplay.runtimeCombineFlagsBefore;
        publication.runtimeCombineFlagsAfter = corrected.gameplay.runtimeCombineFlagsAfter;
        publication.runtimeCombineMismatch = corrected.gameplay.runtimeCombineMismatch;
        publication.runtimeCombineWriteAttempted = corrected.gameplay.runtimeCombineWriteAttempted;
        publication.runtimeCombineWriteSucceeded = corrected.gameplay.runtimeCombineWriteSucceeded;
        publication.runtimeCombineReadbackValid = corrected.gameplay.runtimeCombineReadbackValid;
        publication.runtimeCombineConfirmed = corrected.gameplay.runtimeCombineConfirmed;
        publication.runtimeCombineFovApplied = corrected.gameplay.runtimeCombineFovApplied;
        const auto publicationResult = PublishCameraStateSnapshotLocked(
            publicationState, publication);
        pass &= Check(publicationResult.published &&
            publishedSnapshot.evidence.runtimeCombineValid &&
            publishedSnapshot.evidence.runtimeCombineWriteAttempted &&
            publishedSnapshot.evidence.runtimeCombineWriteSucceeded &&
            publishedSnapshot.evidence.runtimeCombineReadbackValid &&
            publishedSnapshot.evidence.runtimeCombineConfirmed &&
            publishedSnapshot.evidence.runtimeCombineFlagsBefore == 0x04 &&
            publishedSnapshot.evidence.runtimeCombineFlagsAfter == 0x04 &&
            std::fabs(publishedSnapshot.evidence.runtimeCombineFovInput - 90.6557f) < 0.001f &&
            std::fabs(publishedSnapshot.evidence.runtimeCombineFovOutput -
                corrected.outputFov) < 0.001f,
            "RuntimeCombine transaction telemetry reaches the published Camera State snapshot");

        const auto alreadyMatched = run(arbitraryTarget, true);
        pass &= Check(alreadyMatched.gameplay.runtimeCombineConfirmed &&
            !alreadyMatched.gameplay.runtimeCombineMismatch &&
            !alreadyMatched.gameplay.runtimeCombineWriteAttempted &&
            memory.aspectWrites == 1 && alreadyMatched.mutateFov,
            "RuntimeCombine skips aspect mutation when the receiver already matches target");

        memory.aspect = native;
        memory.writeSucceeds = false;
        const auto writeFailed = run(arbitraryTarget, true);
        pass &= Check(writeFailed.gameplay.runtimeCombineMismatch &&
            writeFailed.gameplay.runtimeCombineWriteAttempted &&
            !writeFailed.gameplay.runtimeCombineWriteSucceeded &&
            !writeFailed.gameplay.runtimeCombineConfirmed &&
            !writeFailed.gameplay.runtimeCombineFovApplied && !writeFailed.mutateFov &&
            std::string_view(writeFailed.gameplay.reason) == "RUNTIME_ASPECT_UNCONFIRMED" &&
            std::fabs(writeFailed.outputFov - 90.6557f) < 0.001f,
            "RuntimeCombine fails closed and does not HorPlus-transform after an unconfirmed write");

        memory.writeSucceeds = true;
        memory.writeActuallyApplies = false;
        memory.aspect = native;
        const auto unconfirmedReadback = run(arbitraryTarget, true);
        pass &= Check(unconfirmedReadback.gameplay.runtimeCombineWriteSucceeded &&
            unconfirmedReadback.gameplay.runtimeCombineReadbackValid &&
            !unconfirmedReadback.gameplay.runtimeCombineConfirmed &&
            !unconfirmedReadback.gameplay.runtimeCombineFovApplied &&
            !unconfirmedReadback.mutateFov &&
            std::fabs(unconfirmedReadback.outputFov - 90.6557f) < 0.001f,
            "RuntimeCombine fails closed when a reported write does not confirm by read-back");

        memory.writeActuallyApplies = true;
        const auto invalidTarget = run(NAN, false);
        pass &= Check(!invalidTarget.gameplay.runtimeCombineConfirmed &&
            !invalidTarget.gameplay.runtimeCombineWriteAttempted &&
            !invalidTarget.gameplay.runtimeCombineFovApplied && !invalidTarget.mutateFov &&
            std::fabs(invalidTarget.outputFov - 90.6557f) < 0.001f,
            "RuntimeCombine fails closed when runtime target aspect is unavailable");

        const auto invalidMarkedTarget = run(-1.0f, true);
        pass &= Check(!invalidMarkedTarget.gameplay.runtimeCombineConfirmed &&
            !invalidMarkedTarget.gameplay.runtimeCombineWriteAttempted &&
            !invalidMarkedTarget.gameplay.runtimeCombineFovApplied &&
            !invalidMarkedTarget.mutateFov,
            "RuntimeCombine independently validates a target marked available");

        const auto disabled = run(arbitraryTarget, true, false);
        pass &= Check(memory.aspectWrites == 3 && !disabled.gameplay.applied &&
            !disabled.mutateFov && std::fabs(disabled.outputFov - 90.6557f) < 0.001f,
            "RuntimeCombine OFF does not perform correction or mutate the aspect");

        memory.writeSucceeds = true;
        memory.aspect = native;
        const auto aspect169 = run(native, true);
        const auto aspect219 = run(2.3888888f, true);
        const auto aspect329 = run(32.0f / 9.0f, true);
        pass &= Check(!aspect169.gameplay.runtimeCombineMismatch &&
            !aspect169.gameplay.runtimeCombineWriteAttempted &&
            !aspect169.mutateFov &&
            std::fabs(aspect169.outputFov - 90.6557f) < 0.001f &&
            aspect219.gameplay.runtimeCombineConfirmed && aspect219.mutateFov &&
            std::fabs(aspect219.outputFov - 107.3f) < 0.15f &&
            aspect329.gameplay.runtimeCombineConfirmed && aspect329.mutateFov &&
            std::fabs(aspect329.outputFov - 127.393f) < 0.02f &&
            memory.aspectWrites == 5 && memory.aspectFlagsWrites == 0,
            "RuntimeCombine production transaction covers 16:9, 21:9, and 32:9 domains");
        return pass;
    }

    bool CheckWriterOffOrder(bool writerFirst)
    {
        using namespace camera;
        std::atomic<CoordinatorState> coordinator{CoordinatorState::Gameplay};
        cinematics::CinematicWriterAssociation association;
        cinematics::CinematicRecoveryOwner owner;
        dialogue::PostCinematicRecoveryExclusion exclusion;
        std::atomic<bool> handoffPending{};
        std::atomic<std::uintptr_t> handoffSource{};
        std::atomic<float> handoffFov{NAN};
        std::atomic<bool> continuationPending{};
        std::atomic<bool> continuationCarryPending{};
        std::atomic<std::uintptr_t> continuationSource{};
        std::atomic<float> continuationTarget{NAN};
        std::atomic<float> continuationPrevious{NAN};
        std::atomic<bool> continuationInterpolation{};
        std::atomic<float> continuationStart{NAN};
        std::atomic<std::uintptr_t> baselineSource{};
        std::atomic<float> recoveryTarget{NAN};
        std::atomic<float> recoveryCinematicTarget{NAN};
        std::atomic<bool> cinematicEnabled{};
        std::atomic<bool> cinematicSelectionValid{};
        std::atomic<std::uint64_t> cinematicGeneration{};
        std::atomic<bool> cinematicTargetEstablished{};
        std::atomic<float> cinematicAuthoredFov{NAN};
        std::atomic<float> cinematicTargetFov{NAN};
        std::atomic<std::uintptr_t> cinematicContinuationSource{};
        std::atomic<float> cinematicContinuationStart{NAN};
        std::atomic<bool> cinematicContinuationActive{};
        plugin::CameraCorrectionGate gate;
        plugin::CameraWriterRecoveryStateView writerState{coordinator, association,
            owner, exclusion, handoffPending, handoffSource, handoffFov,
            continuationPending, continuationCarryPending, continuationSource,
            continuationTarget,
            continuationPrevious, continuationInterpolation, continuationStart,
            baselineSource, recoveryTarget, recoveryCinematicTarget};
        SettingsFixture settings;
        GameplayWriterFixture gameplayState;
        ModeTransitionBarrier modeOperation{};
        modeOperation.pause = writerFirst;
        FreshFieldsFixture freshFields{0xCA};
        freshFields.barrier = &modeOperation;
        const plugin::CameraWriterEngineOperations freshReader{
            &freshFields, &ReadFreshFields, nullptr, &WriteFreshAspectAndFlags};
        gameplayState.modeTransitionPending.store(writerFirst,
            std::memory_order_release);
        plugin::CameraWriterCinematicStateView cinematicState{
            cinematicEnabled, cinematicSelectionValid, cinematicGeneration,
            cinematicTargetEstablished, cinematicAuthoredFov, cinematicTargetFov,
            cinematicContinuationSource, cinematicContinuationStart,
            cinematicContinuationActive, association};
        float finalOutput = 90.6557f;
        bool correctionApplied = false;
        const plugin::CameraWriterGameplayOperations gameplayOperations{};

        auto executeWriter = [&](config::GameplayMode mode =
                config::GameplayMode::HorPlus) {
            const plugin::CameraWriterInput input{
                0xCA, 0xCB, 90.6557f, 3.55556f, true, 0x04, true,
                0.02f, settings.gameplayEnabled.load(std::memory_order_acquire),
                mode, 16.0f / 9.0f};
            auto policyInput = input;
            policyInput.currentViewportAspect = 3.55556f;
            policyInput.currentViewportAspectValid = true;
            auto gameplayView = gameplayState.View();
            const auto observed = plugin::ProcessCameraWriter(gate, writerState,
                policyInput, cinematicState, freshReader, {}, &gameplayView,
                gameplayOperations);
            finalOutput = observed.outputFov;
            correctionApplied = observed.mutateFov;
            return observed;
        };

        if (writerFirst) {
            std::mutex readyMutex;
            std::condition_variable readyCv;
            bool setterStarted = false;
            plugin::CameraSettingTransactionResult setterResult{};
            std::atomic<bool> offReturned{};
            std::thread correction([&] {
                auto writerGuard = gate.Acquire();
                static_cast<void>(executeWriter());
            });
            {
                std::unique_lock lock(modeOperation.mutex);
                modeOperation.condition.wait(lock, [&] { return modeOperation.entered; });
            }
            std::thread setter([&] {
                {
                    std::lock_guard lock(readyMutex);
                    setterStarted = true;
                }
                readyCv.notify_one();
                setterResult = plugin::ApplyCameraSetting(gate,
                    settings.View(),
                    plugin::RuntimeSettingMutation::GameplayEnabled(false),
                    true, [] {});
                offReturned.store(true, std::memory_order_release);
            });
            bool offStayedBlocked = false;
            {
                std::unique_lock lock(readyMutex);
                readyCv.wait(lock, [&] { return setterStarted; });
                offStayedBlocked = !offReturned.load(std::memory_order_acquire);
            }
            {
                std::lock_guard lock(modeOperation.mutex);
                modeOperation.release = true;
            }
            modeOperation.condition.notify_all();
            correction.join();
            setter.join();
            return setterResult.mutation.accepted &&
                offStayedBlocked && offReturned.load(std::memory_order_acquire) &&
                !settings.gameplayEnabled.load(std::memory_order_acquire) &&
                correctionApplied && std::fabs(finalOutput - 127.393f) < 0.01f &&
                modeOperation.calls == 2;
        }

        const auto off = plugin::ApplyCameraSetting(gate, settings.View(),
            plugin::RuntimeSettingMutation::GameplayEnabled(false), true, [] {});
        {
            auto writerGuard = gate.Acquire();
            static_cast<void>(executeWriter());
        }
        bool offAllModesInert = true;
        const std::array<config::GameplayMode, 2> modes{
            config::GameplayMode::AspectRecalculation,
            config::GameplayMode::HorPlus};
        for (const auto mode : modes) {
            auto writerGuard = gate.Acquire();
            const auto result = executeWriter(mode);
            offAllModesInert &= result.gameplay.handled &&
                result.gameplay.reason == std::string_view("GAMEPLAY_DISABLED") &&
                !result.mutateFov && result.outputFov == 90.6557f;
        }
        return off.mutation.accepted &&
            !settings.gameplayEnabled.load(std::memory_order_acquire) &&
            !correctionApplied && finalOutput == 90.6557f && offAllModesInert &&
            modeOperation.calls == (writerFirst ? 1u : 0u);
    }

    bool CheckGameplayPolicyUsesValidatedViewportAspect()
    {
        using namespace camera;
        std::atomic<CoordinatorState> coordinator{CoordinatorState::Gameplay};
        cinematics::CinematicWriterAssociation association;
        cinematics::CinematicRecoveryOwner owner;
        dialogue::PostCinematicRecoveryExclusion exclusion;
        std::atomic<bool> handoffPending{};
        std::atomic<std::uintptr_t> handoffSource{};
        std::atomic<float> handoffFov{NAN};
        std::atomic<bool> continuationPending{};
        std::atomic<bool> continuationCarryPending{};
        std::atomic<std::uintptr_t> continuationSource{};
        std::atomic<float> continuationTarget{NAN};
        std::atomic<float> continuationPrevious{NAN};
        std::atomic<bool> continuationInterpolation{};
        std::atomic<float> continuationStart{NAN};
        std::atomic<std::uintptr_t> baselineSource{};
        std::atomic<float> recoveryTarget{NAN};
        std::atomic<float> recoveryCinematicTarget{NAN};
        std::atomic<bool> cinematicEnabled{};
        std::atomic<bool> cinematicSelectionValid{};
        std::atomic<std::uint64_t> cinematicGeneration{};
        std::atomic<bool> cinematicTargetEstablished{};
        std::atomic<float> cinematicAuthoredFov{NAN};
        std::atomic<float> cinematicTargetFov{NAN};
        std::atomic<std::uintptr_t> cinematicContinuationSource{};
        std::atomic<float> cinematicContinuationStart{NAN};
        std::atomic<bool> cinematicContinuationActive{};
        plugin::CameraWriterRecoveryStateView writerState{coordinator,
            association, owner, exclusion, handoffPending, handoffSource,
            handoffFov, continuationPending, continuationCarryPending,
            continuationSource, continuationTarget, continuationPrevious,
            continuationInterpolation, continuationStart, baselineSource,
            recoveryTarget, recoveryCinematicTarget};
        plugin::CameraWriterCinematicStateView cinematicState{
            cinematicEnabled, cinematicSelectionValid, cinematicGeneration,
            cinematicTargetEstablished, cinematicAuthoredFov,
            cinematicTargetFov, cinematicContinuationSource,
            cinematicContinuationStart, cinematicContinuationActive,
            association};
        plugin::CameraCorrectionGate gate;
        GameplayWriterFixture gameplayState;
        FreshFieldsFixture fields{0xCA};
        fields.aspect = 3.55556f;
        const plugin::CameraWriterEngineOperations engineOperations{
            &fields, &ReadFreshFields, nullptr, &WriteFreshAspectAndFlags};

        CameraStateSnapshot snapshot{};
        bool snapshotValid{};
        float neutralCandidate{NAN};
        std::uint32_t neutralSamples{};
        std::uintptr_t neutralSource{};
        std::atomic<bool> zoomHold{};
        std::atomic<std::uint64_t> eventSequence{};
        CameraStatePublicationView publicationState{snapshot, snapshotValid,
            neutralCandidate, neutralSamples, neutralSource, zoomHold,
            eventSequence};

        const auto publishWriter = [&](float inputFov, float nativeWriterAspect,
            float viewportAspect,
            bool viewportValid = true) {
            fields.aspect = nativeWriterAspect;
            plugin::CameraWriterInput input{0xCA, 0xCB, inputFov, nativeWriterAspect, true,
                0x04, true, 0.02f, true, config::GameplayMode::HorPlus,
                16.0f / 9.0f};
            input.currentViewportAspect = viewportAspect;
            input.currentViewportAspectValid = viewportValid;
            auto gameplayView = gameplayState.View();
            auto guard = gate.Acquire();
            const auto result = plugin::ProcessCameraWriter(gate, writerState,
                input, cinematicState, engineOperations, {}, &gameplayView);
            CameraStatePublicationInput publication{};
            publication.observationPublished = result.gameplay.observationPublished;
            publication.observation = result.gameplay.observation;
            publication.source = result.gameplay.source;
            publication.applied = result.gameplay.applied;
            publication.presentation = PresentationState::Gameplay;
            publication.dialogue = DialogueState::Inactive;
            publication.gameplayMode = static_cast<std::uint8_t>(
                config::GameplayMode::HorPlus);
            publication.nativeAspect = 16.0f / 9.0f;
            const auto published = PublishCameraStateSnapshotLocked(
                publicationState, publication);
            return std::pair{result, published};
        };

        bool pass = true;
        const auto at169 = publishWriter(90.6557f, 16.0f / 9.0f, 32.0f / 9.0f);
        const auto at219 = publishWriter(90.6557f, 2.3888888f, 16.0f / 9.0f);
        const auto at329 = publishWriter(90.6557f, 32.0f / 9.0f, 16.0f / 9.0f);
        pass &= Check(!at169.first.gameplay.applied &&
            !at169.first.mutateFov &&
            std::fabs(at169.first.outputFov - 90.6557f) < 0.001f &&
            at219.first.gameplay.applied && at219.first.mutateFov &&
            std::fabs(at219.first.outputFov - 107.3f) < 0.15f &&
            at329.first.gameplay.applied && at329.first.mutateFov &&
            std::fabs(at329.first.outputFov - 127.393f) < 0.02f,
            "production writer transaction uses native writer aspect for 16:9/21:9/32:9 despite viewport divergence");
        pass &= Check(at329.second.published && snapshot.evidence.aspectValid &&
            std::fabs(fields.aspect - 32.0f / 9.0f) < 0.0001f &&
            std::fabs(snapshot.evidence.aspect - 3.55556f) < 0.0001f &&
            snapshot.evidence.gameplayPolicyAspectValid &&
            std::fabs(snapshot.evidence.gameplayPolicyAspect - 32.0f / 9.0f) < 0.0001f &&
            snapshot.evidence.gameplayPolicyAspectProvenance ==
                EvidenceProvenance::NativeNumericObservation &&
            snapshot.evidence.currentViewportAspectValid &&
            std::fabs(snapshot.evidence.currentViewportAspect - 16.0f / 9.0f) < 0.0001f &&
            snapshot.evidence.currentViewportAspectProvenance ==
                EvidenceProvenance::ClientViewportObservation &&
            snapshot.freshness.transformedResult == SnapshotFreshness::Current,
            "snapshot keeps native policy authority and independent viewport observation/provenance");

        const auto adsWriter = publishWriter(85.0f, 32.0f / 9.0f,
            16.0f / 9.0f).first;
        pass &= Check(adsWriter.gameplay.applied &&
            std::fabs(adsWriter.outputFov - at329.first.outputFov) > 1.0f &&
            std::fabs(adsWriter.gameplay.observation.aspect.value - 3.55556f) < 0.0001f &&
            std::fabs(adsWriter.gameplay.observation.policyAspect.value - 32.0f / 9.0f) < 0.0001f &&
            adsWriter.gameplay.observation.policyAspect.provenance ==
                camera::FovProvenance::NativeWriterAspect &&
            std::fabs(adsWriter.gameplay.observation.currentViewportAspect.value - 16.0f / 9.0f) < 0.0001f,
            "ADS changes writer FOV while HorPlus authority and viewport remain separately observed");

        const auto invalidViewport = publishWriter(90.6557f,
            32.0f / 9.0f, NAN, false).first;
        pass &= Check(invalidViewport.gameplay.applied &&
            invalidViewport.mutateFov &&
            std::fabs(invalidViewport.outputFov - 127.393f) < 0.02f &&
            !invalidViewport.gameplay.observation.currentViewportAspect.valid,
            "unavailable viewport telemetry does not replace or disable native writer policy authority");

        plugin::CameraWriterInput disabledInput{0xCA, 0xCB, 90.6557f, 3.55556f,
            true, 0x04, true, 0.02f, false, config::GameplayMode::HorPlus,
            16.0f / 9.0f};
        disabledInput.currentViewportAspect = 32.0f / 9.0f;
        disabledInput.currentViewportAspectValid = true;
        auto disabledGameplayView = gameplayState.View();
        plugin::CameraWriterResult disabledWriter{};
        {
            auto guard = gate.Acquire();
            disabledWriter = plugin::ProcessCameraWriter(gate, writerState,
                disabledInput, cinematicState, engineOperations, {},
                &disabledGameplayView);
        }
        pass &= Check(!disabledWriter.gameplay.applied &&
            !disabledWriter.mutateFov && disabledWriter.outputFov == 90.6557f,
            "Gameplay OFF remains pass-through regardless of viewport observation");
        return pass;
    }

    bool CheckCameraStatePublicationAfterWriterChange()
    {
        constexpr std::uintptr_t sourceA = 0xA1;
        constexpr std::uintptr_t sourceB = 0xB1;
        std::atomic<camera::CoordinatorState> coordinator{
            camera::CoordinatorState::Gameplay};
        cinematics::CinematicWriterAssociation association;
        cinematics::CinematicRecoveryOwner owner;
        dialogue::PostCinematicRecoveryExclusion exclusion;
        std::atomic<bool> handoffPending{};
        std::atomic<std::uintptr_t> handoffSource{};
        std::atomic<float> handoffFov{NAN};
        std::atomic<bool> continuationPending{};
        std::atomic<bool> continuationCarryPending{};
        std::atomic<std::uintptr_t> continuationSource{};
        std::atomic<float> continuationTarget{NAN};
        std::atomic<float> continuationPrevious{NAN};
        std::atomic<bool> continuationInterpolation{};
        std::atomic<float> continuationStart{NAN};
        std::atomic<std::uintptr_t> baselineSource{};
        std::atomic<float> recoveryTarget{NAN};
        std::atomic<float> recoveryCinematicTarget{NAN};
        std::atomic<bool> cinematicEnabled{};
        std::atomic<bool> cinematicSelectionValid{};
        std::atomic<std::uint64_t> cinematicGeneration{};
        std::atomic<bool> cinematicTargetEstablished{};
        std::atomic<float> cinematicAuthoredFov{NAN};
        std::atomic<float> cinematicTargetFov{NAN};
        std::atomic<std::uintptr_t> cinematicContinuationSource{};
        std::atomic<float> cinematicContinuationStart{NAN};
        std::atomic<bool> cinematicContinuationActive{};
        plugin::CameraCorrectionGate gate;
        plugin::CameraWriterRecoveryStateView writerState{coordinator, association,
            owner, exclusion, handoffPending, handoffSource, handoffFov,
            continuationPending, continuationCarryPending, continuationSource,
            continuationTarget, continuationPrevious, continuationInterpolation,
            continuationStart, baselineSource, recoveryTarget,
            recoveryCinematicTarget};
        plugin::CameraWriterCinematicStateView cinematicState{
            cinematicEnabled, cinematicSelectionValid, cinematicGeneration,
            cinematicTargetEstablished, cinematicAuthoredFov, cinematicTargetFov,
            cinematicContinuationSource, cinematicContinuationStart,
            cinematicContinuationActive, association};
        SettingsFixture settings;
        GameplayWriterFixture gameplayState;
        FreshFieldsFixture fields{sourceA};
        const plugin::CameraWriterEngineOperations engineOperations{
            &fields, &ReadFreshFields, nullptr, &WriteFreshAspectAndFlags};

        std::mutex snapshotMutex;
        camera::CameraStateSnapshot snapshot{};
        bool snapshotValid{};
        float neutralCandidate{NAN};
        std::uint32_t neutralSamples{};
        std::uintptr_t neutralCandidateSource{};
        std::atomic<bool> zoomHold{};
        std::atomic<std::uint64_t> snapshotSequence{};
        overlay::PresenterWakeGate cameraOverlayWake;
        bool cameraOverlayVisible = true;
        bool cameraOverlayRefreshScheduled{};
        camera::CameraStatePublicationView publicationState{
            snapshot, snapshotValid, neutralCandidate, neutralSamples,
            neutralCandidateSource, zoomHold, snapshotSequence};
        const auto readSnapshot = [&] {
            std::lock_guard lock(snapshotMutex);
            return camera::ReadCameraStateSnapshotLocked(snapshot, snapshotValid);
        };
        CameraStateFreshnessSink freshnessSink{
            &snapshotMutex, &snapshot, &snapshotValid};
        settings.cameraStateFreshnessContext = &freshnessSink;
        settings.cameraSettingChanged = &CameraStateFreshnessSink::OnSettingChanged;
        const auto writer = [&](std::uintptr_t source, std::uintptr_t output,
            float inputFov) {
            fields.expectedSource = source;
            const plugin::CameraWriterInput input{
                source, output, inputFov, 3.55556f, true, 0x04, true,
                0.02f, settings.gameplayEnabled.load(std::memory_order_acquire),
                settings.gameplayMode.load(std::memory_order_acquire), 16.0f / 9.0f};
            auto policyInput = input;
            policyInput.currentViewportAspect = 3.55556f;
            policyInput.currentViewportAspectValid = true;
            auto gameplayView = gameplayState.View();
            auto writerGate = gate.Acquire();
            const auto result = plugin::ProcessCameraWriter(gate, writerState,
                policyInput, cinematicState, engineOperations, {}, &gameplayView);
            if (result.pairTransition.changed &&
                result.pairTransition.previous.source !=
                    result.pairTransition.current.source) {
                std::lock_guard lock(snapshotMutex);
                camera::MarkCameraWriterSourceChanged(snapshot, snapshotValid);
            }
            camera::CameraStatePublicationInput publication{};
            publication.observationPublished = result.gameplay.observationPublished;
            publication.observation = result.gameplay.observation;
            publication.source = result.gameplay.source;
            publication.applied = result.gameplay.applied;
            publication.presentation = camera::PresentationState::Gameplay;
            publication.dialogue = camera::DialogueState::Inactive;
            publication.modeTransitionPending = gameplayState.modeTransitionPending.load();
            publication.gameplayMode = static_cast<std::uint8_t>(input.gameplayMode);
            publication.nativeAspect = input.nativeAspect;
            {
                std::lock_guard lock(snapshotMutex);
                const auto published = camera::PublishCameraStateSnapshotLocked(
                    publicationState, publication);
                cameraOverlayRefreshScheduled = published.published &&
                    overlay::TryScheduleVisibleCameraStateRedraw(
                        cameraOverlayVisible, cameraOverlayWake);
            }
            return result;
        };

        bool pass = true;
        for (unsigned sample = 1; sample <= 3; ++sample) {
            const auto result = writer(sourceA, 0xA2, 90.6557f);
            const auto read = readSnapshot();
            pass &= Check(result.gameplay.observationPublished && result.mutateFov &&
                std::fabs(result.outputFov - 127.393f) < 0.01f &&
                read.valid && read.snapshot.source.gameplayWriter.value == sourceA &&
                read.snapshot.generation.eventSequence == sample &&
                 read.snapshot.evidence.nativeFov.valid &&
                 std::fabs(read.snapshot.evidence.nativeFov.value - 90.6557f) < 0.001f &&
                read.snapshot.evidence.gameplayFov.valid == (sample == 3),
                "F9 A writer publication accumulates retained FOV only on the third sample");
            pass &= Check(cameraOverlayRefreshScheduled &&
                cameraOverlayWake.pending(),
                "visible Camera State schedules from each published A writer sample");
            cameraOverlayWake.BeginHandling();
            pass &= Check(read.snapshot.freshness.writerObservation ==
                    camera::SnapshotFreshness::Current &&
                read.snapshot.freshness.transformedResult ==
                    camera::SnapshotFreshness::Current &&
                (sample == 3
                    ? read.snapshot.evidence.gameplayFov.freshness ==
                        camera::SnapshotFreshness::Current
                    : read.snapshot.evidence.gameplayFov.freshness ==
                        camera::SnapshotFreshness::Unavailable),
                "F9 initial A samples publish only facts with sufficient provenance");
        }
        const auto beforeOff = readSnapshot();
        plugin::OverlaySemanticSnapshot beforeOffSemantic{};
        beforeOffSemantic.cameraState = beforeOff.snapshot;
        beforeOffSemantic.cameraStateValid = beforeOff.valid;
        const auto beforeOffIntegration =
            overlay::ProjectCameraIntegration(beforeOffSemantic);
        pass &= Check(beforeOffIntegration.nativeFovValid &&
            beforeOffIntegration.horPlusTargetFovValid &&
            beforeOffIntegration.currentOutputFovValid &&
            beforeOffIntegration.gameplayFovValid,
            "F9 confirmed A writer facts are current before a freshness boundary");
        const auto off = plugin::ApplyCameraSetting(gate, settings.View(),
            plugin::RuntimeSettingMutation::GameplayEnabled(false), true, [] {});
        const auto afterOff = readSnapshot();
        pass &= Check(off.mutation.accepted && !settings.gameplayEnabled.load() &&
            afterOff.valid && afterOff.snapshot.generation.eventSequence ==
                beforeOff.snapshot.generation.eventSequence &&
             afterOff.snapshot.source.gameplayWriter.value == sourceA &&
             afterOff.snapshot.evidence.nativeFov.valid &&
             std::fabs(afterOff.snapshot.evidence.nativeFov.value - 90.6557f) < 0.001f &&
             afterOff.snapshot.evidence.gameplayFov.valid &&
             std::fabs(afterOff.snapshot.evidence.gameplayFov.value - 90.6557f) < 0.001f &&
             afterOff.snapshot.freshness.writerObservation ==
                camera::SnapshotFreshness::Current &&
             afterOff.snapshot.freshness.transformedResult ==
                camera::SnapshotFreshness::Historical &&
             afterOff.snapshot.evidence.gameplayFov.freshness ==
                camera::SnapshotFreshness::Retained,
            "F9 Gameplay OFF preserves the previously published A camera state");

        const auto writerB = writer(sourceB, 0xB2, 94.0f);
        const auto afterB = readSnapshot();
        const auto latest = association.Read().latestWriter;
        plugin::OverlaySemanticSnapshot semantic{};
        semantic.gameplayEnabled = {false, true,
            plugin::SemanticProvenance::RuntimeSettings,
            plugin::SemanticFreshness::AtRead};
        semantic.cameraState = afterB.snapshot;
        semantic.cameraStateValid = afterB.valid;
        const auto integration = overlay::ProjectCameraIntegration(semantic);
        pass &= Check(writerB.processed && !writerB.mutateFov &&
            writerB.outputFov == 94.0f &&
            latest.source == sourceB && latest.output == 0xB2 &&
            !writerB.gameplay.observationPublished &&
            afterB.valid && afterB.snapshot.source.gameplayWriter.value == sourceA &&
            afterB.snapshot.generation.eventSequence ==
                beforeOff.snapshot.generation.eventSequence &&
            afterB.snapshot.evidence.nativeFov.valid &&
            std::fabs(afterB.snapshot.evidence.nativeFov.value - 90.6557f) < 0.001f &&
            afterB.snapshot.evidence.gameplayFov.valid &&
            std::fabs(afterB.snapshot.evidence.gameplayFov.value - 90.6557f) < 0.001f &&
             afterB.snapshot.freshness.writerObservation ==
                camera::SnapshotFreshness::Historical &&
             afterB.snapshot.evidence.gameplayFov.freshness ==
                camera::SnapshotFreshness::Retained &&
             integration.nativeFovValid && integration.gameplayFovValid &&
             integration.horPlusTargetFovValid && integration.currentOutputFovValid &&
             integration.nativeFovFreshness == camera::SnapshotFreshness::Historical &&
             integration.gameplayFovFreshness == camera::SnapshotFreshness::Retained,
            "F9 A×3 → OFF → B projects retained and last-known role facts with freshness");
        pass &= Check(!cameraOverlayRefreshScheduled &&
            !cameraOverlayWake.pending(),
            "a writer callback without snapshot publication does not fabricate a refresh event");

        const auto on = plugin::ApplyCameraSetting(gate, settings.View(),
            plugin::RuntimeSettingMutation::GameplayEnabled(true), true, [] {});
        const auto firstBPublication = writer(sourceB, 0xB2, 94.0f);
        const auto afterFirstB = readSnapshot();
        pass &= Check(on.mutation.accepted && firstBPublication.gameplay.observationPublished &&
            afterFirstB.snapshot.source.gameplayWriter.value == sourceB &&
            afterFirstB.snapshot.freshness.writerObservation ==
                camera::SnapshotFreshness::Current &&
            afterFirstB.snapshot.freshness.transformedResult ==
                camera::SnapshotFreshness::Current &&
            afterFirstB.snapshot.evidence.gameplayFov.source == sourceA &&
            afterFirstB.snapshot.evidence.gameplayFov.freshness ==
                camera::SnapshotFreshness::Historical,
            "F9 B publication refreshes writer facts without rebinding retained A FOV");
        pass &= Check(cameraOverlayRefreshScheduled &&
            cameraOverlayWake.pending() &&
            readSnapshot().snapshot.source.gameplayWriter.value == sourceB,
            "post-transition B publication wakes visible Camera State with B facts");
        cameraOverlayWake.BeginHandling();
        const auto secondBPublication = writer(sourceB, 0xB2, 94.0f);
        const bool secondBRefreshScheduled = cameraOverlayRefreshScheduled;
        cameraOverlayWake.BeginHandling();
        const auto thirdBPublication = writer(sourceB, 0xB2, 94.0f);
        const auto afterStableB = readSnapshot();
        pass &= Check(secondBPublication.gameplay.observationPublished &&
            thirdBPublication.gameplay.observationPublished &&
            afterStableB.snapshot.evidence.gameplayFov.source == sourceB &&
            afterStableB.snapshot.evidence.gameplayFov.freshness ==
                camera::SnapshotFreshness::Current &&
            std::fabs(afterStableB.snapshot.evidence.gameplayFov.value - 94.0f) < 0.001f,
            "F9 retained FOV becomes current only after source B's own neutral confirmation");
        pass &= Check(secondBRefreshScheduled && cameraOverlayRefreshScheduled &&
            cameraOverlayWake.pending(),
            "successive B publications keep the visible semantic panel current");

        {
            std::lock_guard lock(snapshotMutex);
            camera::MarkCameraPresentationTransition(snapshot, snapshotValid);
        }
        const auto afterPresentation = readSnapshot();
        pass &= Check(afterPresentation.snapshot.freshness.presentation ==
                camera::SnapshotFreshness::Historical &&
            afterPresentation.snapshot.freshness.writerObservation ==
                camera::SnapshotFreshness::Historical &&
            afterPresentation.snapshot.evidence.nativeFov.freshness ==
                camera::SnapshotFreshness::Historical &&
            afterPresentation.snapshot.evidence.currentOutputFov.freshness ==
                camera::SnapshotFreshness::Historical &&
            afterPresentation.snapshot.evidence.gameplayFov.freshness ==
                camera::SnapshotFreshness::Retained,
            "presentation boundary retires prior writer facts but retains Gameplay baseline");

        {
            std::lock_guard lock(snapshotMutex);
            camera::CameraFovTransactionPublication cinematicTransaction{};
            cinematicTransaction.nativeInput = 26.14f;
            cinematicTransaction.nativeInputValid = true;
            cinematicTransaction.horPlusTarget = 34.65f;
            cinematicTransaction.horPlusTargetValid = true;
            cinematicTransaction.output = 34.65f;
            cinematicTransaction.outputValid = true;
            cinematicTransaction.transactionOwner =
                camera::PresentationState::CinematicActive;
            cinematicTransaction.source = sourceB;
            cinematicTransaction.sequence = snapshot.generation.eventSequence + 1;
            camera::PublishCameraFovTransactionLocked(snapshot, snapshotValid,
                cinematicTransaction);
        }
        auto afterCinematicTransaction = readSnapshot();
        pass &= Check(afterCinematicTransaction.snapshot.evidence.gameplayFov.freshness ==
                camera::SnapshotFreshness::Retained &&
            afterCinematicTransaction.snapshot.evidence.nativeFov.freshness ==
                camera::SnapshotFreshness::Current &&
            afterCinematicTransaction.snapshot.evidence.horPlusTargetFov.freshness ==
                camera::SnapshotFreshness::Current &&
            afterCinematicTransaction.snapshot.evidence.currentOutputFov.freshness ==
                camera::SnapshotFreshness::Current,
            "Cinematic transaction refreshes live FOV roles without changing Gameplay baseline lifetime");

        {
            std::lock_guard lock(snapshotMutex);
            camera::MarkCameraDialogueTransition(snapshot, snapshotValid);
        }
        const auto afterDialogue = readSnapshot();
        pass &= Check(afterDialogue.snapshot.freshness.dialogue ==
                camera::SnapshotFreshness::Historical &&
            afterDialogue.snapshot.freshness.writerObservation ==
                camera::SnapshotFreshness::Historical &&
            afterDialogue.snapshot.evidence.nativeFov.freshness ==
                camera::SnapshotFreshness::Current &&
            afterDialogue.snapshot.evidence.currentOutputFov.freshness ==
                camera::SnapshotFreshness::Current,
            "Dialogue lifecycle transition does not invalidate independent live FOV transaction facts");

        {
            std::lock_guard lock(snapshotMutex);
            camera::CameraFovTransactionPublication continuation{};
            continuation.nativeInput = 26.14f;
            continuation.nativeInputValid = true;
            continuation.horPlusTarget = 34.65f;
            continuation.horPlusTargetValid = true;
            continuation.output = 34.65f;
            continuation.outputValid = true;
            continuation.transactionOwner =
                camera::PresentationState::CinematicActive;
            continuation.source = sourceB;
            continuation.sequence = snapshot.generation.eventSequence + 1;
            camera::PublishCameraFovTransactionLocked(snapshot, snapshotValid,
                continuation);

            camera::CameraFovTransactionPublication recovery{};
            recovery.nativeInput = 30.0f;
            recovery.nativeInputValid = true;
            recovery.horPlusTarget = 34.65f;
            recovery.horPlusTargetValid = true;
            recovery.output = 31.2f;
            recovery.outputValid = true;
            recovery.transactionOwner =
                camera::PresentationState::CinematicExiting;
            recovery.source = sourceB;
            recovery.sequence = continuation.sequence + 1;
            camera::PublishCameraFovTransactionLocked(snapshot, snapshotValid,
                recovery);
        }
        const auto duringRecovery = readSnapshot();
        pass &= Check(duringRecovery.snapshot.evidence.gameplayFov.freshness ==
                camera::SnapshotFreshness::Retained &&
            duringRecovery.snapshot.evidence.nativeFov.freshness ==
                camera::SnapshotFreshness::Current &&
            duringRecovery.snapshot.evidence.horPlusTargetFov.freshness ==
                camera::SnapshotFreshness::Current &&
            duringRecovery.snapshot.evidence.currentOutputFov.freshness ==
                camera::SnapshotFreshness::Current &&
            std::fabs(duringRecovery.snapshot.evidence.horPlusTargetFov.value -
                duringRecovery.snapshot.evidence.currentOutputFov.value) > 1.0f,
            "Cinematic continuation/recovery transactions keep target and actual output distinct");

        {
            std::lock_guard lock(snapshotMutex);
            camera::MarkCameraPresentationTransition(snapshot, snapshotValid);
        }
        const auto afterRecoveryBoundary = readSnapshot();
        pass &= Check(afterRecoveryBoundary.snapshot.evidence.nativeFov.freshness ==
                camera::SnapshotFreshness::Historical &&
            afterRecoveryBoundary.snapshot.evidence.currentOutputFov.freshness ==
                camera::SnapshotFreshness::Historical &&
            afterRecoveryBoundary.snapshot.evidence.gameplayFov.freshness ==
                camera::SnapshotFreshness::Retained,
            "EXIT/recovery boundary retires Cinematic live facts without dropping retained Gameplay FOV");
        static_cast<void>(writer(sourceB, 0xB2, 94.0f));
        const auto afterGameplayHandoff = readSnapshot();
        pass &= Check(afterGameplayHandoff.snapshot.evidence.nativeFov.freshness ==
                camera::SnapshotFreshness::Current &&
            afterGameplayHandoff.snapshot.evidence.currentOutputFov.freshness ==
                camera::SnapshotFreshness::Current &&
            afterGameplayHandoff.snapshot.freshness.writerObservation ==
                camera::SnapshotFreshness::Current,
            "first full Gameplay publication after recovery re-establishes live writer facts");

        camera::CameraStatePublicationInput zoomObservation{};
        zoomObservation.observationPublished =
            thirdBPublication.gameplay.observationPublished;
        zoomObservation.observation = thirdBPublication.gameplay.observation;
        zoomObservation.source = sourceB;
        zoomObservation.applied = thirdBPublication.gameplay.applied;
        zoomObservation.presentation = camera::PresentationState::Gameplay;
        zoomObservation.dialogue = camera::DialogueState::Inactive;
        zoomObservation.zoomActive = true;
        zoomObservation.zoomObservationValid = true;
        zoomObservation.zoomDirection = camera::ZoomDirection::In;
        zoomObservation.zoomSequence = 1;
        zoomObservation.zoomSource = sourceB;
        zoomObservation.zoomPrimary = 0.25f;
        zoomObservation.zoomSecondary = 0.75f;
        zoomObservation.gameplayMode = static_cast<std::uint8_t>(
            config::GameplayMode::HorPlus);
        zoomObservation.nativeAspect = 16.0f / 9.0f;
        {
            std::lock_guard lock(snapshotMutex);
            static_cast<void>(camera::PublishCameraStateSnapshotLocked(
                publicationState, zoomObservation));
        }
        const auto beforeZoomTransition = readSnapshot();
        pass &= Check(beforeZoomTransition.snapshot.freshness.zoom ==
                camera::SnapshotFreshness::Current,
            "F9 zoom event publication establishes current zoom provenance");
        {
            std::lock_guard lock(snapshotMutex);
            camera::MarkCameraZoomTransition(snapshot, snapshotValid);
        }
        const auto afterZoom = readSnapshot();
        pass &= Check(afterZoom.snapshot.freshness.zoom ==
                camera::SnapshotFreshness::Historical &&
            afterZoom.snapshot.evidence.gameplayFov.freshness ==
                camera::SnapshotFreshness::Retained &&
            afterZoom.snapshot.freshness.writerObservation ==
                camera::SnapshotFreshness::Current,
            "F9 zoom transition before next writer marks zoom and retained FOV historical");

        camera::CameraStateSnapshot roleSnapshot{};
        roleSnapshot.presentation.state = camera::PresentationState::Gameplay;
        roleSnapshot.presentation.provenance =
            camera::EvidenceProvenance::ModDerivedState;
        roleSnapshot.evidence.gameplayFov = {90.0f, true,
            camera::EvidenceProvenance::NativeNumericObservation,
            camera::SnapshotFreshness::Current, sourceA, true, 10};
        bool roleSnapshotValid = false;
        camera::CameraFovTransactionPublication roles{};
        roles.nativeInput = 90.0f;
        roles.nativeInputValid = true;
        roles.horPlusTarget = 107.0f;
        roles.horPlusTargetValid = true;
        roles.output = 107.0f;
        roles.outputValid = true;
        roles.transactionOwner = camera::PresentationState::Gameplay;
        roles.source = sourceA;
        roles.sequence = 11;
        camera::PublishCameraFovTransactionLocked(roleSnapshot,
            roleSnapshotValid, roles);
        roles.transactionOwner = camera::PresentationState::CinematicActive;
        roles.nativeInput = 107.0f;
        roles.output = 107.0f;
        roles.sequence = 12;
        camera::PublishCameraFovTransactionLocked(roleSnapshot,
            roleSnapshotValid, roles);
        pass &= Check(roleSnapshotValid && roleSnapshot.evidence.nativeFov.valid &&
            roleSnapshot.evidence.nativeFov.value == 107.0f &&
            roleSnapshot.evidence.horPlusTargetFov.valid &&
            roleSnapshot.evidence.currentOutputFov.value == 107.0f &&
            roleSnapshot.evidence.gameplayFov.freshness ==
                camera::SnapshotFreshness::Retained &&
            roleSnapshot.presentation.state == camera::PresentationState::Gameplay,
            "cinematic_transaction_retains_gameplay_baseline_without_relabeling_last_full_publication_state");

        roles.nativeInput = 26.14f;
        roles.horPlusTargetValid = false;
        roles.output = 34.65f;
        roles.sequence = 13;
        camera::PublishCameraFovTransactionLocked(roleSnapshot,
            roleSnapshotValid, roles);
        pass &= Check(roleSnapshot.evidence.nativeFov.value == 26.14f &&
            roleSnapshot.evidence.currentOutputFov.value == 34.65f &&
            roleSnapshot.evidence.horPlusTargetFov.value == 107.0f &&
            roleSnapshot.evidence.horPlusTargetFov.freshness ==
                camera::SnapshotFreshness::Retained,
            "Dialogue-like output remains distinct from retained HorPlus target");

        roles.nativeInput = std::numeric_limits<float>::quiet_NaN();
        roles.nativeInputValid = false;
        roles.output = std::numeric_limits<float>::quiet_NaN();
        roles.outputValid = false;
        roles.sequence = 14;
        camera::PublishCameraFovTransactionLocked(roleSnapshot,
            roleSnapshotValid, roles);
        pass &= Check(!roleSnapshot.evidence.nativeFov.valid &&
            !roleSnapshot.evidence.currentOutputFov.valid &&
            roleSnapshot.evidence.gameplayFov.valid &&
            roleSnapshot.evidence.gameplayFov.freshness ==
                camera::SnapshotFreshness::Retained,
            "invalid transaction values fail closed without erasing retained Gameplay baseline");

        const auto beforeMode = readSnapshot();
        const auto modeChange = plugin::ApplyCameraSetting(gate, settings.View(),
            plugin::RuntimeSettingMutation::Gameplay(
                config::GameplayMode::AspectRecalculation), true, [] {});
        const auto afterMode = readSnapshot();
        pass &= Check(modeChange.mutation.accepted && afterMode.valid &&
            afterMode.snapshot.generation.eventSequence ==
                beforeMode.snapshot.generation.eventSequence &&
            afterMode.snapshot.freshness.gameplayMode ==
                camera::SnapshotFreshness::Historical &&
            afterMode.snapshot.freshness.transformedResult ==
                camera::SnapshotFreshness::Historical &&
            afterMode.snapshot.freshness.writerObservation ==
                camera::SnapshotFreshness::Current,
            "F9 mode change without writer publication stales only dependent groups");
        return pass;
    }
}

int main()
{
    using namespace camera;
    using namespace cinematics;

    std::atomic<CoordinatorState> coordinator{CoordinatorState::CinematicActive};
    CinematicWriterAssociation association;
    CinematicRecoveryOwner owner;
    dialogue::PostCinematicRecoveryExclusion exclusion;
    std::atomic<bool> handoffPending{true};
    std::atomic<std::uintptr_t> handoffSource{0xA};
    std::atomic<float> handoffFov{92.0f};
    std::atomic<bool> continuationPending{true};
    std::atomic<bool> continuationCarryPending{};
    std::atomic<std::uintptr_t> continuationSource{0xA};
    std::atomic<float> continuationExitTarget{127.0f};
    std::atomic<float> continuationPreviousTarget{126.0f};
    std::atomic<bool> continuationInterpolation{true};
    std::atomic<float> continuationStart{90.0f};
    std::atomic<std::uintptr_t> gameplayBaselineSource{0xA};
    std::atomic<float> exitTarget{90.0f};
    std::atomic<float> exitCinematicTarget{127.0f};
    GameplayWriterFixture gameplayState;
    FreshFieldsFixture freshFields{0xA};
    const plugin::CameraWriterEngineOperations freshReader{
        &freshFields, &ReadFreshFields, nullptr, &WriteFreshAspectAndFlags};
    std::atomic<bool> cinematicEnabled{};
    std::atomic<std::uint64_t> cinematicSettingsEpoch{};
    std::atomic<config::CinematicAspectPolicy> cinematicAspectPolicy{
        config::CinematicAspectPolicy::Auto};
    std::atomic<config::CinematicFovMode> cinematicFovMode{
        config::CinematicFovMode::GameplayHorPlus};
    std::atomic<bool> cinematicSelectionValid{};
    std::atomic<std::uint64_t> cinematicGeneration{};
    std::atomic<bool> cinematicTargetEstablished{};
    std::atomic<float> cinematicAuthoredFov{NAN};
    std::atomic<float> preEnterNativeFov{NAN};
    std::atomic<float> preEnterHorPlusFov{NAN};
    std::atomic<float> preEnterAspect{NAN};
    std::atomic<bool> preEnterPairValid{};
    std::atomic<float> cinematicTargetFov{NAN};
    std::atomic<std::uintptr_t> cinematicContinuationSource{};
    std::atomic<float> cinematicContinuationStart{NAN};
    std::atomic<bool> cinematicContinuationActive{};
    plugin::CameraCorrectionGate gate;
    plugin::CameraWriterRecoveryStateView state{coordinator, association, owner, exclusion,
        handoffPending, handoffSource, handoffFov, continuationPending,
        continuationCarryPending,
        continuationSource, continuationExitTarget, continuationPreviousTarget,
        continuationInterpolation, continuationStart, gameplayBaselineSource,
        exitTarget, exitCinematicTarget};
    plugin::CameraWriterCinematicStateView cinematicState{
        cinematicEnabled, cinematicSelectionValid, cinematicGeneration,
        cinematicTargetEstablished, cinematicAuthoredFov, cinematicTargetFov,
        cinematicContinuationSource, cinematicContinuationStart,
        cinematicContinuationActive, association};
    plugin::CinematicLifecycleStateView cinematicLifecycleState{
        coordinator, cinematicSelectionValid, cinematicGeneration,
        cinematicEnabled, cinematicSettingsEpoch, cinematicAspectPolicy,
        cinematicFovMode, cinematicTargetEstablished, cinematicTargetFov,
        exitTarget, exitCinematicTarget, cinematicAuthoredFov,
        preEnterNativeFov, preEnterHorPlusFov, preEnterAspect,
        preEnterPairValid, gameplayBaselineSource, continuationPending,
        continuationCarryPending,
        continuationSource, continuationExitTarget, continuationPreviousTarget,
        continuationInterpolation, cinematicContinuationStart, association,
        owner, exclusion, handoffPending, handoffSource, handoffFov};

    bool pass = true;
    association.BeginGeneration(4, 90.0f, 127.0f,
        {true, config::CinematicAspectPolicy::Auto,
            config::CinematicFovMode::GameplayHorPlus});
    association.ObserveAspectStoreTarget(4, 0xA);
    static_cast<void>(association.ObserveWriter(CoordinatorState::CinematicActive,
        0xA, 0x10, 90.0f, 3.55556f, true, 0x4, true));
    pass &= Check(owner.Capture(association.Read(), 4),
        "accepted lifecycle primitives establish owner A");
    exclusion.Arm(90.0f);
    const auto exit = gameplay::ResolveCinematicExitTransition(true, true,
        config::GameplayMode::HorPlus);
    coordinator.store(exit.nextState, std::memory_order_release);

    plugin::CameraWriterResult sameSource{};
    {
        auto writerGate = gate.Acquire();
        auto gameplayView = gameplayState.View();
        sameSource = plugin::ProcessCameraWriter(gate, state,
            {0xA, 0x10, 92.0f, 3.55556f, true, 0x4, true}, cinematicState,
            freshReader, {}, &gameplayView);
    }
    pass &= Check(sameSource.processed && !sameSource.recoveryRetired &&
        coordinator.load(std::memory_order_acquire) == CoordinatorState::CinematicExiting &&
        owner.Read().HasRecoveryAuthority(),
        "owner A keeps recovery on an A writer sample under correction gate");

    plugin::CameraWriterResult foreign{};
    {
        freshFields.expectedSource = 0xB;
        auto writerGate = gate.Acquire();
        auto gameplayView = gameplayState.View();
        plugin::CameraWriterInput foreignInput{
            0xB, 0x20, 90.6557f, 3.55556f, true, 0x4, true};
        foreignInput.currentViewportAspect = 3.55556f;
        foreignInput.currentViewportAspectValid = true;
        foreign = plugin::ProcessCameraWriter(gate, state, foreignInput, cinematicState,
            freshReader, {}, &gameplayView);
    }
    pass &= Check(foreign.recoveryRetired &&
        foreign.coordinatorAfter == CoordinatorState::Gameplay &&
        coordinator.load(std::memory_order_acquire) == CoordinatorState::Gameplay,
        "A to B retires A authority and returns coordinator to Gameplay");
    pass &= Check(!owner.Read().valid && !exclusion.IsActive() &&
        !handoffPending.load() && handoffSource.load() == 0 &&
        std::isnan(handoffFov.load()) && !continuationPending.load() &&
        continuationSource.load() == 0 && std::isnan(continuationExitTarget.load()) &&
        std::isnan(continuationPreviousTarget.load()) &&
        !continuationInterpolation.load() && std::isnan(continuationStart.load()) &&
        gameplayBaselineSource.load() == 0 && std::isnan(exitTarget.load()) &&
        std::isnan(exitCinematicTarget.load()),
        "retirement clears every recovery-dependent field");
    pass &= Check(foreign.gameplay.handled && foreign.mutateFov &&
        std::fabs(foreign.outputFov - 127.393f) < 0.01f,
        "the same post-retirement writer transaction applies Gameplay HorPlus output");

    auto runGameplayWriter = [&](float fov, float aspect, std::uint8_t flags,
        bool fieldsReadable = true,
        config::GameplayMode mode = config::GameplayMode::HorPlus) {
        freshFields.expectedSource = 0xB;
        freshFields.aspect = aspect;
        freshFields.flags = flags;
        freshFields.readable = fieldsReadable;
        plugin::CameraWriterInput input{
            0xB, 0x20, fov, aspect, true, flags, true,
            0.02f, true, mode, 16.0f / 9.0f};
        input.currentViewportAspect = aspect;
        input.currentViewportAspectValid = true;
        auto gameplayView = gameplayState.View();
        auto writerGate = gate.Acquire();
        return plugin::ProcessCameraWriter(gate, state, input, cinematicState,
            freshReader, {}, &gameplayView);
    };
    const auto horPlusWriter = runGameplayWriter(90.6557f, 3.55556f, 0x04);
    pass &= Check(horPlusWriter.gameplay.handled && horPlusWriter.gameplay.applied &&
        horPlusWriter.mutateFov &&
        std::fabs(horPlusWriter.outputFov - 127.393f) < 0.01f &&
        horPlusWriter.gameplay.observationPublished &&
        horPlusWriter.gameplay.baselineProjection.baseline.valid,
        "G01 production transaction returns HorPlus output and publishes observation/baseline");
    const auto nativeAspectWriter = runGameplayWriter(90.6557f, 16.0f / 9.0f, 0x04);
    const auto invalidFlagsWriter = runGameplayWriter(90.6557f, 3.55556f, 0x00);
    const auto invalidFovWriter = runGameplayWriter(180.0f, 3.55556f, 0x04);
    const auto unreadableWriter = runGameplayWriter(90.6557f, 3.55556f, 0x04, false);
    pass &= Check(!nativeAspectWriter.mutateFov &&
        nativeAspectWriter.outputFov == 90.6557f &&
        nativeAspectWriter.gameplay.reason == std::string_view("INPUT_BYPASS") &&
        !invalidFlagsWriter.mutateFov && invalidFlagsWriter.outputFov == 90.6557f &&
        !invalidFovWriter.mutateFov && invalidFovWriter.outputFov == 180.0f &&
        !invalidFovWriter.gameplay.observationPublished &&
        unreadableWriter.gameplay.reason == std::string_view("SOURCE_READ_FAILED") &&
        !unreadableWriter.mutateFov && unreadableWriter.outputFov == 90.6557f,
        "G02 native aspect, invalid flags/FOV and unreadable fields fail through production transaction");

    freshFields.expectedSource = 0xB;
    freshFields.calls = 0;
    freshFields.useAspectSequence = true;
    freshFields.aspectSequence = {16.0f / 9.0f, 3.55556f, 0.0f, 0.0f};
    freshFields.aspectFlagsWriteSucceeds = true;
    freshFields.aspectFlagsWriteCalls = 0;
    gameplayState.aspectRestoration.Update(3.55556f, {0xB, true});
    gameplayState.modeTransitionTarget.store(config::GameplayMode::HorPlus,
        std::memory_order_release);
    gameplayState.modeTransitionPending.store(true, std::memory_order_release);
    const auto restoreModeTransition = runGameplayWriter(90.6557f,
        3.55556f, 0x04);
    pass &= Check(restoreModeTransition.gameplay.modeTransition.event ==
            plugin::GameplayModeTransitionEvent::RestoreApplied &&
        restoreModeTransition.gameplay.modeTransitionPending == false &&
        !gameplayState.modeTransitionPending.load(std::memory_order_acquire) &&
        freshFields.calls == 2 && freshFields.aspectFlagsWriteCalls == 1 &&
        restoreModeTransition.mutateFov &&
        std::fabs(restoreModeTransition.outputFov - 127.393f) < 0.01f,
        "mode transition restores native aspect before the same transaction resolves HorPlus output");

    freshFields.calls = 0;
    freshFields.aspectSequence = {16.0f / 9.0f, 16.0f / 9.0f, 0.0f, 0.0f};
    freshFields.aspectFlagsWriteSucceeds = false;
    freshFields.aspectFlagsWriteCalls = 0;
    gameplayState.modeTransitionPending.store(true, std::memory_order_release);
    const auto failedModeWrite = runGameplayWriter(90.6557f,
        16.0f / 9.0f, 0x04);
    pass &= Check(failedModeWrite.gameplay.modeTransition.event ==
            plugin::GameplayModeTransitionEvent::RestoreWriteFailed &&
        failedModeWrite.gameplay.modeTransitionPending &&
        gameplayState.modeTransitionPending.load(std::memory_order_acquire) &&
        freshFields.calls == 2 && freshFields.aspectFlagsWriteCalls == 1 &&
        !failedModeWrite.mutateFov && failedModeWrite.outputFov == 90.6557f,
        "failed native restoration write preserves pending mode transition and native output");
    gameplayState.modeTransitionPending.store(false, std::memory_order_release);
    freshFields.useAspectSequence = false;

    freshFields.aspectFlagsWriteSucceeds = true;
    freshFields.aspectFlagsWriteCalls = 0;
    gameplayState.replayState.store(gameplay::ReplayState::Complete,
        std::memory_order_release);
    const auto constrainedPass = runGameplayWriter(90.6557f, 2.33333f, 0x5,
        true, config::GameplayMode::AspectRecalculation);
    pass &= Check(constrainedPass.gameplay.handled &&
        constrainedPass.gameplay.constrainedAspectPass &&
        constrainedPass.gameplay.nativeAspectWriteSucceeded &&
        freshFields.aspectFlagsWriteCalls == 1 &&
        !constrainedPass.mutateFov && constrainedPass.outputFov == 90.6557f,
        "AspectRecalculation constrained pass requests native aspect/flags write and preserves FOV");

    gameplayState.replayState.store(gameplay::ReplayState::WaitingForAutomaticUpdate,
        std::memory_order_release);
    freshFields.aspectFlagsWriteCalls = 0;
    const auto automaticRestore = runGameplayWriter(90.6557f, 3.55556f, 0x4,
        true, config::GameplayMode::AspectRecalculation);
    pass &= Check(automaticRestore.gameplay.nativeAspectWriteAttempted &&
        automaticRestore.gameplay.nativeAspectWriteSucceeded &&
        gameplayState.replayState.load(std::memory_order_acquire) ==
            gameplay::ReplayState::Complete &&
        gameplayState.lastAutoRestoreSource.load(std::memory_order_acquire) == 0xB &&
        freshFields.aspectFlagsWriteCalls == 1 && !automaticRestore.mutateFov &&
        automaticRestore.outputFov == 90.6557f,
        "AspectRecalculation automatic native restore commits state only after successful engine write");

    gameplayState.replayState.store(gameplay::ReplayState::WaitingForAutomaticUpdate,
        std::memory_order_release);
    freshFields.aspectFlagsWriteSucceeds = false;
    freshFields.aspectFlagsWriteCalls = 0;
    const auto failedAutomaticRestore = runGameplayWriter(90.6557f, 3.55556f, 0x4,
        true, config::GameplayMode::AspectRecalculation);
    pass &= Check(failedAutomaticRestore.gameplay.nativeAspectWriteAttempted &&
        !failedAutomaticRestore.gameplay.nativeAspectWriteSucceeded &&
        gameplayState.replayState.load(std::memory_order_acquire) ==
            gameplay::ReplayState::WaitingForAutomaticUpdate &&
        freshFields.aspectFlagsWriteCalls == 1 && !failedAutomaticRestore.mutateFov,
        "failed AspectRecalculation write leaves AutomaticUpdate pending for retry");
    freshFields.aspectFlagsWriteSucceeds = true;
    GameplayWriterFixture successfulHandoffGameplayState;
    GameplayWriterFixture failedHandoffGameplayState;
    GameplayWriterFixture endpointGameplayState;
    auto prepareAtomicHandoff = [&](bool gameplayEnabledAtExit,
        GameplayWriterFixture& scenarioGameplayState) {
        const auto selection = plugin::PrepareCinematicEnter(cinematicLifecycleState,
            config::CinematicAspectPolicy::Forced21x9,
            config::CinematicFovMode::GameplayHorPlus);
        plugin::AcceptedCinematicEnterInput enterInput{};
        enterInput.selection = selection;
        enterInput.correctionEnabled = true;
        enterInput.settingsEpoch = 1;
        enterInput.authoredFov = 90.6557f;
        enterInput.effectiveAspect = 3.55556f;
        enterInput.nativeAspect = 16.0f / 9.0f;
        const auto enter = plugin::ProcessAcceptedCinematicEnter(
            cinematicLifecycleState, enterInput);
        pass &= Check(enter.accepted &&
            coordinator.load(std::memory_order_acquire) ==
                CoordinatorState::CinematicActive &&
            cinematicSelectionValid.load(std::memory_order_acquire) &&
            cinematicGeneration.load(std::memory_order_acquire) == enter.generation,
            "atomic handoff setup obtains active generation through production ENTER event");
        const auto aspectStore = plugin::ObserveCinematicAspectStore(
            cinematicLifecycleState, {0xA, true, true, false, true, true,
                16.0f / 9.0f});
        pass &= Check(aspectStore.accepted &&
            association.Read().aspectStoreTargetObject == 0xA,
            "atomic handoff setup obtains target relation through production aspect-store event");
        freshFields.expectedSource = 0xA;
        freshFields.aspect = 3.55556f;
        freshFields.flags = 0x4;
        freshFields.readable = true;
        freshFields.aspectFlagsWriteCalls = 0;
        const plugin::CameraWriterInput cinematicInput{
            0xA, 0x10, 90.6557f, 3.55556f, true, 0x4, true,
            0.02f, true, config::GameplayMode::AspectRecalculation,
            16.0f / 9.0f};
        auto cinematicGameplayView = scenarioGameplayState.View();
        auto cinematicGate = gate.Acquire();
        const auto cinematicWriter = plugin::ProcessCameraWriter(gate, state,
            cinematicInput, cinematicState, freshReader, {},
            &cinematicGameplayView);
        pass &= Check(cinematicWriter.cinematicHandled &&
            association.Read().associationValid &&
            association.Read().associatedWriterRsi == 0xA,
            "atomic handoff setup associates writer through production writer transaction");
        const plugin::AcceptedCinematicExitInput exitInput{
            coordinator.load(std::memory_order_acquire),
            CoordinatorState::CinematicExiting, enter.generation, 90.6557f,
            gameplayEnabledAtExit, config::GameplayMode::AspectRecalculation,
            0xA, cinematicWriter.outputFov};
        const auto exit = plugin::ProcessAcceptedCinematicExit(
            cinematicLifecycleState, exitInput);
        pass &= Check(exit.accepted && exit.recoveryOwnerCaptured &&
            coordinator.load(std::memory_order_acquire) ==
                CoordinatorState::CinematicExiting &&
            owner.Read().HasRecoveryAuthority() &&
            handoffPending.load(std::memory_order_acquire) == gameplayEnabledAtExit,
            "atomic handoff setup obtains recovery owner and pending state through production EXIT event");
        freshFields.expectedSource = 0xA;
        freshFields.aspect = 3.55556f;
        freshFields.flags = 0x4;
        freshFields.readable = true;
    };
    auto runAtomicHandoffWriter = [&](float fov,
        GameplayWriterFixture& scenarioGameplayState) {
        auto gameplayView = scenarioGameplayState.View();
        const plugin::CameraWriterInput input{
            0xA, 0x10, fov, 3.55556f, true, 0x4, true,
            0.02f, true, config::GameplayMode::AspectRecalculation, 16.0f / 9.0f};
        auto writerGate = gate.Acquire();
        return plugin::ProcessCameraWriter(gate, state, input, cinematicState,
            freshReader, {}, &gameplayView);
    };
    prepareAtomicHandoff(true, successfulHandoffGameplayState);
    const auto handoffFirstSample = runAtomicHandoffWriter(126.0f,
        successfulHandoffGameplayState);
    const auto handoffApplied = runAtomicHandoffWriter(120.0f,
        successfulHandoffGameplayState);
    pass &= Check(handoffFirstSample.atomicExitHandoff.event ==
            plugin::AtomicExitHandoffEvent::FirstSample &&
        handoffApplied.atomicExitHandoff.event ==
            plugin::AtomicExitHandoffEvent::Applied &&
        handoffApplied.atomicExitHandoff.writeSucceeded &&
        freshFields.aspectFlagsWriteCalls == 1 &&
        successfulHandoffGameplayState.replayState.load(std::memory_order_acquire) ==
            gameplay::ReplayState::Complete &&
        successfulHandoffGameplayState.lastAutoRestoreSource.load(std::memory_order_acquire) == 0xA,
        "event-only ENTER/EXIT then writer transaction applies atomic handoff and commits replay state after native write");

    prepareAtomicHandoff(true, failedHandoffGameplayState);
    freshFields.aspectFlagsWriteSucceeds = false;
    static_cast<void>(runAtomicHandoffWriter(126.0f,
        failedHandoffGameplayState));
    const auto handoffWriteFailed = runAtomicHandoffWriter(120.0f,
        failedHandoffGameplayState);
    pass &= Check(handoffWriteFailed.atomicExitHandoff.event ==
            plugin::AtomicExitHandoffEvent::WriteFailed &&
        !handoffWriteFailed.atomicExitHandoff.writeSucceeded &&
        freshFields.aspectFlagsWriteCalls == 1 &&
        failedHandoffGameplayState.replayState.load(std::memory_order_acquire) ==
            gameplay::ReplayState::WaitingForAutomaticUpdate &&
        failedHandoffGameplayState.lastAutoRestoreSource.load(std::memory_order_acquire) == 0,
        "failed atomic EXIT handoff write does not commit Gameplay replay state");
    freshFields.aspectFlagsWriteSucceeds = true;

    prepareAtomicHandoff(false, endpointGameplayState);
    const plugin::CameraWriterInput aspectEndpointInput{
        0xA, 0x10, 90.6557f, 3.55556f, true, 0x4, true,
        0.02f, false, config::GameplayMode::AspectRecalculation, 16.0f / 9.0f};
    plugin::CameraWriterResult aspectEndpoint{};
    {
        auto gameplayView = endpointGameplayState.View();
        auto writerGate = gate.Acquire();
        aspectEndpoint = plugin::ProcessCameraWriter(gate, state,
            aspectEndpointInput, cinematicState, freshReader, {}, &gameplayView);
    }
    pass &= Check(aspectEndpoint.aspectRecoveryEndpoint &&
        aspectEndpoint.horPlusRecovery.completed &&
        coordinator.load(std::memory_order_acquire) == CoordinatorState::Gameplay &&
        !owner.Read().HasRecoveryAuthority() && !aspectEndpoint.mutateFov &&
        aspectEndpoint.outputFov == 90.6557f,
        "AspectRecalculation recovery endpoint completes owner and returns native FOV with Gameplay OFF");
    pass &= Check(CheckWriterOffOrder(true),
        "writer-first commits shared HorPlus output before OFF linearizes");
    pass &= Check(CheckWriterOffOrder(false),
        "OFF-first reaches shared writer policy as pass-through");
    pass &= Check(CheckGameplayPolicyUsesValidatedViewportAspect(),
        "Gameplay HorPlus production transaction uses validated viewport policy aspect independently of native writer aspect");
    pass &= Check(CheckRuntimeCombineProductionTransaction(),
        "RuntimeCombine production writer transaction confirms aspect synchronization before HorPlus");
    pass &= Check(CheckCameraStatePublicationAfterWriterChange(),
        "F9 production writer and shared snapshot publisher characterize stale Overlay state");

    EngineOperationFixture effectFixture{};
    const plugin::CameraWriterEngineOperations engineOperations{
        &effectFixture, &RecordEngineRead, &RecordAspectOnlyWrite,
        &RecordAspectFlagsWrite};
    const auto aspectOnlyFields = plugin::ReadCameraWriterAspectFlags(
        engineOperations, 0xA, false);
    const bool aspectOnlySucceeded = plugin::RequestCameraWriterAspectOnly(
        engineOperations, 0xA, 16.0f / 9.0f);
    const bool aspectFlagsSucceeded = plugin::RequestCameraWriterAspectAndFlags(
        engineOperations, 0xA, 3.55556f, 0x04);
    pass &= Check(effectFixture.count == 3 &&
        effectFixture.operations[0] == 2u && effectFixture.operations[1] == 3u &&
        effectFixture.operations[2] == 4u && aspectOnlyFields.aspectReadable &&
        !aspectOnlyFields.flagsReadable && aspectOnlySucceeded &&
        !aspectFlagsSucceeded,
        "engine operation boundary preserves read/write order and explicit success/failure outcomes");

    std::cout << (pass ? "PASS" : "FAIL") << ": camera writer transaction\n";
    return pass ? 0 : 1;
}
