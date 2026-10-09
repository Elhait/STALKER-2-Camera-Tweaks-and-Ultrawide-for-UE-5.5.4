#include "../../src/overlay/camera_integration.hpp"
#include "../../src/overlay/aspect_ratio_format.hpp"

#include <cmath>
#include <iostream>
#include <limits>

namespace
{
    bool Check(bool condition, const char* name)
    {
        if (!condition) std::cerr << "FAIL: " << name << "\n";
        return condition;
    }

    plugin::OverlaySemanticSnapshot BaseSnapshot()
    {
        plugin::OverlaySemanticSnapshot snapshot{};
        snapshot.gameplayEnabled = {true, true,
            plugin::SemanticProvenance::RuntimeSettings,
            plugin::SemanticFreshness::AtRead};
        snapshot.gameplayMode = {config::GameplayMode::HorPlus, true,
            plugin::SemanticProvenance::RuntimeSettings,
            plugin::SemanticFreshness::AtRead};
        snapshot.cinematicsEnabled = {true, true,
            plugin::SemanticProvenance::RuntimeSettings,
            plugin::SemanticFreshness::AtRead};
        snapshot.cinematicAspectPolicy = {config::CinematicAspectPolicy::Auto, true,
            plugin::SemanticProvenance::RuntimeSettings,
            plugin::SemanticFreshness::AtRead};
        snapshot.cinematicFovMode = {config::CinematicFovMode::GameplayHorPlus, true,
            plugin::SemanticProvenance::RuntimeSettings,
            plugin::SemanticFreshness::AtRead};
        snapshot.cinematicSelectionActive = {false, true,
            plugin::SemanticProvenance::CinematicSelection,
            plugin::SemanticFreshness::AtRead};
        snapshot.runtimeViewportAspect = {3.0f, true,
            plugin::SemanticProvenance::RuntimeViewportResolver,
            plugin::SemanticFreshness::AtRead};
        snapshot.coordinator = {plugin::OverlayCoordinatorState::Gameplay, true,
            plugin::SemanticProvenance::PresentationCoordinator,
            plugin::SemanticFreshness::AtRead};
        return snapshot;
    }

    camera::CameraFovFact FovFact(float value,
        camera::SnapshotFreshness freshness)
    {
        return {value, true, camera::EvidenceProvenance::ModDerivedState,
            freshness, 0, false, 1};
    }

    void SetFovRoles(plugin::OverlaySemanticSnapshot& snapshot,
        camera::SnapshotFreshness gameplayFreshness)
    {
        auto& evidence = snapshot.cameraState.evidence;
        evidence.gameplayFov = FovFact(90.6557f, gameplayFreshness);
        evidence.nativeFov = FovFact(90.6557f,
            camera::SnapshotFreshness::Current);
        evidence.horPlusTargetFov = FovFact(107.31f,
            camera::SnapshotFreshness::Current);
        evidence.currentOutputFov = FovFact(107.31f,
            camera::SnapshotFreshness::Current);
    }

    bool HasAllFovRoles(const overlay::CameraIntegrationStatus& status)
    {
        return status.gameplayFovValid && status.nativeFovValid &&
            status.horPlusTargetFovValid && status.currentOutputFovValid &&
            std::fabs(status.gameplayFov - 90.6557f) < 0.001f &&
            std::fabs(status.nativeFov - 90.6557f) < 0.001f &&
            std::fabs(status.horPlusTargetFov - 107.31f) < 0.001f &&
            std::fabs(status.currentOutputFov - 107.31f) < 0.001f;
    }
}

