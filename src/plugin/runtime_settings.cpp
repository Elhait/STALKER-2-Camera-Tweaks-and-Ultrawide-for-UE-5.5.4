#include "runtime_internal.hpp"

#include "../config/feature_config.hpp"
#include "../localization/locale_registry.hpp"

namespace plugin::detail
{
    bool RuntimeHotkeyBinding(RuntimeSettingKind kind,
        config::HotkeyBindingId& binding) noexcept
    {
        switch (kind) {
        case RuntimeSettingKind::GameplayCycleKey:
            binding = config::HotkeyBindingId::GameplayMode; return true;
        case RuntimeSettingKind::CinematicCycleKey:
            binding = config::HotkeyBindingId::CinematicAspect; return true;
        case RuntimeSettingKind::CinematicFovCycleKey:
            binding = config::HotkeyBindingId::CinematicFov; return true;
        case RuntimeSettingKind::DialogueCycleKey:
            binding = config::HotkeyBindingId::DialogueZoom; return true;
        case RuntimeSettingKind::OverlayToggleKey:
            binding = config::HotkeyBindingId::OverlayToggle; return true;
        default: return false;
        }
    }

    bool PersistRuntimeSetting(const RuntimeSettingMutation& mutation,
        const std::filesystem::path& configPath, std::string_view currentLocaleCode,
        const config::LogFunction& log)
    {
        switch (mutation.kind) {
        case RuntimeSettingKind::GameplayEnabled: {
            const auto* value = std::get_if<bool>(&mutation.value);
            return value && config::PersistConfigValue(configPath, "Gameplay", "Enabled",
                *value ? "true" : "false", log);
        }
        case RuntimeSettingKind::GameplayMode: {
            const auto* value = std::get_if<config::GameplayMode>(&mutation.value);
            return value && config::PersistConfigValue(configPath, "Gameplay", "Mode",
                config::GameplayModeName(*value), log);
        }
        case RuntimeSettingKind::CinematicsEnabled: {
            const auto* value = std::get_if<bool>(&mutation.value);
            return value && config::PersistConfigValue(configPath, "Cinematics", "Enabled",
                *value ? "true" : "false", log);
        }
        case RuntimeSettingKind::CinematicAspectPolicy: {
            const auto* value = std::get_if<config::CinematicAspectPolicy>(&mutation.value);
            return value && config::PersistConfigValue(configPath, "Cinematics", "Mode",
                config::CinematicAspectPolicyName(*value), log);
        }
        case RuntimeSettingKind::CinematicFovMode: {
            const auto* value = std::get_if<config::CinematicFovMode>(&mutation.value);
            return value && config::PersistConfigValue(configPath, "Cinematics", "FovMode",
                config::CinematicFovModeName(*value), log);
        }
        case RuntimeSettingKind::DialogueEnabled: {
            const auto* value = std::get_if<bool>(&mutation.value);
            return value && config::PersistConfigValue(configPath, "Dialogue", "Enabled",
                *value ? "true" : "false", log);
        }
        case RuntimeSettingKind::DialogueZoomPolicy: {
            const auto* value = std::get_if<config::DialogueZoomPolicy>(&mutation.value);
            return value && *value != config::DialogueZoomPolicy::Native &&
                config::PersistConfigValue(configPath, "Dialogue", "Mode",
                config::DialogueZoomPolicyName(*value), log);
        }
        case RuntimeSettingKind::OverlayLocaleCode: {
            const auto* value = std::get_if<std::string>(&mutation.value);
            const auto* descriptor = value
                ? localization::FindLocaleDescriptor(*value) : nullptr;
            if (!descriptor) return false;
            return config::PersistConfigValues(configPath,
                {{"Overlay", "Language", *value},
                 {"Overlay", "FontSize", std::to_string(descriptor->initialFontSize)}},
                log);
        }
        case RuntimeSettingKind::OverlayAutoLocale:
            return config::PersistConfigValue(configPath, "Overlay", "Language",
                std::get<bool>(mutation.value) ? std::string("Auto") :
                    std::string(currentLocaleCode), log);
        case RuntimeSettingKind::OverlayFontSize: {
            const auto* value = std::get_if<int>(&mutation.value);
            if (!value || *value < config::OverlayFontSizeMin ||
                *value > config::OverlayFontSizeMax) return false;
            const auto valueText = std::to_string(*value);
            return config::PersistConfigValue(configPath, "Overlay", "FontSize",
                valueText, log);
        }
        case RuntimeSettingKind::HotkeysEnabled: {
            const auto* value = std::get_if<bool>(&mutation.value);
            return value && config::PersistConfigValue(configPath, "Hotkeys", "Enabled",
                *value ? "true" : "false", log);
        }
        case RuntimeSettingKind::GameplayCycleKey:
        case RuntimeSettingKind::CinematicCycleKey:
        case RuntimeSettingKind::CinematicFovCycleKey:
        case RuntimeSettingKind::DialogueCycleKey:
        case RuntimeSettingKind::OverlayToggleKey: {
            const auto* value = std::get_if<int>(&mutation.value);
            if (!value) return false;
            config::HotkeyBindingId binding{};
            if (!RuntimeHotkeyBinding(mutation.kind, binding)) return false;
            const auto* descriptor = config::FindHotkeyBinding(binding);
            return descriptor && config::PersistConfigValue(configPath,
                descriptor->section, descriptor->configKey,
                config::HotkeyConfigValue(*value), log);
        }
        }
        return false;
    }
}
