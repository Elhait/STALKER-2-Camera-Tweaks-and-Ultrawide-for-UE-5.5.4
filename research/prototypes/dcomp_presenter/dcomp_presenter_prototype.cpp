#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <d3d11.h>
#include <dcomp.h>
#include <dwrite.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <cwchar>
#include <iterator>
#include <string>

namespace
{
    using Microsoft::WRL::ComPtr;

    constexpr UINT_PTR RefreshMessage = WM_APP + 0x251;
    constexpr UINT_PTR StopMessage = WM_APP + 0x252;

    HMODULE g_module{};
    DWORD g_uiThreadId{};
    DWORD g_processId{};
    HANDLE g_logFile{INVALID_HANDLE_VALUE};
    HWINEVENTHOOK g_objectEvents{};
    HWINEVENTHOOK g_locationEvents{};
    HWND g_gameWindow{};
    UINT g_surfaceWidth{};
    UINT g_surfaceHeight{};
    std::uint64_t g_surfaceGeneration{};
    bool g_wasMinimized{};
    bool g_topmostTarget{};

    // Keep COM owners process-resident; avoid graphics teardown in DLL detach.
    auto& g_d3dDevice = *new ComPtr<ID3D11Device>;
    auto& g_dxgiDevice = *new ComPtr<IDXGIDevice>;
    auto& g_d2dFactory = *new ComPtr<ID2D1Factory1>;
    auto& g_d2dDevice = *new ComPtr<ID2D1Device>;
    auto& g_writeFactory = *new ComPtr<IDWriteFactory>;
    auto& g_textFormat = *new ComPtr<IDWriteTextFormat>;
    auto& g_compositionDevice = *new ComPtr<IDCompositionDesktopDevice>;
    auto& g_compositionTarget = *new ComPtr<IDCompositionTarget>;
    auto& g_visual = *new ComPtr<IDCompositionVisual2>;
    auto& g_surface = *new ComPtr<IDCompositionSurface>;

    void Log(const char* message) noexcept
    {
        if (g_logFile == INVALID_HANDLE_VALUE) return;
        SYSTEMTIME now{};
        GetLocalTime(&now);
        char line[768]{};
        const int length = wsprintfA(line, "%04u-%02u-%02u %02u:%02u:%02u.%03u %s\r\n",
            now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute,
            now.wSecond, now.wMilliseconds, message);
        if (length <= 0) return;
        DWORD written{};
        WriteFile(g_logFile, line, static_cast<DWORD>(length), &written, nullptr);
        FlushFileBuffers(g_logFile);
    }

