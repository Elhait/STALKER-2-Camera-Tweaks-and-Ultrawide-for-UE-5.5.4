#pragma once

#include "camera_writer_transaction.hpp"
#include "../gameplay/gameplay_state.hpp"
#include "../cinematics/cinematic_fov.hpp"
#include "../cinematics/cinematic_aspect.hpp"
#include "../cinematics/cinematic_selection.hpp"
#include "../cinematics/cinematic_exit_authority.hpp"

namespace plugin
{
    struct CinematicLifecycleStateView
    {
        std::atomic<camera::CoordinatorState>& coordinator;
        std::atomic<bool>& selectionValid;
        std::atomic<std::uint64_t>& selectionGeneration;
        std::atomic<bool>& activeEnabled;
        std::atomic<std::uint64_t>& activeSettingsEpoch;
        std::atomic<config::CinematicAspectPolicy>& activeAspectPolicy;
        std::atomic<config::CinematicFovMode>& activeFovMode;
        std::atomic<bool>& fovApplied;
        std::atomic<float>& transformedFov;
        std::atomic<float>& exitTargetFov;
        std::atomic<float>& exitTransformedFov;
        std::atomic<float>& enterNativeFov;
        std::atomic<float>& preEnterNativeFov;
        std::atomic<float>& preEnterHorPlusFov;
        std::atomic<float>& preEnterAspect;
        std::atomic<bool>& preEnterPairValid;
        std::atomic<std::uintptr_t>& gameplayContinuationSource;
        std::atomic<bool>& continuationPending;
        std::atomic<bool>& continuationCarryPending;
        std::atomic<std::uintptr_t>& continuationSource;
        std::atomic<float>& continuationExitNativeTarget;
        std::atomic<float>& continuationPreviousTarget;
        std::atomic<bool>& continuationInterpolationActive;
        std::atomic<float>& continuationStartFov;
        cinematics::CinematicWriterAssociation& association;
        cinematics::CinematicRecoveryOwner& recoveryOwner;
        dialogue::PostCinematicRecoveryExclusion& exclusion;
        std::atomic<bool>& handoffPending;
        std::atomic<std::uintptr_t>& handoffSource;
        std::atomic<float>& handoffPreviousFov;

        CameraWriterRecoveryStateView WriterRecoveryView() const
        {
            return {coordinator, association, recoveryOwner, exclusion,
                handoffPending, handoffSource, handoffPreviousFov,
                continuationPending, continuationCarryPending, continuationSource,
                continuationExitNativeTarget, continuationPreviousTarget,
                continuationInterpolationActive, continuationStartFov,
                gameplayContinuationSource, exitTargetFov, exitTransformedFov};
        }
    };

    // Read-only state returned to scenario assertions. Production owns and
    // mutates the referenced RuntimeState; callers cannot use this view to seed
    // lifecycle state.
    struct CinematicLifecycleSnapshot
    {
        camera::CoordinatorState coordinator{camera::CoordinatorState::Gameplay};
        bool selectionValid{};
        std::uint64_t generation{};
        bool correctionEnabled{};
        std::uint64_t settingsEpoch{};
        bool targetEstablished{};
        float targetFov{std::numeric_limits<float>::quiet_NaN()};
        bool continuationPending{};
        bool continuationCarryPending{};
        bool continuationActive{};
        std::uintptr_t continuationSource{};
        float continuationStartFov{std::numeric_limits<float>::quiet_NaN()};
        float continuationExitNativeTarget{std::numeric_limits<float>::quiet_NaN()};
        float continuationPreviousTarget{std::numeric_limits<float>::quiet_NaN()};
        cinematics::CinematicWriterAssociationSnapshot association{};
        cinematics::CinematicRecoveryOwnerSnapshot recoveryOwner{};
        bool exclusionActive{};
        bool handoffPending{};
    };

