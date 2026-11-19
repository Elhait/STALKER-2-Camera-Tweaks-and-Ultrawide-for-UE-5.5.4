#include "../../src/cinematics/cinematic_fov.hpp"
#include "../../src/cinematics/cinematic_enter_activation.hpp"
#include "../../src/diagnostics/cinematic_continuation_trace.hpp"
#include "../../src/diagnostics/cinematic_baseline_provenance.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <memory>

namespace
{
    bool Check(bool condition, const char* name)
    {
        if (!condition) std::cerr << name << ": FAIL\n";
        return condition;
    }

    bool Near(float actual, float expected, float tolerance = 0.02f)
    {
        return std::isfinite(actual) && std::fabs(actual - expected) <= tolerance;
    }

    camera::GameplayBaseline Baseline(float native, float aspect, bool transformed)
    {
        camera::GameplayBaseline baseline{};
        baseline.valid = true;
        baseline.nativeFov = { native, camera::FovSpace::Native,
            camera::FovProvenance::NativeRegisterInput, true };
        baseline.aspect = aspect;
        baseline.source = { 0x1000, true };
        baseline.observationSequence = 7;
        if (transformed) {
            baseline.horPlusFov = { 143.132f, camera::FovSpace::Transformed,
                camera::FovProvenance::ModTransformResult, true };
        }
        return baseline;
    }
}

