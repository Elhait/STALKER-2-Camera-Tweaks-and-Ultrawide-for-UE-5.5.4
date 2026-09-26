#include "config_template.hpp"
#include "config_repository.hpp"

#include "canonical_ini_documentation.hpp"
#include "feature_config.hpp"

#include <array>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <Windows.h>

namespace config
{
    namespace
    {
        struct ManagedKey
        {
            std::string_view name;
            std::string_view defaultLine;
            const std::string_view* comments;
            std::size_t commentCount;
        };

        using canonical_ini_documentation::GameplayEnabledComments;
        using canonical_ini_documentation::GameplayModeComments;
        using canonical_ini_documentation::CinematicAspectComments;
        using canonical_ini_documentation::CinematicFovComments;
        using canonical_ini_documentation::DialogueZoomComments;
        using canonical_ini_documentation::HotkeysEnabledComments;
        using canonical_ini_documentation::HotkeyGameplayCycleComments;
        using canonical_ini_documentation::HotkeyCinematicAspectCycleComments;
        using canonical_ini_documentation::HotkeyCinematicFovCycleComments;
        using canonical_ini_documentation::HotkeyDialogueCycleComments;
        using canonical_ini_documentation::OverlayToggleComments;
        constexpr std::string_view diagnosticsComments[] = {
            "; Enables the supported read-only runtime telemetry in the canonical ASI.",
            "; false - keep diagnostic hooks and telemetry disabled.",
            "; true  - enable CameraState, ZOOM and HorPlus FOV telemetry.",
        };
        constexpr std::string_view overlayPositionComments[] = {
            "; Last overlay position in screen coordinates. Updated automatically when dragged.",
        };
        constexpr std::string_view overlayMigrationComments[] = {
            "; Internal one-time migration marker for the former separate overlay INI.",
        };
        constexpr std::string_view overlayLanguageComments[] = {
            "; Auto follows the game's Interface Language when the overlay opens.",
            "; Or select a locale code from the registry to override the game language.",
            "; Missing or unknown values use English; Auto remains configured on read failure.",
        };
        constexpr std::string_view overlayFontSizeComments[] = {
            "; Overlay font size in pixels. Supported range: 12-24; locale changes select that locale's initial size.",
            "; Changes apply immediately and are saved to this configuration file.",
        };

        constexpr ManagedKey gameplayKeys[] = {
            {"Enabled", "Enabled=true", GameplayEnabledComments, std::size(GameplayEnabledComments)},
            {"Mode", "Mode=HorPlus", GameplayModeComments, std::size(GameplayModeComments)},
        };
        constexpr ManagedKey cinematicKeys[] = {
            {"AspectRatio", "AspectRatio=Auto", CinematicAspectComments, std::size(CinematicAspectComments)},
            {"FovMode", "FovMode=GameplayHorPlus", CinematicFovComments, std::size(CinematicFovComments)},
        };
        constexpr ManagedKey dialogueKeys[] = {
            {"Zoom", "Zoom=Adaptive", DialogueZoomComments, std::size(DialogueZoomComments)},
        };
        constexpr ManagedKey diagnosticsKeys[] = {
            {"Enabled", "Enabled=false", diagnosticsComments, std::size(diagnosticsComments)},
        };
        constexpr ManagedKey hotkeyKeys[] = {
            {"Enabled", "Enabled=false", HotkeysEnabledComments,
                std::size(HotkeysEnabledComments)},
            {"GameplayCycle", "GameplayCycle=F9", HotkeyGameplayCycleComments,
                std::size(HotkeyGameplayCycleComments)},
            {"CinematicCycle", "CinematicCycle=F10", HotkeyCinematicAspectCycleComments,
                std::size(HotkeyCinematicAspectCycleComments)},
            {"CinematicFovCycle", "CinematicFovCycle=F11", HotkeyCinematicFovCycleComments,
                std::size(HotkeyCinematicFovCycleComments)},
            {"DialogueCycle", "DialogueCycle=F12", HotkeyDialogueCycleComments,
                std::size(HotkeyDialogueCycleComments)},
        };
        constexpr ManagedKey overlayKeys[] = {
            {"ToggleKey", "ToggleKey=VK_2E", OverlayToggleComments,
                std::size(OverlayToggleComments)},
            {"Language", "Language=Auto", overlayLanguageComments,
                std::size(overlayLanguageComments)},
            {"FontSize", "FontSize=13", overlayFontSizeComments,
                std::size(overlayFontSizeComments)},
            {"PositionX", "PositionX=30", overlayPositionComments,
                std::size(overlayPositionComments)},
            {"PositionY", "PositionY=30", overlayPositionComments,
                std::size(overlayPositionComments)},
            {"PositionMigrationComplete", "PositionMigrationComplete=0",
                overlayMigrationComments, std::size(overlayMigrationComments)},
        };

