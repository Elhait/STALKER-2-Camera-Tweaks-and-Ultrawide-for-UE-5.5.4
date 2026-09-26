#include "camera_integration.hpp"
#include "../cinematics/cinematic_aspect.hpp"

#include <cmath>

namespace overlay
{
    namespace
    {
        bool IsUsableAspect(float aspect) noexcept
        {
            return std::isfinite(aspect) && aspect > 0.0f;
        }

        bool ResolveConfiguredAspect(
            config::CinematicAspectPolicy policy, float viewportAspect,
            float& resolved) noexcept
        {
            switch (policy) {
            case config::CinematicAspectPolicy::Auto:
                if (!IsUsableAspect(viewportAspect)) return false;
                resolved = viewportAspect;
                return true;
            case config::CinematicAspectPolicy::Native:
                return false;
            case config::CinematicAspectPolicy::Forced16x9:
                resolved = 16.0f / 9.0f;
                return true;
            case config::CinematicAspectPolicy::Forced21x9:
                resolved = cinematics::Forced21x9Aspect;
                return true;
            case config::CinematicAspectPolicy::Forced32x9:
                resolved = 32.0f / 9.0f;
                return true;
            }
            return false;
        }
    }

    CameraIntegrationStatus ProjectCameraIntegration(
        const plugin::OverlaySemanticSnapshot& snapshot,
        float aspectTolerance) noexcept
    {
        CameraIntegrationStatus result{};
        if (snapshot.cameraStateValid) {
            result.gameplayFov = snapshot.cameraState.evidence.retainedGameplayFov;
            result.nativeFov = snapshot.cameraState.evidence.nativeWriterFov;
            result.horPlusFov = snapshot.cameraState.evidence.transformedFov;
            result.gameplayFovValid = snapshot.cameraState.evidence.retainedGameplayFovValid;
            result.nativeFovValid = snapshot.cameraState.evidence.nativeWriterFovValid;
            result.horPlusFovValid = snapshot.cameraState.evidence.transformedFovValid;
        }
        const bool aspectPolicyValid = snapshot.cinematicAspectPolicy.valid;
        const bool cinematicAspectOverrideRequested = aspectPolicyValid &&
            snapshot.cinematicAspectPolicy.value != config::CinematicAspectPolicy::Native;
        const bool fovModeValid = snapshot.cinematicFovMode.valid;
        const bool gameplayConfigurationValid = snapshot.gameplayEnabled.valid &&
            snapshot.gameplayMode.valid;
        const bool viewportValid = snapshot.runtimeViewportAspect.valid &&
            IsUsableAspect(snapshot.runtimeViewportAspect.value);
        const float viewportAspect = viewportValid
            ? snapshot.runtimeViewportAspect.value : 0.0f;

        if (aspectPolicyValid && ResolveConfiguredAspect(
            snapshot.cinematicAspectPolicy.value,
            viewportAspect, result.resolvedCinematicAspect)) {
            result.resolvedCinematicAspectValid = true;
            if (viewportValid && std::isfinite(aspectTolerance) && aspectTolerance >= 0.0f) {
                result.aspect = std::fabs(result.resolvedCinematicAspect - viewportAspect) <=
                    aspectTolerance
                    ? AspectAssessment::Matched
                    : AspectAssessment::Mismatch;
            }
        }

        if (fovModeValid) {
            switch (snapshot.cinematicFovMode.value) {
            case config::CinematicFovMode::NativeHorPlus:
                result.fovPath = FovPathAssessment::Independent;
                break;
            case config::CinematicFovMode::GameplayHorPlus:
                if (!cinematicAspectOverrideRequested ||
                    !snapshot.cinematicFovLifecycleAvailable.valid ||
                    !snapshot.cinematicFovLifecycleAvailable.value) {
                    break;
                }
                if (snapshot.gameplayBaselineUsable.valid &&
                    !snapshot.gameplayBaselineUsable.value) {
                    result.fovPath = FovPathAssessment::AuthoredFallback;
                } else if (snapshot.gameplayBaselineUsable.valid &&
                    snapshot.gameplayBaselineUsable.value) {
                    result.fovPath = FovPathAssessment::GameplayLinked;
                }
                break;
            }
        }

        if (result.aspect == AspectAssessment::Mismatch ||
            result.fovPath == FovPathAssessment::Independent) {
            result.overall = OverallIntegrationAssessment::NotAligned;
        } else if (result.aspect == AspectAssessment::Matched &&
            result.fovPath == FovPathAssessment::AuthoredFallback &&
            gameplayConfigurationValid && snapshot.gameplayEnabled.value &&
            snapshot.gameplayMode.value == config::GameplayMode::HorPlus &&
            snapshot.cinematicAspectComponentAvailable.valid &&
            snapshot.cinematicAspectComponentAvailable.value &&
            snapshot.cinematicFovLifecycleAvailable.valid &&
            snapshot.cinematicFovLifecycleAvailable.value) {
            result.overall = OverallIntegrationAssessment::Waiting;
        } else if (result.fovPath == FovPathAssessment::AuthoredFallback) {
            result.overall = OverallIntegrationAssessment::NotAligned;
        } else if (result.aspect == AspectAssessment::Matched &&
            result.fovPath == FovPathAssessment::GameplayLinked &&
            snapshot.cinematicAspectComponentAvailable.valid &&
            snapshot.cinematicAspectComponentAvailable.value &&
            gameplayConfigurationValid && snapshot.gameplayEnabled.value &&
            snapshot.gameplayMode.value == config::GameplayMode::HorPlus) {
            result.overall = OverallIntegrationAssessment::ConfigurationAligned;
        }
        return result;
    }
}
