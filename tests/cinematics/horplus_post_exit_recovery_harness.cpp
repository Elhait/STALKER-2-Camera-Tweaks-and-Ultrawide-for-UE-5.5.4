#include "../../src/camera/gameplay_baseline.hpp"
#include "../../src/camera/horplus.hpp"
#include "../../src/cinematics/cinematic_fov.hpp"
#include "../../src/dialogue/dialogue_state.hpp"
#include "../../src/gameplay/gameplay_state.hpp"
#include "../../src/gameplay/horplus_gameplay.hpp"

#include <cmath>
#include <iostream>
#include <limits>

namespace
{
    constexpr float kNativeAspect = 16.0f / 9.0f;
    constexpr float kAspect = 3440.0f / 1440.0f;
    constexpr float kRecoveryEpsilon = 0.01f;
    constexpr std::uintptr_t kCameraSource = 0x1234;

    bool Check(bool condition, const char* name)
    {
        if (!condition) std::cerr << name << ": FAIL\n";
        return condition;
    }

    bool Near(float left, float right, float epsilon = 0.02f)
    {
        return std::isfinite(left) && std::isfinite(right) &&
            std::fabs(left - right) <= epsilon;
    }

    camera::CameraFovObservation MakeObservation(
        float nativeFov, float resultFov, bool transformed, std::uint64_t sequence,
        float aspect = kAspect, std::uintptr_t source = kCameraSource,
        std::uint8_t flags = 0x4)
    {
        camera::CameraFovObservation observation{};
        observation.boundary = camera::FovObservationBoundary::CameraWriter;
        observation.inputFov = { nativeFov, camera::FovSpace::Native,
            camera::FovProvenance::NativeRegisterInput, true };
        observation.resultFov = { resultFov,
            transformed ? camera::FovSpace::Transformed : camera::FovSpace::Native,
            transformed ? camera::FovProvenance::ModTransformResult :
                camera::FovProvenance::PassThroughResult, true };
        observation.aspect = { aspect, camera::FovProvenance::ResolvedAspect, true };
        observation.writerSource = { source, true };
        observation.writerFlags = flags;
        observation.writerFlagsValid = true;
        observation.publicationSequence = sequence;
        return observation;
    }

    float ProcessWriterSample(float inputFov, float exitTarget,
        dialogue::PostCinematicRecoveryExclusion& recovery,
        camera::CoordinatorState& coordinator,
        camera::GameplayBaselineStore& baseline, std::uint64_t sequence,
        float aspect = kAspect, std::uintptr_t source = kCameraSource,
        std::uint8_t flags = 0x4, bool cameraReadable = true,
        float cachedCinematicFov = std::numeric_limits<float>::quiet_NaN(),
        bool gameplayEnabled = true)
    {
        auto observation = recovery.ObserveValidated(
            source, inputFov, kRecoveryEpsilon);
        const auto retainedBaseline = baseline.Read();
        gameplay::HorPlusRecoverySample sample{};
        sample.source = source;
        sample.validatedSource = observation ==
                dialogue::PostCinematicRecoveryExclusion::Observation::Recovered
            ? source
            : camera::IsUsableGameplayBaseline(retainedBaseline) && retainedBaseline.source.valid
                ? retainedBaseline.source.value : 0;
        sample.inputFov = inputFov;
        sample.exitNativeTarget = exitTarget;
        sample.cachedCinematicFov = cachedCinematicFov;
        sample.aspect = aspect;
        sample.flags = flags;
        sample.cameraReadable = cameraReadable;
        sample.gameplayEnabled = gameplayEnabled;
        auto action = gameplay::ResolveHorPlusRecoveryAction(coordinator,
            sample, gameplay::IsNativeHorPlusRecoverySample(sample, kRecoveryEpsilon),
            kRecoveryEpsilon);
        if (action == gameplay::HorPlusRecoveryAction::HoldNativePassThrough) {
            if (observation == dialogue::PostCinematicRecoveryExclusion::Observation::Inactive ||
                observation == dialogue::PostCinematicRecoveryExclusion::Observation::Cancelled) {
                recovery.Arm(exitTarget);
                recovery.ObserveValidated(source, inputFov, kRecoveryEpsilon);
            }
            return inputFov;
        }
        if (action == gameplay::HorPlusRecoveryAction::TransformRecoveryInterpolation) {
            const float mapped = gameplay::ResolveHorPlusRecoveryInterpolationFov(
                sample, kNativeAspect);
            return std::isfinite(mapped) ? mapped : inputFov;
        }
        if (action == gameplay::HorPlusRecoveryAction::ResumeGameplay)
            coordinator = camera::CoordinatorState::Gameplay;
        if (coordinator != camera::CoordinatorState::Gameplay) return inputFov;

        const auto transform = gameplay::EvaluateHorPlus(
            inputFov, aspect, flags, kNativeAspect);
        const float output = transform.applied ? transform.outputFov : inputFov;
        const auto nativeObservation = MakeObservation(
            inputFov, output, transform.applied, sequence, aspect, source, flags);
        baseline.Project(nativeObservation, true);
        return output;
    }

