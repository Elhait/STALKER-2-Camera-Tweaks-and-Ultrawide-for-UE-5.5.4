#include "overlay_lifecycle.hpp"
#include "discovery_evidence.hpp"
#include "game_language_reader.hpp"
#include "optional_overlay_boundary.hpp"
#include "../diagnostics/diagnostic_runtime.hpp"
#include "../diagnostics/performance_telemetry.hpp"
#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
#include "renderer_runtime.hpp"
#include "input_state.hpp"
#include "../plugin/runtime.hpp"
#include <backends/imgui_impl_win32.h>
#include <imgui_internal.h>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

#include <dxgi1_4.h>
#include <d3d12.h>
#include <safetyhook.hpp>

#include <Windows.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace
{
    using CreateFactoryFn = HRESULT(WINAPI*)(REFIID, void**);
    using CreateSwapChainFn = HRESULT(STDMETHODCALLTYPE*)(
        IDXGIFactory*, IUnknown*, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**);
    using CreateSwapChainForHwndFn = HRESULT(STDMETHODCALLTYPE*)(
        IDXGIFactory2*, IUnknown*, HWND, const DXGI_SWAP_CHAIN_DESC1*,
        const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*, IDXGIOutput*, IDXGISwapChain1**);
    using CreateSwapChainForCoreWindowFn = HRESULT(STDMETHODCALLTYPE*)(
        IDXGIFactory2*, IUnknown*, const DXGI_SWAP_CHAIN_DESC1*,
        IUnknown*, IDXGISwapChain1**);
    using PresentFn = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);
    using ResizeBuffersFn = HRESULT(STDMETHODCALLTYPE*)(
        IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

    struct FactoryHooks
    {
        void* object{};
        safetyhook::VmtHook vmt;
        safetyhook::VmHook createSwapChain;
        safetyhook::VmHook createSwapChainForHwnd;
        safetyhook::VmHook createSwapChainForCoreWindow;
        safetyhook::VmHook createSwapChainForComposition;
    };

    struct SwapchainHooks
    {
        IDXGISwapChain* object{};
        safetyhook::VmtHook vmt;
        safetyhook::VmHook present;
        safetyhook::VmHook resizeBuffers;
        std::uint32_t presentObservations{};
        std::uint32_t resizeObservations{};
    };

    std::mutex g_mutex;
    std::ofstream g_log;
    std::vector<std::unique_ptr<FactoryHooks>> g_factoryHooks;
    std::vector<std::unique_ptr<SwapchainHooks>> g_swapchainHooks;
    overlay::FactoryEvidenceStore g_factoryEvidence;
    safetyhook::InlineHook g_createFactory;
    safetyhook::InlineHook g_createFactory1;
    safetyhook::InlineHook g_createFactory2;
    overlay::Lifecycle g_lifecycle;
    overlay::ObservationWindow g_presentWindow;
    overlay::QueueEvidenceStore g_evidence;
#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
    overlay::Renderer g_renderer;
    HWND g_inputWindow{};
    WNDPROC g_originalWindowProc{};
#endif
    std::atomic<bool> g_stopping{};

    void RestoreInputAfterTerminalFailure()
    {
#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
        if (!g_renderer.disabled()) return;
        plugin::SetHotkeyRebindCaptureActive(false);
        plugin::MarkHotkeyRebindKeyConsumed(0);
        if (g_inputWindow && g_originalWindowProc) {
            CallWindowProcW(g_originalWindowProc, g_inputWindow, WM_SETCURSOR,
                reinterpret_cast<WPARAM>(g_inputWindow),
                MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
        }
#endif
    }

    void DisableAfterOverlayException() noexcept
    {
        g_lifecycle.Fail();
#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
        g_renderer.Disable("overlay_callback_exception");
#endif
    }
    HMODULE g_module{};

    void Log(const std::string& message)
    {
        std::scoped_lock lock{g_mutex};
        if (!g_log) return;
        g_log << message << '\n';
        g_log.flush();
    }

    void LogDiagnostic(const std::string& message)
    {
        if (diagnostics::Enabled()) Log(message);
    }

#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
#endif

    std::string Hex(const void* value)
    {
        char buffer[32]{};
        std::snprintf(buffer, sizeof(buffer), "0x%llx",
            static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(value)));
        return buffer;
    }

    const char* QueueTypeName(D3D12_COMMAND_LIST_TYPE type)
    {
        switch (type) {
        case D3D12_COMMAND_LIST_TYPE_DIRECT: return "DIRECT";
        case D3D12_COMMAND_LIST_TYPE_BUNDLE: return "BUNDLE";
        case D3D12_COMMAND_LIST_TYPE_COMPUTE: return "COMPUTE";
        case D3D12_COMMAND_LIST_TYPE_COPY: return "COPY";
        default: return "UNKNOWN";
        }
    }

    const char* AssociationStateName(overlay::AssociationState state)
    {
        switch (state) {
        case overlay::AssociationState::Unknown: return "UNKNOWN";
        case overlay::AssociationState::Supported: return "SUPPORTED";
        case overlay::AssociationState::ResizeRevalidationRequired:
            return "RESIZE_REVALIDATION_REQUIRED";
        case overlay::AssociationState::Ambiguous: return "AMBIGUOUS";
        default: return "UNKNOWN";
        }
    }

    SwapchainHooks* FindSwapchain(IDXGISwapChain* object)
    {
        for (auto& candidate : g_swapchainHooks)
            if (candidate->object == object) return candidate.get();
        return nullptr;
    }

    FactoryHooks* FindFactory(void* object)
    {
        for (auto& candidate : g_factoryHooks)
            if (candidate->object == object) return candidate.get();
        return nullptr;
    }

    const char* FactoryInterfaceName(REFIID riid)
    {
        if (riid == __uuidof(IDXGIFactory2)) return "IDXGIFactory2";
        if (riid == __uuidof(IDXGIFactory1)) return "IDXGIFactory1";
        if (riid == __uuidof(IDXGIFactory)) return "IDXGIFactory";
        return "Other";
    }

    const char* FactoryMethodName(overlay::FactoryMethod method)
    {
        switch (method) {
        case overlay::FactoryMethod::CreateSwapChain: return "CreateSwapChain";
        case overlay::FactoryMethod::CreateSwapChainForHwnd: return "CreateSwapChainForHwnd";
        case overlay::FactoryMethod::CreateSwapChainForCoreWindow: return "CreateSwapChainForCoreWindow";
        case overlay::FactoryMethod::CreateSwapChainForComposition: return "CreateSwapChainForComposition";
        default: return "Unknown";
        }
    }

#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
    bool IsInputWindowMessage(UINT message) noexcept
    {
        return overlay::GetInputState().ShouldCapture(message);
    }

    bool IsRawMouseInput(LPARAM rawInputHandle) noexcept
    {
        RAWINPUTHEADER header{};
        UINT headerSize = sizeof(header);
        const UINT copied = GetRawInputData(
            reinterpret_cast<HRAWINPUT>(rawInputHandle), RID_HEADER,
            &header, &headerSize, sizeof(RAWINPUTHEADER));
        return copied == sizeof(header) && header.dwType == RIM_TYPEMOUSE;
    }

    struct OverlayWindowDecision
    {
        bool handled{};
        LRESULT result{};
        bool callDefaultProcedure{};
        bool restoreGameCursor{};
    };

    bool IsAnyImGuiPopupOpen() noexcept
    {
        return ImGui::GetCurrentContext() && ImGui::IsPopupOpen(
            static_cast<ImGuiID>(0), ImGuiPopupFlags_AnyPopup);
    }

    OverlayWindowDecision ProcessOverlayWindowMessage(HWND window, UINT message,
        WPARAM wParam, LPARAM lParam)
    {
        OverlayWindowDecision decision{};
        auto& input = overlay::GetInputState();
        if (message == overlay::AutoLanguageSyncMessage) {
            overlay::SynchronizeAutoLanguageIfConfigured();
            return {true, 0};
        }
        const auto captureResult = overlay::InputState::IsRebindMessage(message)
            ? input.HandleRebindMessage(message, wParam,
                overlay::InputState::IsRebindKeyDownMessage(message) &&
                    ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0 ||
                        (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0 ||
                        (GetAsyncKeyState(VK_MENU) & 0x8000) != 0))
            : overlay::RebindMessageResult::PassThrough;
        if (captureResult == overlay::RebindMessageResult::Cancelled) {
            plugin::SetHotkeyRebindCaptureActive(false);
            return {true, 0};
        }
        if (captureResult == overlay::RebindMessageResult::CapturedKey) {
            const int key = static_cast<int>(wParam);
            const auto action = input.rebindAction();
            plugin::MarkHotkeyRebindKeyConsumed(key);
            auto& api = plugin::GetRuntimeSettingsApi();
            const auto makeMutation = [action, key]() {
                switch (action) {
                case overlay::HotkeyAction::GameplayMode:
                    return plugin::RuntimeSettingMutation::GameplayHotkey(key);
                case overlay::HotkeyAction::CinematicAspect:
                    return plugin::RuntimeSettingMutation::CinematicAspectHotkey(key);
                case overlay::HotkeyAction::CinematicFov:
                    return plugin::RuntimeSettingMutation::CinematicFovHotkey(key);
                case overlay::HotkeyAction::DialogueZoom:
                    return plugin::RuntimeSettingMutation::DialogueHotkey(key);
                case overlay::HotkeyAction::OverlayToggle:
                    return plugin::RuntimeSettingMutation::OverlayToggleHotkey(key);
                }
                return plugin::RuntimeSettingMutation::GameplayHotkey(key);
            };
            const auto mutation = makeMutation();
            const auto result = api.Apply(mutation);
            const auto notificationAction = [action]() {
                switch (action) {
                case overlay::HotkeyAction::GameplayMode: return plugin::OverlayNotificationAction::GameplayMode;
                case overlay::HotkeyAction::CinematicAspect: return plugin::OverlayNotificationAction::CinematicAspect;
                case overlay::HotkeyAction::CinematicFov: return plugin::OverlayNotificationAction::CinematicFov;
                case overlay::HotkeyAction::DialogueZoom: return plugin::OverlayNotificationAction::DialogueZoom;
                case overlay::HotkeyAction::OverlayToggle: return plugin::OverlayNotificationAction::OverlayToggle;
                }
                return plugin::OverlayNotificationAction::AnotherSetting;
            };
            if (result.accepted) {
                const bool persisted = api.Persist(mutation);
                plugin::PublishOverlayNotification(plugin::OverlayNotificationKind::BindingChanged,
                    notificationAction(), config::HotkeyName(input.previousBinding()),
                    config::HotkeyName(key),
                    persisted ? plugin::OverlayNotificationStatus::Applied
                        : plugin::OverlayNotificationStatus::Error);
                input.CommitAcceptedRebind(key);
                plugin::SetHotkeyRebindCaptureActive(false);
            } else {
                plugin::RuntimeSettingsSnapshot settings{};
                api.Snapshot(settings);
                const auto conflictAction = settings.overlayToggleKey == key
                    ? plugin::OverlayNotificationAction::OverlayToggle :
                    settings.gameplayCycleKey == key ? plugin::OverlayNotificationAction::GameplayMode :
                    settings.cinematicCycleKey == key ? plugin::OverlayNotificationAction::CinematicAspect :
                    settings.cinematicFovCycleKey == key ? plugin::OverlayNotificationAction::CinematicFov :
                    settings.dialogueCycleKey == key ? plugin::OverlayNotificationAction::DialogueZoom :
                    plugin::OverlayNotificationAction::AnotherSetting;
                plugin::PublishOverlayNotification(plugin::OverlayNotificationKind::HotkeyConflict,
                    conflictAction, config::HotkeyName(key), {},
                    plugin::OverlayNotificationStatus::Error);
            }
            return {true, 0};
        }
        if (captureResult == overlay::RebindMessageResult::Consumed) {
            if (message == WM_KEYUP || message == WM_SYSKEYUP)
                plugin::MarkHotkeyRebindKeyConsumed(0);
            return {true, 0};
        }
        const bool isEscape = wParam == VK_ESCAPE &&
            (message == WM_KEYDOWN || message == WM_KEYUP ||
                message == WM_SYSKEYDOWN || message == WM_SYSKEYUP);
        const bool escapeDismiss = input.ShouldDismissOnEscapeMessage(message,
            wParam, isEscape && input.visible() && IsAnyImGuiPopupOpen());
        plugin::RuntimeSettingsSnapshot toggleSettings{};
        const int toggleKey = isEscape
            ? (escapeDismiss ? VK_ESCAPE : 0)
            : message == WM_KEYUP &&
                plugin::GetRuntimeSettingsApi().Snapshot(toggleSettings)
                ? toggleSettings.overlayToggleKey : VK_DELETE;
        const UINT toggleMessage = escapeDismiss ? WM_KEYUP : message;
        if (input.HandleToggleMessage(toggleMessage, wParam, toggleKey)) {
            plugin::SetHotkeyRebindCaptureActive(false);
            g_renderer.SetVisible(input.visible());
            if (input.visible()) {
                diagnostics::RequestRuntimePerformanceSummary();
                overlay::SynchronizeAutoLanguageOnOverlayOpen(input.visible());
                SetCursor(nullptr);
            } else if (g_originalWindowProc) {
                decision.restoreGameCursor = true;
                Log("OVERLAY_CURSOR_RESTORE=GAME_WNDPROC");
            }
            Log(std::string("OVERLAY_VISIBILITY=") +
                (input.visible() ? "VISIBLE" : "HIDDEN"));
            decision.handled = true;
            return decision;
        }
        if (input.visible()) {
            ImGui_ImplWin32_WndProcHandler(window, message, wParam, lParam);
            // Escape belongs exclusively to the open overlay: ImGui still sees
            // it for popup dismissal, but the game must not open its pause menu.
            if (input.ShouldConsumeEscapeFromGame(message, wParam))
                return {true, 0};
            if (message == WM_SETCURSOR && LOWORD(lParam) == HTCLIENT) {
                SetCursor(nullptr);
                return {true, TRUE};
            }
            if (message == WM_INPUT &&
                input.ShouldCaptureRawInput(IsRawMouseInput(lParam))) {
                return {true, 0, true};
            }
            if (IsInputWindowMessage(message)) {
                return {true, 0};
            }
        }
        if (message == WM_KILLFOCUS ||
            (message == WM_ACTIVATEAPP && wParam == FALSE)) {
            input.ResetEscapeDismissSequence();
            input.CancelRebind();
            plugin::SetHotkeyRebindCaptureActive(false);
        }
        return decision;
    }

    void FinishOverlayWindowMessage(UINT message)
    {
        auto& input = overlay::GetInputState();
        if (message == WM_DESTROY || message == WM_NCDESTROY) {
            if (input.visible()) {
                input.Toggle();
                input.CancelRebind();
                plugin::SetHotkeyRebindCaptureActive(false);
                g_renderer.SetVisible(false);
                SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32512)));
                Log("OVERLAY_CURSOR_RESTORE=WINDOW_DESTROY_ARROW");
            }
        }
        if (message == WM_NCDESTROY) {
            g_inputWindow = nullptr;
            g_originalWindowProc = nullptr;
        }
    }

    LRESULT CALLBACK OverlayWindowProc(HWND window, UINT message,
        WPARAM wParam, LPARAM lParam)
    {
        OverlayWindowDecision decision{};
        overlay::RunOptionalOverlayWork([&]() {
            decision = ProcessOverlayWindowMessage(window, message, wParam, lParam);
        }, &DisableAfterOverlayException);
        RestoreInputAfterTerminalFailure();
        if (decision.restoreGameCursor && g_originalWindowProc) {
            CallWindowProcW(g_originalWindowProc, window, WM_SETCURSOR,
                reinterpret_cast<WPARAM>(window), MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
        }
        if (decision.handled) {
            if (decision.callDefaultProcedure)
                return DefWindowProcW(window, message, wParam, lParam);
            return decision.result;
        }
        const auto original = g_originalWindowProc;
        const LRESULT result = original
            ? CallWindowProcW(original, window, message, wParam, lParam)
            : DefWindowProcW(window, message, wParam, lParam);
        overlay::RunOptionalOverlayWork([&]() {
            FinishOverlayWindowMessage(message);
        }, &DisableAfterOverlayException);
        RestoreInputAfterTerminalFailure();
        return result;
    }

    void InstallInputHook(HWND window)
    {
        if (!window || g_originalWindowProc || g_inputWindow) return;
        SetLastError(0);
        const auto previous = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
            window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&OverlayWindowProc)));
        if (!previous) {
            Log("OVERLAY_INPUT_HOOK_FAILED");
            return;
        }
        g_inputWindow = window;
        g_originalWindowProc = previous;
        plugin::RuntimeSettingsSnapshot settings{};
        const int key = plugin::GetRuntimeSettingsApi().Snapshot(settings)
            ? settings.overlayToggleKey : VK_DELETE;
        Log(std::string("OVERLAY_INPUT_HOOK_OK key=") + config::HotkeyName(key));
    }
