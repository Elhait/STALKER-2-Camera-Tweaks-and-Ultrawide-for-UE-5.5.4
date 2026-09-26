#include "feature_config.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <utility>

namespace config
{
    std::string Trim(std::string value)
    {
        const auto notSpace = [](unsigned char character) { return !std::isspace(character); };
        value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
        value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
        return value;
    }

    bool ParseBool(std::string value, bool& result)
    {
        value = Trim(std::move(value));
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        if (value == "1" || value == "true" || value == "yes" || value == "on") {
            result = true;
            return true;
        }
        if (value == "0" || value == "false" || value == "no" || value == "off") {
            result = false;
            return true;
        }
        return false;
    }

    bool ParseLocaleCode(std::string value, std::string& result)
    {
        value = Trim(std::move(value));
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        const auto* descriptor = localization::FindLocaleDescriptor(value);
        if (!descriptor) return false;
        result.assign(descriptor->code);
        return true;
    }

    bool ParseOverlayFontSize(std::string value, int& result)
    {
        value = Trim(std::move(value));
        if (value.empty()) return false;
        char* end = nullptr;
        const long parsed = std::strtol(value.c_str(), &end, 10);
        if (end == value.c_str() || *end != '\0' ||
            parsed < OverlayFontSizeMin || parsed > OverlayFontSizeMax)
            return false;
        result = static_cast<int>(parsed);
        return true;
    }

    bool ParseHotkey(std::string value, int& result)
    {
        value = Trim(std::move(value));
        if (value.size() == 5 && (value[0] == 'V' || value[0] == 'v') &&
            (value[1] == 'K' || value[1] == 'k') && value[2] == '_') {
            char* end = nullptr;
            const long parsed = std::strtol(value.c_str() + 3, &end, 16);
            if (!end || *end != '\0' || parsed < 0 || parsed > 0xff ||
                !IsSupportedHotkey(static_cast<int>(parsed))) return false;
            result = static_cast<int>(parsed);
            return true;
        }
        if (value.size() == 1) {
            const char key = static_cast<char>(std::toupper(static_cast<unsigned char>(value.front())));
            if (key >= 'A' && key <= 'Z') { result = key; return true; }
            if (key >= '0' && key <= '9') { result = key; return true; }
            return false;
        }
        if (value.size() < 2 || (value.front() != 'F' && value.front() != 'f')) return false;
        int number = 0;
        for (std::size_t index = 1; index < value.size(); ++index) {
            if (value[index] < '0' || value[index] > '9') return false;
            number = number * 10 + (value[index] - '0');
            if (number > 24) return false;
        }
        if (number < 1 || number > 24) return false;
        result = VK_F1 + number - 1;
        return true;
    }

    bool IsSupportedHotkey(int key) noexcept
    {
        return key == VK_BACK || key == VK_TAB || key == VK_CLEAR ||
            key == VK_RETURN || key == VK_PAUSE || key == VK_CAPITAL ||
            key == VK_SPACE || (key >= VK_PRIOR && key <= VK_HELP &&
                key != 0x2a && key != 0x2b) ||
            (key >= '0' && key <= '9') || (key >= 'A' && key <= 'Z') ||
            (key >= VK_NUMPAD0 && key <= VK_DIVIDE) ||
            (key >= VK_F1 && key <= VK_F24) ||
            key == VK_NUMLOCK || key == VK_SCROLL ||
            (key >= VK_BROWSER_BACK && key <= VK_LAUNCH_APP2) ||
            key == VK_OEM_1 || key == VK_OEM_PLUS || key == VK_OEM_COMMA ||
            key == VK_OEM_MINUS || key == VK_OEM_PERIOD || key == VK_OEM_2 ||
            key == VK_OEM_3 || key == VK_OEM_4 || key == VK_OEM_5 ||
            key == VK_OEM_6 || key == VK_OEM_7 || key == VK_OEM_8 ||
            key == VK_OEM_102 || key == VK_OEM_AX;
    }

