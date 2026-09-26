#include "../../src/camera/camera_state_snapshot.hpp"

#include <cmath>
#include <iostream>
#include <string>

namespace
{
    bool Check(bool value, const char* name)
    {
        if (!value) std::cerr << "FAIL: " << name << "\n";
        return value;
    }
}

int main()
{
    using namespace camera;
    bool pass = true;

    CameraStateInput input{};
    input.presentation = { PresentationState::Gameplay,
        EvidenceProvenance::ModDerivedState, 1, true };
    input.dialogue = { DialogueState::Inactive, EvidenceProvenance::ConfirmedModLifecycle,
        false, false, std::numeric_limits<float>::quiet_NaN(), false, {} };
    input.gameplayMode = { 1, EvidenceProvenance::ModDerivedState, true };
    input.evidence = { 90.0f, 3.55556f, 0x4, 126.87f, true, true, true,
        EvidenceProvenance::NativeNumericObservation,
        EvidenceProvenance::NativeNumericObservation,
        EvidenceProvenance::ModDerivedState, false };
    input.evidence.retainedGameplayFov = 90.6557f;
    input.evidence.retainedGameplayFovValid = true;
    input.evidence.retainedGameplayFovProvenance =
        EvidenceProvenance::NativeNumericObservation;
    input.source.gameplayWriter = { 0x1000, true };
    input.generation = { 1, true, 1, true };
    const auto gameplay = BuildCameraStateSnapshot(input);
    pass &= Check(gameplay.dialogue.state == DialogueState::Inactive,
        "gameplay_dialogue_inactive");
    pass &= Check(gameplay.evidence.configuredGameplayFovKnown == false,
        "configured_fov_unknown");
    pass &= Check(gameplay.evidence.retainedGameplayFovValid &&
        std::fabs(gameplay.evidence.retainedGameplayFov - 90.6557f) < 0.001f,
        "retained_neutral_gameplay_fov");
    pass &= Check(std::fabs(gameplay.evidence.nativeWriterFov - 90.0f) < 0.001f &&
        std::fabs(gameplay.evidence.transformedFov - 126.87f) < 0.001f,
        "coherent_native_horplus_pair");

    input.dialogue.state = DialogueState::Candidate;
    input.dialogue.provenance = EvidenceProvenance::ClassifierHypothesis;
    const auto candidate = BuildCameraStateSnapshot(input);
    pass &= Check(candidate.dialogue.provenance == EvidenceProvenance::ClassifierHypothesis,
        "candidate_is_hypothesis");
    pass &= Check(candidate.presentation.state == gameplay.presentation.state,
        "candidate_does_not_change_presentation");

    input.dialogue.state = DialogueState::Active;
    input.dialogue.provenance = EvidenceProvenance::ConfirmedModLifecycle;
    input.dialogue.activePolicyValid = true;
    const auto active = BuildCameraStateSnapshot(input);
    pass &= Check(active.dialogue.state == DialogueState::Active,
        "active_is_lifecycle_state");
    pass &= Check(std::string(DialogueStateName(active.dialogue.state)) == "Active",
        "named_dialogue_state");
    pass &= Check(active.gameplayMode.value == gameplay.gameplayMode.value,
        "mode_is_independent_axis");

    input.presentation.state = PresentationState::CinematicActive;
    input.presentation.epoch = 2;
    const auto cinematicDialogue = BuildCameraStateSnapshot(input);
    pass &= Check(cinematicDialogue.presentation.state == PresentationState::CinematicActive,
        "cinematic_dialogue_representable");
    pass &= Check(cinematicDialogue.dialogue.state == DialogueState::Active,
        "cinematic_dialogue_preserved");

    input.zoom = { ZoomDirection::In, EvidenceProvenance::NativeEvent, 7, true,
        0.25f, 0.75f, { 0x2000, true }, true };
    const auto zoom = BuildCameraStateSnapshot(input);
    pass &= Check(zoom.zoom.provenance == EvidenceProvenance::NativeEvent,
        "zoom_is_native_observation");
    pass &= Check(zoom.dialogue.state == DialogueState::Active,
        "zoom_does_not_create_owner");
    pass &= Check(zoom.source.gameplayWriter.value == 0x1000 &&
        zoom.source.gameplayWriter.valid,
        "gameplay_source_is_exact");
    pass &= Check(zoom.dialogue.source.value == 0 &&
        !zoom.dialogue.source.valid,
        "dialogue_source_is_unavailable_when_not_observed");
    pass &= Check(zoom.zoom.source.value == 0x2000 &&
        zoom.zoom.source.valid,
        "zoom_source_is_exact");
    pass &= Check(zoom.zoom.direction == ZoomDirection::In &&
        zoom.zoom.sequence == 7 && zoom.zoom.valid,
        "zoom_is_last_observation");
    pass &= Check(zoom.zoom.active, "zoom_activity_is_explicit");

    auto zoomEndpoint = gameplay;
    zoomEndpoint.zoom = { ZoomDirection::Out, EvidenceProvenance::NativeEvent, 8, true,
        0.75f, 0.25f, { 0x2000, true }, false };
    zoomEndpoint.evidence.nativeWriterFov = 26.1406f;
    zoomEndpoint.evidence.transformedFov = 49.813f;
    pass &= Check(zoomEndpoint.evidence.retainedGameplayFovValid &&
        std::fabs(zoomEndpoint.evidence.retainedGameplayFov - 90.6557f) < 0.001f,
        "zoom_endpoint_does_not_replace_retained_gameplay_fov");
    pass &= Check(std::fabs(zoomEndpoint.evidence.nativeWriterFov - 26.1406f) < 0.001f &&
        std::fabs(zoomEndpoint.evidence.transformedFov - 49.813f) < 0.001f,
        "zoom_endpoint_keeps_coherent_live_pair");

    input.dialogue.source = { 0x3000, true };
    const auto dialogueSource = BuildCameraStateSnapshot(input);
    pass &= Check(dialogueSource.dialogue.source.value == 0x3000 &&
        dialogueSource.dialogue.source.valid,
        "dialogue_source_is_exact");

    input.dialogue.recoveryExclusionActive = true;
    const auto recovery = BuildCameraStateSnapshot(input);
    pass &= Check(recovery.dialogue.recoveryExclusionActive,
        "recovery_exclusion_is_observable");

    input.evidence.aspect = std::numeric_limits<float>::quiet_NaN();
    input.evidence.aspectValid = false;
    const auto unavailable = BuildCameraStateSnapshot(input);
    pass &= Check(!unavailable.evidence.aspectValid &&
        !std::isfinite(unavailable.evidence.aspect), "unavailable_remains_unknown");
    pass &= Check(SnapshotChanged(unavailable, zoom), "change_only_detects_snapshot_change");
    pass &= Check(!SnapshotSemanticsChanged(unavailable, unavailable),
        "identical_semantics_are_quiet");
    pass &= Check(!SnapshotChanged(unavailable, unavailable), "identical_snapshot_is_quiet");

    const auto diagnosticsOffState = BuildCameraStateSnapshot(input);
    const auto diagnosticsOnState = BuildCameraStateSnapshot(input);
    pass &= Check(!SnapshotChanged(diagnosticsOffState, diagnosticsOnState),
        "diagnostics_do_not_change_camera_state");

    auto numericOnly = unavailable;
    numericOnly.evidence.nativeWriterFov += 1.0f;
    numericOnly.evidence.transformedFov += 1.0f;
    pass &= Check(!SnapshotSemanticsChanged(numericOnly, unavailable),
        "numeric_animation_is_not_semantic_transition");

    input.generation.presentationEpoch = 3;
    const auto stale = BuildCameraStateSnapshot(input);
    pass &= Check(stale.generation.presentationEpoch != unavailable.generation.presentationEpoch,
        "generation_is_diagnostic_provenance");

    std::cout << "camera_state_snapshot=" << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
