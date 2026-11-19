#include "dialogue_fov.hpp"
#include "../config/feature_config.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace dialogue
{
    float ResolveDialogueDestinationTarget(config::DialogueZoomPolicy policy,
        float baseline, float nativeTarget) noexcept
    {
        const auto validFov = [](float value) noexcept {
            return std::isfinite(value) && value > 1.0f && value < 179.0f;
        };
        switch (policy) {
        case config::DialogueZoomPolicy::Native:
            return validFov(nativeTarget) ? nativeTarget
                : std::numeric_limits<float>::quiet_NaN();
        case config::DialogueZoomPolicy::Adaptive:
            return validFov(baseline) ? AdaptiveTarget(baseline)
                : std::numeric_limits<float>::quiet_NaN();
        case config::DialogueZoomPolicy::Reduced:
            return validFov(baseline) ? ReducedTarget(baseline)
                : std::numeric_limits<float>::quiet_NaN();
        case config::DialogueZoomPolicy::Disabled:
            return validFov(baseline) ? baseline
                : std::numeric_limits<float>::quiet_NaN();
        }
        return std::numeric_limits<float>::quiet_NaN();
    }

    float TransformProjectionSample(float incoming, float baseline, float target)
    {
        return TransformProjectionSample(incoming, baseline, 70.0f, target);
    }

    float TransformProjectionSample(float incoming, float baseline,
        float nativeDialogueTarget, float target)
    {
        constexpr float projectionIntervalTolerance = 0.25f;
        constexpr float radiansPerDegree = 0.01745329251994329577f;
        const float intervalLow = baseline < nativeDialogueTarget ? baseline : nativeDialogueTarget;
        const float intervalHigh = baseline > nativeDialogueTarget ? baseline : nativeDialogueTarget;
        if (incoming < intervalLow - projectionIntervalTolerance ||
            incoming > intervalHigh + projectionIntervalTolerance) return incoming;
        const float baselineProjection = std::tan(baseline * 0.5f * radiansPerDegree);
        const float nativeTargetProjection = std::tan(nativeDialogueTarget * 0.5f * radiansPerDegree);
        const float incomingProjection = std::tan(incoming * 0.5f * radiansPerDegree);
        const float targetProjection = std::tan(target * 0.5f * radiansPerDegree);
        const float nativeSpan = nativeTargetProjection - baselineProjection;
        if (!std::isfinite(nativeSpan) || std::fabs(nativeSpan) <= 1.0e-6f) return incoming;
        const float progress = std::clamp(
            (incomingProjection - baselineProjection) / nativeSpan, 0.0f, 1.0f);
        const float outputProjection = baselineProjection +
            progress * (targetProjection - baselineProjection);
        const float output = 2.0f * std::atan(outputProjection) / radiansPerDegree;
        return std::isfinite(output) ? output : incoming;
    }

    float TransformExitSample(float incoming, float baseline, float target, float exitStart)
    {
        constexpr float radiansPerDegree = 0.01745329251994329577f;
        const float startProjection = std::tan(exitStart * 0.5f * radiansPerDegree);
        const float baselineProjection = std::tan(baseline * 0.5f * radiansPerDegree);
        const float incomingProjection = std::tan(incoming * 0.5f * radiansPerDegree);
        const float targetProjection = std::tan(target * 0.5f * radiansPerDegree);
        const float nativeSpan = baselineProjection - startProjection;
        if (!std::isfinite(nativeSpan) || std::fabs(nativeSpan) <= 1.0e-6f) return incoming;
        const float progress = std::clamp(
            (incomingProjection - startProjection) / nativeSpan, 0.0f, 1.0f);
        const float outputProjection = targetProjection +
            progress * (baselineProjection - targetProjection);
        const float output = 2.0f * std::atan(outputProjection) / radiansPerDegree;
        return std::isfinite(output) ? output : incoming;
    }

    float MapProjectionTransitionSample(float incoming, float nativeStart,
        float nativeTarget, float outputStart, float outputTarget) noexcept
    {
        constexpr float radiansPerDegree = 0.01745329251994329577f;
        const float values[]{incoming, nativeStart, nativeTarget, outputStart, outputTarget};
        for (const float value : values)
            if (!std::isfinite(value) || value <= 1.0f || value >= 179.0f)
                return std::numeric_limits<float>::quiet_NaN();

        const float sourceStart = std::tan(nativeStart * 0.5f * radiansPerDegree);
        const float sourceTarget = std::tan(nativeTarget * 0.5f * radiansPerDegree);
        const float sourceSample = std::tan(incoming * 0.5f * radiansPerDegree);
        const float outputStartProjection = std::tan(outputStart * 0.5f * radiansPerDegree);
        const float outputTargetProjection = std::tan(outputTarget * 0.5f * radiansPerDegree);
        const float sourceSpan = sourceTarget - sourceStart;
        if (!std::isfinite(sourceSpan) || std::fabs(sourceSpan) <= 1.0e-6f ||
            !std::isfinite(sourceSample) ||
            !std::isfinite(outputStartProjection) ||
            !std::isfinite(outputTargetProjection))
            return std::numeric_limits<float>::quiet_NaN();

        const float progress = (sourceSample - sourceStart) / sourceSpan;
        if (!std::isfinite(progress) || progress < -0.002f || progress > 1.002f)
            return std::numeric_limits<float>::quiet_NaN();
        const float outputProjection = outputStartProjection +
            std::clamp(progress, 0.0f, 1.0f) *
                (outputTargetProjection - outputStartProjection);
        const float output = 2.0f * std::atan(outputProjection) / radiansPerDegree;
        return std::isfinite(output) && output > 1.0f && output < 179.0f
            ? output : std::numeric_limits<float>::quiet_NaN();
    }

    float AdaptiveTarget(float baseline)
    {
        constexpr float nativeDialogueTarget = 70.0f;
        constexpr float nativeReferenceGameplay = 90.0f;
        constexpr float radiansPerDegree = 0.01745329251994329577f;
        const float nativeZoom =
            std::tan(nativeReferenceGameplay * 0.5f * radiansPerDegree) /
            std::tan(nativeDialogueTarget * 0.5f * radiansPerDegree);
        const float projection = std::tan(baseline * 0.5f * radiansPerDegree) / nativeZoom;
        const float output = 2.0f * std::atan(projection) / radiansPerDegree;
        return std::isfinite(output) ? output : baseline;
    }

    float ReducedTarget(float baseline)
    {
        constexpr float nativeDialogueTarget = 70.0f;
        constexpr float nativeReferenceGameplay = 90.0f;
        constexpr float radiansPerDegree = 0.01745329251994329577f;
        const float nativeZoom =
            std::tan(nativeReferenceGameplay * 0.5f * radiansPerDegree) /
            std::tan(nativeDialogueTarget * 0.5f * radiansPerDegree);
        const float projection = std::tan(baseline * 0.5f * radiansPerDegree) /
            std::sqrt(nativeZoom);
        const float output = 2.0f * std::atan(projection) / radiansPerDegree;
        return std::isfinite(output) ? output : baseline;
    }
}
