#include "config_repository.hpp"
#include "config_line_reader.hpp"

#include "feature_config.hpp"

#include <Windows.h>

#include <algorithm>
#include <cstring>
#include <mutex>
#include <utility>
#include <vector>

namespace config
{
    namespace
    {
        std::mutex& PersistenceMutex()
        {
            static std::mutex mutex;
            return mutex;
        }

        bool ReplaceConfig(const std::filesystem::path& path,
            const std::vector<std::string>& lines,
            const detail::ConfigTextFormat& format)
        {
            const auto temporary = path.wstring() + L".tmp";
            {
                platform::win32::NativeFile output;
                if (!output.CreateExclusive(temporary.c_str())) return false;
                bool written = true;
                for (std::size_t index = 0; index < lines.size(); ++index) {
                    written = written && output.Write(lines[index]);
                    if (index + 1 < lines.size() || format.trailingLineEnding)
                        written = written && output.Write(format.lineEnding);
                }
                const bool flushed = written && output.FlushToDisk();
                const bool closed = output.Close();
                if (!flushed || !closed) { DeleteFileW(temporary.c_str()); return false; }
            }
            if (MoveFileExW(temporary.c_str(), path.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
            DeleteFileW(temporary.c_str());
            return false;
        }

        bool ReadLines(const std::filesystem::path& path,
            std::vector<std::string>& lines,
            detail::ConfigTextFormat* format = nullptr)
        {
            return detail::ReadConfigFile(path.c_str(), lines, format);
        }

        std::string ReadValue(const std::vector<std::string>& lines,
            const char* targetSection, const char* targetKey,
            const std::string& fallback)
        {
            std::string section;
            std::string result = fallback;
            for (const auto& raw : lines) {
                const auto line = Trim(raw);
                if (line.size() >= 2 && line.front() == '[' && line.back() == ']') {
                    section = Trim(line.substr(1, line.size() - 2));
                    continue;
                }
                const auto separator = line.find('=');
                if (section == targetSection && separator != std::string::npos &&
                    Trim(line.substr(0, separator)) == targetKey)
                    result = Trim(line.substr(separator + 1));
            }
            return result;
        }

        std::string FormatAssignment(const std::string& raw)
        {
            const auto line = Trim(raw);
            const auto separator = line.find('=');
            if (separator == std::string::npos) return raw;
            const auto key = Trim(line.substr(0, separator));
            const auto value = Trim(line.substr(separator + 1));
            return key + " = " + value;
        }

        void SetValue(std::vector<std::string>& lines, const char* targetSection,
            const char* targetKey, const std::string& value)
        {
            std::size_t sectionStart = lines.size();
            std::size_t sectionEnd = lines.size();
            bool sectionFound = false;
            bool valueFound = false;
            std::string section;
            for (std::size_t index = 0; index < lines.size(); ++index) {
                const auto line = Trim(lines[index]);
                if (line.size() >= 2 && line.front() == '[' && line.back() == ']') {
                    if (section == targetSection && sectionFound)
                        sectionEnd = index;
                    section = Trim(line.substr(1, line.size() - 2));
                    if (section == targetSection) {
                        sectionStart = index;
                        sectionEnd = lines.size();
                        sectionFound = true;
                    }
                } else if (section == targetSection && sectionStart != lines.size()) {
                    const auto separator = lines[index].find('=');
                    if (separator != std::string::npos &&
                        Trim(lines[index].substr(0, separator)) == targetKey) {
                        const auto valueBegin = lines[index].find_first_not_of(" \t", separator + 1);
                        const auto valueEnd = lines[index].find_last_not_of(" \t");
                        const auto existingValue = valueBegin == std::string::npos ||
                            valueBegin > valueEnd ? std::string{} :
                                lines[index].substr(valueBegin, valueEnd - valueBegin + 1);
                        if (existingValue != value) {
                            if (valueBegin == std::string::npos || valueBegin > valueEnd)
                                lines[index].replace(separator + 1, std::string::npos, value);
                            else
                                lines[index].replace(valueBegin, valueEnd - valueBegin + 1, value);
                        }
                        valueFound = true;
                    }
                }
            }
            if (sectionFound) {
                const auto key = std::string(targetKey) + "=" + value;
                if (!valueFound) lines.insert(lines.begin() + sectionEnd, key);
                return;
            }
            if (!lines.empty() && !lines.back().empty()) lines.emplace_back();
            lines.emplace_back(std::string("[") + targetSection + "]");
            lines.emplace_back(std::string(targetKey) + "=" + value);
        }

        int ParseInteger(const std::string& value, int fallback)
        {
            try {
                std::size_t consumed = 0;
                const int parsed = std::stoi(value, &consumed);
                return consumed == value.size() ? parsed : fallback;
            } catch (...) { return fallback; }
        }
    }

    std::filesystem::path DefaultConfigPath(const std::filesystem::path& moduleDirectory)
    {
        return moduleDirectory / "STALKER2CameraTweaks.ini";
    }

    bool RunSerializedConfigUpdate(const std::filesystem::path& path,
        const std::function<bool()>& update)
    {
        if (path.empty() || !update) return false;
        std::lock_guard<std::mutex> lock(PersistenceMutex());
        return update();
    }

    bool LoadFeatureConfig(const std::filesystem::path& path, FeatureConfig& config,
        const TemplateSynchronizer& synchronizeTemplate, const LogFunction& log)
    {
        config.overlayEnabled = true;
        config.overlayLocaleCode = std::string(localization::CanonicalLocaleCode);
        config.overlayLocaleAuto = true;
        config.overlayFontSize = OverlayFontSizeDefault;
        std::error_code existenceError;
        const bool existedBeforeSynchronization = std::filesystem::exists(path,
            existenceError);
        if (existenceError) return false;
        const bool templateSynchronized = synchronizeTemplate && synchronizeTemplate(path);
        if (!existedBeforeSynchronization && !templateSynchronized) return false;
        if (templateSynchronized && log)
            log("Config template synchronized: updated managed descriptions and hotkey settings.");

        std::string section;
        std::vector<std::string> configLines;
        if (!detail::ReadConfigFile(path.c_str(), configLines)) return false;
        bool overlayFontSizeValid = false;
        bool overlayToggleKeyExplicit = false;
        bool cinematicEnabledExplicit = false;
        bool legacyCinematicEnabledSpecified = false;
        bool legacyCinematicEnabled = true;
        bool dialogueEnabledExplicit = false;
        bool dialogueEnabledValue = true;
        bool dialogueModeValid = false;
        DialogueZoomPolicy dialogueModeValue = DialogueZoomPolicy::Adaptive;
        bool dialogueZoomValid = false;
        DialogueZoomPolicy dialogueZoomValue = DialogueZoomPolicy::Adaptive;
        for (auto& line : configLines) {
            line = Trim(line);
            if (line.empty() || line.front() == ';' || line.front() == '#') continue;
            if (line.front() == '[' && line.back() == ']') {
                section = Trim(line.substr(1, line.size() - 2));
                continue;
            }
            const auto separator = line.find('=');
            if (separator == std::string::npos) continue;
            const auto key = Trim(line.substr(0, separator));
            if (section == "Overlay" && key == "Enabled") {
                bool enabled = true;
                if (ParseBool(line.substr(separator + 1), enabled))
                    config.overlayEnabled = enabled;
                else if (log)
                    log("Unknown Overlay.Enabled; using true.");
                continue;
            }
            if (section == "Overlay" && key == "Language") {
                if (Trim(line.substr(separator + 1)) == "Auto") {
                    config.overlayLocaleAuto = true;
                    continue;
                }
                std::string localeCode;
                if (ParseLocaleCode(line.substr(separator + 1), localeCode)) {
                    config.overlayLocaleCode = std::move(localeCode);
                    config.overlayLocaleAuto = false;
                } else if (log)
                    log("Unknown Overlay.Language; using canonical locale.");
                continue;
            }
            if (const auto* binding = FindHotkeyBinding(section.c_str(), key.c_str())) {
                int hotkey = binding->defaultVirtualKey;
                if (ParseHotkey(line.substr(separator + 1), hotkey)) {
                    SetHotkeyBindingValue(config, binding->id, hotkey);
                    if (binding->id == HotkeyBindingId::OverlayToggle)
                        overlayToggleKeyExplicit = true;
                }
                continue;
            }
            if (section == "Overlay" && key == "FontSize") {
                int fontSize = OverlayFontSizeDefault;
                if (ParseOverlayFontSize(line.substr(separator + 1), fontSize)) {
                    config.overlayFontSize = fontSize;
                    overlayFontSizeValid = true;
                } else if (log) {
                    log("Unknown Overlay.FontSize; using locale initial size.");
                }
                continue;
            }
            if (section == "Cinematics" && key == "Enabled") {
                bool enabled = true;
                if (ParseBool(line.substr(separator + 1), enabled)) {
                    config.cinematicsEnabled = enabled;
                    cinematicEnabledExplicit = true;
                } else if (log) log("Unknown Cinematics.Enabled; using true.");
                continue;
            }
            if (section == "Cinematics" && (key == "Mode" || key == "AspectRatio")) {
                CinematicAspectPolicy policy{};
                if (ParseCinematicAspectPolicy(line.substr(separator + 1), policy)) {
                    if (policy == CinematicAspectPolicy::Native) {
                        config.cinematicAspectPolicy = CinematicAspectPolicy::Auto;
                        legacyCinematicEnabled = false;
                    } else {
                        config.cinematicAspectPolicy = policy;
                        legacyCinematicEnabled = true;
                    }
                    legacyCinematicEnabledSpecified = true;
                    config.cinematicAspectPolicyExplicit = true;
                }
                continue;
            }
            if (section == "Cinematics" && key == "FovMode") {
                CinematicFovMode mode{};
                if (ParseCinematicFovMode(line.substr(separator + 1), mode))
                    config.cinematicFovMode = mode;
                else if (log)
                    log("Unknown Cinematics.FovMode; using GameplayHorPlus.");
                continue;
            }
            if (section == "Gameplay" && key == "Mode") {
                GameplayMode mode{};
                if (ParseGameplayMode(line.substr(separator + 1), mode))
                    config.gameplayMode = mode;
                else if (log)
                    log("Unknown Gameplay.Mode; using RuntimeCombine.");
                continue;
            }
            if (section == "Dialogue" && key == "Enabled") {
                bool enabled = true;
                if (ParseBool(line.substr(separator + 1), enabled)) {
                    dialogueEnabledExplicit = true;
                    dialogueEnabledValue = enabled;
                } else if (log) log("Unknown Dialogue.Enabled; using migrated/default semantics.");
                continue;
            }
            if (section == "Dialogue" && key == "Mode") {
                DialogueZoomPolicy policy{};
                if (ParseDialogueMode(line.substr(separator + 1), policy)) {
                    dialogueModeValid = true;
                    dialogueModeValue = policy;
                }
                continue;
            }
            if (section == "Dialogue" && key == "Zoom") {
                DialogueZoomPolicy policy{};
                if (ParseDialogueZoomPolicy(line.substr(separator + 1), policy)) {
                    dialogueZoomValid = true;
                    dialogueZoomValue = policy;
                }
                continue;
            }
            if (section == "Diagnostics" && key == "Enabled") {
                bool enabled = false;
                if (ParseBool(line.substr(separator + 1), enabled))
                    config.diagnosticsEnabled = enabled;
                continue;
            }
            if (section == "Hotkeys" && key == "Enabled") {
                bool enabled = true;
                if (ParseBool(line.substr(separator + 1), enabled)) config.hotkeysEnabled = enabled;
                continue;
            }
            if (section == "Hotkeys" && key == "OverlayToggle") {
                int hotkey = DefaultHotkeyKey(HotkeyBindingId::OverlayToggle);
                if (!overlayToggleKeyExplicit && ParseHotkey(line.substr(separator + 1), hotkey))
                    config.overlayToggleKey = hotkey;
                continue;
            }
            bool value = true;
            if (!ParseBool(line.substr(separator + 1), value)) continue;
            if (section == "Gameplay" && key == "Enabled") config.gameplayEnabled = value;
            else if (section == "Cinematics" && key == "AspectFix" && !config.cinematicAspectPolicyExplicit) {
                config.cinematicAspectPolicy = CinematicAspectPolicy::Auto;
                legacyCinematicEnabled = value;
                legacyCinematicEnabledSpecified = true;
            }
            else if (section == "Features" && key == "GameplayAspectFix") config.gameplayEnabled = value;
            else if (section == "Features" && key == "CinematicAspectFix" && !config.cinematicAspectPolicyExplicit) {
                config.cinematicAspectPolicy = CinematicAspectPolicy::Auto;
                legacyCinematicEnabled = value;
                legacyCinematicEnabledSpecified = true;
            }
        }
        if (!cinematicEnabledExplicit && legacyCinematicEnabledSpecified)
            config.cinematicsEnabled = legacyCinematicEnabled;
        if (dialogueModeValid)
            config.dialogueZoomPolicy = dialogueModeValue;
        else if (dialogueZoomValid && dialogueZoomValue != DialogueZoomPolicy::Native)
            config.dialogueZoomPolicy = dialogueZoomValue;
        else
            config.dialogueZoomPolicy = DialogueZoomPolicy::Adaptive;
        config.dialogueEnabled = dialogueEnabledExplicit
            ? dialogueEnabledValue
            : dialogueZoomValid
                ? dialogueZoomValue != DialogueZoomPolicy::Native
                : true;
        if (!overlayFontSizeValid) {
            const auto* locale = localization::FindLocaleDescriptor(
                config.overlayLocaleCode);
            config.overlayFontSize = locale
                ? locale->initialFontSize : OverlayFontSizeDefault;
        }
        for (std::size_t index = 0; index < HotkeyBindingRegistry.size(); ++index) {
            const auto id = HotkeyBindingRegistry[index].id;
            const int key = HotkeyBindingValue(config, id);
            bool duplicate = false;
            for (int earlier = 0; earlier < index; ++earlier)
                duplicate |= HotkeyBindingValue(config,
                    HotkeyBindingRegistry[earlier].id) == key;
            if (!duplicate) continue;
            int replacement = HotkeyBindingRegistry[index].defaultVirtualKey;
            bool replacementUsed = true;
            while (replacementUsed) {
                replacementUsed = false;
                for (int earlier = 0; earlier < index; ++earlier)
                    replacementUsed |= HotkeyBindingValue(config,
                        HotkeyBindingRegistry[earlier].id) == replacement;
                if (replacementUsed) {
                    for (const auto& candidate : HotkeyBindingRegistry) {
                        bool used = false;
                        for (int earlier = 0; earlier < index; ++earlier)
                            used |= HotkeyBindingValue(config,
                                HotkeyBindingRegistry[earlier].id) == candidate.defaultVirtualKey;
                        if (!used) {
                            replacement = candidate.defaultVirtualKey;
                            replacementUsed = false;
                            break;
                        }
                    }
                }
            }
            SetHotkeyBindingValue(config, id, replacement);
            if (log) log("Duplicate Hotkeys binding; restored a unique default for the later action.");
        }
        return true;
    }

    bool PersistConfigValue(const std::filesystem::path& path,
        const char* targetSection, const char* targetKey,
        const std::string& value, const LogFunction& log)
    {
        if (!targetSection || !targetKey) return false;
        return PersistConfigValues(path,
            {{targetSection, targetKey, value}}, log);
    }

    bool PersistConfigValues(const std::filesystem::path& path,
        const std::vector<ConfigValueUpdate>& updates, const LogFunction& log)
    {
        if (path.empty() || updates.empty()) return false;
        std::lock_guard<std::mutex> serializedUpdate(PersistenceMutex());
        std::vector<std::string> lines;
        detail::ConfigTextFormat format{};
        if (!ReadLines(path, lines, &format)) return false;
        const auto originalLines = lines;
        for (const auto& update : updates) {
            if (update.section.empty() || update.key.empty()) return false;
            SetValue(lines, update.section.c_str(), update.key.c_str(), update.value);
        }

        if (lines == originalLines) return true;

        if (ReplaceConfig(path, lines, format)) return true;

        const auto replaceError = GetLastError();
        if (log) log("Atomic INI replacement unavailable: win32Error=" +
            std::to_string(replaceError) + "; preserving existing config; no destructive fallback.");
        return false;
    }

    bool LoadOverlayPlacement(const std::filesystem::path& path,
        const std::filesystem::path& legacyPath, int& positionX, int& positionY,
        bool& migrated)
    {
        if (path.empty()) return false;
        std::lock_guard<std::mutex> serializedUpdate(PersistenceMutex());
        std::vector<std::string> lines;
        detail::ConfigTextFormat format{};
        if (!ReadLines(path, lines, &format)) return false;
        migrated = false;
        const int currentX = ParseInteger(ReadValue(lines, "Overlay", "PositionX", "30"), 30);
        const int currentY = ParseInteger(ReadValue(lines, "Overlay", "PositionY", "30"), 30);
        if (ReadValue(lines, "Overlay", "PositionMigrationComplete", "0") != "1") {
            int migratedX = currentX;
            int migratedY = currentY;
            std::vector<std::string> legacyLines;
            if (!legacyPath.empty() && ReadLines(legacyPath, legacyLines)) {
                migratedX = ParseInteger(ReadValue(legacyLines, "Window", "X", std::to_string(currentX)), currentX);
                migratedY = ParseInteger(ReadValue(legacyLines, "Window", "Y", std::to_string(currentY)), currentY);
                migrated = true;
            }
            SetValue(lines, "Overlay", "PositionX", std::to_string(migratedX));
            SetValue(lines, "Overlay", "PositionY", std::to_string(migratedY));
            SetValue(lines, "Overlay", "PositionMigrationComplete", "1");
            if (!ReplaceConfig(path, lines, format)) { migrated = false; return false; }
            positionX = migratedX;
            positionY = migratedY;
            return true;
        }
        positionX = currentX;
        positionY = currentY;
        return true;
    }

    bool PersistOverlayPlacement(const std::filesystem::path& path,
        int positionX, int positionY)
    {
        if (path.empty()) return false;
        std::lock_guard<std::mutex> serializedUpdate(PersistenceMutex());
        std::vector<std::string> lines;
        detail::ConfigTextFormat format{};
        if (!ReadLines(path, lines, &format)) return false;
        const auto originalLines = lines;
        SetValue(lines, "Overlay", "PositionX", std::to_string(positionX));
        SetValue(lines, "Overlay", "PositionY", std::to_string(positionY));
        if (lines == originalLines) return true;
        return ReplaceConfig(path, lines, format);
    }
}