        const ManagedKey* KeysForSection(const std::string& section, std::size_t& count)
        {
            if (section == "Gameplay") { count = std::size(gameplayKeys); return gameplayKeys; }
            if (section == "Cinematics") { count = std::size(cinematicKeys); return cinematicKeys; }
            if (section == "Dialogue") { count = std::size(dialogueKeys); return dialogueKeys; }
            if (section == "Diagnostics") { count = std::size(diagnosticsKeys); return diagnosticsKeys; }
            if (section == "Hotkeys") { count = std::size(hotkeyKeys); return hotkeyKeys; }
            if (section == "Overlay") { count = std::size(overlayKeys); return overlayKeys; }
            count = 0;
            return nullptr;
        }

        struct CommentBlock
        {
            std::string_view section;
            std::string_view key;
            const std::string_view* lines;
            std::size_t lineCount;
        };

        constexpr std::string_view legacyGameplayEnabled[] = {
            "; Correct gameplay aspect behavior on ultrawide displays.",
            "; Enables ultrawide aspect-ratio correction during gameplay.",
            "; Use true to enable the feature or false to disable it.",
        };
        constexpr std::string_view legacyGameplayEnabledGenerated[] = {
            "; Enables ultrawide aspect-ratio correction during gameplay.",
        };
        constexpr std::string_view legacyCinematicAspect[] = {
            "; Controls cinematic framing on ultrawide displays.",
            "; Auto   - use the detected display aspect ratio.",
            "; Native - keep the game's original cinematic behavior.",
            "; 16:9   - force the native 16:9 cinematic frame.",
            "; 21:9   - force a 21:9 cinematic frame.",
            "; 32:9   - force a 32:9 cinematic frame.",
            "; Auto, Native, 16:9, 21:9, 32:9",
        };
        constexpr std::string_view legacyCinematicAspectGenerated[] = {
            "; Controls cinematic framing on ultrawide displays.",
            "; Auto   - use the automatic display-aspect policy.",
            "; Native - keep the game's original cinematic behavior.",
            "; 16:9   - force the native 16:9 cinematic frame.",
            "; 21:9   - force a 21:9 cinematic frame.",
            "; 32:9   - force a 32:9 cinematic frame.",
        };
        constexpr std::string_view legacyDialogueZoom[] = {
            "; Controls the native dialogue camera zoom.",
            "; Native   - use the game's original dialogue zoom, currently targeting 70°.",
            "; Adaptive - preserve the native optical zoom strength relative to the current gameplay FOV.",
            "; Reduced  - apply half of the Adaptive optical zoom strength.",
            ";            Example: 110° gameplay FOV -> Adaptive ≈90°, Reduced ≈100°.",
            "; Disabled - keep the current gameplay FOV during dialogue.",
            "; Native, Reduced, Disabled",
        };
        constexpr std::string_view legacyDialogueZoomGenerated[] = {
            "; Controls the native dialogue camera zoom.",
            "; Native   - use the game's original dialogue zoom, currently targeting 70°.",
            "; Adaptive - preserve the native optical zoom strength relative to the current gameplay FOV.",
            "; Reduced  - apply half of the Adaptive optical zoom strength.",
            ";            Example: 110° gameplay FOV -> Adaptive ≈90°, Reduced ≈100°.",
            "; Disabled - keep the current gameplay FOV during dialogue.",
        };
        constexpr std::string_view legacyHotkeysEnabled[] = {
            "; Enables or disables all runtime hotkeys.",
        };
        constexpr std::string_view legacyHotkeysEnabledOptions[] = {
            "; Use true to enable all runtime hotkeys or false to disable them.",
        };
        constexpr std::string_view legacyHotkeysEditorial[] = {
            "; Optional runtime controls for quickly testing different settings without restarting the game.",
            "; Intended mainly for comparing modes and finding a preferred configuration; disable for normal use.",
        };
        constexpr std::string_view legacyHotkeysCurrentEnabled[] = {
            "; Optional runtime controls for quickly comparing cinematic and dialogue modes",
            "; without restarting the game.", ";",
            "; true  - enable all runtime hotkeys listed below.",
            "; false - disable all runtime hotkeys. Recommended for normal gameplay.",
        };
        constexpr std::string_view legacyHotkeysEnabledGenerated[] = {
            "; Enables or disables all runtime hotkeys.",
        };
        constexpr std::string_view legacyHotkeySupportedKeys[] = {
            "; Supported keys: F1-F12, 0-9 and A-Z.",
            "; Key used to cycle the cinematic mode for the next cinematic.",
            "; Auto -> Native -> 16:9 -> 21:9 -> 32:9 -> Auto.",
            "; Does not affect a cinematic that is already playing.",
        };
        constexpr std::string_view legacyCinematicAspectCycle[] = {
            "; Key used to cycle the cinematic mode for the next cinematic.",
            "; Auto -> Native -> 16:9 -> 21:9 -> 32:9 -> Auto.",
            "; Does not affect a cinematic that is already playing.",
        };
        constexpr std::string_view legacyDialogueCycle[] = {
            "; Key used to cycle the dialogue zoom mode for the next dialogue.",
            "; Native -> Adaptive -> Reduced -> Disabled -> Native.",
            "; Does not affect a dialogue that is already in progress.",
        };
        constexpr std::string_view legacyGameplayCycle[] = {
            "; Key used to cycle the gameplay correction mode immediately.",
            "; AspectRecalculation -> HorPlus -> AspectRecalculation.",
        };
        constexpr std::string_view legacyGameplayCycleCurrent[] = {
            "; Key used to cycle the gameplay correction mode immediately.",
            "; AspectRecalculation -> HorPlus -> AspectRecalculation.",
            "; Bind a single ordinary keyboard key; modifiers and mouse buttons are not supported.",
        };
        constexpr std::string_view legacyCinematicAspectCycleCurrent[] = {
            "; Key used to cycle the cinematic mode for the next cinematic.",
            "; Auto -> Native -> 16:9 -> 21:9 -> 32:9 -> Auto.",
            "; Does not affect a cinematic that is already playing.",
            "; Bind a single ordinary keyboard key; modifiers and mouse buttons are not supported.",
        };
        constexpr std::string_view legacyCinematicFovCycle[] = {
            "; Key used to cycle the cinematic FOV mode for the next cinematic.",
            "; NativeHorPlus -> GameplayHorPlus -> NativeHorPlus.",
            "; It does not change a cinematic that is already playing.",
        };
        constexpr std::string_view legacyCinematicFovCycleCurrent[] = {
            "; Key used to cycle the cinematic FOV mode for the next cinematic.",
            "; NativeHorPlus -> GameplayHorPlus -> NativeHorPlus.",
            "; Does not affect a cinematic that is already playing.",
            "; Bind a single ordinary keyboard key; modifiers and mouse buttons are not supported.",
        };
        constexpr std::string_view legacyDialogueCycleCurrent[] = {
            "; Key used to cycle the dialogue zoom mode for the next dialogue.",
            "; Native -> Adaptive -> Reduced -> Disabled -> Native.",
            "; Does not affect a dialogue that is already in progress.",
            "; Bind a single ordinary keyboard key; modifiers and mouse buttons are not supported.",
        };
        constexpr std::string_view legacyOverlayToggleCurrent[] = {
            "; Key that opens or closes the overlay, independent of Hotkeys.Enabled.",
            "; Default: Delete. Escape cancels while choosing a key.",
            "; Bindings are single keyboard keys; modifier combinations and mouse buttons are not supported.",
        };
        constexpr std::string_view legacyOverlayToggleInsert[] = {
            "; Key that opens or closes the overlay, independent of Hotkeys.Enabled.",
            "; Default: Insert. Escape cancels while choosing a key.",
            "; Bindings are single ordinary keyboard keys; Escape cancels rebinding.",
        };
        constexpr std::string_view legacyOverlayToggleDelete[] = {
            "; Key that opens or closes the overlay, independent of Hotkeys.Enabled.",
            "; Default: Delete. Escape cancels while choosing a key.",
            "; Bindings are single ordinary keyboard keys; Escape cancels rebinding.",
        };
        constexpr CommentBlock legacyManagedCommentBlocks[] = {
            {"Gameplay", "Enabled", legacyGameplayEnabled, std::size(legacyGameplayEnabled)},
            {"Gameplay", "Enabled", legacyGameplayEnabledGenerated, std::size(legacyGameplayEnabledGenerated)},
            {"Cinematics", "AspectRatio", legacyCinematicAspect, std::size(legacyCinematicAspect)},
            {"Cinematics", "AspectRatio", legacyCinematicAspectGenerated, std::size(legacyCinematicAspectGenerated)},
            {"Dialogue", "Zoom", legacyDialogueZoom, std::size(legacyDialogueZoom)},
            {"Dialogue", "Zoom", legacyDialogueZoomGenerated, std::size(legacyDialogueZoomGenerated)},
            {"Hotkeys", "Enabled", legacyHotkeysEnabled, std::size(legacyHotkeysEnabled)},
            {"Hotkeys", "Enabled", legacyHotkeysEnabledOptions, std::size(legacyHotkeysEnabledOptions)},
            {"Hotkeys", "Enabled", legacyHotkeysEditorial, std::size(legacyHotkeysEditorial)},
            {"Hotkeys", "Enabled", legacyHotkeysCurrentEnabled, std::size(legacyHotkeysCurrentEnabled)},
            {"Hotkeys", "Enabled", legacyHotkeysEnabledGenerated, std::size(legacyHotkeysEnabledGenerated)},
            {"Hotkeys", "GameplayCycle", legacyGameplayCycle, std::size(legacyGameplayCycle)},
            {"Hotkeys", "GameplayCycle", legacyGameplayCycleCurrent, std::size(legacyGameplayCycleCurrent)},
            {"Hotkeys", "CinematicCycle", legacyHotkeySupportedKeys, std::size(legacyHotkeySupportedKeys)},
            {"Hotkeys", "CinematicCycle", legacyCinematicAspectCycle, std::size(legacyCinematicAspectCycle)},
            {"Hotkeys", "CinematicCycle", legacyCinematicAspectCycleCurrent, std::size(legacyCinematicAspectCycleCurrent)},
            {"Hotkeys", "CinematicFovCycle", legacyCinematicFovCycle, std::size(legacyCinematicFovCycle)},
            {"Hotkeys", "CinematicFovCycle", legacyCinematicFovCycleCurrent, std::size(legacyCinematicFovCycleCurrent)},
            {"Hotkeys", "DialogueCycle", legacyDialogueCycleCurrent, std::size(legacyDialogueCycleCurrent)},
            {"Hotkeys", "DialogueCycle", legacyDialogueCycle, std::size(legacyDialogueCycle)},
            {"Hotkeys", "OverlayToggle", legacyOverlayToggleCurrent, std::size(legacyOverlayToggleCurrent)},
            {"Hotkeys", "OverlayToggle", legacyOverlayToggleInsert, std::size(legacyOverlayToggleInsert)},
            {"Hotkeys", "OverlayToggle", legacyOverlayToggleDelete, std::size(legacyOverlayToggleDelete)},
        };

