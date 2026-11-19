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

    bool CheckAdmittedReferenceAndOriginalInputContract()
    {
        using namespace camera;
        CameraStateSnapshot snapshot{};
        bool valid{};
        float neutralCandidate{NAN};
        std::uint32_t neutralSamples{};
        std::uintptr_t neutralSource{};
        std::atomic<bool> hold{};
        std::atomic<std::uint64_t> sequence{};
        CameraStatePublicationView view{snapshot, valid, neutralCandidate,
            neutralSamples, neutralSource, hold, sequence};
        CameraStatePublicationInput input{};
        input.observationPublished = true;
        input.presentation = PresentationState::Gameplay;
        input.dialogue = DialogueState::Inactive;
        input.source = 0xA;
        input.applied = true;
        input.nativeAspect = 16.0f / 9.0f;
        auto& observation = input.observation;
        observation.boundary = FovObservationBoundary::CameraWriter;
        observation.observedDuringGameplay = true;
        observation.writerContext = WriterContext::GameplayEligible;
        observation.writerSource = {0xA, true};
        observation.inputFov = {130.3115f, FovSpace::Native,
            FovProvenance::NativeRegisterInput, true};
        observation.resultFov = {153.931f, FovSpace::Transformed,
            FovProvenance::ModTransformResult, true};
        observation.aspect = {32.0f / 9.0f,
            FovProvenance::NativeWriterAspect, true};
        observation.writerFlags = 0x4;
        observation.writerFlagsValid = true;
        observation.publicationSequence = 1;
        GameplayBaselineStore baselineStore;
        const auto baseline = baselineStore.Project(observation, true);
        bool pass = Check(IsUsableGameplayBaseline(baseline.baseline),
            "reference_contract_production_baseline_admitted_without_zoom");
        input.gameplayReference = baseline.baseline;
        input.gameplayReferenceProvided = true;
        input.originalInputAvailable = true;
        for (unsigned sample = 0; sample < 4; ++sample)
            PublishCameraStateSnapshotLocked(view, input);
        pass &= Check(snapshot.evidence.gameplayFov.valid &&
            std::fabs(snapshot.evidence.gameplayFov.value - 130.3115f) < 0.001f &&
            snapshot.evidence.gameplayFov.source == 0xA &&
            snapshot.evidence.gameplayFov.freshness == SnapshotFreshness::Current,
            "admitted_gameplay_reference_must_be_visible_without_any_zoom_event");

        input.dialogue = DialogueState::Active;
        observation.observedDuringGameplay = false;
        observation.writerContext = WriterContext::ConfirmedProviderDialogueLifecycleObserved;
        observation.inputFov.value = 122.087f;
        observation.resultFov.value = 149.071f;
        observation.publicationSequence = 2;
        const auto retained = baselineStore.Project(observation, false);
        pass &= Check(IsUsableGameplayBaseline(retained.baseline) &&
            std::fabs(retained.baseline.nativeFov.value - 130.3115f) < 0.001f,
            "reference_contract_confirmed_dialogue_retains_production_reference");
        input.gameplayReference = retained.baseline;
        input.originalInputAvailable = false;
        PublishCameraStateSnapshotLocked(view, input);
        pass &= Check(IsUsableGameplayBaseline(retained.baseline) &&
            snapshot.evidence.gameplayFov.valid &&
            std::fabs(snapshot.evidence.gameplayFov.value - 130.3115f) < 0.001f &&
            snapshot.evidence.gameplayFov.freshness == SnapshotFreshness::Retained,
            "confirmed_dialogue_keeps_admitted_reference_visible_as_retained_not_live");

        // Drive receiver invalidation through the real store and the actual publisher inputs.
        input.source = 0xB;
        observation.writerSource = {0xB, true};
        observation.writerContext = WriterContext::TransitionAmbiguous;
        input.gameplayReference = baselineStore.Project(observation, false).baseline;
        PublishCameraStateSnapshotLocked(view, input);
        pass &= Check(!snapshot.evidence.gameplayReferenceUsable &&
            snapshot.evidence.gameplayFov.freshness == SnapshotFreshness::Historical,
            "publisher_cannot_show_foreign_receiver_baseline_as_current_or_usable");
        input.source = 0xA;
        observation.writerSource = {0xA, true};
        observation.writerContext = WriterContext::ConfirmedProviderDialogueLifecycleObserved;
        input.gameplayReference = baselineStore.Project(observation, false).baseline;
        PublishCameraStateSnapshotLocked(view, input);
        pass &= Check(!snapshot.evidence.gameplayReferenceUsable &&
            snapshot.evidence.gameplayFov.freshness == SnapshotFreshness::Historical,
            "confirmed_dialogue_cannot_resurrect_invalidated_reference");
        input.gameplayReference = baselineStore.Invalidate().baseline;
        PublishCameraStateSnapshotLocked(view, input);
        pass &= Check(!snapshot.evidence.gameplayReferenceUsable,
            "production_invalidation_reaches_ui_publication");

        // Known upstream-corrected input cannot establish an original FOV.
        CameraFovTransactionPublication prepared{};
        prepared.nativeInput = 122.087f;
        prepared.nativeInputValid = true;
        prepared.nativeProvenance = EvidenceProvenance::ModDerivedState;
        prepared.output = 149.071f;
        prepared.outputValid = true;
        prepared.outputProvenance = EvidenceProvenance::ModDerivedState;
        prepared.source = 0xA;
        prepared.sequence = 3;
        prepared.transactionOwner = PresentationState::Gameplay;
        CameraStateSnapshot originalUnknown{};
        bool originalUnknownValid{};
        PublishCameraFovTransactionLocked(originalUnknown, originalUnknownValid, prepared);
        pass &= Check(!originalUnknown.evidence.nativeFov.valid,
            "upstream_corrected_writer_input_must_not_create_original_native_fov");

        auto captured = prepared;
        captured.nativeInput = 70.0f;
        captured.nativeProvenance = EvidenceProvenance::NativeNumericObservation;
        captured.sequence = 4;
        PublishCameraFovTransactionLocked(originalUnknown, originalUnknownValid, captured);
        pass &= Check(originalUnknown.evidence.nativeFov.valid &&
            originalUnknown.evidence.nativeFov.value == 70.0f &&
            originalUnknown.evidence.nativeFov.source == 0xA,
            "reference_contract_direct_original_capture_is_published");
        prepared.nativeInput = 153.931f;
        prepared.output = 153.931f;
        prepared.sequence = 5;
        prepared.transactionOwner = PresentationState::CinematicActive;
        PublishCameraFovTransactionLocked(originalUnknown, originalUnknownValid, prepared);
        pass &= Check(!originalUnknown.evidence.nativeFov.valid ||
            (std::fabs(originalUnknown.evidence.nativeFov.value - 70.0f) < 0.001f &&
                originalUnknown.evidence.nativeFov.freshness != SnapshotFreshness::Current),
            "prepared_cinematic_input_cannot_replace_original_capture_as_current_native");
        return pass;
    }
    bool CheckWriterReadbackContract()
    {
        using namespace camera;
        bool pass = true, valid = false;
        CameraStateSnapshot snapshot{};
        CameraFovTransactionPublication plan{};
        plan.nativeInput = 122.087f; plan.nativeInputValid = true;
        plan.nativeProvenance = EvidenceProvenance::ModDerivedState;
        plan.writerInputPublication = true;
        plan.output = 149.071f; plan.outputValid = true;
        plan.source = 0xA; plan.writerTransaction = {11, 0xA, 0xB, 7, 42};
        PublishCameraFovTransactionLocked(snapshot, valid, plan);
        pass &= Check(!EffectiveCameraFov(snapshot.evidence).valid,
            "planned_is_not_effective_without_readback");
        CameraFovReadbackPublication read{plan.writerTransaction,
            CameraFovBoundary::PrimaryPostStore, 148.0f, true, true};
        pass &= Check(PublishCameraFovReadbackLocked(snapshot, valid, read) &&
            EffectiveCameraFov(snapshot.evidence).value == 148.0f,
            "actual_mismatch_is_observed_not_replaced_with_plan");
        read.boundary = CameraFovBoundary::PostHelper; read.value = 147.0f;
        PublishCameraFovReadbackLocked(snapshot, valid, read);
        read.boundary = CameraFovBoundary::PrimaryPostStore; read.value = 148.0f;
        PublishCameraFovReadbackLocked(snapshot, valid, read);
        pass &= Check(snapshot.evidence.postStoreFov.value == 148.0f &&
            snapshot.evidence.postHelperFov.value == 147.0f &&
            EffectiveCameraFov(snapshot.evidence).boundary == CameraFovBoundary::PostHelper,
            "both_readback_boundaries_remain_distinct_in_either_delivery_order");
        ++plan.writerTransaction.id;
        PublishCameraFovTransactionLocked(snapshot, valid, plan);
        pass &= Check(!PublishCameraFovReadbackLocked(snapshot, valid, read) &&
            !EffectiveCameraFov(snapshot.evidence).valid, "readback_N_cannot_verify_N_plus_1");
        for (unsigned mismatch = 0; mismatch < 5; ++mismatch) {
            read.identity = plan.writerTransaction;
            switch (mismatch) {
            case 0: ++read.identity.id; break;
            case 1: ++read.identity.receiver; break;
            case 2: ++read.identity.output; break;
            case 3: ++read.identity.thread; break;
            case 4: ++read.identity.lifecycle; break;
            }
            pass &= Check(!PublishCameraFovReadbackLocked(snapshot, valid, read),
                "foreign_transaction_identity_rejected");
        }
        read.identity = plan.writerTransaction; read.readable = false;
        PublishCameraFovReadbackLocked(snapshot, valid, read);
        pass &= Check(!EffectiveCameraFov(snapshot.evidence).valid, "unreadable_is_unavailable");
        read.readable = true; read.correlated = false;
        pass &= Check(!PublishCameraFovReadbackLocked(snapshot, valid, read),
            "uncertain_correlation_never_verifies");
        read.correlated = true;
        read.boundary = CameraFovBoundary::Unavailable;
        pass &= Check(!PublishCameraFovReadbackLocked(snapshot, valid, read),
            "unnamed_boundary_cannot_verify_output");
        read.boundary = CameraFovBoundary::PrimaryPostStore;
        PublishCameraFovReadbackLocked(snapshot, valid, read);
        MarkCameraPresentationTransition(snapshot, valid);
        pass &= Check(!EffectiveCameraFov(snapshot.evidence).valid &&
            !PublishCameraFovReadbackLocked(snapshot, valid, read),
            "lifecycle_boundary_retires_the_prior_readback_identity");
        plan.writerTransaction = {};
        PublishCameraFovTransactionLocked(snapshot, valid, plan);
        pass &= Check(!EffectiveCameraFov(snapshot.evidence).valid &&
            snapshot.evidence.currentOutputFov.value == 149.071f,
            "diagnostics_off_keeps_plan_without_invented_effective");
        return pass;
    }

}

