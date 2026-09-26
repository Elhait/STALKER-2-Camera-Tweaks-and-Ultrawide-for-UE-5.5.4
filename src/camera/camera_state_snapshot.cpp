#include "camera_state_snapshot.hpp"

#include <cmath>

namespace camera
{
    const char* EvidenceProvenanceName(EvidenceProvenance value) noexcept
    {
        switch (value) {
        case EvidenceProvenance::Unavailable: return "Unavailable";
        case EvidenceProvenance::NativeEvent: return "NativeEvent";
        case EvidenceProvenance::NativeNumericObservation: return "NativeNumericObservation";
        case EvidenceProvenance::ModDerivedState: return "ModDerivedState";
        case EvidenceProvenance::ClassifierHypothesis: return "ClassifierHypothesis";
        case EvidenceProvenance::ConfirmedModLifecycle: return "ConfirmedModLifecycle";
        }
        return "Unknown";
    }

    const char* PresentationStateName(PresentationState value) noexcept
    {
        switch (value) {
        case PresentationState::Unknown: return "Unknown";
        case PresentationState::Gameplay: return "Gameplay";
        case PresentationState::CinematicActive: return "CinematicActive";
        case PresentationState::CinematicExiting: return "CinematicExiting";
        }
        return "Unknown";
    }

    const char* DialogueStateName(DialogueState value) noexcept
    {
        switch (value) {
        case DialogueState::Inactive: return "Inactive";
        case DialogueState::Candidate: return "Candidate";
        case DialogueState::Active: return "Active";
        case DialogueState::Exiting: return "Exiting";
        case DialogueState::Recovery: return "Recovery";
        case DialogueState::Unknown: return "Unknown";
        }
        return "Unknown";
    }

    const char* ZoomDirectionName(ZoomDirection value) noexcept
    {
        switch (value) {
        case ZoomDirection::None: return "None";
        case ZoomDirection::In: return "In";
        case ZoomDirection::Out: return "Out";
        }
        return "Unknown";
    }

    CameraStateSnapshot BuildCameraStateSnapshot(const CameraStateInput& input) noexcept
    {
        return {
            input.presentation, input.zoom, input.dialogue, input.gameplayMode,
            input.evidence, input.source, input.generation };
    }

    namespace
    {
        bool FloatChanged(float current, float previous) noexcept
        {
            return std::isfinite(current) != std::isfinite(previous) ||
                (std::isfinite(current) && std::isfinite(previous) &&
                    std::fabs(current - previous) > 0.01f);
        }
    }

    bool SnapshotChanged(const CameraStateSnapshot& current,
        const CameraStateSnapshot& previous) noexcept
    {
        if (current.presentation.state != previous.presentation.state ||
            current.presentation.provenance != previous.presentation.provenance ||
            current.presentation.epoch != previous.presentation.epoch ||
            current.presentation.epochValid != previous.presentation.epochValid ||
            current.zoom.direction != previous.zoom.direction ||
            current.zoom.provenance != previous.zoom.provenance ||
            current.zoom.valid != previous.zoom.valid ||
            current.zoom.active != previous.zoom.active ||
            FloatChanged(current.zoom.primaryWeight, previous.zoom.primaryWeight) ||
            FloatChanged(current.zoom.secondaryWeight, previous.zoom.secondaryWeight) ||
            current.zoom.source.value != previous.zoom.source.value ||
            current.zoom.source.valid != previous.zoom.source.valid ||
            current.dialogue.state != previous.dialogue.state ||
            current.dialogue.provenance != previous.dialogue.provenance ||
            current.dialogue.activePolicyValid != previous.dialogue.activePolicyValid ||
            current.dialogue.recoveryExclusionActive != previous.dialogue.recoveryExclusionActive ||
            FloatChanged(current.dialogue.nativeTarget, previous.dialogue.nativeTarget) ||
            current.dialogue.nativeTargetValid != previous.dialogue.nativeTargetValid ||
            current.dialogue.source.value != previous.dialogue.source.value ||
            current.dialogue.source.valid != previous.dialogue.source.valid ||
            current.gameplayMode.value != previous.gameplayMode.value ||
            current.gameplayMode.provenance != previous.gameplayMode.provenance ||
            current.gameplayMode.valid != previous.gameplayMode.valid ||
            FloatChanged(current.evidence.retainedGameplayFov, previous.evidence.retainedGameplayFov) ||
            FloatChanged(current.evidence.nativeWriterFov, previous.evidence.nativeWriterFov) ||
            FloatChanged(current.evidence.aspect, previous.evidence.aspect) ||
            current.evidence.flags != previous.evidence.flags ||
            FloatChanged(current.evidence.transformedFov, previous.evidence.transformedFov) ||
            current.evidence.retainedGameplayFovValid != previous.evidence.retainedGameplayFovValid ||
            current.evidence.nativeWriterFovValid != previous.evidence.nativeWriterFovValid ||
            current.evidence.aspectValid != previous.evidence.aspectValid ||
            current.evidence.transformedFovValid != previous.evidence.transformedFovValid ||
            current.evidence.retainedGameplayFovProvenance != previous.evidence.retainedGameplayFovProvenance ||
            current.evidence.nativeWriterFovProvenance != previous.evidence.nativeWriterFovProvenance ||
            current.evidence.aspectProvenance != previous.evidence.aspectProvenance ||
            current.evidence.transformedFovProvenance != previous.evidence.transformedFovProvenance ||
            current.evidence.configuredGameplayFovKnown != previous.evidence.configuredGameplayFovKnown ||
            current.source.gameplayWriter.value != previous.source.gameplayWriter.value ||
            current.source.gameplayWriter.valid != previous.source.gameplayWriter.valid ||
            current.generation.presentationEpoch != previous.generation.presentationEpoch ||
            current.generation.presentationEpochValid != previous.generation.presentationEpochValid ||
            current.generation.eventSequence != previous.generation.eventSequence ||
            current.generation.eventSequenceValid != previous.generation.eventSequenceValid)
            return true;
        return false;
    }

