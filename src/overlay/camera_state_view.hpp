#pragma once

#include "localization_manager.hpp"
#include "../plugin/runtime_settings.hpp"

namespace overlay
{
    void DrawCameraStateView(const LocalizationManager& i18n,
        const plugin::OverlaySemanticSnapshot& semantic);
}
