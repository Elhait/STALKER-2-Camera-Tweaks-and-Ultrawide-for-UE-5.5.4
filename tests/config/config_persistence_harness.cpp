#include "../../src/config/config_repository.hpp"
#include "../../src/config/config_template.hpp"
#include "canonical_ini_documentation.hpp"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace
{
    std::string ReadText(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    }

    template <std::size_t Count>
    bool ContainsGeneratedComments(const std::string& text,
        const std::string_view (&comments)[Count])
    {
        for (const auto comment : comments)
            if (comment != ";" && text.find(comment) == std::string::npos) return false;
        return true;
    }

    bool WriteText(const std::filesystem::path& path, const std::string& value)
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << value;
        return static_cast<bool>(output);
    }

    std::filesystem::path MakeTestDirectory()
    {
        wchar_t temporaryPath[MAX_PATH]{};
        if (GetTempPathW(MAX_PATH, temporaryPath) == 0) return {};
        const auto directory = std::filesystem::path(temporaryPath) /
            (L"STALKER2ConfigPersistenceHarness-" + std::to_wstring(GetCurrentProcessId()));
        std::error_code error;
        std::filesystem::remove_all(directory, error);
        std::filesystem::create_directories(directory, error);
        return error ? std::filesystem::path{} : directory;
    }

    bool Persist(const std::filesystem::path& path, const char* value)
    {
        return config::PersistConfigValue(path, "Gameplay", "Enabled", value,
            [](std::string) {});
    }

    bool Synchronize(const std::filesystem::path& path)
    {
        return config::SynchronizeManagedConfigTemplate(path,
            [](std::string) {});
    }

    bool TestCanonicalInitialConfigDocumentation(const std::filesystem::path& path)
    {
        config::FeatureConfig configuration{};
        const auto synchronizer = [](const std::filesystem::path& target) {
            return Synchronize(target);
        };
        if (!config::LoadFeatureConfig(path, configuration, synchronizer,
            [](std::string) {})) return false;

        const auto initial = ReadText(path);
        const bool canonical = initial.find(
                "; STALKER 2 Camera Tweaks and Ultrawide for UE 5.5.4") != std::string::npos &&
            initial.find("v1.0.0") == std::string::npos &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::GameplayEnabledComments) &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::GameplayModeComments) &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::CinematicAspectComments) &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::CinematicFovComments) &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::DialogueZoomComments) &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::HotkeysEnabledComments) &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::HotkeyGameplayCycleComments) &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::HotkeyCinematicAspectCycleComments) &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::HotkeyCinematicFovCycleComments) &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::HotkeyDialogueCycleComments) &&
            ContainsGeneratedComments(initial,
                config::canonical_ini_documentation::OverlayToggleComments) &&
            std::size(config::canonical_ini_documentation::OverlayToggleComments) == 3 &&
            config::canonical_ini_documentation::OverlayToggleComments[0].substr(2) ==
                config::canonical_ini_documentation::Find("tooltip.overlay.always_active") &&
            config::canonical_ini_documentation::OverlayToggleComments[1] ==
                config::canonical_ini_documentation::HotkeyGameplayCycleComments[1] &&
            config::canonical_ini_documentation::OverlayToggleComments[2] ==
                config::canonical_ini_documentation::HotkeyGameplayCycleComments[2];
        if (!canonical || Synchronize(path)) return false;
        return ReadText(path) == initial;
    }

    bool TestNormalTemplateSynchronization(const std::filesystem::path& path,
        const std::string& original)
    {
        if (!WriteText(path, original)) return false;
        if (!Synchronize(path)) return false;
        const auto synchronized = ReadText(path);
        return synchronized.find("[Hotkeys]") != std::string::npos &&
            synchronized.find("[Diagnostics]") != std::string::npos &&
            synchronized.find("Enabled=false") != std::string::npos &&
            synchronized.find(config::canonical_ini_documentation::GameplayEnabledComments[0]) !=
                std::string::npos;
    }

    bool TestTemplateStagingFailurePreservesOriginal(const std::filesystem::path& path,
        const std::string& original)
    {
        if (!WriteText(path, original)) return false;
        const auto staging = std::filesystem::path(path.wstring() + L".tmp");
        std::error_code error;
        std::filesystem::create_directory(staging, error);
        if (error) return false;
        const bool result = Synchronize(path);
        std::filesystem::remove(staging, error);
        return !result && ReadText(path) == original;
    }

    bool TestTemplateReplacementFailurePreservesOriginal(const std::filesystem::path& path,
        const std::string& original)
    {
        if (!WriteText(path, original)) return false;
        const auto handle = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle == INVALID_HANDLE_VALUE) return false;

        const bool result = Synchronize(path);
        CloseHandle(handle);
        const auto staging = std::filesystem::path(path.wstring() + L".tmp");
        std::error_code error;
        std::filesystem::remove(staging, error);
        return !result && ReadText(path) == original;
    }

    bool TestTemplateRepairPreservesValuesAndIsIdempotent(const std::filesystem::path& path)
    {
        const std::string damaged =
            "; user header\n"
            "[Gameplay]\n"
            "; user gameplay note\n"
            "Enabled=false\n"
            "UnknownGameplay=keep\n"
            "\n"
            "[Dialogue]\n"
            "Zoom=Potato\n";
        if (!WriteText(path, damaged)) return false;

        if (!Synchronize(path)) return false;
        const auto repaired = ReadText(path);
        const bool preserved =
            repaired.find("; user header") != std::string::npos &&
            repaired.find("; user gameplay note") != std::string::npos &&
            repaired.find("Enabled=false") != std::string::npos &&
            repaired.find("UnknownGameplay=keep") != std::string::npos &&
            repaired.find("Zoom=Potato") != std::string::npos &&
            repaired.find("[Cinematics]") != std::string::npos &&
            repaired.find("[Overlay]") != std::string::npos &&
            repaired.find("FontSize=13") != std::string::npos &&
            repaired.find("[Hotkeys]") != std::string::npos &&
            repaired.find("; Native - Use the game's original dialogue zoom behavior.") != std::string::npos &&
            repaired.find("; 90° → 70°.") != std::string::npos;
        if (!preserved) return false;

        const auto secondBefore = repaired;
        if (Synchronize(path)) return false;
        return ReadText(path) == secondBefore;
    }

    bool TestNormalPersistence(const std::filesystem::path& path, const std::string& original)
    {
        if (!WriteText(path, original)) return false;
        return Persist(path, "false") && ReadText(path).find("Enabled=false") != std::string::npos;
    }

    bool TestDuplicateManagedOccurrencesConverge(const std::filesystem::path& path)
    {
        const std::string duplicate =
            "[Gameplay]\nEnabled=true\n\n"
            "[Gameplay]\nEnabled=true\n";
        if (!WriteText(path, duplicate) || !Persist(path, "false")) return false;
        config::FeatureConfig configuration{};
        if (!config::LoadFeatureConfig(path, configuration,
            [](const std::filesystem::path&) { return false; },
            [](std::string) {}) || configuration.gameplayEnabled)
            return false;
        const auto text = ReadText(path);
        const auto first = text.find("Enabled=false");
        const auto second = first == std::string::npos ? std::string::npos :
            text.find("Enabled=false", first + 1);
        return first != std::string::npos && second != std::string::npos;
    }

    bool TestStagingFailurePreservesOriginal(const std::filesystem::path& path,
        const std::string& original)
    {
        if (!WriteText(path, original)) return false;
        const auto staging = std::filesystem::path(path.wstring() + L".tmp");
        std::error_code error;
        std::filesystem::create_directory(staging, error);
        if (error) return false;
        const bool result = Persist(path, "false");
        std::filesystem::remove(staging, error);
        return !result && ReadText(path) == original;
    }

    bool TestReplacementFailurePreservesOriginal(const std::filesystem::path& path,
        const std::string& original)
    {
        if (!WriteText(path, original)) return false;
        const auto handle = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle == INVALID_HANDLE_VALUE) return false;

        const bool result = Persist(path, "false");
        CloseHandle(handle);
        const auto staging = std::filesystem::path(path.wstring() + L".tmp");
        std::error_code error;
        std::filesystem::remove(staging, error);
        return !result && ReadText(path) == original;
    }

    bool TestDiagnosticsConfig(const std::filesystem::path& path)
    {
        if (!WriteText(path, "[Diagnostics]\nEnabled=true\n")) return false;
        config::FeatureConfig configuration{};
        return config::LoadFeatureConfig(path, configuration,
            [](const std::filesystem::path&) { return false; },
            [](std::string) {}) && configuration.diagnosticsEnabled;
    }

    bool TestOverlayLocaleCodeConfig(const std::filesystem::path& path)
    {
        const auto synchronizer = [](const std::filesystem::path&) { return false; };
        const auto log = [](std::string) {};
        config::FeatureConfig configuration{};
        if (!WriteText(path, "[Overlay]\nLanguage=Auto\n") ||
            !config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            !configuration.overlayLocaleAuto || configuration.overlayLocaleCode != "en")
            return false;
        if (!config::PersistConfigValue(path, "Overlay", "Language", "Auto", log))
            return false;
        configuration = {};
        if (!config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            !configuration.overlayLocaleAuto || configuration.overlayLocaleCode != "en")
            return false;
        if (!WriteText(path, "[Overlay]\nLanguage=uk\n") ||
            !config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            configuration.overlayLocaleCode != "uk" || configuration.overlayLocaleAuto)
            return false;
        if (!WriteText(path, "[Gameplay]\nEnabled=true\n") ||
            !config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            configuration.overlayLocaleCode != "en")
            return false;
        if (!WriteText(path, "[Overlay]\nLanguage=unsupported\n") ||
            !config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            configuration.overlayLocaleCode != "en")
            return false;
        if (!WriteText(path, "[Overlay]\nLanguage=UK\n") ||
            !config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            configuration.overlayLocaleCode != "uk" ||
            configuration.overlayLocaleAuto ||
            configuration.overlayFontSize != 15)
            return false;
        if (!config::PersistConfigValue(path, "Overlay", "Language", "uk", log))
            return false;
        configuration = {};
        if (!config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            configuration.overlayLocaleCode != "uk")
            return false;

        for (const auto& locale : localization::LocaleRegistry) {
            std::string inputCode(locale.code);
            std::transform(inputCode.begin(), inputCode.end(), inputCode.begin(),
                [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
            if (!WriteText(path, "[Overlay]\nLanguage=" + inputCode + "\n") ||
                !config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
                configuration.overlayLocaleCode != locale.code ||
                configuration.overlayFontSize != locale.initialFontSize ||
                !config::PersistConfigValue(path, "Overlay", "Language",
                    std::string(locale.code), log))
                return false;
            configuration = {};
            if (!config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
                configuration.overlayLocaleCode != locale.code ||
                configuration.overlayFontSize != locale.initialFontSize)
                return false;
        }
        return true;
    }

    bool TestOverlayFontSizeConfig(const std::filesystem::path& path)
    {
        const auto synchronizer = [](const std::filesystem::path&) { return false; };
        const auto log = [](std::string) {};
        config::FeatureConfig configuration{};
        if (!WriteText(path, "[Overlay]\nFontSize=20\n") ||
            !config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            configuration.overlayFontSize != 20)
            return false;
        if (!WriteText(path, "[Gameplay]\nEnabled=true\n") ||
            !config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            configuration.overlayFontSize != config::OverlayFontSizeDefault)
            return false;
        if (!WriteText(path, "[Overlay]\nFontSize=25\n") ||
            !config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            configuration.overlayFontSize != config::OverlayFontSizeDefault)
            return false;
        if (!WriteText(path, "[Overlay]\nLanguage=ja\nFontSize=25\n") ||
            !config::LoadFeatureConfig(path, configuration, synchronizer, log) ||
            configuration.overlayFontSize !=
                localization::FindLocaleDescriptor("ja")->initialFontSize)
            return false;
        if (!config::PersistConfigValue(path, "Overlay", "FontSize", "12", log))
            return false;
        configuration = {};
        return config::LoadFeatureConfig(path, configuration, synchronizer, log) &&
            configuration.overlayFontSize == config::OverlayFontSizeMin &&
            config::ParseOverlayFontSize(" 24 ", configuration.overlayFontSize) &&
            configuration.overlayFontSize == config::OverlayFontSizeMax &&
            !config::ParseOverlayFontSize("11", configuration.overlayFontSize) &&
            !config::ParseOverlayFontSize("16px", configuration.overlayFontSize);
    }

    bool TestCinematicFovModeConfig(const std::filesystem::path& path)
    {
        if (!WriteText(path, "[Cinematics]\nFovMode=GameplayHorPlus\n")) return false;
        config::FeatureConfig configuration{};
        if (!config::LoadFeatureConfig(path, configuration,
            [](const std::filesystem::path&) { return false; },
            [](std::string) {})) return false;
        if (configuration.cinematicFovMode != config::CinematicFovMode::GameplayHorPlus)
            return false;
        if (!WriteText(path, "[Cinematics]\nFovMode=invalid\n")) return false;
        configuration = {};
        if (!config::LoadFeatureConfig(path, configuration,
            [](const std::filesystem::path&) { return false; },
            [](std::string) {})) return false;
        if (configuration.cinematicFovMode != config::CinematicFovMode::GameplayHorPlus)
            return false;
        if (config::NextCinematicFovMode(config::CinematicFovMode::NativeHorPlus) !=
                config::CinematicFovMode::GameplayHorPlus ||
            config::NextCinematicFovMode(config::CinematicFovMode::GameplayHorPlus) !=
                config::CinematicFovMode::NativeHorPlus)
            return false;
        const std::array<config::CinematicAspectPolicy, 5> aspectPolicies{
            config::CinematicAspectPolicy::Auto,
            config::CinematicAspectPolicy::Native,
            config::CinematicAspectPolicy::Forced16x9,
            config::CinematicAspectPolicy::Forced21x9,
            config::CinematicAspectPolicy::Forced32x9};
        auto policy = aspectPolicies.front();
        for (std::size_t index = 1; index <= aspectPolicies.size(); ++index) {
            policy = config::NextCinematicAspectPolicy(policy);
            if (policy != aspectPolicies[index % aspectPolicies.size()]) return false;
        }
        if (!WriteText(path, "[Hotkeys]\nCinematicFovCycle=F12\n")) return false;
        configuration = {};
        if (!config::LoadFeatureConfig(path, configuration,
            [](const std::filesystem::path&) { return false; },
            [](std::string) {})) return false;
        return configuration.cinematicFovCycleKey == VK_F12;
    }

    bool TestHotkeyValidationAndPersistence(const std::filesystem::path& path)
    {
        if (!WriteText(path,
            "[Hotkeys]\nEnabled=false\nGameplayCycle=F9\nCinematicCycle=F10\n"
            "CinematicFovCycle=F11\nDialogueCycle=F12\n")) return false;
        config::FeatureConfig configuration{};
        const auto log = [](std::string) {};
        if (!config::LoadFeatureConfig(path, configuration,
            [](const std::filesystem::path&) { return false; }, log)) return false;
        const bool validated = !configuration.hotkeysEnabled &&
            configuration.overlayToggleKey == VK_DELETE &&
            config::IsSupportedHotkey(VK_F24) && config::IsSupportedHotkey('A') &&
            config::IsSupportedHotkey('7') && config::IsSupportedHotkey(VK_INSERT) &&
            config::IsSupportedHotkey(VK_HOME) && config::IsSupportedHotkey(VK_NUMPAD7) &&
            config::IsSupportedHotkey(VK_OEM_1) &&
            std::strcmp(config::HotkeyName('A'), "A") == 0 &&
            std::strcmp(config::HotkeyName('7'), "7") == 0 &&
            std::strcmp(config::HotkeyName(VK_F24), "F24") == 0 &&
            std::strcmp(config::HotkeyName(VK_NUMPAD7), "Num 7") == 0 &&
            std::strcmp(config::HotkeyName(VK_INSERT), "Insert") == 0 &&
            std::strcmp(config::HotkeyName(VK_DELETE), "Delete") == 0 &&
            std::strcmp(config::HotkeyName(VK_HOME), "Home") == 0 &&
            !config::IsSupportedHotkey(VK_ESCAPE) &&
            !config::IsSupportedHotkey(VK_CONTROL) &&
            !config::IsSupportedHotkey(VK_LBUTTON) &&
            !config::IsSupportedHotkey(VK_PACKET) &&
            config::HasHotkeyConflict(configuration, VK_F10,
                config::HotkeyBindingId::GameplayMode) &&
            config::HasHotkeyConflict(configuration, VK_DELETE,
                config::HotkeyBindingId::GameplayMode) &&
            !config::HasHotkeyConflict(configuration, VK_F6,
                config::HotkeyBindingId::GameplayMode);
        const int previousGameplayKey = configuration.gameplayCycleKey;
        // A conflicting candidate must be rejected without changing the current binding.
        const bool conflictPreserved =
            configuration.gameplayCycleKey == previousGameplayKey;
        if (!validated || !conflictPreserved) {
            std::printf("hotkey_validation_details enabled=%d supported=%d conflict=%d free=%d preserved=%d keys=%d,%d,%d,%d\n",
                configuration.hotkeysEnabled, config::IsSupportedHotkey(VK_F6),
                config::HasHotkeyConflict(configuration, VK_F10,
                    config::HotkeyBindingId::OverlayToggle),
                !config::HasHotkeyConflict(configuration, VK_F6,
                    config::HotkeyBindingId::OverlayToggle), conflictPreserved,
                configuration.gameplayCycleKey, configuration.cinematicCycleKey,
                configuration.cinematicFovCycleKey, configuration.dialogueCycleKey);
            return false;
        }
        if (!config::PersistConfigValue(path, "Hotkeys", "GameplayCycle",
            config::HotkeyConfigValue(VK_OEM_1), log)) {
            std::printf("hotkey_persist=FAIL\n");
            return false;
        }
        configuration = {};
        const bool loaded = config::LoadFeatureConfig(path, configuration,
            [](const std::filesystem::path&) { return false; }, log) &&
            configuration.gameplayCycleKey == VK_OEM_1 &&
            configuration.cinematicCycleKey == VK_F10 &&
            configuration.overlayToggleKey == VK_DELETE;
        if (!loaded)
            std::printf("hotkey_reload_details keys=%d,%d,%d,%d\n",
                configuration.gameplayCycleKey, configuration.cinematicCycleKey,
                configuration.cinematicFovCycleKey, configuration.dialogueCycleKey);
        if (!loaded) return false;

        struct OemKeyLabel { int key; const char* label; };
        constexpr OemKeyLabel oemKeys[] = {
            {VK_OEM_1, ";"}, {VK_OEM_PLUS, "="}, {VK_OEM_COMMA, ","},
            {VK_OEM_MINUS, "-"}, {VK_OEM_PERIOD, "."}, {VK_OEM_2, "/"},
            {VK_OEM_3, "`"}, {VK_OEM_4, "["}, {VK_OEM_5, "\\"},
            {VK_OEM_6, "]"}, {VK_OEM_7, "'"}, {VK_OEM_8, "OEM 8"},
            {VK_OEM_102, "\\"}, {VK_OEM_AX, "OEM AX"}};
        for (const auto& [key, expectedLabel] : oemKeys) {
            int parsedKey = 0;
            const char* displayName = config::HotkeyName(key);
            if (!config::ParseHotkey(config::HotkeyConfigValue(key), parsedKey) ||
                parsedKey != key || !displayName ||
                std::strcmp(displayName, expectedLabel) != 0 ||
                !config::PersistConfigValue(path, "Hotkeys", "GameplayCycle",
                    config::HotkeyConfigValue(key), log))
                return false;
            configuration = {};
            if (!config::LoadFeatureConfig(path, configuration,
                [](const std::filesystem::path&) { return false; }, log) ||
                configuration.gameplayCycleKey != key ||
                std::strcmp(config::HotkeyName(configuration.gameplayCycleKey),
                    expectedLabel) != 0)
                return false;
        }
        if (!WriteText(path, "[Hotkeys]\nOverlayToggle=VK_46\nGameplayCycle=VK_46\n"))
            return false;
        configuration = {};
        return config::LoadFeatureConfig(path, configuration,
            [](const std::filesystem::path&) { return false; }, log) &&
            configuration.overlayToggleKey == 'F' &&
            configuration.gameplayCycleKey == VK_F9 &&
            !config::HasHotkeyConflict(configuration,
                configuration.overlayToggleKey,
                config::HotkeyBindingId::OverlayToggle);
    }

    bool TestHotkeyBindingRegistryRoundTrip(const std::filesystem::path& path)
    {
        constexpr config::HotkeyBindingId expectedIds[] = {
            config::HotkeyBindingId::OverlayToggle,
            config::HotkeyBindingId::GameplayMode,
            config::HotkeyBindingId::CinematicAspect,
            config::HotkeyBindingId::CinematicFov,
            config::HotkeyBindingId::DialogueZoom};
        if (config::HotkeyBindingRegistry.size() != std::size(expectedIds) ||
            !WriteText(path, "[Overlay]\nToggleKey=VK_2E\n[Hotkeys]\n"))
            return false;

        config::FeatureConfig defaults{};
        for (std::size_t index = 0; index < config::HotkeyBindingRegistry.size(); ++index) {
            const auto& descriptor = config::HotkeyBindingRegistry[index];
            if (descriptor.id != expectedIds[index] ||
                config::FindHotkeyBinding(descriptor.id) != &descriptor ||
                config::FindHotkeyBinding(descriptor.section, descriptor.configKey) != &descriptor ||
                config::HotkeyBindingValue(defaults, descriptor.id) !=
                    descriptor.defaultVirtualKey)
                return false;
            for (std::size_t other = index + 1;
                other < config::HotkeyBindingRegistry.size(); ++other) {
                const auto& candidate = config::HotkeyBindingRegistry[other];
                if (descriptor.id == candidate.id ||
                    (std::strcmp(descriptor.section, candidate.section) == 0 &&
                        std::strcmp(descriptor.configKey, candidate.configKey) == 0))
                    return false;
            }
            const int roundTripKey = 'A' + static_cast<int>(index);
            config::SetHotkeyBindingValue(defaults, descriptor.id, roundTripKey);
            if (config::HotkeyBindingValue(defaults, descriptor.id) != roundTripKey ||
                !config::PersistConfigValue(path, descriptor.section,
                    descriptor.configKey, config::HotkeyConfigValue(roundTripKey),
                    [](std::string) {}))
                return false;
        }

        config::FeatureConfig loaded{};
        if (!config::LoadFeatureConfig(path, loaded,
                [](const std::filesystem::path&) { return false; },
                [](std::string) {})) return false;
        for (std::size_t index = 0; index < config::HotkeyBindingRegistry.size(); ++index) {
            if (config::HotkeyBindingValue(loaded,
                    config::HotkeyBindingRegistry[index].id) !=
                'A' + static_cast<int>(index)) return false;
            for (std::size_t other = 0; other < config::HotkeyBindingRegistry.size(); ++other) {
                if (other == index) continue;
                if (!config::HasHotkeyConflict(loaded,
                        config::HotkeyBindingValue(loaded,
                            config::HotkeyBindingRegistry[other].id),
                        config::HotkeyBindingRegistry[index].id)) return false;
            }
        }
        return true;
    }

    bool TestLegacyOverlayToggleMigration(const std::filesystem::path& path)
    {
        const std::string legacy =
            "[Overlay]\nLanguage=Auto\n\n"
            "[Hotkeys]\nEnabled=false\nOverlayToggle=VK_41\n";
        if (!WriteText(path, legacy) || !Synchronize(path)) return false;
        const auto migratedText = ReadText(path);
        if (migratedText.find("ToggleKey=VK_41") == std::string::npos ||
            migratedText.find("OverlayToggle=") != std::string::npos)
            return false;
        config::FeatureConfig configuration{};
        if (!config::LoadFeatureConfig(path, configuration,
            [](const std::filesystem::path&) { return false; }, [](std::string) {}) ||
            configuration.overlayToggleKey != 'A')
            return false;
        return !Synchronize(path) && ReadText(path) == migratedText;
    }

    bool TestLegacyDocumentationMigration(const std::filesystem::path& path)
    {
        const std::string legacy =
            "[Gameplay]\n"
            "; Enables ultrawide aspect-ratio correction during gameplay.\n"
            "Enabled=true\n\n"
            "[Cinematics]\n"
            "; Controls cinematic framing on ultrawide displays.\n"
            "; Auto   - use the automatic display-aspect policy.\n"
            "; Native - keep the game's original cinematic behavior.\n"
            "; 16:9   - force the native 16:9 cinematic frame.\n"
            "; 21:9   - force a 21:9 cinematic frame.\n"
            "; 32:9   - force a 32:9 cinematic frame.\n"
            "AspectRatio=Native\n\n"
            "[Dialogue]\n"
            "; Controls the native dialogue camera zoom.\n"
            "; Native   - use the game's original dialogue zoom, currently targeting 70°.\n"
            "; Adaptive - preserve the native optical zoom strength relative to the current gameplay FOV.\n"
            "; Reduced  - apply half of the Adaptive optical zoom strength.\n"
            ";            Example: 110° gameplay FOV -> Adaptive ≈90°, Reduced ≈100°.\n"
            "; Disabled - keep the current gameplay FOV during dialogue.\n"
            "Zoom=Native\n\n"
            "[Hotkeys]\n"
            "; Enables or disables all runtime hotkeys.\n"
            "Enabled=false\n"
            "; Supported keys: F1-F12, 0-9 and A-Z.\n"
            "; Key used to cycle the cinematic mode for the next cinematic.\n"
            "; Auto -> Native -> 16:9 -> 21:9 -> 32:9 -> Auto.\n"
            "; Does not affect a cinematic that is already playing.\n"
            "CinematicCycle=F9\n"
            "; Key used to cycle the dialogue zoom mode for the next dialogue.\n"
            "; Native -> Adaptive -> Reduced -> Disabled -> Native.\n"
            "; Does not affect a dialogue that is already in progress.\n"
            "DialogueCycle=F10\n";
        if (!WriteText(path, legacy) || !Synchronize(path)) return false;
        const auto migrated = ReadText(path);
        const bool cleaned =
            migrated.find("; Enables ultrawide aspect-ratio correction during gameplay.") == std::string::npos &&
            migrated.find("; Controls cinematic framing on ultrawide displays.") == std::string::npos &&
            migrated.find("; Controls the native dialogue camera zoom.") == std::string::npos &&
            migrated.find("; Supported keys: F1-F12, 0-9 and A-Z.") == std::string::npos &&
            migrated.find("Enabled=true") != std::string::npos &&
            migrated.find("AspectRatio=Native") != std::string::npos &&
            migrated.find("Zoom=Native") != std::string::npos &&
            migrated.find("CinematicCycle=F9") != std::string::npos &&
            migrated.find("DialogueCycle=F10") != std::string::npos &&
            ContainsGeneratedComments(migrated,
                config::canonical_ini_documentation::HotkeyCinematicAspectCycleComments);
        if (!cleaned || Synchronize(path)) return false;
        return ReadText(path) == migrated;
    }

    bool TestCommentOwnershipPreservesUserNotes(const std::filesystem::path& path)
    {
        const std::string legacy =
            "[Cinematics]\n"
            "; my note: use this for screenshots\n"
            "; Controls cinematic framing on ultrawide displays.\n"
            "; Auto   - use the automatic display-aspect policy.\n"
            "; Native - keep the game's original cinematic behavior.\n"
            "; 16:9   - force the native 16:9 cinematic frame.\n"
            "; 21:9   - force a 21:9 cinematic frame.\n"
            "; 32:9   - force a 32:9 cinematic frame.\n"
            "AspectRatio=Native\n";
        if (!WriteText(path, legacy) || !Synchronize(path)) return false;
        const auto migrated = ReadText(path);
        return migrated.find("; my note: use this for screenshots") != std::string::npos &&
            migrated.find("; Controls cinematic framing on ultrawide displays.") == std::string::npos &&
            migrated.find("AspectRatio=Native") != std::string::npos &&
            ContainsGeneratedComments(migrated,
                config::canonical_ini_documentation::CinematicAspectComments) &&
            !Synchronize(path) && ReadText(path) == migrated;
    }

    bool TestModifiedLegacyBlockIsPreserved(const std::filesystem::path& path)
    {
        const std::string modified =
            "[Cinematics]\n"
            "; Controls cinematic framing on ultrawide displays.\n"
            "; Auto - my custom aspect note.\n"
            "; Native - keep the game's original cinematic behavior.\n"
            "; 16:9   - force the native 16:9 cinematic frame.\n"
            "; 21:9   - force a 21:9 cinematic frame.\n"
            "; 32:9   - force a 32:9 cinematic frame.\n"
            "AspectRatio=Native\n";
        if (!WriteText(path, modified) || !Synchronize(path)) return false;
        const auto synchronized = ReadText(path);
        return synchronized.find("; Auto - my custom aspect note.") != std::string::npos &&
            synchronized.find("AspectRatio=Native") != std::string::npos &&
            ContainsGeneratedComments(synchronized,
                config::canonical_ini_documentation::CinematicAspectComments) &&
            !Synchronize(path) && ReadText(path) == synchronized;
    }
}

int main()
{
    const auto directory = MakeTestDirectory();
    if (directory.empty()) {
        std::printf("setup=FAIL\n");
        return 1;
    }

    const auto path = directory / "config.ini";
    const auto initialPath = directory / "initial.ini";
    const std::string original = "[Gameplay]\nEnabled=true\n";
    const bool normal = TestNormalPersistence(path, original);
    const bool duplicateOccurrences = TestDuplicateManagedOccurrencesConverge(path);
    const bool staging = TestStagingFailurePreservesOriginal(path, original);
    const bool replacement = TestReplacementFailurePreservesOriginal(path, original);
    const bool templateNormal = TestNormalTemplateSynchronization(path, original);
    const bool templateStaging = TestTemplateStagingFailurePreservesOriginal(path, original);
    const bool templateReplacement = TestTemplateReplacementFailurePreservesOriginal(path, original);
    const bool templateRepair = TestTemplateRepairPreservesValuesAndIsIdempotent(path);
    const bool diagnostics = TestDiagnosticsConfig(path);
    const bool overlayLanguage = TestOverlayLocaleCodeConfig(path);
    const bool overlayFontSize = TestOverlayFontSizeConfig(path);
    const bool cinematicFovMode = TestCinematicFovModeConfig(path);
    const bool hotkeyBindings = TestHotkeyValidationAndPersistence(path);
    const bool hotkeyRegistry = TestHotkeyBindingRegistryRoundTrip(path);
    const bool legacyOverlayToggle = TestLegacyOverlayToggleMigration(path);
    const bool canonicalInitial = TestCanonicalInitialConfigDocumentation(initialPath);
    const bool legacyDocumentation = TestLegacyDocumentationMigration(path);
    const bool userCommentPreserved = TestCommentOwnershipPreservesUserNotes(path);
    const bool modifiedLegacyPreserved = TestModifiedLegacyBlockIsPreserved(path);

    std::error_code error;
    std::filesystem::remove_all(directory, error);
    std::printf("normal=%s staging_failure_preserves=%s replacement_failure_preserves=%s "
        "template_normal=%s template_staging_failure_preserves=%s "
        "template_replacement_failure_preserves=%s template_repair_idempotent=%s "
        "diagnostics_config=%s overlay_language=%s overlay_font_size=%s cinematic_fov_mode=%s duplicate_occurrences=%s hotkey_bindings=%s hotkey_registry=%s legacy_overlay_toggle=%s canonical_initial_documentation=%s legacy_documentation=%s user_comment_preserved=%s modified_legacy_preserved=%s\n",
        normal ? "PASS" : "FAIL", staging ? "PASS" : "FAIL",
        replacement ? "PASS" : "FAIL", templateNormal ? "PASS" : "FAIL",
        templateStaging ? "PASS" : "FAIL", templateReplacement ? "PASS" : "FAIL",
        templateRepair ? "PASS" : "FAIL", diagnostics ? "PASS" : "FAIL",
        overlayLanguage ? "PASS" : "FAIL",
        overlayFontSize ? "PASS" : "FAIL",
        cinematicFovMode ? "PASS" : "FAIL", duplicateOccurrences ? "PASS" : "FAIL",
        hotkeyBindings ? "PASS" : "FAIL", hotkeyRegistry ? "PASS" : "FAIL",
        legacyOverlayToggle ? "PASS" : "FAIL", canonicalInitial ? "PASS" : "FAIL",
        legacyDocumentation ? "PASS" : "FAIL",
        userCommentPreserved ? "PASS" : "FAIL",
        modifiedLegacyPreserved ? "PASS" : "FAIL");
    return normal && staging && replacement && templateNormal && templateStaging &&
        templateReplacement && templateRepair && diagnostics && overlayLanguage &&
        overlayFontSize && cinematicFovMode &&
        duplicateOccurrences && hotkeyBindings && hotkeyRegistry &&
        legacyOverlayToggle && canonicalInitial && legacyDocumentation &&
        userCommentPreserved && modifiedLegacyPreserved ? 0 : 1;
}
