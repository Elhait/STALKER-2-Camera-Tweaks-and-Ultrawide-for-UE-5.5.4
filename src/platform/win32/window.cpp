#include "window.hpp"

#include <cstdint>
#include <iterator>

namespace
{
    struct WindowCandidate
    {
        HWND window{};
        std::uint64_t clientArea{};
    };

    BOOL CALLBACK FindCurrentProcessWindowCallback(HWND window, LPARAM parameter)
    {
        if (!platform::win32::IsCurrentProcessViewportWindow(window)) return TRUE;
        RECT client{};
        if (!GetClientRect(window, &client)) return TRUE;
        const auto width = static_cast<std::uint64_t>(client.right - client.left);
        const auto height = static_cast<std::uint64_t>(client.bottom - client.top);
        const auto area = width * height;
        auto& best = *reinterpret_cast<WindowCandidate*>(parameter);
        if (area > best.clientArea) best = {window, area};
        return TRUE;
    }
}

namespace platform::win32
{
    bool IsOverlayDisplayWindow(HWND window) noexcept
    {
        if (!window) return false;
        wchar_t className[96]{};
        const int length = GetClassNameW(window, className,
            static_cast<int>(std::size(className)));
        return length > 0 && lstrcmpW(className, OverlayDisplayWindowClassName) == 0;
    }

    bool IsCurrentProcessViewportWindow(HWND window) noexcept
    {
        if (!window || IsOverlayDisplayWindow(window) || !IsWindowVisible(window) ||
            GetWindow(window, GW_OWNER) ||
            (GetWindowLongPtrW(window, GWL_STYLE) & WS_CHILD)) return false;
        DWORD processId = 0;
        GetWindowThreadProcessId(window, &processId);
        if (processId != GetCurrentProcessId()) return false;
        RECT client{};
        if (!GetClientRect(window, &client)) return false;
        const auto width = client.right - client.left;
        const auto height = client.bottom - client.top;
        return width >= 320 && height >= 200;
    }

    HWND FindCurrentProcessWindow()
    {
        WindowCandidate best{};
        EnumWindows(FindCurrentProcessWindowCallback, reinterpret_cast<LPARAM>(&best));
        return best.window;
    }

    HWND SelectCurrentProcessViewportWindow(HWND foregroundWindow,
        HWND fallbackWindow) noexcept
    {
        if (IsCurrentProcessViewportWindow(foregroundWindow))
            return foregroundWindow;
        return IsCurrentProcessViewportWindow(fallbackWindow)
            ? fallbackWindow : nullptr;
    }
}
