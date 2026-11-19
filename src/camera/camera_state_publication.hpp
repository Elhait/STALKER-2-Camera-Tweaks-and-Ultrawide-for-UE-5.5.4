#pragma once

#include "camera_state_snapshot.hpp"
#include "fov_observation.hpp"
#include "gameplay_baseline.hpp"

#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>

namespace camera
{
    // The caller holds the camera-state lock. This view uses the existing
    // process-resident storage, so moving publication here does not change
    // RuntimeState's layout or the lock ordering of the writer callback.
    struct CameraStatePublicationView
    {
        CameraStateSnapshot& snapshot;
        bool& valid;
        float& neutralGameplayFovCandidate;
        std::uint32_t& neutralGameplayFovSamples;
        std::uintptr_t& neutralGameplayFovCandidateSource;
        std::atomic<bool>& zoomHold;
        std::atomic<std::uint64_t>& eventSequence;
    };

    struct CameraStatePublicationInput
    {
        bool observationPublished{};
        CameraFovObservation observation{};
        std::uintptr_t source{};
        bool applied{};
        PresentationState presentation{PresentationState::Unknown};
        DialogueState dialogue{DialogueState::Unknown};
        bool activeDialoguePolicyValid{};
        bool recoveryExcluded{};
        bool modeTransitionPending{};
        bool zoomObserverAvailable{};
        bool zoomActive{};
        bool zoomObservationValid{};
        ZoomDirection zoomDirection{ZoomDirection::None};
        std::uint64_t zoomSequence{};
        std::uintptr_t zoomSource{};
        float zoomPrimary{std::numeric_limits<float>::quiet_NaN()};
        float zoomSecondary{std::numeric_limits<float>::quiet_NaN()};
        std::uint8_t gameplayMode{};
        float nativeAspect{};
        bool runtimeCombineValid{};
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
        GameplayBaseline gameplayReference{};
        bool gameplayReferenceProvided{};
        bool originalInputAvailable{};
        CameraWriterTransactionIdentity writerTransaction{};
    };

    struct CameraStatePublicationResult
    {
        bool published{};
        CameraStateSnapshot previous{};
        CameraStateSnapshot current{};
    };

    enum class CameraStateFreshnessGroup : std::uint8_t
    {
        WriterObservation,
        TransformedResult,
        GameplayFov,
        NeutralGameplayFov,
        Presentation,
        Dialogue,
        GameplayMode,
        Zoom,
    };

    inline SnapshotFreshness FreshnessAfterInvalidation(bool valueValid) noexcept
    {
        return valueValid ? SnapshotFreshness::Historical
                          : SnapshotFreshness::Unavailable;
    }

    inline void RetainFovFact(CameraFovFact& fact) noexcept
    {
        if (fact.valid) fact.freshness = SnapshotFreshness::Retained;
    }

    inline void MarkFovFactHistorical(CameraFovFact& fact) noexcept
    {
        fact.freshness = fact.valid ? SnapshotFreshness::Historical
                                    : SnapshotFreshness::Unavailable;
    }

