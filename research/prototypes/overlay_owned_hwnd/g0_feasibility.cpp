#define NOMINMAX
#include <windows.h>
#include <d3d11_1.h>
#include <dcomp.h>
#include <dwmapi.h>
#include <wrl/client.h>
#include <atomic>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <thread>
#include <vector>

using Microsoft::WRL::ComPtr;
namespace {
constexpr int kWidth = 360, kHeight = 220;
constexpr COLORREF kBackground = RGB(40, 100, 160);
std::atomic<int> hostClicks{}, overlayClicks{}, mouseActivations{};
std::atomic<bool> finished{};
std::atomic<int> result{2};
HWND host{};
DWORD hostThread{};

void Require(bool condition, const char* label) {
    if (!condition) throw std::runtime_error(label);
}
void Hr(HRESULT hr, const char* operation) {
    std::printf("API %s hr=0x%08lX\n", operation, static_cast<unsigned long>(hr));
    Require(SUCCEEDED(hr), operation);
}
LRESULT CALLBACK HostProc(HWND window, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_PAINT) {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        HBRUSH brush = CreateSolidBrush(kBackground);
        FillRect(dc, &paint.rcPaint, brush);
        DeleteObject(brush);
        EndPaint(window, &paint);
        return 0;
    }
    if (msg == WM_LBUTTONDOWN) ++hostClicks;
    return DefWindowProcW(window, msg, wp, lp);
}
LRESULT CALLBACK OverlayProc(HWND window, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_MOUSEACTIVATE) { ++mouseActivations; return MA_NOACTIVATE; }
    if (msg == WM_NCHITTEST) return HTCLIENT;
    if (msg == WM_LBUTTONDOWN) ++overlayClicks;
    return DefWindowProcW(window, msg, wp, lp);
}
void PumpFor(DWORD duration) {
    const ULONGLONG end = GetTickCount64() + duration;
    do {
        MSG msg{};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg); DispatchMessageW(&msg);
        }
        MsgWaitForMultipleObjectsEx(0, nullptr, 10, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    } while (GetTickCount64() < end);
}
bool FocusOnHost() {
    GUITHREADINFO info{sizeof(info)};
    return GetForegroundWindow() == host &&
        GetGUIThreadInfo(hostThread, &info) && info.hwndFocus == host;
}
void Click(POINT point, HWND overlay) {
    Require(FocusOnHost(), "foreground/focus lost before fixture input");
    const HWND under = WindowFromPoint(point);
    Require(under == host || under == overlay, "foreign window under fixture click");
    const int left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    const int top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    const int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    const int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    Require(width > 1 && height > 1, "virtual desktop geometry");
    INPUT move{};
    move.type = INPUT_MOUSE;
    move.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
    move.mi.dx = MulDiv(point.x - left, 65535, width - 1);
    move.mi.dy = MulDiv(point.y - top, 65535, height - 1);
    const UINT moved = SendInput(1, &move, sizeof(INPUT));
    PumpFor(100);
    POINT actual{};
    const BOOL read = GetCursorPos(&actual);
    std::printf("CURSOR SendInput=%u read=%d requested=%ld,%ld actual=%ld,%ld\n",
        moved, read, point.x, point.y, actual.x, actual.y);
    Require(moved == 1 && read && abs(actual.x - point.x) <= 1 &&
        abs(actual.y - point.y) <= 1, "native cursor delivery unavailable");
    Require(FocusOnHost(), "foreground/focus lost after cursor move");
    const HWND recipient = WindowFromPoint(actual);
    Require(recipient == host || recipient == overlay, "foreign window after cursor move");
    INPUT inputs[2]{};
    inputs[0].type = inputs[1].type = INPUT_MOUSE;
    inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    Require(SendInput(2, inputs, sizeof(INPUT)) == 2, "SendInput");
    PumpFor(250);
}
bool Near(COLORREF actual, COLORREF expected) {
    return actual != CLR_INVALID &&
        abs(static_cast<int>(GetRValue(actual)) - GetRValue(expected)) <= 12 &&
        abs(static_cast<int>(GetGValue(actual)) - GetGValue(expected)) <= 12 &&
        abs(static_cast<int>(GetBValue(actual)) - GetBValue(expected)) <= 12;
}
bool Sample(HDC screen, POINT origin, int x, COLORREF expected, const char* name) {
    const COLORREF pixel = GetPixel(screen, origin.x + x, origin.y + 40);
    const bool pass = Near(pixel, expected);
    std::printf("PIXEL %s actual=%u,%u,%u expected=%u,%u,%u %s\n", name,
        GetRValue(pixel), GetGValue(pixel), GetBValue(pixel), GetRValue(expected),
        GetGValue(expected), GetBValue(expected), pass ? "PASS" : "FAIL");
    return pass;
}
void Screenshot(HDC screen, POINT origin, const char* filename) {
    HDC memory = CreateCompatibleDC(screen);
    Require(memory != nullptr, "CreateCompatibleDC");
    BITMAPINFO info{};
    info.bmiHeader = {sizeof(BITMAPINFOHEADER), kWidth, -kHeight, 1, 32, BI_RGB};
    void* pixels{};
    HBITMAP bitmap = CreateDIBSection(screen, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
    if (!bitmap) { DeleteDC(memory); throw std::runtime_error("CreateDIBSection"); }
    HGDIOBJ previous = SelectObject(memory, bitmap);
    const BOOL copied = BitBlt(memory, 0, 0, kWidth, kHeight, screen,
        origin.x, origin.y, SRCCOPY | CAPTUREBLT);
    const DWORD bytes = kWidth * kHeight * 4;
    BITMAPFILEHEADER header{0x4d42, sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bytes,
        0, 0, sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)};
    std::ofstream output(filename, std::ios::binary);
    output.write(reinterpret_cast<const char*>(&header), sizeof(header));
    output.write(reinterpret_cast<const char*>(&info.bmiHeader), sizeof(info.bmiHeader));
    output.write(static_cast<const char*>(pixels), bytes);
    const bool saved = output.good();
    SelectObject(memory, previous); DeleteObject(bitmap); DeleteDC(memory);
    Require(copied && saved, "screen screenshot");
}