    inline CinematicLifecycleSnapshot ReadCinematicLifecycleSnapshot(
        const CinematicLifecycleStateView& state)
    {
        CinematicLifecycleSnapshot snapshot{};
        snapshot.coordinator = state.coordinator.load(std::memory_order_acquire);
        snapshot.selectionValid = state.selectionValid.load(std::memory_order_acquire);
        snapshot.generation = state.selectionGeneration.load(std::memory_order_acquire);
        snapshot.correctionEnabled = state.activeEnabled.load(std::memory_order_acquire);
        snapshot.settingsEpoch = state.activeSettingsEpoch.load(std::memory_order_acquire);
        snapshot.targetEstablished = state.fovApplied.load(std::memory_order_acquire);
        snapshot.targetFov = state.transformedFov.load(std::memory_order_acquire);
        snapshot.continuationPending = state.continuationPending.load(
            std::memory_order_acquire);
        snapshot.continuationCarryPending = state.continuationCarryPending.load(
            std::memory_order_acquire);
        snapshot.continuationActive = state.continuationInterpolationActive.load(
            std::memory_order_acquire);
        snapshot.continuationSource = state.continuationSource.load(
            std::memory_order_acquire);
        snapshot.continuationStartFov = state.continuationStartFov.load(
            std::memory_order_acquire);
        snapshot.continuationExitNativeTarget =
            state.continuationExitNativeTarget.load(std::memory_order_acquire);
        snapshot.continuationPreviousTarget = state.continuationPreviousTarget.load(
            std::memory_order_acquire);
        snapshot.association = state.association.Read();
        snapshot.recoveryOwner = state.recoveryOwner.Read();
        snapshot.exclusionActive = state.exclusion.IsActive();
        snapshot.handoffPending = state.handoffPending.load(std::memory_order_acquire);
        return snapshot;
    }

    struct AcceptedCinematicEnterInput
    {
        cinematics::CinematicSelectionSnapshot selection{};
        bool correctionEnabled{};
        std::uint64_t settingsEpoch{};
        float authoredFov{};
        camera::GameplayBaseline gameplayBaseline{};
        float effectiveAspect{};
        float nativeAspect{};
        std::uintptr_t previousContinuationSource{};
        float previousExitNativeTarget{std::numeric_limits<float>::quiet_NaN()};
        float previousTransformedTarget{std::numeric_limits<float>::quiet_NaN()};
        bool awaitingValidatedRecovery{};
    };

    struct AcceptedCinematicEnterResult
    {
        bool accepted{};
        bool continuedOwnership{};
        cinematics::CinematicSelectionSnapshot selection{};
        bool handoffCancelled{};
        bool exclusionCleared{};
        bool recoveryOwnerCleared{};
        cinematics::CinematicBaselineSelection baselineSelection{};
        bool transformed{};
        std::uint64_t generation{};
        float outputFov{};
    };

    inline cinematics::CinematicSelectionSnapshot PrepareCinematicEnter(
        CinematicLifecycleStateView state,
        config::CinematicAspectPolicy aspectPolicy,
        config::CinematicFovMode fovMode)
    {
        cinematics::CinematicSelectionSnapshot candidate{};
        const auto nextGeneration = state.selectionGeneration.load(
            std::memory_order_acquire) + 1;
        if (!cinematics::TryCaptureCinematicSelection(
                state.selectionValid.load(std::memory_order_acquire),
                aspectPolicy, fovMode, nextGeneration, candidate))
            return {};

        candidate.generation = state.selectionGeneration.fetch_add(
            1, std::memory_order_acq_rel) + 1;
        return candidate;
    }