        bool IsCommentOrBlank(const std::string& line)
        {
            const auto trimmed = Trim(line);
            return trimmed.empty() || trimmed.front() == ';';
        }

        std::vector<std::string> RemoveExactCommentBlocks(
            const std::vector<std::string>& body, const std::string& section,
            const CommentBlock* blocks, std::size_t blockCount)
        {
            std::vector<bool> remove(body.size(), false);
            for (std::size_t keyLine = 0; keyLine < body.size(); ++keyLine) {
                const auto trimmedKeyLine = Trim(body[keyLine]);
                const auto separator = trimmedKeyLine.find('=');
                if (separator == std::string::npos) continue;
                const auto keyName = Trim(trimmedKeyLine.substr(0, separator));
                std::size_t runStart = keyLine;
                while (runStart > 0 && IsCommentOrBlank(body[runStart - 1])) --runStart;
                for (std::size_t blockIndex = 0; blockIndex < blockCount; ++blockIndex) {
                    const auto& block = blocks[blockIndex];
                    if (block.section != section || block.key != keyName ||
                        block.lineCount == 0 || block.lineCount > keyLine - runStart)
                        continue;
                    const auto lastStart = keyLine - block.lineCount;
                    for (std::size_t start = runStart; start <= lastStart; ++start) {
                        bool matches = true;
                        for (std::size_t lineIndex = 0; lineIndex < block.lineCount; ++lineIndex) {
                            if (Trim(body[start + lineIndex]) != block.lines[lineIndex]) {
                                matches = false;
                                break;
                            }
                        }
                        if (!matches) continue;
                        for (std::size_t lineIndex = 0; lineIndex < block.lineCount; ++lineIndex)
                            remove[start + lineIndex] = true;
                    }
                }
            }

            std::vector<std::string> result;
            result.reserve(body.size());
            for (std::size_t index = 0; index < body.size(); ++index)
                if (!remove[index]) result.emplace_back(body[index]);
            return result;
        }

