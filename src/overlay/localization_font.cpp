#include "localization_font.hpp"
#include "localization_resource_ids.h"
#include "../localization/locale_registry.hpp"

#include <array>
#include <algorithm>
#include <cstdint>
#include <string_view>

namespace overlay
{
    namespace
    {
        struct FontProfileDescriptor
        {
            std::string_view code;
            std::uint16_t resourceId;
            const ImWchar* glyphRanges;
        };

        HMODULE FontResourceModule() noexcept
        {
            HMODULE module{};
            return GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&FontResourceModule), &module)
                ? module : nullptr;
        }

        const ImWchar* CyrillicGlyphRanges() noexcept
        {
            static constexpr ImWchar ranges[] = {0x0400, 0x052F, 0};
            return ranges;
        }

        const ImWchar* CommonFontGlyphRanges() noexcept
        {
            static constexpr ImWchar ranges[] = {
                0x0100, 0x017F, 0x2014, 0x2014, 0x2019, 0x2019,
                0x2192, 0x2192, 0
            };
            return ranges;
        }

        const ImWchar* ArabicGlyphRanges() noexcept
        {
            static constexpr ImWchar ranges[] = {0x0600, 0x06FF, 0};
            return ranges;
        }

        const ImWchar* JapaneseGlyphRanges() noexcept
        {
            static constexpr ImWchar ranges[] = {
                0x3000, 0x30FF, 0x4E00, 0x9FFF, 0xFF00, 0xFFEF, 0
            };
            return ranges;
        }

        const ImWchar* KoreanGlyphRanges() noexcept
        {
            static constexpr ImWchar ranges[] = {0xAC00, 0xD7AF, 0};
            return ranges;
        }

        const ImWchar* SimplifiedChineseGlyphRanges() noexcept
        {
            static constexpr ImWchar ranges[] = {
                0x3000, 0x303F, 0x4E00, 0x9FFF, 0xFF00, 0xFFEF, 0
            };
            return ranges;
        }

        const ImWchar* TraditionalChineseGlyphRanges() noexcept
        {
            static constexpr ImWchar ranges[] = {
                0x3000, 0x303F, 0x4E00, 0x9FFF, 0xFF00, 0xFFEF, 0
            };
            return ranges;
        }

        const std::array ScriptFontProfiles{
            FontProfileDescriptor{"arabic", IDR_FONT_ARABIC, ArabicGlyphRanges()},
            FontProfileDescriptor{"japanese", IDR_FONT_JAPANESE, JapaneseGlyphRanges()},
            FontProfileDescriptor{"korean", IDR_FONT_KOREAN, KoreanGlyphRanges()},
            FontProfileDescriptor{"chinese-simplified", IDR_FONT_CHINESE_SIMPLIFIED,
                SimplifiedChineseGlyphRanges()},
            FontProfileDescriptor{"chinese-traditional", IDR_FONT_CHINESE_TRADITIONAL,
                TraditionalChineseGlyphRanges()},
        };

        constexpr std::array<std::string_view, 6> ProfileCodes{
            "base", "arabic", "japanese", "korean",
            "chinese-simplified", "chinese-traditional"};
        std::array<std::array<ImWchar, 257>, 6> selectorGlyphRanges{};
        std::array<ImFont*, 6> selectorFonts{};
        bool selectorRangesInitialized{};

        std::uint32_t DecodeUtf8(std::string_view text, std::size_t& index) noexcept
        {
            const auto first = static_cast<unsigned char>(text[index++]);
            if (first < 0x80) return first;
            const std::size_t extra = first < 0xE0 ? 1 : first < 0xF0 ? 2 : 3;
            std::uint32_t codepoint = first & (extra == 1 ? 0x1F :
                extra == 2 ? 0x0F : 0x07);
            for (std::size_t part = 0; part < extra && index < text.size(); ++part)
                codepoint = (codepoint << 6) |
                    (static_cast<unsigned char>(text[index++]) & 0x3F);
            return codepoint;
        }

        bool BuildSelectorGlyphRanges() noexcept
        {
            if (selectorRangesInitialized) return true;
            for (std::size_t profileIndex = 0; profileIndex < ProfileCodes.size();
                ++profileIndex) {
                std::array<ImWchar, 512> points{};
                std::size_t count{};
                for (const auto& locale : localization::LocaleRegistry) {
                    if (locale.fontProfileCode != ProfileCodes[profileIndex]) continue;
                    for (std::size_t offset = 0; offset < locale.displayName.size();) {
                        const auto codepoint = DecodeUtf8(locale.displayName, offset);
                        if (codepoint > 0xFFFF || count == points.size()) return false;
                        points[count++] = static_cast<ImWchar>(codepoint);
                    }
                }
                std::sort(points.begin(), points.begin() + count);
                count = static_cast<std::size_t>(std::unique(points.begin(),
                    points.begin() + count) - points.begin());
                auto& ranges = selectorGlyphRanges[profileIndex];
                std::size_t output{};
                for (std::size_t point = 0; point < count;) {
                    const ImWchar first = points[point];
                    ImWchar last = first;
                    while (++point < count && points[point] == last + 1)
                        last = points[point];
                    if (output + 2 >= ranges.size()) return false;
                    ranges[output++] = first;
                    ranges[output++] = last;
                }
                ranges[output] = 0;
            }
            selectorRangesInitialized = true;
            return true;
        }

        const FontProfileDescriptor* FindScriptProfile(
            std::string_view profileCode) noexcept
        {
            for (const auto& profile : ScriptFontProfiles)
                if (profile.code == profileCode) return &profile;
            return nullptr;
        }

        bool MergeResource(ImFontAtlas& atlas, HMODULE module,
            std::uint16_t resourceId, int fontSizePixels,
            const ImWchar* glyphRanges, float verticalOffset) noexcept
        {
            const HRSRC resource = FindResourceW(module,
                MAKEINTRESOURCEW(resourceId), MAKEINTRESOURCEW(10));
            if (!resource) return false;
            const DWORD size = SizeofResource(module, resource);
            const HGLOBAL loaded = LoadResource(module, resource);
            const void* data = loaded ? LockResource(loaded) : nullptr;
            if (!data || size == 0) return false;

            ImFontConfig config{};
            config.MergeMode = true;
            config.PixelSnapH = true;
            config.OversampleH = 1;
            config.OversampleV = 1;
            config.GlyphOffset.y = verticalOffset;
            config.FontDataOwnedByAtlas = false;
            config.GlyphRanges = glyphRanges;
            return atlas.AddFontFromMemoryTTF(const_cast<void*>(data), size,
                static_cast<float>(fontSizePixels), &config, glyphRanges) != nullptr;
        }
    }

    ImFont* AddLocalizationFont(int fontSizePixels,
        std::string_view profileCode) noexcept
    {
        ImFontAtlas* atlas = ImGui::GetIO().Fonts;
        if (!atlas || fontSizePixels < 1) return nullptr;

        ImFontConfig baseConfig{};
        baseConfig.SizePixels = static_cast<float>(fontSizePixels);
        baseConfig.PixelSnapH = true;
        baseConfig.OversampleH = 1;
        baseConfig.OversampleV = 1;
        ImFont* base = atlas->AddFontDefault(&baseConfig);
        if (!base) return nullptr;

        const HMODULE module = FontResourceModule();
        if (!module || !MergeResource(*atlas, module, IDR_FONT_CYRILLIC,
                fontSizePixels, CyrillicGlyphRanges(), 1.0f) ||
            !MergeResource(*atlas, module, IDR_FONT_CATALOG_COMMON,
                fontSizePixels, CommonFontGlyphRanges(), 0.0f))
            return nullptr;

        if (profileCode == "base") return base;
        for (const auto& profile : ScriptFontProfiles) {
            if (profile.code == profileCode)
                return MergeResource(*atlas, module, profile.resourceId,
                    fontSizePixels, profile.glyphRanges, 0.0f) ? base : nullptr;
        }
        return nullptr;
    }

    bool AddLocalizationSelectorFonts(int fontSizePixels) noexcept
    {
        ImFontAtlas* atlas = ImGui::GetIO().Fonts;
        if (!atlas || fontSizePixels < 1 || !BuildSelectorGlyphRanges()) return false;
        const HMODULE module = FontResourceModule();
        if (!module) return false;
        selectorFonts.fill(nullptr);
        for (std::size_t index = 0; index < ProfileCodes.size(); ++index) {
            ImFontConfig config{};
            config.SizePixels = static_cast<float>(fontSizePixels);
            config.PixelSnapH = true;
            config.OversampleH = 1;
            config.OversampleV = 1;
            ImFont* font = atlas->AddFontDefault(&config);
            if (!font) return false;
            const ImWchar* ranges = selectorGlyphRanges[index].data();
            if (ProfileCodes[index] == "base") {
                if (!MergeResource(*atlas, module, IDR_FONT_CYRILLIC,
                        fontSizePixels, CyrillicGlyphRanges(), 1.0f) ||
                    !MergeResource(*atlas, module, IDR_FONT_CATALOG_COMMON,
                        fontSizePixels, CommonFontGlyphRanges(), 0.0f))
                    return false;
            } else {
                const auto* profile = FindScriptProfile(ProfileCodes[index]);
                if (!profile || !MergeResource(*atlas, module, profile->resourceId,
                        fontSizePixels, ranges, 0.0f))
                    return false;
            }
            selectorFonts[index] = font;
        }
        return true;
    }

    ImFont* LocalizationSelectorFont(std::string_view profileCode) noexcept
    {
        for (std::size_t index = 0; index < ProfileCodes.size(); ++index)
            if (ProfileCodes[index] == profileCode) return selectorFonts[index];
        return nullptr;
    }

    std::span<const std::string_view> LocalizationFontProfileCodes() noexcept
    {
        return ProfileCodes;
    }
}
