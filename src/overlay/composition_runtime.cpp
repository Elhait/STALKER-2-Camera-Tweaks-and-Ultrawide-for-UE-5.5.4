#include "composition_runtime.hpp"

#include "game_language_reader.hpp"
#include "input_state.hpp"
#include "optional_overlay_boundary.hpp"
#include "presenter_runtime_policy.hpp"
#include "presentation_contracts.hpp"
#include "renderer_runtime.hpp"
#include "../diagnostics/diagnostic_runtime.hpp"
#include "../diagnostics/performance_telemetry.hpp"
#include "../diagnostics/startup_journal.hpp"
#include "../plugin/runtime.hpp"
#include "../config/feature_config.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <iterator>
#include <string>
#include <vector>

namespace
{
    constexpr UINT WakeMessage = WM_APP + 0x5A1;
    constexpr UINT RefreshMessage = WM_APP + 0x5A2;
    constexpr UINT FrameClockMessage = WM_APP + 0x5A3;
    constexpr UINT FrameClockUnavailableMessage = WM_APP + 0x5A4;
    constexpr UINT_PTR RenderTimerSeed = 0x5A3;
    constexpr UINT FallbackAnimationIntervalMs = 16;

    std::recursive_mutex g_rendererMutex;
    overlay::PresenterTimerIdentity g_renderTimer;
    overlay::PresenterWakeGate g_frameClockTickPending;
    HANDLE g_frameClockControl{};
    HANDLE g_frameClockStop{};
    HANDLE g_frameClockReady{};
    HANDLE g_frameClockThread{};
    std::atomic<bool> g_frameClockAvailable{};
    std::atomic<bool> g_frameClockDemand{};
    std::atomic<bool> g_presenterMinimized{};
    auto& g_renderer = *new overlay::Renderer;
    std::ofstream g_log;
    HMODULE g_module{};
    DWORD g_processId{};
    std::atomic<DWORD> g_uiThreadId{};
    std::atomic<bool> g_cameraCoreReady{};
    std::atomic<bool> g_workerStarted{};
    overlay::PresenterWakeGate g_redrawWakePending;
    std::atomic<bool> g_imguiPopupOpen{};
    std::atomic<bool> g_firstRawMouseCaptureLogged{};
    std::atomic<bool> g_firstAbsoluteMouseCaptureLogged{};
    HWINEVENTHOOK g_windowEvents{};
    HWINEVENTHOOK g_locationEvents{};
    std::atomic<HWND> g_inputWindow{};
    std::mutex g_logMutex;
    struct InputWindowBinding { HWND window{}; WNDPROC original{}; };
    std::mutex g_inputBindingsMutex;
    std::vector<InputWindowBinding> g_inputBindings;
    void Log(const std::string& message);

    using WaitForCompositorClockFunction = DWORD(WINAPI*)(UINT,
        const HANDLE*, DWORD);

    DWORD WINAPI FrameClockWorker(void*) noexcept
    {
        auto module = GetModuleHandleW(L"dcomp.dll");
        auto waitForClock = module ? reinterpret_cast<
            WaitForCompositorClockFunction>(GetProcAddress(module,
                "DCompositionWaitForCompositorClock")) : nullptr;
        g_frameClockAvailable.store(waitForClock != nullptr,
            std::memory_order_release);
        if (g_frameClockReady) SetEvent(g_frameClockReady);
        if (!waitForClock) return 0;

        const HANDLE clockEvents[]{g_frameClockControl, g_frameClockStop};
        while (true) {
            if (!g_frameClockDemand.load(std::memory_order_acquire)) {
                const DWORD wait = WaitForMultipleObjects(2, clockEvents,
                    FALSE, INFINITE);
                if (wait == WAIT_OBJECT_0 + 1) return 0;
                if (wait != WAIT_OBJECT_0) break;
                continue;
            }

            const DWORD wait = waitForClock(2, clockEvents, INFINITE);
            if (wait == WAIT_OBJECT_0 + 1) return 0;
            if (wait == WAIT_OBJECT_0) continue;
            if (wait == WAIT_OBJECT_0 + 2) {
                if (g_frameClockDemand.load(std::memory_order_acquire) &&
                    g_frameClockTickPending.TrySchedule() &&
                    !PostThreadMessageW(
                        g_uiThreadId.load(std::memory_order_acquire),
                        FrameClockMessage, 0, 0))
                    g_frameClockTickPending.CancelFailedPost();
                continue;
            }
            g_frameClockAvailable.store(false, std::memory_order_release);
            g_frameClockDemand.store(false, std::memory_order_release);
            PostThreadMessageW(g_uiThreadId.load(std::memory_order_acquire),
                FrameClockUnavailableMessage, wait, 0);
            return 0;
        }

        g_frameClockAvailable.store(false, std::memory_order_release);
        g_frameClockDemand.store(false, std::memory_order_release);
        PostThreadMessageW(g_uiThreadId.load(std::memory_order_acquire),
            FrameClockUnavailableMessage, GetLastError(), 0);
        return 0;
    }

