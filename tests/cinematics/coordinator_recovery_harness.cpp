#include "../../src/gameplay/gameplay_state.hpp"

#include <iostream>

namespace
{
    bool Check(bool condition, const char* name)
    {
        if (!condition) std::cerr << name << ": FAIL\n";
        return condition;
    }

    bool CheckGameplayAvailable()
    {
        const auto transition = gameplay::ResolveCinematicExitTransition(true);
        return Check(transition.nextState == camera::CoordinatorState::CinematicExiting,
                "available_state") &&
            Check(transition.armGameplayHandoff, "available_handoff");
    }

    bool CheckGameplayDisabled()
    {
        const auto transition = gameplay::ResolveCinematicExitTransition(false);
        return Check(transition.nextState == camera::CoordinatorState::Gameplay,
                "disabled_state") &&
            Check(!transition.armGameplayHandoff, "disabled_handoff");
    }

    bool CheckGameplayInitializationFailure()
    {
        const auto transition = gameplay::ResolveCinematicExitTransition(false);
        return Check(transition.nextState == camera::CoordinatorState::Gameplay,
                "failed_state") &&
            Check(!transition.armGameplayHandoff, "failed_handoff");
    }

    bool CheckHorPlusMode()
    {
        const auto transition = gameplay::ResolveCinematicExitTransition(true, true,
            config::GameplayMode::HorPlus);
        const auto disabledTransition = gameplay::ResolveCinematicExitTransition(true, false,
            config::GameplayMode::HorPlus);
        const auto unavailableTransition = gameplay::ResolveCinematicExitTransition(false, true,
            config::GameplayMode::HorPlus);
        return Check(transition.nextState == camera::CoordinatorState::CinematicExiting,
                "horplus_state") &&
            Check(!transition.armGameplayHandoff, "horplus_handoff") &&
            Check(disabledTransition.nextState == camera::CoordinatorState::CinematicExiting &&
                !disabledTransition.armGameplayHandoff, "horplus_disabled_still_observes_recovery") &&
            Check(unavailableTransition.nextState == camera::CoordinatorState::Gameplay &&
                !unavailableTransition.armGameplayHandoff, "horplus_missing_observer_fails_open");
    }

    bool CheckHorPlusRecoveryActions()
    {
        gameplay::HorPlusRecoverySample sample{};
        sample.source = 1;
        sample.validatedSource = 1;
        sample.inputFov = 105.0f;
        sample.exitNativeTarget = 90.0f;
        sample.cachedCinematicFov = 106.688f;
        sample.aspect = 3440.0f / 1440.0f;
        sample.flags = 0x4;
        sample.cameraReadable = true;
        auto converged = sample;
        converged.inputFov = 90.0f;
        return Check(gameplay::ResolveHorPlusRecoveryAction(
                camera::CoordinatorState::CinematicExiting, sample, false, 0.01f) ==
                gameplay::HorPlusRecoveryAction::TransformRecoveryInterpolation,
                "validated_gameplay_interpolation_is_transformed_without_resuming") &&
            Check(gameplay::ResolveHorPlusRecoveryAction(
                camera::CoordinatorState::CinematicExiting, sample, true, 0.01f) ==
                gameplay::HorPlusRecoveryAction::TransformRecoveryInterpolation,
                "unconverged_sample_cannot_resume_even_with_external_recovery_flag") &&
            Check(gameplay::ResolveHorPlusRecoveryAction(
                camera::CoordinatorState::CinematicExiting, gameplay::HorPlusRecoverySample{}, false, 0.01f) ==
                gameplay::HorPlusRecoveryAction::HoldNativePassThrough,
                "ambiguous_transition_sample_remains_native_passthrough") &&
            Check(gameplay::ResolveHorPlusRecoveryAction(
                camera::CoordinatorState::CinematicExiting, converged, true, 0.01f) ==
                gameplay::HorPlusRecoveryAction::ResumeGameplay,
                "horplus_validated_recovery_resumes") &&
            Check(gameplay::ResolveHorPlusRecoveryAction(
                camera::CoordinatorState::Gameplay, sample, false, 0.01f) ==
                gameplay::HorPlusRecoveryAction::NotWaiting,
                "horplus_normal_gameplay_unchanged");
    }
}

int main()
{
    const bool available = CheckGameplayAvailable();
    const bool disabled = CheckGameplayDisabled();
    const bool failed = CheckGameplayInitializationFailure();
    const bool horPlus = CheckHorPlusMode();
    const bool recoveryActions = CheckHorPlusRecoveryActions();
    std::cout << "available=" << (available ? "PASS" : "FAIL")
        << " disabled=" << (disabled ? "PASS" : "FAIL")
        << " failed=" << (failed ? "PASS" : "FAIL")
        << " horplus=" << (horPlus ? "PASS" : "FAIL")
        << " recoveryActions=" << (recoveryActions ? "PASS" : "FAIL") << "\n";
    return available && disabled && failed && horPlus && recoveryActions ? 0 : 1;
}