int main()
{
    bool pass = true;

    cinematics::CinematicEnterActivation enterActivation;
    enterActivation.Arm(1, 7, 0xA, 90.3115f, 127.119f, true, 126.87f);
    pass &= Check(enterActivation.Capture(true, 1, 7, 0xC, 0xA, 42,
        90.3115f, 126.87f), "enter_activation_capture_exact_caller_generation");
    const auto mappedMiddle = enterActivation.ObserveWriter(true, 1, 7,
        0xA, 0xB, (90.3115f + 126.87f) * 0.5f);
    pass &= Check(mappedMiddle.status == cinematics::EnterActivationStatus::MatchedSample &&
        mappedMiddle.writerSource == 0xA && mappedMiddle.writerOutput == 0xB &&
        std::fabs(mappedMiddle.sampleWeight - 0.5f) < 0.0001f &&
        std::fabs(mappedMiddle.mappedOutput - (127.119f + 126.87f) * 0.5f) < 0.0001f,
        "enter_activation_maps_only_admitted_writer_sample");
    const auto movedOutput = enterActivation.ObserveWriter(true, 1, 7,
        0xA, 0xD, 100.0f);
    pass &= Check(movedOutput.status == cinematics::EnterActivationStatus::MatchedSample &&
        movedOutput.writerOutput == 0xD,
        "enter_activation_uses_per_invocation_output_admission");

    enterActivation.Arm(2, 8, 0xA, 90.3115f, 127.119f, true, 126.87f);
    pass &= Check(enterActivation.Capture(true, 2, 8, 0xC, 0xA, 43,
        70.0f, 126.87f), "enter_activation_captures_c2_native_start");
    const auto missingStart = enterActivation.ObserveWriter(true, 2, 8,
        0xA, 0xB, 98.435f);
    pass &= Check(missingStart.status == cinematics::EnterActivationStatus::MissingCorrectedStart &&
        !missingStart.correctedStartValid && !std::isfinite(missingStart.mappedOutput),
        "enter_activation_fails_closed_for_historical_baseline");
    const auto c2Endpoint = enterActivation.ObserveWriter(true, 2, 8,
        0xA, 0xB, 126.87f);
    pass &= Check(c2Endpoint.status == cinematics::EnterActivationStatus::MissingCorrectedStart &&
        !c2Endpoint.armed &&
        !enterActivation.ObserveWriter(true, 2, 8, 0xA, 0xB, 126.87f).captured,
        "enter_activation_retires_unmapped_c2_at_native_endpoint");

    camera::CameraFovObservation dialogueWriterObservation{};
    dialogueWriterObservation.boundary = camera::FovObservationBoundary::CameraWriter;
    dialogueWriterObservation.writerContext = camera::WriterContext::DialogueLifecycleObserved;
    dialogueWriterObservation.writerSource = {0xA, true};
    dialogueWriterObservation.writerThreadId = 47;
    dialogueWriterObservation.publicationSequence = 77;
    dialogueWriterObservation.aspect = {3.55556f,
        camera::FovProvenance::ResolvedAspect, true};
    dialogueWriterObservation.policyAspect = {3.55556f,
        camera::FovProvenance::ClientViewportAspect, true};
    dialogueWriterObservation.writerFlags = 0x4;
    dialogueWriterObservation.writerFlagsValid = true;
    dialogueWriterObservation.inputFov = {80.1509f, camera::FovSpace::Native,
        camera::FovProvenance::NativeRegisterInput, true};
    dialogueWriterObservation.resultFov = {118.563f, camera::FovSpace::Transformed,
        camera::FovProvenance::ModTransformResult, true};
    const auto dialogueStart = cinematics::MakeDialogueWriterResultCandidate(
        dialogueWriterObservation, 0xA, 22, 1, true);
    auto confirmedObservation = dialogueWriterObservation;
    confirmedObservation.writerContext =
        camera::WriterContext::ConfirmedProviderDialogueLifecycleObserved;
    confirmedObservation.inputFov.value = 122.087f;
    confirmedObservation.resultFov.value = 149.071f;
    const auto confirmedStart = cinematics::MakeDialogueWriterResultCandidate(
        confirmedObservation, 0xA, 22, 1, true);
    cinematics::CinematicEnterActivation confirmedActivation;
    confirmedActivation.Arm(2, 13, 0xA, 130.311f, 153.931f, true,
        153.931f, confirmedStart);
    const bool confirmedCaptured = confirmedActivation.Capture(true, 2, 13,
        0xC, 0xA, 47, 122.087f, 153.931f, 22);
    bool confirmedTrajectory = confirmedCaptured && confirmedStart.valid;
    for (const float weight : {0.0f, 0.03f, 0.17f, 0.49f, 0.83f, 1.0f}) {
        const float input = 122.087f + weight * (153.931f - 122.087f);
        const auto mapped = confirmedActivation.ObserveWriter(true, 2, 13,
            0xA, 0xB, input);
        const float expected = 149.071f + weight * (153.931f - 149.071f);
        confirmedTrajectory &= mapped.correctedStartValid &&
            mapped.correctedStartFromDialogue &&
            std::fabs(mapped.mappedOutput - expected) < 0.02f;
    }
    pass &= Check(confirmedTrajectory,
        "confirmed_provider_dialogue_maps_entire_cinematic_trajectory_without_raw_start_drop");
    pass &= Check(dialogueStart.valid && dialogueStart.dialogueContextActive &&
        dialogueStart.activationStartSample == 80.1509f &&
        dialogueStart.correctedStart == 118.563f &&
        dialogueStart.writerSequence == 77 && !dialogueWriterObservation.observedDuringGameplay,
        "dialogue_candidate_uses_writer_input_and_qualified_writer_result");
    enterActivation.Arm(6, 13, 0xB, 90.3115f, 127.119f, true,
        126.87f, dialogueStart);
    pass &= Check(enterActivation.Capture(true, 6, 13, 0xC, 0xA, 47,
            80.1509f, 126.87f, 22),
        "enter_activation_accepts_current_dialogue_start_for_bound_receiver");
    const auto dialogueMappedStart = enterActivation.ObserveWriter(true, 6, 13,
        0xA, 0xB, 80.1509f);
    pass &= Check(dialogueMappedStart.correctedStartValid &&
        dialogueMappedStart.correctedStartFromDialogue &&
        dialogueMappedStart.correctedStartWriterSequence == 77 &&
        std::fabs(dialogueMappedStart.correctedStart - 118.563f) < 0.001f &&
        std::fabs(dialogueMappedStart.mappedOutput - 118.563f) < 0.001f,
        "enter_activation_maps_from_dialogue_writer_result_not_authored_enter_or_gameplay_history");

    auto staleDialogueCandidate = dialogueStart;
    staleDialogueCandidate.writerThreadId = 48;
    enterActivation.Arm(7, 14, 0xA, 90.3115f, 127.119f, true,
        126.87f, staleDialogueCandidate);
    enterActivation.Capture(true, 7, 14, 0xC, 0xA, 48,
        80.1509f, 126.87f, 23);
    const auto staleDialogueObservation = enterActivation.ObserveWriter(true, 7, 14,
        0xA, 0xB, 80.1509f);
    pass &= Check(!staleDialogueObservation.correctedStartValid &&
        staleDialogueObservation.status == cinematics::EnterActivationStatus::MissingCorrectedStart,
        "enter_activation_does_not_fall_back_to_gameplay_history_when_dialogue_provenance_stales");

    cinematics::EnterActivationCorrectedStartCandidate currentSampleStart{
        0xA, 24, 90.0f, 127.0f, true, false};
    enterActivation.Arm(8, 15, 0xB, 90.0f, 127.0f, true,
        126.87f);
    enterActivation.Capture(true, 8, 15, 0xC, 0xA, 49, 90.0f, 126.87f,
        24, currentSampleStart);
    const auto currentSampleMappedStart = enterActivation.ObserveWriter(true,
        8, 15, 0xA, 0xD, 90.0f);
    pass &= Check(currentSampleMappedStart.correctedStartValid &&
        currentSampleMappedStart.correctedStartFromCurrentSample &&
        std::fabs(currentSampleMappedStart.mappedOutput - 127.0f) < 0.001f,
        "enter_activation_maps_from_receiver_bound_current_native_sample");

    currentSampleStart.dialogueContextActive = true;
    enterActivation.Arm(9, 16, 0xA, 90.0f, 127.0f, true,
        126.87f);
    enterActivation.Capture(true, 9, 16, 0xC, 0xA, 50, 90.0f, 126.87f,
        24, currentSampleStart);
    const auto blockedGameplayFallback = enterActivation.ObserveWriter(true,
        9, 16, 0xA, 0xD, 90.0f);
    pass &= Check(!blockedGameplayFallback.correctedStartValid &&
        blockedGameplayFallback.status == cinematics::EnterActivationStatus::MissingCorrectedStart,
        "enter_activation_blocks_gameplay_baseline_when_dialogue_start_is_unavailable");

    auto wrongContextDialogueObservation = dialogueWriterObservation;
    wrongContextDialogueObservation.writerContext = camera::WriterContext::GameplayEligible;
    const auto wrongContextDialogueStart = cinematics::MakeDialogueWriterResultCandidate(
        wrongContextDialogueObservation, 0xA, 22, 1, true);
    pass &= Check(!wrongContextDialogueStart.valid &&
        wrongContextDialogueStart.dialogueContextActive,
        "dialogue_candidate_rejects_gameplay_context_observation");

    enterActivation.Arm(3, 9, 0xA, 90.0f, 127.0f, true, 126.0f);
    pass &= Check(!enterActivation.Capture(false, 3, 9, 0xC, 0xA, 44,
        90.0f, 126.0f) &&
        !enterActivation.Capture(true, 2, 9, 0xC, 0xA, 44, 90.0f, 126.0f),
        "enter_activation_rejects_wrong_caller_or_generation");
    pass &= Check(enterActivation.Capture(true, 3, 9, 0xC, 0xA, 44,
        90.0f, 126.0f), "enter_activation_captures_current_generation");
    const auto foreignSource = enterActivation.ObserveWriter(true, 3, 9,
        0xE, 0xB, 100.0f);
    pass &= Check(foreignSource.status == cinematics::EnterActivationStatus::ForeignSource,
        "enter_activation_retires_on_foreign_source");

    enterActivation.Arm(4, 10, 0xA, 90.0f, 127.0f, true, 126.0f);
    enterActivation.Capture(true, 4, 10, 0xC, 0xA, 45, 90.0f, 126.0f);
    const auto changedSettings = enterActivation.ObserveWriter(true, 4, 11,
        0xA, 0xB, 100.0f);
    pass &= Check(changedSettings.status == cinematics::EnterActivationStatus::SettingsChanged,
        "enter_activation_retires_on_settings_epoch_change");

    enterActivation.Arm(5, 12, 0xA, 90.0f, 127.0f, true, 126.0f);
    enterActivation.Capture(true, 5, 12, 0xC, 0xA, 46, 90.0f, 126.0f);
    const auto endpoint = enterActivation.ObserveWriter(true, 5, 12,
        0xA, 0xB, 126.0f);
    pass &= Check(endpoint.status == cinematics::EnterActivationStatus::CompletedEndpoint &&
        !endpoint.armed && std::fabs(endpoint.mappedOutput - 126.0f) < 0.0001f &&
        !enterActivation.ObserveWriter(true, 5, 12, 0xA, 0xB, 126.0f).captured,
        "enter_activation_maps_and_retires_at_native_endpoint");
    const float authored = 90.0f;
    const auto transformedBaseline = Baseline(112.623f, 3.55556f, true);
    const auto passThroughBaseline = Baseline(104.0f, 2.4f, false);

    const auto nativeSelection = cinematics::SelectCinematicBaseline(
        false, authored, transformedBaseline);
    pass &= Check(nativeSelection.authoredReferenceFov == authored &&
        !nativeSelection.gameplayRetentionWitnessAvailable &&
        nativeSelection.valueOrigin == cinematics::CinematicBaselineValueOrigin::AuthoredEnter,
        "native_ignores_baseline");

    const auto invalidSelection = cinematics::SelectCinematicBaseline(
        true, authored, {});
    pass &= Check(invalidSelection.authoredReferenceFov == authored &&
        !invalidSelection.gameplayRetentionWitnessAvailable,
        "invalid_baseline_fallback");

    const auto gameplaySelection = cinematics::SelectCinematicBaseline(
        true, authored, transformedBaseline);
    pass &= Check(gameplaySelection.targetBaselineFov == transformedBaseline.nativeFov.value &&
        gameplaySelection.gameplayRetentionWitnessAvailable &&
        gameplaySelection.valueOrigin ==
            cinematics::CinematicBaselineValueOrigin::GameplayNativeNumericBaseline,
        "gameplay_horplus_uses_current_gameplay_native_baseline");

    const auto passThroughSelection = cinematics::SelectCinematicBaseline(
        true, authored, passThroughBaseline);
    pass &= Check(passThroughSelection.targetBaselineFov == passThroughBaseline.nativeFov.value &&
        passThroughSelection.gameplayRetentionWitnessAvailable &&
        passThroughSelection.valueOrigin ==
            cinematics::CinematicBaselineValueOrigin::GameplayNativeNumericBaseline,
        "pass_through_gameplay_native_baseline_remains_usable");

    auto malformed = transformedBaseline;
    malformed.nativeFov.value = std::numeric_limits<float>::quiet_NaN();
    const auto malformedSelection = cinematics::SelectCinematicBaseline(
        true, authored, malformed);
    pass &= Check(malformedSelection.authoredReferenceFov == authored &&
        !malformedSelection.gameplayRetentionWitnessAvailable,
        "malformed_native_fallback");

    const auto aspectRecalculationSelection = cinematics::SelectCinematicBaseline(
        false, authored, Baseline(96.0f, 1.77778f, false));
    pass &= Check(aspectRecalculationSelection.targetBaselineFov == authored &&
        !aspectRecalculationSelection.gameplayRetentionWitnessAvailable,
        "native_horplus_uses_authored_enter_reference");

    const cinematics::SharedWriterObservation preEnterWriter{
        41, 8, 0x2000, 0x3000, 90.25f, 3.55556f, 0x05, true, true };
    const diagnostics::BaselineWriterPublicationObservation publicationAtBaseline{
        true, transformedBaseline.observationSequence, 37, 7, 0x1000, 0x3000 };
    pass &= Check(!diagnostics::ShouldEmitCinematicBaselineProvenance(false) &&
        diagnostics::ShouldEmitCinematicBaselineProvenance(true),
        "cinematic_baseline_capture_and_markers_follow_diagnostic_mode");
    pass &= Check(!diagnostics::ShouldCaptureCinematicBaselineProvenance(false,
            camera::GameplayBaselineProjectionDisposition::Updated) &&
        !diagnostics::ShouldCaptureCinematicBaselineProvenance(true,
            camera::GameplayBaselineProjectionDisposition::Retained),
        "cinematic_baseline_off_and_nonpublication_skip_capture");
    pass &= Check(diagnostics::ShouldCaptureCinematicBaselineProvenance(true,
            camera::GameplayBaselineProjectionDisposition::Updated),
        "cinematic_baseline_diagnostic_on_allows_publication_capture");
    diagnostics::BaselineWriterPublicationObservation disabledPublication{};
    if (diagnostics::ShouldCaptureCinematicBaselineProvenance(false,
            camera::GameplayBaselineProjectionDisposition::Updated))
        disabledPublication = publicationAtBaseline;
    pass &= Check(!disabledPublication.captured &&
        disabledPublication.baselinePublicationSequence == 0 &&
        disabledPublication.writerSequence == 0,
        "cinematic_baseline_off_leaves_publication_snapshot_uncollected");
    const auto selectedBeforeDiagnostic = cinematics::SelectCinematicBaseline(
        true, authored, transformedBaseline);
    auto enterRecord = diagnostics::CaptureCinematicEnterBaselineProvenance(
        12, 0x4000, 0x5000, authored,
        config::CinematicFovMode::GameplayHorPlus,
        config::CinematicFovMode::GameplayHorPlus, transformedBaseline,
        publicationAtBaseline, preEnterWriter);
    pass &= Check(enterRecord.generation == 12 && enterRecord.enterRcx == 0x4000 &&
        enterRecord.enterRsi == 0x5000 && enterRecord.baselineUsable &&
        enterRecord.baselinePublication.captured &&
        enterRecord.baselinePublication.writerSequence == 37 &&
        enterRecord.baselinePublication.writerIdentityEpoch == 7 &&
        enterRecord.latestWriterBeforeEnter.sequence == 41 &&
        enterRecord.latestWriterBeforeEnter.identityEpoch == 8 &&
        enterRecord.latestWriterBeforeEnter.source == 0x2000 &&
        enterRecord.latestWriterBeforeEnter.output == 0x3000,
        "cinematic_baseline_snapshot_publication_and_latest_writer_independently");
    diagnostics::RecordCinematicEnterBaselineSelection(
        enterRecord, selectedBeforeDiagnostic, 126.87f, true);
    const auto selectedAfterDiagnostic = cinematics::SelectCinematicBaseline(
        true, authored, transformedBaseline);
    pass &= Check(enterRecord.selectionAvailable &&
        enterRecord.selectedBaseline.targetBaselineFov == transformedBaseline.nativeFov.value &&
        enterRecord.resultingTargetOutput == 126.87f &&
        selectedAfterDiagnostic.targetBaselineFov ==
            selectedBeforeDiagnostic.targetBaselineFov &&
        selectedAfterDiagnostic.valueOrigin == selectedBeforeDiagnostic.valueOrigin &&
        selectedAfterDiagnostic.gameplayRetentionWitnessAvailable ==
            selectedBeforeDiagnostic.gameplayRetentionWitnessAvailable,
        "cinematic_baseline_diagnostic_snapshot_does_not_change_selected_target");

    auto invalidPublication = publicationAtBaseline;
    invalidPublication.baselinePublicationSequence += 1;
    const auto mismatchedPublicationRecord = diagnostics::CaptureCinematicEnterBaselineProvenance(
        13, 0x4000, 0x5000, authored,
        config::CinematicFovMode::GameplayHorPlus,
        config::CinematicFovMode::GameplayHorPlus, transformedBaseline,
        invalidPublication, preEnterWriter);
    pass &= Check(!mismatchedPublicationRecord.baselinePublication.captured &&
        mismatchedPublicationRecord.baselineUsable &&
        mismatchedPublicationRecord.latestWriterBeforeEnter.source == 0x2000,
        "cinematic_baseline_publication_capture_requires_matching_sequence");

    auto differentSourceBaseline = transformedBaseline;
    differentSourceBaseline.source = {0x1000, true};
    auto differentSourceWriter = preEnterWriter;
    differentSourceWriter.source = 0x2000;
    differentSourceWriter.rawFov = differentSourceBaseline.nativeFov.value;
    const auto differentSourceRecord = diagnostics::CaptureCinematicEnterBaselineProvenance(
        14, 0x4000, 0x5000, authored,
        config::CinematicFovMode::GameplayHorPlus,
        config::CinematicFovMode::GameplayHorPlus, differentSourceBaseline,
        publicationAtBaseline, differentSourceWriter);
    pass &= Check(differentSourceRecord.baselineUsable &&
        differentSourceRecord.baseline.source.value == 0x1000 &&
        differentSourceRecord.latestWriterBeforeEnter.source == 0x2000 &&
        differentSourceRecord.baseline.nativeFov.value ==
            differentSourceRecord.latestWriterBeforeEnter.rawFov,
        "cinematic_baseline_different_sources_and_equal_fov_remain_unclassified");

    const auto missingBaselineRecord = diagnostics::CaptureCinematicEnterBaselineProvenance(
        15, 0x4000, 0x5000, authored,
        config::CinematicFovMode::GameplayHorPlus,
        config::CinematicFovMode::GameplayHorPlus, {}, {}, preEnterWriter);
    const auto missingBaselineSelection = cinematics::SelectCinematicBaseline(
        true, authored, missingBaselineRecord.baseline);
    pass &= Check(!missingBaselineRecord.baselineUsable &&
        !missingBaselineRecord.baselinePublication.captured &&
        missingBaselineSelection.valueOrigin ==
            cinematics::CinematicBaselineValueOrigin::AuthoredEnter &&
        missingBaselineSelection.targetBaselineFov == authored,
        "cinematic_baseline_missing_records_authored_fallback_without_relation");

    float forced21x9 = authored;
    float forced16x9 = authored;
    float forced32x9 = authored;
    pass &= Check(cinematics::TryTransformCinematicFov(authored, authored,
        authored, 21.0f / 9.0f, 16.0f / 9.0f, forced21x9),
        "forced_21x9_transform");
    pass &= Check(cinematics::TryTransformCinematicFov(authored, authored,
        authored, 16.0f / 9.0f, 16.0f / 9.0f, forced16x9),
        "forced_16x9_transform");
    pass &= Check(cinematics::TryTransformCinematicFov(authored, authored,
        authored, 32.0f / 9.0f, 16.0f / 9.0f, forced32x9),
        "forced_32x9_transform");
    pass &= Check(std::isfinite(forced21x9) && std::isfinite(forced16x9) &&
        std::isfinite(forced32x9) && forced21x9 != forced16x9 &&
        forced32x9 != forced21x9,
        "cinematic_aspect_independence");
    pass &= Check(transformedBaseline.horPlusFov.value != forced21x9,
        "stored_gameplay_horplus_not_reused");

    constexpr config::CinematicFovMode fovModes[]{
        config::CinematicFovMode::NativeHorPlus,
        config::CinematicFovMode::GameplayHorPlus,
    };
    for (const auto mode : fovModes) {
        float disabledOutput = -1.0f;
        pass &= Check(!cinematics::TryTransformSelectedCinematicFov(false, mode,
            authored, authored, authored,
            32.0f / 9.0f, 16.0f / 9.0f, disabledOutput) &&
            disabledOutput == authored,
            mode == config::CinematicFovMode::NativeHorPlus
                ? "disabled_native_horplus_mode_keeps_native_output"
                : "disabled_gameplay_horplus_mode_keeps_native_output");

        float enabledOutput = authored;
        pass &= Check(cinematics::TryTransformSelectedCinematicFov(true, mode,
            authored, authored, authored,
            32.0f / 9.0f, 16.0f / 9.0f, enabledOutput) &&
            std::isfinite(enabledOutput) && enabledOutput != authored,
            mode == config::CinematicFovMode::NativeHorPlus
                ? "enabled_native_horplus_mode_still_transforms"
                : "enabled_gameplay_horplus_mode_still_transforms");
    }

    float disabledEnterOutput = authored;
    pass &= Check(!cinematics::TryTransformEnterFov(authored, 32.0f / 9.0f,
        false, 16.0f / 9.0f, disabledEnterOutput) && disabledEnterOutput == authored,
        "native_aspect_disabled_behavior");

    constexpr double oraclePi = 3.14159265358979323846;
    constexpr double cinematicNativeAspect = 16.0 / 9.0;
    constexpr float oracleFovs[] = { 70.0f, 90.0f, 100.0f, 120.0f, 130.0f };
    constexpr float oracleAspects[] = { 16.0f / 9.0f, 21.0f / 9.0f,
        32.0f / 9.0f };
    for (const float cinematicFov : oracleFovs) {
        for (const float targetBaseline : oracleFovs) {
            for (const float aspect : oracleAspects) {
                float actual = std::numeric_limits<float>::quiet_NaN();
                const bool transformed = cinematics::TryTransformCinematicFov(
                    cinematicFov, 90.0f, targetBaseline, aspect,
                    static_cast<float>(cinematicNativeAspect), actual);
                const double matchedNative = 2.0 * std::atan(
                    std::tan(static_cast<double>(cinematicFov) * oraclePi / 360.0) *
                    (std::tan(static_cast<double>(targetBaseline) * oraclePi / 360.0) /
                        std::tan(oraclePi / 4.0))) * 180.0 / oraclePi;
                const double expected = 2.0 * std::atan(
                    std::tan(matchedNative * oraclePi / 360.0) *
                    (static_cast<double>(aspect) / cinematicNativeAspect)) *
                    180.0 / oraclePi;
                const bool expectedValid = std::isfinite(expected) &&
                    expected > 1.0 && expected < 179.0;
                pass &= Check(transformed == expectedValid &&
                    Near(actual, expectedValid ? static_cast<float>(expected) : cinematicFov,
                        0.04f), "cinematic_mapping_matches_independent_oracle");
            }
        }
    }

    // Match production's canonical out-of-line, process-owned trace storage.
    auto continuationTrace = std::make_unique<diagnostics::CinematicContinuationTrace>();
    continuationTrace->Begin(false, 9, 0x1000, 90.0f, 127.393f);
    const diagnostics::ContinuationTraceInput disabledTraceSample{
        0x1000, true, 90.0f, 127.393f, 90.0f, 127.393f,
        static_cast<std::uint8_t>(cinematics::CinematicWriterAction::RetainContinuationTarget),
        true, false
    };
    pass &= Check(!continuationTrace->Record(false, disabledTraceSample) &&
        !continuationTrace->Finish(false, "diagnostic_off").active,
        "continuation_trace_diagnostic_off_does_not_collect");

    cinematics::CinematicWriterSample productionSample{
        0x1000, 0x1000, 110.0f, 90.0f, 127.393f, 90.0f, true, true, false };
    const auto outputBeforeTrace = cinematics::ResolveCinematicWriterSample(productionSample);
    continuationTrace->Begin(true, 9, 0x1000, 90.0f, 127.393f);
    auto traceSample = disabledTraceSample;
    pass &= Check(continuationTrace->Record(true, traceSample),
        "continuation_trace_records_first_same_source_sample");
    pass &= Check(continuationTrace->Record(true, traceSample),
        "continuation_trace_keeps_repeated_semantic_sample_without_dedup");
    traceSample.rawInputFov = 126.5f;
    traceSample.outputFov = 127.393f;
    traceSample.inSemanticTargetRegion = true;
    pass &= Check(continuationTrace->Record(true, traceSample),
        "continuation_trace_records_target_region_entry_sample");
    traceSample.rawInputFov = 125.0f;
    traceSample.inSemanticTargetRegion = false;
    pass &= Check(continuationTrace->Record(true, traceSample),
        "continuation_trace_records_target_region_exit_sample");
    traceSample.rawInputFov = 128.0f;
    traceSample.inProductionRange = false;
    pass &= Check(continuationTrace->Record(true, traceSample),
        "continuation_trace_records_overshoot_sample");
    traceSample.source = 0x2000;
    traceSample.sourceMatch = false;
    pass &= Check(continuationTrace->Record(true, traceSample),
        "continuation_trace_records_source_mismatch");
    for (std::size_t i = 0; i < diagnostics::kContinuationTraceCapacity; ++i)
        static_cast<void>(continuationTrace->Record(true, traceSample));
    const auto traceSnapshot = continuationTrace->Finish(true, "test_exit");
    const auto outputAfterTrace = cinematics::ResolveCinematicWriterSample(productionSample);
    pass &= Check(traceSnapshot.active && traceSnapshot.generation == 9 &&
        traceSnapshot.storedSamples == diagnostics::kContinuationTraceCapacity &&
        traceSnapshot.totalSamples == diagnostics::kContinuationTraceCapacity + 6 &&
        traceSnapshot.overflow && std::string_view(traceSnapshot.clearReason.data()) == "test_exit",
        "continuation_trace_is_bounded_and_marks_overflow");
    pass &= Check(diagnostics::HasContinuationTraceEvent(
            traceSnapshot.samples[2].events,
            diagnostics::ContinuationTraceEvent::FirstTargetRegionEntry) &&
        diagnostics::HasContinuationTraceEvent(
            traceSnapshot.samples[3].events,
            diagnostics::ContinuationTraceEvent::TargetRegionExit) &&
        diagnostics::HasContinuationTraceEvent(
            traceSnapshot.samples[3].events,
            diagnostics::ContinuationTraceEvent::DirectionReversal) &&
        diagnostics::HasContinuationTraceEvent(
            traceSnapshot.samples[4].events,
            diagnostics::ContinuationTraceEvent::TargetOvershoot) &&
        diagnostics::HasContinuationTraceEvent(
            traceSnapshot.samples[4].events,
            diagnostics::ContinuationTraceEvent::TargetCrossing) &&
        diagnostics::HasContinuationTraceEvent(
            traceSnapshot.samples[5].events,
            diagnostics::ContinuationTraceEvent::SourceMismatch),
        "continuation_trace_marks_region_reversal_crossing_and_source_events");
    pass &= Check(outputBeforeTrace.action == outputAfterTrace.action &&
        outputBeforeTrace.outputFov == outputAfterTrace.outputFov &&
        traceSnapshot.samples[0].outputFov == disabledTraceSample.outputFov,
        "continuation_trace_observation_does_not_change_camera_policy_or_output");

    std::cout << "Cinematic FOV cutover harness: " << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