    void OpenLog() noexcept
    {
        wchar_t modulePath[32768]{};
        const DWORD length = GetModuleFileNameW(g_module, modulePath,
            static_cast<DWORD>(std::size(modulePath)));
        if (!length || length >= std::size(modulePath)) return;
        std::wstring path(modulePath, length);
        const auto separator = path.find_last_of(L"\\/");
        if (separator != std::wstring::npos) path.resize(separator + 1);
        path += L"STALKER2CameraTweaksDCompPrototype.log";
        g_logFile = CreateFileW(path.c_str(), FILE_APPEND_DATA,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
            OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    }

    struct WindowCandidate
    {
        HWND window{};
        std::uint64_t area{};
    };

    BOOL CALLBACK InspectWindow(HWND window, LPARAM parameter)
    {
        auto& best = *reinterpret_cast<WindowCandidate*>(parameter);
        DWORD ownerProcess{};
        GetWindowThreadProcessId(window, &ownerProcess);
        if (ownerProcess != g_processId || !IsWindowVisible(window) ||
            GetWindow(window, GW_OWNER) || (GetWindowLongPtrW(window, GWL_STYLE) & WS_CHILD))
            return TRUE;

        RECT client{};
        if (!GetClientRect(window, &client)) return TRUE;
        const auto width = std::max<LONG>(0, client.right - client.left);
        const auto height = std::max<LONG>(0, client.bottom - client.top);
        if (width < 320 || height < 200) return TRUE;
        const auto area = static_cast<std::uint64_t>(width) *
            static_cast<std::uint64_t>(height);
        if (area > best.area) best = {window, area};
        return TRUE;
    }

    HWND FindGameWindow() noexcept
    {
        WindowCandidate best{};
        EnumWindows(&InspectWindow, reinterpret_cast<LPARAM>(&best));
        return best.window;
    }

    void CALLBACK OnWindowEvent(HWINEVENTHOOK, DWORD event, HWND window,
        LONG objectId, LONG childId, DWORD, DWORD)
    {
        if (objectId != OBJID_WINDOW || childId != CHILDID_SELF || !window) return;
        DWORD ownerProcess{};
        GetWindowThreadProcessId(window, &ownerProcess);
        if (ownerProcess != g_processId) return;
        if (event == EVENT_OBJECT_CREATE || event == EVENT_OBJECT_SHOW ||
            event == EVENT_OBJECT_DESTROY || event == EVENT_OBJECT_LOCATIONCHANGE)
            PostThreadMessageW(g_uiThreadId, RefreshMessage,
                reinterpret_cast<WPARAM>(window), static_cast<LPARAM>(event));
    }

    void ResetComposition() noexcept
    {
        if (g_compositionTarget) {
            g_compositionTarget->SetRoot(nullptr);
            if (g_compositionDevice) g_compositionDevice->Commit();
        }
        g_surface.Reset();
        g_visual.Reset();
        g_compositionTarget.Reset();
        g_compositionDevice.Reset();
        g_textFormat.Reset();
        g_writeFactory.Reset();
        g_d2dDevice.Reset();
        g_d2dFactory.Reset();
        g_dxgiDevice.Reset();
        g_d3dDevice.Reset();
        g_surfaceWidth = 0;
        g_surfaceHeight = 0;
        g_topmostTarget = false;
    }

    HRESULT CreateSurface(UINT width, UINT height,
        ComPtr<IDCompositionSurface>& surface) noexcept
    {
        if (!width || !height || width > 16384 || height > 16384)
            return E_INVALIDARG;
        return g_compositionDevice->CreateSurface(width, height,
            DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ALPHA_MODE_PREMULTIPLIED,
            surface.GetAddressOf());
    }

    HRESULT DrawSurface(IDCompositionSurface* surface, UINT width, UINT height) noexcept
    {
        ComPtr<ID2D1DeviceContext> drawContext;
        POINT offset{};
        HRESULT result = surface->BeginDraw(nullptr, __uuidof(ID2D1DeviceContext),
            reinterpret_cast<void**>(drawContext.GetAddressOf()), &offset);
        if (FAILED(result)) return result;

        drawContext->SetTransform(D2D1::Matrix3x2F::Translation(
            static_cast<float>(offset.x), static_cast<float>(offset.y)));
        drawContext->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

        ComPtr<ID2D1SolidColorBrush> panel;
        ComPtr<ID2D1SolidColorBrush> accent;
        ComPtr<ID2D1SolidColorBrush> text;
        result = drawContext->CreateSolidColorBrush(
            D2D1::ColorF(0.025f, 0.035f, 0.045f, 0.92f), panel.GetAddressOf());
        if (SUCCEEDED(result)) result = drawContext->CreateSolidColorBrush(
            D2D1::ColorF(0.1f, 0.95f, 0.4f, 1.0f), accent.GetAddressOf());
        if (SUCCEEDED(result)) result = drawContext->CreateSolidColorBrush(
            D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), text.GetAddressOf());

        const float left = 36.0f;
        const float top = 36.0f;
        const float right = std::min<float>(static_cast<float>(width) - 16.0f, 610.0f);
        const float bottom = std::min<float>(static_cast<float>(height) - 16.0f, 132.0f);
        if (SUCCEEDED(result) && right > left && bottom > top) {
            const auto panelRect = D2D1::RoundedRect(
                D2D1::RectF(left, top, right, bottom), 9.0f, 9.0f);
            drawContext->FillRoundedRectangle(panelRect, panel.Get());
            drawContext->DrawRoundedRectangle(panelRect, accent.Get(), 2.0f);
            const wchar_t* label = L"CAMERA TWEAKS — DIRECTCOMPOSITION PROTOTYPE";
            drawContext->DrawText(label, static_cast<UINT32>(wcslen(label)),
                g_textFormat.Get(), D2D1::RectF(left + 20.0f, top + 19.0f,
                    right - 14.0f, bottom - 10.0f), text.Get());
        }

        const HRESULT endResult = surface->EndDraw();
        return FAILED(result) ? result : endResult;
    }

