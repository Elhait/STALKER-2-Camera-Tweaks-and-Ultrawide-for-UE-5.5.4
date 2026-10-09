#pragma once

#include "camera_correction_gate.hpp"
#include "runtime_settings.hpp"
#include "../camera/fov_semantics.hpp"
#include "../camera/presentation_state.hpp"
#include "../config/feature_config.hpp"
#include "../dialogue/dialogue_state.hpp"
#include "../dialogue/dialogue_fov.hpp"

#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>
#include <mutex>

namespace plugin
{
    struct DialogueBoundaryStateView
    {
        std::mutex& mutex;
        std::atomic<camera::CoordinatorState>& coordinator;
        dialogue::PostCinematicRecoveryExclusion& exclusion;
        dialogue::Phase& phase;
        dialogue::PolicySnapshot& activePolicy;
        dialogue::CorrectionActivation& correctionActivation;
        float& baseline;
        float& previous;
        float& exitIncomingStart;
        dialogue::CandidateTracker& candidate;
        dialogue::RecoveryRearm& recoveryRearm;
        std::atomic<bool>& enabled;
        std::atomic<config::DialogueZoomPolicy>& selectedPolicy;
        std::atomic<std::uint64_t>& settingsEpoch;
        std::atomic<std::uint64_t>& activeSettingsEpoch;
        std::atomic<std::uintptr_t>& snapshotSource;
        std::atomic<float>& snapshotTarget;
        std::atomic<bool>& snapshotTargetValid;
    };

    struct DialogueBoundaryInput
    {
        float incomingFov{std::numeric_limits<float>::quiet_NaN()};
        std::uintptr_t callbackSource{};
        float nativeTarget{std::numeric_limits<float>::quiet_NaN()};
        bool nativeTargetReadable{};
        float transformEpsilon{0.01f};
        float recoveryEndpointTolerance{0.02f};
        std::uint32_t candidateStableSamples{3};
    };

    enum class DialogueBoundaryAction : std::uint8_t
    {
        InvalidInput,
        ExclusionReset,
        NonGameplayReset,
        CandidateCaptured,
        CandidateRejected,
        CandidatePending,
        AcceptedStart,
        CandidateCancelled,
        LifecycleExiting,
        RecoveryCompleted,
        RecoveryCancelled,
        RecoveryStarted,
        Processed,
    };

    struct DialogueBoundaryResult
    {
        DialogueBoundaryAction action{DialogueBoundaryAction::Processed};
        dialogue::Phase phaseBefore{dialogue::Phase::Inactive};
        dialogue::Phase phaseAtCallback{};
        dialogue::Phase phaseAfter{dialogue::Phase::Inactive};
        config::DialogueZoomPolicy selectedPolicy{config::DialogueZoomPolicy::Native};
        config::DialogueZoomPolicy activePolicy{config::DialogueZoomPolicy::Native};
        std::uintptr_t acceptedSource{};
        std::uintptr_t recoverySource{};
        float nativeTarget{std::numeric_limits<float>::quiet_NaN()};
        float baseline{std::numeric_limits<float>::quiet_NaN()};
        float outputFov{std::numeric_limits<float>::quiet_NaN()};
        bool inputValid{};
        bool snapshotPublished{};
        bool candidateContextValid{};
        bool stateReset{};
        bool activePolicyReleased{};
        config::DialogueZoomPolicy releasedPolicy{config::DialogueZoomPolicy::Native};
        bool mutateXmm1{};
        bool earlyReturn{};
    };

    inline void ResetDialogueBoundaryState(DialogueBoundaryStateView state) noexcept
    {
        state.phase = dialogue::Phase::Inactive;
        state.activePolicy.Clear();
        state.correctionActivation.Reset();
        state.baseline = std::numeric_limits<float>::quiet_NaN();
        state.previous = std::numeric_limits<float>::quiet_NaN();
        state.exitIncomingStart = std::numeric_limits<float>::quiet_NaN();
        state.candidate.Reset();
        state.recoveryRearm.Reset();
        state.snapshotSource.store(0, std::memory_order_release);
        state.snapshotTarget.store(std::numeric_limits<float>::quiet_NaN(),
            std::memory_order_release);
        state.snapshotTargetValid.store(false, std::memory_order_release);
    }

    // Caller owns the Dialogue mutex. Shared by the gameplay-writer adapter
    // and scenario harness so source-context invalidation keeps one predicate.
    inline bool DialogueBoundaryIsActive(
        const DialogueBoundaryStateView& state) noexcept
    {
        return state.phase != dialogue::Phase::Inactive;
    }

