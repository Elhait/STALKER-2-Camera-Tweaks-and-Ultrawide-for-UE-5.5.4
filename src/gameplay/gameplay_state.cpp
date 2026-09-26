#include "gameplay_state.hpp"

#include <cmath>

namespace gameplay
{
    const char* ReplayStateName(ReplayState state)
    {
        switch (state) {
        case ReplayState::WaitingForAutomaticUpdate: return "WaitingForAutomaticUpdate";
        case ReplayState::AppliedConstrainPass: return "AppliedConstrainPass";
        case ReplayState::Complete: return "Complete";
        }
        return "Unknown";
    }

    CinematicExitTransition ResolveCinematicExitTransition(bool gameplayAvailable)
    {
        if (gameplayAvailable)
            return { camera::CoordinatorState::CinematicExiting, true };
        return { camera::CoordinatorState::Gameplay, false };
    }

    CinematicExitTransition ResolveCinematicExitTransition(
        bool gameplayAvailable, config::GameplayMode gameplayMode)
    {
        if (gameplayMode == config::GameplayMode::HorPlus)
            return { camera::CoordinatorState::Gameplay, false };
        return ResolveCinematicExitTransition(gameplayAvailable);
    }

    GameplayModeTransitionPlan ResolveGameplayModeTransition(
        config::GameplayMode oldMode, config::GameplayMode newMode,
        camera::CoordinatorState coordinator)
    {
        if (oldMode == newMode) return {};
        return {
            true,
            true,
            true,
            coordinator != camera::CoordinatorState::Gameplay
        };
    }

    GameplayEnabledAction ResolveGameplayEnabledTransition(
        const GameplayEnabledTransitionInput& input) noexcept
    {
        constexpr float nativeAspect = 16.0f / 9.0f;
        const auto isUltrawide = [nativeAspect](float aspect) noexcept {
            return std::isfinite(aspect) && aspect > nativeAspect + 0.001f;
        };
        const bool currentAspectValid = input.cameraReadable &&
            std::isfinite(input.currentAspect) && input.currentAspect > 0.0f;
        const bool currentAspectUltrawide = currentAspectValid &&
            isUltrawide(input.currentAspect);
        const bool restorationAspectValid = std::isfinite(input.restorationAspect) &&
            input.restorationAspect > 0.0f && input.restorationSourceMatches;
        const bool restorationAspectUltrawide = restorationAspectValid &&
            isUltrawide(input.restorationAspect);

        if (input.coordinator != camera::CoordinatorState::Gameplay)
            return GameplayEnabledAction::Defer;

        if (!input.enabled) {
            if (input.replayState == ReplayState::WaitingForAutomaticUpdate ||
                (input.replayState == ReplayState::Complete && currentAspectUltrawide))
                return GameplayEnabledAction::NoAction;
            if (!currentAspectValid || !restorationAspectValid ||
                !restorationAspectUltrawide)
                return GameplayEnabledAction::Defer;
            return GameplayEnabledAction::RestoreNativeAspect;
        }

        if (!currentAspectValid)
            return GameplayEnabledAction::Defer;

        if (input.selectedMode == config::GameplayMode::HorPlus) {
            if (input.replayState != ReplayState::WaitingForAutomaticUpdate &&
                !currentAspectUltrawide) {
                if (!restorationAspectValid || !restorationAspectUltrawide)
                    return GameplayEnabledAction::Defer;
                return GameplayEnabledAction::RestoreNativeAspect;
            }
            return GameplayEnabledAction::ApplyHorPlus;
        }

        if (input.replayState == ReplayState::Complete)
            return GameplayEnabledAction::AlreadyApplied;
        if (input.replayState == ReplayState::AppliedConstrainPass ||
            input.nativeTransitionReady)
            return GameplayEnabledAction::ApplyAspectRecalculation;
        return GameplayEnabledAction::Defer;
    }
}
