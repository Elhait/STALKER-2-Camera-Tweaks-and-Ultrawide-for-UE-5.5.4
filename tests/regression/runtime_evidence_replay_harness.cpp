#include "../fixtures/runtime/gameplay/horplus_evidence.hpp"
#include "../fixtures/runtime/transitions/restoration_evidence.hpp"
#include "../fixtures/runtime/cinematics/cinematic_evidence.hpp"
#include "../fixtures/runtime/dialogue/candidate_evidence.hpp"

#include "../../src/camera/gameplay_aspect_restoration.hpp"
#include "../../src/camera/gameplay_baseline.hpp"
#include "../../src/cinematics/cinematic_fov.hpp"
#include "../../src/dialogue/dialogue_state.hpp"
#include "../../src/gameplay/horplus_gameplay.hpp"
#include "../../src/gameplay/gameplay_state.hpp"

#include <cmath>
#include <iostream>
#include <limits>

namespace
{
    bool Check(bool condition, const char* name)
    {
        if (!condition) std::cerr << name << ": FAIL\n";
        return condition;
    }

    bool Near(float actual, float expected, float epsilon = 0.02f)
    {
        return std::isfinite(actual) && std::fabs(actual - expected) <= epsilon;
    }

    camera::CameraFovObservation Observation(float nativeFov, float resultFov,
        camera::FovSpace resultSpace, camera::FovProvenance resultProvenance,
        float aspect, std::uintptr_t source, std::uint64_t sequence,
        camera::FovObservationBoundary boundary = camera::FovObservationBoundary::CameraWriter)
    {
        camera::CameraFovObservation observation{};
        observation.boundary = boundary;
        observation.inputFov = { nativeFov, camera::FovSpace::Native,
            camera::FovProvenance::NativeRegisterInput, true };
        observation.resultFov = { resultFov, resultSpace, resultProvenance, true };
        observation.aspect = { aspect, camera::FovProvenance::ResolvedAspect, true };
        observation.writerSource = { source, true };
        observation.writerFlags = 0x4;
        observation.writerFlagsValid = true;
        observation.publicationSequence = sequence;
        return observation;
    }
}