    inline void MarkCameraStateGroupHistorical(CameraStateSnapshot& snapshot,
        bool snapshotExists, CameraStateFreshnessGroup group) noexcept
    {
        if (!snapshotExists) return;
        switch (group) {
        case CameraStateFreshnessGroup::WriterObservation:
            snapshot.freshness.writerObservation = FreshnessAfterInvalidation(
                snapshot.evidence.writerObservationHasEvidence);
            MarkFovFactHistorical(snapshot.evidence.nativeFov);
            MarkFovFactHistorical(snapshot.evidence.writerInputFov);
            MarkFovFactHistorical(snapshot.evidence.postStoreFov);
            MarkFovFactHistorical(snapshot.evidence.postHelperFov);
            snapshot.evidence.writerTransaction = {};
            MarkFovFactHistorical(snapshot.evidence.currentOutputFov);
            break;
        case CameraStateFreshnessGroup::TransformedResult:
            snapshot.freshness.transformedResult = FreshnessAfterInvalidation(
                snapshot.freshness.transformedResult != SnapshotFreshness::Unavailable);
            RetainFovFact(snapshot.evidence.horPlusTargetFov);
            break;
        case CameraStateFreshnessGroup::GameplayFov:
            snapshot.evidence.gameplayFov.freshness = !snapshot.evidence.gameplayFov.valid
                ? SnapshotFreshness::Unavailable
                : snapshot.evidence.gameplayReferenceUsable ? SnapshotFreshness::Retained
                    : SnapshotFreshness::Historical;
            RetainFovFact(snapshot.evidence.neutralGameplayFov);
            break;
        case CameraStateFreshnessGroup::NeutralGameplayFov:
            MarkFovFactHistorical(snapshot.evidence.neutralGameplayFov);
            break;
        case CameraStateFreshnessGroup::Presentation:
            snapshot.freshness.presentation = snapshot.presentation.state ==
                PresentationState::Unknown ? SnapshotFreshness::Unavailable
                    : SnapshotFreshness::Historical;
            break;
        case CameraStateFreshnessGroup::Dialogue:
            snapshot.freshness.dialogue = snapshot.dialogue.state == DialogueState::Unknown
                ? SnapshotFreshness::Unavailable : SnapshotFreshness::Historical;
            break;
        case CameraStateFreshnessGroup::GameplayMode:
            snapshot.freshness.gameplayMode = FreshnessAfterInvalidation(
                snapshot.gameplayMode.valid);
            break;
        case CameraStateFreshnessGroup::Zoom:
            snapshot.freshness.zoom = FreshnessAfterInvalidation(snapshot.zoom.valid);
            break;
        }
    }

    inline void MarkCameraWriterSourceChanged(CameraStateSnapshot& snapshot,
        bool snapshotExists) noexcept
    {
        MarkCameraStateGroupHistorical(snapshot, snapshotExists,
            CameraStateFreshnessGroup::WriterObservation);
        MarkCameraStateGroupHistorical(snapshot, snapshotExists,
            CameraStateFreshnessGroup::TransformedResult);
        MarkCameraStateGroupHistorical(snapshot, snapshotExists,
            CameraStateFreshnessGroup::GameplayFov);
        snapshot.evidence.gameplayReferenceUsable = false;
        MarkFovFactHistorical(snapshot.evidence.gameplayFov);
    }

    inline void MarkCameraPresentationTransition(CameraStateSnapshot& snapshot,
        bool snapshotExists) noexcept
    {
        // A lifecycle boundary invalidates the *current writer observation*
        // until a writer transaction for the new owner is published. The
        // numeric sample remains valid history; it must not appear live merely
        // because no new writer callback has arrived yet.
        MarkCameraStateGroupHistorical(snapshot, snapshotExists,
            CameraStateFreshnessGroup::WriterObservation);
        MarkCameraStateGroupHistorical(snapshot, snapshotExists,
            CameraStateFreshnessGroup::Presentation);
        MarkCameraStateGroupHistorical(snapshot, snapshotExists,
            CameraStateFreshnessGroup::TransformedResult);
        MarkCameraStateGroupHistorical(snapshot, snapshotExists,
            CameraStateFreshnessGroup::GameplayFov);
    }

    inline void MarkCameraDialogueTransition(CameraStateSnapshot& snapshot,
        bool snapshotExists) noexcept
    {
        MarkCameraStateGroupHistorical(snapshot, snapshotExists,
            CameraStateFreshnessGroup::Dialogue);
        MarkCameraStateGroupHistorical(snapshot, snapshotExists,
            CameraStateFreshnessGroup::TransformedResult);
        MarkCameraStateGroupHistorical(snapshot, snapshotExists,
            CameraStateFreshnessGroup::GameplayFov);
    }

    inline void MarkCameraZoomTransition(CameraStateSnapshot& snapshot,
        bool snapshotExists) noexcept
    {
        MarkCameraStateGroupHistorical(snapshot, snapshotExists,
            CameraStateFreshnessGroup::Zoom);
        MarkCameraStateGroupHistorical(snapshot, snapshotExists,
            CameraStateFreshnessGroup::NeutralGameplayFov);
    }

    inline bool IsUsableCameraFov(float value) noexcept
    {
        return std::isfinite(value) && value > 1.0f && value < 179.0f;
    }