int main()
{
    bool pass = true;
    pass &= Check(overlay::FormatAspectRatioNine(1.7778f) == "16:9 (1.7778)" &&
        overlay::FormatAspectRatioNine(2.3889f) == "22:9 (2.3889)" &&
        overlay::FormatAspectRatioNine(2.7500f) == "25:9 (2.7500)" &&
        overlay::FormatAspectRatioNine(3.0000f) == "27:9 (3.0000)" &&
        overlay::FormatAspectRatioNine(3.5556f) == "32:9 (3.5556)" &&
        overlay::FormatAspectRatioNine(4.0000f) == "36:9 (4.0000)",
        "aspect_ratio_display_rounds_every_valid_ratio_to_integer_nine_based_numerator");
    pass &= Check(!overlay::FormatAspectRatioNine(0.0f) &&
        !overlay::FormatAspectRatioNine(-1.0f) &&
        !overlay::FormatAspectRatioNine(std::numeric_limits<float>::infinity()) &&
        !overlay::FormatAspectRatioNine(std::numeric_limits<float>::quiet_NaN()),
        "invalid_aspects_have_no_display_ratio");

    const overlay::CameraDomainState matchingGameplay{
        16.0f / 9.0f, 90.0f, true, true};
    const overlay::CameraDomainState matchingCinematic{
        16.0f / 9.0f, 90.8f, true, true};
    auto assessment = overlay::AssessCameraTransition(
        matchingGameplay, matchingCinematic);
    pass &= Check(assessment.aspect == overlay::CameraValueMatch::Matched &&
        assessment.fov == overlay::CameraValueMatch::Matched &&
        assessment.overall == overlay::OverallIntegrationAssessment::Seamless,
        "camera_state_values_within_semantic_fov_tolerance_are_seamless");

    assessment = overlay::AssessCameraTransition(matchingGameplay,
        {16.0f / 9.0f, 100.0f, true, true});
    pass &= Check(assessment.aspect == overlay::CameraValueMatch::Matched &&
        assessment.fov == overlay::CameraValueMatch::Mismatch &&
        assessment.overall == overlay::OverallIntegrationAssessment::Mismatch,
        "fov_state_mismatch_is_reported_independently_of_aspect");

    assessment = overlay::AssessCameraTransition(matchingGameplay,
        {32.0f / 9.0f, 90.0f, true, true});
    pass &= Check(assessment.aspect == overlay::CameraValueMatch::Mismatch &&
        assessment.fov == overlay::CameraValueMatch::Matched &&
        assessment.overall == overlay::OverallIntegrationAssessment::Mismatch,
        "aspect_state_mismatch_is_reported_independently_of_fov");

    assessment = overlay::AssessCameraTransition(matchingGameplay,
        {16.0f / 9.0f, std::numeric_limits<float>::quiet_NaN(), true, true});
    pass &= Check(assessment.aspect == overlay::CameraValueMatch::Matched &&
        assessment.fov == overlay::CameraValueMatch::CannotAssess &&
        assessment.overall == overlay::OverallIntegrationAssessment::CannotAssess,
        "unavailable_camera_state_fov_keeps_overall_assessment_unknown");

    auto snapshot = BaseSnapshot();
    snapshot.cameraStateValid = true;
    SetFovRoles(snapshot, camera::SnapshotFreshness::Current);
    snapshot.cameraState.evidence.aspect = 1.77778f;
    snapshot.cameraState.evidence.aspectValid = true;
    snapshot.cameraState.evidence.gameplayPolicyAspect = 3.0f;
    snapshot.cameraState.evidence.gameplayPolicyAspectValid = true;
    snapshot.cameraState.freshness.writerObservation =
        camera::SnapshotFreshness::Current;
    snapshot.cameraState.freshness.transformedResult =
        camera::SnapshotFreshness::Current;
    auto projected = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(projected.gameplayFovValid && projected.nativeFovValid &&
        projected.horPlusTargetFovValid && projected.currentOutputFovValid &&
        projected.nativeWriterAspectValid &&
        projected.horPlusPolicyAspectValid,
        "current_gameplay_camera_state_facts_remain_visible");
    pass &= Check(projected.aspect == overlay::CameraValueMatch::Waiting &&
        projected.fov == overlay::CameraValueMatch::Waiting &&
        projected.overall == overlay::OverallIntegrationAssessment::Waiting,
        "gameplay_waits_for_cinematic_observation_without_config_inference");
    pass &= Check(projected.cinematicCorrectionEnabled &&
        projected.cinematicAspectPreviewValid &&
        projected.cinematicAspectPreview == 3.0f,
        "auto_aspect_is_a_viewport_based_preview_not_a_production_resolution_claim");

    snapshot.coordinator.value = plugin::OverlayCoordinatorState::CinematicActive;
    snapshot.cameraState.evidence.gameplayFov.freshness =
        camera::SnapshotFreshness::Retained;
    snapshot.cameraState.dialogue.state = camera::DialogueState::Inactive;
    projected = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(HasAllFovRoles(projected) &&
        projected.gameplayFovFreshness == camera::SnapshotFreshness::Retained,
        "cinematic_active_projects_retained_gameplay_and_three_current_roles");

    snapshot.cameraState.dialogue.state = camera::DialogueState::Active;
    snapshot.cameraState.dialogue.provenance =
        camera::EvidenceProvenance::ConfirmedModLifecycle;
    snapshot.cameraState.evidence.nativeFov = FovFact(26.14f,
        camera::SnapshotFreshness::Current);
    snapshot.cameraState.evidence.currentOutputFov = FovFact(34.65f,
        camera::SnapshotFreshness::Current);
    projected = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(projected.gameplayFovValid && projected.nativeFovValid &&
        projected.horPlusTargetFovValid && projected.currentOutputFovValid &&
        std::fabs(projected.nativeFov - 26.14f) < 0.001f &&
        std::fabs(projected.currentOutputFov - 34.65f) < 0.001f &&
        projected.gameplayFovFreshness == camera::SnapshotFreshness::Retained,
        "dialogue_live_roles_remain_distinct_from_retained_enter_transition_evidence");

    snapshot.coordinator.value = plugin::OverlayCoordinatorState::CinematicExiting;
    snapshot.cameraState.dialogue.state = camera::DialogueState::Recovery;
    snapshot.cameraState.evidence.nativeFov = FovFact(107.31f,
        camera::SnapshotFreshness::Current);
    snapshot.cameraState.evidence.currentOutputFov = FovFact(107.31f,
        camera::SnapshotFreshness::Current);
    projected = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(projected.gameplayFovValid && projected.nativeFovValid &&
        projected.horPlusTargetFovValid && projected.currentOutputFovValid &&
        projected.gameplayFovFreshness == camera::SnapshotFreshness::Retained,
        "recovery_projects_valid_roles_without_coordinator_gate");

    snapshot.coordinator.value = plugin::OverlayCoordinatorState::Gameplay;
    snapshot.cameraState.dialogue.state = camera::DialogueState::Inactive;
    snapshot.cameraState.evidence.nativeFov = FovFact(90.6557f,
        camera::SnapshotFreshness::Current);
    snapshot.cameraState.evidence.currentOutputFov = FovFact(107.31f,
        camera::SnapshotFreshness::Current);
    snapshot.cameraState.evidence.gameplayFov.freshness =
        camera::SnapshotFreshness::Current;
    projected = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(HasAllFovRoles(projected) &&
        projected.gameplayFovFreshness == camera::SnapshotFreshness::Current,
        "gameplay_after_handoff_projects_refreshed_current_roles");

    snapshot.cameraState.evidence.horPlusTargetFov.valid = false;
    projected = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(projected.gameplayFovValid && projected.nativeFovValid &&
        !projected.horPlusTargetFovValid && projected.currentOutputFovValid,
        "only_shared_role_invalidity_projects_that_fov_as_unavailable");
    snapshot.cameraState.evidence.horPlusTargetFov = FovFact(107.31f,
        camera::SnapshotFreshness::Current);

    snapshot.gameplayEnabled.value = false;
    snapshot.gameplayMode.value = config::GameplayMode::AspectRecalculation;
    snapshot.cinematicAspectPolicy.value = config::CinematicAspectPolicy::Forced16x9;
    snapshot.cinematicFovMode.value = config::CinematicFovMode::NativeHorPlus;
    projected = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(projected.aspect == overlay::CameraValueMatch::Waiting &&
        projected.fov == overlay::CameraValueMatch::Waiting &&
        projected.overall == overlay::OverallIntegrationAssessment::Waiting,
        "selected_modes_and_enabled_flags_do_not_infer_camera_transition");

    snapshot.cinematicsEnabled.value = false;
    projected = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(!projected.cinematicCorrectionEnabled &&
        projected.cinematicEnabledKnown &&
        !projected.cinematicAspectPreviewValid &&
        projected.overall == overlay::OverallIntegrationAssessment::Waiting,
        "disabled_cinematics_display_default_without_inventing_native_camera_values");

    snapshot.cinematicsEnabled.value = true;
    snapshot.cameraState.freshness.writerObservation =
        camera::SnapshotFreshness::Historical;
    snapshot.cameraState.freshness.transformedResult =
        camera::SnapshotFreshness::Historical;
    projected = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(projected.nativeFovValid && !projected.nativeWriterAspectValid &&
        projected.horPlusTargetFovValid && projected.currentOutputFovValid &&
        !projected.horPlusPolicyAspectValid &&
        projected.aspect == overlay::CameraValueMatch::Waiting &&
        projected.fov == overlay::CameraValueMatch::Waiting &&
        projected.overall == overlay::OverallIntegrationAssessment::Waiting,
        "historical_camera_state_is_not_used_while_waiting_for_next_boundary");
    pass &= Check(snapshot.cameraState.evidence.aspectValid &&
        snapshot.cameraState.freshness.writerObservation ==
            camera::SnapshotFreshness::Historical,
        "expanded_details_keep_last_valid_writer_aspect_with_explicit_historical_freshness");

    camera::CameraTransitionBoundaryObservation boundary{};
    boundary.accepted = true;
    boundary.lifecycleGeneration = 7;
    boundary.gameplayValid = true;
    boundary.cinematicValid = true;
    boundary.gameplay.boundary = camera::FovObservationBoundary::CameraWriter;
    boundary.gameplay.publicationSequence = 10;
    boundary.gameplay.observedDuringGameplay = true;
    boundary.gameplay.resultFov = {119.27f, camera::FovSpace::Transformed,
        camera::FovProvenance::ModTransformResult, true};
    boundary.gameplay.aspect = {1.77778f,
        camera::FovProvenance::ResolvedAspect, true};
    boundary.gameplay.policyAspect = {3.0f,
        camera::FovProvenance::ClientViewportAspect, true};
    boundary.cinematic.boundary = camera::FovObservationBoundary::CinematicEnter;
    boundary.cinematic.publicationSequence = 11;
    boundary.cinematic.resultFov = {119.27f, camera::FovSpace::Transformed,
        camera::FovProvenance::ModTransformResult, true};
    boundary.cinematic.aspect = {3.0f,
        camera::FovProvenance::ResolvedAspect, true};
    snapshot.cameraTransitionBoundary = {boundary, true,
        plugin::SemanticProvenance::CameraStateSnapshot,
        plugin::SemanticFreshness::LifecycleScoped};
    snapshot.coordinator.value = plugin::OverlayCoordinatorState::CinematicActive;
    projected = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(projected.transitionBoundaryObserved &&
        projected.aspect == overlay::CameraValueMatch::Matched &&
        projected.fov == overlay::CameraValueMatch::Matched &&
        projected.overall == overlay::OverallIntegrationAssessment::Seamless &&
        projected.gameplayBoundaryFovValid &&
        std::fabs(projected.gameplayBoundaryFov - 119.27f) < 0.001f &&
        projected.cinematicEnterFovValid &&
        std::fabs(projected.cinematicEnterFov - 119.27f) < 0.001f &&
        projected.transitionGeneration == 7 &&
        projected.gameplayBoundaryAspectValid &&
        projected.cinematicEnterAspectValid && projected.aspectDeltaValid &&
        std::fabs(projected.aspectDelta - 0.0f) < 0.001f &&
        projected.fovDeltaValid && std::fabs(projected.fovDelta) < 0.001f,
        "accepted_cinematic_enter_compares_actual_gameplay_and_cinematic_boundary_samples");

    snapshot.gameplayEnabled.value = true;
    snapshot.gameplayMode.value = config::GameplayMode::HorPlus;
    snapshot.cinematicAspectPolicy.value = config::CinematicAspectPolicy::Auto;
    snapshot.cinematicFovMode.value = config::CinematicFovMode::GameplayHorPlus;
    snapshot.cameraState.evidence.aspect = 3.0f;
    snapshot.cameraState.evidence.aspectValid = true;
    snapshot.cameraState.evidence.gameplayPolicyAspect = 3.0f;
    snapshot.cameraState.evidence.gameplayPolicyAspectValid = true;
    snapshot.cameraState.freshness.writerObservation =
        camera::SnapshotFreshness::Current;
    snapshot.cameraState.evidence.gameplayFov = FovFact(90.6557f,
        camera::SnapshotFreshness::Current);
    projected = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(projected.potential.aspect == overlay::CameraValueMatch::Matched &&
        projected.potential.fov == overlay::CameraValueMatch::Matched &&
        projected.potential.overall ==
            overlay::OverallIntegrationAssessment::Seamless &&
        projected.overall == overlay::OverallIntegrationAssessment::Seamless,
        "potential_assessment_uses_current_settings_and_live_gameplay_baseline");

    snapshot.cinematicAspectPolicy.value =
        config::CinematicAspectPolicy::Forced16x9;
    projected = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(projected.potential.aspect == overlay::CameraValueMatch::Mismatch &&
        projected.potential.overall == overlay::OverallIntegrationAssessment::Mismatch &&
        projected.overall == overlay::OverallIntegrationAssessment::Seamless &&
        projected.transitionBoundaryObserved && projected.transitionGeneration == 7,
        "changing_selected_aspect_recalculates_potential_without_rewriting_actual");

    snapshot.cinematicAspectPolicy.value = config::CinematicAspectPolicy::Auto;
    snapshot.cinematicFovMode.value = config::CinematicFovMode::NativeHorPlus;
    projected = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(projected.potential.aspect == overlay::CameraValueMatch::Matched &&
        projected.potential.fov == overlay::CameraValueMatch::Unknown &&
        projected.potential.overall == overlay::OverallIntegrationAssessment::Unknown &&
        projected.overall == overlay::OverallIntegrationAssessment::Seamless,
        "native_horplus_potential_fov_is_unknown_until_authored_enter_observation");

    snapshot.cameraState.dialogue.state = camera::DialogueState::Active;
    snapshot.cameraState.evidence.nativeFov = FovFact(26.14f,
        camera::SnapshotFreshness::Current);
    snapshot.cameraState.evidence.currentOutputFov = FovFact(34.65f,
        camera::SnapshotFreshness::Current);
    projected = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(std::fabs(projected.nativeFov - 26.14f) < 0.001f &&
        std::fabs(projected.currentOutputFov - 34.65f) < 0.001f &&
        projected.cinematicEnterFovValid &&
        std::fabs(projected.cinematicEnterFov - 119.27f) < 0.001f,
        "dialogue_live_roles_remain_distinct_from_retained_enter_transition_evidence");

    snapshot.coordinator.value = plugin::OverlayCoordinatorState::CinematicExiting;
    snapshot.cameraState.dialogue.state = camera::DialogueState::Recovery;
    projected = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(projected.transitionBoundaryObserved &&
        projected.cinematicEnterFovValid &&
        projected.overall == overlay::OverallIntegrationAssessment::Seamless,
        "recovery_keeps_enter_evidence_separate_from_live_camera_roles");

    snapshot.coordinator.value = plugin::OverlayCoordinatorState::Gameplay;
    snapshot.cameraState.dialogue.state = camera::DialogueState::Inactive;
    projected = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(projected.transitionBoundaryObserved &&
        projected.cinematicEnterFovValid &&
        projected.overall == overlay::OverallIntegrationAssessment::Seamless,
        "post_exit_projects_last_validated_transition_not_live_cinematic_fov");

    boundary.cinematic.resultFov.value = 125.0f;
    snapshot.cameraTransitionBoundary.value = boundary;
    projected = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(projected.aspect == overlay::CameraValueMatch::Matched &&
        projected.fov == overlay::CameraValueMatch::Mismatch &&
        projected.overall == overlay::OverallIntegrationAssessment::Mismatch,
        "active_cinematic_reports_observed_fov_mismatch");

    boundary.cinematic.resultFov.value = 119.27f;
    boundary.cinematic.aspect.value = 2.3889f;
    snapshot.cameraTransitionBoundary.value = boundary;
    projected = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(projected.aspect == overlay::CameraValueMatch::Mismatch &&
        projected.fov == overlay::CameraValueMatch::Matched &&
        projected.overall == overlay::OverallIntegrationAssessment::Mismatch,
        "active_cinematic_reports_observed_aspect_mismatch");

    snapshot.coordinator.value = plugin::OverlayCoordinatorState::Gameplay;
    projected = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(projected.transitionBoundaryObserved &&
        projected.overall == overlay::OverallIntegrationAssessment::Mismatch &&
        projected.gameplayBoundaryFovValid && projected.cinematicEnterFovValid &&
        projected.transitionGeneration == 7,
        "exit_retains_last_validated_transition_as_evidence_not_live_fov");

    // Both the summary projection and expanded Camera State consume this same
    // projection contract, so retained roles cannot disagree across views.
    pass &= Check(projected.gameplayFovValid && projected.nativeFovValid &&
        projected.horPlusTargetFovValid && projected.currentOutputFovValid &&
        projected.gameplayFovFreshness == snapshot.cameraState.evidence.gameplayFov.freshness,
        "summary_and_expanded_camera_state_share_role_projection_for_one_snapshot");

    snapshot.coordinator.value = plugin::OverlayCoordinatorState::CinematicActive;
    boundary.gameplayValid = false;
    snapshot.cameraTransitionBoundary.value = boundary;
    projected = overlay::ProjectCameraIntegration(snapshot);
    pass &= Check(!projected.transitionBoundaryObserved &&
        projected.overall == overlay::OverallIntegrationAssessment::CannotAssess,
        "active_cinematic_without_a_gameplay_boundary_sample_remains_unassessable");

    std::cout << "Overlay camera integration harness: " << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