int main()
{
    bool pass = true;
    constexpr auto gameplay = runtime_evidence::gameplay::kRecordedGameplay;
    constexpr auto cinematic = runtime_evidence::cinematics::kRecorded32x9;
    constexpr auto forcedCinematic = runtime_evidence::cinematics::kRecorded21x9;

    float gameplayOutput = 0.0f;
    const auto gameplayResult = gameplay::EvaluateHorPlus(
        gameplay.nativeFov, gameplay.aspect, 0x4, 1.77778f);
    pass &= Check(gameplayResult.eligible && gameplayResult.applied &&
        Near(gameplayResult.outputFov, gameplay.expectedHorPlus),
        "recorded_gameplay_horplus");
    pass &= Check(gameplay::TryTransformHorPlus(
        gameplay.nativeFov, gameplay.aspect, 0x4, 1.77778f, gameplayOutput) &&
        Near(gameplayOutput, gameplay.expectedHorPlus),
        "recorded_gameplay_pair");
    const auto nativeAspectResult = gameplay::EvaluateHorPlus(
        80.0f, 1.77778f, 0x4, 1.77778f);
    pass &= Check(!nativeAspectResult.applied &&
        nativeAspectResult.outputFov == nativeAspectResult.inputFov,
        "native_aspect_is_identity");

    constexpr float fovSweep[] = { 30.0f, 80.0f, 112.623f, 150.0f };
    for (const auto& fixture : runtime_evidence::gameplay::kAspectSweep) {
        for (const float fov : fovSweep) {
            for (const std::uint8_t flags : { std::uint8_t{0x4}, std::uint8_t{0x5} }) {
                float output = 0.0f;
                pass &= Check(gameplay::TryTransformHorPlus(
                    fov, fixture.aspect, flags, 1.77778f, output) &&
                    std::isfinite(output), "arbitrary_fov_aspect_is_finite");
            }
        }
    }
    float ineligibleOutput = 0.0f;
    pass &= Check(!gameplay::TryTransformHorPlus(
        gameplay.nativeFov, gameplay.aspect, 0, 1.77778f, ineligibleOutput) &&
        ineligibleOutput == gameplay.nativeFov, "unsupported_flags_pass_through");
    float invalidOutput = 123.0f;
    pass &= Check(!gameplay::TryTransformHorPlus(
        std::numeric_limits<float>::quiet_NaN(), 3.55556f, 0, 1.77778f,
        invalidOutput), "invalid_gameplay_input_fails_closed");

    float infinityOutput = 123.0f;
    pass &= Check(!gameplay::TryTransformHorPlus(
        std::numeric_limits<float>::infinity(), 4.0f, 0x4, 1.77778f,
        infinityOutput), "infinite_gameplay_input_fails_closed");

    camera::GameplayBaselineStore baselineStore;
    const auto baselineA = baselineStore.Project(Observation(
        112.623f, 143.132f, camera::FovSpace::Transformed,
        camera::FovProvenance::ModTransformResult, 3.55556f, 0x1000, 1), true);
    pass &= Check(baselineA.baseline.valid && baselineA.baseline.horPlusFov.valid,
        "baseline_a_is_retained");
    const auto cinematicObservation = baselineStore.Project(Observation(
        90.0f, 126.87f, camera::FovSpace::Transformed,
        camera::FovProvenance::ModTransformResult, 3.55556f, 0x2000, 2,
        camera::FovObservationBoundary::CinematicEnter), false);
    pass &= Check(cinematicObservation.baseline.nativeFov.value == 112.623f &&
        cinematicObservation.baseline.horPlusFov.value == 143.132f,
        "cinematic_cannot_contaminate_gameplay_baseline");
    const auto passThroughB = baselineStore.Project(Observation(
        80.0f, 80.0f, camera::FovSpace::Native,
        camera::FovProvenance::PassThroughResult, 2.4f, 0x3000, 3), true);
    pass &= Check(passThroughB.baseline.nativeFov.value == 80.0f &&
        !passThroughB.baseline.horPlusFov.valid,
        "passthrough_b_clears_stale_transformed_result");
    auto transformedInput = Observation(
        126.87f, 143.132f, camera::FovSpace::Transformed,
        camera::FovProvenance::ModTransformResult, 3.55556f, 0x4000, 4);
    transformedInput.inputFov.provenance = camera::FovProvenance::ModTransformResult;
    transformedInput.inputFov.space = camera::FovSpace::Transformed;
    const auto rejectedTransformedInput = baselineStore.Project(transformedInput, true);
    pass &= Check(rejectedTransformedInput.baseline.nativeFov.value == 80.0f &&
        !rejectedTransformedInput.eligible,
        "transformed_sample_cannot_become_native_baseline");

    camera::GameplayBaselineStore dynamicAspectBaseline;
    constexpr float dynamicAspects[] = { 1.77778f, 2.4f, 3.0f, 3.55556f, 2.4f,
        1.77778f };
    std::uint64_t dynamicSequence = 10;
    for (const float aspect : dynamicAspects) {
        const bool wider = aspect > 1.77778f + 0.001f;
        float transformed = 90.0f;
        const bool applied = wider && gameplay::TryTransformHorPlus(
            90.0f, aspect, 0x4, 1.77778f, transformed);
        const auto observation = Observation(
            90.0f, applied ? transformed : 90.0f,
            applied ? camera::FovSpace::Transformed : camera::FovSpace::Native,
            applied ? camera::FovProvenance::ModTransformResult :
                camera::FovProvenance::PassThroughResult,
            aspect, 0x5000, dynamicSequence++);
        const auto projected = dynamicAspectBaseline.Project(observation, true);
        pass &= Check(projected.baseline.aspect == aspect &&
            projected.baseline.nativeFov.value == 90.0f,
            "dynamic_aspect_updates_native_baseline");
    }
    const auto dynamicFinal = dynamicAspectBaseline.Read();
    pass &= Check(dynamicFinal.aspect == 1.77778f && dynamicFinal.nativeFov.value == 90.0f,
        "dynamic_aspect_return_does_not_use_stale_aspect");
    const auto aspectToHorPlus = gameplay::ResolveGameplayModeTransition(
        config::GameplayMode::AspectRecalculation, config::GameplayMode::HorPlus,
        camera::CoordinatorState::Gameplay);
    const auto horPlusToAspect = gameplay::ResolveGameplayModeTransition(
        config::GameplayMode::HorPlus, config::GameplayMode::AspectRecalculation,
        camera::CoordinatorState::Gameplay);
    pass &= Check(aspectToHorPlus.changed && aspectToHorPlus.invalidateHorPlusState &&
        horPlusToAspect.changed && horPlusToAspect.invalidateHorPlusState,
        "gameplay_mode_round_trip_contract");

    camera::GameplayAspectRestorationStore restoration;
    constexpr auto roundTrip = runtime_evidence::transitions::kRecordedRoundTrip;
    const auto update = restoration.Update(roundTrip.retainedAspect,
        { roundTrip.source, true });
    pass &= Check(update.state.valid && Near(update.state.aspect,
        roundTrip.retainedAspect, 0.0001f), "restoration_update_retains_target");
    const auto invalidated = restoration.Invalidate();
    pass &= Check(invalidated.changed && !invalidated.state.valid,
        "restoration_invalidate_clears_state");
    const auto republished = restoration.Update(roundTrip.retainedAspect,
        { roundTrip.source, true });
    pass &= Check(republished.state.valid && Near(republished.state.aspect,
        roundTrip.retainedAspect, 0.0001f), "restoration_republishes_target");
    const auto decision = camera::ResolveGameplayAspectRestorationDecision({
        true, true, true, true, true, true, false, true, true, true },
        republished.state.aspect);
    pass &= Check(decision.decision ==
        camera::GameplayAspectRestorationDecision::Restore &&
        Near(decision.restorationAspect, roundTrip.retainedAspect, 0.0001f),
        "restoration_round_trip_restores_target");

    float nativeHorPlus = 0.0f;
    float gameplayHorPlus = 0.0f;
    pass &= Check(cinematics::TryTransformCinematicFov(
        cinematic.authoredEnterFov, cinematic.authoredEnterFov,
        cinematic.authoredEnterFov, cinematic.aspect, cinematic.nativeAspect,
        nativeHorPlus) && Near(nativeHorPlus, cinematic.expectedNativeHorPlus),
        "native_horplus_recorded_endpoint");
    pass &= Check(cinematics::TryTransformCinematicFov(
        cinematic.authoredEnterFov, cinematic.authoredEnterFov,
        cinematic.gameplayBaselineNativeFov, cinematic.aspect,
        cinematic.nativeAspect, gameplayHorPlus) &&
        Near(gameplayHorPlus, cinematic.expectedGameplayHorPlus),
        "gameplay_horplus_recorded_endpoint");
    pass &= Check(!Near(nativeHorPlus, gameplayHorPlus),
        "cinematic_modes_have_distinct_reference_inputs");

    float forcedNativeHorPlus = 0.0f;
    float forcedGameplayHorPlus = 0.0f;
    pass &= Check(cinematics::TryTransformCinematicFov(
        forcedCinematic.authoredEnterFov, forcedCinematic.authoredEnterFov,
        forcedCinematic.authoredEnterFov, forcedCinematic.aspect,
        forcedCinematic.nativeAspect, forcedNativeHorPlus) &&
        Near(forcedNativeHorPlus, forcedCinematic.expectedNativeHorPlus),
        "forced_21x9_native_horplus_recorded_endpoint");
    pass &= Check(cinematics::TryTransformCinematicFov(
        forcedCinematic.authoredEnterFov, forcedCinematic.authoredEnterFov,
        forcedCinematic.gameplayBaselineNativeFov, forcedCinematic.aspect,
        forcedCinematic.nativeAspect, forcedGameplayHorPlus) &&
        Near(forcedGameplayHorPlus, forcedCinematic.expectedGameplayHorPlus),
        "forced_21x9_gameplay_horplus_recorded_endpoint");

    constexpr float authoredSweep[] = { 55.0f, 90.0f, 112.0f, 140.0f };
    constexpr float aspectSweep[] = { 1.77778f, 1.8f, 2.4f, 3.55556f, 4.0f };
    for (const float authoredFov : authoredSweep) {
        for (const float aspect : aspectSweep) {
            float nativeMode = 0.0f;
            float gameplayMode = 0.0f;
            const bool nativeValid = cinematics::TryTransformCinematicFov(
                authoredFov, authoredFov, authoredFov, aspect, 1.77778f, nativeMode);
            const bool gameplayValid = cinematics::TryTransformCinematicFov(
                authoredFov, authoredFov, cinematic.gameplayBaselineNativeFov,
                aspect, 1.77778f, gameplayMode);
            pass &= Check(nativeValid && gameplayValid && std::isfinite(nativeMode) &&
                std::isfinite(gameplayMode), "cinematic_sweep_is_finite");
        }
    }
    float invalidCinematic = 99.0f;
    pass &= Check(!cinematics::TryTransformCinematicFov(
        90.0f, std::numeric_limits<float>::quiet_NaN(), 112.623f,
        3.55556f, 1.77778f, invalidCinematic) && invalidCinematic == 90.0f,
        "invalid_cinematic_reference_fails_closed");

    constexpr auto candidate = runtime_evidence::dialogue::kRecordedCandidate;
    dialogue::CandidateTracker tracker;
    tracker.Begin(candidate.initialSample, candidate.source, candidate.nativeTarget);
    pass &= Check(tracker.Observe(candidate.descendingSample, candidate.source,
        candidate.nativeTarget, 0.01f, 0.01f) ==
        dialogue::CandidateDecision::Activate,
        "coherent_candidate_promotes");
    tracker.Begin(candidate.initialSample, candidate.source, candidate.nativeTarget);
    pass &= Check(tracker.Observe(candidate.descendingSample, candidate.changedSource,
        candidate.nativeTarget, 0.01f, 0.01f) ==
        dialogue::CandidateDecision::Cancel && !tracker.IsPending(),
        "changed_source_cancels_candidate");
    tracker.Begin(candidate.initialSample, candidate.source, candidate.nativeTarget);
    pass &= Check(tracker.Observe(candidate.descendingSample, candidate.source,
        candidate.contradictoryTarget, 0.01f, 0.01f) ==
        dialogue::CandidateDecision::Cancel && !tracker.IsPending(),
        "contradictory_target_cancels_candidate");

    std::cout << "runtime_evidence_replay=" << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