    inline void PublishGameplayReference(CameraEvidenceSnapshot& evidence,
        const GameplayBaseline& baseline, bool provided,
        std::uintptr_t receiver, bool liveGameplay) noexcept
    {
        if (!provided) return;
        evidence.gameplayReferenceUsable = IsUsableGameplayBaseline(baseline) &&
            baseline.source.valid && baseline.source.value == receiver;
        if (!baseline.valid || !baseline.nativeFov.valid) {
            evidence.gameplayFov = {};
            return;
        }
        evidence.gameplayFov = {baseline.nativeFov.value, true,
            EvidenceProvenance::NativeNumericObservation,
            !evidence.gameplayReferenceUsable ? SnapshotFreshness::Historical
                : liveGameplay ? SnapshotFreshness::Current : SnapshotFreshness::Retained,
            baseline.source.value, baseline.source.valid, baseline.observationSequence,
            CameraFovBoundary::GameplayWriter};
    }

    inline void RetireWriterReadbacks(CameraEvidenceSnapshot& evidence) noexcept
    {
        evidence.postStoreFov = {};
        evidence.postHelperFov = {};
        evidence.writerTransaction = {};
    }

    inline void UpdateNeutralGameplayFov(CameraStatePublicationView state,
        const CameraStatePublicationInput& input, CameraFovFact& neutral) noexcept
    {
        const auto& observation = input.observation;
        const float nativeFov = observation.inputFov.value;
        const bool nativeValid = observation.inputFov.valid &&
            observation.inputFov.space == FovSpace::Native &&
            std::isfinite(nativeFov);
        const bool horPlusValid = observation.resultFov.valid &&
            observation.resultFov.space == FovSpace::Transformed &&
            std::isfinite(observation.resultFov.value);
        const bool validWriterAspect = observation.aspect.valid &&
            std::isfinite(observation.aspect.value) && observation.aspect.value > 0.0f;
        const bool validWriterSource = observation.writerSource.valid &&
            observation.writerSource.value != 0 && input.source != 0 &&
            observation.writerSource.value == input.source;
        const bool matchingResultSemantics = input.applied
            ? horPlusValid && observation.resultFov.provenance == FovProvenance::ModTransformResult
            : observation.resultFov.valid &&
                observation.resultFov.space == FovSpace::Native &&
                observation.resultFov.provenance == FovProvenance::PassThroughResult &&
                IsUsableCameraFov(observation.resultFov.value);
        // The baseline is a stable neutral Gameplay reference, not necessarily
        // an ultrawide transform. Pass-through is valid at native aspects too;
        // source, writer-boundary, lifecycle and zoom gates remain mandatory.
        const bool neutralContext = input.presentation == PresentationState::Gameplay &&
            input.dialogue == DialogueState::Inactive && !input.recoveryExcluded &&
            !input.modeTransitionPending && input.zoomObserverAvailable &&
            input.zoomObservationValid && !input.zoomActive && nativeValid &&
            IsUsableCameraFov(nativeFov) && validWriterAspect && validWriterSource &&
            observation.boundary == FovObservationBoundary::CameraWriter &&
            observation.observedDuringGameplay && matchingResultSemantics;

        if (!neutralContext && !state.zoomHold.load(std::memory_order_acquire)) {
            state.neutralGameplayFovSamples = 0;
            state.neutralGameplayFovCandidate = std::numeric_limits<float>::quiet_NaN();
            state.neutralGameplayFovCandidateSource = 0;
        }

        if (neutralContext) {
            // A valid OUT can be the first known inactive observation. Without
            // a prior neutral reference there is nothing for hold to restore;
            // start the existing stable-sample qualification from this point.
            if (!neutral.valid)
                state.zoomHold.store(false, std::memory_order_release);
            if (state.zoomHold.load(std::memory_order_acquire)) {
                if (neutral.valid &&
                    std::fabs(neutral.value - nativeFov) <= 0.01f) {
                    ++state.neutralGameplayFovSamples;
                    if (state.neutralGameplayFovSamples >= 3) {
                        state.zoomHold.store(false, std::memory_order_release);
                        state.neutralGameplayFovSamples = 0;
                    }
                } else {
                    state.neutralGameplayFovSamples = 0;
                }
            } else if (std::isfinite(state.neutralGameplayFovCandidate) &&
                state.neutralGameplayFovCandidateSource == input.source &&
                std::fabs(state.neutralGameplayFovCandidate - nativeFov) <= 0.01f) {
                ++state.neutralGameplayFovSamples;
            } else {
                state.neutralGameplayFovCandidate = nativeFov;
                state.neutralGameplayFovCandidateSource = input.source;
                state.neutralGameplayFovSamples = 1;
            }

            if (state.neutralGameplayFovSamples >= 3 &&
                (!neutral.valid ||
                    std::fabs(neutral.value - nativeFov) > 0.01f)) {
                neutral.value = nativeFov;
                neutral.valid = true;
                neutral.provenance =
                    EvidenceProvenance::NativeNumericObservation;
            }
        }

        const bool gameplayFovConfirmed = neutralContext &&
            state.neutralGameplayFovSamples >= 3;
        if (gameplayFovConfirmed) {
            neutral.source = input.source;
            neutral.sourceValid = input.source != 0;
            neutral.freshness = neutral.valid
                ? SnapshotFreshness::Current : SnapshotFreshness::Unavailable;
        } else if (!neutralContext && neutral.valid) {
            neutral.freshness = SnapshotFreshness::Historical;
        } else if (neutral.valid &&
            (neutral.source != input.source ||
                std::fabs(neutral.value - nativeFov) > 0.01f)) {
            neutral.freshness = SnapshotFreshness::Historical;
        }

    }