#endif

    void TraceFactoryMethod(overlay::FactoryMethod method, HRESULT result,
        IDXGISwapChain* swapchain)
    {
        LogDiagnostic(std::string("FACTORY_METHOD_INVOKED method=") + FactoryMethodName(method) +
            " result=" + std::to_string(result) +
            " returnedSwapchain=" + Hex(swapchain));
    }

    void TraceSwapchainIdentity(IDXGISwapChain* swapchain, IUnknown* creationDevice,
        const char* source, HWND window, UINT width, UINT height, UINT buffers)
    {
        DXGI_SWAP_CHAIN_DESC desc{};
        if (FAILED(swapchain->GetDesc(&desc))) {
            Log(std::string("SWAPCHAIN_DESC_FAILED source=") + source);
            return;
        }

        ID3D12Device* device = nullptr;
        const HRESULT deviceResult = swapchain->GetDevice(
            __uuidof(ID3D12Device), reinterpret_cast<void**>(&device));

        ID3D12CommandQueue* queue = nullptr;
        D3D12_COMMAND_LIST_TYPE queueType = D3D12_COMMAND_LIST_TYPE_DIRECT;
        HRESULT queueResult = E_NOINTERFACE;
        if (creationDevice)
            queueResult = creationDevice->QueryInterface(
                __uuidof(ID3D12CommandQueue), reinterpret_cast<void**>(&queue));
        if (queue) {
            const auto queueDesc = queue->GetDesc();
            queueType = queueDesc.Type;
        }

        IDXGISwapChain3* swapchain3 = nullptr;
        UINT backBufferIndex = 0;
        const bool hasSwapchain3 = SUCCEEDED(swapchain->QueryInterface(
            __uuidof(IDXGISwapChain3), reinterpret_cast<void**>(&swapchain3)));
        if (hasSwapchain3 && swapchain3)
            backBufferIndex = swapchain3->GetCurrentBackBufferIndex();

        bool candidateAdded = false;
        std::size_t candidateCount = 0;
        if (queue && device) {
            std::scoped_lock lock{g_mutex};
            candidateAdded = g_evidence.ObserveCandidate(
                reinterpret_cast<std::uintptr_t>(swapchain),
                reinterpret_cast<std::uintptr_t>(device),
                reinterpret_cast<std::uintptr_t>(queue),
                static_cast<std::uint32_t>(queueType));
            candidateCount = g_evidence.CandidateCount(
                reinterpret_cast<std::uintptr_t>(swapchain));
        }

        Log(std::string("SWAPCHAIN_DISCOVERED source=") + source +
            " hwnd=" + Hex(window) +
            " width=" + std::to_string(width ? width : desc.BufferDesc.Width) +
            " height=" + std::to_string(height ? height : desc.BufferDesc.Height) +
            " buffers=" + std::to_string(buffers ? buffers : desc.BufferCount) +
            " queueType=" + QueueTypeName(queueType));
        LogDiagnostic(std::string("SWAPCHAIN_DETAILS swapchain=") + Hex(swapchain) +
            " creationObject=" + Hex(creationDevice) +
            " device=" + Hex(device) +
            " deviceHr=" + std::to_string(deviceResult) +
            " queue=" + Hex(queue) +
            " queueHr=" + std::to_string(queueResult) +
            " actualBuffers=" + std::to_string(desc.BufferCount) +
            " backBufferIndex=" + std::to_string(backBufferIndex) +
            " queueCandidateAdded=" + (candidateAdded ? "true" : "false") +
            " queueCandidateCount=" + std::to_string(candidateCount));

#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
        if (queue && device && queueType == D3D12_COMMAND_LIST_TYPE_DIRECT) {
            if (g_renderer.Initialize(swapchain, device, queue, window,
                desc.BufferCount, desc.BufferDesc.Format)) {
                g_renderer.SetVisible(overlay::GetInputState().visible());
                InstallInputHook(window);
            }
        }
#endif

        if (swapchain3) swapchain3->Release();
        if (queue) queue->Release();
        if (device) device->Release();
    }

    HRESULT STDMETHODCALLTYPE HookPresent(IDXGISwapChain* self, UINT syncInterval, UINT flags)
    {
        SwapchainHooks* hooks = nullptr;
        std::uint32_t sequence = 0;
        bool shouldLog = false;
        bool revalidated = false;
        overlay::AssociationState stateBefore = overlay::AssociationState::Unknown;
        overlay::AssociationState stateAfter = overlay::AssociationState::Unknown;
        std::uintptr_t device = 0;
        std::uintptr_t queue = 0;
        overlay::RunOptionalOverlayWork([&]() {
        {
            std::scoped_lock lock{g_mutex};
            hooks = FindSwapchain(self);
            if (hooks) {
                sequence = ++hooks->presentObservations;
                stateBefore = g_evidence.State(reinterpret_cast<std::uintptr_t>(self));
                g_evidence.ObservePresent(reinterpret_cast<std::uintptr_t>(self));
                shouldLog = diagnostics::Enabled() && g_presentWindow.Consume();
                stateAfter = g_evidence.State(reinterpret_cast<std::uintptr_t>(self));
                revalidated = stateBefore == overlay::AssociationState::ResizeRevalidationRequired &&
                    stateAfter == overlay::AssociationState::Supported;
                g_evidence.GetSingleCandidate(reinterpret_cast<std::uintptr_t>(self),
                    device, queue);
            }
        }
#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
        if (hooks && stateAfter == overlay::AssociationState::Supported)
            g_renderer.Render(self, stateAfter);
#endif
        if (hooks && shouldLog) {
            std::scoped_lock lock{g_mutex};
            IDXGISwapChain3* swapchain3 = nullptr;
            UINT index = 0;
            if (SUCCEEDED(self->QueryInterface(__uuidof(IDXGISwapChain3),
                reinterpret_cast<void**>(&swapchain3))) && swapchain3) {
                index = swapchain3->GetCurrentBackBufferIndex();
                swapchain3->Release();
            }
            if (sequence == 1)
                g_log << "PRESENT_FIRST swapchain=" << Hex(self);
            else
                g_log << "PRESENT_OBSERVED swapchain=" << Hex(self)
                      << " sequence=" << sequence;
            g_log << " backBufferIndex=" << index << " syncInterval=" << syncInterval
                  << " flags=" << flags
                  << " device=" << Hex(reinterpret_cast<void*>(device))
                  << " queue=" << Hex(reinterpret_cast<void*>(queue))
                  << " queueAssociation=" << AssociationStateName(stateAfter)
                  << " associationBeforePresent=" << AssociationStateName(stateBefore)
                  << " associationRevalidated=" << (revalidated ? "true" : "false")
                  << '\n';
            g_log.flush();
        }
        }, &DisableAfterOverlayException);
        RestoreInputAfterTerminalFailure();
        PresentFn original = hooks ? hooks->present.original<PresentFn>() : nullptr;
        if (!original) return E_FAIL;
        return original(self, syncInterval, flags);
    }

    HRESULT STDMETHODCALLTYPE HookResizeBuffers(IDXGISwapChain* self, UINT count,
        UINT width, UINT height, DXGI_FORMAT format, UINT flags)
    {
        SwapchainHooks* hooks = nullptr;
        {
            std::scoped_lock lock{g_mutex};
            hooks = FindSwapchain(self);
        }
        ResizeBuffersFn original = hooks ? hooks->resizeBuffers.original<ResizeBuffersFn>() : nullptr;
        const auto resizeHookStart = std::chrono::steady_clock::now();
        std::uint32_t resizeSequence = 0;
        overlay::AssociationState associationBefore = overlay::AssociationState::Unknown;
        std::uintptr_t device = 0;
        std::uintptr_t queue = 0;
        overlay::RunOptionalOverlayWork([&]() {
        if (hooks) {
#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
            g_renderer.BeforeResize();
#endif
            {
                std::scoped_lock lock{g_mutex};
                resizeSequence = ++hooks->resizeObservations;
                hooks->presentObservations = 0;
                associationBefore = g_evidence.State(
                    reinterpret_cast<std::uintptr_t>(self));
                g_evidence.BeginResize(reinterpret_cast<std::uintptr_t>(self));
                g_presentWindow.Arm(16);
                g_lifecycle.BeginResize();
                g_evidence.GetSingleCandidate(reinterpret_cast<std::uintptr_t>(self),
                    device, queue);
            }
            Log(std::string("OVERLAY_RESIZE_BEGIN sequence=") +
                std::to_string(resizeSequence) + " width=" + std::to_string(width) +
                " height=" + std::to_string(height) +
                " buffers=" + std::to_string(count) +
                " format=" + std::to_string(static_cast<unsigned>(format)) +
                " flags=" + std::to_string(flags));
            LogDiagnostic(std::string("OVERLAY_RESIZE_ASSOCIATION before=") +
                AssociationStateName(associationBefore) +
                " device=" + Hex(reinterpret_cast<void*>(device)) +
                " queue=" + Hex(reinterpret_cast<void*>(queue)));
        }
        }, &DisableAfterOverlayException);
        RestoreInputAfterTerminalFailure();
        if (!original) return E_FAIL;
        const auto originalStart = std::chrono::steady_clock::now();
        const HRESULT result = original(self, count, width, height, format, flags);
        const auto originalResizeMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - originalStart).count();
        const auto totalResizeHookMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - resizeHookStart).count();
        overlay::RunOptionalOverlayWork([&]() {
        if (hooks) {
            const bool ready = SUCCEEDED(result);
            {
                std::scoped_lock lock{g_mutex};
                g_evidence.CompleteResize(reinterpret_cast<std::uintptr_t>(self), ready);
            }
            if (ready) g_lifecycle.MarkReady();
            else g_lifecycle.CompleteResize(false);
#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
            g_renderer.OnResizeResult(ready, originalResizeMs, totalResizeHookMs);
#endif
            Log(std::string("OVERLAY_RESIZE_END result=") + std::to_string(result) +
                " success=" + (ready ? "true" : "false") +
                " originalResizeBuffersMs=" + std::to_string(originalResizeMs) +
                " resizeHookMs=" + std::to_string(totalResizeHookMs));
            LogDiagnostic(std::string("OVERLAY_RESIZE_STATE lifecycle=") +
                overlay::StateName(g_lifecycle.state()) + " association=" +
                AssociationStateName(g_evidence.State(reinterpret_cast<std::uintptr_t>(self))));
        }
        }, &DisableAfterOverlayException);
        RestoreInputAfterTerminalFailure();
        return result;
    }

    void InstallSwapchainHooks(IDXGISwapChain* swapchain, IUnknown* creationDevice,
        const char* source, HWND window, UINT width, UINT height, UINT buffers)
    {
        if (!swapchain || FindSwapchain(swapchain)) return;
        auto result = safetyhook::VmtHook::create(swapchain);
        if (!result) {
            Log(std::string("SWAPCHAIN_HOOK_FAILED swapchain=") + Hex(swapchain));
            return;
        }
        auto hooks = std::make_unique<SwapchainHooks>();
        hooks->object = swapchain;
        hooks->vmt = std::move(*result);
        auto present = hooks->vmt.hook_method(8, &HookPresent);
        auto resize = hooks->vmt.hook_method(13, &HookResizeBuffers);
        if (!present || !resize) {
            Log(std::string("SWAPCHAIN_METHOD_HOOK_FAILED swapchain=") + Hex(swapchain));
            return;
        }
        hooks->present = std::move(*present);
        hooks->resizeBuffers = std::move(*resize);
        hooks->vmt.apply(swapchain);
        {
            std::scoped_lock lock{g_mutex};
            g_evidence.BeginSwapchain(reinterpret_cast<std::uintptr_t>(swapchain));
            g_presentWindow.Arm(32);
        }
        g_swapchainHooks.push_back(std::move(hooks));
        g_lifecycle.MarkReady();
        TraceSwapchainIdentity(swapchain, creationDevice, source, window, width, height, buffers);
    }

    HRESULT STDMETHODCALLTYPE HookCreateSwapChain(IDXGIFactory* self, IUnknown* device,
        DXGI_SWAP_CHAIN_DESC* desc, IDXGISwapChain** outSwapchain)
    {
        auto* hooks = FindFactory(self);
        const auto original = hooks ? hooks->createSwapChain.original<CreateSwapChainFn>() : nullptr;
        if (!original) return E_FAIL;
        const HRESULT result = original(self, device, desc, outSwapchain);
        overlay::RunOptionalOverlayWork([&]() {
            TraceFactoryMethod(overlay::FactoryMethod::CreateSwapChain, result,
                (SUCCEEDED(result) && outSwapchain) ? *outSwapchain : nullptr);
            if (SUCCEEDED(result) && outSwapchain && *outSwapchain)
                InstallSwapchainHooks(*outSwapchain, device, "CreateSwapChain",
                    desc ? desc->OutputWindow : nullptr, desc ? desc->BufferDesc.Width : 0,
                    desc ? desc->BufferDesc.Height : 0, desc ? desc->BufferCount : 0);
        }, &DisableAfterOverlayException);
        RestoreInputAfterTerminalFailure();
        return result;
    }

    HRESULT STDMETHODCALLTYPE HookCreateSwapChainForHwnd(IDXGIFactory2* self, IUnknown* device,
        HWND window, const DXGI_SWAP_CHAIN_DESC1* desc,
        const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreen, IDXGIOutput* output,
        IDXGISwapChain1** outSwapchain)
    {
        auto* hooks = FindFactory(self);
        const auto original = hooks ? hooks->createSwapChainForHwnd.original<CreateSwapChainForHwndFn>() : nullptr;
        if (!original) return E_FAIL;
        const HRESULT result = original(self, device, window, desc, fullscreen, output, outSwapchain);
        overlay::RunOptionalOverlayWork([&]() {
            TraceFactoryMethod(overlay::FactoryMethod::CreateSwapChainForHwnd, result,
                (SUCCEEDED(result) && outSwapchain) ? *outSwapchain : nullptr);
            if (SUCCEEDED(result) && outSwapchain && *outSwapchain)
                InstallSwapchainHooks(*outSwapchain, device, "CreateSwapChainForHwnd", window,
                    desc ? desc->Width : 0, desc ? desc->Height : 0, desc ? desc->BufferCount : 0);
        }, &DisableAfterOverlayException);
        RestoreInputAfterTerminalFailure();
        return result;
    }

    HRESULT STDMETHODCALLTYPE HookCreateSwapChainForCoreWindow(IDXGIFactory2* self,
        IUnknown* device, const DXGI_SWAP_CHAIN_DESC1* desc, IUnknown* window,
        IDXGISwapChain1** outSwapchain)
    {
        auto* hooks = FindFactory(self);
        const auto original = hooks ? hooks->createSwapChainForCoreWindow.original<
            CreateSwapChainForCoreWindowFn>() : nullptr;
        if (!original) return E_FAIL;
        const HRESULT result = original(self, device, desc, window, outSwapchain);
        overlay::RunOptionalOverlayWork([&]() {
            TraceFactoryMethod(overlay::FactoryMethod::CreateSwapChainForCoreWindow, result,
                (SUCCEEDED(result) && outSwapchain) ? *outSwapchain : nullptr);
            if (SUCCEEDED(result) && outSwapchain && *outSwapchain)
                InstallSwapchainHooks(*outSwapchain, device, "CreateSwapChainForCoreWindow",
                    nullptr, desc ? desc->Width : 0, desc ? desc->Height : 0,
                    desc ? desc->BufferCount : 0);
        }, &DisableAfterOverlayException);
        RestoreInputAfterTerminalFailure();
        return result;
    }

    HRESULT STDMETHODCALLTYPE HookCreateSwapChainForComposition(IDXGIFactory2* self,
        IUnknown* device, const DXGI_SWAP_CHAIN_DESC1* desc, IDXGIOutput* output,
        IDXGISwapChain1** outSwapchain)
    {
        auto* hooks = FindFactory(self);
        using Function = HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory2*, IUnknown*,
            const DXGI_SWAP_CHAIN_DESC1*, IDXGIOutput*, IDXGISwapChain1**);
        const auto original = hooks ? hooks->createSwapChainForComposition.original<Function>() : nullptr;
        if (!original) return E_FAIL;
        const HRESULT result = original(self, device, desc, output, outSwapchain);
        overlay::RunOptionalOverlayWork([&]() {
            TraceFactoryMethod(overlay::FactoryMethod::CreateSwapChainForComposition, result,
                (SUCCEEDED(result) && outSwapchain) ? *outSwapchain : nullptr);
            if (SUCCEEDED(result) && outSwapchain && *outSwapchain)
                InstallSwapchainHooks(*outSwapchain, device, "CreateSwapChainForComposition",
                    nullptr, desc ? desc->Width : 0, desc ? desc->Height : 0,
                    desc ? desc->BufferCount : 0);
        }, &DisableAfterOverlayException);
        return result;
    }

    void InstallFactoryHooks(IUnknown* factory, bool factory2)
    {
        if (!factory || FindFactory(factory)) return;
        auto result = safetyhook::VmtHook::create(factory);
        if (!result) {
            Log(std::string("FACTORY_HOOK_FAILED factory=") + Hex(factory));
            return;
        }
        auto hooks = std::make_unique<FactoryHooks>();
        hooks->object = factory;
        hooks->vmt = std::move(*result);
        auto createSwapChain = hooks->vmt.hook_method(10, &HookCreateSwapChain);
        if (!createSwapChain) {
            Log(std::string("FACTORY_METHOD_HOOK_FAILED factory=") + Hex(factory));
            return;
        }
        hooks->createSwapChain = std::move(*createSwapChain);
        std::uint32_t methods = static_cast<std::uint32_t>(
            overlay::FactoryMethod::CreateSwapChain);
        if (factory2) {
            auto createSwapChainForHwnd = hooks->vmt.hook_method(15, &HookCreateSwapChainForHwnd);
            auto createSwapChainForCoreWindow = hooks->vmt.hook_method(16, &HookCreateSwapChainForCoreWindow);
            auto createSwapChainForComposition = hooks->vmt.hook_method(24, &HookCreateSwapChainForComposition);
            if (!createSwapChainForHwnd || !createSwapChainForCoreWindow ||
                !createSwapChainForComposition) {
                Log(std::string("FACTORY_METHOD_HOOK_FAILED factory=") + Hex(factory));
                return;
            }
            hooks->createSwapChainForHwnd = std::move(*createSwapChainForHwnd);
            hooks->createSwapChainForCoreWindow = std::move(*createSwapChainForCoreWindow);
            hooks->createSwapChainForComposition = std::move(*createSwapChainForComposition);
            methods |= static_cast<std::uint32_t>(overlay::FactoryMethod::CreateSwapChainForHwnd) |
                static_cast<std::uint32_t>(overlay::FactoryMethod::CreateSwapChainForCoreWindow) |
                static_cast<std::uint32_t>(overlay::FactoryMethod::CreateSwapChainForComposition);
        }
        hooks->vmt.apply(factory);
        g_factoryHooks.push_back(std::move(hooks));
        {
            std::scoped_lock lock{g_mutex};
            g_factoryEvidence.ObserveFactory(reinterpret_cast<std::uintptr_t>(factory));
            g_factoryEvidence.RecordMethod(reinterpret_cast<std::uintptr_t>(factory),
                overlay::FactoryMethod::CreateSwapChain);
            if (factory2) {
                g_factoryEvidence.RecordMethod(reinterpret_cast<std::uintptr_t>(factory),
                    overlay::FactoryMethod::CreateSwapChainForHwnd);
                g_factoryEvidence.RecordMethod(reinterpret_cast<std::uintptr_t>(factory),
                    overlay::FactoryMethod::CreateSwapChainForCoreWindow);
                g_factoryEvidence.RecordMethod(reinterpret_cast<std::uintptr_t>(factory),
                    overlay::FactoryMethod::CreateSwapChainForComposition);
            }
        }
        LogDiagnostic(std::string("FACTORY_HOOKED factory=") + Hex(factory) +
            " interface=" + (factory2 ? "IDXGIFactory2" : "IDXGIFactory") +
            " methods=" + std::to_string(methods));
    }

    void ObserveFactoryResult(HRESULT result, REFIID riid, void** output)
    {
        if (FAILED(result) || !output || !*output) return;
        const auto object = reinterpret_cast<std::uintptr_t>(*output);
        bool firstObservation = false;
        {
            std::scoped_lock lock{g_mutex};
            firstObservation = g_factoryEvidence.ObserveFactory(object);
        }
        if (firstObservation)
            LogDiagnostic(std::string("FACTORY_OBSERVED result=") + std::to_string(result) +
                " object=" + Hex(*output) + " interface=" + FactoryInterfaceName(riid));
        IDXGIFactory2* factory2 = nullptr;
        if (SUCCEEDED(reinterpret_cast<IUnknown*>(*output)->QueryInterface(
            __uuidof(IDXGIFactory2), reinterpret_cast<void**>(&factory2))) && factory2) {
            InstallFactoryHooks(factory2, true);
            factory2->Release();
        }
        InstallFactoryHooks(reinterpret_cast<IUnknown*>(*output), false);
    }

    HRESULT WINAPI HookCreateFactory(REFIID riid, void** output)
    {
        const auto original = g_createFactory.original<CreateFactoryFn>();
        const HRESULT result = original ? original(riid, output) : E_FAIL;
        overlay::RunOptionalOverlayWork([&]() { ObserveFactoryResult(result, riid, output); },
            &DisableAfterOverlayException);
        RestoreInputAfterTerminalFailure();
        return result;
    }

    HRESULT WINAPI HookCreateFactory1(REFIID riid, void** output)
    {
        const auto original = g_createFactory1.original<CreateFactoryFn>();
        const HRESULT result = original ? original(riid, output) : E_FAIL;
        overlay::RunOptionalOverlayWork([&]() { ObserveFactoryResult(result, riid, output); },
            &DisableAfterOverlayException);
        RestoreInputAfterTerminalFailure();
        return result;
    }

    HRESULT WINAPI HookCreateFactory2(UINT flags, REFIID riid, void** output)
    {
        using Function = HRESULT(WINAPI*)(UINT, REFIID, void**);
        const auto original = g_createFactory2.original<Function>();
        const HRESULT result = original ? original(flags, riid, output) : E_FAIL;
        overlay::RunOptionalOverlayWork([&]() { ObserveFactoryResult(result, riid, output); },
            &DisableAfterOverlayException);
        RestoreInputAfterTerminalFailure();
        return result;
    }

    bool InstallExportHook(const char* name, safetyhook::InlineHook& hook, void* destination)
    {
        const auto module = GetModuleHandleW(L"dxgi.dll");
        const auto address = module ? GetProcAddress(module, name) : nullptr;
        if (!address) {
        Log(std::string("EXPORT_NOT_FOUND name=") + name);
            return false;
        }
        auto result = safetyhook::InlineHook::create(address, destination);
        if (!result) {
            Log(std::string("EXPORT_HOOK_FAILED name=") + name);
            return false;
        }
        hook = std::move(*result);
        return true;
    }

    DWORD InitializeImpl()
    {
        WCHAR modulePath[MAX_PATH]{};
        GetModuleFileNameW(g_module, modulePath, MAX_PATH);
        std::wstring path(modulePath);
        path.replace(path.find_last_of(L"\\/"), std::wstring::npos,
            L"\\STALKER2CameraTweaksOverlay.log");
        {
            std::scoped_lock lock{g_mutex};
        g_log.open(path, std::ios::out | std::ios::trunc);
        }
#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
        g_renderer.SetLogger(&Log);
#endif
        Log("OVERLAY_START");
        g_lifecycle.BeginDiscovery();
        HWND window = GetForegroundWindow();
        Log(std::string("PROCESS_HWND hwnd=") + Hex(window));

        bool installed = false;
        std::string installedExports;
        const auto recordInstalled = [&installedExports](const char* name, bool success) {
            if (!success) return;
            if (!installedExports.empty()) installedExports += ',';
            installedExports += name;
        };
        const bool factoryInstalled = InstallExportHook("CreateDXGIFactory", g_createFactory,
            reinterpret_cast<void*>(&HookCreateFactory));
        recordInstalled("CreateDXGIFactory", factoryInstalled);
        installed |= factoryInstalled;
        const bool factory1Installed = InstallExportHook("CreateDXGIFactory1", g_createFactory1,
            reinterpret_cast<void*>(&HookCreateFactory1));
        recordInstalled("CreateDXGIFactory1", factory1Installed);
        installed |= factory1Installed;
        const bool factory2Installed = InstallExportHook("CreateDXGIFactory2", g_createFactory2,
            reinterpret_cast<void*>(&HookCreateFactory2));
        recordInstalled("CreateDXGIFactory2", factory2Installed);
        installed |= factory2Installed;
        if (!installed) {
            g_lifecycle.Fail();
            Log("OVERLAY_DISCOVERY_FAILED reason=no_DXGI_factory_export_hook");
        } else {
            Log("DXGI_HOOKS_INSTALLED exports=" + installedExports);
        }
        return 0;
    }

    DWORD WINAPI Initialize(void*) noexcept
    {
        overlay::RunOptionalOverlayWork([]() { InitializeImpl(); },
            &DisableAfterOverlayException);
        return 0;
    }
}

#ifdef OVERLAY_COMBINED
extern "C" void StartOverlayDiscovery(HMODULE module)
{
    g_module = module;
    DisableThreadLibraryCalls(module);
    const auto thread = CreateThread(nullptr, 0, Initialize, nullptr, 0, nullptr);
    if (thread) CloseHandle(thread);
}
#else
BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = module;
        DisableThreadLibraryCalls(module);
        const auto thread = CreateThread(nullptr, 0, Initialize, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    } else if (reason == DLL_PROCESS_DETACH) {
        g_stopping.store(true, std::memory_order_release);
    }
    return TRUE;
}
#endif