    bool CheckAlreadyNativeRecovery(float nativeFov, float aspect)
    {
        bool pass = true;
        const auto expected = gameplay::EvaluateHorPlus(nativeFov, aspect, 0x4, kNativeAspect);
        float cinematicFov = 0.0f;
        pass &= Check(cinematics::TryTransformCinematicFov(
            nativeFov, nativeFov, nativeFov, aspect, kNativeAspect, cinematicFov) &&
            Near(cinematicFov, camera::HorPlus(nativeFov, aspect, kNativeAspect)),
            "already_native_scenario_has_transformed_cinematic_before_exit");
        camera::GameplayBaselineStore baseline;
        baseline.Project(MakeObservation(nativeFov, expected.outputFov,
            expected.applied, 1, aspect), true);
        const auto exit = gameplay::ResolveCinematicExitTransition(
            true, true, config::GameplayMode::HorPlus);
        auto coordinator = exit.nextState;
        dialogue::PostCinematicRecoveryExclusion recovery;
        recovery.Arm(nativeFov);

        const float firstOutput = ProcessWriterSample(nativeFov, nativeFov,
            recovery, coordinator, baseline, 2, aspect);
        pass &= Check(coordinator == camera::CoordinatorState::Gameplay &&
            Near(firstOutput, expected.outputFov),
            "first_already_native_sample_resumes_without_departure");
        pass &= Check(baseline.Read().observationSequence == 2 &&
            Near(baseline.Read().nativeFov.value, nativeFov),
            "immediate_recovery_projects_native_baseline");
        pass &= Check(recovery.IsActive(),
            "immediate_gameplay_recovery_does_not_clear_dialogue_exclusion");

        const float steadyOutput = ProcessWriterSample(nativeFov, nativeFov,
            recovery, coordinator, baseline, 3, aspect);
        pass &= Check(Near(steadyOutput, expected.outputFov) && recovery.IsActive(),
            "steady_native_gameplay_does_not_require_ads_to_unlock");

        const float adsFov = nativeFov - 10.0f;
        const auto adsExpected = gameplay::EvaluateHorPlus(adsFov, aspect, 0x4, kNativeAspect);
        const float adsOutput = ProcessWriterSample(adsFov, nativeFov,
            recovery, coordinator, baseline, 4, aspect);
        pass &= Check(Near(adsOutput, adsExpected.outputFov),
            "ads_after_immediate_recovery_remains_eligible");
        pass &= Check(recovery.IsActive(), "dialogue_still_requires_return_after_departure");
        const float returnedOutput = ProcessWriterSample(nativeFov, nativeFov,
            recovery, coordinator, baseline, 5, aspect);
        pass &= Check(Near(returnedOutput, expected.outputFov) && !recovery.IsActive(),
            "dialogue_depart_return_semantics_preserved");

        const float changedFov = nativeFov + 5.0f;
        const auto changedExpected = gameplay::EvaluateHorPlus(
            changedFov, aspect, 0x4, kNativeAspect);
        const float changedOutput = ProcessWriterSample(changedFov, nativeFov,
            recovery, coordinator, baseline, 6, aspect);
        pass &= Check(coordinator == camera::CoordinatorState::Gameplay &&
            Near(changedOutput, changedExpected.outputFov) &&
            Near(baseline.Read().nativeFov.value, changedFov),
            "later_fov_change_after_immediate_recovery_updates_baseline");
        return pass;
    }

