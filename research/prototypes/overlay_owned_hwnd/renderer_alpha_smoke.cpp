// Actual production window creation and Renderer; no game hooks are installed.
#include "src/overlay/presenter/renderer_runtime.hpp"
#include <dwmapi.h>
#include <cstdio>
#include <stdexcept>
#include <string_view>

namespace {
bool legacyAlpha{}, noRedirection{};
HWND CreateAlphaFixtureWindow(DWORD styles, LPCWSTR cls, LPCWSTR title,
    DWORD windowStyle, int x, int y, int width, int height, HWND parent,
    HMENU menu, HINSTANCE instance, LPVOID parameter)
{
    if (noRedirection) styles |= WS_EX_NOREDIRECTIONBITMAP;
    HWND window = ::CreateWindowExW(styles, cls, title, windowStyle,
        x, y, width, height, parent, menu, instance, parameter);
    if (window && legacyAlpha &&
        !SetLayeredWindowAttributes(window, 0, 255, LWA_ALPHA)) {
        DestroyWindow(window);
        return nullptr;
    }
    return window;
}
}
#define CreateWindowExW CreateAlphaFixtureWindow
#include "src/overlay/composition/composition_runtime.cpp"
#undef CreateWindowExW

namespace {
constexpr int Width = 1200, Height = 800;
constexpr COLORREF HostColor = RGB(40, 100, 160);
unsigned displayStateLogCount{};
void Require(bool result, const char* text)
{
    if (!result) throw std::runtime_error(text);
}
LRESULT CALLBACK HostProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_PAINT) {
        PAINTSTRUCT paint{};
        const HDC dc = BeginPaint(window, &paint);
        const HBRUSH brush = CreateSolidBrush(HostColor);
        FillRect(dc, &paint.rcPaint, brush);
        DeleteObject(brush);
        EndPaint(window, &paint);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}
bool Snapshot(plugin::RuntimeSettingsSnapshot& out, void*)
{
    out = {};
    out.overlayLocaleCode = "en";
    out.overlayLocaleAuto = false;
    return true;
}
bool Semantic(plugin::OverlaySemanticSnapshot& out, void*) { out = {}; return true; }
void FixtureLog(const std::string& value)
{
    if (value.starts_with("OVERLAY_DISPLAY_STATE ")) ++displayStateLogCount;
    std::printf("%s\n", value.c_str());
}
void Pump(overlay::Renderer& renderer, DWORD duration)
{
    const auto until = GetTickCount64() + duration;
    do {
        MSG message{};
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        if (renderer.visible()) renderer.RequestRedraw();
        if (renderer.needsRenderWork()) renderer.Render();
        Require(!renderer.disabled(), "Renderer disabled");
        Sleep(16);
    } while (GetTickCount64() < until);
}
bool Near(COLORREF value, COLORREF expected)
{
    return value != CLR_INVALID &&
        abs(static_cast<int>(GetRValue(value)) - GetRValue(expected)) < 12 &&
        abs(static_cast<int>(GetGValue(value)) - GetGValue(expected)) < 12 &&
        abs(static_cast<int>(GetBValue(value)) - GetBValue(expected)) < 12;
}
bool SampleFrame(HWND host, const char* phase)
{
    Require(IsWindowVisible(g_overlayDisplayWindow.load()), "display HWND hidden");
    POINT origin{};
    Require(ClientToScreen(host, &origin) != FALSE, "client origin");
    Require(SUCCEEDED(DwmFlush()), "DwmFlush for local pixel sampling");
    const HDC screen = GetDC(nullptr);
    Require(screen != nullptr, "screen DC");
    const HDC capture = CreateCompatibleDC(screen);
    const HBITMAP bitmap = CreateCompatibleBitmap(screen, Width, Height);
    Require(capture && bitmap, "desktop capture allocation");
    const HGDIOBJ previous = SelectObject(capture, bitmap);
    Require(BitBlt(capture,0,0,Width,Height,screen,origin.x,origin.y,
        SRCCOPY | CAPTUREBLT) != FALSE, "desktop frame capture");
    bool transparent = true;
    const POINT corners[]{{10,10},{Width-10,10},{10,Height-10},{Width-10,Height-10}};
    for (const auto point : corners) {
        const COLORREF pixel = GetPixel(capture, point.x, point.y);
        const bool match = Near(pixel, HostColor);
        std::printf("PIXEL phase=%s x=%ld y=%ld rgb=%u,%u,%u background=%d\n",
            phase, point.x, point.y, GetRValue(pixel), GetGValue(pixel), GetBValue(pixel), match);
        transparent &= match;
    }
    unsigned uiPixels = 0;
    for (int y = 20; y < Height - 20; y += 12)
        for (int x = 20; x < Width - 20; x += 12) {
            const COLORREF pixel = GetPixel(capture, x, y);
            if (pixel != CLR_INVALID && !Near(pixel, HostColor)) ++uiPixels;
        }
    SelectObject(capture, previous);
    DeleteObject(bitmap);
    DeleteDC(capture);
    ReleaseDC(nullptr, screen);
    std::printf("FRAME phase=%s transparent=%d ui_samples=%u result=%s\n",
        phase, transparent, uiPixels, transparent && uiPixels > 20 ? "PASS" : "FAIL");
    return transparent && uiPixels > 20;
}
}

