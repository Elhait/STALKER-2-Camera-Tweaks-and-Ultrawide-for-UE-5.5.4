#pragma once

#include "runtime_settings.hpp"
#include "../config/config_repository.hpp"

#include <filesystem>
#include <string_view>

// Private integration boundary for plugin translation units. The concrete
// RuntimeState layout and its sole owner remain in runtime.cpp.
namespace plugin::detail
{
    struct RuntimeState;

    RuntimeState& GetRuntimeState() noexcept;

    bool RuntimeHotkeyBinding(RuntimeSettingKind kind,
        config::HotkeyBindingId& binding) noexcept;
    bool PersistHotkeyActionValue(const std::filesystem::path& configPath,
        const char* section, const char* key, const char* value,
        const config::LogFunction& log);
    bool PersistRuntimeSetting(const RuntimeSettingMutation& mutation,
        const std::filesystem::path& configPath, std::string_view currentLocaleCode,
        const config::LogFunction& log);
}
