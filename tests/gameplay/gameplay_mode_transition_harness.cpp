#include "../../src/gameplay/gameplay_state.hpp"
#include "../../src/camera/horplus.hpp"
#include "../../src/cinematics/cinematic_selection.hpp"
#include "../../src/cinematics/cinematic_fov.hpp"
#include "../../src/camera/fov_semantics.hpp"

#include <iostream>
#include <limits>

namespace
{
    bool Check(bool condition, const char* name)
    {
        if (!condition) std::cerr << name << ": FAIL\n";
        return condition;
    }
}

int main()
{
    using config::GameplayMode;
    using camera::CoordinatorState;
    bool pass = true;

    constexpr std::uintptr_t cinematicSource = 0x1234;
    constexpr float cinematicEnterNativeFov = 90.0f;
    const float cinematicTargetFov = camera::HorPlus(
        cinematicEnterNativeFov, 32.0f / 9.0f, 16.0f / 9.0f);
    const gameplay::CinematicReentrySample transientReentry{
        CoordinatorState::CinematicExiting,
        cinematicSource,
        cinematicSource,
        cinematicEnterNativeFov + 0.6557f,
        cinematicTargetFov,
        cinematicEnterNativeFov + 0.6557f,
        cinematicTargetFov + 0.523f,
        true,
        true
    };
    pass &= Check(gameplay::IsCinematicOwnershipContinuation(transientReentry),
        "subdegree_native_and_transformed_variance_preserves_cinematic_ownership");
    pass &= Check(camera::AreFovValuesSemanticallyEquivalent(90.0f, 90.6557f) &&
        camera::AreFovValuesSemanticallyEquivalent(126.87f, 127.393f) &&
        !camera::AreFovValuesSemanticallyEquivalent(90.0f, 91.01f),
        "canonical_fov_semantic_tolerance_is_inclusive_one_degree");

    auto recoveredBeforeReentry = transientReentry;
    recoveredBeforeReentry.awaitingValidatedRecovery = false;
    pass &= Check(!gameplay::IsCinematicOwnershipContinuation(recoveredBeforeReentry),
        "validated_recovery_ends_cinematic_continuity");
    auto differentReentrySource = transientReentry;
    differentReentrySource.currentSource += 1;
    pass &= Check(!gameplay::IsCinematicOwnershipContinuation(differentReentrySource),
        "cinematic_continuity_requires_same_source");

    const cinematics::CinematicWriterSample baselineRewrite{
        cinematicSource, cinematicSource, cinematicEnterNativeFov,
        cinematicEnterNativeFov, cinematicTargetFov,
        std::numeric_limits<float>::quiet_NaN(), true, false, true };
    const auto retainedTarget = cinematics::ResolveCinematicWriterSample(baselineRewrite);
    pass &= Check(retainedTarget.action == cinematics::CinematicWriterAction::RetainTransformedTarget &&
        std::fabs(retainedTarget.outputFov - cinematicTargetFov) <= 0.001f,
        "cinematic_same_source_baseline_write_retains_horplus_target");

    auto nativeRecalculation = baselineRewrite;
    nativeRecalculation.inputFov = 90.6557f;
    const auto equivalentNative = cinematics::ResolveCinematicWriterSample(nativeRecalculation);
    pass &= Check(equivalentNative.action ==
            cinematics::CinematicWriterAction::RetainTransformedTarget &&
        std::fabs(equivalentNative.outputFov - cinematicTargetFov) <= 0.001f,
        "native_recalculation_within_one_degree_does_not_change_cinematic_intent");

    auto differentSourceSmallDelta = nativeRecalculation;
    differentSourceSmallDelta.source += 1;
    differentSourceSmallDelta.initialEstablishmentAuthorized = false;
    const auto unprovenSource = cinematics::ResolveCinematicWriterSample(
        differentSourceSmallDelta);
    pass &= Check(unprovenSource.action ==
            cinematics::CinematicWriterAction::PassThroughUnclassified &&
        unprovenSource.outputFov == differentSourceSmallDelta.inputFov,
        "fov_proximity_never_proves_cinematic_writer_source");

    // Duplicate ENTER acceptance/generation is covered through the shared
    // production lifecycle transaction in cinematic_lifecycle_transaction_harness.

    constexpr float continuationInterpolation[] = {
        90.0192f, 91.0837f, 92.4727f, 93.5113f, 94.6316f, 95.9674f,
        97.5641f, 99.2429f, 101.125f, 102.173f, 104.288f, 105.4f,
        106.664f, 107.866f, 109.035f, 110.449f, 111.517f, 112.634f,
        113.868f, 114.889f, 116.032f, 117.87f, 119.587f, 121.121f,
        122.399f, 123.582f, 124.615f, 125.731f, 126.755f
    };
    bool continuationRetainsTarget = true;
    for (const float sampleFov : continuationInterpolation) {
        const cinematics::CinematicWriterSample sample{
            cinematicSource, cinematicSource, sampleFov,
            cinematicEnterNativeFov, cinematicTargetFov,
            cinematicEnterNativeFov, true, true };
        const auto resolution = cinematics::ResolveCinematicWriterSample(sample);
        continuationRetainsTarget &=
            (resolution.action == cinematics::CinematicWriterAction::RetainContinuationTarget ||
                resolution.action == cinematics::CinematicWriterAction::RetainTransformedTarget) &&
            std::fabs(resolution.outputFov - cinematicTargetFov) <= 0.001f;
    }
    pass &= Check(continuationRetainsTarget,
        "transient_same_source_interpolation_keeps_cinematic_target_visible");

    const cinematics::CinematicWriterSample ordinaryInterpolation{
        cinematicSource, cinematicSource, 91.0837f,
        cinematicEnterNativeFov, cinematicTargetFov,
        std::numeric_limits<float>::quiet_NaN(), true, false };
    const auto ordinaryInterpolationResult =
        cinematics::ResolveCinematicWriterSample(ordinaryInterpolation);
    pass &= Check(ordinaryInterpolationResult.action ==
        cinematics::CinematicWriterAction::PassThroughInterpolation &&
        ordinaryInterpolationResult.outputFov == ordinaryInterpolation.inputFov,
        "ordinary_cinematic_interpolation_keeps_existing_passthrough");

    const cinematics::CinematicWriterSample mismatchedSource{
        cinematicSource + 1, cinematicSource, cinematicEnterNativeFov,
        cinematicEnterNativeFov, cinematicTargetFov,
        cinematicEnterNativeFov, true, true };
    const auto unownedBaseline = cinematics::ResolveCinematicWriterSample(mismatchedSource);
    pass &= Check(unownedBaseline.action ==
        cinematics::CinematicWriterAction::PassThroughUnclassified &&
        unownedBaseline.outputFov == cinematicEnterNativeFov,
        "cinematic_target_retention_requires_established_writer_source");

    constexpr config::DialogueZoomPolicy dialogueModes[] = {
        config::DialogueZoomPolicy::Adaptive,
        config::DialogueZoomPolicy::Reduced,
        config::DialogueZoomPolicy::Disabled
    };
    bool dialogueModeIndependent = true;
    for (const auto selectedDialogueMode : dialogueModes) {
        (void)selectedDialogueMode;
        const cinematics::CinematicWriterSample sample{
            cinematicSource, cinematicSource, 91.0837f,
            cinematicEnterNativeFov, cinematicTargetFov,
            cinematicEnterNativeFov, true, true };
        const auto resolution = cinematics::ResolveCinematicWriterSample(sample);
        dialogueModeIndependent &=
            resolution.action == cinematics::CinematicWriterAction::RetainContinuationTarget &&
            std::fabs(resolution.outputFov - cinematicTargetFov) <= 0.001f;
    }
    pass &= Check(dialogueModeIndependent,
        "native_adaptive_reduced_disabled_dialogue_modes_cannot_change_cinematic_fov");

    const auto startup = gameplay::ResolveGameplayModeTransition(
        GameplayMode::HorPlus, GameplayMode::HorPlus, CoordinatorState::Gameplay);
    pass &= Check(!startup.changed, "startup_same_mode_is_noop");
    pass &= Check(!startup.resetAspectRecalculation && !startup.invalidateHorPlusState,
        "startup_has_no_previous_mode_cleanup");

    const auto aspectToHorPlus = gameplay::ResolveGameplayModeTransition(
        GameplayMode::AspectRecalculation, GameplayMode::HorPlus,
        CoordinatorState::Gameplay);
    pass &= Check(aspectToHorPlus.changed, "aspect_to_horplus_changes");
    pass &= Check(aspectToHorPlus.resetAspectRecalculation,
        "aspect_to_horplus_resets_replay_contract");
    pass &= Check(aspectToHorPlus.invalidateHorPlusState,
        "aspect_to_horplus_invalidates_transition_state");
    pass &= Check(!aspectToHorPlus.deferPhysicalTransition,
        "gameplay_boundary_applies_immediately");

    const auto aspectToHorPlusCinematic = gameplay::ResolveGameplayModeTransition(
        GameplayMode::AspectRecalculation, GameplayMode::HorPlus,
        CoordinatorState::CinematicActive);
    pass &= Check(aspectToHorPlusCinematic.deferPhysicalTransition,
        "cinematic_transition_is_deferred");

    const auto horPlusToAspect = gameplay::ResolveGameplayModeTransition(
        GameplayMode::HorPlus, GameplayMode::AspectRecalculation,
        CoordinatorState::Gameplay);
    pass &= Check(horPlusToAspect.changed && horPlusToAspect.resetAspectRecalculation,
        "horplus_to_aspect_resets_replay_contract");
    pass &= Check(horPlusToAspect.invalidateHorPlusState,
        "horplus_to_aspect_invalidates_horplus_state");

    const auto noOp = gameplay::ResolveGameplayModeTransition(
        GameplayMode::AspectRecalculation, GameplayMode::AspectRecalculation,
        CoordinatorState::CinematicExiting);
    pass &= Check(!noOp.changed && !noOp.deferPhysicalTransition,
        "aspect_same_mode_is_noop");

    using gameplay::GameplayEnabledAction;
    const auto disableHorPlus = gameplay::ResolveGameplayEnabledTransition({
        false, GameplayMode::HorPlus, CoordinatorState::Gameplay,
        gameplay::ReplayState::WaitingForAutomaticUpdate, true, 1.77778f, false, 3.0f, true });
    pass &= Check(disableHorPlus == GameplayEnabledAction::NoAction,
        "disable_horplus_needs_no_camera_restore");
    const auto disableAspectRecalculation = gameplay::ResolveGameplayEnabledTransition({
        false, GameplayMode::AspectRecalculation, CoordinatorState::Gameplay,
        gameplay::ReplayState::Complete, true, 1.77778f, false, 3.0f, true });
    pass &= Check(disableAspectRecalculation == GameplayEnabledAction::RestoreNativeAspect,
        "disable_aspect_recalculation_restores_retained_native_aspect");
    const auto disableWithoutTarget = gameplay::ResolveGameplayEnabledTransition({
        false, GameplayMode::AspectRecalculation, CoordinatorState::Gameplay,
        gameplay::ReplayState::AppliedConstrainPass, true, 1.77778f, false,
        std::numeric_limits<float>::quiet_NaN(), true });
    pass &= Check(disableWithoutTarget == GameplayEnabledAction::Defer,
        "disable_fails_closed_without_native_target");
    const auto disablePartialAspectPass = gameplay::ResolveGameplayEnabledTransition({
        false, GameplayMode::AspectRecalculation, CoordinatorState::Gameplay,
        gameplay::ReplayState::AppliedConstrainPass, true, 2.4f, false, 3.0f, true });
    pass &= Check(disablePartialAspectPass == GameplayEnabledAction::RestoreNativeAspect,
        "disable_restores_partial_aspect_transition");
    const auto enableCustomHorPlus = gameplay::ResolveGameplayEnabledTransition({
        true, GameplayMode::HorPlus, CoordinatorState::Gameplay,
        gameplay::ReplayState::WaitingForAutomaticUpdate, true, 3.0f, false,
        std::numeric_limits<float>::quiet_NaN(), false });
    pass &= Check(enableCustomHorPlus == GameplayEnabledAction::ApplyHorPlus,
        "enable_horplus_accepts_custom_valid_aspect");
    const auto enableHorPlusRestoresFirst = gameplay::ResolveGameplayEnabledTransition({
        true, GameplayMode::HorPlus, CoordinatorState::Gameplay,
        gameplay::ReplayState::Complete, true, 1.77778f, false, 3.0f, true });
    pass &= Check(enableHorPlusRestoresFirst == GameplayEnabledAction::RestoreNativeAspect,
        "enable_horplus_restores_aspect_recalculation_state_first");
    const auto enableRejectsStaleSourceTarget = gameplay::ResolveGameplayEnabledTransition({
        true, GameplayMode::HorPlus, CoordinatorState::Gameplay,
        gameplay::ReplayState::Complete, true, 1.77778f, false, 3.0f, false });
    pass &= Check(enableRejectsStaleSourceTarget == GameplayEnabledAction::Defer,
        "enable_does_not_restore_aspect_from_stale_camera_source");
    const auto enableAspectRecalculation = gameplay::ResolveGameplayEnabledTransition({
        true, GameplayMode::AspectRecalculation, CoordinatorState::Gameplay,
        gameplay::ReplayState::WaitingForAutomaticUpdate, true, 3.0f, true,
        std::numeric_limits<float>::quiet_NaN(), false });
    pass &= Check(enableAspectRecalculation == GameplayEnabledAction::ApplyAspectRecalculation,
        "enable_aspect_recalculation_uses_native_transition");
    const auto enableInvalidAspect = gameplay::ResolveGameplayEnabledTransition({
        true, GameplayMode::AspectRecalculation, CoordinatorState::Gameplay,
        gameplay::ReplayState::WaitingForAutomaticUpdate, false,
        std::numeric_limits<float>::quiet_NaN(), false,
        std::numeric_limits<float>::quiet_NaN(), false });
    pass &= Check(enableInvalidAspect == GameplayEnabledAction::Defer,
        "enable_defers_without_valid_state");
    const auto enableDuringCinematic = gameplay::ResolveGameplayEnabledTransition({
        true, GameplayMode::HorPlus, CoordinatorState::CinematicActive,
        gameplay::ReplayState::WaitingForAutomaticUpdate, true, 2.4f, false,
        std::numeric_limits<float>::quiet_NaN(), false });
    pass &= Check(enableDuringCinematic == GameplayEnabledAction::Defer,
        "enable_waits_for_gameplay_boundary");
    const auto newestHorPlusAfterDisabledSelection = gameplay::ResolveGameplayEnabledTransition({
        true, GameplayMode::HorPlus, CoordinatorState::Gameplay,
        gameplay::ReplayState::WaitingForAutomaticUpdate, true, 2.4f, false, 2.4f, true });
    const auto newestAspectRecAfterDisabledSelection = gameplay::ResolveGameplayEnabledTransition({
        true, GameplayMode::AspectRecalculation, CoordinatorState::Gameplay,
        gameplay::ReplayState::WaitingForAutomaticUpdate, true, 2.4f, true, 2.4f, true });
    pass &= Check(newestHorPlusAfterDisabledSelection == GameplayEnabledAction::ApplyHorPlus &&
        newestAspectRecAfterDisabledSelection == GameplayEnabledAction::ApplyAspectRecalculation,
        "reenable_uses_latest_selected_mode");
    const auto disabledSuppressesPendingHorPlus = gameplay::ResolveGameplayEnabledTransition({
        false, GameplayMode::HorPlus, CoordinatorState::Gameplay,
        gameplay::ReplayState::WaitingForAutomaticUpdate, true, 2.4f, false, 2.4f, true });
    pass &= Check(disabledSuppressesPendingHorPlus == GameplayEnabledAction::NoAction,
        "disabled_state_suppresses_pending_mode_application");
    const auto repeatedDisableAfterRestore = gameplay::ResolveGameplayEnabledTransition({
        false, GameplayMode::AspectRecalculation, CoordinatorState::Gameplay,
        gameplay::ReplayState::WaitingForAutomaticUpdate, true, 2.4f, false, 2.4f, true });
    const auto repeatedEnableAfterRestore = gameplay::ResolveGameplayEnabledTransition({
        true, GameplayMode::AspectRecalculation, CoordinatorState::Gameplay,
        gameplay::ReplayState::WaitingForAutomaticUpdate, true, 2.4f, true, 2.4f, true });
    pass &= Check(repeatedDisableAfterRestore == GameplayEnabledAction::NoAction &&
        repeatedEnableAfterRestore == GameplayEnabledAction::ApplyAspectRecalculation,
        "repeated_enable_disable_uses_current_state");
    const auto enableNanAspect = gameplay::ResolveGameplayEnabledTransition({
        true, GameplayMode::HorPlus, CoordinatorState::Gameplay,
        gameplay::ReplayState::WaitingForAutomaticUpdate, true,
        std::numeric_limits<float>::quiet_NaN(), false, 3.0f, true });
    pass &= Check(enableNanAspect == GameplayEnabledAction::Defer,
        "enable_rejects_nan_aspect");
    const auto enableInfiniteAspect = gameplay::ResolveGameplayEnabledTransition({
        true, GameplayMode::HorPlus, CoordinatorState::Gameplay,
        gameplay::ReplayState::WaitingForAutomaticUpdate, true,
        std::numeric_limits<float>::infinity(), false, 3.0f, true });
    pass &= Check(enableInfiniteAspect == GameplayEnabledAction::Defer,
        "enable_rejects_infinite_aspect");

    std::cout << "gameplay_mode_transition=" << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