    void SetFrameClockDemand(bool active) noexcept
    {
        if (!g_frameClockAvailable.load(std::memory_order_acquire) ||
            !g_frameClockControl)
            return;
        const bool previous = g_frameClockDemand.exchange(active,
            std::memory_order_acq_rel);
        if (previous != active) SetEvent(g_frameClockControl);
    }

    void StopFrameClock() noexcept
    {
        g_frameClockDemand.store(false, std::memory_order_release);
        if (g_frameClockStop) SetEvent(g_frameClockStop);
        if (g_frameClockControl) SetEvent(g_frameClockControl);
        if (g_frameClockThread) {
            WaitForSingleObject(g_frameClockThread, INFINITE);
            CloseHandle(g_frameClockThread);
            g_frameClockThread = nullptr;
        }
        if (g_frameClockReady) CloseHandle(g_frameClockReady);
        if (g_frameClockControl) CloseHandle(g_frameClockControl);
        if (g_frameClockStop) CloseHandle(g_frameClockStop);
        g_frameClockReady = nullptr;
        g_frameClockControl = nullptr;
        g_frameClockStop = nullptr;
        g_frameClockAvailable.store(false, std::memory_order_release);
    }

    void StartFrameClock() noexcept
    {
        g_frameClockControl = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        g_frameClockStop = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        g_frameClockReady = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!g_frameClockControl || !g_frameClockStop || !g_frameClockReady) {
            StopFrameClock();
            Log("OVERLAY_COMPOSITOR_CLOCK_UNAVAILABLE reason=event_creation");
            return;
        }
        g_frameClockThread = CreateThread(nullptr, 0, &FrameClockWorker,
            nullptr, 0, nullptr);
        if (!g_frameClockThread) {
            StopFrameClock();
            Log("OVERLAY_COMPOSITOR_CLOCK_UNAVAILABLE reason=thread_creation");
            return;
        }
        if (WaitForSingleObject(g_frameClockReady, 2000) != WAIT_OBJECT_0 ||
            !g_frameClockAvailable.load(std::memory_order_acquire)) {
            Log("OVERLAY_COMPOSITOR_CLOCK_UNAVAILABLE reason=api_missing_or_timeout");
        } else {
            Log("OVERLAY_COMPOSITOR_CLOCK_READY");
        }
    }

    WNDPROC OriginalWindowProc(HWND window) noexcept
    {
        std::scoped_lock lock{g_inputBindingsMutex};
        const auto found = std::find_if(g_inputBindings.begin(), g_inputBindings.end(),
            [window](const auto& binding) { return binding.window == window; });
        return found == g_inputBindings.end() ? nullptr : found->original;
    }

    void ForgetInputWindow(HWND window) noexcept
    {
        std::scoped_lock lock{g_inputBindingsMutex};
        std::erase_if(g_inputBindings,
            [window](const auto& binding) { return binding.window == window; });
    }

    void Log(const std::string& message)
    {
        std::scoped_lock lock{g_logMutex};
        if (g_log) {
            g_log << message << '\n';
            g_log.flush();
        }
    }

    void OpenLog() noexcept
    {
        wchar_t path[MAX_PATH]{};
        const DWORD length = GetModuleFileNameW(g_module, path,
            static_cast<DWORD>(std::size(path)));
        if (!length || length >= std::size(path)) return;
        std::wstring logPath(path, length);
        const auto separator = logPath.find_last_of(L"\\/");
        if (separator != std::wstring::npos) logPath.resize(separator + 1);
        logPath += L"STALKER2CameraTweaksOverlay.log";
        g_log.open(logPath, std::ios::out | std::ios::trunc);
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
            GetWindow(window, GW_OWNER) ||
            (GetWindowLongPtrW(window, GWL_STYLE) & WS_CHILD))
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
        PostThreadMessageW(g_uiThreadId.load(std::memory_order_acquire), RefreshMessage,
                reinterpret_cast<WPARAM>(window), static_cast<LPARAM>(event));
    }

    bool IsInputWindowMessage(UINT message) noexcept
    {
        return overlay::GetInputState().ShouldCapture(message);
    }

    bool IsKeyboardMessage(UINT message) noexcept
    {
        return message == WM_KEYDOWN || message == WM_KEYUP ||
            message == WM_SYSKEYDOWN || message == WM_SYSKEYUP ||
            message == WM_CHAR || message == WM_SYSCHAR;
    }

    bool QueueOverlayMouseInput(HWND window, UINT message,
        WPARAM wParam, LPARAM lParam) noexcept
    {
        auto& bridge = overlay::GetInputEventBridge();
        switch (message) {
        case WM_MOUSEMOVE: {
            POINT position{
                static_cast<short>(LOWORD(lParam)),
                static_cast<short>(HIWORD(lParam))};
            // The subclass receives client coordinates in the game's HWND
            // coordinate space. Convert explicitly for this HWND instead of
            // guessing a scale from GetDpiForWindow (which is 96 for unaware
            // windows and can differ from the physical monitor DPI).
            const bool physical = LogicalToPhysicalPointForPerMonitorDPI(
                window, &position) != FALSE;
            const bool queued = bridge.PushAbsoluteMousePosition(
                position.x, position.y);
            if (queued && !g_firstAbsoluteMouseCaptureLogged.exchange(true,
                    std::memory_order_acq_rel))
                Log(std::string("OVERLAY_INPUT_ABSOLUTE_MOUSE_CAPTURED source=WM_MOUSEMOVE coordinate_space=") +
                    (physical ? "physical_client" : "host_client_fallback"));
            if (queued) RequestOverlayRedraw();
            return true;
        }
        case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN: case WM_XBUTTONUP: case WM_XBUTTONDBLCLK:
            if (bridge.PushMouseButton(message, wParam)) RequestOverlayRedraw();
            return true;
        case WM_MOUSEWHEEL: case WM_MOUSEHWHEEL:
            if (bridge.PushMouseWheel(message, wParam, lParam))
                RequestOverlayRedraw();
            return true;
        case WM_INPUT: {
            RAWINPUT raw{};
            UINT bytes = sizeof(raw);
            const UINT copied = GetRawInputData(
                reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT,
                &raw, &bytes, sizeof(RAWINPUTHEADER));
            if (copied != sizeof(RAWINPUT) || raw.header.dwType != RIM_TYPEMOUSE)
                return false;
            if (!(raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) &&
                bridge.PushRelativeMouseMotion(raw.data.mouse.lLastX,
                    raw.data.mouse.lLastY)) {
                RequestOverlayRedraw();
                if (!g_firstRawMouseCaptureLogged.exchange(true,
                        std::memory_order_acq_rel))
                    Log("OVERLAY_INPUT_RAW_MOUSE_CAPTURED dx=" +
                        std::to_string(raw.data.mouse.lLastX) + " dy=" +
                        std::to_string(raw.data.mouse.lLastY));
            }
            (void)window;
            return true;
        }
        default:
            return false;
        }
    }

    void ActivateOverlayInput(HWND window) noexcept
    {
        if (!overlay::GetInputState().ShouldOwnInput()) return;
        auto& bridge = overlay::GetInputEventBridge();
        if (bridge.active()) return;
        POINT position{};
        if (!GetCursorPos(&position) || !ScreenToClient(window, &position))
            position = {};
        // This runs on the presenter's PMv2 owner thread, so ScreenToClient
        // yields the physical client-pixel coordinate used by the surface.
        bridge.Activate(position.x, position.y);
        RequestOverlayRedraw();
        Log("OVERLAY_INPUT_OWNERSHIP=OVERLAY_ACTIVE source=game_hwnd_bridge");
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
        return g_imguiPopupOpen.load(std::memory_order_acquire);
    }

    void RemoveInputHook() noexcept;

    void DisableAfterOverlayException()
    {
        std::scoped_lock lock{g_rendererMutex};
        g_renderer.Disable("composition_runtime_exception");
        overlay::GetInputEventBridge().Deactivate();
        RemoveInputHook();
        const DWORD threadId = g_uiThreadId.load(std::memory_order_acquire);
        if (threadId) PostThreadMessageW(threadId, WakeMessage, 0, 0);
    }

    OverlayWindowDecision ProcessOverlayWindowMessage(HWND window,
        UINT message, WPARAM wParam, LPARAM lParam)
    {
        OverlayWindowDecision decision{};
        auto& input = overlay::GetInputState();
        if (g_renderer.disabled() || window !=
                g_inputWindow.load(std::memory_order_acquire))
            return decision;
        if (message == overlay::AutoLanguageSyncMessage) {
            overlay::SynchronizeAutoLanguageIfConfigured();
            RequestOverlayRedraw();
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
            RequestOverlayRedraw();
            return {true, 0};
        }
        if (captureResult == overlay::RebindMessageResult::CapturedKey) {
            const int key = static_cast<int>(wParam);
            const auto action = input.rebindAction();
            plugin::MarkHotkeyRebindKeyConsumed(key);
            auto& api = plugin::GetRuntimeSettingsApi();
            const auto mutationForAction = [action, key]() {
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
            const auto mutation = mutationForAction();
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
                plugin::PublishOverlayNotification(
                    plugin::OverlayNotificationKind::BindingChanged,
                    notificationAction(), config::HotkeyName(input.previousBinding()),
                    config::HotkeyName(key), persisted
                        ? plugin::OverlayNotificationStatus::Applied
                        : plugin::OverlayNotificationStatus::Error);
                input.CommitAcceptedRebind(key);
                plugin::SetHotkeyRebindCaptureActive(false);
            } else {
                plugin::RuntimeSettingsSnapshot settings{};
                api.Snapshot(settings);
                const auto conflictAction = settings.overlayToggleKey == key
                    ? plugin::OverlayNotificationAction::OverlayToggle
                    : settings.gameplayCycleKey == key
                        ? plugin::OverlayNotificationAction::GameplayMode
                    : settings.cinematicCycleKey == key
                        ? plugin::OverlayNotificationAction::CinematicAspect
                    : settings.cinematicFovCycleKey == key
                        ? plugin::OverlayNotificationAction::CinematicFov
                    : settings.dialogueCycleKey == key
                        ? plugin::OverlayNotificationAction::DialogueZoom
                        : plugin::OverlayNotificationAction::AnotherSetting;
                plugin::PublishOverlayNotification(
                    plugin::OverlayNotificationKind::HotkeyConflict,
                    conflictAction, config::HotkeyName(key), {},
                    plugin::OverlayNotificationStatus::Error);
            }
            RequestOverlayRedraw();
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
                input.SetFocused(true);
                diagnostics::RequestRuntimePerformanceSummary();
                overlay::SynchronizeAutoLanguageOnOverlayOpen(input.visible());
                ActivateOverlayInput(window);
                SetCursor(nullptr);
            } else {
                overlay::GetInputEventBridge().Deactivate();
                RequestOverlayRedraw();
                if (OriginalWindowProc(window)) {
                    decision.restoreGameCursor = true;
                    Log("OVERLAY_CURSOR_RESTORE=GAME_WNDPROC");
                }
            }
            Log(std::string("OVERLAY_VISIBILITY=") +
                (input.visible() ? "VISIBLE" : "HIDDEN"));
            decision.handled = true;
            PostThreadMessageW(g_uiThreadId.load(std::memory_order_acquire),
                WakeMessage, 0, 0);
            return decision;
        }
        if (input.ShouldOwnInput()) {
            if (IsKeyboardMessage(message) &&
                overlay::GetInputEventBridge().PushKeyboardMessage(
                    message, wParam, lParam))
                RequestOverlayRedraw();
            if (input.ShouldConsumeEscapeFromGame(message, wParam))
                return {true, 0};
            if (message == WM_SETCURSOR && LOWORD(lParam) == HTCLIENT) {
                SetCursor(nullptr);
                return {true, TRUE};
            }
            if (message == WM_INPUT &&
                QueueOverlayMouseInput(window, message, wParam, lParam))
                return {true, 0, true};
            if (IsInputWindowMessage(message)) {
                QueueOverlayMouseInput(window, message, wParam, lParam);
                return {true, 0};
            }
        }
        if (message == WM_KILLFOCUS ||
            (message == WM_ACTIVATEAPP && wParam == FALSE)) {
            input.SetFocused(false);
            input.ResetEscapeDismissSequence();
            input.CancelRebind();
            plugin::SetHotkeyRebindCaptureActive(false);
            if (input.visible()) {
                overlay::GetInputEventBridge().Deactivate();
                RequestOverlayRedraw();
            }
        } else if ((message == WM_SETFOCUS ||
                (message == WM_ACTIVATEAPP && wParam != FALSE))) {
            input.SetFocused(true);
            if (input.visible()) ActivateOverlayInput(window);
        }
        return decision;
    }

    void FinishWindowMessage(HWND window, UINT message)
    {
        auto& input = overlay::GetInputState();
        if (message == WM_DESTROY || message == WM_NCDESTROY) {
            overlay::GetInputEventBridge().Deactivate();
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
            ForgetInputWindow(window);
            if (g_inputWindow.load(std::memory_order_acquire) == window)
                g_inputWindow.store(nullptr, std::memory_order_release);
            PostThreadMessageW(g_uiThreadId.load(std::memory_order_acquire),
                RefreshMessage, 0,
                EVENT_OBJECT_DESTROY);
        } else if (message == WM_SIZE || message == WM_MOVE ||
            message == WM_DISPLAYCHANGE || message == WM_DPICHANGED) {
            PostThreadMessageW(g_uiThreadId.load(std::memory_order_acquire), RefreshMessage,
                reinterpret_cast<WPARAM>(window),
                EVENT_OBJECT_LOCATIONCHANGE);
        }
    }

    LRESULT CALLBACK OverlayWindowProc(HWND window, UINT message,
        WPARAM wParam, LPARAM lParam)
    {
        OverlayWindowDecision decision{};
        WNDPROC original{};
        overlay::RunOptionalOverlayWork([&]() {
            std::scoped_lock lock{g_rendererMutex};
            original = OriginalWindowProc(window);
            decision = ProcessOverlayWindowMessage(window, message, wParam, lParam);
        }, &DisableAfterOverlayException);
        if (decision.restoreGameCursor && original)
            CallWindowProcW(original, window, WM_SETCURSOR,
                reinterpret_cast<WPARAM>(window), MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
        if (decision.handled) {
            if (decision.callDefaultProcedure)
                return DefWindowProcW(window, message, wParam, lParam);
            return decision.result;
        }
        const LRESULT result = original
            ? CallWindowProcW(original, window, message, wParam, lParam)
            : DefWindowProcW(window, message, wParam, lParam);
        overlay::RunOptionalOverlayWork([&]() {
            std::scoped_lock lock{g_rendererMutex};
            FinishWindowMessage(window, message);
        }, &DisableAfterOverlayException);
        return result;
    }

    bool InstallInputHook(HWND window)
    {
        if (!window || g_inputWindow.load(std::memory_order_acquire)) return false;
        std::scoped_lock bindingLock{g_inputBindingsMutex};
        const auto existing = std::find_if(g_inputBindings.begin(), g_inputBindings.end(),
            [window](const auto& binding) { return binding.window == window; });
        if (existing != g_inputBindings.end()) return false;
        g_inputBindings.push_back({window, nullptr});
        SetLastError(0);
        const auto previous = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
            window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&OverlayWindowProc)));
        if (!previous) {
            std::erase_if(g_inputBindings,
                [window](const auto& binding) { return binding.window == window; });
            Log("OVERLAY_INPUT_HOOK_FAILED");
            return false;
        }
        const auto binding = std::find_if(g_inputBindings.begin(), g_inputBindings.end(),
            [window](const auto& value) { return value.window == window; });
        if (binding == g_inputBindings.end()) return false;
        binding->original = previous;
        g_inputWindow.store(window, std::memory_order_release);
        plugin::RuntimeSettingsSnapshot settings{};
        const int key = plugin::GetRuntimeSettingsApi().Snapshot(settings)
            ? settings.overlayToggleKey : VK_DELETE;
        Log(std::string("OVERLAY_INPUT_HOOK_OK key=") + config::HotkeyName(key));