    bool CheckRecoveryEvidenceGates()
    {
        bool pass = true;
        gameplay::HorPlusRecoverySample native{};
        native.source = kCameraSource;
        native.validatedSource = kCameraSource;
        native.inputFov = 90.6557f;
        native.exitNativeTarget = 90.6557f;
        native.aspect = 3.0f;
        native.flags = 0x4;
        native.cameraReadable = true;
        pass &= Check(gameplay::IsNativeHorPlusRecoverySample(native, kRecoveryEpsilon),
            "recorded_native_recovery_evidence_accepted");
        const auto reject = [&pass](const gameplay::HorPlusRecoverySample& sample,
            const char* name) {
            pass &= Check(!gameplay::IsNativeHorPlusRecoverySample(sample, kRecoveryEpsilon), name);
        };
        auto sample = native;
        sample.inputFov = 119.272f;
        reject(sample, "transformed_sample_is_not_native_recovery");
        sample = native;
        sample.validatedSource = 0;
        reject(sample, "missing_source_ownership_is_not_recovery");
        sample = native;
        sample.source = kCameraSource + 1;
        reject(sample, "different_source_numeric_match_is_not_recovery");
        sample = native;
        sample.source = 0;
        reject(sample, "null_source_is_not_recovery");
        sample = native;
        sample.flags = 0x5;
        reject(sample, "cinematic_flags_are_not_gameplay_recovery");
        sample = native;
        sample.cameraReadable = false;
        reject(sample, "unreadable_camera_is_not_recovery");
        sample = native;
        sample.aspect = 0.0f;
        reject(sample, "invalid_aspect_is_not_recovery");
        sample = native;
        sample.aspect = std::numeric_limits<float>::quiet_NaN();
        reject(sample, "nonfinite_aspect_is_not_recovery");
        sample = native;
        sample.inputFov = std::numeric_limits<float>::quiet_NaN();
        reject(sample, "nonfinite_fov_is_not_recovery");
        sample = native;
        sample.exitNativeTarget = 0.0f;
        reject(sample, "invalid_exit_target_is_not_recovery");
        pass &= Check(!gameplay::IsNativeHorPlusRecoverySample(native, -1.0f) &&
            !gameplay::IsNativeHorPlusRecoverySample(native,
                std::numeric_limits<float>::quiet_NaN()), "invalid_epsilon_is_not_recovery");

        camera::GameplayBaselineStore baseline;
        const auto transformed = gameplay::EvaluateHorPlus(90.0f, kAspect, 0x4, kNativeAspect);
        baseline.Project(MakeObservation(90.0f, transformed.outputFov, true, 1), true);
        auto coordinator = camera::CoordinatorState::CinematicExiting;
        dialogue::PostCinematicRecoveryExclusion recovery;
        recovery.Arm(90.0f);
        const auto unownedSource = kCameraSource + 1;
        const float unownedOutput = ProcessWriterSample(90.0f, 90.0f,
            recovery, coordinator, baseline, 2, kAspect, unownedSource);
        pass &= Check(coordinator == camera::CoordinatorState::CinematicExiting &&
            Near(unownedOutput, 90.0f) && baseline.Read().observationSequence == 1,
            "unowned_numeric_match_does_not_resume_or_publish_baseline");
        const float wrongFlagsOutput = ProcessWriterSample(90.0f, 90.0f,
            recovery, coordinator, baseline, 3, kAspect, kCameraSource, 0x5);
        pass &= Check(coordinator == camera::CoordinatorState::CinematicExiting &&
            Near(wrongFlagsOutput, 90.0f) && baseline.Read().observationSequence == 1,
            "wrong_flags_do_not_resume_or_publish_baseline");
        const float validOutput = ProcessWriterSample(90.0f, 90.0f,
            recovery, coordinator, baseline, 4);
        pass &= Check(coordinator == camera::CoordinatorState::Gameplay &&
            Near(validOutput, transformed.outputFov) && recovery.IsActive(),
            "validated_owner_resumes_without_weakening_dialogue_after_rebind");
        return pass;
    }
}