void OverlayExperiment(POINT origin) {
    HWND overlay{};
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    try {
        Hr(com, "CoInitializeEx");
        {
            overlay = CreateWindowExW(WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW |
                WS_EX_LAYERED | WS_EX_TRANSPARENT, L"OwnedHwndG0Overlay", L"G0 overlay",
                WS_POPUP, origin.x, origin.y, kWidth, kHeight, host, nullptr,
                GetModuleHandleW(nullptr), nullptr);
            Require(overlay != nullptr, "CreateWindowEx overlay");
            Require(SetLayeredWindowAttributes(overlay, 0, 255, LWA_ALPHA) != FALSE,
                "SetLayeredWindowAttributes alpha255");
            ComPtr<ID3D11Device> device;
            ComPtr<ID3D11DeviceContext> context;
            D3D_FEATURE_LEVEL level{};
            Hr(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION,
                &device, &level, &context), "D3D11CreateDevice hardware");
            ComPtr<IDXGIDevice> dxgi;
            Hr(device.As(&dxgi), "IDXGIDevice");
            ComPtr<IDCompositionDevice> composition;
            Hr(DCompositionCreateDevice(dxgi.Get(), IID_PPV_ARGS(&composition)),
                "DCompositionCreateDevice");
            ComPtr<IDCompositionTarget> target;
            // The only CreateTargetForHwnd call binds Overlay HWND, never surrogate host.
            Hr(composition->CreateTargetForHwnd(overlay, TRUE, &target), "CreateTarget OverlayHwnd");
            const void* const targetIdentity = target.Get();
            std::printf("TARGET created=1 host_calls=0 target=%p host_thread=%lu overlay_thread=%lu\n",
                targetIdentity, hostThread, GetCurrentThreadId());
            ComPtr<IDCompositionVisual> visual;
            ComPtr<IDCompositionSurface> surface;
            Hr(composition->CreateVisual(&visual), "CreateVisual");
            Hr(composition->CreateSurface(kWidth, kHeight, DXGI_FORMAT_B8G8R8A8_UNORM,
                DXGI_ALPHA_MODE_PREMULTIPLIED, &surface), "CreateSurface");
            ComPtr<ID3D11Texture2D> texture;
            POINT offset{};
            Hr(surface->BeginDraw(nullptr, IID_PPV_ARGS(&texture), &offset), "BeginDraw");
            std::vector<unsigned char> pixels(kWidth * kHeight * 4, 0);
            for (int y = 0; y < kHeight; ++y) for (int x = 120; x < kWidth; ++x) {
                const size_t i = (static_cast<size_t>(y) * kWidth + x) * 4;
                pixels[i + 2] = pixels[i + 3] = x < 240 ? 128 : 255;
            }
            D3D11_BOX box{static_cast<UINT>(offset.x), static_cast<UINT>(offset.y), 0,
                static_cast<UINT>(offset.x + kWidth), static_cast<UINT>(offset.y + kHeight), 1};
            context->UpdateSubresource(texture.Get(), 0, &box, pixels.data(), kWidth * 4, 0);
            Hr(surface->EndDraw(), "EndDraw");
            Hr(visual->SetContent(surface.Get()), "SetContent");
            Hr(target->SetRoot(visual.Get()), "SetRoot attached");
            Hr(composition->Commit(), "Commit attached");
            Require(SetWindowPos(overlay, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE |
                SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW) != FALSE, "show overlay");
            PumpFor(700);
            Hr(DwmFlush(), "DwmFlush sampling only");
            Require(FocusOnHost(), "host focus before samples");
            HDC screen = GetDC(nullptr);
            Require(screen != nullptr, "screen DC");
            bool alpha = Sample(screen, origin, 60, kBackground, "transparent");
            alpha = Sample(screen, origin, 180, RGB(148, 50, 80), "half_alpha") && alpha;
            alpha = Sample(screen, origin, 300, RGB(255, 0, 0), "opaque_marker") && alpha;
            Screenshot(screen, origin, "g0-passive.bmp");
            ReleaseDC(nullptr, screen);
            // Hit the opaque region so pass-through cannot be explained by zero-alpha pixels.
            POINT click{origin.x + 300, origin.y + 150};
            Click(click, overlay);
            const bool passive = hostClicks == 1 && overlayClicks == 0 && FocusOnHost();
            std::printf("INPUT passive host=%d overlay=%d focus=%d %s\n",
                hostClicks.load(), overlayClicks.load(), FocusOnHost(), passive ? "PASS" : "FAIL");
            SetLastError(0);
            const LONG_PTR prior = SetWindowLongPtrW(overlay, GWL_EXSTYLE,
                GetWindowLongPtrW(overlay, GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);
            Require(prior != 0 || GetLastError() == 0, "remove WS_EX_TRANSPARENT");
            Require(SetWindowPos(overlay, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE |
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED) != FALSE, "apply interactive style");
            PumpFor(200);
            Click(click, overlay);
            const bool interactive = overlayClicks == 1 && hostClicks == 1 && FocusOnHost();
            std::printf("INPUT first_interactive host=%d overlay=%d mouseactivate=%d focus=%d %s\n",
                hostClicks.load(), overlayClicks.load(), mouseActivations.load(), FocusOnHost(),
                interactive ? "PASS" : "FAIL");
            Require(target.Get() == targetIdentity, "persistent target identity");
            std::printf("RESULT alpha=%d passive=%d interactive=%d persistent_target=1 host_target_calls=0\n",
                alpha, passive, interactive);
            result = alpha && passive && interactive ? 0 : 1;
            Hr(target->SetRoot(nullptr), "cleanup SetRoot null");
            Hr(composition->Commit(), "cleanup Commit");
            ShowWindow(overlay, SW_HIDE);
        } // All COM resources released on their creating worker before HWND destruction.
    } catch (const std::exception& error) {
        std::printf("UNVERIFIED_OR_ERROR %s win32=%lu\n", error.what(), GetLastError());
        result = 2;
    }
    if (overlay) DestroyWindow(overlay);
    if (SUCCEEDED(com)) CoUninitialize();
    finished = true;
}
}
int main() {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    hostThread = GetCurrentThreadId();
    POINT savedCursor{}; GetCursorPos(&savedCursor);
    HWND savedForeground = GetForegroundWindow();
    WNDCLASSW hostClass{};
    hostClass.hInstance = GetModuleHandleW(nullptr);
    hostClass.lpfnWndProc = HostProc;
    hostClass.lpszClassName = L"OwnedHwndG0Host";
    WNDCLASSW overlayClass = hostClass;
    overlayClass.lpfnWndProc = OverlayProc;
    overlayClass.lpszClassName = L"OwnedHwndG0Overlay";
    if (!RegisterClassW(&hostClass) || !RegisterClassW(&overlayClass)) return 2;
    RECT rect{0, 0, kWidth, kHeight};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    host = CreateWindowExW(0, hostClass.lpszClassName, L"Isolated G0 feasibility (not the game)",
        WS_OVERLAPPEDWINDOW, 120, 120, rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, hostClass.hInstance, nullptr);
    if (!host) return 2;
    ShowWindow(host, SW_SHOW); UpdateWindow(host);
    SetForegroundWindow(host); SetFocus(host);
    PumpFor(250);
    POINT origin{}; ClientToScreen(host, &origin);
    std::thread worker(OverlayExperiment, origin);
    while (!finished) PumpFor(20);
    worker.join();
    const bool restore = GetForegroundWindow() == host;
    DestroyWindow(host);
    if (restore) {
        SetCursorPos(savedCursor.x, savedCursor.y);
        if (IsWindow(savedForeground)) SetForegroundWindow(savedForeground);
    }
    std::printf("EXIT %d\n", result.load());
    return result;
}
