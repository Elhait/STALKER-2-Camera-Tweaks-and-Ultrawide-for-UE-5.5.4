#include "config_template.hpp"
#include "config_repository.hpp"
#include "config_line_reader.hpp"

#include "canonical_ini_documentation.hpp"
#include "feature_config.hpp"

#include <algorithm>
#include <array>
#include <cwchar>
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
        using canonical_ini_documentation::CinematicEnabledComments;
        using canonical_ini_documentation::CinematicAspectComments;
        using canonical_ini_documentation::CinematicFovComments;
        using canonical_ini_documentation::DialogueEnabledComments;
        using canonical_ini_documentation::DialogueZoomComments;
        using canonical_ini_documentation::DiagnosticsEnabledComments;
        using canonical_ini_documentation::HotkeysEnabledComments;
        using canonical_ini_documentation::HotkeyGameplayCycleComments;
        using canonical_ini_documentation::HotkeyCinematicAspectCycleComments;
        using canonical_ini_documentation::HotkeyCinematicFovCycleComments;
        using canonical_ini_documentation::HotkeyDialogueCycleComments;
        using canonical_ini_documentation::OverlayToggleComments;
        using canonical_ini_documentation::OverlayEnabledComments;
        constexpr std::string_view overlayPositionComments[] = {
            "# ------------------------------------------------------------------------------",
            "# Automatically managed Overlay layout settings",
            "# ------------------------------------------------------------------------------",
            "# Saved Overlay window position in pixels.",
            "# Default: 30, 30",
        };
        constexpr std::string_view overlayMigrationComments[] = {
            "# Internal one-time migration marker for the former separate Overlay INI.",
            "# Default: 0",
        };
        constexpr std::string_view legacyOverlayPositionCurrent[] = {
            "# Automatically managed settings. Do not edit unless resetting the Overlay layout.",
            "# Default: 30",
        };
        constexpr std::string_view overlayLanguageComments[] = {
            "# Auto follows the game's Interface Language when the Overlay opens.",
            "# Or select a locale code from the supported locale registry.",
            "# Default: Auto",
        };
        constexpr std::string_view overlayFontSizeComments[] = {
            "# Overlay font size in pixels. Supported range: 12-24.",
            "# Changes apply immediately and are saved to this configuration file.",
            "# Default: 13",
        };

        constexpr ManagedKey gameplayKeys[] = {
            {"Enabled", "Enabled=true", GameplayEnabledComments, std::size(GameplayEnabledComments)},
            {"Mode", "Mode=RuntimeCombine", GameplayModeComments, std::size(GameplayModeComments)},
        };
        constexpr ManagedKey cinematicKeys[] = {
            {"Enabled", "Enabled=true", CinematicEnabledComments, std::size(CinematicEnabledComments)},
            {"Mode", "Mode=Auto", CinematicAspectComments, std::size(CinematicAspectComments)},
            {"FovMode", "FovMode=GameplayHorPlus", CinematicFovComments, std::size(CinematicFovComments)},
        };
        constexpr ManagedKey dialogueKeys[] = {
            {"Enabled", "Enabled=true", DialogueEnabledComments, std::size(DialogueEnabledComments)},
            {"Mode", "Mode=Adaptive", DialogueZoomComments, std::size(DialogueZoomComments)},
        };
        constexpr ManagedKey diagnosticsKeys[] = {
            {"Enabled", "Enabled=false", DiagnosticsEnabledComments,
                std::size(DiagnosticsEnabledComments)},
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
            {"Enabled", "Enabled=true", OverlayEnabledComments,
                std::size(OverlayEnabledComments)},
            {"ToggleKey", "ToggleKey=VK_2E", OverlayToggleComments,
                std::size(OverlayToggleComments)},
            {"Language", "Language=Auto", overlayLanguageComments,
                std::size(overlayLanguageComments)},
            {"FontSize", "FontSize=13", overlayFontSizeComments,
                std::size(overlayFontSizeComments)},
            {"PositionX", "PositionX=30", overlayPositionComments,
                std::size(overlayPositionComments)},
            {"PositionY", "PositionY=30", nullptr, 0},
            {"PositionMigrationComplete", "PositionMigrationComplete=0",
                overlayMigrationComments, std::size(overlayMigrationComments)},
        };

        constexpr std::string_view canonicalHeader[] = {
            "# ==============================================================================",
            "# STALKER 2 CAMERA TWEAKS",
            "# ==============================================================================",
            "# Author: Elhait",
            "# Engine: Unreal Engine 5.5.4",
            "#",
            "# GitHub: https://github.com/Elhait/STALKER-2-Camera-Tweaks-and-Ultrawide-for-UE-5.5.4",
            "# Nexus: https://www.nexusmods.com/stalker2heartofchornobyl/mods/2416",
            "#",
            "# Edit this file while the game is closed.",
            "# Settings changed in the Overlay are saved automatically where supported.",
            "# Boolean values: true / false.",
            "# ==============================================================================",
        };
        constexpr std::string_view legacyHeaderLines[] = {
            "# STALKER 2 Camera Tweaks and Ultrawide for UE 5.5.4",
            "# Author: Elhait",
            "# GitHub: https://github.com/Elhait/STALKER-2-Camera-Tweaks-and-Ultrawide-for-UE-5.5.4",
            "# Nexus Mods: https://www.nexusmods.com/stalker2heartofchornobyl/mods/2416",
            "#",
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

        bool IsContinuationManagedKey(const std::string& section,
            std::string_view key) noexcept
        {
            return section == "Overlay" && key == "PositionY";
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
        constexpr std::string_view legacyGameplayModeGenerated[] = {
            "; HorPlus - Preserves Gameplay writer FOV changes and transforms each sample using the aspect observed at that writer, including during zoom.",
            ";", "; Examples - Gameplay FOV · no zoom:",
            "; 90° → 16:9: 90°; 21:9: ~106.69°; 32:9: ~126.87°.",
            "; 100° → 16:9: 100°; 21:9: ~116.04°; 32:9: ~134.48°.",
            "; 110° → 16:9: 110°; 21:9: ~124.95°; 32:9: ~141.41°.",
            ";", "; AspectRecalculation - Uses the game's native aspect transition while preserving the selected Gameplay FOV. On custom windowed aspects, restoring Auto may return the window to its native aspect and size.",
            ";", "; RuntimeCombine - Recommended. When they differ, synchronizes the Gameplay camera aspect to the current viewport aspect, confirms the change, and only then applies Hor+ FOV correction. If synchronization cannot be confirmed, Hor+ is not applied."
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
        constexpr std::string_view legacyCinematicAspectModeCurrent[] = {
            "# Available modes:",
            "#   Auto - Uses the current runtime viewport aspect for cinematic framing. Supports arbitrary valid aspect ratios.",
            "#   Native - Native behavior is controlled by the separate Enabled switch; framing choices are Auto, 16:9, 21:9 and 32:9.",
            "#   16:9 - Uses 16:9 framing for cinematics, regardless of the display aspect.",
            "#   21:9 - Uses 21:9 framing for cinematics, regardless of the display aspect.",
            "#   32:9 - Uses 32:9 framing for cinematics, regardless of the display aspect.",
            "# Example: 32:9 on a 16:9 display produces wider framing with black bars above and below.",
            "# Default: Auto"
        };
        constexpr std::string_view legacyCinematicAspectModeDuplicateExample[] = {
            "# Available modes:",
            "#   Auto - Uses the current runtime viewport aspect for cinematic framing. Supports arbitrary valid aspect ratios.",
            "#   Native - Native behavior is controlled by the separate Enabled switch; framing choices are Auto, 16:9, 21:9 and 32:9.",
            "#   16:9 - Uses 16:9 framing for cinematics, regardless of the display aspect.",
            "#   21:9 - Uses 21:9 framing for cinematics, regardless of the display aspect.",
            "#   32:9 - Uses 32:9 framing for cinematics, regardless of the display aspect.",
            "# Example: Example: 32:9 on a 16:9 display produces wider framing with black bars above and below.",
            "# Default: Auto"
        };
        constexpr std::string_view legacyCinematicEnabledGenerated[] = {
            "; true - When enabled, applies the selected cinematic framing and FOV corrections. When disabled, Camera Tweaks leaves cinematic camera output native.",
            "; false - use the game's native cinematic camera output without Camera Tweaks corrections."
        };
        constexpr std::string_view legacyCinematicFovGenerated[] = {
            "; GameplayHorPlus - Follows current Gameplay FOV while preserving cinematic variation. Falls back to NativeHorPlus if needed context is unavailable.",
            ";", "; Examples - Cinematic FOV 90° · Gameplay FOV inputs:",
            "; 90° → 16:9: 90°; 21:9: ~106.69°; 32:9: ~126.87°.",
            "; 112.6° → 16:9: 112.6°; 21:9: 118.51°; 32:9: ~143.13°.",
            ";", "; NativeHorPlus - Applies HorPlus to the authored cinematic FOV, independently of the player's Gameplay FOV setting.",
            ";", "; Examples - Cinematic FOV 90° · results by aspect:",
            "; 16:9: 90°; 21:9: ~106.69°; 32:9: ~126.87°."
        };
        constexpr std::string_view legacyDialogueEnabledGenerated[] = {
            "; true - apply the selected Dialogue mode after an accepted dialogue start.",
            "; false - keep native camera output while continuing to observe dialogue lifecycle."
        };
        constexpr std::string_view legacyDiagnosticsEnabled[] = {
            "# Optional read-only runtime telemetry.",
            "# Enable CameraState, ZOOM and HorPlus FOV telemetry.",
            "# Default: false",
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
        constexpr std::string_view legacyDialogueZoom[] = {
            "; Controls the native dialogue camera zoom.",
            "; Native   - use the game's original dialogue zoom, currently targeting 70°.",
            "; Adaptive - preserve the native optical zoom strength relative to the current gameplay FOV.",
            "; Reduced  - apply half of the Adaptive optical zoom strength.",
            ";            Example: 110° gameplay FOV -> Adaptive ≈90°, Reduced ≈100°.",
            "; Disabled - keep the current gameplay FOV during dialogue.",
        };
        constexpr std::string_view legacyDialogueZoomGenerated[] = {
            "; Adaptive - Preserve the game's original optical zoom strength relative to current Gameplay FOV.",
            "; Examples - Gameplay FOV → dialogue FOV:",
            "; 90° → 70°.",
            "; 110° → approximately 90°.",
            ";",
            "; Adaptive - Preserve the game's original optical zoom strength relative to current Gameplay FOV.",
            "; Examples - Gameplay FOV → dialogue FOV:",
            "; 90° → 70°.",
            "; 110° → approximately 90°.",
            ";",
            "; Reduced - Use half of the Adaptive optical zoom strength.",
            "; Examples - Gameplay FOV → dialogue FOV:",
            "; 90° → approximately 80°.",
            "; 110° → approximately 100°.",
            ";",
            "; Disabled - Disable dialogue zoom and keep current Gameplay FOV.",
            "; Examples - Gameplay FOV → dialogue FOV:",
            "; 90° → 90°.",
            "; 110° → 110°."
        };
        constexpr std::string_view legacyDialogueZoomModeCurrent[] = {
            "# Available modes:",
            "#   Native - use the game's original Dialogue zoom behavior.",
            "#   Adaptive - Preserve the game's original optical zoom strength relative to current Gameplay FOV.",
            "# Examples - Gameplay FOV → dialogue FOV:",
            "# 90° → 70°.",
            "# 110° → approximately 90°.",
            "#   Reduced - Use half of the Adaptive optical zoom strength.",
            "# 90° → approximately 80°.",
            "# 110° → approximately 100°.",
            "#   Disabled - Disable dialogue zoom and keep current Gameplay FOV.",
            "# 90° → 90°.",
            "# 110° → 110°.",
            "# Default: Adaptive"
        };
        constexpr std::string_view legacyOverlayEnabled[] = {
            "; Controls the optional Overlay subsystem.",
            "; Startup-only. Restart the game after changing this value.",
            "; false - disable the Overlay completely; camera corrections remain active.",
            "; true - enable the Overlay."
        };
        constexpr std::string_view legacyGameplayCycle[] = {
            "; Key used to cycle the gameplay correction mode immediately.",
            "; AspectRecalculation -> HorPlus -> RuntimeCombine -> AspectRecalculation.",
        };
        constexpr std::string_view legacyGameplayCycleCurrent[] = {
            "; Key used to cycle the gameplay correction mode immediately.",
            "; AspectRecalculation -> HorPlus -> RuntimeCombine -> AspectRecalculation.",
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
            "; Adaptive -> Reduced -> Disabled -> Adaptive.",
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
            {"Gameplay", "Mode", legacyGameplayModeGenerated, std::size(legacyGameplayModeGenerated)},
            {"Cinematics", "AspectRatio", legacyCinematicAspect, std::size(legacyCinematicAspect)},
            {"Cinematics", "AspectRatio", legacyCinematicAspectGenerated, std::size(legacyCinematicAspectGenerated)},
            {"Cinematics", "Mode", legacyCinematicAspectModeCurrent,
                std::size(legacyCinematicAspectModeCurrent)},
            {"Cinematics", "Mode", legacyCinematicAspectModeDuplicateExample,
                std::size(legacyCinematicAspectModeDuplicateExample)},
            {"Cinematics", "Enabled", legacyCinematicEnabledGenerated, std::size(legacyCinematicEnabledGenerated)},
            {"Cinematics", "FovMode", legacyCinematicFovGenerated, std::size(legacyCinematicFovGenerated)},
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
            {"Dialogue", "Zoom", legacyDialogueZoom, std::size(legacyDialogueZoom)},
            {"Dialogue", "Enabled", legacyDialogueEnabledGenerated, std::size(legacyDialogueEnabledGenerated)},
            {"Dialogue", "Mode", legacyDialogueZoomGenerated,
                std::size(legacyDialogueZoomGenerated)},
            {"Dialogue", "Mode", legacyDialogueZoomModeCurrent,
                std::size(legacyDialogueZoomModeCurrent)},
            {"Diagnostics", "Enabled", legacyDiagnosticsEnabled,
                std::size(legacyDiagnosticsEnabled)},
            {"Overlay", "Enabled", legacyOverlayEnabled, std::size(legacyOverlayEnabled)},
            {"Overlay", "PositionX", legacyOverlayPositionCurrent,
                std::size(legacyOverlayPositionCurrent)},
            {"Overlay", "PositionY", legacyOverlayPositionCurrent,
                std::size(legacyOverlayPositionCurrent)},
            {"Hotkeys", "OverlayToggle", legacyOverlayToggleCurrent, std::size(legacyOverlayToggleCurrent)},
            {"Hotkeys", "OverlayToggle", legacyOverlayToggleInsert, std::size(legacyOverlayToggleInsert)},
            {"Hotkeys", "OverlayToggle", legacyOverlayToggleDelete, std::size(legacyOverlayToggleDelete)},
        };

        bool IsSectionHeader(const std::string& line, std::string& section);

        bool IsCommentOrBlank(const std::string& line)
        {
            const auto trimmed = Trim(line);
            return trimmed.empty() || trimmed.front() == ';' || trimmed.front() == '#';
        }

        std::string NormalizeCommentPrefix(std::string line)
        {
            const auto first = line.find_first_not_of(" \t");
            if (first != std::string::npos && line[first] == ';')
                line[first] = '#';
            return line;
        }

        bool EquivalentComment(std::string actual, std::string expected)
        {
            actual = Trim(std::move(actual));
            expected = Trim(std::move(expected));
            if (!actual.empty() && (actual.front() == ';' || actual.front() == '#'))
                actual = Trim(actual.substr(1));
            if (!expected.empty() && (expected.front() == ';' || expected.front() == '#'))
                expected = Trim(expected.substr(1));
            return actual == expected;
        }

        std::vector<std::string> NormalizeHeader(
            const std::vector<std::string>& source)
        {
            std::vector<std::string> retainedPreamble;
            std::size_t firstSection = source.size();
            for (std::size_t index = 0; index < source.size(); ++index) {
                std::string section;
                if (IsSectionHeader(Trim(source[index]), section)) {
                    firstSection = index;
                    break;
                }
                const auto line = Trim(source[index]);
                const bool oldHeader = std::find(std::begin(legacyHeaderLines),
                    std::end(legacyHeaderLines), line) != std::end(legacyHeaderLines);
                const bool canonicalLine = std::find(std::begin(canonicalHeader),
                    std::end(canonicalHeader), line) != std::end(canonicalHeader);
                const bool emptyComment = line == "#" || line == ";";
                if (!oldHeader && !canonicalLine && !emptyComment && !line.empty())
                    retainedPreamble.emplace_back(source[index]);
            }

            std::vector<std::string> result;
            result.reserve(std::size(canonicalHeader) + retainedPreamble.size() +
                source.size() - firstSection + 2);
            for (const auto line : canonicalHeader) result.emplace_back(line);
            if (!retainedPreamble.empty()) {
                result.emplace_back("");
                result.insert(result.end(), retainedPreamble.begin(),
                    retainedPreamble.end());
            }
            result.emplace_back("");
            result.insert(result.end(), source.begin() + firstSection, source.end());
            return result;
        }

        std::string FormatAssignment(const std::string& line)
        {
            const auto trimmed = Trim(line);
            const auto separator = trimmed.find('=');
            if (separator == std::string::npos) return line;
            const auto key = Trim(trimmed.substr(0, separator));
            const auto value = Trim(trimmed.substr(separator + 1));
            return key + " = " + value;
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
                            if (!EquivalentComment(body[start + lineIndex],
                                    std::string(block.lines[lineIndex]))) {
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

        std::vector<std::string> RemoveManagedDocumentationPreambles(
            const std::vector<std::string>& body, const ManagedKey* keys,
            std::size_t keyCount)
        {
            // Legacy templates have no ownership delimiters. Treat any remaining
            // contiguous comment preamble of a supported key as stale module
            // documentation, then emit the explicit canonical block below.
            std::vector<bool> remove(body.size(), false);
            for (std::size_t keyLine = 0; keyLine < body.size(); ++keyLine) {
                const auto trimmed = Trim(body[keyLine]);
                const auto separator = trimmed.find('=');
                if (separator == std::string::npos) continue;
                const auto key = Trim(trimmed.substr(0, separator));
                bool managed = false;
                for (std::size_t keyIndex = 0; keyIndex < keyCount; ++keyIndex)
                    managed |= key == keys[keyIndex].name;
                if (!managed) continue;

                std::size_t runStart = keyLine;
                while (runStart > 0 && IsCommentOrBlank(body[runStart - 1])) --runStart;
                for (std::size_t index = runStart; index < keyLine; ++index)
                    remove[index] = true;
            }

            std::vector<std::string> result;
            result.reserve(body.size());
            for (std::size_t index = 0; index < body.size(); ++index)
                if (!remove[index]) result.emplace_back(body[index]);
            return result;
        }

        void AppendManagedDocumentation(std::vector<std::string>& output,
            std::string_view section, const ManagedKey& key)
        {
            if (!key.commentCount) return;
            const auto identity = std::string(section) + "." + std::string(key.name);
            output.emplace_back("# <" + identity + ">");
            for (std::size_t index = 0; index < key.commentCount; ++index)
                output.emplace_back(key.comments[index]);
            output.emplace_back("# </" + identity + ">");
        }

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
            return !section.empty();
        }

        bool SanitizeRecoverableConfig(std::vector<std::string>& lines)
        {
            bool malformed = false;
            bool hasSection = false;
            bool hasKnownSection = false;
            std::string activeSection;
            std::vector<std::string> recoverable;
            recoverable.reserve(lines.size());
            if (!lines.empty() && lines.front().compare(0, 3, "\xEF\xBB\xBF", 3) == 0)
                lines.front().erase(0, 3);
            for (const auto& line : lines) {
                if (line.find('\0') != std::string::npos) {
                    malformed = true;
                    continue;
                }
                const auto trimmed = Trim(line);
                if (trimmed.empty() || trimmed.front() == ';' || trimmed.front() == '#') {
                    recoverable.push_back(line);
                    continue;
                }

                std::string section;
                if (IsSectionHeader(trimmed, section)) {
                    activeSection = std::move(section);
                    hasSection = true;
                    hasKnownSection = hasKnownSection || activeSection == "Gameplay" ||
                        activeSection == "Cinematics" || activeSection == "Dialogue" ||
                        activeSection == "Diagnostics" || activeSection == "Hotkeys" ||
                        activeSection == "Overlay" || activeSection == "Features";
                    recoverable.push_back(line);
                    continue;
                }
                if (trimmed.front() == '[' || trimmed.back() == ']') {
                    malformed = true;
                    activeSection.clear();
                    continue;
                }

                const auto separator = trimmed.find('=');
                if (separator == std::string::npos ||
                    Trim(trimmed.substr(0, separator)).empty() || activeSection.empty()) {
                    malformed = true;
                    continue;
                }
                recoverable.push_back(line);
            }
            if (!hasSection || !hasKnownSection) malformed = true;
            if (malformed) lines = std::move(recoverable);
            return malformed;
        }

        bool CreateConfigBackup(const std::filesystem::path& path,
            std::filesystem::path& backupPath)
        {
            SYSTEMTIME now{};
            GetSystemTime(&now);
            wchar_t timestamp[32]{};
            swprintf_s(timestamp, L"%04u%02u%02u-%02u%02u%02u-%03u",
                static_cast<unsigned int>(now.wYear),
                static_cast<unsigned int>(now.wMonth),
                static_cast<unsigned int>(now.wDay),
                static_cast<unsigned int>(now.wHour),
                static_cast<unsigned int>(now.wMinute),
                static_cast<unsigned int>(now.wSecond),
                static_cast<unsigned int>(now.wMilliseconds));
            const auto base = path.parent_path() /
                (path.filename().wstring() + L".backup-" + timestamp);
            for (unsigned int suffix = 0; suffix != 100; ++suffix) {
                const std::wstring suffixText = suffix == 0 ? std::wstring{} :
                    L"-" + std::to_wstring(suffix);
                backupPath = std::filesystem::path(base.wstring() + suffixText + L".ini");
                if (CopyFileW(path.c_str(), backupPath.c_str(), TRUE)) return true;
                const auto error = GetLastError();
                if (error != ERROR_FILE_EXISTS && error != ERROR_ALREADY_EXISTS)
                    return false;
            }
            return false;
        }
    }

    static bool SynchronizeManagedConfigTemplateBody(const std::filesystem::path& path,
        const TemplateLogFunction& log)
    {
        std::vector<std::string> source;
        detail::ConfigTextFormat format{};
        std::error_code existenceError;
        const bool fileExists = std::filesystem::exists(path, existenceError);
        if (existenceError) return false;
        if (fileExists) {
            if (!detail::ReadConfigFile(path.c_str(), source, &format)) return false;
        }
        const auto original = source;
        const bool requiresRecoveryBackup = fileExists &&
            SanitizeRecoverableConfig(source);

        const auto sourceWithoutLegacyDescriptions = RemoveLegacyManagedCommentBlocks(source);
        source = sourceWithoutLegacyDescriptions;
        for (auto& current : source)
            current = NormalizeCommentPrefix(std::move(current));
        source.erase(std::remove_if(source.begin(), source.end(), [](const std::string& line) {
            const auto trimmed = Trim(line);
            return trimmed.empty() || trimmed.front() == '#' || trimmed.front() == ';';
        }), source.end());
        const auto normalizedHeader = NormalizeHeader(source);
        source = normalizedHeader;

        // Resolve the legacy Gameplay switch before canonical defaults exist.
        // A valid canonical value is authoritative regardless of section order.
        bool canonicalGameplayEnabled = false;
        bool legacyGameplayEnabledFound = false;
        bool migratedGameplayEnabled = true;
        {
            std::string section;
            for (const auto& current : source) {
                const auto line = Trim(current);
                if (IsSectionHeader(line, section)) continue;
                const auto separator = line.find('=');
                if (separator == std::string::npos) continue;
                const auto key = Trim(line.substr(0, separator));
                bool value{};
                if (!ParseBool(line.substr(separator + 1), value)) continue;
                if (section == "Gameplay" && key == "Enabled")
                    canonicalGameplayEnabled = true;
                else if (section == "Features" && key == "GameplayAspectFix") {
                    legacyGameplayEnabledFound = true;
                    migratedGameplayEnabled = value;
                }
            }
        }
        if (legacyGameplayEnabledFound) {
            std::vector<std::string> migrated;
            std::string section;
            for (const auto& current : source) {
                const auto line = Trim(current);
                if (IsSectionHeader(line, section)) {
                    migrated.push_back(current);
                    continue;
                }
                const auto separator = line.find('=');
                const auto key = separator == std::string::npos
                    ? std::string{} : Trim(line.substr(0, separator));
                if (section == "Features" && key == "GameplayAspectFix") continue;
                if (!canonicalGameplayEnabled && section == "Gameplay" &&
                    key == "Enabled") continue;
                migrated.push_back(current);
            }
            if (!canonicalGameplayEnabled) {
                migrated.emplace_back("[Gameplay]");
                migrated.emplace_back(migratedGameplayEnabled
                    ? "Enabled=true" : "Enabled=false");
            }
            source = std::move(migrated);
        }

        std::string legacyOverlayToggle;
        bool hasOverlayToggle = false;
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
        }

        // Migrate the former Cinematics.AspectRatio setting into the new
        // Enabled + Mode model before inserting defaults. This preserves an
        // old Native selection instead of letting the new Enabled=true
        // default accidentally override it.
        bool cinematicEnabledFound = false;
        bool cinematicEnabledValid = false;
        bool cinematicAspectFound = false;
        bool cinematicModeValid = false;
        bool cinematicAspectValid = false;
        bool legacyAspectFixFound = false;
        bool legacyAspectFix = true;
        std::string cinematicLegacyMode;
        {
            std::string activeSection;
            std::string activeLegacySection;
            for (const auto& current : source) {
                std::string section;
                if (IsSectionHeader(Trim(current), section)) {
                    activeSection = std::move(section);
                    activeLegacySection = activeSection;
                    continue;
                }
                const auto trimmed = Trim(current);
                const auto separator = trimmed.find('=');
                if (separator == std::string::npos) continue;
                const auto key = Trim(trimmed.substr(0, separator));
                const auto value = Trim(trimmed.substr(separator + 1));
                if (activeSection == "Cinematics") {
                    if (key == "Enabled") {
                        cinematicEnabledFound = true;
                        bool parsed = false;
                        if (ParseBool(value, parsed)) cinematicEnabledValid = true;
                    } else if (key == "Mode") {
                        CinematicAspectPolicy parsed{};
                        if (ParseCinematicAspectPolicy(value, parsed)) {
                            cinematicModeValid = true;
                            cinematicLegacyMode = value;
                        }
                    } else if (key == "AspectRatio") {
                        cinematicAspectFound = true;
                        CinematicAspectPolicy parsed{};
                        if (ParseCinematicAspectPolicy(value, parsed)) {
                            cinematicAspectValid = true;
                            if (!cinematicModeValid) cinematicLegacyMode = value;
                        }
                    } else if (key == "AspectFix") {
                        bool parsed = true;
                        if (ParseBool(value, parsed)) {
                            legacyAspectFixFound = true;
                            legacyAspectFix = parsed;
                        }
                    }
                } else if (activeLegacySection == "Features" && key == "CinematicAspectFix") {
                    bool parsed = true;
                    if (ParseBool(value, parsed)) {
                        legacyAspectFixFound = true;
                        legacyAspectFix = parsed;
                    }
                }
            }
        }
        const bool legacyPolicyFound = cinematicModeValid || cinematicAspectValid;
        const bool legacyNative = _stricmp(cinematicLegacyMode.c_str(), "Native") == 0;
        const bool cinematicEnabledMigrationNeeded = !cinematicEnabledValid &&
            (legacyPolicyFound || legacyAspectFixFound);
        if (cinematicEnabledMigrationNeeded || cinematicAspectFound ||
            (cinematicModeValid && legacyNative) ||
            (cinematicEnabledFound && !cinematicEnabledValid && legacyPolicyFound)) {
            const std::string migratedEnabled = legacyPolicyFound
                ? (legacyNative ? "false" : "true")
                : (legacyAspectFix ? "true" : "false");
            std::vector<std::string> migrated;
            migrated.reserve(source.size() + (cinematicEnabledMigrationNeeded ? 1 : 0));
            std::string activeSection;
            bool insertedEnabled = false;
            for (const auto& current : source) {
                std::string section;
                if (IsSectionHeader(Trim(current), section)) {
                    activeSection = std::move(section);
                    migrated.push_back(current);
                    if (cinematicEnabledMigrationNeeded && activeSection == "Cinematics" &&
                        !insertedEnabled) {
                        migrated.emplace_back("Enabled=" + migratedEnabled);
                        insertedEnabled = true;
                    }
                    continue;
                }
                const auto trimmed = Trim(current);
                const auto separator = trimmed.find('=');
                if (activeSection == "Cinematics" && separator != std::string::npos) {
                    const auto key = Trim(trimmed.substr(0, separator));
                    const auto value = Trim(trimmed.substr(separator + 1));
                    if (key == "AspectRatio") {
                        CinematicAspectPolicy parsed{};
                        if (!cinematicModeValid &&
                            ParseCinematicAspectPolicy(value, parsed)) {
                            migrated.emplace_back("Mode=" +
                                (parsed == CinematicAspectPolicy::Native
                                    ? std::string("Auto")
                                    : std::string(CinematicAspectPolicyName(parsed))));
                        }
                        continue;
                    }
                    if (key == "Mode") {
                        CinematicAspectPolicy parsed{};
                        if (ParseCinematicAspectPolicy(value, parsed) &&
                            parsed == CinematicAspectPolicy::Native) {
                            migrated.emplace_back("Mode=Auto");
                            continue;
                        }
                    }
                    if (key == "AspectFix") continue;
                } else if (activeSection == "Features" && separator != std::string::npos &&
                    Trim(trimmed.substr(0, separator)) == "CinematicAspectFix") {
                    continue;
                }
                migrated.push_back(current);
            }
            if (cinematicEnabledMigrationNeeded && !insertedEnabled) {
                migrated.emplace_back("[Cinematics]");
                migrated.emplace_back("Enabled=" + migratedEnabled);
            }
            source = std::move(migrated);
        }

        // Resolve Dialogue's former Zoom key before managed defaults are
        // inserted. Build both canonical keys in one source rewrite so an
        // interrupted migration can never leave a half-updated file.
        bool dialogueEnabledFound = false;
        bool dialogueEnabledValid = false;
        bool dialogueEnabledValue = true;
        bool dialogueModeFound = false;
        bool dialogueModeValid = false;
        DialogueZoomPolicy dialogueModeValue = DialogueZoomPolicy::Adaptive;
        bool dialogueZoomFound = false;
        bool dialogueZoomValid = false;
        DialogueZoomPolicy dialogueZoomValue = DialogueZoomPolicy::Adaptive;
        bool dialogueModeNeedsCanonicalization = false;
        {
            std::string activeSection;
            for (const auto& current : source) {
                std::string section;
                if (IsSectionHeader(Trim(current), section)) {
                    activeSection = std::move(section);
                    continue;
                }
                if (activeSection != "Dialogue") continue;
                const auto trimmed = Trim(current);
                const auto separator = trimmed.find('=');
                if (separator == std::string::npos) continue;
                const auto key = Trim(trimmed.substr(0, separator));
                const auto value = Trim(trimmed.substr(separator + 1));
                if (key == "Enabled") {
                    dialogueEnabledFound = true;
                    bool parsed = false;
                    if (ParseBool(value, parsed)) {
                        dialogueEnabledValid = true;
                        dialogueEnabledValue = parsed;
                    }
                } else if (key == "Mode") {
                    dialogueModeFound = true;
                    DialogueZoomPolicy parsed{};
                    if (ParseDialogueMode(value, parsed)) {
                        dialogueModeValid = true;
                        dialogueModeValue = parsed;
                    } else {
                        dialogueModeNeedsCanonicalization = true;
                    }
                } else if (key == "Zoom") {
                    dialogueZoomFound = true;
                    DialogueZoomPolicy parsed{};
                    if (ParseDialogueZoomPolicy(value, parsed)) {
                        dialogueZoomValid = true;
                        dialogueZoomValue = parsed;
                    }
                }
            }
        }
        if (dialogueZoomFound ||
            (dialogueModeFound && dialogueModeNeedsCanonicalization) ||
            (dialogueEnabledFound && !dialogueEnabledValid)) {
            const auto resolvedMode = dialogueModeValid
                ? dialogueModeValue
                : dialogueZoomValid && dialogueZoomValue != DialogueZoomPolicy::Native
                    ? dialogueZoomValue
                    : DialogueZoomPolicy::Adaptive;
            const bool resolvedEnabled = dialogueEnabledValid
                ? dialogueEnabledValue
                : dialogueZoomValid
                    ? dialogueZoomValue != DialogueZoomPolicy::Native
                    : true;
            std::vector<std::string> migrated;
            migrated.reserve(source.size() + 1);
            std::string activeSection;
            bool insertedCanonical = false;
            for (const auto& current : source) {
                std::string section;
                if (IsSectionHeader(Trim(current), section)) {
                    activeSection = std::move(section);
                    migrated.push_back(current);
                    if (activeSection == "Dialogue" && !insertedCanonical) {
                        migrated.emplace_back(std::string("Enabled=") +
                            (resolvedEnabled ? "true" : "false"));
                        migrated.emplace_back(std::string("Mode=") +
                            DialogueZoomPolicyName(resolvedMode));
                        insertedCanonical = true;
                    }
                    continue;
                }
                const auto trimmed = Trim(current);
                const auto separator = trimmed.find('=');
                if (activeSection == "Dialogue" && separator != std::string::npos) {
                    const auto key = Trim(trimmed.substr(0, separator));
                    if (key == "Enabled" || key == "Mode" || key == "Zoom")
                        continue;
                }
                migrated.push_back(current);
            }
            if (!insertedCanonical) {
                migrated.emplace_back("");
                migrated.emplace_back("[Dialogue]");
                migrated.emplace_back(std::string("Enabled=") +
                    (resolvedEnabled ? "true" : "false"));
                migrated.emplace_back(std::string("Mode=") +
                    DialogueZoomPolicyName(resolvedMode));
            }
            source = std::move(migrated);
        }

        const auto appendSection = [](std::vector<std::string>& output,
            const char* sectionName, const ManagedKey* keys, std::size_t keyCount) {
                if (!output.empty() && !output.back().empty()) output.emplace_back("");
                output.emplace_back("[" + std::string(sectionName) + "]");
                for (std::size_t keyIndex = 0; keyIndex < keyCount; ++keyIndex) {
                    if (keyIndex > 0 &&
                        !IsContinuationManagedKey(sectionName, keys[keyIndex].name))
                        output.emplace_back("");
                    AppendManagedDocumentation(output, sectionName, keys[keyIndex]);
                    output.emplace_back(FormatAssignment(
                        std::string(keys[keyIndex].defaultLine)));
                }
            };

        const auto repairSection = [](const std::vector<std::string>& body,
            const std::string& section, const ManagedKey* keys, std::size_t keyCount) {
                std::vector<std::string> repaired;
                std::vector<bool> found(keyCount, false);
                const auto withoutKnownComments = RemoveCurrentManagedCommentBlocks(
                    body, section, keys, keyCount);
                const auto withoutCurrentComments = RemoveManagedDocumentationPreambles(
                    withoutKnownComments, keys, keyCount);
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
                        if (!repaired.empty() && !repaired.back().empty() &&
                            !IsContinuationManagedKey(section, keys[matchingKey].name))
                            repaired.emplace_back("");
                        AppendManagedDocumentation(repaired, section, keys[matchingKey]);
                        found[matchingKey] = true;
                    }
                    repaired.emplace_back(separator != std::string::npos
                        ? FormatAssignment(original) : original);
                }

                for (std::size_t keyIndex = 0; keyIndex < keyCount; ++keyIndex) {
                    if (found[keyIndex]) continue;
                    if (!repaired.empty() && !repaired.back().empty() &&
                        !IsContinuationManagedKey(section, keys[keyIndex].name))
                        repaired.emplace_back("");
                    AppendManagedDocumentation(repaired, section, keys[keyIndex]);
                    repaired.emplace_back(FormatAssignment(
                        std::string(keys[keyIndex].defaultLine)));
                }
                if (section == "Overlay") {
                    const auto enabled = std::find_if(repaired.begin(), repaired.end(),
                        [](const std::string& line) {
                            const auto trimmed = Trim(line);
                            const auto separator = trimmed.find('=');
                            return separator != std::string::npos &&
                                Trim(trimmed.substr(0, separator)) == "Enabled";
                        });
                    const auto enabledKey = std::find_if(keys, keys + keyCount,
                        [](const ManagedKey& key) { return key.name == "Enabled"; });
                    if (enabled != repaired.end() && enabledKey != keys + keyCount) {
                        const auto documentationStart = std::find(repaired.begin(), enabled,
                            "# <Overlay.Enabled>");
                        if (documentationStart != enabled) {
                            std::vector<std::string> enabledBlock(
                                documentationStart, enabled + 1);
                            repaired.erase(documentationStart, enabled + 1);
                            if (!repaired.empty() && !repaired.front().empty())
                                enabledBlock.emplace_back("");
                            repaired.insert(repaired.begin(), enabledBlock.begin(),
                                enabledBlock.end());
                        }
                    }
                }
                return repaired;
            };

        std::vector<std::string> output;
        // Treat repeated managed sections as one logical body. Repairing each
        // occurrence separately would append defaults that override earlier
        // compatible assignments. Preserve assignment order (last value wins).
        std::vector<std::string> emittedManagedSections;
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

            if (std::find(emittedManagedSections.begin(), emittedManagedSections.end(),
                    section) != emittedManagedSections.end()) {
                index = sectionEnd;
                continue;
            }
            emittedManagedSections.push_back(section);
            std::vector<std::string> body;
            std::string logicalSection;
            for (const auto& current : source) {
                std::string nextSection;
                if (IsSectionHeader(Trim(current), nextSection)) {
                    logicalSection = std::move(nextSection);
                    continue;
                }
                if (logicalSection == section) body.push_back(current);
            }
            const auto repaired = repairSection(body, section, keys, keyCount);
            output.emplace_back(source[sectionStart]);
            output.insert(output.end(), repaired.begin(), repaired.end());

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
        }

        std::vector<std::string> compactOutput;
        compactOutput.reserve(output.size());
        for (auto& line : output) {
            if (line.empty() && !compactOutput.empty() && compactOutput.back().empty())
                continue;
            compactOutput.emplace_back(std::move(line));
        }
        output = std::move(compactOutput);

        std::vector<std::string> spacedOutput;
        spacedOutput.reserve(output.size() + std::size(sectionNames) * 2);
        for (const auto& outputLine : output) {
            std::string section;
            if (IsSectionHeader(Trim(outputLine), section) && !spacedOutput.empty()) {
                while (!spacedOutput.empty() && spacedOutput.back().empty())
                    spacedOutput.pop_back();
                if (!spacedOutput.empty()) {
                    spacedOutput.emplace_back("");
                    spacedOutput.emplace_back("");
                }
            }
            spacedOutput.emplace_back(outputLine);
        }
        output = std::move(spacedOutput);

        if (output == original) return false;
        if (requiresRecoveryBackup) {
            std::filesystem::path backupPath;
            if (!CreateConfigBackup(path, backupPath)) {
                if (log) log("Config recovery backup failed; existing config preserved.");
                return false;
            }
            if (log) log("Structurally invalid config backed up before recovery.");
        }
        const auto temporary = std::filesystem::path(path.wstring() + L".tmp");
        {
            platform::win32::NativeFile file;
            if (!file.CreateExclusive(temporary.c_str())) {
                if (log) log("Config template staging open failed; existing config preserved.");
                return false;
            }
            bool written = true;
            for (std::size_t outputIndex = 0; outputIndex < output.size(); ++outputIndex) {
                written = written && file.Write(output[outputIndex]);
                if (outputIndex + 1 < output.size() || format.trailingLineEnding)
                    written = written && file.Write(format.lineEnding);
            }
            const bool flushed = written && file.FlushToDisk();
            if (!flushed) {
                file.Close();
                DeleteFileW(temporary.c_str());
                if (log) log("Config template staging write failed; existing config preserved.");
                return false;
            }
            if (!file.Close()) {
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