#if defined(OVERLAY_STARTUP_JOURNAL)
        diagnostics::startup_journal::MarkOnce(8, "INPUT_ACTIVATED",
            "window_proc_installed");
        diagnostics::startup_journal::Flush();
#endif
        return true;
    }

    void RemoveInputHook() noexcept
    {
        const HWND window = g_inputWindow.load(std::memory_order_acquire);
        if (!window) return;
        const WNDPROC original = OriginalWindowProc(window);
        if (!original) {
            g_inputWindow.store(nullptr, std::memory_order_release);
            return;
        }
        if (IsWindow(window) && reinterpret_cast<WNDPROC>(GetWindowLongPtrW(
                window, GWLP_WNDPROC)) == &OverlayWindowProc) {
            SetWindowLongPtrW(window, GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(original));
            ForgetInputWindow(window);
        } else if (IsWindow(window)) {
            Log("OVERLAY_INPUT_CHAIN_RETAINED foreign_subclass_after_us=1");
        } else {
            ForgetInputWindow(window);
        }
        g_inputWindow.store(nullptr, std::memory_order_release);
        overlay::GetInputEventBridge().Deactivate();
    }

    void UpdateRenderTimer()
    {
        std::scoped_lock lock{g_rendererMutex};
        const bool minimized = g_presenterMinimized.load(
            std::memory_order_acquire);
        const auto frameState = overlay::SelectPresenterFrameState(
            g_renderer.ready(), minimized,
            g_renderer.notificationAnimationActive(),
            g_renderer.needsRenderWork(), g_renderer.visible());
        if (overlay::NeedsCompositorClock(frameState) &&
            g_frameClockAvailable.load(std::memory_order_acquire)) {
            const auto timerId = g_renderTimer.TakeForCancellation();
            if (timerId) KillTimer(nullptr, static_cast<UINT_PTR>(timerId));
            SetFrameClockDemand(true);
            return;
        }
        SetFrameClockDemand(false);
        if (!overlay::NeedsFallbackAnimationTimer(frameState)) {
            const auto timerId = g_renderTimer.TakeForCancellation();
            if (timerId) KillTimer(nullptr, static_cast<UINT_PTR>(timerId));
            return;
        }
        const UINT interval = FallbackAnimationIntervalMs;
        if (!g_renderTimer.NeedsArm(interval)) return;
        const auto previousId = g_renderTimer.TakeForCancellation();
        if (previousId)
            KillTimer(nullptr, static_cast<UINT_PTR>(previousId));
        const auto returnedId = SetTimer(nullptr, RenderTimerSeed, interval, nullptr);
        if (!returnedId) {
            Log("OVERLAY_RENDER_TIMER_FAILED");
            g_renderer.Disable("presenter_timer_unavailable");
            overlay::GetInputEventBridge().Deactivate();
            RemoveInputHook();
            return;
        }
        g_renderTimer.Arm(returnedId, interval);
        Log("OVERLAY_RENDER_TIMER_ARMED id=" +
            std::to_string(static_cast<std::uint64_t>(returnedId)) +
            " interval_ms=" + std::to_string(interval));
    }

    void ReconcileTerminalInputOwnership() noexcept
    {
        if (!g_renderer.disabled()) return;
        overlay::GetInputEventBridge().Deactivate();
        RemoveInputHook();
    }

    void RefreshPresenter(HWND observedWindow, DWORD event)
    {
        (void)event;
        if (!g_cameraCoreReady.load(std::memory_order_acquire)) return;
        const HWND candidate = FindGameWindow();
        if (!candidate) {
            const HWND activeWindow = g_inputWindow.load(std::memory_order_acquire);
            if (activeWindow && !IsWindow(activeWindow)) RemoveInputHook();
            return;
        }
        RECT client{};
        if (!GetClientRect(candidate, &client)) return;
        const UINT width = static_cast<UINT>(std::max<LONG>(0,
            client.right - client.left));
        const UINT height = static_cast<UINT>(std::max<LONG>(0,
            client.bottom - client.top));
        const UINT dpi = GetDpiForWindow(candidate);
        const bool minimized = IsIconic(candidate) || !width || !height;
        g_presenterMinimized.store(minimized, std::memory_order_release);

        std::scoped_lock lock{g_rendererMutex};
        if (g_renderer.disabled()) {
            overlay::GetInputEventBridge().Deactivate();
            RemoveInputHook();
            return;
        }
        const auto route = overlay::SelectPresenterRoute(
            g_renderer.hasSurfaceGeneration(), g_renderer.OwnsWindow(candidate),
            minimized, observedWindow == candidate);
        if (route == overlay::PresenterRoute::WaitForWindow) {
            UpdateRenderTimer();
            return;
        }
        if (route == overlay::PresenterRoute::Initialize) {
#if defined(OVERLAY_STARTUP_JOURNAL)
            diagnostics::startup_journal::MarkOnce(4, "GAME_WINDOW_SELECTED",
                "visible_process_top_level_window",
                static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(candidate)));
            diagnostics::startup_journal::Flush();
#endif
            if (g_renderer.Initialize(candidate, width, height, dpi)) {
                if (InstallInputHook(candidate))
                    Log("OVERLAY_PRESENTER_ATTACHED hwnd=" +
                        std::to_string(reinterpret_cast<std::uintptr_t>(candidate)));
                else
                    Log("OVERLAY_INPUT_ATTACH_UNAVAILABLE");
            }
        } else if (route == overlay::PresenterRoute::RebindWindow) {
            RemoveInputHook();
            if (g_renderer.RebindWindow(candidate, width, height, dpi) &&
                InstallInputHook(candidate))
                Log("OVERLAY_PRESENTER_REBOUND hwnd=" +
                    std::to_string(reinterpret_cast<std::uintptr_t>(candidate)));
        } else if (route == overlay::PresenterRoute::UpdateGeometry) {
            if (minimized) {
                g_renderer.OnWindowGeometry(candidate, width, height, true, dpi);
            } else if (g_renderer.OnWindowGeometry(candidate, width, height, false, dpi)) {
                Log("OVERLAY_WINDOW_GEOMETRY_UPDATED width=" +
                    std::to_string(width) + " height=" + std::to_string(height));
            }
        }
        ReconcileTerminalInputOwnership();
        ReconcileTerminalInputOwnership();
        UpdateRenderTimer();
    }

    DWORD WINAPI Worker(void*) noexcept
    {
        const DPI_AWARENESS_CONTEXT previousDpiContext =
            SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        g_uiThreadId = GetCurrentThreadId();
        g_processId = GetCurrentProcessId();
        MSG message{};
        PeekMessageW(&message, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
        OpenLog();
        g_renderer.SetLogger(&Log);
        Log(previousDpiContext ? "OVERLAY_DPI_THREAD_CONTEXT=PER_MONITOR_V2"
            : "OVERLAY_DPI_THREAD_CONTEXT=UNCHANGED_API_UNAVAILABLE");
        Log("OVERLAY_DCOMP_RUNTIME v=1 dxgi_factory_hooks=0 swapchain_hooks=0");
        StartFrameClock();
#if defined(OVERLAY_STARTUP_JOURNAL)
        diagnostics::startup_journal::MarkOnce(1, "OVERLAY_WORKER_START",
            "composition_owner_thread");
        diagnostics::startup_journal::Flush();
#endif
        g_windowEvents = SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_SHOW,
            nullptr, &OnWindowEvent, g_processId, 0, WINEVENT_OUTOFCONTEXT);
        g_locationEvents = SetWinEventHook(EVENT_OBJECT_LOCATIONCHANGE,
            EVENT_OBJECT_LOCATIONCHANGE, nullptr, &OnWindowEvent,
            g_processId, 0, WINEVENT_OUTOFCONTEXT);
        Log(g_windowEvents && g_locationEvents ? "OVERLAY_WINDOW_EVENTS_ARMED"
            : "OVERLAY_WINDOW_EVENTS_PARTIAL");
        if (g_cameraCoreReady.load(std::memory_order_acquire))
            RefreshPresenter(nullptr, 0);

        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            if (message.message == RefreshMessage) {
                overlay::RunOptionalOverlayWork([&]() {
                    RefreshPresenter(reinterpret_cast<HWND>(message.wParam),
                        static_cast<DWORD>(message.lParam));
                }, &DisableAfterOverlayException);
            } else if (message.message == WakeMessage) {
                g_redrawWakePending.BeginHandling();
                overlay::RunOptionalOverlayWork([&]() {
                    HWND candidate = FindGameWindow();
                    bool discoveryNeeded{};
                    {
                        std::scoped_lock lock{g_rendererMutex};
                        g_renderer.RequestRedraw();
                        discoveryNeeded = !g_renderer.hasSurfaceGeneration() ||
                            (candidate && !g_renderer.OwnsWindow(candidate));
                        if (!discoveryNeeded &&
                            !g_frameClockAvailable.load(std::memory_order_acquire) &&
                            !g_presenterMinimized.load(std::memory_order_acquire))
                            g_renderer.Render();
                        if (!discoveryNeeded) UpdateRenderTimer();
                    }
                    if (discoveryNeeded)
                        RefreshPresenter(candidate, EVENT_OBJECT_SHOW);
                }, &DisableAfterOverlayException);
            } else if (message.message == FrameClockMessage) {
                g_frameClockTickPending.BeginHandling();
                overlay::RunOptionalOverlayWork([&]() {
                    std::scoped_lock lock{g_rendererMutex};
                    if (g_renderer.ready() &&
                        !g_presenterMinimized.load(std::memory_order_acquire) &&
                        g_renderer.needsRenderWork())
                        g_renderer.Render();
                    ReconcileTerminalInputOwnership();
                    UpdateRenderTimer();
                }, &DisableAfterOverlayException);
            } else if (message.message == FrameClockUnavailableMessage) {
                Log("OVERLAY_COMPOSITOR_CLOCK_LOST status=" +
                    std::to_string(static_cast<std::uint32_t>(message.wParam)) +
                    " fallback=event_redraw_and_animation_timer");
                overlay::RunOptionalOverlayWork([&]() {
                    std::scoped_lock lock{g_rendererMutex};
                    if (!g_presenterMinimized.load(std::memory_order_acquire) &&
                        g_renderer.needsRenderWork())
                        g_renderer.Render();
                    UpdateRenderTimer();
                }, &DisableAfterOverlayException);
            } else if (message.message == WM_TIMER &&
                g_renderTimer.Matches(message.wParam)) {
                overlay::RunOptionalOverlayWork([&]() {
                    std::scoped_lock lock{g_rendererMutex};
                    const HWND activeWindow =
                        g_inputWindow.load(std::memory_order_acquire);
                    if (g_renderer.ready() && (!activeWindow || !IsIconic(activeWindow)) &&
                        g_renderer.needsRenderWork())
                        g_renderer.Render();
                    if (g_renderer.disabled()) {
                        ReconcileTerminalInputOwnership();
                    }
                    UpdateRenderTimer();
                }, &DisableAfterOverlayException);
            }
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        if (g_windowEvents) UnhookWinEvent(g_windowEvents);
        if (g_locationEvents) UnhookWinEvent(g_locationEvents);
        const auto timerId = g_renderTimer.TakeForCancellation();
        if (timerId) KillTimer(nullptr, static_cast<UINT_PTR>(timerId));
        StopFrameClock();
        RemoveInputHook();
        overlay::GetInputEventBridge().Deactivate();
        if (g_log) g_log.close();
        if (previousDpiContext) SetThreadDpiAwarenessContext(previousDpiContext);
        return 0;
    }
}

