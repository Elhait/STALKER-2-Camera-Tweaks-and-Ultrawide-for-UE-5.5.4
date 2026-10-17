#define NOMINMAX
#include <windows.h>
#include <d3d11_1.h>
#include <dcomp.h>
#include <dwmapi.h>
#include <dxgi.h>
#include <wrl/client.h>

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <thread>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace {
constexpr wchar_t kHostClass[] = L"CameraTweaksG1BHost";
constexpr wchar_t kOverlayClass[] = L"CameraTweaksG1BOverlay";
constexpr int kHostWidth = 480;
constexpr int kHostHeight = 300;

struct SharedState {
    HANDLE ready{};
    HANDLE shutdown{};
    HANDLE ownerStopped{};
    std::atomic<HWND> overlay{};
    std::atomic<DWORD> ownerThread{};
    std::atomic<bool> ownerCreated{};
    std::atomic<bool> ownerDestroyed{};
    std::atomic<bool> ownerDestroyOnCorrectThread{};
    std::atomic<bool> overlayNcDestroy{};
    std::atomic<bool> ownerCompositionDetached{};
    std::atomic<bool> ownerResourceRefsReleased{};
    std::atomic<LONG> error{};
};

SharedState g{};
HWND g_host{};
DWORD g_hostThread{};
HWND g_savedForeground{};
POINT g_savedCursor{};
constexpr COLORREF kHostColor = RGB(40, 100, 160);
std::atomic<int> g_hostClicks{};
std::atomic<int> g_overlayClicks{};

struct CompositionResources {
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGIDevice> dxgiDevice;
    ComPtr<IDCompositionDesktopDevice> compositionDevice;
    ComPtr<IDCompositionTarget> target;
    ComPtr<IDCompositionVisual2> visual;
    ComPtr<IDCompositionSurface> surface;

    void ReleaseAll() noexcept {
        target.Reset();
        visual.Reset();
        surface.Reset();
        compositionDevice.Reset();
        dxgiDevice.Reset();
        context.Reset();
        device.Reset();
    }

    bool Empty() const noexcept {
        return !target && !visual && !surface && !compositionDevice &&
            !dxgiDevice && !context && !device;
    }
};

void Require(bool condition, const char* what);

void Fail(const char* what) {
    throw std::runtime_error(what);
}

LRESULT CALLBACK HostProc(HWND window, UINT message, WPARAM wParam,
    LPARAM lParam) {
    if (message == WM_PAINT) {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        HBRUSH brush = CreateSolidBrush(kHostColor);
        FillRect(dc, &paint.rcPaint, brush);
        DeleteObject(brush);
        EndPaint(window, &paint);
        return 0;
    }
    if (message == WM_LBUTTONDOWN) ++g_hostClicks;
    return DefWindowProcW(window, message, wParam, lParam);
}

LRESULT CALLBACK OverlayProc(HWND window, UINT message, WPARAM wParam,
    LPARAM lParam) {
    if (message == WM_MOUSEACTIVATE) return MA_NOACTIVATE;
    if (message == WM_LBUTTONDOWN) ++g_overlayClicks;
    if (message == WM_NCDESTROY) {
        g.overlayNcDestroy.store(true, std::memory_order_release);
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

bool InitializeComposition(HWND overlay, CompositionResources& resources) {
    D3D_FEATURE_LEVEL selected{};
    constexpr D3D_FEATURE_LEVEL levels[]{D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0};
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT, levels,
        static_cast<UINT>(std::size(levels)), D3D11_SDK_VERSION,
        &resources.device, &selected, &resources.context);
    if (hr == E_INVALIDARG) {
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT, &levels[1], 1,
            D3D11_SDK_VERSION, &resources.device, &selected,
            &resources.context);
    }
    if (FAILED(hr) || FAILED(resources.device.As(&resources.dxgiDevice)) ||
        FAILED(DCompositionCreateDevice2(resources.dxgiDevice.Get(),
            __uuidof(IDCompositionDesktopDevice),
            reinterpret_cast<void**>(resources.compositionDevice.GetAddressOf()))) ||
        FAILED(resources.compositionDevice->CreateTargetForHwnd(overlay, TRUE,
            &resources.target)) ||
        FAILED(resources.compositionDevice->CreateVisual(&resources.visual)) ||
        FAILED(resources.compositionDevice->CreateSurface(kHostWidth, kHostHeight,
            DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ALPHA_MODE_PREMULTIPLIED,
            &resources.surface)))
        return false;

    ComPtr<ID3D11Texture2D> texture;
    POINT offset{};
    if (FAILED(resources.surface->BeginDraw(nullptr, IID_PPV_ARGS(&texture),
            &offset)))
        return false;
    std::vector<std::uint8_t> pixels(
        static_cast<std::size_t>(kHostWidth) * kHostHeight * 4, 0);
    for (int y = 0; y < kHostHeight; ++y) {
        for (int x = 0; x < kHostWidth; ++x) {
            const std::size_t index =
                (static_cast<std::size_t>(y) * kHostWidth + x) * 4;
            if (x >= 160) {
                const std::uint8_t alpha = x < 320 ? 128 : 255;
                pixels[index + 2] = alpha;
                pixels[index + 3] = alpha;
            }
        }
    }
    const D3D11_BOX box{static_cast<UINT>(offset.x),
        static_cast<UINT>(offset.y), 0,
        static_cast<UINT>(offset.x + kHostWidth),
        static_cast<UINT>(offset.y + kHostHeight), 1};
    resources.context->UpdateSubresource(texture.Get(), 0, &box,
        pixels.data(), kHostWidth * 4, 0);
    texture.Reset();
    hr = resources.surface->EndDraw();
    if (FAILED(hr) || FAILED(resources.visual->SetContent(resources.surface.Get())) ||
        FAILED(resources.target->SetRoot(resources.visual.Get())) ||
        FAILED(resources.compositionDevice->Commit()))
        return false;

    const DWORD targetThread = GetWindowThreadProcessId(overlay, nullptr);
    std::printf("TARGET hwnd=%p host=%p target_is_overlay=%d owner_thread=%lu current_thread=%lu feature_level=0x%X\n",
        overlay, g_host, overlay != g_host, targetThread, GetCurrentThreadId(),
        static_cast<unsigned>(selected));
    return targetThread == GetCurrentThreadId() && overlay != g_host;
}

bool Near(COLORREF actual, COLORREF expected) {
    return actual != CLR_INVALID &&
        std::abs(static_cast<int>(GetRValue(actual)) - GetRValue(expected)) <= 12 &&
        std::abs(static_cast<int>(GetGValue(actual)) - GetGValue(expected)) <= 12 &&
        std::abs(static_cast<int>(GetBValue(actual)) - GetBValue(expected)) <= 12;
}

bool SamplePixel(HDC screen, POINT origin, int x, COLORREF expected,
    const char* label) {
    const COLORREF actual = GetPixel(screen, origin.x + x, origin.y + 80);
    const bool pass = Near(actual, expected);
    std::printf("PIXEL %s actual=%u,%u,%u expected=%u,%u,%u %s\n", label,
        GetRValue(actual), GetGValue(actual), GetBValue(actual),
        GetRValue(expected), GetGValue(expected), GetBValue(expected),
        pass ? "PASS" : "FAIL");
    return pass;
}

bool IsAboveInZOrder(HWND upper, HWND lower) {
    for (HWND current = GetTopWindow(nullptr); current;
        current = GetWindow(current, GW_HWNDNEXT)) {
        if (current == upper) return true;
        if (current == lower) return false;
    }
    return false;
}

struct ForegroundSnapshot {
    HWND window{};
    HWND focus{};
};

ForegroundSnapshot ReadForegroundSnapshot() {
    ForegroundSnapshot snapshot{};
    snapshot.window = GetForegroundWindow();
    if (!snapshot.window) return snapshot;
    const DWORD thread = GetWindowThreadProcessId(snapshot.window, nullptr);
    GUITHREADINFO info{sizeof(info)};
    if (thread && GetGUIThreadInfo(thread, &info)) snapshot.focus = info.hwndFocus;
    return snapshot;
}

void PumpMainFor(DWORD durationMs) {
    const ULONGLONG end = GetTickCount64() + durationMs;
    do {
        MSG message{};
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        Sleep(1);
    } while (GetTickCount64() < end);
}

void OwnerThread() noexcept {
    g.ownerThread.store(GetCurrentThreadId(), std::memory_order_release);
    MSG message{};
    PeekMessageW(&message, nullptr, WM_USER, WM_USER, PM_NOREMOVE);

    HWND overlay{};
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    try {
        if (FAILED(com)) Fail("owner CoInitializeEx failed");
        overlay = CreateWindowExW(WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW |
            WS_EX_LAYERED | WS_EX_TRANSPARENT, kOverlayClass,
            L"G1-B unowned lifecycle fixture", WS_POPUP,
            0, 0, kHostWidth, kHostHeight, nullptr, nullptr,
            GetModuleHandleW(nullptr), nullptr);
        if (!overlay) Fail("owner CreateWindowExW failed");
        g.overlay.store(overlay, std::memory_order_release);
        {
            CompositionResources resources{};
            if (!InitializeComposition(overlay, resources))
                Fail("overlay DirectComposition initialization failed");
            g.ownerCreated.store(true, std::memory_order_release);
            SetEvent(g.ready);

            const HANDLE waits[]{g.shutdown};
            bool done{};
            while (!done) {
                const DWORD wait = MsgWaitForMultipleObjectsEx(1, waits, INFINITE,
                    QS_ALLINPUT, MWMO_INPUTAVAILABLE);
                if (wait == WAIT_OBJECT_0) {
                    done = true;
                    continue;
                }
                if (wait == WAIT_FAILED) {
                    g.error.store(static_cast<LONG>(GetLastError()),
                        std::memory_order_release);
                    done = true;
                    continue;
                }
                if (wait != WAIT_OBJECT_0 + 1) continue;
                while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                    if (message.message == WM_QUIT) {
                        done = true;
                        break;
                    }
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                }
            }
            const HRESULT detach = resources.target->SetRoot(nullptr);
            const HRESULT commit = SUCCEEDED(detach)
                ? resources.compositionDevice->Commit() : detach;
            g.ownerCompositionDetached.store(SUCCEEDED(commit),
                std::memory_order_release);
            if (FAILED(commit))
                g.error.store(static_cast<LONG>(commit), std::memory_order_release);
            resources.ReleaseAll();
            g.ownerResourceRefsReleased.store(resources.Empty(),
                std::memory_order_release);
        }
    } catch (...) {
        g.error.store(static_cast<LONG>(GetLastError()),
            std::memory_order_release);
        SetEvent(g.ready);
    }
    if (SUCCEEDED(com)) CoUninitialize();

    if (overlay && IsWindow(overlay)) {
        const bool correctThread = GetWindowThreadProcessId(overlay, nullptr) ==
            GetCurrentThreadId();
        g.ownerDestroyOnCorrectThread.store(correctThread,
            std::memory_order_release);
        ShowWindow(overlay, SW_HIDE);
        const BOOL destroyed = correctThread ? DestroyWindow(overlay) : FALSE;
        g.ownerDestroyed.store(destroyed != FALSE, std::memory_order_release);
        if (!destroyed && !g.error.load(std::memory_order_acquire))
            g.error.store(static_cast<LONG>(GetLastError()),
                std::memory_order_release);
    }
    SetEvent(g.ownerStopped);
}