    inline AcceptedCinematicEnterResult ProcessAcceptedCinematicEnter(
        CinematicLifecycleStateView state, const AcceptedCinematicEnterInput& input)
    {
        AcceptedCinematicEnterResult result{};
        if (!input.selection.valid || state.selectionValid.load(std::memory_order_acquire))
            return result;

        result.selection = input.selection;
        const auto& selection = result.selection;
        result.generation = selection.generation;
        const bool continuationSelectionUnchanged =
            state.activeAspectPolicy.load(std::memory_order_acquire) == selection.aspectPolicy &&
            state.activeFovMode.load(std::memory_order_acquire) == selection.fovMode;
        const bool baselineUsable = input.correctionEnabled &&
            camera::IsUsableGameplayBaseline(input.gameplayBaseline);
        const auto gameplayBaseline = baselineUsable
            ? input.gameplayBaseline : camera::GameplayBaseline{};
        const auto baselineSource = baselineUsable &&
                input.gameplayBaseline.source.valid
            ? input.gameplayBaseline.source.value : 0;
        result.baselineSelection = input.correctionEnabled
            ? cinematics::SelectCinematicBaseline(
                selection.fovMode == config::CinematicFovMode::GameplayHorPlus,
                input.authoredFov, gameplayBaseline)
            : cinematics::CinematicBaselineSelection{input.authoredFov,
                cinematics::CinematicBaselineValueOrigin::AuthoredEnter, false};
        result.outputFov = input.authoredFov;
        result.transformed = input.correctionEnabled &&
            cinematics::OverrideEnabled(selection.aspectPolicy) &&
            cinematics::TryTransformSelectedCinematicFov(true, selection.fovMode,
                input.authoredFov, input.authoredFov,
                result.baselineSelection.targetBaselineFov,
                input.effectiveAspect, input.nativeAspect, result.outputFov);
        if (!result.transformed) result.outputFov = input.authoredFov;
        result.handoffCancelled = state.handoffPending.load(std::memory_order_acquire);
        result.exclusionCleared = state.exclusion.IsActive();
        result.recoveryOwnerCleared = state.recoveryOwner.Read().valid;
        state.recoveryOwner.Clear();
        state.handoffPending.store(false, std::memory_order_release);
        state.handoffSource.store(0, std::memory_order_release);
        state.handoffPreviousFov.store(
            std::numeric_limits<float>::quiet_NaN(), std::memory_order_release);
        state.exclusion.Reset();
        state.exitTransformedFov.store(
            std::numeric_limits<float>::quiet_NaN(), std::memory_order_release);
        state.transformedFov.store(
            result.transformed ? result.outputFov :
                std::numeric_limits<float>::quiet_NaN(), std::memory_order_release);
        state.gameplayContinuationSource.store(
            baselineUsable && input.gameplayBaseline.source.valid
                ? input.gameplayBaseline.source.value : 0,
            std::memory_order_release);
        state.enterNativeFov.store(input.authoredFov, std::memory_order_release);
        state.preEnterNativeFov.store(baselineUsable
                ? input.gameplayBaseline.nativeFov.value
                : std::numeric_limits<float>::quiet_NaN(),
            std::memory_order_release);
        state.preEnterAspect.store(baselineUsable ? input.gameplayBaseline.aspect
                : std::numeric_limits<float>::quiet_NaN(), std::memory_order_release);
        state.preEnterHorPlusFov.store(baselineUsable &&
                input.gameplayBaseline.horPlusFov.valid
                ? input.gameplayBaseline.horPlusFov.value
                : std::numeric_limits<float>::quiet_NaN(), std::memory_order_release);
        state.preEnterPairValid.store(baselineUsable &&
                input.gameplayBaseline.horPlusFov.valid, std::memory_order_release);
        state.association.BeginGeneration(selection.generation, input.authoredFov,
            result.transformed ? result.outputFov : input.authoredFov,
            {input.correctionEnabled, selection.aspectPolicy, selection.fovMode},
            baselineUsable ? input.gameplayBaseline.nativeFov.value
                : std::numeric_limits<float>::quiet_NaN(),
            baselineSource);
        state.fovApplied.store(input.correctionEnabled, std::memory_order_release);
        state.activeEnabled.store(input.correctionEnabled, std::memory_order_release);
        state.activeSettingsEpoch.store(input.correctionEnabled
            ? input.settingsEpoch : 0, std::memory_order_release);
        state.activeAspectPolicy.store(selection.aspectPolicy, std::memory_order_relaxed);
        state.activeFovMode.store(selection.fovMode, std::memory_order_relaxed);
        state.selectionGeneration.store(selection.generation, std::memory_order_release);
        state.selectionValid.store(true, std::memory_order_release);

        const bool inheritedEmptyGenerationContinuation =
            state.continuationCarryPending.exchange(false, std::memory_order_acq_rel);
        const gameplay::CinematicReentrySample reentry{
            state.coordinator.load(std::memory_order_acquire),
            input.previousContinuationSource,
            input.correctionEnabled ? baselineSource : 0,
            input.previousExitNativeTarget,
            input.previousTransformedTarget,
            input.authoredFov,
            result.transformed ? result.outputFov : input.authoredFov,
            input.awaitingValidatedRecovery || inheritedEmptyGenerationContinuation,
            result.transformed
        };
        result.continuedOwnership = input.correctionEnabled &&
            gameplay::IsCinematicOwnershipContinuation(reentry);
        // GameplayHorPlus can establish from the qualified Gameplay native
        // baseline rather than the authored ENTER operand. Reentry must use
        // that same native intent, while retaining the source, pending EXIT
        // and transformed-target checks of the existing continuation predicate.
        // Do not widen the strict baseline tolerance or change authored evidence.
        if (!result.continuedOwnership && continuationSelectionUnchanged && baselineSource != 0 &&
            result.baselineSelection.usedGameplayBaselineValue &&
            std::isfinite(input.previousExitNativeTarget) &&
            std::fabs(input.previousExitNativeTarget -
                result.baselineSelection.targetBaselineFov) <= 0.05f) {
            auto baselineReentry = reentry;
            baselineReentry.authoredEnterFov = result.baselineSelection.targetBaselineFov;
            result.continuedOwnership = gameplay::IsCinematicOwnershipContinuation(baselineReentry);
        }
        state.continuationPending.store(false, std::memory_order_release);
        state.continuationInterpolationActive.store(result.continuedOwnership,
            std::memory_order_release);
        state.continuationStartFov.store(result.continuedOwnership
                ? input.previousExitNativeTarget
                : std::numeric_limits<float>::quiet_NaN(),
            std::memory_order_release);
        if (!result.continuedOwnership) {
            state.continuationSource.store(0, std::memory_order_release);
            state.continuationExitNativeTarget.store(
                std::numeric_limits<float>::quiet_NaN(), std::memory_order_release);
            state.continuationPreviousTarget.store(
                std::numeric_limits<float>::quiet_NaN(), std::memory_order_release);
        }
        state.coordinator.store(camera::CoordinatorState::CinematicActive,
            std::memory_order_release);
        result.accepted = true;
        return result;
    }

