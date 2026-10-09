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
    struct CinematicReentrySample
    {
        camera::CoordinatorState previousCoordinator{camera::CoordinatorState::Gameplay};
        std::uintptr_t previousSource{};
        std::uintptr_t currentSource{};
        float previousExitNativeTarget{std::numeric_limits<float>::quiet_NaN()};
        float previousTransformedTarget{std::numeric_limits<float>::quiet_NaN()};
        float authoredEnterFov{std::numeric_limits<float>::quiet_NaN()};
        float transformedTargetFov{std::numeric_limits<float>::quiet_NaN()};
        bool awaitingValidatedRecovery{};
        bool targetEstablished{};
    };

    bool IsCinematicOwnershipContinuation(const CinematicReentrySample& sample) noexcept;

    CinematicExitTransition ResolveCinematicExitTransition(bool gameplayAvailable);
    CinematicExitTransition ResolveCinematicExitTransition(
        bool recoveryObserverAvailable, bool gameplayEnabled,
        config::GameplayMode gameplayMode);

    enum class HorPlusRecoveryAction : std::uint8_t
    {
        NotWaiting,
        HoldNativePassThrough,
        TransformRecoveryInterpolation,
        ResumeGameplay,
    };

    struct HorPlusRecoverySample
    {
        std::uintptr_t source{};
        std::uintptr_t validatedSource{};
        float inputFov{std::numeric_limits<float>::quiet_NaN()};
        float exitNativeTarget{std::numeric_limits<float>::quiet_NaN()};
        float cachedCinematicFov{std::numeric_limits<float>::quiet_NaN()};
        float aspect{std::numeric_limits<float>::quiet_NaN()};
        float policyAspect{std::numeric_limits<float>::quiet_NaN()};
        bool policyAspectValid{};
        std::uint8_t flags{};
        bool cameraReadable{};
        bool gameplayEnabled{true};
    };

    bool IsNativeHorPlusRecoverySample(const HorPlusRecoverySample& sample,
        float endpointToleranceDegrees) noexcept;
    HorPlusRecoveryAction ResolveHorPlusRecoveryAction(
        camera::CoordinatorState coordinator, const HorPlusRecoverySample& sample,
        bool nativeRecoveryValidated, float endpointToleranceDegrees) noexcept;
    float ResolveHorPlusRecoveryInterpolationFov(
        const HorPlusRecoverySample& sample, float nativeAspect) noexcept;

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
        ApplyRuntimeCombine,
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