    int HotkeyBindingValue(const FeatureConfig& config,
        HotkeyBindingId id) noexcept
    {
        switch (id) {
        case HotkeyBindingId::GameplayMode: return config.gameplayCycleKey;
        case HotkeyBindingId::CinematicAspect: return config.cinematicCycleKey;
        case HotkeyBindingId::CinematicFov: return config.cinematicFovCycleKey;
        case HotkeyBindingId::DialogueZoom: return config.dialogueCycleKey;
        case HotkeyBindingId::OverlayToggle: return config.overlayToggleKey;
        }
        return 0;
    }

    void SetHotkeyBindingValue(FeatureConfig& config, HotkeyBindingId id,
        int value) noexcept
    {
        switch (id) {
        case HotkeyBindingId::GameplayMode: config.gameplayCycleKey = value; break;
        case HotkeyBindingId::CinematicAspect: config.cinematicCycleKey = value; break;
        case HotkeyBindingId::CinematicFov: config.cinematicFovCycleKey = value; break;
        case HotkeyBindingId::DialogueZoom: config.dialogueCycleKey = value; break;
        case HotkeyBindingId::OverlayToggle: config.overlayToggleKey = value; break;
        }
    }

    const HotkeyBindingDescriptor* FindHotkeyBinding(HotkeyBindingId id) noexcept
    {
        for (const auto& descriptor : HotkeyBindingRegistry)
            if (descriptor.id == id) return &descriptor;
        return nullptr;
    }

    const HotkeyBindingDescriptor* FindHotkeyBinding(const char* section,
        const char* configKey) noexcept
    {
        if (!section || !configKey) return nullptr;
        for (const auto& descriptor : HotkeyBindingRegistry)
            if (std::strcmp(descriptor.section, section) == 0 &&
                std::strcmp(descriptor.configKey, configKey) == 0)
                return &descriptor;
        return nullptr;
    }

    bool HasHotkeyConflict(const FeatureConfig& config, int key,
        HotkeyBindingId binding) noexcept
    {
        for (const auto& descriptor : HotkeyBindingRegistry)
            if (descriptor.id != binding &&
                HotkeyBindingValue(config, descriptor.id) == key)
                return true;
        return false;
    }

