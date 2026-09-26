#pragma once

#include <cstdint>
#include <limits>

#include "../config/feature_config.hpp"
#include "../camera/presentation_state.hpp"

namespace gameplay
{
    enum class ReplayState : std::uint32_t
    {
        WaitingForAutomaticUpdate,
        AppliedConstrainPass,
        Complete
    };

    struct CinematicExitTransition
    {
        camera::CoordinatorState nextState;
        bool armGameplayHandoff;
    };

    const char* ReplayStateName(ReplayState state);
    CinematicExitTransition ResolveCinematicExitTransition(bool gameplayAvailable);
    CinematicExitTransition ResolveCinematicExitTransition(
        bool gameplayAvailable, config::GameplayMode gameplayMode);

    struct GameplayModeTransitionPlan
    {
        bool changed{};
        bool resetAspectRecalculation{};
        bool invalidateHorPlusState{};
        bool deferPhysicalTransition{};
    };

    GameplayModeTransitionPlan ResolveGameplayModeTransition(
        config::GameplayMode oldMode, config::GameplayMode newMode,
        camera::CoordinatorState coordinator);

    enum class GameplayEnabledAction : std::uint8_t
    {
        NoAction,
        Defer,
        RestoreNativeAspect,
        ApplyHorPlus,
        ApplyAspectRecalculation,
        AlreadyApplied,
    };

    struct GameplayEnabledTransitionInput
    {
        bool enabled{};
        config::GameplayMode selectedMode{config::GameplayMode::HorPlus};
        camera::CoordinatorState coordinator{camera::CoordinatorState::Gameplay};
        ReplayState replayState{ReplayState::WaitingForAutomaticUpdate};
        bool cameraReadable{};
        float currentAspect{std::numeric_limits<float>::quiet_NaN()};
        bool nativeTransitionReady{};
        float restorationAspect{std::numeric_limits<float>::quiet_NaN()};
        bool restorationSourceMatches{};
    };

    GameplayEnabledAction ResolveGameplayEnabledTransition(
        const GameplayEnabledTransitionInput& input) noexcept;
}
