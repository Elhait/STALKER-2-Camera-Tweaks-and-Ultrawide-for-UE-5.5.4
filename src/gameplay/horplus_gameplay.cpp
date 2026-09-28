#include "horplus_gameplay.hpp"
#include "aspect_policy.hpp"
#include "gameplay_state.hpp"

#include "../camera/horplus.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace gameplay
{
    HorPlusTransformResult EvaluateHorPlus(float inputFov, float aspect,
        std::uint8_t flags, float nativeAspect)
    {
        HorPlusTransformResult result{ inputFov, inputFov, false, false };
        if (!std::isfinite(inputFov) || inputFov <= 1.0f || inputFov >= 179.0f ||
            !IsUltrawideAspect(aspect, nativeAspect) ||
            (flags != 0x4 && flags != 0x5)) return result;

        result.eligible = true;
        result.outputFov = camera::HorPlus(inputFov, aspect, nativeAspect);
        result.applied = std::isfinite(result.outputFov) &&
            result.outputFov > 1.0f && result.outputFov < 179.0f;
        if (!result.applied) result.outputFov = result.inputFov;
        return result;
    }

    bool TryTransformHorPlus(float inputFov, float aspect, std::uint8_t flags,
        float nativeAspect, float& transformedFov)
    {
        const auto result = EvaluateHorPlus(inputFov, aspect, flags, nativeAspect);
        transformedFov = result.outputFov;
        return result.applied;
    }

    float ResolveHorPlusRecoveryInterpolationFov(
        const HorPlusRecoverySample& sample, float nativeAspect) noexcept
    {
        const bool ownedGameplaySample = sample.gameplayEnabled &&
            sample.cameraReadable && sample.source != 0 &&
            sample.source == sample.validatedSource && sample.flags == 0x4 &&
            std::isfinite(sample.aspect) && sample.aspect > 0.0f &&
            std::isfinite(sample.inputFov) && sample.inputFov > 1.0f &&
            sample.inputFov < 179.0f && std::isfinite(sample.exitNativeTarget) &&
            sample.exitNativeTarget > 1.0f && sample.exitNativeTarget < 179.0f &&
            std::isfinite(sample.cachedCinematicFov) &&
            sample.cachedCinematicFov > 1.0f && sample.cachedCinematicFov < 179.0f;
        if (!ownedGameplaySample || !std::isfinite(nativeAspect) || nativeAspect <= 0.0f)
            return std::numeric_limits<float>::quiet_NaN();

        const float span = sample.cachedCinematicFov - sample.exitNativeTarget;
        if (!std::isfinite(span) || span == 0.0f)
            return std::numeric_limits<float>::quiet_NaN();
        const float lower = (std::min)(sample.cachedCinematicFov,
            sample.exitNativeTarget);
        const float upper = (std::max)(sample.cachedCinematicFov,
            sample.exitNativeTarget);
        if (sample.inputFov < lower || sample.inputFov > upper)
            return std::numeric_limits<float>::quiet_NaN();

        const float progress = std::clamp(
            (sample.cachedCinematicFov - sample.inputFov) / span, 0.0f, 1.0f);
        const auto gameplayTarget = EvaluateHorPlus(sample.exitNativeTarget,
            sample.aspect, 0x4, nativeAspect);
        if (!gameplayTarget.applied)
            return std::numeric_limits<float>::quiet_NaN();

        // The first endpoint is already in transformed cinematic space. Map
        // the validated native interpolation's progress between that endpoint
        // and the HorPlus gameplay target instead of transforming each raw
        // intermediate value again. Recovery stays pending, so this path never
        // publishes an intermediate sample as GameplayBaseline.
        const float output = sample.cachedCinematicFov +
            (gameplayTarget.outputFov - sample.cachedCinematicFov) * progress;
        return std::isfinite(output) && output > 1.0f && output < 179.0f
            ? output : std::numeric_limits<float>::quiet_NaN();
    }
}