        std::vector<std::string> RemoveCurrentManagedCommentBlocks(
            const std::vector<std::string>& body, const std::string& section,
            const ManagedKey* keys, std::size_t keyCount)
        {
            std::vector<CommentBlock> blocks;
            blocks.reserve(keyCount);
            for (std::size_t index = 0; index < keyCount; ++index)
                if (keys[index].commentCount != 0)
                    blocks.push_back({section, keys[index].name, keys[index].comments,
                        keys[index].commentCount});
            return RemoveExactCommentBlocks(body, section, blocks.data(), blocks.size());
        }

        bool IsSectionHeader(const std::string& line, std::string& section);

        std::vector<std::string> RemoveLegacyManagedCommentBlocks(
            const std::vector<std::string>& source)
        {
            std::vector<std::string> result;
            result.reserve(source.size());
            std::size_t index = 0;
            std::string section;
            while (index < source.size()) {
                std::string nextSection;
                if (IsSectionHeader(Trim(source[index]), nextSection)) {
                    section = std::move(nextSection);
                    result.emplace_back(source[index++]);
                    continue;
                }
                const auto start = index++;
                while (index < source.size() && !IsSectionHeader(Trim(source[index]), nextSection))
                    ++index;
                const std::vector<std::string> body(source.begin() + start, source.begin() + index);
                const auto cleaned = RemoveExactCommentBlocks(body, section,
                    legacyManagedCommentBlocks, std::size(legacyManagedCommentBlocks));
                result.insert(result.end(), cleaned.begin(), cleaned.end());
            }
            return result;
        }

