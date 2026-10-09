#include "../../src/camera/gameplay_baseline.hpp"
#include "../../src/camera/horplus.hpp"
#include "../../src/cinematics/cinematic_fov.hpp"
#include "../../src/cinematics/cinematic_recovery_retirement.hpp"
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
    constexpr float kRecoveryEndpointToleranceDegrees = 0.01f;
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
            source, inputFov, kRecoveryEndpointToleranceDegrees);
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
        sample.policyAspect = aspect;
        sample.policyAspectValid = std::isfinite(aspect) && aspect > 0.0f;
        sample.flags = flags;
        sample.cameraReadable = cameraReadable;
        sample.gameplayEnabled = gameplayEnabled;
        auto action = gameplay::ResolveHorPlusRecoveryAction(coordinator,
            sample, gameplay::IsNativeHorPlusRecoverySample(
                sample, kRecoveryEndpointToleranceDegrees),
            kRecoveryEndpointToleranceDegrees);
        if (action == gameplay::HorPlusRecoveryAction::HoldNativePassThrough) {
            if (observation == dialogue::PostCinematicRecoveryExclusion::Observation::Inactive ||
                observation == dialogue::PostCinematicRecoveryExclusion::Observation::Cancelled) {
                recovery.Arm(exitTarget);
                recovery.ObserveValidated(source, inputFov,
                    kRecoveryEndpointToleranceDegrees);
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
        native.inputFov = 90.0f;
        native.exitNativeTarget = 90.0f;
        native.aspect = 3.0f;
        native.flags = 0x4;
        native.cameraReadable = true;
        pass &= Check(gameplay::IsNativeHorPlusRecoverySample(
                native, kRecoveryEndpointToleranceDegrees),
            "recorded_native_recovery_evidence_accepted");
        const auto reject = [&pass](const gameplay::HorPlusRecoverySample& sample,
            const char* name) {
            pass &= Check(!gameplay::IsNativeHorPlusRecoverySample(
                    sample, kRecoveryEndpointToleranceDegrees), name);
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
        sample = native;
        sample.inputFov = 90.6557f;
        pass &= Check(!gameplay::IsNativeHorPlusRecoverySample(
                sample, kRecoveryEndpointToleranceDegrees),
            "subdegree_difference_alone_does_not_complete_recovery_lifecycle");
        sample.inputFov = 91.01f;
        pass &= Check(!gameplay::IsNativeHorPlusRecoverySample(
                sample, kRecoveryEndpointToleranceDegrees),
            "native_recovery_requires_more_than_one_degree_to_remain_unconverged");

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

    bool CheckGenerationScopedRecoveryOwner()
    {
        bool pass = true;
        constexpr std::uintptr_t sourceA = 0xA100;
        constexpr std::uintptr_t sourceB = 0xB200;
        cinematics::CinematicWriterAssociationSnapshot association{};
        association.generationActive = true;
        association.generation = 41;
        association.aspectStoreTargetObject = sourceA;
        association.associatedWriterRsi = sourceA;
        association.observedWriterRbx = 0xC300;
        association.associationValid = true;

        cinematics::CinematicRecoveryOwner owner;
        pass &= Check(owner.Capture(association, 41) && owner.MatchesSource(sourceA),
            "accepted_exit_captures_the_generation_scoped_associated_rsi");
        auto snapshot = owner.Read();
        pass &= Check(snapshot.valid && snapshot.generation == 41 &&
            snapshot.writerRsi == sourceA && snapshot.observedWriterRbx == 0xC300 &&
            !snapshot.recoveryComplete,
            "recovery_owner_keeps_generation_rsi_and_rbx_metadata");

        const auto foreignSample = [&](float input, std::uint8_t flags,
            bool& wouldMutate, bool& wouldComplete) {
            wouldMutate = owner.MatchesSource(sourceB) && flags == 0x4;
            wouldComplete = wouldMutate && std::fabs(input - 90.0f) <=
                kRecoveryEndpointToleranceDegrees;
            return wouldMutate ? 106.688f : input;
        };
        bool bMutation = false;
        bool bCompletion = false;
        const float bDeparture = foreignSample(118.0f, 0x4, bMutation, bCompletion);
        pass &= Check(Near(bDeparture, 118.0f) && !bMutation && !bCompletion &&
            owner.MatchesSource(sourceA) && !owner.Read().recoveryComplete,
            "foreign_b_departure_is_pass_through_and_does_not_change_owner_or_progress");
        const float bEndpoint = foreignSample(90.0f, 0x4, bMutation, bCompletion);
        pass &= Check(Near(bEndpoint, 90.0f) && !bMutation && !bCompletion &&
            owner.MatchesSource(sourceA) && !owner.Read().recoveryComplete,
            "foreign_b_endpoint_cannot_complete_or_capture_horplus_recovery");

        gameplay::HorPlusRecoverySample aRecovery{};
        aRecovery.source = sourceA;
        aRecovery.validatedSource = owner.Read().writerRsi;
        aRecovery.inputFov = 90.0f;
        aRecovery.exitNativeTarget = 90.0f;
        aRecovery.cachedCinematicFov = 106.688f;
        aRecovery.aspect = kAspect;
        aRecovery.flags = 0x4;
        aRecovery.cameraReadable = true;
        aRecovery.gameplayEnabled = true;
        const auto aAction = gameplay::ResolveHorPlusRecoveryAction(
            camera::CoordinatorState::CinematicExiting, aRecovery,
            gameplay::IsNativeHorPlusRecoverySample(aRecovery,
                kRecoveryEndpointToleranceDegrees), kRecoveryEndpointToleranceDegrees);
        const float aOutput = owner.MatchesSource(sourceA) &&
            aAction == gameplay::HorPlusRecoveryAction::ResumeGameplay
            ? gameplay::EvaluateHorPlus(aRecovery.inputFov, aRecovery.aspect,
                aRecovery.flags, kNativeAspect).outputFov
            : aRecovery.inputFov;
        pass &= Check(aAction == gameplay::HorPlusRecoveryAction::ResumeGameplay &&
            Near(aOutput, 106.688f) && owner.MarkComplete(41) &&
            owner.Read().recoveryComplete && !owner.MatchesSource(sourceA),
            "associated_a_mutates_to_gameplay_output_and_completes_after_foreign_b");

        owner.Clear();
        pass &= Check(!owner.Read().valid && !owner.MatchesSource(sourceA),
            "new_generation_or_terminal_clear_does_not_inherit_old_owner");
        pass &= Check(!owner.Capture(association, 42),
            "generation_mismatch_cannot_capture_recovery_owner");
        auto invalid = association;
        invalid.associationConflict = true;
        pass &= Check(!owner.Capture(invalid, 41),
            "conflicted_association_cannot_arm_recovery");
        invalid = association;
        invalid.associationValid = false;
        auto noOwnerCoordinator = camera::CoordinatorState::CinematicExiting;
        const float noOwnerInput = 94.0f;
        const float noOwnerOutput = owner.Capture(invalid, 41)
            ? 106.688f : noOwnerInput;
        if (!owner.Read().valid)
            noOwnerCoordinator = camera::CoordinatorState::Gameplay;
        pass &= Check(Near(noOwnerOutput, noOwnerInput) &&
            noOwnerCoordinator == camera::CoordinatorState::Gameplay &&
            !owner.Read().valid,
            "exit_without_association_passes_output_and_cannot_stick_exiting");
        invalid = association;
        invalid.associatedWriterRsi = sourceB;
        pass &= Check(!owner.Capture(invalid, 41),
            "association_source_target_conflict_fails_closed");

        // The same source gate used before AspectRecalculation endpoint and
        // atomic handoff paths must reject B without touching either output.
        constexpr float aspectEndpoint = 90.0f;
        bool aspectCompletion = owner.MatchesSource(sourceB) &&
            std::fabs(aspectEndpoint - 90.0f) <= kRecoveryEndpointToleranceDegrees;
        bool aspectMutation = owner.MatchesSource(sourceB) && aspectCompletion;
        const float aspectOutput = aspectMutation ? 3.0f : aspectEndpoint;
        pass &= Check(Near(aspectOutput, aspectEndpoint) && !aspectMutation &&
            !aspectCompletion && !owner.Read().valid,
            "foreign_b_aspect_endpoint_keeps_output_and_does_not_complete");
        bool atomicHandoffWrite = owner.MatchesSource(sourceB);
        const float atomicHandoffAspect = atomicHandoffWrite ? 1.7778f : 3.0f;
        pass &= Check(!atomicHandoffWrite && Near(atomicHandoffAspect, 3.0f),
            "foreign_b_cannot_capture_or_apply_atomic_aspect_handoff");
        return pass;
    }

    bool CheckCompletedRecoveryReleasesGameplayWriterGate()
    {
        constexpr std::uintptr_t sourceA = 0xA100;
        constexpr std::uintptr_t sourceB = 0xB200;
        constexpr std::uint64_t generation = 52;
        bool pass = true;

        cinematics::CinematicWriterAssociationSnapshot association{};
        association.generationActive = true;
        association.generation = generation;
        association.aspectStoreTargetObject = sourceA;
        association.associatedWriterRsi = sourceA;
        association.observedWriterRbx = 0xC300;
        association.associationValid = true;
        cinematics::CinematicRecoveryOwner owner;
        pass &= Check(owner.Capture(association, generation),
            "completion_gate_test_captures_owner_a");

        dialogue::PostCinematicRecoveryExclusion exclusion;
        exclusion.Arm(90.0f);
        const auto aObservation = exclusion.ObserveValidated(sourceA, 100.0f, 0.01f);
        pass &= Check(aObservation ==
                dialogue::PostCinematicRecoveryExclusion::Observation::Suppressed &&
            exclusion.IsActive() && owner.MatchesSource(sourceA),
            "active_recovery_a_keeps_independent_dialogue_exclusion_active");

        pass &= Check(owner.MarkComplete(generation),
            "recovery_completion_marks_owner_metadata");
        auto completedOwner = owner.Read();
        pass &= Check(completedOwner.valid && completedOwner.recoveryComplete &&
            !completedOwner.HasRecoveryAuthority() && !owner.MatchesSource(sourceA) &&
            exclusion.IsActive(),
            "completed_owner_retains_metadata_but_has_no_recovery_authority");

        constexpr float nativeInput = 90.6557f;
        const bool rejectedByStaleOwner =
            cinematics::ShouldRejectPostRecoveryForeignWriter(
                completedOwner, exclusion.IsActive(), sourceB);
        const auto bExclusionObservation = exclusion.ObserveValidated(
            sourceB, nativeInput, 0.01f);
        if (completedOwner.valid && completedOwner.recoveryComplete &&
            bExclusionObservation !=
                dialogue::PostCinematicRecoveryExclusion::Observation::Suppressed) {
            owner.Clear();
            completedOwner = {};
        }
        const auto gameplayOutput = gameplay::EvaluateHorPlus(
            nativeInput, kAspect, 0x4, kNativeAspect);
        const float expectedGameplayOutput = camera::HorPlus(
            nativeInput, kAspect, kNativeAspect);
        const bool bAppliedGameplayCorrection = gameplayOutput.applied;
        pass &= Check(!rejectedByStaleOwner &&
            bExclusionObservation ==
                dialogue::PostCinematicRecoveryExclusion::Observation::Cancelled &&
            !exclusion.IsActive() && !completedOwner.valid &&
            bAppliedGameplayCorrection &&
            Near(gameplayOutput.outputFov, expectedGameplayOutput) &&
            !Near(gameplayOutput.outputFov, nativeInput),
            "writer_b_cancels_own_exclusion_context_then_gets_gameplay_output");
        pass &= Check(!owner.Read().valid &&
            !cinematics::ShouldRejectPostRecoveryForeignWriter(
                owner.Read(), exclusion.IsActive(), sourceB),
            "completed_owner_is_cleared_after_exclusion_source_mismatch");
        return pass;
    }

    bool CheckWriterChangeRetiresRecoveryBeforeGameplayDispatch()
    {
        bool pass = true;
        constexpr std::uintptr_t sourceA = 0xA100;
        constexpr std::uintptr_t outputA = 0xA200;
        constexpr std::uintptr_t sourceB = 0xB100;
        constexpr std::uintptr_t outputB = 0xB200;
        constexpr float nativeInput = 90.0f;

        cinematics::CinematicWriterAssociation observedWriter;
        static_cast<void>(observedWriter.ObserveWriter(
            camera::CoordinatorState::Gameplay, sourceA, outputA,
            nativeInput, kAspect, true, 0x4, true));
        cinematics::SharedWriterPairTransition sameOwnerTransition{};
        static_cast<void>(observedWriter.ObserveWriter(
            camera::CoordinatorState::CinematicExiting, sourceA, outputA,
            nativeInput, kAspect, true, 0x4, true, &sameOwnerTransition));

        cinematics::CinematicWriterAssociationSnapshot association{};
        association.generationActive = true;
        association.generation = 52;
        association.aspectStoreTargetObject = sourceA;
        association.associatedWriterRsi = sourceA;
        association.observedWriterRbx = outputA;
        association.associationValid = true;
        cinematics::CinematicRecoveryOwner owner;
        pass &= Check(owner.Capture(association, 52),
            "writer_change_test_captures_owner_a");

        auto coordinator = camera::CoordinatorState::CinematicExiting;
        const bool sameOwnerRetired = cinematics::RetireRecoveryForWriterRsiChange(
            coordinator, owner.Read(), sameOwnerTransition, [&] {
                owner.Clear();
                coordinator = camera::CoordinatorState::Gameplay;
            });
        pass &= Check(!sameOwnerTransition.changed && !sameOwnerRetired &&
            coordinator == camera::CoordinatorState::CinematicExiting &&
            owner.MatchesSource(sourceA),
            "same_owner_callback_keeps_recovery_a_active");

        cinematics::SharedWriterPairTransition transitionToB{};
        static_cast<void>(observedWriter.ObserveWriter(
            camera::CoordinatorState::CinematicExiting, sourceB, outputB,
            nativeInput, kAspect, true, 0x4, true, &transitionToB));
        pass &= Check(transitionToB.changed &&
            transitionToB.previous.source == sourceA &&
            transitionToB.current.source == sourceB,
            "shared_observer_reports_a_to_b_receiver_transition");

        struct RecoveryAuxiliaryState
        {
            bool continuationPending{};
            bool continuationInterpolationActive{};
            bool postCinematicExclusion{};
            bool atomicHandoffPending{};
            std::uintptr_t continuationSource{};
            std::uintptr_t gameplayBaselineContinuationSource{};
            float continuationExitNativeTarget{};
            float continuationPreviousTarget{};
            float continuationStartFov{};
            float exitNativeTarget{};
            float exitCinematicTarget{};
            std::uintptr_t atomicHandoffSource{};
            float atomicHandoffPreviousFov{};
            bool postExitTraceArmed{};
            std::uint32_t postExitTraceWriterCount{};
            std::uint64_t postExitTraceStart{};
            bool dialogueLifecycleActive{};
        } auxiliary{ true, true, true, true, sourceA, sourceA, 90.0f,
            100.0f, 95.0f, 90.0f, 106.688f, sourceA, 90.5f, true, 3, 123, true };

        const bool retired = cinematics::RetireRecoveryForWriterRsiChange(
            coordinator, owner.Read(), transitionToB, [&] {
                owner.Clear();
                auxiliary.continuationPending = false;
                auxiliary.continuationInterpolationActive = false;
                auxiliary.postCinematicExclusion = false;
                auxiliary.atomicHandoffPending = false;
                auxiliary.continuationSource = 0;
                auxiliary.gameplayBaselineContinuationSource = 0;
                auxiliary.continuationExitNativeTarget =
                    std::numeric_limits<float>::quiet_NaN();
                auxiliary.continuationPreviousTarget =
                    std::numeric_limits<float>::quiet_NaN();
                auxiliary.continuationStartFov =
                    std::numeric_limits<float>::quiet_NaN();
                auxiliary.exitNativeTarget = std::numeric_limits<float>::quiet_NaN();
                auxiliary.exitCinematicTarget = std::numeric_limits<float>::quiet_NaN();
                auxiliary.atomicHandoffSource = 0;
                auxiliary.atomicHandoffPreviousFov =
                    std::numeric_limits<float>::quiet_NaN();
                auxiliary.postExitTraceArmed = false;
                auxiliary.postExitTraceWriterCount = 0;
                auxiliary.postExitTraceStart = 0;
                coordinator = camera::CoordinatorState::Gameplay;
            });

        // This models the same writer callback continuing directly into the
        // regular Gameplay policy after retirement; no retry is needed.
        const auto gameplayTransform = gameplay::EvaluateHorPlus(
            nativeInput, kAspect, 0x4, kNativeAspect);
        const float outputAfterRetirement = retired &&
                coordinator == camera::CoordinatorState::Gameplay
            ? gameplayTransform.outputFov : nativeInput;
        pass &= Check(retired && coordinator == camera::CoordinatorState::Gameplay &&
            !owner.Read().valid && !owner.MatchesSource(sourceB) &&
            Near(outputAfterRetirement, gameplayTransform.outputFov) &&
            Near(outputAfterRetirement, 106.688f),
            "retirement_precedes_same_callback_gameplay_horplus_output_without_rebinding_b");
        pass &= Check(!auxiliary.continuationPending &&
            !auxiliary.continuationInterpolationActive &&
            !auxiliary.postCinematicExclusion && !auxiliary.atomicHandoffPending &&
            auxiliary.continuationSource == 0 &&
            auxiliary.gameplayBaselineContinuationSource == 0 &&
            !std::isfinite(auxiliary.continuationExitNativeTarget) &&
            !std::isfinite(auxiliary.continuationPreviousTarget) &&
            !std::isfinite(auxiliary.continuationStartFov) &&
            !std::isfinite(auxiliary.exitNativeTarget) &&
            !std::isfinite(auxiliary.exitCinematicTarget) &&
            auxiliary.atomicHandoffSource == 0 &&
            !std::isfinite(auxiliary.atomicHandoffPreviousFov) &&
            !auxiliary.postExitTraceArmed && auxiliary.postExitTraceWriterCount == 0 &&
            auxiliary.postExitTraceStart == 0 &&
            auxiliary.dialogueLifecycleActive,
            "retirement_clears_all_recovery_continuation_exclusion_and_handoff_state");

        // The retirement decision is independent of Gameplay.Enabled; the OFF
        // branch still retires stale owner state but leaves this callback native.
        cinematics::CinematicRecoveryOwner disabledOwner;
        pass &= Check(disabledOwner.Capture(association, 52),
            "gameplay_off_case_captures_owner_a");
        auto disabledCoordinator = camera::CoordinatorState::CinematicExiting;
        bool gameplayEnabled = false;
        const bool diagnosticsEnabled = false;
        bool retirementEventEmitted = false;
        const bool disabledRetired = cinematics::RetireRecoveryForWriterRsiChange(
            disabledCoordinator, disabledOwner.Read(), transitionToB, [&] {
                disabledOwner.Clear();
                disabledCoordinator = camera::CoordinatorState::Gameplay;
                if (diagnosticsEnabled) retirementEventEmitted = true;
            });
        const float disabledOutput = disabledRetired && gameplayEnabled
            ? gameplayTransform.outputFov : nativeInput;
        pass &= Check(disabledRetired &&
            disabledCoordinator == camera::CoordinatorState::Gameplay &&
            !disabledOwner.Read().valid && Near(disabledOutput, nativeInput) &&
            !retirementEventEmitted,
            "gameplay_off_and_diagnostics_off_retire_without_correction_or_event");

        cinematics::SharedWriterPairTransition rbxOnly{};
        static_cast<void>(observedWriter.ObserveWriter(
            camera::CoordinatorState::CinematicExiting, sourceA, outputA,
            nativeInput, kAspect, true, 0x4, true));
        static_cast<void>(observedWriter.ObserveWriter(
            camera::CoordinatorState::CinematicExiting, sourceA, 0xA300,
            nativeInput, kAspect, true, 0x4, true, &rbxOnly));
        auto rbxOwner = cinematics::CinematicRecoveryOwner{};
        pass &= Check(rbxOwner.Capture(association, 52),
            "rbx_only_case_captures_owner_a");
        bool rbxRetired = false;
        rbxRetired = cinematics::RetireRecoveryForWriterRsiChange(
            camera::CoordinatorState::CinematicExiting, rbxOwner.Read(), rbxOnly,
            [&] { rbxOwner.Clear(); });
        pass &= Check(rbxOnly.changed && rbxOnly.previous.source == sourceA &&
            rbxOnly.current.source == sourceA && !rbxRetired &&
            rbxOwner.MatchesSource(sourceA),
            "rbx_only_change_does_not_retire_receiver_owned_recovery");

        bool noOwnerRetired = false;
        noOwnerRetired = cinematics::RetireRecoveryForWriterRsiChange(
            camera::CoordinatorState::CinematicExiting, {}, transitionToB,
            [&] { noOwnerRetired = true; });
        auto cinematicOwner = cinematics::CinematicRecoveryOwner{};
        pass &= Check(cinematicOwner.Capture(association, 52),
            "cinematic_active_case_captures_owner_a");
        bool activeCinematicRetired = false;
        activeCinematicRetired = cinematics::RetireRecoveryForWriterRsiChange(
            camera::CoordinatorState::CinematicActive, cinematicOwner.Read(),
            transitionToB, [&] {
                activeCinematicRetired = true;
                cinematicOwner.Clear();
            });
        pass &= Check(!noOwnerRetired && !activeCinematicRetired &&
            cinematicOwner.MatchesSource(sourceA),
            "no_owner_and_cinematic_active_writer_changes_do_not_retire");

        bool inactiveRecoveryRetired = false;
        inactiveRecoveryRetired = cinematics::RetireRecoveryForWriterRsiChange(
            camera::CoordinatorState::Gameplay, cinematicOwner.Read(), transitionToB,
            [&] { inactiveRecoveryRetired = true; });
        pass &= Check(!inactiveRecoveryRetired && cinematicOwner.MatchesSource(sourceA),
            "writer_change_without_active_recovery_has_no_f4_side_effect");

        auto mismatchedPrior = transitionToB;
        mismatchedPrior.previous.source = 0xC100;
        bool mismatchedPriorRetired = false;
        mismatchedPriorRetired = cinematics::RetireRecoveryForWriterRsiChange(
            camera::CoordinatorState::CinematicExiting, cinematicOwner.Read(),
            mismatchedPrior, [&] { mismatchedPriorRetired = true; });
        auto nullCurrent = transitionToB;
        nullCurrent.current.source = 0;
        bool nullCurrentRetired = false;
        nullCurrentRetired = cinematics::RetireRecoveryForWriterRsiChange(
            camera::CoordinatorState::CinematicExiting, cinematicOwner.Read(),
            nullCurrent, [&] { nullCurrentRetired = true; });
        pass &= Check(!mismatchedPriorRetired && !nullCurrentRetired &&
            cinematicOwner.MatchesSource(sourceA),
            "prior_source_mismatch_or_unknown_current_source_fails_closed");

        auto completedOwner = cinematics::CinematicRecoveryOwner{};
        pass &= Check(completedOwner.Capture(association, 52) &&
            completedOwner.MarkComplete(52),
            "completed_recovery_owner_setup");
        bool completedRetired = false;
        completedRetired = cinematics::RetireRecoveryForWriterRsiChange(
            camera::CoordinatorState::CinematicExiting, completedOwner.Read(),
            transitionToB, [&] { completedRetired = true; });
        pass &= Check(!completedRetired && completedOwner.Read().valid &&
            completedOwner.Read().recoveryComplete,
            "completed_owner_is_not_reclassified_as_active_recovery");
        return pass;
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
                kRecoveryEndpointToleranceDegrees) ==
                gameplay::HorPlusRecoveryAction::HoldNativePassThrough,
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
    pass &= CheckGenerationScopedRecoveryOwner();
    pass &= CheckCompletedRecoveryReleasesGameplayWriterGate();
    pass &= CheckWriterChangeRetiresRecoveryBeforeGameplayDispatch();
    pass &= TestRecordedRecoveryInterpolationRemainsInHorPlusSpace();

    std::cout << "horplus_post_exit_recovery=" << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
