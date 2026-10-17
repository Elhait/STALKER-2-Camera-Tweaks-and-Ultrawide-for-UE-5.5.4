#include "../../src/platform/win32/viewport.hpp"
#include "../../src/platform/win32/window.hpp"

#include <iostream>
#include <cmath>

namespace
{
    constexpr wchar_t HostClassName[] = L"CameraTweaks.ViewportHostFixture";

    LRESULT CALLBACK FixtureWindowProc(HWND window, UINT message,
        WPARAM wParam, LPARAM lParam)
    {
        return DefWindowProcW(window, message, wParam, lParam);
    }
}

int main()
{
    bool pass = true;
    pass &= platform::win32::IsUsableClientViewport(1920, 1080);
    pass &= platform::win32::IsUsableClientViewport(4000, 1000);
    pass &= !platform::win32::IsUsableClientViewport(1, 1080);
    pass &= !platform::win32::IsUsableClientViewport(1920, 1);
    pass &= !platform::win32::IsUsableClientViewport(0, 0);
    std::cout << "viewport_plausibility=" << (pass ? "PASS" : "FAIL") << "\n";

    const HINSTANCE instance = GetModuleHandleW(nullptr);
    WNDCLASSEXW hostClass{};
    hostClass.cbSize = sizeof(hostClass);
    hostClass.lpfnWndProc = FixtureWindowProc;
    hostClass.hInstance = instance;
    hostClass.lpszClassName = HostClassName;
    const bool hostClassRegistered = RegisterClassExW(&hostClass) != 0;

    WNDCLASSEXW displayClass{};
    displayClass.cbSize = sizeof(displayClass);
    displayClass.lpfnWndProc = FixtureWindowProc;
    displayClass.hInstance = instance;
    displayClass.lpszClassName = platform::win32::OverlayDisplayWindowClassName;
    const bool displayClassRegistered = RegisterClassExW(&displayClass) != 0;

    HWND host = nullptr;
    HWND display = nullptr;
    if (hostClassRegistered && displayClassRegistered) {
        host = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            HostClassName, L"", WS_POPUP, 0, 0, 2560, 1080,
            nullptr, nullptr, instance, nullptr);
        display = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            platform::win32::OverlayDisplayWindowClassName, L"", WS_POPUP,
            0, 0, 5000, 1800, nullptr, nullptr, instance, nullptr);
    }

    if (host && display) {
        ShowWindow(host, SW_SHOWNOACTIVATE);
        ShowWindow(display, SW_SHOWNOACTIVATE);
        RECT client{};
        GetClientRect(host, &client);
        const float expectedAspect = static_cast<float>(client.right - client.left) /
            static_cast<float>(client.bottom - client.top);
        const float actualAspect = platform::win32::ReadCurrentProcessClientViewportAspect();
        const bool selectedHost =
            platform::win32::IsCurrentProcessViewportWindow(host) &&
            !platform::win32::IsCurrentProcessViewportWindow(display) &&
            platform::win32::IsOverlayDisplayWindow(display) &&
            platform::win32::FindCurrentProcessWindow() == host &&
            platform::win32::SelectCurrentProcessViewportWindow(display, host) == host &&
            platform::win32::SelectCurrentProcessViewportWindow(
                GetDesktopWindow(), host) == host &&
            std::isfinite(actualAspect) && std::abs(actualAspect - expectedAspect) < 0.001f;
        pass &= selectedHost;
        std::cout << "viewport_overlay_excluded_and_game_fallback="
            << (selectedHost ? "PASS" : "FAIL") << "\n";
    } else {
        pass = false;
        std::cout << "viewport_overlay_excluded_and_game_fallback=FAIL"
            << " window_setup_error=" << GetLastError() << "\n";
    }

    if (display) DestroyWindow(display);
    if (host) DestroyWindow(host);
    if (displayClassRegistered)
        UnregisterClassW(platform::win32::OverlayDisplayWindowClassName, instance);
    if (hostClassRegistered) UnregisterClassW(HostClassName, instance);
    return pass ? 0 : 1;
}
