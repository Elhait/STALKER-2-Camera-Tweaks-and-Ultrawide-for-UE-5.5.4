#pragma once

#include <windows.h>

namespace platform::win32
{
    inline constexpr wchar_t OverlayDisplayWindowClassName[] =
        L"STALKER2CameraTweaks.OverlayDisplayWindow";

    HWND FindCurrentProcessWindow();
    HWND SelectCurrentProcessViewportWindow(HWND foregroundWindow,
        HWND fallbackWindow) noexcept;
    bool IsOverlayDisplayWindow(HWND window) noexcept;
    bool IsCurrentProcessViewportWindow(HWND window) noexcept;
}
