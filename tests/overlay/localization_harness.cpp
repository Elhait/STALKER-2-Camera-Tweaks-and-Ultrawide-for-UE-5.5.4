#include "../../src/overlay/localization_catalog.hpp"
#include "../../src/overlay/localization_formatter.hpp"
#include "../../src/overlay/localization_manager.hpp"
#include "../../src/overlay/localization_validator.hpp"
#include "../../src/overlay/localization_font.hpp"
#include "../../src/overlay/game_language_reader.hpp"
#include "../../src/overlay/setting_tooltip_content.hpp"
#include "../../src/overlay/selector_examples_geometry.hpp"
#include "../../src/config/feature_config.hpp"
#include "canonical_ini_documentation.hpp"

#include <imgui.h>

#include <cassert>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

int main()
{
    using overlay::loc::Key;
    using overlay::setting_tooltip_content::Option;
    for (const float fontSize : std::array{14.0f, 21.0f}) {
        constexpr float contentWidth = 240.0f;
        constexpr float contentHeight = 54.0f;
        const ImVec2 frameMin(37.0f, 61.0f);
        const auto geometry = overlay::selector_examples_geometry::
            CalculateFrameGeometry(frameMin, contentWidth, contentHeight, fontSize);
        const float scale = fontSize / 14.0f;
        assert(std::fabs((geometry.contentMin.x - geometry.frameMin.x) -
            10.0f * scale) < 0.001f);
        assert(std::fabs((geometry.frameMax.x - geometry.contentMax.x) -
            10.0f * scale) < 0.001f);
        assert(std::fabs((geometry.contentMin.y - geometry.frameMin.y) -
            8.0f * scale) < 0.001f);
        assert(std::fabs((geometry.frameMax.y - geometry.contentMax.y) -
            8.0f * scale) < 0.001f);
    }
    const auto matches = [](const Option& option, std::span<const Key> descriptions,
        Key context, std::span<const Key> examples) {
        return option.descriptions.size() == descriptions.size() &&
            std::equal(option.descriptions.begin(), option.descriptions.end(),
                descriptions.begin(), [](Key left, Key right) {
                    return left.path == right.path;
                }) &&
            option.examplesContext.path == context.path &&
            option.examples.size() == examples.size() &&
            std::equal(option.examples.begin(), option.examples.end(), examples.begin(),
                [](Key left, Key right) { return left.path == right.path; });
    };
    const auto namesMatch = [](std::span<const Option> options,
        std::span<const std::string_view> names) {
        return options.size() == names.size() && std::equal(options.begin(), options.end(),
            names.begin(), [](const Option& option, std::string_view name) {
                return option.canonicalName == name;
            });
    };
    const auto selectorMappingMatches = [](auto mapping, std::span<const Option> docs,
        const auto& expectedValues) {
        if (mapping.size() != docs.size() ||
            mapping.size() != std::size(expectedValues))
            return false;
        for (std::size_t index = 0; index < mapping.size(); ++index) {
            if (mapping[index].value != expectedValues[index] ||
                mapping[index].documentation != &docs[index] ||
                mapping[index].documentation->canonicalName != docs[index].canonicalName)
                return false;
        }
        return true;
    };
    constexpr std::string_view gameplayNames[]{"AspectRecalculation", "HorPlus"};
    constexpr std::string_view aspectNames[]{"Auto", "Native", "16:9", "21:9", "32:9"};
    constexpr std::string_view fovNames[]{"NativeHorPlus", "GameplayHorPlus"};
    constexpr std::string_view dialogueNames[]{"Native", "Adaptive", "Reduced", "Disabled"};
    constexpr config::GameplayMode gameplayValues[]{
        config::GameplayMode::AspectRecalculation, config::GameplayMode::HorPlus};
    constexpr config::CinematicAspectPolicy aspectValues[]{
        config::CinematicAspectPolicy::Auto, config::CinematicAspectPolicy::Native,
        config::CinematicAspectPolicy::Forced16x9, config::CinematicAspectPolicy::Forced21x9,
        config::CinematicAspectPolicy::Forced32x9};
    constexpr config::CinematicFovMode fovValues[]{
        config::CinematicFovMode::NativeHorPlus, config::CinematicFovMode::GameplayHorPlus};
    constexpr config::DialogueZoomPolicy dialogueValues[]{
        config::DialogueZoomPolicy::Native, config::DialogueZoomPolicy::Adaptive,
        config::DialogueZoomPolicy::Reduced, config::DialogueZoomPolicy::Disabled};
    constexpr Key aspectRecalculationDescriptions[]{
        overlay::loc::tooltip::gameplay::AspectRecalculation};
    constexpr Key horPlusDescriptions[]{overlay::loc::tooltip::gameplay::HorPlus};
    constexpr Key horPlusExamples[]{
        overlay::loc::tooltip::gameplay::HorPlusExample90,
        overlay::loc::tooltip::gameplay::HorPlusExample100,
        overlay::loc::tooltip::gameplay::HorPlusExample110};
    constexpr Key aspectAutoDescriptions[]{overlay::loc::tooltip::cinematics::AspectAuto};
    constexpr Key aspectNativeDescriptions[]{overlay::loc::tooltip::cinematics::AspectNative};
    constexpr Key aspect169Descriptions[]{overlay::loc::tooltip::cinematics::Aspect169};
    constexpr Key aspect219Descriptions[]{overlay::loc::tooltip::cinematics::Aspect219};
    constexpr Key aspect329Descriptions[]{overlay::loc::tooltip::cinematics::Aspect329};
    constexpr Key aspect329Examples[]{overlay::loc::tooltip::cinematics::AspectForcedExample};
    constexpr Key fovNativeDescriptions[]{overlay::loc::tooltip::cinematics::FovNative};
    constexpr Key fovNativeExamples[]{overlay::loc::tooltip::cinematics::FovNativeExample90};
    constexpr Key fovGameplayDescriptions[]{overlay::loc::tooltip::cinematics::FovGameplay};
    constexpr Key fovGameplayExamples[]{
        overlay::loc::tooltip::cinematics::FovGameplayExample90,
        overlay::loc::tooltip::cinematics::FovGameplayExample112_6};
    constexpr Key dialogueContext = overlay::loc::tooltip::dialogue::ZoomExamplesContext;
    constexpr Key dialogueNativeDescriptions[]{overlay::loc::tooltip::dialogue::ZoomNative};
    constexpr Key dialogueNativeExamples[]{
        overlay::loc::tooltip::dialogue::ZoomNativeExample90,
        overlay::loc::tooltip::dialogue::ZoomNativeExample110};
    constexpr Key dialogueAdaptiveDescriptions[]{overlay::loc::tooltip::dialogue::ZoomAdaptive};
    constexpr Key dialogueAdaptiveExamples[]{
        overlay::loc::tooltip::dialogue::ZoomAdaptiveExample90,
        overlay::loc::tooltip::dialogue::ZoomAdaptiveExample110};
    constexpr Key dialogueReducedDescriptions[]{overlay::loc::tooltip::dialogue::ZoomReduced};
    constexpr Key dialogueReducedExamples[]{
        overlay::loc::tooltip::dialogue::ZoomReducedExample90,
        overlay::loc::tooltip::dialogue::ZoomReducedExample110};
    constexpr Key dialogueDisabledDescriptions[]{overlay::loc::tooltip::dialogue::ZoomDisabled};
    constexpr Key dialogueDisabledExamples[]{
        overlay::loc::tooltip::dialogue::ZoomDisabledExample90,
        overlay::loc::tooltip::dialogue::ZoomDisabledExample110};
    const auto gameplayOptions = overlay::setting_tooltip_content::Gameplay();
    const auto aspectOptions = overlay::setting_tooltip_content::CinematicAspect();
    const auto fovOptions = overlay::setting_tooltip_content::CinematicFov();
    const auto dialogueOptions = overlay::setting_tooltip_content::Dialogue();
    assert(namesMatch(gameplayOptions, gameplayNames));
    assert(namesMatch(aspectOptions, aspectNames));
    assert(namesMatch(fovOptions, fovNames));
    assert(namesMatch(dialogueOptions, dialogueNames));
    assert(selectorMappingMatches(overlay::setting_tooltip_content::GameplaySelectorOptions(),
        gameplayOptions, gameplayValues));
    assert(selectorMappingMatches(overlay::setting_tooltip_content::CinematicAspectSelectorOptions(),
        aspectOptions, aspectValues));
    assert(selectorMappingMatches(overlay::setting_tooltip_content::CinematicFovSelectorOptions(),
        fovOptions, fovValues));
    assert(selectorMappingMatches(overlay::setting_tooltip_content::DialogueSelectorOptions(),
        dialogueOptions, dialogueValues));
    assert(matches(gameplayOptions[0], aspectRecalculationDescriptions, {}, {}));
    assert(matches(gameplayOptions[1], horPlusDescriptions,
        overlay::loc::tooltip::gameplay::HorPlusExamplesContext, horPlusExamples));
    assert(matches(aspectOptions[0], aspectAutoDescriptions, {}, {}));
    assert(matches(aspectOptions[1], aspectNativeDescriptions, {}, {}));
    assert(matches(aspectOptions[2], aspect169Descriptions, {}, {}));
    assert(matches(aspectOptions[3], aspect219Descriptions, {}, {}));
    assert(matches(aspectOptions[4], aspect329Descriptions, {}, aspect329Examples));
    assert(matches(fovOptions[0], fovNativeDescriptions,
        overlay::loc::tooltip::cinematics::FovNativeExamplesContext, fovNativeExamples));
    assert(matches(fovOptions[1], fovGameplayDescriptions,
        overlay::loc::tooltip::cinematics::FovGameplayExamplesContext, fovGameplayExamples));
    assert(matches(dialogueOptions[0], dialogueNativeDescriptions,
        dialogueContext, dialogueNativeExamples));
    assert(matches(dialogueOptions[1], dialogueAdaptiveDescriptions,
        dialogueContext, dialogueAdaptiveExamples));
    assert(matches(dialogueOptions[2], dialogueReducedDescriptions,
        dialogueContext, dialogueReducedExamples));
    assert(matches(dialogueOptions[3], dialogueDisabledDescriptions,
        dialogueContext, dialogueDisabledExamples));

    assert(!overlay::ShouldSynchronizeAutoLanguage(false, false));
    assert(!overlay::ShouldSynchronizeAutoLanguage(false, true));
    assert(!overlay::ShouldSynchronizeAutoLanguage(true, false));
    assert(overlay::ShouldSynchronizeAutoLanguage(true, true));
    std::string error;
    overlay::LocalizationCatalog catalog;
    assert(catalog.LoadJson("{\"ui\":{\"notice\":\"Press {key}\"}}", error));
    assert(catalog.Find({"ui.notice"}) == "Press {key}");
    assert(!catalog.Contains({"ui"}));
    overlay::LocalizationCatalog unicode;
    assert(unicode.LoadJson("{\"word\":\"\\u041c\\u043e\\u0432\\u0430\"}", error));
    assert(unicode.Find({"word"}) == "\xd0\x9c\xd0\xbe\xd0\xb2\xd0\xb0");
    const std::string invalidUtf8 = "{\"word\":\"\xff\"}";
    assert(!unicode.LoadJson(invalidUtf8, error));

    overlay::LocalizationCatalog duplicate;
    assert(!duplicate.LoadJson("{\"a\":{\"b\":\"one\"},\"a.b\":\"two\"}", error));
    overlay::LocalizationCatalog invalid;
    assert(!invalid.LoadJson("{\"x\":true}", error));
    assert(!invalid.LoadJson("{\"x\":\"\\uD800\"}", error));
    overlay::LocalizationCatalog unexpected;
    assert(unexpected.LoadJson("{\"ui\":{\"notice\":\"Press {key}\"},\"unused\":\"old\"}", error));
    constexpr overlay::loc::Key expectedOnly[] = {{"ui.notice"}, {"ui.notice"}};
    assert(!overlay::LocalizationValidator::ValidateCanonical(unexpected,
        expectedOnly, error));
    overlay::LocalizationCatalog placeholderMismatch;
    assert(placeholderMismatch.LoadJson("{\"ui\":{\"notice\":\"Press {button}\"}}", error));
    assert(!overlay::LocalizationValidator::ValidateCompatible(catalog,
        placeholderMismatch, error));

    assert(overlay::LocalizationFormatter::Format("Press {key}", {{"key", "Insert"}})
        == "Press Insert");
    std::vector<std::string> placeholders;
    assert(overlay::LocalizationFormatter::Placeholders("{key} then {key}", placeholders, error));
    assert(placeholders.size() == 1 && placeholders.front() == "key");
    assert(!overlay::LocalizationFormatter::Placeholders("{bad-name}", placeholders, error));

    const auto* canonicalDescriptor = localization::CanonicalLocaleDescriptor();
    assert(canonicalDescriptor && canonicalDescriptor->code == "en");
    overlay::LocalizationCatalog english;
    {
        std::string path{"locales/"};
        path += canonicalDescriptor->catalogFileName;
        std::ifstream file(path, std::ios::binary);
        assert(file.good());
        const std::string bytes((std::istreambuf_iterator<char>(file)), {});
        assert(english.LoadJson(bytes, error));
    }
    const auto assertIniContentMatches = [&](const Option& option) {
        for (const Key key : option.descriptions)
            assert(config::canonical_ini_documentation::Find(key.path) == english.Find(key));
        if (!option.examplesContext.path.empty())
            assert(config::canonical_ini_documentation::Find(option.examplesContext.path) ==
                english.Find(option.examplesContext));
        for (const Key key : option.examples)
            assert(config::canonical_ini_documentation::Find(key.path) == english.Find(key));
    };
    for (const Option& option : gameplayOptions) assertIniContentMatches(option);
    for (const Option& option : aspectOptions) assertIniContentMatches(option);
    for (const Option& option : fovOptions) assertIniContentMatches(option);
    for (const Option& option : dialogueOptions) assertIniContentMatches(option);
    assert(overlay::LocalizationValidator::ValidateCanonical(
        english, overlay::loc::Inventory(), error));
    assert(english.size() == overlay::loc::Inventory().size());
    overlay::LocalizationCatalog ukrainian;
    {
        const auto* descriptor = localization::FindLocaleDescriptor("uk");
        assert(descriptor);
        std::string path{"locales/"};
        path += descriptor->catalogFileName;
        std::ifstream file(path, std::ios::binary);
        assert(file.good());
        const std::string bytes((std::istreambuf_iterator<char>(file)), {});
        assert(ukrainian.LoadJson(bytes, error));
    }
    assert(english.Find(overlay::loc::tooltip::gameplay::HorPlus) !=
        ukrainian.Find(overlay::loc::tooltip::gameplay::HorPlus));
    assert(english.Find(overlay::loc::tooltip::gameplay::HorPlusExamplesContext) !=
        ukrainian.Find(overlay::loc::tooltip::gameplay::HorPlusExamplesContext));
    for (const auto& descriptor : localization::LocaleRegistry) {
        std::string path{"locales/"};
        path += descriptor.catalogFileName;
        std::ifstream localeFile(path, std::ios::binary);
        assert(localeFile.good());
        const std::string localeBytes((std::istreambuf_iterator<char>(localeFile)), {});
        overlay::LocalizationCatalog localeCatalog;
        assert(localeCatalog.LoadJson(localeBytes, error));
        assert(!localeCatalog.Contains({"option.auto"}));
        assert(!localeCatalog.Contains({"option.native"}));
        assert(!localeCatalog.Contains({"option.adaptive"}));
        assert(!localeCatalog.Contains({"option.reduced"}));
        assert(!localeCatalog.Contains({"option.disabled"}));
        assert(descriptor.canonical
            ? overlay::LocalizationValidator::ValidateCanonical(
                localeCatalog, overlay::loc::Inventory(), error)
            : overlay::LocalizationValidator::ValidateCompatible(
                english, localeCatalog, error));
    }

    ImGui::CreateContext();
    struct AtlasSnapshot {
        int width{};
        int height{};
        float fontSize{};
        float ascent{};
        float descent{};
        float fallbackAdvance{};
        std::vector<unsigned char> pixels;
        std::vector<ImFontGlyph> glyphs;
        std::vector<ImFontConfig> configs;
    };
    const auto captureAtlas = [] {
        AtlasSnapshot snapshot{};
        ImGuiIO& io = ImGui::GetIO();
        ImFont* font = io.FontDefault;
        assert(font && io.Fonts->IsBuilt());
        unsigned char* pixels{};
        io.Fonts->GetTexDataAsAlpha8(&pixels, &snapshot.width, &snapshot.height);
        assert(pixels && snapshot.width > 0 && snapshot.height > 0);
        snapshot.pixels.assign(pixels,
            pixels + static_cast<std::size_t>(snapshot.width) * snapshot.height);
        snapshot.fontSize = font->FontSize;
        snapshot.ascent = font->Ascent;
        snapshot.descent = font->Descent;
        snapshot.fallbackAdvance = font->FallbackAdvanceX;
        snapshot.glyphs.assign(font->Glyphs.begin(), font->Glyphs.end());
        for (int i = 0; i < font->SourcesCount; ++i)
            snapshot.configs.push_back(font->Sources[i]);
        return snapshot;
    };
    const auto assertAtlasEquivalent = [](const AtlasSnapshot& expected,
        const AtlasSnapshot& actual) {
        assert(expected.width == actual.width && expected.height == actual.height);
        assert(expected.fontSize == actual.fontSize);
        assert(expected.ascent == actual.ascent && expected.descent == actual.descent);
        assert(expected.fallbackAdvance == actual.fallbackAdvance);
        assert(expected.pixels == actual.pixels);
        assert(expected.glyphs.size() == actual.glyphs.size());
        for (std::size_t i = 0; i < expected.glyphs.size(); ++i) {
            const ImFontGlyph& a = expected.glyphs[i];
            const ImFontGlyph& b = actual.glyphs[i];
            assert(a.Codepoint == b.Codepoint && a.AdvanceX == b.AdvanceX);
            assert(a.X0 == b.X0 && a.Y0 == b.Y0 && a.X1 == b.X1 && a.Y1 == b.Y1);
            assert(a.U0 == b.U0 && a.V0 == b.V0 && a.U1 == b.U1 && a.V1 == b.V1);
        }
        assert(expected.configs.size() == actual.configs.size());
        for (std::size_t i = 0; i < expected.configs.size(); ++i) {
            const ImFontConfig& a = expected.configs[i];
            const ImFontConfig& b = actual.configs[i];
            assert(a.SizePixels == b.SizePixels && a.MergeMode == b.MergeMode);
            assert(a.PixelSnapH == b.PixelSnapH && a.OversampleH == b.OversampleH);
            assert(a.OversampleV == b.OversampleV);
            assert(a.GlyphOffset.x == b.GlyphOffset.x && a.GlyphOffset.y == b.GlyphOffset.y);
            assert(a.RasterizerDensity == b.RasterizerDensity);
            assert(a.RasterizerMultiply == b.RasterizerMultiply);
        }
    };
    const auto verifyTextGlyphs = [](ImFont* uiFont, std::string_view value) {
        assert(uiFont);
        for (std::size_t i = 0; i < value.size();) {
                const auto first = static_cast<unsigned char>(value[i]);
                std::uint32_t codepoint{};
                std::size_t length{};
                if (first < 0x80) { codepoint = first; length = 1; }
                else if ((first & 0xE0) == 0xC0) { codepoint = first & 0x1F; length = 2; }
                else if ((first & 0xF0) == 0xE0) { codepoint = first & 0x0F; length = 3; }
                else { codepoint = first & 0x07; length = 4; }
                for (std::size_t j = 1; j < length; ++j)
                    codepoint = (codepoint << 6) |
                        (static_cast<unsigned char>(value[i + j]) & 0x3F);
                assert(codepoint <= 0xFFFF);
                if (!uiFont->FindGlyphNoFallback(static_cast<ImWchar>(codepoint)))
                    std::fprintf(stderr, "missing_text_glyph=U+%04X\n",
                        static_cast<unsigned>(codepoint));
                assert(uiFont->FindGlyphNoFallback(static_cast<ImWchar>(codepoint)));
                i += length;
        }
    };
    const auto verifyGlyphs = [&](ImFont* uiFont,
        const overlay::LocalizationCatalog& source) {
        for (const std::string_view key : source.Keys()) {
            verifyTextGlyphs(uiFont, source.Find({key}));
        }
    };
    const auto profileForLocale = [](std::string_view localeCode) {
        const auto* descriptor = localization::FindLocaleDescriptor(localeCode);
        assert(descriptor);
        return descriptor->fontProfileCode;
    };
    for (int fontSize = config::OverlayFontSizeMin;
         fontSize <= config::OverlayFontSizeMax; ++fontSize) {
        ImGuiIO& io = ImGui::GetIO();
        int allLoadedGlyphCount = 0;
        for (const auto profileCode : overlay::LocalizationFontProfileCodes()) {
            ImFont* profileFont = overlay::AddLocalizationFont(fontSize, profileCode);
            assert(profileFont);
        }
        assert(overlay::AddLocalizationSelectorFonts(fontSize));
        assert(io.Fonts->Build());
        int allLoadedWidth{};
        int allLoadedHeight{};
        unsigned char* atlasPixels{};
        io.Fonts->GetTexDataAsAlpha8(&atlasPixels, &allLoadedWidth, &allLoadedHeight);
        assert(atlasPixels && allLoadedWidth > 0 && allLoadedHeight > 0);
        for (ImFont* profileFont : io.Fonts->Fonts)
            allLoadedGlyphCount += profileFont->Glyphs.Size;
        const std::size_t allLoadedRgbaBytes = static_cast<std::size_t>(
            allLoadedWidth) * allLoadedHeight * 4;

        int dynamicMaxWidth{};
        int dynamicMaxHeight{};
        int dynamicMaxGlyphCount{};
        std::size_t dynamicMaxRgbaBytes{};
        for (const auto profileCode : overlay::LocalizationFontProfileCodes()) {
            io.Fonts->Clear();
            io.FontDefault = overlay::AddLocalizationFont(fontSize, profileCode);
            assert(io.FontDefault && overlay::AddLocalizationSelectorFonts(fontSize));
            assert(io.Fonts->Build());
            unsigned char* profilePixels{};
            int profileWidth{};
            int profileHeight{};
            io.Fonts->GetTexDataAsAlpha8(&profilePixels, &profileWidth, &profileHeight);
            assert(profilePixels && profileWidth > 0 && profileHeight > 0);
            int profileGlyphCount{};
            for (ImFont* font : io.Fonts->Fonts)
                profileGlyphCount += font->Glyphs.Size;
            const std::size_t profileRgbaBytes = static_cast<std::size_t>(
                profileWidth) * profileHeight * 4;
            if (profileRgbaBytes > dynamicMaxRgbaBytes) {
                dynamicMaxWidth = profileWidth;
                dynamicMaxHeight = profileHeight;
                dynamicMaxGlyphCount = profileGlyphCount;
                dynamicMaxRgbaBytes = profileRgbaBytes;
            }
            for (const auto& descriptor : localization::LocaleRegistry) {
                verifyTextGlyphs(overlay::LocalizationSelectorFont(
                    descriptor.fontProfileCode), descriptor.displayName);
                if (descriptor.fontProfileCode != profileCode) continue;
                std::string localePath{"locales/"};
                localePath += descriptor.catalogFileName;
                std::ifstream localeFile(localePath, std::ios::binary);
                assert(localeFile.good());
                const std::string localeBytes((std::istreambuf_iterator<char>(localeFile)), {});
                overlay::LocalizationCatalog localeCatalog;
                assert(localeCatalog.LoadJson(localeBytes, error));
                verifyGlyphs(io.FontDefault, localeCatalog);
            }
        }
        std::printf("font_atlas size=%d all_loaded=%dx%d glyphs=%d rgba_bytes=%zu "
            "dynamic_profile_max=%dx%d glyphs=%d rgba_bytes=%zu\n",
            fontSize, allLoadedWidth, allLoadedHeight, allLoadedGlyphCount,
            allLoadedRgbaBytes, dynamicMaxWidth, dynamicMaxHeight,
            dynamicMaxGlyphCount, dynamicMaxRgbaBytes);

        io.Fonts->Clear();
        io.FontDefault = overlay::AddLocalizationFont(fontSize, "base");
        assert(io.FontDefault && overlay::AddLocalizationSelectorFonts(fontSize));
        assert(io.FontDefault && io.Fonts->Build());
        const AtlasSnapshot coldStart = captureAtlas();
        io.Fonts->Clear();
        io.FontDefault = overlay::AddLocalizationFont(fontSize == 13 ? 14 : 13,
            "base");
        assert(io.FontDefault && overlay::AddLocalizationSelectorFonts(
            fontSize == 13 ? 14 : 13));
        assert(io.FontDefault && io.Fonts->Build());
        io.Fonts->Clear();
        io.FontDefault = overlay::AddLocalizationFont(fontSize, "base");
        assert(io.FontDefault && overlay::AddLocalizationSelectorFonts(fontSize));
        assert(io.FontDefault && io.Fonts->Build());
        const AtlasSnapshot rebuilt = captureAtlas();
        assertAtlasEquivalent(coldStart, rebuilt);
    }

    overlay::LocalizationManager manager;
    assert(localization::LocaleRegistry.size() == 18);
    for (const auto& descriptor : localization::LocaleRegistry) {
        assert(descriptor.initialFontSize >= config::OverlayFontSizeMin);
        assert(descriptor.initialFontSize <= config::OverlayFontSizeMax);
    }
    assert(localization::FindLocaleDescriptor("uk")->initialFontSize == 15);
    assert(localization::FindLocaleDescriptor("ar")->initialFontSize == 20);
    assert(localization::FindLocaleDescriptor("ru")->initialFontSize == 15);
    assert(localization::FindLocaleDescriptor("sr-Cyrl")->initialFontSize == 15);
    assert(localization::FindLocaleDescriptor("en") != nullptr);
    assert(localization::FindLocaleDescriptor("uk") != nullptr);
    const auto* serbian = localization::FindLocaleDescriptor("sr-cYRL");
    assert(serbian && serbian->code == "sr-Cyrl");
    assert(localization::FindLocaleDescriptor("xx") == nullptr);
    assert(manager.locales().size() == localization::LocaleRegistry.size());
    if (!manager.Initialize("en", GetModuleHandleW(nullptr), error)) {
        std::fprintf(stderr, "embedded locale initialization failed: %s\n", error.c_str());
        return 1;
    }
    assert(manager.Text(overlay::loc::section::RuntimeSettings) == "Runtime settings");
    assert(manager.Format(overlay::loc::notice::MouseCapture, {{"key", "Insert"}})
        .find("Press Insert") != std::string::npos);
    for (const auto& descriptor : manager.locales()) {
        assert(localization::FindLocaleDescriptor(descriptor.code) == &descriptor);
        assert(manager.SetLocale(descriptor.code));
        assert(manager.localeCode() == descriptor.code);
        const std::string runtimeLabel = manager.Text(overlay::loc::section::RuntimeSettings);
        assert(!runtimeLabel.empty() && runtimeLabel.rfind("[missing:", 0) != 0);
        const auto startupHint = manager.Format(
            overlay::loc::notification::OpenOverlayHint, {{"key", "Delete"}});
        assert(startupHint.find("Delete") != std::string::npos &&
            startupHint.find("[Delete]") == std::string::npos &&
            startupHint.rfind("[missing:", 0) != 0);
    }
    assert(manager.SetLocale("uk"));
    assert(manager.Text(overlay::loc::section::RuntimeSettings) == "Налаштування гри");
    assert(manager.Format(overlay::loc::notice::MouseCapture, {{"key", "Delete"}})
        .find("Delete") != std::string::npos);
    assert(!manager.SetLocale("xx"));
    assert(manager.localeCode() == "uk");
    assert(manager.SetLocale("en"));
    assert(manager.Text({"not.a.real.key"}) == "[missing: not.a.real.key]");
    overlay::LocalizationManager invalidInitialLocale;
    assert(invalidInitialLocale.Initialize("xx", GetModuleHandleW(nullptr), error));
    assert(invalidInitialLocale.localeCode() == "en");
    overlay::LocalizationManager unavailableCatalog;
    error.clear();
    assert(!unavailableCatalog.Initialize("en", nullptr, error));
    assert(unavailableCatalog.initialized() && !unavailableCatalog.ready());
    assert(unavailableCatalog.Text(overlay::loc::section::RuntimeSettings)
        .rfind("[missing:", 0) == 0);
    ImGui::DestroyContext();
}