extern "C" void PublishOverlayPopupState(bool open) noexcept
{
    g_imguiPopupOpen.store(open, std::memory_order_release);
}

extern "C" bool StartOverlayDiscovery(HMODULE module)
{
    g_module = module;
    DisableThreadLibraryCalls(module);
    bool expected = false;
    if (!g_workerStarted.compare_exchange_strong(expected, true,
            std::memory_order_acq_rel))
        return true;
    const HANDLE thread = CreateThread(nullptr, 0, &Worker, nullptr, 0, nullptr);
    if (!thread) {
        g_workerStarted.store(false, std::memory_order_release);
        return false;
    }
    CloseHandle(thread);
    return true;
}

extern "C" void NotifyOverlayCameraCoreReady()
{
    g_cameraCoreReady.store(true, std::memory_order_release);
#if defined(OVERLAY_STARTUP_JOURNAL)
    diagnostics::startup_journal::MarkOnce(3, "CAMERA_CORE_READY",
        "runtime_initialized");
    diagnostics::startup_journal::Flush();
#endif
    RequestOverlayRedraw();
}

extern "C" void RequestOverlayRedraw()
{
    const DWORD threadId = g_uiThreadId.load(std::memory_order_acquire);
    if (!threadId) return;
    if (!g_redrawWakePending.TrySchedule()) return;
    if (!PostThreadMessageW(threadId, WakeMessage, 0, 0))
        g_redrawWakePending.CancelFailedPost();
}
