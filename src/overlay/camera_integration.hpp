#pragma once

#include "../plugin/runtime_settings.hpp"

namespace overlay
{
    enum class AspectAssessment
    {
        Matched,
        Mismatch,
        CannotAssess,
    };

    enum class FovPathAssessment
    {
        GameplayLinked,
        Independent,
        AuthoredFallback,
        CannotAssess,
    };

    enum class OverallIntegrationAssessment
    {
        Waiting,
        ConfigurationAligned,
        NotAligned,
        CannotAssess,
    };

    struct CameraIntegrationStatus
    {
        AspectAssessment aspect{AspectAssessment::CannotAssess};
        FovPathAssessment fovPath{FovPathAssessment::CannotAssess};
        OverallIntegrationAssessment overall{
            OverallIntegrationAssessment::CannotAssess};
        float gameplayFov{};
        float nativeFov{};
        float horPlusFov{};
        bool gameplayFovValid{};
        bool nativeFovValid{};
        bool horPlusFovValid{};
        float resolvedCinematicAspect{};
        bool resolvedCinematicAspectValid{};
    };

    CameraIntegrationStatus ProjectCameraIntegration(
        const plugin::OverlaySemanticSnapshot& snapshot,
        float aspectTolerance = 0.01f) noexcept;
}