    inline CameraStatePublicationResult PublishCameraStateSnapshotLocked(
        CameraStatePublicationView state, const CameraStatePublicationInput& input)
    {
        CameraStatePublicationResult result{};
        if (!input.observationPublished) return result;

        const auto& observation = input.observation;
        const float nativeFov = observation.inputFov.value;
        const bool nativeValid = observation.inputFov.valid &&
            observation.inputFov.space == FovSpace::Native &&
            std::isfinite(nativeFov);
        const bool horPlusValid = observation.resultFov.valid &&
            observation.resultFov.space == FovSpace::Transformed &&
            std::isfinite(observation.resultFov.value);
        result.previous = state.valid ? state.snapshot : CameraStateSnapshot{};
        auto next = result.previous;
        UpdateNeutralGameplayFov(state, input, next.evidence.neutralGameplayFov);

        next.presentation.state = input.presentation;
        next.presentation.provenance = EvidenceProvenance::ModDerivedState;
        next.freshness.presentation = input.presentation == PresentationState::Unknown
            ? SnapshotFreshness::Unavailable : SnapshotFreshness::Current;
        switch (input.dialogue) {
        case DialogueState::Inactive:
            next.dialogue.state = DialogueState::Inactive;
            next.dialogue.provenance = EvidenceProvenance::ConfirmedModLifecycle;
            break;
        case DialogueState::Candidate:
            next.dialogue.state = DialogueState::Candidate;
            next.dialogue.provenance = EvidenceProvenance::ClassifierHypothesis;
            break;
        case DialogueState::Active:
        case DialogueState::Exiting:
        case DialogueState::Recovery:
            next.dialogue.state = input.dialogue;
            next.dialogue.provenance = EvidenceProvenance::ConfirmedModLifecycle;
            next.dialogue.activePolicyValid = input.activeDialoguePolicyValid;
            break;
        case DialogueState::Unknown:
            break;
        }
        next.dialogue.activePolicyValid = input.activeDialoguePolicyValid;
        next.freshness.dialogue = input.dialogue == DialogueState::Unknown
            ? SnapshotFreshness::Unavailable : SnapshotFreshness::Current;
        next.dialogue.recoveryExclusionActive = input.recoveryExcluded;
        next.freshness.writerObservation = nativeValid &&
            observation.aspect.valid && observation.writerFlagsValid
            ? SnapshotFreshness::Current : SnapshotFreshness::Unavailable;
        next.evidence.writerObservationHasEvidence = nativeValid ||
            (observation.aspect.valid && std::isfinite(observation.aspect.value));
        next.freshness.transformedResult = horPlusValid
            ? SnapshotFreshness::Current : SnapshotFreshness::Unavailable;
        next.evidence.writerInputFov = {nativeFov, nativeValid,
            nativeValid ? EvidenceProvenance::NativeNumericObservation
                        : EvidenceProvenance::Unavailable,
            nativeValid ? SnapshotFreshness::Current : SnapshotFreshness::Unavailable,
            input.source, input.source != 0, 0};
        next.evidence.writerInputFov.boundary = CameraFovBoundary::GameplayWriter;
        if (input.originalInputAvailable && nativeValid) {
            next.evidence.nativeFov = next.evidence.writerInputFov;
        } else if (next.evidence.nativeFov.valid) {
            next.evidence.nativeFov.freshness = SnapshotFreshness::Historical;
        }
        PublishGameplayReference(next.evidence, input.gameplayReference,
            input.gameplayReferenceProvided, input.source,
            observation.writerContext == WriterContext::GameplayEligible &&
                input.presentation == PresentationState::Gameplay &&
                input.dialogue == DialogueState::Inactive);
        RetireWriterReadbacks(next.evidence);
        const bool outputValid = observation.resultFov.valid &&
            IsUsableCameraFov(observation.resultFov.value);
        next.evidence.currentOutputFov = {
            outputValid ? observation.resultFov.value
                        : std::numeric_limits<float>::quiet_NaN(),
            outputValid,
            outputValid ? (input.applied ? EvidenceProvenance::ModDerivedState
                                         : EvidenceProvenance::PassThroughResult)
                        : EvidenceProvenance::Unavailable,
            outputValid ? SnapshotFreshness::Current : SnapshotFreshness::Unavailable,
            input.source, input.source != 0, 0};
        next.evidence.currentOutputFov.boundary = CameraFovBoundary::GameplayWriter;
        next.evidence.writerTransaction = input.writerTransaction;
        next.evidence.currentOutputFov.writerTransactionId = input.writerTransaction.id;
        next.evidence.writerInputFov.writerTransactionId = input.writerTransaction.id;
        if (input.applied && horPlusValid) {
            next.evidence.horPlusTargetFov = next.evidence.currentOutputFov;
            next.evidence.horPlusTargetFov.provenance =
                EvidenceProvenance::ModDerivedState;
        } else if (next.evidence.horPlusTargetFov.valid) {
            next.evidence.horPlusTargetFov.freshness = SnapshotFreshness::Historical;
        }
        next.evidence.aspect = observation.aspect.value;
        next.evidence.aspectValid = observation.aspect.valid &&
            std::isfinite(observation.aspect.value);
        next.evidence.aspectProvenance = EvidenceProvenance::NativeNumericObservation;
        next.evidence.gameplayPolicyAspect = observation.policyAspect.value;
        next.evidence.gameplayPolicyAspectValid = observation.policyAspect.valid &&
            std::isfinite(observation.policyAspect.value) &&
            observation.policyAspect.value > 0.0f;
        next.evidence.gameplayPolicyAspectProvenance =
            next.evidence.gameplayPolicyAspectValid
                ? EvidenceProvenance::NativeNumericObservation
                : EvidenceProvenance::Unavailable;
        next.evidence.currentViewportAspect = observation.currentViewportAspect.value;
        next.evidence.currentViewportAspectValid =
            observation.currentViewportAspect.valid &&
            std::isfinite(observation.currentViewportAspect.value) &&
            observation.currentViewportAspect.value > 0.0f;
        next.evidence.currentViewportAspectProvenance =
            next.evidence.currentViewportAspectValid
                ? EvidenceProvenance::ClientViewportObservation
                : EvidenceProvenance::Unavailable;
        next.evidence.runtimeCombineValid = input.runtimeCombineValid;
        next.evidence.runtimeCombineAspectBefore = input.runtimeCombineAspectBefore;
        next.evidence.runtimeCombineTargetAspect = input.runtimeCombineTargetAspect;
        next.evidence.runtimeCombineAspectAfter = input.runtimeCombineAspectAfter;
        next.evidence.runtimeCombineFovInput = input.runtimeCombineFovInput;
        next.evidence.runtimeCombineFovOutput = input.runtimeCombineFovOutput;
        next.evidence.runtimeCombineFlagsBefore = input.runtimeCombineFlagsBefore;
        next.evidence.runtimeCombineFlagsAfter = input.runtimeCombineFlagsAfter;
        next.evidence.runtimeCombineMismatch = input.runtimeCombineMismatch;
        next.evidence.runtimeCombineWriteAttempted = input.runtimeCombineWriteAttempted;
        next.evidence.runtimeCombineWriteSucceeded = input.runtimeCombineWriteSucceeded;
        next.evidence.runtimeCombineReadbackValid = input.runtimeCombineReadbackValid;
        next.evidence.runtimeCombineConfirmed = input.runtimeCombineConfirmed;
        next.evidence.runtimeCombineFovApplied = input.runtimeCombineFovApplied;
        next.evidence.flags = observation.writerFlags;
        next.evidence.configuredGameplayFovKnown = false;
        next.zoom.active = input.zoomActive;
        next.zoom.direction = input.zoomDirection;
        next.zoom.sequence = input.zoomSequence;
        next.zoom.valid = next.zoom.sequence != 0;
        next.zoom.source = {input.zoomSource, input.zoomSource != 0};
        next.zoom.primaryWeight = std::isfinite(input.zoomPrimary)
            ? input.zoomPrimary : std::numeric_limits<float>::quiet_NaN();
        next.zoom.secondaryWeight = std::isfinite(input.zoomSecondary)
            ? input.zoomSecondary : std::numeric_limits<float>::quiet_NaN();
        next.zoom.provenance = next.zoom.valid
            ? EvidenceProvenance::NativeEvent : EvidenceProvenance::Unavailable;
        next.source.gameplayWriter = {input.source, input.source != 0};
        next.gameplayMode = {input.gameplayMode,
            EvidenceProvenance::ModDerivedState, true};
        next.freshness.gameplayMode = SnapshotFreshness::Current;
        if (input.zoomObserverAvailable && input.zoomObservationValid && next.zoom.valid)
            next.freshness.zoom = SnapshotFreshness::Current;
        else if (next.zoom.valid)
            next.freshness.zoom = SnapshotFreshness::Historical;
        else
            next.freshness.zoom = SnapshotFreshness::Unavailable;
        next.generation.eventSequence =
            state.eventSequence.fetch_add(1, std::memory_order_acq_rel) + 1;
        next.generation.eventSequenceValid = true;
        next.evidence.writerInputFov.sequence = next.generation.eventSequence;
        if (input.originalInputAvailable)
            next.evidence.nativeFov.sequence = next.generation.eventSequence;
        next.evidence.currentOutputFov.sequence = next.generation.eventSequence;
        if (next.evidence.horPlusTargetFov.valid && input.applied)
            next.evidence.horPlusTargetFov.sequence = next.generation.eventSequence;
        if (next.evidence.neutralGameplayFov.valid)
            next.evidence.neutralGameplayFov.sequence = next.generation.eventSequence;
        state.snapshot = next;
        state.valid = true;
        result.published = true;
        result.current = next;
        return result;
    }