    struct CinematicAspectStoreObservationResult
    {
        bool accepted{};
        bool correctionPolicyActive{};
        config::CinematicAspectPolicy effectivePolicy{
            config::CinematicAspectPolicy::Native};
        float aspectToStore{16.0f / 9.0f};
        float autoAspectForLog{std::numeric_limits<float>::quiet_NaN()};
        cinematics::CinematicWriterAssociationSnapshot association{};
    };

    struct CinematicAspectStoreInput
    {
        std::uintptr_t targetObject{};
        bool hookEnabled{};
        bool selectionValid{};
        bool stopping{};
        bool runtimeEnabled{};
        bool activeCorrectionEnabled{};
        float nativeAspect{16.0f / 9.0f};
        float cinemaAspect{cinematics::Forced21x9Aspect};
        float wideAspect{32.0f / 9.0f};
        cinematics::AutoAspectResolver readAutoAspectForLog{};
        cinematics::AutoAspectResolver resolveAutoAspect{};
    };

    inline CinematicAspectStoreObservationResult ObserveCinematicAspectStore(
        CinematicLifecycleStateView state, const CinematicAspectStoreInput& input)
    {
        CinematicAspectStoreObservationResult result{};
        if (!input.stopping) {
            result.accepted = input.targetObject != 0 &&
                state.selectionValid.load(std::memory_order_acquire) &&
                state.coordinator.load(std::memory_order_acquire) ==
                    camera::CoordinatorState::CinematicActive;
            if (result.accepted)
                result.association = state.association.ObserveAspectStoreTarget(
                    state.selectionGeneration.load(std::memory_order_acquire),
                    input.targetObject);
            else
                result.association = state.association.Read();
        } else {
            result.association = state.association.Read();
        }

        result.correctionPolicyActive = input.hookEnabled && input.selectionValid &&
            !input.stopping && input.runtimeEnabled && input.activeCorrectionEnabled;
        if (result.correctionPolicyActive) {
            result.effectivePolicy = state.activeAspectPolicy.load(
                std::memory_order_acquire);
            if (result.effectivePolicy == config::CinematicAspectPolicy::Auto &&
                input.readAutoAspectForLog != nullptr)
                result.autoAspectForLog = input.readAutoAspectForLog();
            result.aspectToStore = cinematics::ResolveAspect(
                result.effectivePolicy, input.nativeAspect, input.cinemaAspect,
                input.wideAspect, input.resolveAutoAspect);
        } else {
            result.effectivePolicy = config::CinematicAspectPolicy::Native;
            result.aspectToStore = input.nativeAspect;
        }
        return result;
    }

