#include "config_repository.hpp"

#include "feature_config.hpp"

#include <Windows.h>

#include <fstream>
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
            std::vector<std::string>& lines)
        {
            const auto temporary = path.wstring() + L".tmp";
            {
                std::ofstream output(temporary, std::ios::out | std::ios::trunc);
                if (!output) return false;
                for (std::size_t index = 0; index < lines.size(); ++index) {
                    output << lines[index];
                    if (index + 1 < lines.size()) output << '\n';
                }
                output.flush();
                if (!output) { output.close(); DeleteFileW(temporary.c_str()); return false; }
                output.close();
                if (output.fail()) { DeleteFileW(temporary.c_str()); return false; }
            }
            if (MoveFileExW(temporary.c_str(), path.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
            DeleteFileW(temporary.c_str());
            return false;
        }

        bool ReadLines(const std::filesystem::path& path,
            std::vector<std::string>& lines)
        {
            std::ifstream input(path);
            if (!input) return false;
            std::string line;
            while (std::getline(input, line)) lines.push_back(std::move(line));
            return !input.bad();
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

        void SetValue(std::vector<std::string>& lines, const char* targetSection,
            const char* targetKey, const std::string& value)
        {
            std::size_t sectionStart = lines.size();
            std::size_t sectionEnd = lines.size();
            std::string section;
            for (std::size_t index = 0; index < lines.size(); ++index) {
                const auto line = Trim(lines[index]);
                if (line.size() >= 2 && line.front() == '[' && line.back() == ']') {
                    if (section == targetSection && sectionStart != lines.size()) {
                        sectionEnd = index;
                        break;
                    }
                    section = Trim(line.substr(1, line.size() - 2));
                    if (section == targetSection) sectionStart = index;
                } else if (section == targetSection && sectionStart != lines.size()) {
                    const auto separator = line.find('=');
                    if (separator != std::string::npos &&
                        Trim(line.substr(0, separator)) == targetKey)
                        lines[index] = std::string(targetKey) + "=" + value;
                }
            }
            if (sectionStart != lines.size()) {
                if (sectionEnd == lines.size()) sectionEnd = lines.size();
                const auto key = std::string(targetKey) + "=" + value;
                bool present = false;
                for (std::size_t index = sectionStart + 1; index < sectionEnd; ++index) {
                    const auto line = Trim(lines[index]);
                    const auto separator = line.find('=');
                    present |= separator != std::string::npos &&
                        Trim(line.substr(0, separator)) == targetKey;
                }
                if (!present) lines.insert(lines.begin() + sectionEnd, key);
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
        config.overlayLocaleCode = std::string(localization::CanonicalLocaleCode);
        config.overlayLocaleAuto = true;
        config.overlayFontSize = OverlayFontSizeDefault;
        bool createdInitialConfig = false;
        if (!std::filesystem::exists(path)) {
            std::lock_guard<std::mutex> serializedUpdate(PersistenceMutex());
            if (!std::filesystem::exists(path)) {
            std::ofstream created(path, std::ios::out | std::ios::trunc);
            if (!created) return false;
            created << "; STALKER 2 Camera Tweaks and Ultrawide for UE 5.5.4\n"
                << "; Author: Elhait\n"
                << "; GitHub: https://github.com/Elhait/STALKER-2-Camera-Tweaks-and-Ultrawide-for-UE-5.5.4\n"
                << "; Nexus Mods: https://www.nexusmods.com/stalker2heartofchornobyl/mods/2416\n"
                << ";\n";
            created.flush();
            if (!created) return false;
            created.close();
            if (created.fail()) return false;
            createdInitialConfig = true;
            }
        }

        const bool templateSynchronized = synchronizeTemplate && synchronizeTemplate(path);
        if (createdInitialConfig && !templateSynchronized) return false;
        if (templateSynchronized && log)
            log("Config template synchronized: updated managed descriptions and hotkey settings.");

        std::ifstream input(path);
        if (!input) return false;
        std::string section;
        std::string line;
        bool overlayFontSizeValid = false;
        bool overlayToggleKeyExplicit = false;
        while (std::getline(input, line)) {
            line = Trim(line);
            if (line.empty() || line.front() == ';' || line.front() == '#') continue;
            if (line.front() == '[' && line.back() == ']') {
                section = Trim(line.substr(1, line.size() - 2));
                continue;
            }
            const auto separator = line.find('=');
            if (separator == std::string::npos) continue;
            const auto key = Trim(line.substr(0, separator));
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
            if (section == "Cinematics" && key == "AspectRatio") {
                CinematicAspectPolicy policy{};
                if (ParseCinematicAspectPolicy(line.substr(separator + 1), policy)) {
                    config.cinematicAspectPolicy = policy;
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
                    log("Unknown Gameplay.Mode; using HorPlus.");
                continue;
            }
            if (section == "Dialogue" && key == "Zoom") {
                DialogueZoomPolicy policy{};
                if (ParseDialogueZoomPolicy(line.substr(separator + 1), policy))
                    config.dialogueZoomPolicy = policy;
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
            else if (section == "Cinematics" && key == "AspectFix" && !config.cinematicAspectPolicyExplicit)
                config.cinematicAspectPolicy = value ? CinematicAspectPolicy::Auto : CinematicAspectPolicy::Native;
            else if (section == "Features" && key == "GameplayAspectFix") config.gameplayEnabled = value;
            else if (section == "Features" && key == "CinematicAspectFix" && !config.cinematicAspectPolicyExplicit)
                config.cinematicAspectPolicy = value ? CinematicAspectPolicy::Auto : CinematicAspectPolicy::Native;
        }
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
        std::lock_guard<std::mutex> serializedUpdate(PersistenceMutex());
        std::ifstream input(path);
        if (!input) return false;

        std::vector<std::string> lines;
        std::string line;
        while (std::getline(input, line)) lines.push_back(std::move(line));
        input.close();

        bool inSection = false;
        bool replaced = false;
        for (auto& current : lines) {
            const auto trimmed = Trim(current);
            if (trimmed.size() >= 2 && trimmed.front() == '[' && trimmed.back() == ']')
                inSection = Trim(trimmed.substr(1, trimmed.size() - 2)) == targetSection;
            if (!inSection) continue;

            const auto separator = trimmed.find('=');
            if (separator != std::string::npos && Trim(trimmed.substr(0, separator)) == targetKey) {
                current = std::string(targetKey) + "=" + value;
                replaced = true;
            }
        }
        if (!replaced) {
            lines.push_back("");
            lines.push_back(std::string("[") + targetSection + "]");
            lines.push_back(std::string(targetKey) + "=" + value);
        }

        if (ReplaceConfig(path, lines)) return true;

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
        if (!ReadLines(path, lines)) return false;
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
            if (!ReplaceConfig(path, lines)) { migrated = false; return false; }
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
        if (!ReadLines(path, lines)) return false;
        SetValue(lines, "Overlay", "PositionX", std::to_string(positionX));
        SetValue(lines, "Overlay", "PositionY", std::to_string(positionY));
        return ReplaceConfig(path, lines);
    }
}
