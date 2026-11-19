#pragma once

#include "camera_integration.hpp"
#include "../../plugin/runtime_settings.hpp"

namespace overlay
{
    class LocalizationManager;
    void DrawCameraStateView(const plugin::OverlaySemanticSnapshot& semantic,
        const CameraIntegrationStatus& integration, const LocalizationManager& localization);
}