        bool IsSectionHeader(const std::string& line, std::string& section)
        {
            if (line.size() < 2 || line.front() != '[' || line.back() != ']') return false;
            section = Trim(line.substr(1, line.size() - 2));
            return true;
        }
    }

    static bool SynchronizeManagedConfigTemplateBody(const std::filesystem::path& path,
        const TemplateLogFunction& log)
    {
        std::ifstream input(path);
        if (!input) return false;

        std::vector<std::string> source;
        std::string line;
        while (std::getline(input, line)) source.push_back(std::move(line));
        input.close();

        const auto sourceWithoutLegacyDescriptions = RemoveLegacyManagedCommentBlocks(source);
        const bool removedLegacyDescriptions = sourceWithoutLegacyDescriptions != source;
        source = sourceWithoutLegacyDescriptions;

        std::string legacyOverlayToggle;
        bool hasOverlayToggle = false;
        bool sourceMigrated = removedLegacyDescriptions;
        std::string sourceSection;
        for (const auto& current : source) {
            const auto trimmed = Trim(current);
            if (IsSectionHeader(trimmed, sourceSection)) continue;
            const auto separator = trimmed.find('=');
            if (separator == std::string::npos) continue;
            const auto key = Trim(trimmed.substr(0, separator));
            if (sourceSection == "Overlay" && key == "ToggleKey") hasOverlayToggle = true;
            if (sourceSection == "Hotkeys" && key == "OverlayToggle")
                legacyOverlayToggle = Trim(trimmed.substr(separator + 1));
        }
        if (!legacyOverlayToggle.empty() && !hasOverlayToggle) {
            bool inserted = false;
            std::string activeSection;
            std::vector<std::string> migrated;
            migrated.reserve(source.size() + 1);
            for (const auto& current : source) {
                std::string section;
                if (IsSectionHeader(Trim(current), section)) {
                    activeSection = section;
                    migrated.push_back(current);
                    if (activeSection == "Overlay" && !inserted) {
                        migrated.emplace_back("ToggleKey=" + legacyOverlayToggle);
                        inserted = true;
                    }
                    continue;
                }
                const auto trimmed = Trim(current);
                const auto separator = trimmed.find('=');
                if (activeSection == "Hotkeys" && separator != std::string::npos &&
                    Trim(trimmed.substr(0, separator)) == "OverlayToggle") {
                    continue;
                }
                migrated.push_back(current);
            }
            if (!inserted) {
                migrated.emplace_back("");
                migrated.emplace_back("[Overlay]");
                migrated.emplace_back("ToggleKey=" + legacyOverlayToggle);
            }
            source = std::move(migrated);
            sourceMigrated = true;
        } else if (!legacyOverlayToggle.empty()) {
            std::string activeSection;
            std::vector<std::string> migrated;
            migrated.reserve(source.size());
            for (const auto& current : source) {
                std::string section;
                if (IsSectionHeader(Trim(current), section)) {
                    activeSection = section;
                    migrated.push_back(current);
                    continue;
                }
                const auto trimmed = Trim(current);
                const auto separator = trimmed.find('=');
                if (activeSection == "Hotkeys" && separator != std::string::npos &&
                    Trim(trimmed.substr(0, separator)) == "OverlayToggle")
                    continue;
                migrated.push_back(current);
            }
            source = std::move(migrated);
            sourceMigrated = true;
        }

        const auto appendSection = [](std::vector<std::string>& output,
            const char* sectionName, const ManagedKey* keys, std::size_t keyCount) {
                if (!output.empty() && !output.back().empty()) output.emplace_back("");
                output.emplace_back("[" + std::string(sectionName) + "]");
                for (std::size_t keyIndex = 0; keyIndex < keyCount; ++keyIndex) {
                    for (std::size_t commentIndex = 0; commentIndex < keys[keyIndex].commentCount; ++commentIndex)
                        output.emplace_back(std::string(keys[keyIndex].comments[commentIndex]));
                    output.emplace_back(std::string(keys[keyIndex].defaultLine));
                }
            };

        const auto repairSection = [](const std::vector<std::string>& body,
            const std::string& section, const ManagedKey* keys, std::size_t keyCount) {
                std::vector<std::string> repaired;
                std::vector<bool> found(keyCount, false);
                const auto withoutCurrentComments = RemoveCurrentManagedCommentBlocks(
                    body, section, keys, keyCount);
                for (const auto& original : withoutCurrentComments) {
                    const auto trimmed = Trim(original);

                    std::size_t matchingKey = keyCount;
                    const auto separator = trimmed.find('=');
                    if (separator != std::string::npos) {
                        const auto key = Trim(trimmed.substr(0, separator));
                        for (std::size_t keyIndex = 0; keyIndex < keyCount; ++keyIndex)
                            if (key == keys[keyIndex].name) matchingKey = keyIndex;
                    }

                    if (matchingKey < keyCount && !found[matchingKey]) {
                        for (std::size_t commentIndex = 0; commentIndex < keys[matchingKey].commentCount; ++commentIndex)
                            repaired.emplace_back(std::string(keys[matchingKey].comments[commentIndex]));
                        found[matchingKey] = true;
                    }
                    repaired.emplace_back(original);
                }

                for (std::size_t keyIndex = 0; keyIndex < keyCount; ++keyIndex) {
                    if (found[keyIndex]) continue;
                    if (!repaired.empty() && !repaired.back().empty()) repaired.emplace_back("");
                    for (std::size_t commentIndex = 0; commentIndex < keys[keyIndex].commentCount; ++commentIndex)
                        repaired.emplace_back(std::string(keys[keyIndex].comments[commentIndex]));
                    repaired.emplace_back(std::string(keys[keyIndex].defaultLine));
                }
                return repaired;
            };

        std::vector<std::string> output;
        bool changed = sourceMigrated;
        std::array<bool, 6> foundSections{};
        std::size_t index = 0;
        while (index < source.size()) {
            std::string section;
            if (!IsSectionHeader(Trim(source[index]), section)) {
                output.emplace_back(source[index++]);
                continue;
            }

            const auto sectionStart = index++;
            std::size_t sectionEnd = index;
            while (sectionEnd < source.size()) {
                std::string nextSection;
                if (IsSectionHeader(Trim(source[sectionEnd]), nextSection)) break;
                ++sectionEnd;
            }

            std::size_t keyCount = 0;
            const auto* keys = KeysForSection(section, keyCount);
            if (keys == nullptr) {
                output.insert(output.end(), source.begin() + sectionStart, source.begin() + sectionEnd);
                index = sectionEnd;
                continue;
            }

            const std::vector<std::string> body(source.begin() + sectionStart + 1,
                source.begin() + sectionEnd);
            const auto repaired = repairSection(body, section, keys, keyCount);
            output.emplace_back(source[sectionStart]);
            output.insert(output.end(), repaired.begin(), repaired.end());
            if (repaired != body) changed = true;

            if (section == "Gameplay") foundSections[0] = true;
            if (section == "Cinematics") foundSections[1] = true;
            if (section == "Dialogue") foundSections[2] = true;
            if (section == "Diagnostics") foundSections[3] = true;
            if (section == "Overlay") foundSections[4] = true;
            if (section == "Hotkeys") foundSections[5] = true;
            index = sectionEnd;
        }

        constexpr const char* sectionNames[] = {
            "Gameplay", "Cinematics", "Dialogue", "Diagnostics", "Overlay", "Hotkeys"};
        for (std::size_t sectionIndex = 0; sectionIndex < std::size(sectionNames); ++sectionIndex) {
            if (foundSections[sectionIndex]) continue;
            std::size_t keyCount = 0;
            const auto* keys = KeysForSection(sectionNames[sectionIndex], keyCount);
            appendSection(output, sectionNames[sectionIndex], keys, keyCount);
            changed = true;
        }

        if (!changed && output == source) return false;
        const auto temporary = std::filesystem::path(path.wstring() + L".tmp");
        {
            std::ofstream file(temporary, std::ios::out | std::ios::trunc);
            if (!file) {
                DeleteFileW(temporary.c_str());
                if (log) log("Config template staging open failed; existing config preserved.");
                return false;
            }
            for (std::size_t outputIndex = 0; outputIndex < output.size(); ++outputIndex) {
                file << output[outputIndex];
                if (outputIndex + 1 < output.size()) file << '\n';
            }
            file.flush();
            if (!file) {
                file.close();
                DeleteFileW(temporary.c_str());
                if (log) log("Config template staging write failed; existing config preserved.");
                return false;
            }
            file.close();
            if (file.fail()) {
                DeleteFileW(temporary.c_str());
                if (log) log("Config template staging close failed; existing config preserved.");
                return false;
            }
        }

        if (MoveFileExW(temporary.c_str(), path.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;

        const auto replaceError = GetLastError();
        DeleteFileW(temporary.c_str());
        if (log) log("Config template replacement failed; existing config preserved. win32Error=" +
            std::to_string(replaceError) + ".");
        return false;
    }

    bool SynchronizeManagedConfigTemplate(const std::filesystem::path& path,
        const TemplateLogFunction& log)
    {
        return RunSerializedConfigUpdate(path, [&] {
            return SynchronizeManagedConfigTemplateBody(path, log);
        });
    }
}