bool TestRecordedRecoveryInterpolationRemainsInHorPlusSpace()
{
    bool pass = true;
    constexpr float nativeFov = 90.0f;
    constexpr float cachedCinematicFov = 106.688f;
    constexpr float exitTarget = nativeFov;
    constexpr float horPlusTarget = 106.688f;
    const float transitionSamples[]{
        106.688f, 105.512f, 104.237f, 102.448f, 101.175f,
        99.7347f, 98.682f, 97.3072f, 95.8426f, 94.5296f,
        93.216f, 91.983f, 90.8222f
    };

    camera::GameplayBaselineStore baseline;
    baseline.Project(MakeObservation(nativeFov, horPlusTarget, true, 1), true);
    auto coordinator = camera::CoordinatorState::CinematicExiting;
    dialogue::PostCinematicRecoveryExclusion recovery;
    recovery.Arm(exitTarget);
    float previousOutput = cachedCinematicFov;
    std::uint64_t sequence = 2;
    for (const float sample : transitionSamples) {
        const float output = ProcessWriterSample(sample, exitTarget, recovery,
            coordinator, baseline, sequence++, kAspect, kCameraSource, 0x4,
            true, cachedCinematicFov);
        pass &= Check(coordinator == camera::CoordinatorState::CinematicExiting,
            "same_source_interpolation_remains_in_explicit_recovery_state");
        pass &= Check(Near(output, horPlusTarget),
            "native_recovery_interpolation_is_mapped_continuously_in_horplus_space");
        pass &= Check(std::fabs(output - previousOutput) < 0.02f,
            "horplus_space_output_has_no_exit_or_recovery_snap");
        pass &= Check(baseline.Read().observationSequence == 1 &&
                Near(baseline.Read().nativeFov.value, nativeFov),
            "transitional_recovery_samples_do_not_contaminate_gameplay_baseline");
        previousOutput = output;
    }

    gameplay::HorPlusRecoverySample evidence{};
    evidence.source = kCameraSource;
    evidence.validatedSource = kCameraSource;
    evidence.inputFov = 105.512f;
    evidence.exitNativeTarget = exitTarget;
    evidence.cachedCinematicFov = cachedCinematicFov;
    evidence.aspect = kAspect;
    evidence.flags = 0x4;
    evidence.cameraReadable = true;
    const auto rejectInterpolation = [&pass](
        gameplay::HorPlusRecoverySample candidate, const char* name) {
        pass &= Check(gameplay::ResolveHorPlusRecoveryAction(
                camera::CoordinatorState::CinematicExiting, candidate, false,
                kRecoveryEpsilon) == gameplay::HorPlusRecoveryAction::HoldNativePassThrough,
            name);
    };
    auto ambiguous = evidence;
    ambiguous.source += 1;
    rejectInterpolation(ambiguous, "replacement_source_requires_its_own_recovery_evidence");
    ambiguous = evidence;
    ambiguous.flags = 0x5;
    rejectInterpolation(ambiguous, "cinematic_writer_flags_remain_pass_through");
    ambiguous = evidence;
    ambiguous.gameplayEnabled = false;
    rejectInterpolation(ambiguous, "disabled_gameplay_does_not_transform_interpolation");
    ambiguous = evidence;
    ambiguous.aspect = std::numeric_limits<float>::quiet_NaN();
    rejectInterpolation(ambiguous, "invalid_aspect_remains_pass_through");
    ambiguous = evidence;
    ambiguous.validatedSource = 0;
    rejectInterpolation(ambiguous, "missing_validated_owner_remains_pass_through");
    ambiguous = evidence;
    ambiguous.cachedCinematicFov = std::numeric_limits<float>::quiet_NaN();
    rejectInterpolation(ambiguous, "missing_cinematic_endpoint_remains_pass_through");

    const float recoveredOutput = ProcessWriterSample(90.008f, exitTarget,
        recovery, coordinator, baseline, sequence++, kAspect, kCameraSource,
        0x4, true, cachedCinematicFov);
    pass &= Check(coordinator == camera::CoordinatorState::Gameplay &&
            Near(recoveredOutput, gameplay::EvaluateHorPlus(90.008f,
                kAspect, 0x4, kNativeAspect).outputFov),
        "native_target_convergence_completes_recovery_and_resumes_gameplay");
    pass &= Check(Near(baseline.Read().nativeFov.value, 90.008f) &&
            baseline.Read().observationSequence == sequence - 1,
        "only_converged_native_sample_updates_gameplay_baseline");

    const float adsNative = 80.0f;
    const auto adsExpected = gameplay::EvaluateHorPlus(
        adsNative, kAspect, 0x4, kNativeAspect);
    const float adsOutput = ProcessWriterSample(adsNative, exitTarget, recovery,
        coordinator, baseline, sequence++, kAspect, kCameraSource);
    pass &= Check(coordinator == camera::CoordinatorState::Gameplay &&
            Near(adsOutput, adsExpected.outputFov) &&
            Near(baseline.Read().nativeFov.value, adsNative),
        "post_recovery_ads_and_native_fov_changes_remain_eligible");
    return pass;
}

