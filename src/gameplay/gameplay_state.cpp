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
        bool recoveryObserverAvailable, bool gameplayEnabled,
        config::GameplayMode gameplayMode)
    {
        if (gameplayMode == config::GameplayMode::HorPlus) {
            return { recoveryObserverAvailable
                    ? camera::CoordinatorState::CinematicExiting
                    : camera::CoordinatorState::Gameplay,
                false };
        }
        return ResolveCinematicExitTransition(recoveryObserverAvailable && gameplayEnabled);
    }

    bool IsNativeHorPlusRecoverySample(
        const HorPlusRecoverySample& sample, float epsilon) noexcept
    {
        // Gameplay may already be native on the first post-EXIT writer call.
        // Dialogue's depart-then-return sequence is not a Gameplay prerequisite.
        return sample.cameraReadable && sample.source != 0 &&
            sample.source == sample.validatedSource && sample.flags == 0x4 &&
            std::isfinite(sample.aspect) && sample.aspect > 0.0f &&
            std::isfinite(sample.inputFov) && sample.inputFov > 1.0f &&
            sample.inputFov < 179.0f &&
            std::isfinite(sample.exitNativeTarget) && sample.exitNativeTarget > 1.0f &&
            sample.exitNativeTarget < 179.0f &&
            std::isfinite(epsilon) && epsilon >= 0.0f &&
            std::fabs(sample.inputFov - sample.exitNativeTarget) <= epsilon;
    }

    namespace
    {
        bool HasValidatedGameplayRecoveryOwnership(
            const HorPlusRecoverySample& sample) noexcept
        {
            return sample.gameplayEnabled && sample.cameraReadable &&
                sample.source != 0 && sample.source == sample.validatedSource &&
                sample.flags == 0x4 && std::isfinite(sample.aspect) &&
                sample.aspect > 0.0f && std::isfinite(sample.inputFov) &&
                sample.inputFov > 1.0f && sample.inputFov < 179.0f &&
                std::isfinite(sample.exitNativeTarget) &&
                sample.exitNativeTarget > 1.0f && sample.exitNativeTarget < 179.0f;
        }

        bool IsRecoveryInterpolationSample(const HorPlusRecoverySample& sample,
            float epsilon) noexcept
        {
            if (!HasValidatedGameplayRecoveryOwnership(sample) ||
                !std::isfinite(sample.cachedCinematicFov) ||
                sample.cachedCinematicFov <= 1.0f ||
                sample.cachedCinematicFov >= 179.0f ||
                !std::isfinite(epsilon) || epsilon < 0.0f)
                return false;

            const float lower = (std::min)(sample.cachedCinematicFov,
                sample.exitNativeTarget) - epsilon;
            const float upper = (std::max)(sample.cachedCinematicFov,
                sample.exitNativeTarget) + epsilon;
            return std::fabs(sample.cachedCinematicFov -
                    sample.exitNativeTarget) > epsilon &&
                sample.inputFov >= lower && sample.inputFov <= upper;
        }
    }

    HorPlusRecoveryAction ResolveHorPlusRecoveryAction(
        camera::CoordinatorState coordinator, const HorPlusRecoverySample& sample,
        bool nativeRecoveryValidated, float epsilon) noexcept
    {
        if (coordinator != camera::CoordinatorState::CinematicExiting)
            return HorPlusRecoveryAction::NotWaiting;
        if (nativeRecoveryValidated && IsNativeHorPlusRecoverySample(sample, epsilon))
            return HorPlusRecoveryAction::ResumeGameplay;
        return IsRecoveryInterpolationSample(sample, 0.0f)
            ? HorPlusRecoveryAction::TransformRecoveryInterpolation
            : HorPlusRecoveryAction::HoldNativePassThrough;
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