    struct AcceptedCinematicExitInput
    {
        camera::CoordinatorState previousCoordinator{camera::CoordinatorState::Gameplay};
        camera::CoordinatorState nextCoordinator{camera::CoordinatorState::Gameplay};
        std::uint64_t generation{};
        float nativeFov{};
        bool gameplayEnabled{};
        config::GameplayMode gameplayMode{config::GameplayMode::HorPlus};
        std::uintptr_t previousContinuationSource{};
        float previousTransformedTarget{std::numeric_limits<float>::quiet_NaN()};
    };

    struct AcceptedCinematicExitResult
    {
        bool accepted{};
        bool recoveryOwnerCaptured{};
        bool handoffCancelled{};
        bool exclusionCleared{};
        bool associationInvalidated{};
        bool continuityCanBeEvaluated{};
        bool preservedInheritedContinuation{};
        bool armGameplayHandoff{};
        cinematics::CinematicRecoveryOwnerSnapshot recoveryOwner{};
        cinematics::CinematicWriterAssociationSnapshot associationAtExit{};
    };

    inline AcceptedCinematicExitResult ProcessAcceptedCinematicExit(
        CinematicLifecycleStateView state, const AcceptedCinematicExitInput& input)
    {
        AcceptedCinematicExitResult result{};
        if (!cinematics::IsAcceptedCinematicExit(input.previousCoordinator,
                state.selectionValid.load(std::memory_order_acquire)))
            return result;

        result.accepted = true;
        result.associationAtExit = state.association.Read();
        const bool continuedGeneration = state.continuationInterpolationActive.load(
            std::memory_order_acquire);
        const auto inheritedContinuationSource = state.continuationSource.load(
            std::memory_order_acquire);
        const float inheritedContinuationStartFov = state.continuationStartFov.load(
            std::memory_order_acquire);
        const float inheritedContinuationExitNativeTarget =
            state.continuationExitNativeTarget.load(std::memory_order_acquire);
        const float inheritedContinuationPreviousTarget =
            state.continuationPreviousTarget.load(std::memory_order_acquire);
        result.handoffCancelled = state.handoffPending.load(std::memory_order_acquire);
        result.exclusionCleared = state.exclusion.IsActive();
        state.recoveryOwner.Clear();
        result.recoveryOwnerCaptured =
            input.nextCoordinator == camera::CoordinatorState::CinematicExiting &&
            state.recoveryOwner.Capture(result.associationAtExit, input.generation);
        result.recoveryOwner = state.recoveryOwner.Read();
        state.handoffPending.store(false, std::memory_order_release);
        state.handoffSource.store(0, std::memory_order_release);
        state.handoffPreviousFov.store(
            std::numeric_limits<float>::quiet_NaN(), std::memory_order_release);
        state.exitTargetFov.store(input.nativeFov, std::memory_order_release);
        state.exitTransformedFov.store(state.activeEnabled.load(std::memory_order_acquire)
                ? state.transformedFov.load(std::memory_order_acquire)
                : std::numeric_limits<float>::quiet_NaN(),
            std::memory_order_release);
        if (result.recoveryOwnerCaptured)
            state.exclusion.Arm(input.nativeFov);
        state.fovApplied.store(false, std::memory_order_release);
        state.transformedFov.store(std::numeric_limits<float>::quiet_NaN(),
            std::memory_order_release);
        result.associationInvalidated =
            input.previousCoordinator == camera::CoordinatorState::CinematicActive;
        if (result.associationInvalidated)
            state.association.InvalidateGeneration(input.generation);
        state.gameplayContinuationSource.store(0, std::memory_order_release);
        state.enterNativeFov.store(std::numeric_limits<float>::quiet_NaN(),
            std::memory_order_release);
        state.preEnterPairValid.store(false, std::memory_order_release);
        state.selectionValid.store(false, std::memory_order_release);
        result.continuityCanBeEvaluated = result.recoveryOwnerCaptured &&
            input.previousCoordinator == camera::CoordinatorState::CinematicActive &&
            input.nextCoordinator == camera::CoordinatorState::CinematicExiting &&
            input.previousContinuationSource != 0 &&
            std::isfinite(input.previousTransformedTarget) &&
            std::isfinite(input.nativeFov) && input.nativeFov > 1.0f &&
            input.nativeFov < 179.0f;
        state.continuationPending.store(result.continuityCanBeEvaluated,
            std::memory_order_release);
        state.continuationSource.store(result.continuityCanBeEvaluated
                ? input.previousContinuationSource : 0,
            std::memory_order_release);
        state.continuationExitNativeTarget.store(result.continuityCanBeEvaluated
                ? input.nativeFov : std::numeric_limits<float>::quiet_NaN(),
            std::memory_order_release);
        state.continuationPreviousTarget.store(result.continuityCanBeEvaluated
                ? input.previousTransformedTarget
                : std::numeric_limits<float>::quiet_NaN(),
            std::memory_order_release);
        state.continuationInterpolationActive.store(false, std::memory_order_release);
        if (!result.recoveryOwnerCaptured ||
            input.nextCoordinator != camera::CoordinatorState::CinematicExiting) {
            result.preservedInheritedContinuation =
                !result.recoveryOwnerCaptured &&
                input.previousCoordinator == camera::CoordinatorState::CinematicActive &&
                input.nextCoordinator == camera::CoordinatorState::CinematicExiting &&
                continuedGeneration && !result.associationAtExit.writerObserved;
            const auto cleanup = ClearCameraWriterRecoveryState(
                state.WriterRecoveryView());
            result.handoffCancelled = cleanup.handoffCancelled;
            result.exclusionCleared = cleanup.exclusionCleared;
            if (result.preservedInheritedContinuation) {
                state.continuationCarryPending.store(true, std::memory_order_release);
                state.continuationSource.store(inheritedContinuationSource,
                    std::memory_order_release);
                state.continuationStartFov.store(inheritedContinuationStartFov,
                    std::memory_order_release);
                state.continuationExitNativeTarget.store(
                    inheritedContinuationExitNativeTarget, std::memory_order_release);
                state.continuationPreviousTarget.store(
                    inheritedContinuationPreviousTarget, std::memory_order_release);
                state.continuationInterpolationActive.store(true,
                    std::memory_order_release);
            }
            state.coordinator.store(input.nextCoordinator,
                std::memory_order_release);
        } else {
            state.coordinator.store(camera::CoordinatorState::CinematicExiting,
                std::memory_order_release);
            result.armGameplayHandoff = input.gameplayMode ==
                config::GameplayMode::AspectRecalculation && input.gameplayEnabled;
            if (result.armGameplayHandoff)
                state.handoffPending.store(true, std::memory_order_release);
        }
        return result;
    }
}