int main()
{
    using namespace camera;
    bool pass = CheckAdmittedReferenceAndOriginalInputContract();
    pass &= CheckWriterReadbackContract();

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
    for (int sample = 0; sample < 4; ++sample)
        publication = PublishCameraStateSnapshotLocked(publicationView, publicationInput);
    pass &= Check(!publication.current.evidence.neutralGameplayFov.valid,
        "unavailable_zoom_cannot_confirm_neutral_fov");
    publicationInput.zoomObserverAvailable = true;
    for (int sample = 0; sample < 4; ++sample)
        publication = PublishCameraStateSnapshotLocked(publicationView, publicationInput);
    pass &= Check(!publication.current.evidence.neutralGameplayFov.valid,
        "installed_but_unobserved_zoom_cannot_confirm_neutral_fov");
    publicationInput.zoomObservationValid = true;
    zoomHold = true;

    for (std::uint64_t sequence = 1; sequence <= 3; ++sequence) {
        publicationInput.observation.publicationSequence = sequence;
        publication = PublishCameraStateSnapshotLocked(publicationView,
            publicationInput);
    }
    pass &= Check(publication.current.evidence.neutralGameplayFov.valid &&
        publication.current.evidence.neutralGameplayFov.freshness == SnapshotFreshness::Current &&
        std::fabs(publication.current.evidence.neutralGameplayFov.value - 90.6557f) < 0.001f,
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
    pass &= Check(publication.current.evidence.neutralGameplayFov.valid &&
        std::fabs(publication.current.evidence.neutralGameplayFov.value - 90.6557f) < 0.001f,
        "zoom_source_observation_cannot_replace_neutral_gameplay_baseline");

    publicationInput.zoomActive = false;
    publicationInput.zoomObserverAvailable = false;
    for (int sample = 0; sample < 4; ++sample)
        publication = PublishCameraStateSnapshotLocked(publicationView, publicationInput);
    pass &= Check(std::fabs(publication.current.evidence.neutralGameplayFov.value - 90.6557f) < 0.001f &&
        publication.current.evidence.neutralGameplayFov.freshness == SnapshotFreshness::Historical,
        "capability_loss_preserves_old_reference_as_historical");

    input.generation.presentationEpoch = 3;
    const auto stale = BuildCameraStateSnapshot(input);
    pass &= Check(stale.generation.presentationEpoch != unavailable.generation.presentationEpoch,
        "generation_is_diagnostic_provenance");

    std::cout << "camera_state_snapshot=" << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