    bool SnapshotSemanticsChanged(const CameraStateSnapshot& current,
        const CameraStateSnapshot& previous) noexcept
    {
        return current.presentation.state != previous.presentation.state ||
            current.presentation.provenance != previous.presentation.provenance ||
            current.presentation.epoch != previous.presentation.epoch ||
            current.presentation.epochValid != previous.presentation.epochValid ||
            current.zoom.direction != previous.zoom.direction ||
            current.zoom.provenance != previous.zoom.provenance ||
            current.zoom.valid != previous.zoom.valid ||
            current.zoom.active != previous.zoom.active ||
            current.zoom.source.value != previous.zoom.source.value ||
            current.zoom.source.valid != previous.zoom.source.valid ||
            current.dialogue.state != previous.dialogue.state ||
            current.dialogue.provenance != previous.dialogue.provenance ||
            current.dialogue.activePolicyValid != previous.dialogue.activePolicyValid ||
            current.dialogue.recoveryExclusionActive != previous.dialogue.recoveryExclusionActive ||
            current.dialogue.nativeTargetValid != previous.dialogue.nativeTargetValid ||
            current.dialogue.source.value != previous.dialogue.source.value ||
            current.dialogue.source.valid != previous.dialogue.source.valid ||
            current.gameplayMode.value != previous.gameplayMode.value ||
            current.gameplayMode.provenance != previous.gameplayMode.provenance ||
            current.gameplayMode.valid != previous.gameplayMode.valid ||
            current.evidence.flags != previous.evidence.flags ||
            current.evidence.retainedGameplayFovValid != previous.evidence.retainedGameplayFovValid ||
            current.evidence.nativeWriterFovValid != previous.evidence.nativeWriterFovValid ||
            current.evidence.aspectValid != previous.evidence.aspectValid ||
            current.evidence.transformedFovValid != previous.evidence.transformedFovValid ||
            current.evidence.retainedGameplayFovProvenance != previous.evidence.retainedGameplayFovProvenance ||
            current.evidence.nativeWriterFovProvenance != previous.evidence.nativeWriterFovProvenance ||
            current.evidence.aspectProvenance != previous.evidence.aspectProvenance ||
            current.evidence.transformedFovProvenance != previous.evidence.transformedFovProvenance ||
            current.evidence.configuredGameplayFovKnown != previous.evidence.configuredGameplayFovKnown ||
            current.source.gameplayWriter.value != previous.source.gameplayWriter.value ||
            current.source.gameplayWriter.valid != previous.source.gameplayWriter.valid ||
            current.generation.presentationEpoch != previous.generation.presentationEpoch ||
            current.generation.presentationEpochValid != previous.generation.presentationEpochValid;
    }
}
