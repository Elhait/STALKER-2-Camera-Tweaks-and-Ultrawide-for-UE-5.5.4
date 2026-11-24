#pragma once

#include "camera_correction_gate.hpp"
#include "runtime_settings.hpp"
#include "../camera/fov_semantics.hpp"
#include "../camera/presentation_state.hpp"
#include "../camera/fov_observation.hpp"
#include "../camera/gameplay_baseline.hpp"
#include "../config/feature_config.hpp"
#include "../dialogue/dialogue_state.hpp"
#include "../dialogue/dialogue_fov.hpp"
#include "../dialogue/post_exit_provider_handoff.hpp"

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
        bool& postExitProviderOwned;
        std::uintptr_t& postExitProviderCallbackSource;
        std::uintptr_t& postExitProviderReceiver;
        std::uint64_t& postExitProviderGeneration;
        std::uint64_t& postExitProviderSettingsEpoch;
        std::uint32_t& postExitProviderThreadId;
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
        bool handledProviderActivation{};
        std::uintptr_t validatedReceiver{};
        std::uint64_t generation{};
        std::uintptr_t observedReceiver{};
        float providerGameplayBaseline{std::numeric_limits<float>::quiet_NaN()};
        bool providerGameplayBaselineValid{};
        config::DialogueZoomPolicy providerDialoguePolicy{
            config::DialogueZoomPolicy::Native};
        bool providerDialoguePolicyValid{};
        std::uint32_t threadId{};
    };

    enum class DialogueBoundaryAction : std::uint8_t
    {
        InvalidInput,
        ExclusionReset,
        ProviderCallbackIgnored,
        NonGameplayReset,
        HandledProviderReset,
        ProviderDialogueStarted,
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
        state.postExitProviderOwned = false;
        state.postExitProviderCallbackSource = 0;
        state.postExitProviderReceiver = 0;
        state.postExitProviderGeneration = 0;
        state.postExitProviderSettingsEpoch = 0;
        state.postExitProviderThreadId = 0;
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

    // Confirms the provider activation captured at the accepted post-EXIT
    // handoff. The active policy epoch may advance independently; receiver,
    // callback source, provider generation, and thread identity may not.
    inline bool IsConfirmedProviderDialogueLifecycle(
        const DialogueBoundaryStateView& state,
        const dialogue::PostExitProviderActivation& activation,
        std::uintptr_t writerSource, std::uint32_t writerThreadId,
        std::uint64_t generation) noexcept
    {
        return state.phase == dialogue::Phase::Active &&
            state.postExitProviderOwned && state.activePolicy.IsValid() &&
            state.activePolicy.Value() == activation.dialoguePolicy &&
            activation.receiver != 0 && activation.threadId != 0 &&
            activation.generation != 0 &&
            activation.receiver == state.postExitProviderReceiver &&
            activation.threadId == state.postExitProviderThreadId &&
            activation.generation == state.postExitProviderGeneration &&
            activation.dialogueSettingsEpoch == state.postExitProviderSettingsEpoch &&
            writerSource == activation.receiver && writerThreadId == activation.threadId &&
            generation == activation.generation &&
            state.postExitProviderCallbackSource != 0 &&
            state.candidate.HasReceiverIdentity(activation.receiver,
                state.postExitProviderCallbackSource, activation.generation);
    }

    // Called at the setter-argument boundary under the Dialogue mutex and
    // camera gate. Record every accepted corrected update, not just admission.
    inline void RecordProviderDialogueSetterResult(
        const DialogueBoundaryStateView& state, const DialogueBoundaryInput& input,
        const DialogueBoundaryResult& result,
        dialogue::PostExitProviderHandoff& handoff) noexcept
    {
        if (result.action == DialogueBoundaryAction::ProviderCallbackIgnored) return;
        const auto& activation = handoff.Activation();
        const bool owned = handoff.RecoveryCompleted() &&
            handoff.DialogueAdmissionConsumed() &&
            input.validatedReceiver != 0 &&
            input.callbackSource == state.postExitProviderCallbackSource &&
            IsConfirmedProviderDialogueLifecycle(state, activation,
                input.validatedReceiver, input.threadId, input.generation);
        if (owned && result.mutateXmm1 && !result.earlyReturn)
            static_cast<void>(handoff.RecordCorrectedDialogueSetterSample(result.outputFov));
        else
            handoff.ClearCorrectedDialogueSetterSample();
    }

    // A stationary setter callback after recovery is only a hypothesis. A
    // matching, still-current production Gameplay reference supplies a terminal
    // writer boundary without waiting for another setter. Never revive a stale
    // baseline, infer Gameplay from a foreign camera, or retire an active policy.
    // Caller holds the correction gate and Dialogue mutex.
    inline bool RetireStationaryDialogueCandidateOnGameplayWriter(
        DialogueBoundaryStateView state, const camera::GameplayBaseline& baseline,
        std::uintptr_t source, float inputFov, float aspect, bool aspectValid,
        std::uint64_t generation, bool providerEndpoint, float endpointTolerance) noexcept
    {
        if (state.phase != dialogue::Phase::Candidate ||
            state.coordinator.load(std::memory_order_acquire) != camera::CoordinatorState::Gameplay ||
            state.exclusion.IsActive() || providerEndpoint || state.postExitProviderOwned ||
            state.activePolicy.IsValid() ||
            !camera::IsUsableGameplayBaseline(baseline) ||
            !baseline.source.valid || baseline.source.value != source ||
            baseline.observationSequence == 0 || !aspectValid ||
            !std::isfinite(aspect) || std::fabs(aspect - baseline.aspect) > 0.001f ||
            !std::isfinite(inputFov) || !std::isfinite(endpointTolerance) ||
            endpointTolerance < 0.0f ||
            std::fabs(inputFov - baseline.nativeFov.value) > endpointTolerance ||
            !state.candidate.HasReceiverBinding(source,
                state.settingsEpoch.load(std::memory_order_acquire), generation) ||
            !state.candidate.IsStationaryAtReference(baseline.nativeFov.value, endpointTolerance))
            return false;
        ResetDialogueBoundaryState(state);
        return true;
    }

    // Caller holds the Dialogue mutex; snapshot before any writer-triggered reset.
    inline camera::WriterContext ClassifyDialogueWriterContext(
        const DialogueBoundaryStateView& state, std::uintptr_t writerSource,
        std::uint64_t settingsEpoch, std::uint64_t generation = 0,
        bool postExitProviderEndpoint = false,
        bool confirmedProviderDialogue = false) noexcept
    {
        if (state.phase != dialogue::Phase::Inactive) {
            if (confirmedProviderDialogue)
                return camera::WriterContext::ConfirmedProviderDialogueLifecycleObserved;
            if (state.candidate.AffectsReceiver(writerSource, settingsEpoch, generation) &&
                state.phase == dialogue::Phase::Active &&
                state.candidate.HasReceiverBinding(writerSource, settingsEpoch, generation))
                return camera::WriterContext::DialogueLifecycleObserved;
            // A raw writer-source mismatch does not establish that this is an
            // independent Gameplay camera. Preserve the correction path, but
            // keep baseline admission ambiguous until ownership is evidenced.
            return camera::WriterContext::TransitionAmbiguous;
        }
        if (postExitProviderEndpoint)
            return camera::WriterContext::TransitionAmbiguous;
        return camera::WriterContext::GameplayEligible;
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

        const auto receiver = input.validatedReceiver
            ? input.validatedReceiver : input.observedReceiver;
        const auto settingsEpoch = state.settingsEpoch.load(std::memory_order_acquire);
        const bool providerLifecyclePhase = state.postExitProviderOwned &&
            (state.phase == dialogue::Phase::Active ||
                state.phase == dialogue::Phase::Exiting ||
                state.phase == dialogue::Phase::RearmPending);
        const bool providerReceiverAndLifecycleMatch = providerLifecyclePhase &&
            state.coordinator.load(std::memory_order_acquire) ==
                camera::CoordinatorState::Gameplay &&
            receiver != 0 && receiver == state.postExitProviderReceiver &&
            input.threadId != 0 && input.threadId == state.postExitProviderThreadId &&
            input.generation == state.postExitProviderGeneration &&
            state.candidate.HasReceiverIdentity(receiver,
                state.postExitProviderCallbackSource, input.generation);
        const bool providerLifecycleContinuation =
            providerReceiverAndLifecycleMatch &&
            input.callbackSource == state.postExitProviderCallbackSource;
        if (state.exclusion.IsActive() && !input.handledProviderActivation &&
            providerReceiverAndLifecycleMatch && !providerLifecycleContinuation) {
            result.action = DialogueBoundaryAction::ProviderCallbackIgnored;
            result.earlyReturn = true;
            result.phaseAfter = state.phase;
            return result;
        }
        if (state.exclusion.IsActive() && !input.handledProviderActivation &&
            !providerLifecycleContinuation) {
            resetState();
            result.action = DialogueBoundaryAction::ExclusionReset;
            result.earlyReturn = true;
            result.phaseAfter = state.phase;
            return result;
        }

        if (state.phase == dialogue::Phase::Active ||
            state.phase == dialogue::Phase::Exiting ||
            state.phase == dialogue::Phase::RearmPending)
            state.candidate.BindReceiver(receiver, settingsEpoch,
                input.callbackSource, input.generation,
                input.validatedReceiver != 0);
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
        if (input.handledProviderActivation) {
            const auto baselineSource = input.validatedReceiver
                ? input.validatedReceiver : input.observedReceiver;
            const bool providerBaselineValid = input.providerGameplayBaselineValid &&
                baselineSource != 0 && input.nativeTargetReadable &&
                std::isfinite(input.providerGameplayBaseline) &&
                input.providerGameplayBaseline > 1.0f &&
                input.providerGameplayBaseline < 179.0f;
            if (!providerBaselineValid || !input.providerDialoguePolicyValid) {
                resetState();
                result.action = DialogueBoundaryAction::HandledProviderReset;
                result.earlyReturn = true;
                result.phaseAfter = state.phase;
                return result;
            }

            // The exact post-recovery provider activation is the Dialogue
            // lifecycle evidence. Seed its baseline from the previously
            // admitted Gameplay reference, never from the native ~70 degree
            // endpoint, so policy correction can take ownership without
            // granting that endpoint Gameplay baseline admission.
            state.baseline = input.providerGameplayBaseline;
            state.phase = dialogue::Phase::Active;
            state.postExitProviderOwned = true;
            state.postExitProviderCallbackSource = input.callbackSource;
            state.postExitProviderReceiver = baselineSource;
            state.postExitProviderGeneration = input.generation;
            state.postExitProviderSettingsEpoch = settingsEpoch;
            state.postExitProviderThreadId = input.threadId;
            result.activePolicy = input.providerDialoguePolicy;
            state.activePolicy.Capture(result.activePolicy);
            state.correctionActivation.AcceptedStart(enabled);
            state.activeSettingsEpoch.store(enabled
                    ? state.settingsEpoch.load(std::memory_order_acquire) : 0,
                std::memory_order_release);
            state.candidate.Begin(state.baseline, input.callbackSource,
                input.nativeTarget);
            state.candidate.BindReceiver(baselineSource,
                state.settingsEpoch.load(std::memory_order_acquire),
                input.callbackSource, input.generation, true);
            state.exitIncomingStart = std::numeric_limits<float>::quiet_NaN();
            result.baseline = state.baseline;
            result.candidateContextValid = true;
            result.acceptedSource = input.callbackSource;
            result.action = DialogueBoundaryAction::ProviderDialogueStarted;
        }
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
                state.candidate.BindReceiver(input.validatedReceiver ? input.validatedReceiver : input.observedReceiver,
                    state.settingsEpoch.load(std::memory_order_acquire), input.callbackSource,
                    input.generation, input.validatedReceiver != 0);
                state.exitIncomingStart = std::numeric_limits<float>::quiet_NaN();
                result.action = DialogueBoundaryAction::CandidateCaptured;
            }
        } else if (state.phase == dialogue::Phase::Candidate) {
            state.candidate.BindReceiver(input.validatedReceiver ? input.validatedReceiver : input.observedReceiver,
                state.settingsEpoch.load(std::memory_order_acquire), input.callbackSource,
                input.generation, input.validatedReceiver != 0);
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
        const bool boundReturnEndpoint = state.phase == dialogue::Phase::Exiting &&
            input.validatedReceiver && state.candidate.HasReceiverBinding(
                input.validatedReceiver, settingsEpoch, input.generation) &&
            state.candidate.HasReceiverIdentity(input.validatedReceiver,
                input.callbackSource, input.generation) &&
            input.nativeTargetReadable && std::isfinite(input.nativeTarget) &&
            input.nativeTarget > 1.0f && input.nativeTarget < 179.0f &&
            std::isfinite(state.candidate.NativeTarget()) &&
            std::fabs(input.nativeTarget - state.candidate.NativeTarget()) >
                input.recoveryEndpointTolerance &&
            std::isfinite(state.exitIncomingStart) &&
            input.nativeTarget > state.exitIncomingStart + input.transformEpsilon &&
            ascending &&
            std::fabs(input.incomingFov - input.nativeTarget) <= input.recoveryEndpointTolerance;
        if (boundReturnEndpoint &&
            !camera::AreFovValuesSemanticallyEquivalent(input.incomingFov, state.baseline)) {
            result.baseline = state.baseline;
            resetState();
            result.action = DialogueBoundaryAction::RecoveryCompleted;
        } else if (state.phase == dialogue::Phase::Exiting && ascending &&
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
            const float target = dialogue::ResolveDialogueDestinationTarget(
                policy, state.baseline, input.nativeTarget);
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
            result.outputFov = dialogue::ResolveDialogueDestinationTarget(
                policy, state.baseline, input.nativeTarget);
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