    struct CameraStateReadResult
    {
        CameraStateSnapshot snapshot{};
        bool valid{};
    };

    struct CameraFovTransactionPublication
    {
        float nativeInput{std::numeric_limits<float>::quiet_NaN()};
        bool nativeInputValid{};
        EvidenceProvenance nativeProvenance{EvidenceProvenance::NativeNumericObservation};
        float horPlusTarget{std::numeric_limits<float>::quiet_NaN()};
        bool horPlusTargetValid{};
        EvidenceProvenance targetProvenance{EvidenceProvenance::ModDerivedState};
        float output{std::numeric_limits<float>::quiet_NaN()};
        bool outputValid{};
        EvidenceProvenance outputProvenance{EvidenceProvenance::PassThroughResult};
        // Context of this specific FOV publication transaction. This is not
        // the snapshot's last full-publication presentation or live coordinator.
        PresentationState transactionOwner{PresentationState::Unknown};
        std::uintptr_t source{};
        std::uint64_t sequence{};
        CameraFovBoundary boundary{CameraFovBoundary::DialogueSetter};
        CameraWriterTransactionIdentity writerTransaction{};
        bool writerInputPublication{};
        GameplayBaseline gameplayReference{};
        bool gameplayReferenceProvided{};
        std::uintptr_t referenceReceiver{};
        bool liveGameplayReference{};
    };