int main(int argc, char** argv)
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    const std::string_view variant = argc > 1 ? argv[1] : "current";
    const bool initialShow = argc > 2 && std::string_view{argv[2]} == "initial-show";
    legacyAlpha = variant == "legacy-alpha" || variant == "legacy-no-redirection";
    noRedirection = variant == "legacy-no-redirection";
    if (variant != "current" && !legacyAlpha) return 2;
    HWND host{};
    HWND savedForeground = GetForegroundWindow();
    POINT savedCursor{};
    GetCursorPos(&savedCursor);
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    int result = 2;
    overlay::Renderer renderer;
    try {
        Require(SUCCEEDED(com), "COM initialization");
        SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        g_module = GetModuleHandleW(nullptr);
        WNDCLASSW cls{};
        cls.lpfnWndProc = HostProc;
        cls.hInstance = g_module;
        cls.lpszClassName = L"CameraTweaksActualRendererAlphaHost";
        Require(RegisterClassW(&cls) != 0, "host class");
        RECT bounds{0,0,Width,Height};
        AdjustWindowRect(&bounds, WS_OVERLAPPEDWINDOW, FALSE);
        host = CreateWindowExW(0, cls.lpszClassName, L"Actual Renderer alpha fixture",
            WS_OVERLAPPEDWINDOW, 100,100,bounds.right-bounds.left,bounds.bottom-bounds.top,
            nullptr,nullptr,g_module,nullptr);
        Require(host != nullptr, "host HWND");
        ShowWindow(host, SW_SHOWNOACTIVATE);
        UpdateWindow(host);
        Require(SetWindowPos(host, HWND_TOPMOST, 0,0,0,0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE) != FALSE, "surrogate z-order");
        if (initialShow) {
            POINT point{60,60};
            Require(ClientToScreen(host, &point) != FALSE, "surrogate click origin");
            Require(WindowFromPoint(point) == host, "foreign window at fixture click");
            INPUT inputs[3]{};
            inputs[0].type = inputs[1].type = inputs[2].type = INPUT_MOUSE;
            inputs[0].mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
            inputs[0].mi.dx = MulDiv(point.x - GetSystemMetrics(SM_XVIRTUALSCREEN), 65535,
                GetSystemMetrics(SM_CXVIRTUALSCREEN) - 1);
            inputs[0].mi.dy = MulDiv(point.y - GetSystemMetrics(SM_YVIRTUALSCREEN), 65535,
                GetSystemMetrics(SM_CYVIRTUALSCREEN) - 1);
            inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            inputs[2].mi.dwFlags = MOUSEEVENTF_LEFTUP;
            Require(SendInput(3, inputs, sizeof(INPUT)) == 3, "native fixture click unavailable");
            MSG message{};
            const auto until = GetTickCount64() + 250;
            do {
                while (PeekMessageW(&message,nullptr,0,0,PM_REMOVE)) {
                    TranslateMessage(&message); DispatchMessageW(&message);
                }
                Sleep(5);
            } while (GetTickCount64() < until);
            Require(GetForegroundWindow() == host, "surrogate did not gain foreground");
        }
        Require(CreateOverlayDisplayWindow(), "production display HWND creation");
        Require(plugin::GetRuntimeSettingsApi().Publish({nullptr,nullptr,Snapshot,
            nullptr,Semantic,nullptr,nullptr,nullptr}), "fixture settings publication");
        renderer.SetLogger(FixtureLog);
        diagnostics::SetEnabled(true);
        Require(renderer.Initialize(host, g_overlayDisplayWindow.load(), Width, Height),
            "production Renderer initialization");
        if (initialShow) {
            // No later geometry/activation callback may rescue a hidden startup HWND.
            Require(IsWindowVisible(g_overlayDisplayWindow.load()) != FALSE,
                "initial attached root did not show the display HWND");
            std::printf("INITIAL_SHOW same_root_without_later_host_callback=PASS\n");
        } else {
            // Pixel-only mode supplies activation; it does not establish focus semantics.
            Require(renderer.OnHostActivation(host, true), "surrogate active-state observation");
        }
        Pump(renderer, 600);
        bool pass = SampleFrame(host, "startup-toast");
        overlay::GetInputState().Toggle();
        renderer.SetVisible(true);
        Require(renderer.Render(), "menu Render");
        const auto stableLogCount = displayStateLogCount;
        Pump(renderer, 250);
        Require(displayStateLogCount == stableLogCount && stableLogCount != 0,
            "display state diagnostics emitted on unchanged frames");
        pass = SampleFrame(host, "menu-open") && pass;
        std::printf("VARIANT %.*s result=%s\n", static_cast<int>(variant.size()),
            variant.data(), pass ? "PASS" : "FAIL");
        result = pass ? 0 : 1;
    } catch (const std::exception& error) {
        std::printf("UNVERIFIED %s win32=%lu\n", error.what(), GetLastError());
    }
    const bool released = renderer.Shutdown();
    if (!released || !DestroyOverlayDisplayWindow()) result = 2;
    if (host) DestroyWindow(host);
    if (initialShow) SetCursorPos(savedCursor.x, savedCursor.y);
    if (IsWindow(savedForeground)) SetForegroundWindow(savedForeground);
    if (SUCCEEDED(com)) CoUninitialize();
    return result;
}
