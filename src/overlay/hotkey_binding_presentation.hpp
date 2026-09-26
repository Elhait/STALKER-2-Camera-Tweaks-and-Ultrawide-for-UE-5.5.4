#pragma once

#include "../config/feature_config.hpp"
#include "../plugin/runtime_settings.hpp"
#include "localization_keys.hpp"

#include <array>

namespace overlay
{
    enum class HotkeyBindingGroup : unsigned char { Overlay, Hotkeys };

    struct HotkeyBindingPresentation
    {
        config::HotkeyBindingId id;
        loc::Key label;
        HotkeyBindingGroup group;
        int plugin::RuntimeSettingsSnapshot::*settingKey;
    };

    inline constexpr std::array<HotkeyBindingPresentation, 5>
        HotkeyBindingPresentations{{
            {config::HotkeyBindingId::GameplayMode, loc::setting::GameplayMode,
                HotkeyBindingGroup::Hotkeys,
                &plugin::RuntimeSettingsSnapshot::gameplayCycleKey},
            {config::HotkeyBindingId::CinematicAspect, loc::setting::CinematicAspect,
                HotkeyBindingGroup::Hotkeys,
                &plugin::RuntimeSettingsSnapshot::cinematicCycleKey},
            {config::HotkeyBindingId::CinematicFov, loc::setting::CinematicFov,
                HotkeyBindingGroup::Hotkeys,
                &plugin::RuntimeSettingsSnapshot::cinematicFovCycleKey},
            {config::HotkeyBindingId::DialogueZoom, loc::setting::DialogueZoom,
                HotkeyBindingGroup::Hotkeys,
                &plugin::RuntimeSettingsSnapshot::dialogueCycleKey},
            {config::HotkeyBindingId::OverlayToggle, loc::setting::OverlayToggle,
                HotkeyBindingGroup::Overlay,
                &plugin::RuntimeSettingsSnapshot::overlayToggleKey},
        }};

    constexpr const HotkeyBindingPresentation* FindHotkeyPresentation(
        config::HotkeyBindingId id) noexcept
    {
        for (const auto& presentation : HotkeyBindingPresentations)
            if (presentation.id == id) return &presentation;
        return nullptr;
    }
}