    inline void PublishCameraFovTransactionLocked(CameraStateSnapshot& snapshot,
        bool& snapshotValid, const CameraFovTransactionPublication& transaction)
    {
        const auto setCurrent = [&](CameraFovFact& fact, float value, bool valid,
            EvidenceProvenance provenance) {
            fact.value = valid && IsUsableCameraFov(value)
                ? value : std::numeric_limits<float>::quiet_NaN();
            fact.valid = valid && IsUsableCameraFov(value);
            fact.provenance = fact.valid ? provenance : EvidenceProvenance::Unavailable;
            fact.freshness = fact.valid ? SnapshotFreshness::Current
                                        : SnapshotFreshness::Unavailable;
            fact.source = transaction.source;
            fact.sourceValid = transaction.source != 0;
            fact.sequence = transaction.sequence;
        };

        if (transaction.transactionOwner != PresentationState::Gameplay &&
            snapshot.evidence.gameplayFov.valid &&
            snapshot.evidence.gameplayReferenceUsable) {
            snapshot.evidence.gameplayFov.freshness = SnapshotFreshness::Retained;
            snapshot.evidence.gameplayFov.provenance =
                EvidenceProvenance::NativeNumericObservation;
        }
        if (transaction.nativeProvenance == EvidenceProvenance::NativeNumericObservation &&
            transaction.nativeInputValid) {
            setCurrent(snapshot.evidence.nativeFov, transaction.nativeInput,
                true, transaction.nativeProvenance);
            snapshot.evidence.nativeFov.boundary = transaction.boundary;
        } else if (snapshot.evidence.nativeFov.valid) {
            snapshot.evidence.nativeFov.freshness = SnapshotFreshness::Historical;
        }
        if (transaction.writerInputPublication) {
            setCurrent(snapshot.evidence.writerInputFov, transaction.nativeInput,
                transaction.nativeInputValid, EvidenceProvenance::NativeNumericObservation);
            snapshot.evidence.writerInputFov.boundary = CameraFovBoundary::GameplayWriter;
        }
        setCurrent(snapshot.evidence.currentOutputFov, transaction.output,
            transaction.outputValid, transaction.outputProvenance);
        snapshot.evidence.currentOutputFov.boundary = transaction.boundary;
        RetireWriterReadbacks(snapshot.evidence);
        snapshot.evidence.writerTransaction = transaction.writerTransaction;
        snapshot.evidence.currentOutputFov.writerTransactionId = transaction.writerTransaction.id;
        if (transaction.writerInputPublication)
            snapshot.evidence.writerInputFov.writerTransactionId = transaction.writerTransaction.id;
        PublishGameplayReference(snapshot.evidence, transaction.gameplayReference,
            transaction.gameplayReferenceProvided, transaction.referenceReceiver,
            transaction.liveGameplayReference);
        if (transaction.horPlusTargetValid)
            setCurrent(snapshot.evidence.horPlusTargetFov,
                transaction.horPlusTarget, true, transaction.targetProvenance);
        else if (snapshot.evidence.horPlusTargetFov.valid)
            snapshot.evidence.horPlusTargetFov.freshness = SnapshotFreshness::Retained;

        snapshotValid = true;
    }

