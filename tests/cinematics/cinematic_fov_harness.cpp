#include "../../src/cinematics/cinematic_fov.hpp"
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
    const float authored = 90.0f;
    const auto transformedBaseline = Baseline(112.623f, 3.55556f, true);
    const auto passThroughBaseline = Baseline(104.0f, 2.4f, false);

    const auto nativeSelection = cinematics::SelectCinematicBaseline(
        false, authored, transformedBaseline);
    pass &= Check(nativeSelection.targetBaselineFov == authored &&
        !nativeSelection.usedGameplayBaselineValue &&
        nativeSelection.valueOrigin == cinematics::CinematicBaselineValueOrigin::AuthoredEnter,
        "native_ignores_baseline");

    const auto invalidSelection = cinematics::SelectCinematicBaseline(
        true, authored, {});
    pass &= Check(invalidSelection.targetBaselineFov == authored &&
        !invalidSelection.usedGameplayBaselineValue,
        "invalid_baseline_fallback");

    const auto gameplaySelection = cinematics::SelectCinematicBaseline(
        true, authored, transformedBaseline);
    pass &= Check(gameplaySelection.targetBaselineFov == 112.623f &&
        gameplaySelection.usedGameplayBaselineValue &&
        gameplaySelection.valueOrigin == cinematics::CinematicBaselineValueOrigin::GameplayNativeNumericBaseline,
        "transformed_baseline_native_target");

    const auto passThroughSelection = cinematics::SelectCinematicBaseline(
        true, authored, passThroughBaseline);
    pass &= Check(passThroughSelection.targetBaselineFov == 104.0f &&
        passThroughSelection.usedGameplayBaselineValue,
        "passthrough_baseline_native_target");

    auto malformed = transformedBaseline;
    malformed.nativeFov.value = std::numeric_limits<float>::quiet_NaN();
    const auto malformedSelection = cinematics::SelectCinematicBaseline(
        true, authored, malformed);
    pass &= Check(malformedSelection.targetBaselineFov == authored &&
        !malformedSelection.usedGameplayBaselineValue,
        "malformed_native_fallback");

    const auto aspectRecalculationSelection = cinematics::SelectCinematicBaseline(
        true, authored, Baseline(96.0f, 1.77778f, false));
    pass &= Check(aspectRecalculationSelection.targetBaselineFov == 96.0f &&
        aspectRecalculationSelection.usedGameplayBaselineValue,
        "aspect_recalculation_native_target");

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
        enterRecord, selectedBeforeDiagnostic, 127.393f, true);
    const auto selectedAfterDiagnostic = cinematics::SelectCinematicBaseline(
        true, authored, transformedBaseline);
    pass &= Check(enterRecord.selectionAvailable &&
        enterRecord.selectedBaseline.targetBaselineFov == 112.623f &&
        enterRecord.resultingTargetOutput == 127.393f &&
        selectedAfterDiagnostic.targetBaselineFov ==
            selectedBeforeDiagnostic.targetBaselineFov &&
        selectedAfterDiagnostic.valueOrigin == selectedBeforeDiagnostic.valueOrigin &&
        selectedAfterDiagnostic.usedGameplayBaselineValue ==
            selectedBeforeDiagnostic.usedGameplayBaselineValue,
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
        gameplaySelection.targetBaselineFov, 21.0f / 9.0f, 16.0f / 9.0f, forced21x9),
        "forced_21x9_transform");
    pass &= Check(cinematics::TryTransformCinematicFov(authored, authored,
        gameplaySelection.targetBaselineFov, 16.0f / 9.0f, 16.0f / 9.0f, forced16x9),
        "forced_16x9_transform");
    pass &= Check(cinematics::TryTransformCinematicFov(authored, authored,
        gameplaySelection.targetBaselineFov, 32.0f / 9.0f, 16.0f / 9.0f, forced32x9),
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
            authored, authored, gameplaySelection.targetBaselineFov,
            32.0f / 9.0f, 16.0f / 9.0f, disabledOutput) &&
            disabledOutput == authored,
            mode == config::CinematicFovMode::NativeHorPlus
                ? "disabled_native_horplus_mode_keeps_native_output"
                : "disabled_gameplay_horplus_mode_keeps_native_output");

        float enabledOutput = authored;
        pass &= Check(cinematics::TryTransformSelectedCinematicFov(true, mode,
            authored, authored, gameplaySelection.targetBaselineFov,
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
