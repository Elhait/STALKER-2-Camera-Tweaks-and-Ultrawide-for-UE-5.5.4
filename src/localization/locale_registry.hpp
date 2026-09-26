#pragma once

#include "../overlay/localization_resource_ids.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace localization
{
    struct LocaleDescriptor
    {
        std::string_view displayName;
        std::string_view code;
        std::string_view catalogFileName;
        std::string_view fontProfileCode;
        int initialFontSize;
        std::uint16_t embeddedResourceId;
        bool canonical{};
    };

    inline constexpr std::string_view CanonicalLocaleCode{"en"};
    inline constexpr std::array LocaleRegistry{
        LocaleDescriptor{"English", "en", "en.json", "base", 13, IDR_LOCALE_001, true},
        LocaleDescriptor{"Українська", "uk", "uk.json", "base", 15, IDR_LOCALE_002, false},
        LocaleDescriptor{"العربية", "ar", "ar.json", "arabic", 20, IDR_LOCALE_003, false},
        LocaleDescriptor{"Čeština", "cs", "cs.json", "base", 13, IDR_LOCALE_004, false},
        LocaleDescriptor{"Français", "fr", "fr.json", "base", 13, IDR_LOCALE_005, false},
        LocaleDescriptor{"Deutsch", "de", "de.json", "base", 13, IDR_LOCALE_006, false},
        LocaleDescriptor{"Italiano", "it", "it.json", "base", 13, IDR_LOCALE_007, false},
        LocaleDescriptor{"日本語", "ja", "ja.json", "japanese", 18, IDR_LOCALE_008, false},
        LocaleDescriptor{"한국어", "ko", "ko.json", "korean", 18, IDR_LOCALE_009, false},
        LocaleDescriptor{"Polski", "pl", "pl.json", "base", 13, IDR_LOCALE_010, false},
        LocaleDescriptor{"Português (Brasil)", "pt-BR", "pt-BR.json", "base", 13, IDR_LOCALE_011, false},
        LocaleDescriptor{"Русский", "ru", "ru.json", "base", 15, IDR_LOCALE_012, false},
        LocaleDescriptor{"Српски (ћирилица)", "sr-Cyrl", "sr-Cyrl.json", "base", 15, IDR_LOCALE_013, false},
        LocaleDescriptor{"简体中文", "zh-Hans", "zh-Hans.json", "chinese-simplified", 18, IDR_LOCALE_014, false},
        LocaleDescriptor{"Español (Latinoamérica)", "es-419", "es-419.json", "base", 13, IDR_LOCALE_015, false},
        LocaleDescriptor{"Español (España)", "es-ES", "es-ES.json", "base", 13, IDR_LOCALE_016, false},
        LocaleDescriptor{"繁體中文", "zh-Hant", "zh-Hant.json", "chinese-traditional", 18, IDR_LOCALE_017, false},
        LocaleDescriptor{"Türkçe", "tr", "tr.json", "base", 13, IDR_LOCALE_018, false},
    };

    constexpr char FoldLocaleCodeAscii(char value) noexcept
    {
        return value >= 'A' && value <= 'Z'
            ? static_cast<char>(value + ('a' - 'A')) : value;
    }

    constexpr bool LocaleCodesEqual(std::string_view left,
        std::string_view right) noexcept
    {
        if (left.size() != right.size()) return false;
        for (std::size_t index = 0; index < left.size(); ++index)
            if (FoldLocaleCodeAscii(left[index]) != FoldLocaleCodeAscii(right[index]))
                return false;
        return true;
    }

    constexpr const LocaleDescriptor* FindLocaleDescriptor(
        std::string_view code) noexcept
    {
        for (const auto& descriptor : LocaleRegistry)
            if (LocaleCodesEqual(descriptor.code, code)) return &descriptor;
        return nullptr;
    }

    constexpr const LocaleDescriptor* CanonicalLocaleDescriptor() noexcept
    {
        for (const auto& descriptor : LocaleRegistry)
            if (descriptor.canonical) return &descriptor;
        return nullptr;
    }
}