    struct CameraFovReadbackPublication
    {
        CameraWriterTransactionIdentity identity{};
        CameraFovBoundary boundary{CameraFovBoundary::Unavailable};
        float value{std::numeric_limits<float>::quiet_NaN()};
        bool readable{};
        bool correlated{};
    };

    inline bool PublishCameraFovReadbackLocked(CameraStateSnapshot& snapshot,
        bool snapshotValid, const CameraFovReadbackPublication& readback) noexcept
    {
        if (!snapshotValid || !readback.correlated || !readback.identity.Valid() ||
            !(readback.identity == snapshot.evidence.writerTransaction) ||
            snapshot.evidence.currentOutputFov.writerTransactionId != readback.identity.id)
            return false;
        CameraFovFact* fact{};
        if (readback.boundary == CameraFovBoundary::PrimaryPostStore)
            fact = &snapshot.evidence.postStoreFov;
        else if (readback.boundary == CameraFovBoundary::PostHelper)
            fact = &snapshot.evidence.postHelperFov;
        else return false;
        *fact = {};
        if (readback.readable && IsUsableCameraFov(readback.value))
            *fact = {readback.value, true, EvidenceProvenance::NativeNumericObservation,
                SnapshotFreshness::Current, readback.identity.receiver, true,
                snapshot.generation.eventSequence, readback.boundary, readback.identity.id};
        return true;
    }

    inline CameraStateReadResult ReadCameraStateSnapshotLocked(
        const CameraStateSnapshot& snapshot, bool valid) noexcept
    {
        return {snapshot, valid};
    }
}
