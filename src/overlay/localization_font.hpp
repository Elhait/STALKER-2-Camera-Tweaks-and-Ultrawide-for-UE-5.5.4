#pragma once

#include <Windows.h>
#include <imgui.h>

#include <span>
#include <string_view>

namespace overlay
{
    ImFont* AddLocalizationFont(int fontSizePixels,
        std::string_view profileCode) noexcept;
    bool AddLocalizationSelectorFonts(int fontSizePixels) noexcept;
    ImFont* LocalizationSelectorFont(std::string_view profileCode) noexcept;
    std::span<const std::string_view> LocalizationFontProfileCodes() noexcept;
}
