#pragma once

#include "localization_manager.hpp"
#include "selector_tooltip_state.hpp"
#include "setting_tooltip_content.hpp"

#include <span>

namespace overlay
{
    void ShowSettingOptionsTooltip(const LocalizationManager& i18n,
        std::span<const setting_tooltip_content::Option> options,
        SelectorInteractionState selectorState);
}
