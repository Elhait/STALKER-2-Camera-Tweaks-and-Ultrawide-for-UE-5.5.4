#include "../../src/gameplay/gameplay_state.hpp"

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