    const char* HotkeyName(int key)
    {
        static thread_local char display[128];
        if (!IsSupportedHotkey(key)) return "Unknown";
        switch (key) {
        case VK_BACK: return "Backspace";
        case VK_TAB: return "Tab";
        case VK_CLEAR: return "Clear";
        case VK_RETURN: return "Enter";
        case VK_PAUSE: return "Pause";
        case VK_CAPITAL: return "Caps Lock";
        case VK_SPACE: return "Space";
        case VK_SELECT: return "Select";
        case VK_SNAPSHOT: return "Print Screen";
        case VK_INSERT: return "Insert";
        case VK_DELETE: return "Delete";
        case VK_HOME: return "Home";
        case VK_END: return "End";
        case VK_PRIOR: return "Page Up";
        case VK_NEXT: return "Page Down";
        case VK_LEFT: return "Left";
        case VK_RIGHT: return "Right";
        case VK_UP: return "Up";
        case VK_DOWN: return "Down";
        case VK_NUMPAD0: case VK_NUMPAD1: case VK_NUMPAD2: case VK_NUMPAD3:
        case VK_NUMPAD4: case VK_NUMPAD5: case VK_NUMPAD6: case VK_NUMPAD7:
        case VK_NUMPAD8: case VK_NUMPAD9:
            std::snprintf(display, sizeof(display), "Num %d", key - VK_NUMPAD0);
            return display;
        case VK_MULTIPLY: return "Num *";
        case VK_ADD: return "Num +";
        case VK_SEPARATOR: return "Num Separator";
        case VK_SUBTRACT: return "Num -";
        case VK_DECIMAL: return "Num .";
        case VK_DIVIDE: return "Num /";
        case VK_NUMLOCK: return "Num Lock";
        case VK_SCROLL: return "Scroll Lock";
        case VK_BROWSER_BACK: return "Browser Back";
        case VK_BROWSER_FORWARD: return "Browser Forward";
        case VK_BROWSER_REFRESH: return "Browser Refresh";
        case VK_BROWSER_STOP: return "Browser Stop";
        case VK_BROWSER_SEARCH: return "Browser Search";
        case VK_BROWSER_FAVORITES: return "Browser Favorites";
        case VK_BROWSER_HOME: return "Browser Home";
        case VK_VOLUME_MUTE: return "Volume Mute";
        case VK_VOLUME_DOWN: return "Volume Down";
        case VK_VOLUME_UP: return "Volume Up";
        case VK_MEDIA_NEXT_TRACK: return "Media Next";
        case VK_MEDIA_PREV_TRACK: return "Media Previous";
        case VK_MEDIA_STOP: return "Media Stop";
        case VK_MEDIA_PLAY_PAUSE: return "Media Play/Pause";
        case VK_LAUNCH_MAIL: return "Launch Mail";
        case VK_LAUNCH_MEDIA_SELECT: return "Launch Media";
        case VK_LAUNCH_APP1: return "Launch App 1";
        case VK_LAUNCH_APP2: return "Launch App 2";
        case VK_OEM_1: return ";";
        case VK_OEM_PLUS: return "=";
        case VK_OEM_COMMA: return ",";
        case VK_OEM_MINUS: return "-";
        case VK_OEM_PERIOD: return ".";
        case VK_OEM_2: return "/";
        case VK_OEM_3: return "`";
        case VK_OEM_4: return "[";
        case VK_OEM_5: return "\\";
        case VK_OEM_6: return "]";
        case VK_OEM_7: return "'";
        case VK_OEM_8: return "OEM 8";
        case VK_OEM_102: return "\\";
        case VK_OEM_AX: return "OEM AX";
        }
        if ((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9')) {
            display[0] = static_cast<char>(key);
            display[1] = '\0';
            return display;
        }
        if (key >= VK_F1 && key <= VK_F24) {
            std::snprintf(display, sizeof(display), "F%d", key - VK_F1 + 1);
            return display;
        }
        std::snprintf(display, sizeof(display), "VK_%02X", key);
        return display;
    }

    const char* HotkeyConfigValue(int key)
    {
        static thread_local char value[8];
        if (!IsSupportedHotkey(key)) return "";
        std::snprintf(value, sizeof(value), "VK_%02X", key);
        return value;
    }

    bool ParseGameplayMode(std::string value, GameplayMode& result)
    {
        value = Trim(std::move(value));
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        if (value == "aspectrecalculation" || value == "aspect-recalculation")
            result = GameplayMode::AspectRecalculation;
        else if (value == "horplus" || value == "hor+")
            result = GameplayMode::HorPlus;
        else return false;
        return true;
    }

    const char* GameplayModeName(GameplayMode mode)
    {
        switch (mode) {
        case GameplayMode::AspectRecalculation: return "AspectRecalculation";
        case GameplayMode::HorPlus: return "HorPlus";
        }
        return "AspectRecalculation";
    }

    bool ParseCinematicAspectPolicy(std::string value, CinematicAspectPolicy& result)
    {
        value = Trim(std::move(value));
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        if (value == "auto" || value == "default" || value == "fix") result = CinematicAspectPolicy::Auto;
        else if (value == "native") result = CinematicAspectPolicy::Native;
        else if (value == "16:9" || value == "16x9") result = CinematicAspectPolicy::Forced16x9;
        else if (value == "21:9" || value == "21x9") result = CinematicAspectPolicy::Forced21x9;
        else if (value == "32:9" || value == "32x9") result = CinematicAspectPolicy::Forced32x9;
        else return false;
        return true;
    }

    const char* CinematicAspectPolicyName(CinematicAspectPolicy policy)
    {
        switch (policy) {
        case CinematicAspectPolicy::Auto: return "Auto";
        case CinematicAspectPolicy::Native: return "Native";
        case CinematicAspectPolicy::Forced16x9: return "16:9";
        case CinematicAspectPolicy::Forced21x9: return "21:9";
        case CinematicAspectPolicy::Forced32x9: return "32:9";
        }
        return "Auto";
    }

    bool ParseCinematicFovMode(std::string value, CinematicFovMode& result)
    {
        value = Trim(std::move(value));
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        if (value == "nativehorplus" || value == "horplus" || value == "hor+")
            result = CinematicFovMode::NativeHorPlus;
        else if (value == "matchgameplay" || value == "match-gameplay")
            result = CinematicFovMode::GameplayHorPlus;
        else if (value == "gameplayhorplus" || value == "gameplay-horplus")
            result = CinematicFovMode::GameplayHorPlus;
        else return false;
        return true;
    }

    const char* CinematicFovModeName(CinematicFovMode mode)
    {
        switch (mode) {
        case CinematicFovMode::NativeHorPlus: return "NativeHorPlus";
        case CinematicFovMode::GameplayHorPlus: return "GameplayHorPlus";
        }
        return "NativeHorPlus";
    }

    bool ParseDialogueZoomPolicy(std::string value, DialogueZoomPolicy& result)
    {
        value = Trim(std::move(value));
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        if (value == "native") result = DialogueZoomPolicy::Native;
        else if (value == "adaptive") result = DialogueZoomPolicy::Adaptive;
        else if (value == "reduced") result = DialogueZoomPolicy::Reduced;
        else if (value == "disabled") result = DialogueZoomPolicy::Disabled;
        else return false;
        return true;
    }

    const char* DialogueZoomPolicyName(DialogueZoomPolicy policy)
    {
        switch (policy) {
        case DialogueZoomPolicy::Native: return "Native";
        case DialogueZoomPolicy::Adaptive: return "Adaptive";
        case DialogueZoomPolicy::Reduced: return "Reduced";
        case DialogueZoomPolicy::Disabled: return "Disabled";
        }
        return "Native";
    }

    DialogueZoomPolicy NextDialogueZoomPolicy(DialogueZoomPolicy policy)
    {
        switch (policy) {
        case DialogueZoomPolicy::Native: return DialogueZoomPolicy::Adaptive;
        case DialogueZoomPolicy::Adaptive: return DialogueZoomPolicy::Reduced;
        case DialogueZoomPolicy::Reduced: return DialogueZoomPolicy::Disabled;
        case DialogueZoomPolicy::Disabled: return DialogueZoomPolicy::Native;
        }
        return DialogueZoomPolicy::Native;
    }

    CinematicAspectPolicy NextCinematicAspectPolicy(CinematicAspectPolicy policy)
    {
        switch (policy) {
        case CinematicAspectPolicy::Auto: return CinematicAspectPolicy::Native;
        case CinematicAspectPolicy::Native: return CinematicAspectPolicy::Forced16x9;
        case CinematicAspectPolicy::Forced16x9: return CinematicAspectPolicy::Forced21x9;
        case CinematicAspectPolicy::Forced21x9: return CinematicAspectPolicy::Forced32x9;
        case CinematicAspectPolicy::Forced32x9: return CinematicAspectPolicy::Auto;
        }
        return CinematicAspectPolicy::Auto;
    }

    CinematicFovMode NextCinematicFovMode(CinematicFovMode mode)
    {
        switch (mode) {
        case CinematicFovMode::NativeHorPlus: return CinematicFovMode::GameplayHorPlus;
        case CinematicFovMode::GameplayHorPlus: return CinematicFovMode::NativeHorPlus;
        }
        return CinematicFovMode::NativeHorPlus;
    }

    GameplayMode NextGameplayMode(GameplayMode mode)
    {
        switch (mode) {
        case GameplayMode::AspectRecalculation: return GameplayMode::HorPlus;
        case GameplayMode::HorPlus: return GameplayMode::AspectRecalculation;
        }
        return GameplayMode::AspectRecalculation;
    }
}