int main()
{
    bool pass = true;
    float cinematicFov = 0.0f;
    const bool cinematicTransform = cinematics::TryTransformCinematicFov(
        90.0f, 90.0f, 90.0f, kAspect, kNativeAspect, cinematicFov);
    pass &= Check(cinematicTransform && Near(cinematicFov, 106.688f),
        "cinematic_enter_horplus_90_to_106_688");

    camera::GameplayBaselineStore baseline;
    const auto beforeExit = gameplay::EvaluateHorPlus(90.0f, kAspect, 0x4, kNativeAspect);
    baseline.Project(MakeObservation(90.0f, beforeExit.outputFov,
        beforeExit.applied, 1), true);
    pass &= Check(Near(baseline.Read().nativeFov.value, 90.0f),
        "native_gameplay_baseline_before_cinematic");

    const auto exit = gameplay::ResolveCinematicExitTransition(
        true, true, config::GameplayMode::HorPlus);
    auto coordinator = exit.nextState;
    dialogue::PostCinematicRecoveryExclusion recovery;
    recovery.Arm(90.0f);
    pass &= Check(coordinator == camera::CoordinatorState::CinematicExiting &&
        !exit.armGameplayHandoff, "horplus_exit_waits_without_aspect_handoff");

    const float transitionalOutput = ProcessWriterSample(
        cinematicFov, 90.0f, recovery, coordinator, baseline, 2,
        kAspect, kCameraSource, 0x4, true, cinematicFov);
    const float wouldDoubleTransform = camera::HorPlus(cinematicFov, kAspect, kNativeAspect);
    pass &= Check(coordinator == camera::CoordinatorState::CinematicExiting &&
        Near(transitionalOutput, 106.688f) && !Near(transitionalOutput, 122.044f),
        "transitional_cinematic_fov_is_native_passthrough");
    pass &= Check(Near(wouldDoubleTransform, 122.044f) &&
        Near(baseline.Read().nativeFov.value, 90.0f),
        "transitional_fov_is_neither_retransformed_nor_baselined");

    const float recoveredOutput = ProcessWriterSample(
        90.0f, 90.0f, recovery, coordinator, baseline, 3);
    pass &= Check(coordinator == camera::CoordinatorState::Gameplay &&
        Near(recoveredOutput, 106.688f),
        "validated_native_recovery_resumes_single_horplus_transform");
    pass &= Check(Near(baseline.Read().nativeFov.value, 90.0f) &&
        Near(baseline.Read().horPlusFov.value, 106.688f),
        "validated_recovery_establishes_native_baseline");

    const float legitimateChangedFovOutput = ProcessWriterSample(
        80.0f, 90.0f, recovery, coordinator, baseline, 4);
    pass &= Check(coordinator == camera::CoordinatorState::Gameplay &&
        Near(legitimateChangedFovOutput,
            camera::HorPlus(80.0f, kAspect, kNativeAspect)),
        "later_native_gameplay_fov_change_still_transforms");
    pass &= Check(Near(baseline.Read().nativeFov.value, 80.0f),
        "later_native_change_updates_gameplay_baseline");

    pass &= CheckAlreadyNativeRecovery(90.0f, kAspect);
    pass &= CheckAlreadyNativeRecovery(90.6557f, 3.0f);
    pass &= CheckAlreadyNativeRecovery(110.0f, 48.0f / 9.0f);
    pass &= CheckAlreadyNativeRecovery(70.0f, kNativeAspect);
    pass &= CheckRecoveryEvidenceGates();
    pass &= TestRecordedRecoveryInterpolationRemainsInHorPlusSpace();

    std::cout << "horplus_post_exit_recovery=" << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