    // Caller owns cameraCorrectionGate before this function and the dialogue
    // mutex for its duration. The hook supplies only captured ABI/memory input;
    // all Dialogue lifecycle and XMM1 decisions are shared with the harness.
    inline DialogueBoundaryResult ProcessDialogueBoundary(
        DialogueBoundaryStateView state, const DialogueBoundaryInput& input)
    {
        DialogueBoundaryResult result{};
        result.phaseBefore = state.phase;
        result.phaseAtCallback = state.phase;
        result.phaseAfter = state.phase;
        result.baseline = state.baseline;
        result.outputFov = input.incomingFov;
        result.nativeTarget = input.nativeTarget;
        const auto resetState = [&] {
            result.activePolicyReleased = state.activePolicy.IsValid();
            if (result.activePolicyReleased)
                result.releasedPolicy = state.activePolicy.Value();
            ResetDialogueBoundaryState(state);
            result.stateReset = true;
        };

        if (!std::isfinite(input.incomingFov) || input.incomingFov <= 1.0f ||
            input.incomingFov >= 179.0f) {
            result.action = DialogueBoundaryAction::InvalidInput;
            result.earlyReturn = true;
            if (state.phase == dialogue::Phase::Candidate) {
                resetState();
            }
            result.phaseAfter = state.phase;
            return result;
        }
        result.inputValid = true;
        state.snapshotSource.store(input.callbackSource, std::memory_order_release);
        state.snapshotTarget.store(input.nativeTarget,
            std::memory_order_release);
        state.snapshotTargetValid.store(input.nativeTargetReadable,
            std::memory_order_release);
        result.snapshotPublished = true;

        if (state.exclusion.IsActive()) {
            resetState();
            result.action = DialogueBoundaryAction::ExclusionReset;
            result.earlyReturn = true;
            result.phaseAfter = state.phase;
            return result;
        }
        if (state.coordinator.load(std::memory_order_acquire) !=
            camera::CoordinatorState::Gameplay) {
            resetState();
            result.action = DialogueBoundaryAction::NonGameplayReset;
            result.earlyReturn = true;
            result.phaseAfter = state.phase;
            return result;
        }

        const bool enabled = state.enabled.load(std::memory_order_acquire);
        result.selectedPolicy = enabled
            ? state.selectedPolicy.load(std::memory_order_acquire)
            : config::DialogueZoomPolicy::Native;
        const bool lifecycleUsesActivePolicy = state.phase == dialogue::Phase::Active ||
            state.phase == dialogue::Phase::Exiting ||
            state.phase == dialogue::Phase::RearmPending;
        const auto policy = lifecycleUsesActivePolicy && state.activePolicy.IsValid()
            ? state.activePolicy.Value() : result.selectedPolicy;
        result.activePolicy = policy;
        const bool coordinatorAllows = true;
        const bool recoveryAllows = !state.exclusion.IsActive();
        const bool candidateAllowed = enabled
            ? dialogue::CandidatePromotionAllowed(result.selectedPolicy,
                coordinatorAllows, recoveryAllows)
            : coordinatorAllows && recoveryAllows;
        const bool ascending = std::isfinite(state.previous) &&
            input.incomingFov > state.previous + input.transformEpsilon;

        if (state.phase == dialogue::Phase::RearmPending) {
            const auto decision = state.recoveryRearm.Observe(input.callbackSource,
                input.incomingFov, input.nativeTargetReadable ? input.nativeTarget
                    : std::numeric_limits<float>::quiet_NaN(),
                input.recoveryEndpointTolerance);
            if (decision == dialogue::RecoveryRearm::Decision::Complete) {
                result.baseline = state.baseline;
                resetState();
                result.action = DialogueBoundaryAction::RecoveryCompleted;
            } else if (decision == dialogue::RecoveryRearm::Decision::Cancel) {
                result.baseline = state.baseline;
                resetState();
                result.action = DialogueBoundaryAction::RecoveryCancelled;
            }
        } else if (state.phase == dialogue::Phase::Inactive &&
            !std::isfinite(state.baseline)) {
            result.candidateContextValid = input.callbackSource != 0 &&
                input.nativeTargetReadable &&
                input.nativeTarget > 1.0f && input.nativeTarget < 179.0f;
            if (!candidateAllowed || !result.candidateContextValid) {
                resetState();
                result.action = DialogueBoundaryAction::CandidateRejected;
            } else {
                state.baseline = input.incomingFov;
                state.phase = dialogue::Phase::Candidate;
                state.candidate.Begin(input.incomingFov, input.callbackSource,
                    input.nativeTarget);
                state.exitIncomingStart = std::numeric_limits<float>::quiet_NaN();
                result.action = DialogueBoundaryAction::CandidateCaptured;
            }
        } else if (state.phase == dialogue::Phase::Candidate) {
            result.candidateContextValid = input.callbackSource != 0 &&
                input.nativeTargetReadable &&
                input.nativeTarget > 1.0f && input.nativeTarget < 179.0f;
            if (!candidateAllowed || !result.candidateContextValid) {
                resetState();
                state.previous = input.incomingFov;
                result.action = DialogueBoundaryAction::CandidateRejected;
                result.phaseAfter = state.phase;
                result.earlyReturn = true;
                return result;
            }
            const auto decision = state.candidate.Observe(input.incomingFov,
                input.callbackSource, input.nativeTarget, input.transformEpsilon,
                input.transformEpsilon, input.candidateStableSamples);
            if (decision == dialogue::CandidateDecision::Activate) {
                state.baseline = state.candidate.Baseline();
                state.phase = dialogue::Phase::Active;
                if (enabled) state.activePolicy.Capture(result.selectedPolicy);
                state.correctionActivation.AcceptedStart(enabled);
                state.activeSettingsEpoch.store(enabled
                        ? state.settingsEpoch.load(std::memory_order_acquire) : 0,
                    std::memory_order_release);
                state.exitIncomingStart = std::numeric_limits<float>::quiet_NaN();
                result.acceptedSource = input.callbackSource;
                result.action = DialogueBoundaryAction::AcceptedStart;
            } else if (decision == dialogue::CandidateDecision::Cancel) {
                resetState();
                result.action = DialogueBoundaryAction::CandidateCancelled;
            } else {
                result.action = DialogueBoundaryAction::CandidatePending;
            }
        } else if (state.phase == dialogue::Phase::Active && ascending) {
            state.phase = dialogue::Phase::Exiting;
            state.exitIncomingStart = input.incomingFov;
            result.action = DialogueBoundaryAction::LifecycleExiting;
        }

        result.phaseAtCallback = state.phase;
        if (state.phase == dialogue::Phase::Exiting && ascending &&
            std::isfinite(state.baseline) &&
            camera::AreFovValuesSemanticallyEquivalent(input.incomingFov,
                state.baseline)) {
            const auto nativeTarget = input.nativeTargetReadable &&
                input.nativeTarget > 1.0f && input.nativeTarget < 179.0f
                ? input.nativeTarget : std::numeric_limits<float>::quiet_NaN();
            state.phase = dialogue::Phase::RearmPending;
            state.recoveryRearm.Begin(input.callbackSource, input.incomingFov,
                nativeTarget);
            result.recoverySource = input.callbackSource;
            result.action = DialogueBoundaryAction::RecoveryStarted;
        }

        const bool correctionActive =
            state.correctionActivation.CanCorrect(enabled) &&
            state.activePolicy.IsValid();
        if (correctionActive &&
            (policy == config::DialogueZoomPolicy::Adaptive ||
                policy == config::DialogueZoomPolicy::Reduced) &&
            (state.phase == dialogue::Phase::Active ||
                state.phase == dialogue::Phase::Exiting) &&
            std::isfinite(state.baseline)) {
            const float target = policy == config::DialogueZoomPolicy::Adaptive
                ? dialogue::AdaptiveTarget(state.baseline)
                : dialogue::ReducedTarget(state.baseline);
            result.outputFov = state.phase == dialogue::Phase::Exiting &&
                std::isfinite(state.exitIncomingStart)
                ? dialogue::TransformExitSample(input.incomingFov,
                    state.baseline, target, state.exitIncomingStart)
                : dialogue::TransformProjectionSample(input.incomingFov,
                    state.baseline, target);
            result.mutateXmm1 = true;
        } else if (correctionActive && policy == config::DialogueZoomPolicy::Disabled &&
            (state.phase == dialogue::Phase::Active ||
                state.phase == dialogue::Phase::Exiting) &&
            std::isfinite(state.baseline)) {
            result.outputFov = state.baseline;
            result.mutateXmm1 = true;
        }
        if (!std::isfinite(result.outputFov) || result.outputFov <= 1.0f ||
            result.outputFov >= 179.0f) {
            result.outputFov = input.incomingFov;
            result.mutateXmm1 = false;
        }
        state.previous = input.incomingFov;
        if (result.action == DialogueBoundaryAction::Processed &&
            result.phaseAtCallback == dialogue::Phase::RearmPending &&
            state.phase == dialogue::Phase::Inactive)
            result.action = DialogueBoundaryAction::RecoveryCompleted;
        result.phaseAfter = state.phase;
        if (!result.stateReset) result.baseline = state.baseline;
        return result;
    }
}
