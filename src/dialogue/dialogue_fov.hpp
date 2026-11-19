#pragma once

#include <cstdint>

namespace config
{
    enum class DialogueZoomPolicy : std::uint32_t;
}

namespace dialogue
{
    float TransformProjectionSample(float incoming, float baseline, float target);
    float TransformProjectionSample(float incoming, float baseline,
        float nativeTarget, float target);
    float TransformExitSample(float incoming, float baseline, float target, float exitStart);
    float MapProjectionTransitionSample(float incoming, float nativeStart,
        float nativeTarget, float outputStart, float outputTarget) noexcept;
    float ResolveDialogueDestinationTarget(config::DialogueZoomPolicy policy,
        float baseline, float nativeTarget) noexcept;
    float AdaptiveTarget(float baseline);
    float ReducedTarget(float baseline);
}