    HRESULT InitializeComposition(HWND window, UINT width, UINT height) noexcept
    {
        UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        D3D_FEATURE_LEVEL selected{};
        constexpr D3D_FEATURE_LEVEL levels[]{D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0};
        HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE,
            nullptr, flags, levels, static_cast<UINT>(std::size(levels)),
            D3D11_SDK_VERSION, g_d3dDevice.GetAddressOf(), &selected, nullptr);
        if (result == E_INVALIDARG) {
            result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE,
                nullptr, flags, &levels[1], 1, D3D11_SDK_VERSION,
                g_d3dDevice.GetAddressOf(), &selected, nullptr);
        }
        if (FAILED(result)) return result;
        result = g_d3dDevice.As(&g_dxgiDevice);
        if (FAILED(result)) return result;
        ComPtr<IDXGIAdapter> adapter;
        DXGI_ADAPTER_DESC adapterDescription{};
        bool adapterMatchesWindowMonitor = false;
        if (SUCCEEDED(g_dxgiDevice->GetAdapter(adapter.GetAddressOf())) &&
            SUCCEEDED(adapter->GetDesc(&adapterDescription))) {
            const HMONITOR gameMonitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
            for (UINT index = 0;; ++index) {
                ComPtr<IDXGIOutput> output;
                if (FAILED(adapter->EnumOutputs(index, output.GetAddressOf()))) break;
                DXGI_OUTPUT_DESC outputDescription{};
                if (SUCCEEDED(output->GetDesc(&outputDescription)) &&
                    outputDescription.Monitor == gameMonitor) {
                    adapterMatchesWindowMonitor = true;
                    break;
                }
            }
            char message[256]{};
            wsprintfA(message, "D3D11_DEVICE_READY vendor=0x%04X device=0x%04X window_monitor_match=%u",
                adapterDescription.VendorId, adapterDescription.DeviceId,
                adapterMatchesWindowMonitor ? 1u : 0u);
            Log(message);
        }

        D2D1_FACTORY_OPTIONS options{};
        result = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
            __uuidof(ID2D1Factory1), &options,
            reinterpret_cast<void**>(g_d2dFactory.GetAddressOf()));
        if (FAILED(result)) return result;
        result = g_d2dFactory->CreateDevice(g_dxgiDevice.Get(), g_d2dDevice.GetAddressOf());
        if (FAILED(result)) return result;
        result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(
                g_writeFactory.GetAddressOf()));
        if (FAILED(result)) return result;
        result = g_writeFactory->CreateTextFormat(L"Segoe UI", nullptr,
            DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, 22.0f, L"en-us",
            g_textFormat.GetAddressOf());
        if (FAILED(result)) return result;

        result = DCompositionCreateDevice2(g_d2dDevice.Get(),
            __uuidof(IDCompositionDesktopDevice),
            reinterpret_cast<void**>(g_compositionDevice.GetAddressOf()));
        if (FAILED(result)) return result;
        result = g_compositionDevice->CreateTargetForHwnd(window, TRUE,
            g_compositionTarget.GetAddressOf());
        g_topmostTarget = SUCCEEDED(result);
        if (FAILED(result)) {
            Log("COMPOSITION_TARGET_TOPMOST_FAILED; trying non-topmost target");
            result = g_compositionDevice->CreateTargetForHwnd(window, FALSE,
                g_compositionTarget.GetAddressOf());
        }
        if (FAILED(result)) return result;
        Log(g_topmostTarget ? "COMPOSITION_TARGET_READY layer=topmost"
            : "COMPOSITION_TARGET_READY layer=below_child_windows");
        result = g_compositionDevice->CreateVisual(g_visual.GetAddressOf());
        if (FAILED(result)) return result;
        result = CreateSurface(width, height, g_surface);
        if (FAILED(result)) return result;
        result = g_visual->SetContent(g_surface.Get());
        if (FAILED(result)) return result;
        result = g_compositionTarget->SetRoot(g_visual.Get());
        if (FAILED(result)) return result;
        result = DrawSurface(g_surface.Get(), width, height);
        if (FAILED(result)) return result;
        result = g_compositionDevice->Commit();
        if (FAILED(result)) return result;
        g_surfaceWidth = width;
        g_surfaceHeight = height;
        ++g_surfaceGeneration;
        Log("SURFACE_INITIAL_CONTENT_DRAWN");
        return S_OK;
    }

    void Refresh(HWND observedWindow, DWORD event) noexcept
    {
        HWND candidate = FindGameWindow();
        if (!candidate) return;
        if (candidate != g_gameWindow) {
            ResetComposition();
            g_gameWindow = candidate;
            RECT client{};
            GetClientRect(candidate, &client);
            const UINT width = static_cast<UINT>(std::max<LONG>(0, client.right - client.left));
            const UINT height = static_cast<UINT>(std::max<LONG>(0, client.bottom - client.top));
            char message[256]{};
            wsprintfA(message, "GAME_WINDOW_SELECTED hwnd=%p width=%u height=%u",
                candidate, width, height);
            Log(message);
            const HRESULT result = InitializeComposition(candidate, width, height);
            wsprintfA(message, "COMPOSITION_ATTACH result=0x%08lX route=game_hwnd",
                static_cast<unsigned long>(result));
            Log(message);
            if (FAILED(result)) {
                ResetComposition();
                g_gameWindow = nullptr;
            }
            return;
        }

        if (observedWindow != candidate || event == EVENT_OBJECT_DESTROY) return;
        if (IsIconic(candidate)) {
            if (!g_wasMinimized) Log("GAME_WINDOW_MINIMIZED");
            g_wasMinimized = true;
            return;
        }
        if (g_wasMinimized) {
            Log("GAME_WINDOW_RESTORED");
            g_wasMinimized = false;
        }
        if (!g_compositionDevice || !g_visual || !g_surface) {
            RECT client{};
            if (!GetClientRect(candidate, &client)) return;
            const UINT retryWidth = static_cast<UINT>(std::max<LONG>(0, client.right - client.left));
            const UINT retryHeight = static_cast<UINT>(std::max<LONG>(0, client.bottom - client.top));
            const HRESULT retryResult = InitializeComposition(candidate, retryWidth, retryHeight);
            char retryMessage[128]{};
            wsprintfA(retryMessage, "COMPOSITION_RETRY result=0x%08lX",
                static_cast<unsigned long>(retryResult));
            Log(retryMessage);
            if (FAILED(retryResult)) ResetComposition();
            return;
        }
        RECT client{};
        if (!GetClientRect(candidate, &client)) return;
        const UINT width = static_cast<UINT>(std::max<LONG>(0, client.right - client.left));
        const UINT height = static_cast<UINT>(std::max<LONG>(0, client.bottom - client.top));
        if (width == g_surfaceWidth && height == g_surfaceHeight) return;

        ComPtr<IDCompositionSurface> replacement;
        HRESULT result = CreateSurface(width, height, replacement);
        if (SUCCEEDED(result)) result = DrawSurface(replacement.Get(), width, height);
        if (SUCCEEDED(result)) result = g_visual->SetContent(replacement.Get());
        if (SUCCEEDED(result)) result = g_compositionDevice->Commit();
        if (SUCCEEDED(result)) {
            g_surface = std::move(replacement);
            g_surfaceWidth = width;
            g_surfaceHeight = height;
            ++g_surfaceGeneration;
        }
        char message[320]{};
        std::snprintf(message, sizeof(message),
            "SURFACE_RESIZE event=%lu generation=%llu width=%u height=%u result=0x%08X",
            static_cast<unsigned long>(event),
            static_cast<unsigned long long>(g_surfaceGeneration), width, height,
            static_cast<unsigned int>(result));
        Log(message);
    }

    DWORD WINAPI Worker(void*) noexcept
    {
        g_uiThreadId = GetCurrentThreadId();
        g_processId = GetCurrentProcessId();
        OpenLog();
        char marker[192]{};
        wsprintfA(marker,
            "DCOMP_PROTOTYPE v=1 pid=%lu tid=%lu camera_core=not_included dxgi_factory_hooks=0 swapchain_hooks=0",
            static_cast<unsigned long>(g_processId),
            static_cast<unsigned long>(g_uiThreadId));
        Log(marker);

        MSG message{};
        PeekMessageW(&message, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
        g_objectEvents = SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_SHOW,
            nullptr, &OnWindowEvent, g_processId, 0, WINEVENT_OUTOFCONTEXT);
        g_locationEvents = SetWinEventHook(EVENT_OBJECT_LOCATIONCHANGE,
            EVENT_OBJECT_LOCATIONCHANGE, nullptr, &OnWindowEvent,
            g_processId, 0, WINEVENT_OUTOFCONTEXT);
        Log(g_objectEvents && g_locationEvents
            ? "WINDOW_EVENTS_ARMED mode=out_of_context"
            : "WINDOW_EVENTS_ARM_FAILED");
        Refresh(nullptr, 0);

        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            if (message.message == RefreshMessage) {
                Refresh(reinterpret_cast<HWND>(message.wParam),
                    static_cast<DWORD>(message.lParam));
            } else if (message.message == StopMessage) {
                break;
            }
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        if (g_objectEvents) UnhookWinEvent(g_objectEvents);
        if (g_locationEvents) UnhookWinEvent(g_locationEvents);
        ResetComposition();
        if (g_logFile != INVALID_HANDLE_VALUE) CloseHandle(g_logFile);
        return 0;
    }
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = instance;
        DisableThreadLibraryCalls(instance);
        HANDLE thread = CreateThread(nullptr, 0, &Worker, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
