#pragma once

#include <Windows.h>
#include "../localization/locale_registry.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace config
{
    inline constexpr int OverlayFontSizeMin = 12;
    inline constexpr int OverlayFontSizeMax = 24;
    inline constexpr int OverlayFontSizeDefault =
        localization::CanonicalLocaleDescriptor()->initialFontSize;

    enum class GameplayMode : std::uint32_t
    {
        AspectRecalculation,
        HorPlus,
    };

    enum class CinematicAspectPolicy : std::uint32_t
    {
        Auto,
        Native,
        Forced16x9,
        Forced21x9,
        Forced32x9,
    };

    enum class CinematicFovMode : std::uint32_t
    {
        NativeHorPlus,
        GameplayHorPlus,
    };

    enum class DialogueZoomPolicy : std::uint32_t
    {
        Native,
        Adaptive,
        Reduced,
        Disabled,
    };

    enum class HotkeyBindingId : std::uint8_t
    {
        GameplayMode,
        CinematicAspect,
        CinematicFov,
        DialogueZoom,
        OverlayToggle,
    };

    struct HotkeyBindingDescriptor
    {
        HotkeyBindingId id;
        const char* section;
        const char* configKey;
        int defaultVirtualKey;
    };

    inline constexpr std::array<HotkeyBindingDescriptor, 5> HotkeyBindingRegistry{{
        {HotkeyBindingId::OverlayToggle, "Overlay", "ToggleKey", VK_DELETE},
        {HotkeyBindingId::GameplayMode, "Hotkeys", "GameplayCycle", VK_F9},
        {HotkeyBindingId::CinematicAspect, "Hotkeys", "CinematicCycle", VK_F10},
        {HotkeyBindingId::CinematicFov, "Hotkeys", "CinematicFovCycle", VK_F11},
        {HotkeyBindingId::DialogueZoom, "Hotkeys", "DialogueCycle", VK_F12},
    }};

    constexpr int DefaultHotkeyKey(HotkeyBindingId id) noexcept
    {
        for (const auto& descriptor : HotkeyBindingRegistry)
            if (descriptor.id == id) return descriptor.defaultVirtualKey;
        return 0;
    }

    struct FeatureConfig
    {
        bool gameplayEnabled{true};
        GameplayMode gameplayMode{GameplayMode::HorPlus};
        CinematicAspectPolicy cinematicAspectPolicy{CinematicAspectPolicy::Auto};
        bool cinematicAspectPolicyExplicit{};
        CinematicFovMode cinematicFovMode{CinematicFovMode::GameplayHorPlus};
        DialogueZoomPolicy dialogueZoomPolicy{DialogueZoomPolicy::Adaptive};
        bool diagnosticsEnabled{};
        bool hotkeysEnabled{false};
        std::string overlayLocaleCode{localization::CanonicalLocaleCode};
        bool overlayLocaleAuto{true};
        int overlayFontSize{OverlayFontSizeDefault};
        int overlayToggleKey{DefaultHotkeyKey(HotkeyBindingId::OverlayToggle)};
        int gameplayCycleKey{DefaultHotkeyKey(HotkeyBindingId::GameplayMode)};
        int cinematicCycleKey{DefaultHotkeyKey(HotkeyBindingId::CinematicAspect)};
        int cinematicFovCycleKey{DefaultHotkeyKey(HotkeyBindingId::CinematicFov)};
        int dialogueCycleKey{DefaultHotkeyKey(HotkeyBindingId::DialogueZoom)};
    };

    int HotkeyBindingValue(const FeatureConfig& config, HotkeyBindingId id) noexcept;
    void SetHotkeyBindingValue(FeatureConfig& config, HotkeyBindingId id,
        int value) noexcept;
    const HotkeyBindingDescriptor* FindHotkeyBinding(HotkeyBindingId id) noexcept;
    const HotkeyBindingDescriptor* FindHotkeyBinding(const char* section,
        const char* configKey) noexcept;

    std::string Trim(std::string value);
    bool ParseBool(std::string value, bool& result);
    bool ParseHotkey(std::string value, int& result);
    const char* HotkeyConfigValue(int key);
    bool IsSupportedHotkey(int key) noexcept;
    bool HasHotkeyConflict(const FeatureConfig& config, int key,
        HotkeyBindingId binding) noexcept;
    const char* HotkeyName(int key);
    bool ParseGameplayMode(std::string value, GameplayMode& result);
    const char* GameplayModeName(GameplayMode mode);
    bool ParseCinematicAspectPolicy(std::string value, CinematicAspectPolicy& result);
    const char* CinematicAspectPolicyName(CinematicAspectPolicy policy);
    bool ParseCinematicFovMode(std::string value, CinematicFovMode& result);
    const char* CinematicFovModeName(CinematicFovMode mode);
    bool ParseDialogueZoomPolicy(std::string value, DialogueZoomPolicy& result);
    const char* DialogueZoomPolicyName(DialogueZoomPolicy policy);
    bool ParseLocaleCode(std::string value, std::string& result);
    bool ParseOverlayFontSize(std::string value, int& result);
    DialogueZoomPolicy NextDialogueZoomPolicy(DialogueZoomPolicy policy);
    CinematicAspectPolicy NextCinematicAspectPolicy(CinematicAspectPolicy policy);
    CinematicFovMode NextCinematicFovMode(CinematicFovMode mode);
    GameplayMode NextGameplayMode(GameplayMode mode);
}
