#include "../../src/camera/camera_state_snapshot.hpp"
#include "../../src/camera/camera_state_publication.hpp"
#include "../../src/camera/gameplay_baseline.hpp"

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
    input.evidence.aspect = 3.55556f;
    input.evidence.flags = 0x4;
    input.evidence.aspectValid = true;
    input.evidence.aspectProvenance =
        EvidenceProvenance::NativeNumericObservation;
    input.evidence.nativeFov = {90.0f, true,
        EvidenceProvenance::NativeNumericObservation, SnapshotFreshness::Current,
        0x1000, true, 1};
    input.evidence.currentOutputFov = {126.87f, true,
        EvidenceProvenance::ModDerivedState, SnapshotFreshness::Current,
        0x1000, true, 1};
    input.evidence.horPlusTargetFov = input.evidence.currentOutputFov;
    input.evidence.gameplayFov.value = 90.6557f;
    input.evidence.gameplayFov.valid = true;
    input.evidence.gameplayFov.provenance =
        EvidenceProvenance::NativeNumericObservation;
    input.evidence.gameplayFov.freshness = SnapshotFreshness::Current;
    input.source.gameplayWriter = { 0x1000, true };
    input.generation = { 1, true, 1, true };
    const auto gameplay = BuildCameraStateSnapshot(input);
    pass &= Check(gameplay.dialogue.state == DialogueState::Inactive,
        "gameplay_dialogue_inactive");
    pass &= Check(gameplay.evidence.configuredGameplayFovKnown == false,
        "configured_fov_unknown");
    pass &= Check(gameplay.evidence.gameplayFov.valid &&
        std::fabs(gameplay.evidence.gameplayFov.value - 90.6557f) < 0.001f,
        "canonical_gameplay_fov");
    pass &= Check(std::fabs(gameplay.evidence.nativeFov.value - 90.0f) < 0.001f &&
        std::fabs(gameplay.evidence.horPlusTargetFov.value - 126.87f) < 0.001f &&
        std::fabs(gameplay.evidence.currentOutputFov.value - 126.87f) < 0.001f,
        "canonical_native_target_output_roles");

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
    zoomEndpoint.evidence.nativeFov.value = 26.1406f;
    zoomEndpoint.evidence.currentOutputFov.value = 49.813f;
    zoomEndpoint.evidence.horPlusTargetFov.value = 49.813f;
    pass &= Check(zoomEndpoint.evidence.gameplayFov.valid &&
        std::fabs(zoomEndpoint.evidence.gameplayFov.value - 90.6557f) < 0.001f,
        "zoom_endpoint_does_not_replace_canonical_gameplay_fov");
    pass &= Check(std::fabs(zoomEndpoint.evidence.nativeFov.value - 26.1406f) < 0.001f &&
        std::fabs(zoomEndpoint.evidence.horPlusTargetFov.value - 49.813f) < 0.001f &&
        std::fabs(zoomEndpoint.evidence.currentOutputFov.value - 49.813f) < 0.001f,
        "zoom_endpoint_keeps_canonical_live_roles");

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

    auto freshnessChange = unavailable;
    freshnessChange.evidence.gameplayFov.valid = true;
    freshnessChange.evidence.gameplayFov.value = 90.0f;
    freshnessChange.evidence.gameplayFov.freshness = SnapshotFreshness::Current;
    auto retainedFreshness = freshnessChange;
    retainedFreshness.evidence.gameplayFov.freshness = SnapshotFreshness::Retained;
    pass &= Check(SnapshotSemanticsChanged(retainedFreshness, freshnessChange),
        "freshness_only_change_is_overlay_semantic_change");
    auto aspectMetadataChange = unavailable;
    aspectMetadataChange.evidence.aspectValid = true;
    aspectMetadataChange.evidence.aspect = 16.0f / 9.0f;
    pass &= Check(SnapshotSemanticsChanged(aspectMetadataChange, unavailable),
        "visible_aspect_change_is_overlay_semantic_change");
    pass &= Check(!SnapshotSemanticsChanged(aspectMetadataChange,
        aspectMetadataChange), "unchanged_visible_metadata_is_quiet");

    const auto diagnosticsOffState = BuildCameraStateSnapshot(input);
    const auto diagnosticsOnState = BuildCameraStateSnapshot(input);
    pass &= Check(!SnapshotChanged(diagnosticsOffState, diagnosticsOnState),
        "diagnostics_do_not_change_camera_state");

    CameraStateSnapshot published{};
    bool publishedValid = false;
    float candidateFov = std::numeric_limits<float>::quiet_NaN();
    std::uint32_t stableSamples = 0;
    std::uintptr_t candidateSource = 0;
    std::atomic<bool> zoomHold{false};
    std::atomic<std::uint64_t> eventSequence{0};
    CameraStatePublicationView publicationView{published, publishedValid,
        candidateFov, stableSamples, candidateSource, zoomHold, eventSequence};
    CameraStatePublicationInput publicationInput{};
    publicationInput.observationPublished = true;
    publicationInput.presentation = PresentationState::Gameplay;
    publicationInput.dialogue = DialogueState::Inactive;
    publicationInput.source = 0x1000;
    publicationInput.nativeAspect = 16.0f / 9.0f;
    publicationInput.observation.boundary = FovObservationBoundary::CameraWriter;
    publicationInput.observation.observedDuringGameplay = true;
    publicationInput.observation.writerSource = {0x1000, true};
    publicationInput.observation.inputFov = {90.6557f, FovSpace::Native,
        FovProvenance::NativeRegisterInput, true};
    publicationInput.observation.resultFov = {90.6557f, FovSpace::Native,
        FovProvenance::PassThroughResult, true};
    publicationInput.observation.aspect = {16.0f / 9.0f,
        FovProvenance::NativeWriterAspect, true};
    publicationInput.observation.publicationSequence = 1;
    GameplayBaselineStore productionBaselineStore{};
    const auto productionBaseline = productionBaselineStore.Project(
        publicationInput.observation, true);
    pass &= Check(productionBaseline.eligible &&
        IsUsableGameplayBaseline(productionBaseline.baseline),
        "production_baseline_accepts_valid_native_aspect_pass_through");
    CameraStatePublicationResult publication{};
    for (std::uint64_t sequence = 1; sequence <= 3; ++sequence) {
        publicationInput.observation.publicationSequence = sequence;
        publication = PublishCameraStateSnapshotLocked(publicationView,
            publicationInput);
    }
    pass &= Check(publication.current.evidence.gameplayFov.valid &&
        publication.current.evidence.gameplayFov.freshness == SnapshotFreshness::Current &&
        std::fabs(publication.current.evidence.gameplayFov.value - 90.6557f) < 0.001f,
        "stable_native_16x9_pass_through_confirms_shared_gameplay_baseline");
    publicationInput.observation.publicationSequence = 4;
    publication = PublishCameraStateSnapshotLocked(publicationView, publicationInput);
    pass &= Check(!SnapshotSemanticsChanged(publication.current, publication.previous),
        "sequence_only_camera_sample_does_not_force_overlay_redraw");

    publicationInput.applied = true;
    publicationInput.observation.inputFov = {90.6557f, FovSpace::Native,
        FovProvenance::NativeRegisterInput, true};
    publicationInput.observation.resultFov = {107.31f, FovSpace::Transformed,
        FovProvenance::ModTransformResult, true};
    publicationInput.observation.aspect.value = 3.0f;
    publicationInput.observation.resultFov.valid = true;
    publicationInput.observation.publicationSequence = 5;
    publication = PublishCameraStateSnapshotLocked(publicationView, publicationInput);
    pass &= Check(publication.current.evidence.horPlusTargetFov.valid &&
        publication.current.evidence.horPlusTargetFov.freshness == SnapshotFreshness::Current,
        "applied_writer_publishes_current_horplus_target");
    publicationInput.applied = false;
    publicationInput.observation.resultFov = {90.6557f, FovSpace::Native,
        FovProvenance::PassThroughResult, true};
    publicationInput.observation.aspect.value = 16.0f / 9.0f;
    publicationInput.observation.publicationSequence = 6;
    publication = PublishCameraStateSnapshotLocked(publicationView, publicationInput);
    pass &= Check(publication.current.evidence.horPlusTargetFov.valid &&
        publication.current.evidence.horPlusTargetFov.freshness == SnapshotFreshness::Historical,
        "same_source_pass_through_retires_prior_horplus_target_to_historical");

    publicationInput.zoomActive = true;
    publicationInput.observation.inputFov = {26.14f, FovSpace::Native,
        FovProvenance::NativeRegisterInput, true};
    publicationInput.observation.resultFov = {26.14f, FovSpace::Native,
        FovProvenance::PassThroughResult, true};
    publicationInput.observation.publicationSequence = 7;
    const auto zoomBaseline = productionBaselineStore.Project(
        publicationInput.observation, true);
    pass &= Check(zoomBaseline.eligible &&
        std::fabs(zoomBaseline.baseline.nativeFov.value - 26.14f) < 0.001f,
        "production_store_value_alone_is_not_a_zoom_neutral_gameplay_reference");
    publication = PublishCameraStateSnapshotLocked(publicationView, publicationInput);
    pass &= Check(publication.current.evidence.gameplayFov.valid &&
        std::fabs(publication.current.evidence.gameplayFov.value - 90.6557f) < 0.001f,
        "zoom_source_observation_cannot_replace_neutral_gameplay_baseline");

    input.generation.presentationEpoch = 3;
    const auto stale = BuildCameraStateSnapshot(input);
    pass &= Check(stale.generation.presentationEpoch != unavailable.generation.presentationEpoch,
        "generation_is_diagnostic_provenance");

    std::cout << "camera_state_snapshot=" << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