void Require(bool condition, const char* what) {
    if (!condition) Fail(what);
}
}

int main() {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    g_savedForeground = GetForegroundWindow();
    GetCursorPos(&g_savedCursor);
    g.ready = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    g.shutdown = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    g.ownerStopped = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g.ready || !g.shutdown || !g.ownerStopped) return 2;

    WNDCLASSW hostClass{};
    hostClass.hInstance = GetModuleHandleW(nullptr);
    hostClass.lpfnWndProc = HostProc;
    hostClass.lpszClassName = kHostClass;
    WNDCLASSW overlayClass = hostClass;
    overlayClass.lpfnWndProc = OverlayProc;
    overlayClass.lpszClassName = kOverlayClass;
    if (!RegisterClassW(&hostClass) || !RegisterClassW(&overlayClass)) return 2;

    RECT rect{0, 0, kHostWidth, kHostHeight};
    if (!AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE)) return 2;
    g_host = CreateWindowExW(0, kHostClass, L"G1-B host surrogate",
        WS_OVERLAPPEDWINDOW, 100, 100, rect.right - rect.left,
        rect.bottom - rect.top, nullptr, nullptr, hostClass.hInstance, nullptr);
    if (!g_host) return 2;
    g_hostThread = GetCurrentThreadId();
    ShowWindow(g_host, SW_SHOWNOACTIVATE);
    UpdateWindow(g_host);
    SetWindowPos(g_host, HWND_TOPMOST, 100, 100, rect.right - rect.left,
        rect.bottom - rect.top, SWP_SHOWWINDOW | SWP_NOACTIVATE);
    std::thread owner(OwnerThread);
    try {
    const DWORD readyWait = WaitForSingleObject(g.ready, 30000);
    if (readyWait != WAIT_OBJECT_0 || !g.ownerCreated.load(std::memory_order_acquire)) {
        std::printf("READY wait=%lu created=%d error=%ld last_error=%lu\n",
            readyWait, g.ownerCreated.load(std::memory_order_acquire),
            g.error.load(std::memory_order_acquire), GetLastError());
        std::fflush(stdout);
        SetEvent(g.shutdown);
        owner.join();
        DestroyWindow(g_host);
        return 2;
    }

    const HWND overlay = g.overlay.load(std::memory_order_acquire);
    Require(overlay && IsWindow(overlay), "overlay not created");
    Require(GetWindow(overlay, GW_OWNER) == nullptr,
        "overlay unexpectedly has a Win32 owner");
    Require((GetWindowLongPtrW(overlay, GWL_STYLE) & WS_CHILD) == 0,
        "overlay unexpectedly has child-window style");
    Require(GetWindowThreadProcessId(overlay, nullptr) ==
        g.ownerThread.load(std::memory_order_acquire),
        "overlay creator thread mismatch");
    Require(g.ownerThread.load(std::memory_order_acquire) != g_hostThread,
        "fixture owner and host unexpectedly share a thread");

    RECT clientRect{};
    Require(GetClientRect(g_host, &clientRect) != FALSE, "GetClientRect host");
    POINT origin{};
    Require(ClientToScreen(g_host, &origin) != FALSE, "ClientToScreen host");
    const auto foregroundBefore = ReadForegroundSnapshot();
    Require(foregroundBefore.window != nullptr,
        "no foreground window available for activation check");
    // Keep both fixture windows topmost and place the display window at the
    // top of that band. Inserting after an unrelated foreground application's
    // HWND can fail with access denied under UIPI.
    Require(SetWindowPos(overlay, HWND_TOPMOST, origin.x, origin.y,
        clientRect.right - clientRect.left, clientRect.bottom - clientRect.top,
        SWP_NOACTIVATE | SWP_SHOWWINDOW) != FALSE,
        "show unowned overlay without activation");
    Require(IsAboveInZOrder(overlay, g_host),
        "overlay was not placed above host in z-order");
    const auto foregroundAfter = ReadForegroundSnapshot();
    const bool activationPreserved = foregroundAfter.window == foregroundBefore.window &&
        foregroundAfter.focus == foregroundBefore.focus;
    std::printf("ACTIVATION before=%p/%p after=%p/%p preserved=%d\n",
        foregroundBefore.window, foregroundBefore.focus, foregroundAfter.window,
        foregroundAfter.focus, activationPreserved);
    Require(activationPreserved, "showing overlay changed foreground/focus");

    Require(SUCCEEDED(DwmFlush()), "DwmFlush after target commit");
    HDC screen = GetDC(nullptr);
    Require(screen != nullptr, "GetDC screen");
    const ULONGLONG compositionDeadline = GetTickCount64() + 5000;
    bool pixelsReady{};
    while (GetTickCount64() < compositionDeadline) {
        DwmFlush();
        const COLORREF clearPixel = GetPixel(screen, origin.x + 80, origin.y + 80);
        const COLORREF blendPixel = GetPixel(screen, origin.x + 240, origin.y + 80);
        const COLORREF opaquePixel = GetPixel(screen, origin.x + 400, origin.y + 80);
        if (Near(clearPixel, kHostColor) && Near(blendPixel, RGB(148, 50, 80)) &&
            Near(opaquePixel, RGB(255, 0, 0))) {
            pixelsReady = true;
            break;
        }
        Sleep(50);
    }
    const bool transparent = SamplePixel(screen, origin, 80, kHostColor,
        "transparent_pixel");
    Sleep(100);
    Require(SUCCEEDED(DwmFlush()), "DwmFlush before pixel sampling");
    const bool halfAlpha = SamplePixel(screen, origin, 240,
        RGB(148, 50, 80), "half_alpha_pixel");
    const bool opaque = SamplePixel(screen, origin, 400, RGB(255, 0, 0),
        "opaque_pixel");
    ReleaseDC(nullptr, screen);
    Require(pixelsReady && transparent && halfAlpha && opaque,
        "DComp alpha output mismatch on unowned HWND");

    POINT click{origin.x + 400, origin.y + 180};
    const HWND hitWindow = WindowFromPoint(click);
    std::printf("HITTEST point=%ld,%ld result=%p host=%p overlay=%p overlay_exstyle=0x%llX\n",
        click.x, click.y, hitWindow, g_host, overlay,
        static_cast<unsigned long long>(GetWindowLongPtrW(overlay, GWL_EXSTYLE)));
    Require(hitWindow == g_host,
        "pass-through hit-test did not resolve to GameHwnd surrogate");
    std::printf("INPUT hit_test_pass_through=1 target=GameHwnd\n");

    const HWND host = g_host;
    Require(DestroyWindow(host) != FALSE, "destroy host surrogate");
    g_host = nullptr;
    Require(!IsWindow(host), "host surrogate still exists");
    Require(IsWindow(overlay), "unowned overlay died with host surrogate");
    Require(GetWindow(overlay, GW_OWNER) == nullptr,
        "unowned overlay acquired an owner after host destruction");
    std::printf("HOST_DESTROY overlay_survived=1 owner_null=1 creator_thread=%lu\n",
        g.ownerThread.load(std::memory_order_acquire));

    SetEvent(g.shutdown);
    const DWORD stoppedWait = WaitForSingleObject(g.ownerStopped, 5000);
    Require(stoppedWait == WAIT_OBJECT_0, "owner thread did not finalize");
    owner.join();
    Require(g.ownerDestroyOnCorrectThread.load(std::memory_order_acquire),
        "overlay destruction was not attempted by creator thread");
    Require(g.ownerCompositionDetached.load(std::memory_order_acquire),
        "DComp root detach and commit did not succeed");
    Require(g.ownerResourceRefsReleased.load(std::memory_order_acquire),
        "owner still holds D3D11/DComp references after release");
    Require(g.ownerDestroyed.load(std::memory_order_acquire),
        "owner-thread DestroyWindow failed");
    Require(g.overlayNcDestroy.load(std::memory_order_acquire),
        "overlay WM_NCDESTROY was not observed");
    Require(!IsWindow(overlay), "overlay HWND survived owner termination");
    Require(g.error.load(std::memory_order_acquire) == 0,
        "owner thread reported an error");

    std::printf("OWNER_EXIT dcomp_detached=1 resource_refs_released=1 destroy_on_creator_thread=1 nc_destroy=1 hwnd_gone=1\n");
    std::printf("RESULT unowned_lifecycle=PASS no_activation_theft=PASS z_order=PASS alpha=PASS hit_test_pass_through=PASS host_destroy=PASS owner_cleanup=PASS\n");
    CloseHandle(g.ownerStopped);
    CloseHandle(g.shutdown);
    CloseHandle(g.ready);
    SetCursorPos(g_savedCursor.x, g_savedCursor.y);
    if (IsWindow(g_savedForeground)) SetForegroundWindow(g_savedForeground);
    return 0;
    } catch (const std::exception& error) {
        std::printf("RESULT unowned_lifecycle=FAIL reason=%s win32=%lu\n",
            error.what(), GetLastError());
        if (g.shutdown) SetEvent(g.shutdown);
        if (g.ownerStopped) WaitForSingleObject(g.ownerStopped, 5000);
        if (owner.joinable()) owner.join();
        if (g_host && IsWindow(g_host)) DestroyWindow(g_host);
        SetCursorPos(g_savedCursor.x, g_savedCursor.y);
        if (IsWindow(g_savedForeground)) SetForegroundWindow(g_savedForeground);
        if (g.ownerStopped) CloseHandle(g.ownerStopped);
        if (g.shutdown) CloseHandle(g.shutdown);
        if (g.ready) CloseHandle(g.ready);
        return 1;
    }
}
