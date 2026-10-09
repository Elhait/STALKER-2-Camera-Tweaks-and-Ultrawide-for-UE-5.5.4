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
        case EvidenceProvenance::PassThroughResult: return "PassThroughResult";
        case EvidenceProvenance::ClientViewportObservation: return "ClientViewportObservation";
        case EvidenceProvenance::ModDerivedState: return "ModDerivedState";
        case EvidenceProvenance::ClassifierHypothesis: return "ClassifierHypothesis";
        case EvidenceProvenance::ConfirmedModLifecycle: return "ConfirmedModLifecycle";
        }
        return "Unknown";
    }

    const char* SnapshotFreshnessName(SnapshotFreshness value) noexcept
    {
        switch (value) {
        case SnapshotFreshness::Unavailable: return "Unavailable";
        case SnapshotFreshness::Current: return "Current";
        case SnapshotFreshness::Retained: return "Retained";
        case SnapshotFreshness::Historical: return "Last known";
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
            FloatChanged(current.evidence.gameplayFov.value, previous.evidence.gameplayFov.value) ||
            FloatChanged(current.evidence.nativeFov.value, previous.evidence.nativeFov.value) ||
            FloatChanged(current.evidence.horPlusTargetFov.value, previous.evidence.horPlusTargetFov.value) ||
            FloatChanged(current.evidence.currentOutputFov.value, previous.evidence.currentOutputFov.value) ||
            current.evidence.gameplayFov.source != previous.evidence.gameplayFov.source ||
            current.evidence.gameplayFov.sourceValid != previous.evidence.gameplayFov.sourceValid ||
            current.evidence.nativeFov.source != previous.evidence.nativeFov.source ||
            current.evidence.nativeFov.sourceValid != previous.evidence.nativeFov.sourceValid ||
            current.evidence.horPlusTargetFov.source != previous.evidence.horPlusTargetFov.source ||
            current.evidence.horPlusTargetFov.sourceValid != previous.evidence.horPlusTargetFov.sourceValid ||
            current.evidence.currentOutputFov.source != previous.evidence.currentOutputFov.source ||
            current.evidence.currentOutputFov.sourceValid != previous.evidence.currentOutputFov.sourceValid ||
            FloatChanged(current.evidence.aspect, previous.evidence.aspect) ||
            FloatChanged(current.evidence.gameplayPolicyAspect, previous.evidence.gameplayPolicyAspect) ||
            FloatChanged(current.evidence.currentViewportAspect, previous.evidence.currentViewportAspect) ||
            FloatChanged(current.evidence.runtimeCombineAspectBefore, previous.evidence.runtimeCombineAspectBefore) ||
            FloatChanged(current.evidence.runtimeCombineTargetAspect, previous.evidence.runtimeCombineTargetAspect) ||
            FloatChanged(current.evidence.runtimeCombineAspectAfter, previous.evidence.runtimeCombineAspectAfter) ||
            FloatChanged(current.evidence.runtimeCombineFovInput, previous.evidence.runtimeCombineFovInput) ||
            FloatChanged(current.evidence.runtimeCombineFovOutput, previous.evidence.runtimeCombineFovOutput) ||
            current.evidence.runtimeCombineValid != previous.evidence.runtimeCombineValid ||
            current.evidence.runtimeCombineFlagsBefore != previous.evidence.runtimeCombineFlagsBefore ||
            current.evidence.runtimeCombineFlagsAfter != previous.evidence.runtimeCombineFlagsAfter ||
            current.evidence.runtimeCombineMismatch != previous.evidence.runtimeCombineMismatch ||
            current.evidence.runtimeCombineWriteAttempted != previous.evidence.runtimeCombineWriteAttempted ||
            current.evidence.runtimeCombineWriteSucceeded != previous.evidence.runtimeCombineWriteSucceeded ||
            current.evidence.runtimeCombineReadbackValid != previous.evidence.runtimeCombineReadbackValid ||
            current.evidence.runtimeCombineConfirmed != previous.evidence.runtimeCombineConfirmed ||
            current.evidence.runtimeCombineFovApplied != previous.evidence.runtimeCombineFovApplied ||
            FloatChanged(current.evidence.gameplayFov.value, previous.evidence.gameplayFov.value) ||
            FloatChanged(current.evidence.nativeFov.value, previous.evidence.nativeFov.value) ||
            FloatChanged(current.evidence.horPlusTargetFov.value, previous.evidence.horPlusTargetFov.value) ||
            FloatChanged(current.evidence.currentOutputFov.value, previous.evidence.currentOutputFov.value) ||
            FloatChanged(current.evidence.aspect, previous.evidence.aspect) ||
            FloatChanged(current.evidence.gameplayPolicyAspect, previous.evidence.gameplayPolicyAspect) ||
            FloatChanged(current.evidence.currentViewportAspect, previous.evidence.currentViewportAspect) ||
            FloatChanged(current.evidence.runtimeCombineAspectBefore, previous.evidence.runtimeCombineAspectBefore) ||
            FloatChanged(current.evidence.runtimeCombineTargetAspect, previous.evidence.runtimeCombineTargetAspect) ||
            FloatChanged(current.evidence.runtimeCombineAspectAfter, previous.evidence.runtimeCombineAspectAfter) ||
            FloatChanged(current.evidence.runtimeCombineFovInput, previous.evidence.runtimeCombineFovInput) ||
            FloatChanged(current.evidence.runtimeCombineFovOutput, previous.evidence.runtimeCombineFovOutput) ||
            current.evidence.runtimeCombineValid != previous.evidence.runtimeCombineValid ||
            current.evidence.runtimeCombineFlagsBefore != previous.evidence.runtimeCombineFlagsBefore ||
            current.evidence.runtimeCombineFlagsAfter != previous.evidence.runtimeCombineFlagsAfter ||
            current.evidence.runtimeCombineMismatch != previous.evidence.runtimeCombineMismatch ||
            current.evidence.runtimeCombineWriteAttempted != previous.evidence.runtimeCombineWriteAttempted ||
            current.evidence.runtimeCombineWriteSucceeded != previous.evidence.runtimeCombineWriteSucceeded ||
            current.evidence.runtimeCombineReadbackValid != previous.evidence.runtimeCombineReadbackValid ||
            current.evidence.runtimeCombineConfirmed != previous.evidence.runtimeCombineConfirmed ||
            current.evidence.runtimeCombineFovApplied != previous.evidence.runtimeCombineFovApplied ||
            current.evidence.flags != previous.evidence.flags ||
            current.evidence.gameplayFov.valid != previous.evidence.gameplayFov.valid ||
            current.evidence.gameplayFov.freshness != previous.evidence.gameplayFov.freshness ||
            current.evidence.nativeFov.valid != previous.evidence.nativeFov.valid ||
            current.evidence.nativeFov.freshness != previous.evidence.nativeFov.freshness ||
            current.evidence.horPlusTargetFov.valid != previous.evidence.horPlusTargetFov.valid ||
            current.evidence.horPlusTargetFov.freshness != previous.evidence.horPlusTargetFov.freshness ||
            current.evidence.currentOutputFov.valid != previous.evidence.currentOutputFov.valid ||
            current.evidence.currentOutputFov.freshness != previous.evidence.currentOutputFov.freshness ||
            current.evidence.aspectValid != previous.evidence.aspectValid ||
            current.evidence.writerObservationHasEvidence !=
                previous.evidence.writerObservationHasEvidence ||
            current.evidence.gameplayPolicyAspectValid != previous.evidence.gameplayPolicyAspectValid ||
            current.evidence.currentViewportAspectValid != previous.evidence.currentViewportAspectValid ||
            current.evidence.runtimeCombineValid != previous.evidence.runtimeCombineValid ||
            current.evidence.runtimeCombineFlagsBefore != previous.evidence.runtimeCombineFlagsBefore ||
            current.evidence.runtimeCombineFlagsAfter != previous.evidence.runtimeCombineFlagsAfter ||
            current.evidence.runtimeCombineMismatch != previous.evidence.runtimeCombineMismatch ||
            current.evidence.runtimeCombineWriteAttempted != previous.evidence.runtimeCombineWriteAttempted ||
            current.evidence.runtimeCombineWriteSucceeded != previous.evidence.runtimeCombineWriteSucceeded ||
            current.evidence.runtimeCombineReadbackValid != previous.evidence.runtimeCombineReadbackValid ||
            current.evidence.runtimeCombineConfirmed != previous.evidence.runtimeCombineConfirmed ||
            current.evidence.runtimeCombineFovApplied != previous.evidence.runtimeCombineFovApplied ||
            current.evidence.aspectProvenance != previous.evidence.aspectProvenance ||
            current.evidence.gameplayPolicyAspectProvenance != previous.evidence.gameplayPolicyAspectProvenance ||
            current.evidence.currentViewportAspectProvenance != previous.evidence.currentViewportAspectProvenance ||
            current.evidence.configuredGameplayFovKnown != previous.evidence.configuredGameplayFovKnown ||
            current.freshness.writerObservation != previous.freshness.writerObservation ||
            current.freshness.transformedResult != previous.freshness.transformedResult ||
            current.freshness.presentation != previous.freshness.presentation ||
            current.freshness.dialogue != previous.freshness.dialogue ||
            current.freshness.gameplayMode != previous.freshness.gameplayMode ||
            current.freshness.zoom != previous.freshness.zoom ||
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
        // eventSequence is a per-publication counter; treating it as a visual
        // semantic change would wake the overlay for every writer sample.
        return current.presentation.state != previous.presentation.state ||
            current.presentation.provenance != previous.presentation.provenance ||
            current.presentation.epoch != previous.presentation.epoch ||
            current.presentation.epochValid != previous.presentation.epochValid ||
            current.zoom.direction != previous.zoom.direction ||
            current.zoom.provenance != previous.zoom.provenance ||
            current.zoom.valid != previous.zoom.valid ||
            current.zoom.active != previous.zoom.active ||
            current.zoom.sequence != previous.zoom.sequence ||
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
            FloatChanged(current.evidence.gameplayFov.value, previous.evidence.gameplayFov.value) ||
            FloatChanged(current.evidence.nativeFov.value, previous.evidence.nativeFov.value) ||
            FloatChanged(current.evidence.horPlusTargetFov.value, previous.evidence.horPlusTargetFov.value) ||
            FloatChanged(current.evidence.currentOutputFov.value, previous.evidence.currentOutputFov.value) ||
            current.evidence.gameplayFov.provenance != previous.evidence.gameplayFov.provenance ||
            current.evidence.nativeFov.provenance != previous.evidence.nativeFov.provenance ||
            current.evidence.horPlusTargetFov.provenance != previous.evidence.horPlusTargetFov.provenance ||
            current.evidence.currentOutputFov.provenance != previous.evidence.currentOutputFov.provenance ||
            current.evidence.gameplayFov.source != previous.evidence.gameplayFov.source ||
            current.evidence.gameplayFov.sourceValid != previous.evidence.gameplayFov.sourceValid ||
            current.evidence.nativeFov.source != previous.evidence.nativeFov.source ||
            current.evidence.nativeFov.sourceValid != previous.evidence.nativeFov.sourceValid ||
            current.evidence.horPlusTargetFov.source != previous.evidence.horPlusTargetFov.source ||
            current.evidence.horPlusTargetFov.sourceValid != previous.evidence.horPlusTargetFov.sourceValid ||
            current.evidence.currentOutputFov.source != previous.evidence.currentOutputFov.source ||
            current.evidence.currentOutputFov.sourceValid != previous.evidence.currentOutputFov.sourceValid ||
            FloatChanged(current.evidence.aspect, previous.evidence.aspect) ||
            FloatChanged(current.evidence.gameplayPolicyAspect, previous.evidence.gameplayPolicyAspect) ||
            FloatChanged(current.evidence.currentViewportAspect, previous.evidence.currentViewportAspect) ||
            FloatChanged(current.evidence.runtimeCombineAspectBefore, previous.evidence.runtimeCombineAspectBefore) ||
            FloatChanged(current.evidence.runtimeCombineTargetAspect, previous.evidence.runtimeCombineTargetAspect) ||
            FloatChanged(current.evidence.runtimeCombineAspectAfter, previous.evidence.runtimeCombineAspectAfter) ||
            FloatChanged(current.evidence.runtimeCombineFovInput, previous.evidence.runtimeCombineFovInput) ||
            FloatChanged(current.evidence.runtimeCombineFovOutput, previous.evidence.runtimeCombineFovOutput) ||
            current.evidence.runtimeCombineValid != previous.evidence.runtimeCombineValid ||
            current.evidence.runtimeCombineFlagsBefore != previous.evidence.runtimeCombineFlagsBefore ||
            current.evidence.runtimeCombineFlagsAfter != previous.evidence.runtimeCombineFlagsAfter ||
            current.evidence.runtimeCombineMismatch != previous.evidence.runtimeCombineMismatch ||
            current.evidence.runtimeCombineWriteAttempted != previous.evidence.runtimeCombineWriteAttempted ||
            current.evidence.runtimeCombineWriteSucceeded != previous.evidence.runtimeCombineWriteSucceeded ||
            current.evidence.runtimeCombineReadbackValid != previous.evidence.runtimeCombineReadbackValid ||
            current.evidence.runtimeCombineConfirmed != previous.evidence.runtimeCombineConfirmed ||
            current.evidence.runtimeCombineFovApplied != previous.evidence.runtimeCombineFovApplied ||
            current.evidence.flags != previous.evidence.flags ||
            current.evidence.gameplayFov.valid != previous.evidence.gameplayFov.valid ||
            current.evidence.gameplayFov.freshness != previous.evidence.gameplayFov.freshness ||
            current.evidence.nativeFov.valid != previous.evidence.nativeFov.valid ||
            current.evidence.nativeFov.freshness != previous.evidence.nativeFov.freshness ||
            current.evidence.horPlusTargetFov.valid != previous.evidence.horPlusTargetFov.valid ||
            current.evidence.horPlusTargetFov.freshness != previous.evidence.horPlusTargetFov.freshness ||
            current.evidence.currentOutputFov.valid != previous.evidence.currentOutputFov.valid ||
            current.evidence.currentOutputFov.freshness != previous.evidence.currentOutputFov.freshness ||
            current.evidence.aspectValid != previous.evidence.aspectValid ||
            current.evidence.gameplayPolicyAspectValid != previous.evidence.gameplayPolicyAspectValid ||
            current.evidence.currentViewportAspectValid != previous.evidence.currentViewportAspectValid ||
            current.evidence.aspectProvenance != previous.evidence.aspectProvenance ||
            current.evidence.gameplayPolicyAspectProvenance != previous.evidence.gameplayPolicyAspectProvenance ||
            current.evidence.currentViewportAspectProvenance != previous.evidence.currentViewportAspectProvenance ||
            current.evidence.configuredGameplayFovKnown != previous.evidence.configuredGameplayFovKnown ||
            current.freshness.writerObservation != previous.freshness.writerObservation ||
            current.freshness.transformedResult != previous.freshness.transformedResult ||
            current.freshness.presentation != previous.freshness.presentation ||
            current.freshness.dialogue != previous.freshness.dialogue ||
            current.freshness.gameplayMode != previous.freshness.gameplayMode ||
            current.freshness.zoom != previous.freshness.zoom ||
            current.source.gameplayWriter.value != previous.source.gameplayWriter.value ||
            current.source.gameplayWriter.valid != previous.source.gameplayWriter.valid ||
            current.generation.presentationEpoch != previous.generation.presentationEpoch ||
            current.generation.presentationEpochValid != previous.generation.presentationEpochValid;
    }
}
