#include "composition_runtime.hpp"

#include "../localization/game_language_reader.hpp"
#include "../input/input_state.hpp"
#include "../input/game_thread_subclass.hpp"
#include "../optional_overlay_boundary.hpp"
#include "../presenter/presenter_runtime_policy.hpp"
#include "../presentation/presentation_contracts.hpp"
#include "../presenter/renderer_runtime.hpp"
#include "owner_finalizer.hpp"
#include "stable_record_pool.hpp"
#include "../../diagnostics/diagnostic_runtime.hpp"
#include "../../diagnostics/performance_telemetry.hpp"
#include "../../diagnostics/startup_journal.hpp"
#include "../../plugin/runtime.hpp"
#include "../../config/feature_config.hpp"
#include "../../platform/win32/window.hpp"
#include "../../platform/win32/native_log_file.hpp"
#include "../../platform/process_lifetime.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <iterator>
#include <limits>
#include <new>
#include <string>
#include <vector>

namespace
{
    constexpr UINT WakeMessage = WM_APP + 0x5A1;
    constexpr UINT FrameClockMessage = WM_APP + 0x5A3;
    constexpr UINT FrameClockUnavailableMessage = WM_APP + 0x5A4;
    constexpr UINT InstallInputBindingMessage = WM_APP + 0x5A5;
    constexpr UINT_PTR RenderTimerSeed = 0x5A3;
    constexpr UINT FallbackAnimationIntervalMs = 16;
    constexpr DWORD OwnerHeartbeatIntervalMs = 250;
    constexpr std::uint64_t OwnerHeartbeatMaximumAgeMs = 750;
    constexpr auto OverlayDisplayClassName =
        platform::win32::OverlayDisplayWindowClassName;

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
    std::atomic<overlay::Renderer*> g_renderer{};
    overlay::Renderer& RendererInstance() noexcept
    {
        return *g_renderer.load(std::memory_order_acquire);
    }
    struct OverlayLogState
    {
        std::mutex mutex;
        platform::win32::NativeLogFile file;
    };
    platform::ProcessLifetime<OverlayLogState> g_logStorage;
    auto& g_log = g_logStorage.get().file;
    auto& g_logMutex = g_logStorage.get().mutex;
    HMODULE g_module{};
    DWORD g_processId{};
    std::atomic<DWORD> g_uiThreadId{};
    std::atomic<bool> g_messageQueueReady{};
    std::atomic<bool> g_cameraCoreReady{};
    std::atomic<bool> g_workerStarted{};
    overlay::PresenterRefreshSignal g_presenterRefreshPending;
    std::atomic<std::uint32_t> g_windowScanLogBudget{8};
    std::atomic<std::uint32_t> g_refreshRequestLogBudget{16};
    std::atomic<std::uint32_t> g_refreshDispatchLogBudget{16};
    std::atomic<std::uint32_t> g_refreshStateLogBudget{32};
    std::atomic<std::uint32_t> g_ownerPostSuccessLogBudget{8};
    std::atomic<std::uint32_t> g_ownerPostFailureLogBudget{32};
    const char* g_lastRefreshTransition{}; // owner-thread only
    std::atomic<HWND> g_overlayDisplayWindow{};
    bool g_overlayDisplayClassRegistered{};
    std::atomic<bool> g_ownerFinalizing{};
    overlay::OwnerFinalizer g_ownerFinalizer;
    std::atomic<bool> g_overlayInputReady{};
    overlay::InputOwnerHeartbeat g_ownerHeartbeat;
    overlay::PresenterWakeGate g_redrawWakePending;
    std::atomic<bool> g_imguiPopupOpen{};
    HWINEVENTHOOK g_windowEvents{};
    HWINEVENTHOOK g_locationEvents{};
    std::atomic<HWND> g_inputWindow{};
    struct InputWindowBinding
    {
        HWND window{};
        std::atomic<WNDPROC> original{};
        std::uint64_t generation{};
        enum class InstallState { Pending, Installing, Installed, Failed };
        std::atomic<InstallState> installState{InstallState::Pending};
    };
    constexpr std::size_t InputWindowBindingCapacity = 64;
    overlay::StableRecordPool<InputWindowBinding, InputWindowBindingCapacity>
        g_inputWindowBindings;
    bool g_inputBindingLimitReported{};
    constexpr wchar_t InputBindingProperty[] =
        L"STALKER2CameraTweaks.InputWindowBinding";
    std::atomic<InputWindowBinding*> g_currentInputBinding{};
    std::atomic<InputWindowBinding*> g_pendingInputBinding{};
    HHOOK g_bindingInstallHook{}; // owner-thread handle; callback uses CallNextHookEx(nullptr)
    std::atomic<std::uint64_t> g_nextInputBindingGeneration{1};
    overlay::InputCallbackAdmissionGate g_inputCallbackAdmission;
    HANDLE g_terminalWakeEvent{};
    HANDLE g_callbackDrainEvent{};
    HANDLE g_ownerCommandWakeEvent{};
    overlay::OwnerInputCommandQueue g_ownerInputCommands;
    std::atomic<int> g_overlayToggleKey{VK_DELETE};
    std::atomic<const char*> g_terminalReason{};
    void Log(const std::string& message) noexcept;
    void Log(const char* message) noexcept;

    bool ConsumeDiagnosticBudget(std::atomic<std::uint32_t>& budget) noexcept
    {
        auto remaining = budget.load(std::memory_order_relaxed);
        while (remaining) {
            if (budget.compare_exchange_weak(remaining, remaining - 1,
                    std::memory_order_relaxed, std::memory_order_relaxed))
                return true;
        }
        return false;
    }

    void LogDiscoveryDiagnostic(const std::string& message) noexcept
    {
        if (!diagnostics::Enabled()) return;
        Log(message);
    }

    void LogDiscoveryDiagnostic(const char* message) noexcept
    {
        if (!diagnostics::Enabled()) return;
        Log(message);
    }

    void LogRefreshTransition(const char* state, HWND window = nullptr,
        UINT width = 0, UINT height = 0) noexcept
    {
        if (!diagnostics::Enabled() || g_lastRefreshTransition == state ||
            !ConsumeDiagnosticBudget(g_refreshStateLogBudget)) return;
        g_lastRefreshTransition = state;
        try {
            Log("OVERLAY_REFRESH_STATE state=" + std::string(state) +
                " hwnd=" + std::to_string(reinterpret_cast<std::uintptr_t>(window)) +
                " client=" + std::to_string(width) + "x" +
                std::to_string(height) + " queue_ready=" +
                std::to_string(g_messageQueueReady.load(std::memory_order_acquire)) +
                " refresh_pending=" + std::to_string(
                    g_presenterRefreshPending.pending()));
        } catch (...) {}
    }

    void RequestPresenterRefresh(HWND observedWindow, DWORD event) noexcept
    {
        g_presenterRefreshPending.Request();
        const HANDLE wakeEvent = g_ownerCommandWakeEvent;
        const BOOL signaled = wakeEvent ? SetEvent(wakeEvent) : FALSE;
        const DWORD signalError = signaled ? ERROR_SUCCESS
            : wakeEvent ? GetLastError() : ERROR_INVALID_HANDLE;
        if (diagnostics::Enabled() &&
            ConsumeDiagnosticBudget(g_refreshRequestLogBudget)) {
            try {
                LogDiscoveryDiagnostic("OVERLAY_REFRESH_REQUEST source=window_event hwnd=" +
                    std::to_string(reinterpret_cast<std::uintptr_t>(observedWindow)) +
                    " event=" + std::to_string(event) + " queue_ready=" +
                    std::to_string(g_messageQueueReady.load(std::memory_order_acquire)) +
                    " pending=1 owner_event_signaled=" + std::to_string(signaled != FALSE) +
                    " win32_error=" + std::to_string(signalError));
            } catch (...) {}
        }
        if (!signaled) {
            const DWORD threadId = g_uiThreadId.load(std::memory_order_acquire);
            if (threadId && g_messageQueueReady.load(std::memory_order_acquire)) {
                SetLastError(ERROR_SUCCESS);
                const BOOL posted = PostThreadMessageW(threadId, WakeMessage, 0, 0);
                const DWORD postError = posted ? ERROR_SUCCESS : GetLastError();
                auto& budget = posted ? g_ownerPostSuccessLogBudget
                                      : g_ownerPostFailureLogBudget;
                if (diagnostics::Enabled() && ConsumeDiagnosticBudget(budget)) {
                    try {
                        LogDiscoveryDiagnostic("OVERLAY_REFRESH_FALLBACK_POST result=" +
                            std::string(posted ? "ok" : "failed") + " thread=" +
                            std::to_string(threadId) + " win32_error=" +
                            std::to_string(postError));
                    } catch (...) {}
                }
            }
        }
    }

    void RequestOverlayTerminal() noexcept
    {
        g_overlayInputReady.store(false, std::memory_order_release);
        g_ownerHeartbeat.Reset();
        overlay::CloseInputAdmission(g_inputCallbackAdmission,
            overlay::GetInputEventBridge());
        if (g_callbackDrainEvent &&
            g_inputCallbackAdmission.inFlight() == 0)
            SetEvent(g_callbackDrainEvent);
        if (g_terminalWakeEvent) SetEvent(g_terminalWakeEvent);
        const DWORD threadId = g_uiThreadId.load(std::memory_order_acquire);
        if (threadId) PostThreadMessageW(threadId, WakeMessage, 0, 0);
    }

    void RequestOverlayTerminalWithReason(const char* staticReason) noexcept
    {
        const char* expected{};
        g_terminalReason.compare_exchange_strong(expected, staticReason,
            std::memory_order_release, std::memory_order_relaxed);
        RequestOverlayTerminal();
    }

    void FinishWindowMessage(HWND window, UINT message);

    bool EnqueueOwnerInputCommand(InputWindowBinding* binding, HWND window,
        UINT message, WPARAM wParam, LPARAM lParam,
        bool finishAfterContinuation = false,
        bool rebindModifierDown = false) noexcept;

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

    bool StopFrameClock() noexcept
    {
        g_frameClockDemand.store(false, std::memory_order_release);
        if (g_frameClockStop) SetEvent(g_frameClockStop);
        if (g_frameClockControl) SetEvent(g_frameClockControl);
        if (g_frameClockThread) {
            const DWORD wait = WaitForSingleObject(g_frameClockThread, INFINITE);
            if (wait != WAIT_OBJECT_0) return false;
            if (!CloseHandle(g_frameClockThread)) return false;
            g_frameClockThread = nullptr;
        }
        if (g_frameClockReady && CloseHandle(g_frameClockReady))
            g_frameClockReady = nullptr;
        if (g_frameClockControl && CloseHandle(g_frameClockControl))
            g_frameClockControl = nullptr;
        if (g_frameClockStop && CloseHandle(g_frameClockStop))
            g_frameClockStop = nullptr;
        g_frameClockAvailable.store(false, std::memory_order_release);
        return !g_frameClockThread && !g_frameClockReady &&
            !g_frameClockControl && !g_frameClockStop;
    }

    void StartFrameClock() noexcept
    {
        try {
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
        } catch (...) {
            // Frame-clock setup is optional; a logging/allocation failure must
            // not escape this noexcept worker helper.
            StopFrameClock();
        }
    }

    WNDPROC OriginalWindowProc(HWND window) noexcept
    {
        auto* binding = static_cast<InputWindowBinding*>(
            GetPropW(window, InputBindingProperty));
        return binding ? binding->original.load(std::memory_order_acquire)
            : nullptr;
    }

    void ForgetInputWindow(HWND window) noexcept
    {
        auto* binding = static_cast<InputWindowBinding*>(
            GetPropW(window, InputBindingProperty));
        if (binding) RemovePropW(window, InputBindingProperty);
    }

    void WriteLog(const std::string& message)
    {
        std::scoped_lock lock{g_logMutex};
        if (g_log.isOpen()) {
            g_log.Write(message + "\r\n");
        }
    }

    void Log(const std::string& message) noexcept
    {
        overlay::TryOptionalOverlayLog(&WriteLog, message);
    }

    void Log(const char* message) noexcept
    {
        overlay::TryOptionalOverlayLog(&WriteLog, message);
    }

    void CloseLog() noexcept
    {
        try {
            std::scoped_lock lock{g_logMutex};
            g_log.Close();
        } catch (...) {}
    }

    void OpenLog() noexcept
    {
        try {
        wchar_t path[MAX_PATH]{};
        const DWORD length = GetModuleFileNameW(g_module, path,
            static_cast<DWORD>(std::size(path)));
        if (!length || length >= std::size(path)) return;
        std::wstring logPath(path, length);
        const auto separator = logPath.find_last_of(L"\\/");
        if (separator != std::wstring::npos) logPath.resize(separator + 1);
        logPath += L"STALKER2CameraTweaksOverlay.log";
        std::scoped_lock lock{g_logMutex};
        g_log.Open(logPath.c_str(), true);
        } catch (...) {
            CloseLog();
        }
    }

    LRESULT CALLBACK OverlayDisplayWindowProc(HWND window, UINT message,
        WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_MOUSEACTIVATE) return MA_NOACTIVATE;
        if (message == WM_CLOSE) {
            RequestOverlayTerminalWithReason("display_close");
            return 0;
        }
        if (message == WM_NCDESTROY) {
            if (g_overlayDisplayWindow.load(std::memory_order_acquire) == window)
                g_overlayDisplayWindow.store(nullptr, std::memory_order_release);
            if (!g_ownerFinalizing.load(std::memory_order_acquire))
                RequestOverlayTerminal();
        }
        return DefWindowProcW(window, message, wParam, lParam);
    }

    bool CreateOverlayDisplayWindow() noexcept
    {
        if (!g_module || g_overlayDisplayWindow.load(std::memory_order_acquire))
            return false;
        WNDCLASSEXW windowClass{sizeof(windowClass)};
        if (!GetClassInfoExW(g_module, OverlayDisplayClassName, &windowClass)) {
            windowClass = {};
            windowClass.cbSize = sizeof(windowClass);
            windowClass.style = CS_HREDRAW | CS_VREDRAW;
            windowClass.lpfnWndProc = &OverlayDisplayWindowProc;
            windowClass.hInstance = g_module;
            windowClass.lpszClassName = OverlayDisplayClassName;
            if (!RegisterClassExW(&windowClass)) return false;
            g_overlayDisplayClassRegistered = true;
        } else if (windowClass.lpfnWndProc != &OverlayDisplayWindowProc) {
            return false;
        }
        const HWND window = CreateWindowExW(WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW |
            WS_EX_LAYERED | WS_EX_TRANSPARENT, OverlayDisplayClassName,
            L"S.T.A.L.K.E.R. 2 Camera Tweaks Overlay", WS_POPUP,
            0, 0, 1, 1, nullptr, nullptr, g_module, nullptr);
        if (!window) return false;
        // Match the unowned-HWND pixel fixture: DComp supplies the content,
        // while LAYERED/TRANSPARENT preserves cross-thread mouse pass-through.
        g_overlayDisplayWindow.store(window, std::memory_order_release);
        return true;
    }

    bool DestroyOverlayDisplayWindow() noexcept
    {
        const HWND window = g_overlayDisplayWindow.load(std::memory_order_acquire);
        if (window) {
            if (GetWindowThreadProcessId(window, nullptr) != GetCurrentThreadId()) {
                try { Log("OVERLAY_DISPLAY_WINDOW_DESTROY_REJECTED wrong_thread=1"); }
                catch (...) {}
                return false;
            }
            ShowWindow(window, SW_HIDE);
            if (!DestroyWindow(window)) {
                try {
                    Log("OVERLAY_DISPLAY_WINDOW_DESTROY_FAILED error=" +
                        std::to_string(GetLastError()));
                } catch (...) {}
                return false;
            }
            g_overlayDisplayWindow.store(nullptr, std::memory_order_release);
            try { Log("OVERLAY_DISPLAY_WINDOW_DESTROYED owner_thread=1"); }
            catch (...) {}
        }
        if (g_overlayDisplayClassRegistered) {
            if (!UnregisterClassW(OverlayDisplayClassName, g_module)) {
                try {
                    Log("OVERLAY_DISPLAY_CLASS_UNREGISTER_FAILED error=" +
                        std::to_string(GetLastError()));
                } catch (...) {}
                return false;
            }
            g_overlayDisplayClassRegistered = false;
            try { Log("OVERLAY_DISPLAY_CLASS_UNREGISTERED owner_thread=1"); }
            catch (...) {}
        }
        return true;
    }

    struct WindowCandidate
    {
        HWND window{};
        std::uint64_t area{};
        UINT width{};
        UINT height{};
    };

    struct WindowScanSummary
    {
        WindowCandidate best{};
        std::uint32_t processWindows{};
        std::uint32_t overlayWindows{};
        std::uint32_t invisible{};
        std::uint32_t owned{};
        std::uint32_t child{};
        std::uint32_t clientRectFailed{};
        std::uint32_t undersized{};
        std::uint32_t eligible{};
        HWND sampleInvisible{};
        HWND sampleOwned{};
        HWND sampleChild{};
        HWND sampleClientRectFailed{};
        HWND sampleUndersized{};
        UINT sampleInvisibleWidth{};
        UINT sampleInvisibleHeight{};
        UINT sampleOwnedWidth{};
        UINT sampleOwnedHeight{};
        UINT sampleChildWidth{};
        UINT sampleChildHeight{};
        UINT sampleUndersizedWidth{};
        UINT sampleUndersizedHeight{};
    };

    BOOL CALLBACK InspectWindow(HWND window, LPARAM parameter)
    {
        auto& summary = *reinterpret_cast<WindowScanSummary*>(parameter);
        DWORD ownerProcess{};
        GetWindowThreadProcessId(window, &ownerProcess);
        if (ownerProcess != g_processId)
            return TRUE;
        ++summary.processWindows;
        if (window == g_overlayDisplayWindow.load(std::memory_order_acquire) ||
            platform::win32::IsOverlayDisplayWindow(window)) {
            ++summary.overlayWindows;
            return TRUE;
        }

        RECT client{};
        const bool hasClientRect = GetClientRect(window, &client) != FALSE;
        const auto width = std::max<LONG>(0, client.right - client.left);
        const auto height = std::max<LONG>(0, client.bottom - client.top);
        const UINT clientWidth = static_cast<UINT>(width);
        const UINT clientHeight = static_cast<UINT>(height);
        if (!IsWindowVisible(window)) {
            ++summary.invisible;
            if (!summary.sampleInvisible) {
                summary.sampleInvisible = window;
                summary.sampleInvisibleWidth = clientWidth;
                summary.sampleInvisibleHeight = clientHeight;
            }
            return TRUE;
        }
        if (GetWindow(window, GW_OWNER)) {
            ++summary.owned;
            if (!summary.sampleOwned) {
                summary.sampleOwned = window;
                summary.sampleOwnedWidth = clientWidth;
                summary.sampleOwnedHeight = clientHeight;
            }
            return TRUE;
        }
        if (GetWindowLongPtrW(window, GWL_STYLE) & WS_CHILD) {
            ++summary.child;
            if (!summary.sampleChild) {
                summary.sampleChild = window;
                summary.sampleChildWidth = clientWidth;
                summary.sampleChildHeight = clientHeight;
            }
            return TRUE;
        }
        if (!hasClientRect) {
            ++summary.clientRectFailed;
            if (!summary.sampleClientRectFailed)
                summary.sampleClientRectFailed = window;
            return TRUE;
        }
        if (width < 320 || height < 200) {
            ++summary.undersized;
            if (!summary.sampleUndersized) {
                summary.sampleUndersized = window;
                summary.sampleUndersizedWidth = clientWidth;
                summary.sampleUndersizedHeight = clientHeight;
            }
            return TRUE;
        }
        ++summary.eligible;
        const auto area = static_cast<std::uint64_t>(width) *
            static_cast<std::uint64_t>(height);
        if (area > summary.best.area)
            summary.best = {window, area, clientWidth, clientHeight};
        return TRUE;
    }

    HWND FindGameWindow() noexcept
    {
        WindowScanSummary summary{};
        const BOOL enumerated = EnumWindows(&InspectWindow,
            reinterpret_cast<LPARAM>(&summary));
        if (diagnostics::Enabled() && ConsumeDiagnosticBudget(g_windowScanLogBudget)) {
            try {
                LogDiscoveryDiagnostic("OVERLAY_DISCOVERY_SCAN result=" +
                    std::string(enumerated ? "complete" : "interrupted") +
                    " process_windows=" + std::to_string(summary.processWindows) +
                    " overlay=" + std::to_string(summary.overlayWindows) +
                    " invisible=" + std::to_string(summary.invisible) +
                    " owned=" + std::to_string(summary.owned) +
                    " child=" + std::to_string(summary.child) +
                    " client_rect_failed=" + std::to_string(summary.clientRectFailed) +
                    " undersized=" + std::to_string(summary.undersized) +
                    " eligible=" + std::to_string(summary.eligible) +
                    " selected=" + std::to_string(reinterpret_cast<std::uintptr_t>(
                        summary.best.window)) + " selected_client=" +
                    std::to_string(summary.best.width) + "x" +
                    std::to_string(summary.best.height));
                if (summary.sampleInvisible)
                    LogDiscoveryDiagnostic("OVERLAY_DISCOVERY_REJECT reason=invisible hwnd=" +
                        std::to_string(reinterpret_cast<std::uintptr_t>(summary.sampleInvisible)) +
                        " client=" + std::to_string(summary.sampleInvisibleWidth) + "x" +
                        std::to_string(summary.sampleInvisibleHeight));
                if (summary.sampleOwned)
                    LogDiscoveryDiagnostic("OVERLAY_DISCOVERY_REJECT reason=owned hwnd=" +
                        std::to_string(reinterpret_cast<std::uintptr_t>(summary.sampleOwned)) +
                        " client=" + std::to_string(summary.sampleOwnedWidth) + "x" +
                        std::to_string(summary.sampleOwnedHeight));
                if (summary.sampleChild)
                    LogDiscoveryDiagnostic("OVERLAY_DISCOVERY_REJECT reason=child hwnd=" +
                        std::to_string(reinterpret_cast<std::uintptr_t>(summary.sampleChild)) +
                        " client=" + std::to_string(summary.sampleChildWidth) + "x" +
                        std::to_string(summary.sampleChildHeight));
                if (summary.sampleClientRectFailed)
                    LogDiscoveryDiagnostic("OVERLAY_DISCOVERY_REJECT reason=client_rect_failed hwnd=" +
                        std::to_string(reinterpret_cast<std::uintptr_t>(summary.sampleClientRectFailed)));
                if (summary.sampleUndersized)
                    LogDiscoveryDiagnostic("OVERLAY_DISCOVERY_REJECT reason=undersized hwnd=" +
                        std::to_string(reinterpret_cast<std::uintptr_t>(summary.sampleUndersized)) +
                        " client=" + std::to_string(summary.sampleUndersizedWidth) + "x" +
                        std::to_string(summary.sampleUndersizedHeight));
            } catch (...) {}
        }
        return summary.best.window;
    }

    void CALLBACK OnWindowEvent(HWINEVENTHOOK, DWORD event, HWND window,
        LONG objectId, LONG childId, DWORD, DWORD)
    {
        if (objectId != OBJID_WINDOW || childId != CHILDID_SELF || !window) return;
        DWORD ownerProcess{};
        GetWindowThreadProcessId(window, &ownerProcess);
        if (ownerProcess != g_processId ||
            window == g_overlayDisplayWindow.load(std::memory_order_acquire) ||
            platform::win32::IsOverlayDisplayWindow(window)) return;
        if (event == EVENT_OBJECT_CREATE || event == EVENT_OBJECT_SHOW ||
            event == EVENT_OBJECT_DESTROY || event == EVENT_OBJECT_LOCATIONCHANGE)
            RequestPresenterRefresh(window, event);
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
            LogicalToPhysicalPointForPerMonitorDPI(window, &position);
            const bool queued = bridge.PushAbsoluteMousePosition(
                position.x, position.y);
            if (!queued) return false;
            RequestOverlayRedraw();
            return true;
        }
        case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN: case WM_XBUTTONUP: case WM_XBUTTONDBLCLK:
            if (!bridge.PushMouseButton(message, wParam)) return false;
            RequestOverlayRedraw();
            return true;
        case WM_MOUSEWHEEL: case WM_MOUSEHWHEEL:
            if (!bridge.PushMouseWheel(message, wParam, lParam)) return false;
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
            if (raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) return false;
            if (!bridge.PushRelativeMouseMotion(raw.data.mouse.lLastX,
                    raw.data.mouse.lLastY)) return false;
            RequestOverlayRedraw();
            (void)window;
            return true;
        }
        default:
            return false;
        }
    }

    bool ActivateOverlayInput(HWND window) noexcept
    {
        if (!overlay::GetInputState().ShouldOwnInput()) return false;
        auto& bridge = overlay::GetInputEventBridge();
        if (bridge.active()) return true;
        POINT position{};
        if (!GetCursorPos(&position) || !ScreenToClient(window, &position))
            position = {};
        // This runs on the presenter's PMv2 owner thread, so ScreenToClient
        // yields the physical client-pixel coordinate used by the surface.
        if (!bridge.TryActivate(position.x, position.y)) return false;
        RequestOverlayRedraw();
        Log("OVERLAY_INPUT_OWNERSHIP=OVERLAY_ACTIVE source=game_hwnd_bridge");
        SetCursor(nullptr);
        return true;
    }

    bool ReconcileOverlayInputReadiness() noexcept
    {
        if (g_inputCallbackAdmission.closed()) return false;
        auto& input = overlay::GetInputState();
        auto* renderer = g_renderer.load(std::memory_order_acquire);
        const HWND window = g_inputWindow.load(std::memory_order_acquire);
        const bool ready = renderer && window && input.ShouldOwnInput() &&
            renderer->OwnsWindow(window) && renderer->InputPresentationReady();
        if (!ready) {
            g_overlayInputReady.store(false, std::memory_order_release);
            g_ownerHeartbeat.Reset();
            overlay::GetInputEventBridge().TryDeactivate();
            return false;
        }
        if (!ActivateOverlayInput(window)) {
            g_overlayInputReady.store(false, std::memory_order_release);
            g_ownerHeartbeat.Reset();
            return false;
        }
        g_ownerHeartbeat.Publish(GetTickCount64());
        g_overlayInputReady.store(true, std::memory_order_release);
        return true;
    }

    bool OverlayOwnerResponsiveForInput() noexcept
    {
        if (!g_overlayInputReady.load(std::memory_order_acquire)) return false;
        if (g_ownerHeartbeat.IsFresh(GetTickCount64(),
                OwnerHeartbeatMaximumAgeMs))
            return true;
        RequestOverlayTerminalWithReason("owner_heartbeat_not_fresh");
        return false;
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

    bool RemoveInputHook() noexcept;
    void CompletePendingInputBinding();

    void DisableAfterOverlayException() noexcept
    {
        RequestOverlayTerminalWithReason("callback_exception");
    }

    OverlayWindowDecision ProcessOverlayWindowMessage(HWND window,
        UINT message, WPARAM wParam, LPARAM lParam,
        bool rebindModifierDown)
    {
        OverlayWindowDecision decision{};
        auto& input = overlay::GetInputState();
        if (RendererInstance().disabled() || window !=
                g_inputWindow.load(std::memory_order_acquire))
            return decision;
        if (message == WM_SETCURSOR && input.ShouldOwnInput() &&
            LOWORD(lParam) == HTCLIENT) {
            SetCursor(nullptr);
            return {true, TRUE};
        }
        overlay::HotkeyAction capturedAction{};
        int capturedPreviousKey{};
        const auto captureResult = overlay::InputState::IsRebindMessage(message)
            ? input.HandleRebindMessage(message, wParam,
                overlay::InputState::IsRebindKeyDownMessage(message) &&
                    rebindModifierDown,
                &capturedAction, &capturedPreviousKey)
            : overlay::RebindMessageResult::PassThrough;
        if (captureResult == overlay::RebindMessageResult::Cancelled) {
            input.PublishCallbackRebindKey(0);
            plugin::SetHotkeyRebindCaptureActive(false);
            RequestOverlayRedraw();
            return {true, 0};
        }
        if (captureResult == overlay::RebindMessageResult::CapturedKey) {
            const int key = static_cast<int>(wParam);
            const auto action = capturedAction;
            const int previousKey = capturedPreviousKey;
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
            const auto commit = api.ApplyAndPersist(mutation);
            const auto result = commit.mutation;
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
                if (action == overlay::HotkeyAction::OverlayToggle)
                    g_overlayToggleKey.store(key, std::memory_order_release);
                plugin::PublishOverlayNotification(
                    plugin::OverlayNotificationKind::BindingChanged,
                    notificationAction(), config::HotkeyName(previousKey),
                    config::HotkeyName(key), commit.persisted
                        ? plugin::OverlayNotificationStatus::Applied
                        : plugin::OverlayNotificationStatus::Error);
                if (!commit.persisted)
                    plugin::PublishOverlayNotification(
                        plugin::OverlayNotificationKind::PersistenceFailed,
                        notificationAction(), {}, {},
                        plugin::OverlayNotificationStatus::Error);
                if (!input.TryCommitAcceptedRebind(key)) return decision;
                input.PublishCallbackRebindKey(0);
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
            RendererInstance().SetVisible(input.visible());
            if (input.visible()) {
                input.SetFocused(true);
                diagnostics::RequestRuntimePerformanceSummary();
                overlay::SynchronizeAutoLanguageOnOverlayOpen(window,
                    input.visible());
            } else {
                g_overlayInputReady.store(false, std::memory_order_release);
                g_ownerHeartbeat.Reset();
                overlay::GetInputEventBridge().TryDeactivate();
                RequestOverlayRedraw();
                if (OriginalWindowProc(window)) {
                    decision.restoreGameCursor = true;
                    Log("OVERLAY_CURSOR_RESTORE=GAME_WNDPROC");
                }
            }
            Log(std::string("OVERLAY_VISIBILITY=") +
                (input.visible() ? "VISIBLE" : "HIDDEN"));
            decision.handled = true;
            RequestOverlayRedraw();
            return decision;
        }
        if (input.ShouldOwnInput()) {
            if (IsKeyboardMessage(message)) {
                if (!overlay::GetInputEventBridge().PushKeyboardMessage(
                        message, wParam, lParam)) return decision;
                RequestOverlayRedraw();
            }
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
                if (QueueOverlayMouseInput(window, message, wParam, lParam))
                    return {true, 0};
            }
        }
        if (message == WM_KILLFOCUS ||
            (message == WM_ACTIVATEAPP && wParam == FALSE)) {
            const auto activityEvent = message == WM_KILLFOCUS
                ? overlay::HostDisplayActivityEvent::KeyboardFocusLost
                : overlay::HostDisplayActivityEvent::ApplicationDeactivated;
            const auto activityAction =
                overlay::ResolveHostDisplayActivity(activityEvent);
            if (activityAction == overlay::HostDisplayActivityAction::Deactivate)
                RendererInstance().OnHostActivation(window, false);
            input.PublishCallbackRebindKey(0);
            input.SetFocused(false);
            input.ResetEscapeDismissSequence();
            if (input.visible()) {
                overlay::GetInputEventBridge().TryDeactivate();
                RequestOverlayRedraw();
            }
            if (!input.TryCancelRebind()) return decision;
            plugin::SetHotkeyRebindCaptureActive(false);
        } else if ((message == WM_SETFOCUS ||
                (message == WM_ACTIVATEAPP && wParam != FALSE))) {
            const auto activityEvent = message == WM_SETFOCUS
                ? overlay::HostDisplayActivityEvent::KeyboardFocusGained
                : overlay::HostDisplayActivityEvent::ApplicationActivated;
            const auto activityAction =
                overlay::ResolveHostDisplayActivity(activityEvent);
            if (activityAction == overlay::HostDisplayActivityAction::Activate)
                RendererInstance().OnHostActivation(window, true);
            input.SetFocused(true);
            if (input.visible()) ActivateOverlayInput(window);
        }
        return decision;
    }

    bool EnqueueOwnerInputCommand(InputWindowBinding* binding, HWND window,
        UINT message, WPARAM wParam, LPARAM lParam,
        bool finishAfterContinuation, bool rebindModifierDown) noexcept
    {
        if (!binding || binding->window != window ||
            g_currentInputBinding.load(std::memory_order_acquire) != binding ||
            g_inputCallbackAdmission.closed())
            return false;
        overlay::OwnerInputCommand command{};
        command.window = window;
        command.generation = binding->generation;
        command.message = message;
        command.wParam = wParam;
        command.lParam = lParam;
        command.bindingIdentity = reinterpret_cast<std::uintptr_t>(binding);
        command.finishAfterContinuation = finishAfterContinuation;
        command.rebindModifierDown = rebindModifierDown;
        if (!g_ownerInputCommands.TryPush(command)) {
            RequestOverlayTerminalWithReason("owner_command_rejected");
            return false;
        }
        if (g_ownerCommandWakeEvent) SetEvent(g_ownerCommandWakeEvent);
        return true;
    }

    bool EnqueueOwnerLanguageResult(InputWindowBinding* binding, HWND window,
        const char* localeCode, bool readSucceeded,
        bool startupGameDataProbe) noexcept
    {
        if (!binding || binding->window != window || !localeCode ||
            g_currentInputBinding.load(std::memory_order_acquire) != binding ||
            g_inputCallbackAdmission.closed())
            return false;
        overlay::OwnerInputCommand command{};
        command.window = window;
        command.generation = binding->generation;
        command.message = overlay::AutoLanguageSyncMessage;
        command.bindingIdentity = reinterpret_cast<std::uintptr_t>(binding);
        command.hasLocaleResult = true;
        command.localeReadSucceeded = readSucceeded;
        command.startupGameDataProbe = startupGameDataProbe;
        for (std::size_t index = 0;
            index + 1 < command.localeCode.size() && localeCode[index]; ++index)
            command.localeCode[index] = localeCode[index];
        if (!g_ownerInputCommands.TryPush(command)) {
            RequestOverlayTerminalWithReason("owner_language_command_rejected");
            return false;
        }
        if (g_ownerCommandWakeEvent) SetEvent(g_ownerCommandWakeEvent);
        return true;
    }

    void ProcessOwnerInputCommands()
    {
        constexpr unsigned MaximumCommandsPerTurn = 64;
        overlay::OwnerInputCommand command{};
        unsigned processed{};
        while (processed < MaximumCommandsPerTurn &&
            g_ownerInputCommands.TryPop(command)) {
            ++processed;
            auto* binding = reinterpret_cast<InputWindowBinding*>(
                command.bindingIdentity);
            if (!binding || binding->generation != command.generation ||
                binding != g_currentInputBinding.load(std::memory_order_acquire)) {
                Log("OVERLAY_INPUT_COMMAND_REJECTED reason=stale_generation");
                continue;
            }
            std::scoped_lock lock{g_rendererMutex};
            if (g_inputCallbackAdmission.closed()) return;
            const auto decision = overlay::RunOptionalOverlayWork([&]() {
                if (command.hasLocaleResult) {
                    overlay::ApplyDetectedGameLanguageLocale(
                        command.localeCode.data(), command.localeReadSucceeded);
                    if (command.startupGameDataProbe)
                        RendererInstance().CompleteStartupGameDataProbe(
                            command.localeReadSucceeded);
                    RequestOverlayRedraw();
                    return;
                }
                const auto result = ProcessOverlayWindowMessage(command.window,
                    command.message, command.wParam, command.lParam,
                    command.rebindModifierDown);
                if (result.restoreGameCursor)
                    PostMessageW(command.window, WM_SETCURSOR,
                        reinterpret_cast<WPARAM>(command.window),
                        MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
                if (command.finishAfterContinuation)
                    FinishWindowMessage(command.window, command.message);
            }, &DisableAfterOverlayException);
            if (decision != overlay::OptionalOverlayWorkResult::Completed) return;
            ReconcileOverlayInputReadiness();
        }
        if (!g_ownerInputCommands.empty() && g_ownerCommandWakeEvent)
            SetEvent(g_ownerCommandWakeEvent);
    }

    void FinishWindowMessage(HWND window, UINT message)
    {
        auto& input = overlay::GetInputState();
        if (message == WM_DESTROY || message == WM_NCDESTROY) {
            overlay::GetInputEventBridge().TryDeactivate();
            if (input.visible()) {
                input.Toggle();
                input.TryCancelRebind();
                plugin::SetHotkeyRebindCaptureActive(false);
                RendererInstance().SetVisible(false);
                SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32512)));
                Log("OVERLAY_CURSOR_RESTORE=WINDOW_DESTROY_ARROW");
            }
        }
        if (message == WM_NCDESTROY) {
            ForgetInputWindow(window);
            if (g_inputWindow.load(std::memory_order_acquire) == window)
                g_inputWindow.store(nullptr, std::memory_order_release);
            auto* binding = static_cast<InputWindowBinding*>(
                g_currentInputBinding.load(std::memory_order_acquire));
            if (binding && binding->window == window)
                g_currentInputBinding.store(nullptr, std::memory_order_release);
            RequestPresenterRefresh(window, EVENT_OBJECT_DESTROY);
        } else if (message == WM_SIZE || message == WM_MOVE ||
            message == WM_DISPLAYCHANGE || message == WM_DPICHANGED ||
            message == WM_WINDOWPOSCHANGED) {
            RequestPresenterRefresh(window, EVENT_OBJECT_LOCATIONCHANGE);
        }
    }

    LRESULT CALLBACK OverlayWindowProc(HWND window, UINT message,
        WPARAM wParam, LPARAM lParam)
    {
        auto* const binding = static_cast<InputWindowBinding*>(
            GetPropW(window, InputBindingProperty));
        const WNDPROC original = binding
            ? binding->original.load(std::memory_order_acquire) : nullptr;
        if (!binding || binding->window != window || !original)
            return DefWindowProcW(window, message, wParam, lParam);

        const bool admitted = g_inputCallbackAdmission.TryEnter();
        if (!admitted || g_currentInputBinding.load(std::memory_order_acquire) != binding) {
            if (admitted && g_inputCallbackAdmission.Leave() && g_callbackDrainEvent)
                SetEvent(g_callbackDrainEvent);
            return CallWindowProcW(original, window, message, wParam, lParam);
        }
        struct AdmissionExit
        {
            overlay::InputCallbackAdmissionGate& gate;
            HANDLE drainEvent{};
            bool active{true};
            void Release() noexcept
            {
                if (active) {
                    active = false;
                    if (gate.Leave() && drainEvent) SetEvent(drainEvent);
                }
            }
            ~AdmissionExit() { Release(); }
        } admission{g_inputCallbackAdmission, g_callbackDrainEvent};

        if (g_inputCallbackAdmission.closed()) {
            admission.Release();
            return CallWindowProcW(original, window, message, wParam, lParam);
        }

        const bool ownerResponsive = OverlayOwnerResponsiveForInput();
        if (g_inputCallbackAdmission.closed()) {
            admission.Release();
            return CallWindowProcW(original, window, message, wParam, lParam);
        }

        if (message == WM_KILLFOCUS ||
            (message == WM_ACTIVATEAPP && wParam == FALSE) ||
            message == WM_SIZE || message == WM_MOVE ||
            message == WM_DISPLAYCHANGE || message == WM_DPICHANGED ||
            message == WM_WINDOWPOSCHANGED || message == WM_NCDESTROY) {
            g_overlayInputReady.store(false, std::memory_order_release);
            g_ownerHeartbeat.Reset();
            overlay::GetInputEventBridge().TryDeactivate();
        }

        bool consume{};
        if (message == overlay::AutoLanguageSyncMessage ||
            message == overlay::StartupGameDataProbeMessage) {
            char localeCode[64]{};
            const bool readSucceeded = overlay::ReadGameLanguageLocale(
                localeCode, std::size(localeCode));
            consume = EnqueueOwnerLanguageResult(binding, window, localeCode,
                readSucceeded,
                message == overlay::StartupGameDataProbeMessage);
        } else if (IsKeyboardMessage(message)) {
            auto& input = overlay::GetInputState();
            const bool overlayOwnsKeyboard = input.ShouldOwnInput() &&
                ownerResponsive;
            const bool rebindActive = input.rebindActive();
            const bool rebindModifiersDown =
                (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0 ||
                (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0 ||
                (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
            const bool isToggle = message == WM_KEYUP &&
                !rebindActive &&
                wParam == static_cast<WPARAM>(g_overlayToggleKey.load(
                    std::memory_order_acquire));
            if (isToggle && ownerResponsive) {
                // Revoke capture before the owner processes a close request.
                // Opening remains fail-open until a visible frame is accepted.
                g_overlayInputReady.store(false, std::memory_order_release);
                g_ownerHeartbeat.Reset();
                overlay::GetInputEventBridge().TryDeactivate();
            }
            const bool isRebindKeyDown = rebindActive &&
                overlay::InputState::IsRebindKeyDownMessage(message) &&
                (wParam == VK_ESCAPE ||
                    (overlay::InputState::IsSupportedBindingKey(
                        static_cast<int>(wParam)) &&
                        !rebindModifiersDown));
            const bool isCapturedRebindKeyUp = rebindActive &&
                (message == WM_KEYUP || message == WM_SYSKEYUP) &&
                input.callbackRebindKey() ==
                    static_cast<int>(wParam);
            const bool escape = overlayOwnsKeyboard && wParam == VK_ESCAPE &&
                (message == WM_KEYDOWN || message == WM_KEYUP ||
                    message == WM_SYSKEYDOWN || message == WM_SYSKEYUP);
            const bool queued = (overlayOwnsKeyboard || isToggle) &&
                EnqueueOwnerInputCommand(binding, window, message,
                    wParam, lParam, false, rebindModifiersDown);
            if (queued && isRebindKeyDown && wParam != VK_ESCAPE)
                input.PublishCallbackRebindKey(static_cast<int>(wParam));
            if (queued && isCapturedRebindKeyUp)
                input.PublishCallbackRebindKey(0);
            consume = queued && overlayOwnsKeyboard && (isToggle ||
                isRebindKeyDown || isCapturedRebindKeyUp || escape);
            if (queued && message == WM_CHAR && !overlayOwnsKeyboard)
                consume = false;
            if (!queued && (isToggle || isRebindKeyDown ||
                    isCapturedRebindKeyUp || escape) &&
                g_inputCallbackAdmission.closed()) {
                consume = false;
            }
        } else if (ownerResponsive && message == WM_SETCURSOR &&
            overlay::GetInputState().ShouldOwnInput() &&
            LOWORD(lParam) == HTCLIENT) {
            consume = EnqueueOwnerInputCommand(binding, window, message,
                wParam, lParam);
        } else if (ownerResponsive &&
            overlay::GetInputState().ShouldOwnInput() &&
            (message == WM_INPUT || IsInputWindowMessage(message))) {
            consume = QueueOverlayMouseInput(window, message, wParam, lParam);
            if (!consume && !overlay::GetInputEventBridge().active())
                RequestOverlayTerminalWithReason("input_bridge_unavailable");
        } else if (message == WM_KILLFOCUS ||
            (message == WM_ACTIVATEAPP && wParam == FALSE) ||
            message == WM_SETFOCUS ||
            (message == WM_ACTIVATEAPP && wParam != FALSE)) {
            if (message == WM_KILLFOCUS ||
                (message == WM_ACTIVATEAPP && wParam == FALSE)) {
                overlay::GetInputState().PublishCallbackRebindKey(0);
                overlay::GetInputEventBridge().TryDeactivate();
            }
            EnqueueOwnerInputCommand(binding, window, message, wParam, lParam);
        }

        const bool finishAfterContinuation = message == WM_DESTROY ||
            message == WM_NCDESTROY || message == WM_SIZE ||
            message == WM_MOVE || message == WM_DISPLAYCHANGE ||
            message == WM_DPICHANGED || message == WM_WINDOWPOSCHANGED;
        const LRESULT result = consume
            ? (message == WM_INPUT ? DefWindowProcW(window, message, wParam, lParam) : 0)
            : CallWindowProcW(original, window, message, wParam, lParam);
        if (!g_inputCallbackAdmission.closed() && finishAfterContinuation)
            // WINDOWPOS belongs to this native call only. The owner needs the
            // notification, not a dangling pointer; it observes current HWNDs.
            EnqueueOwnerInputCommand(binding, window, message, wParam,
                message == WM_WINDOWPOSCHANGED ? 0 : lParam, true);
        admission.Release();
        return result;
    }

    LRESULT CALLBACK InstallInputBindingOnGameThread(int code, WPARAM remove,
        LPARAM parameter) noexcept
    {
        if (code >= 0 && remove == PM_REMOVE &&
            g_inputCallbackAdmission.TryEnter()) {
            auto* binding = g_pendingInputBinding.load(std::memory_order_acquire);
            auto* message = reinterpret_cast<MSG*>(parameter);
            if (binding && message && message->hwnd == binding->window &&
                message->message == InstallInputBindingMessage &&
                !g_inputCallbackAdmission.closed()) {
                auto expected = InputWindowBinding::InstallState::Pending;
                if (binding->installState.compare_exchange_strong(expected,
                        InputWindowBinding::InstallState::Installing)) {
                    const bool installed = overlay::InstallGameThreadSubclass(
                        binding->window, InputBindingProperty,
                        reinterpret_cast<HANDLE>(binding), &OverlayWindowProc,
                        binding->original);
                    if (installed) {
                        g_currentInputBinding.store(binding, std::memory_order_release);
                        g_inputWindow.store(binding->window, std::memory_order_release);
                    }
                    binding->installState.store(installed
                        ? InputWindowBinding::InstallState::Installed
                        : InputWindowBinding::InstallState::Failed,
                        std::memory_order_release);
                    message->message = WM_NULL;
                    RequestOverlayRedraw();
                }
            }
            if (g_inputCallbackAdmission.Leave() && g_callbackDrainEvent)
                SetEvent(g_callbackDrainEvent);
        }
        return CallNextHookEx(nullptr, code, remove, parameter);
    }

    bool InstallInputHook(HWND window)
    {
        if (!window || g_inputWindow.load(std::memory_order_acquire)) return false;
        DWORD processId{};
        const DWORD windowThread = GetWindowThreadProcessId(window, &processId);
        if (!windowThread || processId != GetCurrentProcessId()) return false;
        if (g_inputCallbackAdmission.closed() || g_bindingInstallHook ||
            g_pendingInputBinding.load(std::memory_order_acquire) ||
            GetPropW(window, InputBindingProperty)) return false;
        plugin::RuntimeSettingsSnapshot settings{};
        const int key = plugin::GetRuntimeSettingsApi().Snapshot(settings)
            ? settings.overlayToggleKey : VK_DELETE;
        g_overlayToggleKey.store(key, std::memory_order_release);
        auto* binding = g_inputWindowBindings.TryAcquire();
        if (!binding) {
            if (!g_inputBindingLimitReported) {
                g_inputBindingLimitReported = true;
                try {
                    Log("OVERLAY_INPUT_BINDING_BUDGET_EXHAUSTED retained=" +
                        std::to_string(g_inputWindowBindings.retainedCount()) +
                        " capacity=" +
                        std::to_string(g_inputWindowBindings.capacity()));
                } catch (...) {}
                RequestOverlayTerminal();
            }
            return false;
        }
        binding->window = window;
        binding->generation = g_nextInputBindingGeneration.fetch_add(1,
            std::memory_order_relaxed);
        if (!g_inputWindowBindings.Commit(binding)) {
            RequestOverlayTerminal();
            return false;
        }
        g_pendingInputBinding.store(binding, std::memory_order_release);
        g_bindingInstallHook = SetWindowsHookExW(WH_GETMESSAGE,
            &InstallInputBindingOnGameThread, g_module, windowThread);
        if (!g_bindingInstallHook ||
            !PostMessageW(window, InstallInputBindingMessage, 0, 0)) {
            RequestOverlayTerminal();
            return false;
        }
        Log("OVERLAY_INPUT_BINDING_PENDING game_thread_install=1");
        return true;
    }

    void CompletePendingInputBinding()
    {
        auto* binding = g_pendingInputBinding.load(std::memory_order_acquire);
        if (!binding) return;
        const auto state = binding->installState.load(std::memory_order_acquire);
        if (state == InputWindowBinding::InstallState::Pending ||
            state == InputWindowBinding::InstallState::Installing) return;
        if (g_bindingInstallHook && !UnhookWindowsHookEx(g_bindingInstallHook)) {
            RequestOverlayTerminal();
            return;
        }
        g_bindingInstallHook = nullptr;
        g_pendingInputBinding.store(nullptr, std::memory_order_release);
        if (state != InputWindowBinding::InstallState::Installed ||
            !RendererInstance().OwnsWindow(binding->window)) {
            RequestOverlayTerminal();
            return;
        }
        RendererInstance().SetInputBindingReady(binding->window);
        Log("OVERLAY_INPUT_HOOK_OK game_thread_install=1");
        RequestOverlayRedraw();
#if defined(OVERLAY_STARTUP_JOURNAL)
        diagnostics::startup_journal::MarkOnce(8, "INPUT_ACTIVATED",
            "window_proc_installed");
        diagnostics::startup_journal::Flush();
#endif
    }

    bool RemoveInputHook() noexcept
    {
        const HWND window = g_inputWindow.load(std::memory_order_acquire);
        if (!window) return true;
        auto* binding = static_cast<InputWindowBinding*>(
            GetPropW(window, InputBindingProperty));
        const WNDPROC original = OriginalWindowProc(window);
        if (!original) {
            g_inputWindow.store(nullptr, std::memory_order_release);
            return false;
        }
        bool removed = true;
        if (IsWindow(window) && reinterpret_cast<WNDPROC>(GetWindowLongPtrW(
                window, GWLP_WNDPROC)) == &OverlayWindowProc) {
            SetLastError(0);
            const auto previous = SetWindowLongPtrW(window, GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(original));
            if (!previous && GetLastError() != 0) {
                removed = false;
                try {
                    Log("OVERLAY_INPUT_HOOK_REMOVE_FAILED error=" +
                        std::to_string(GetLastError()));
                } catch (...) {}
            } else {
                ForgetInputWindow(window);
            }
        } else if (IsWindow(window)) {
            try {
                Log("OVERLAY_INPUT_CHAIN_RETAINED foreign_subclass_after_us=1");
            } catch (...) {}
            removed = false;
        } else {
            ForgetInputWindow(window);
        }
        if (g_currentInputBinding.load(std::memory_order_acquire) == binding)
            g_currentInputBinding.store(nullptr, std::memory_order_release);
        g_inputWindow.store(nullptr, std::memory_order_release);
        overlay::GetInputEventBridge().Deactivate();
        return removed;
    }

    void UpdateRenderTimer()
    {
        std::scoped_lock lock{g_rendererMutex};
        const bool minimized = g_presenterMinimized.load(
            std::memory_order_acquire);
        const auto frameState = overlay::SelectPresenterFrameState(
            RendererInstance().ready(), minimized,
            RendererInstance().notificationAnimationActive(),
            RendererInstance().needsRenderWork(), RendererInstance().visible());
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
            RendererInstance().Disable("presenter_timer_unavailable");
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
        if (!RendererInstance().disabled()) return;
        overlay::GetInputEventBridge().Deactivate();
        RemoveInputHook();
    }

    void RefreshPresenter(HWND observedWindow, DWORD event,
        HWND scannedCandidate = nullptr, bool candidateWasScanned = false)
    {
        (void)event;
        if (!g_cameraCoreReady.load(std::memory_order_acquire)) {
            LogRefreshTransition("camera_core_not_ready", observedWindow);
            return;
        }
        const HWND candidate = candidateWasScanned
            ? scannedCandidate : FindGameWindow();
        if (!candidate) {
            LogRefreshTransition("no_eligible_game_window", observedWindow);
            const HWND activeWindow = g_inputWindow.load(std::memory_order_acquire);
            if (activeWindow && !IsWindow(activeWindow)) RemoveInputHook();
            return;
        }
        RECT client{};
        if (!GetClientRect(candidate, &client)) {
            LogRefreshTransition("selected_window_client_rect_failed", candidate);
            return;
        }
        const UINT width = static_cast<UINT>(std::max<LONG>(0,
            client.right - client.left));
        const UINT height = static_cast<UINT>(std::max<LONG>(0,
            client.bottom - client.top));
        LogRefreshTransition("eligible_game_window", candidate, width, height);
        const UINT dpi = GetDpiForWindow(candidate);
        const bool minimized = IsIconic(candidate) || !width || !height;
        g_presenterMinimized.store(minimized, std::memory_order_release);

        std::scoped_lock lock{g_rendererMutex};
        if (RendererInstance().disabled()) {
            LogRefreshTransition("renderer_disabled", candidate, width, height);
            overlay::GetInputEventBridge().Deactivate();
            RemoveInputHook();
            return;
        }
        const auto route = overlay::SelectPresenterRoute(
            RendererInstance().hasSurfaceGeneration(), RendererInstance().OwnsWindow(candidate),
            minimized, observedWindow == candidate);
        if (route == overlay::PresenterRoute::WaitForWindow) {
            LogRefreshTransition("waiting_for_non_minimized_window", candidate,
                width, height);
            UpdateRenderTimer();
            return;
        }
        if (route == overlay::PresenterRoute::Initialize) {
            LogRefreshTransition("initializing_presenter", candidate, width, height);
#if defined(OVERLAY_STARTUP_JOURNAL)
            diagnostics::startup_journal::MarkOnce(4, "GAME_WINDOW_SELECTED",
                "visible_process_top_level_window",
                static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(candidate)));
            diagnostics::startup_journal::Flush();
#endif
            const HWND overlayWindow = g_overlayDisplayWindow.load(
                std::memory_order_acquire);
            if (!overlayWindow) {
                LogRefreshTransition("display_window_missing", candidate,
                    width, height);
            } else if (RendererInstance().Initialize(candidate,
                    overlayWindow, width, height, dpi)) {
                if (InstallInputHook(candidate))
                    Log("OVERLAY_PRESENTER_ATTACHED hwnd=" +
                        std::to_string(reinterpret_cast<std::uintptr_t>(candidate)));
                else
                    Log("OVERLAY_INPUT_ATTACH_UNAVAILABLE");
            } else {
                LogRefreshTransition("renderer_initialize_failed", candidate,
                    width, height);
            }
        } else if (route == overlay::PresenterRoute::RebindWindow) {
            if (g_pendingInputBinding.load(std::memory_order_acquire)) {
                RequestOverlayTerminal();
                return;
            }
            g_overlayInputReady.store(false, std::memory_order_release);
            g_ownerHeartbeat.Reset();
            overlay::GetInputEventBridge().TryDeactivate();
            RemoveInputHook();
            if (RendererInstance().RebindWindow(candidate, width, height, dpi) &&
                InstallInputHook(candidate))
                Log("OVERLAY_PRESENTER_REBOUND hwnd=" +
                    std::to_string(reinterpret_cast<std::uintptr_t>(candidate)));
        } else if (route == overlay::PresenterRoute::UpdateGeometry) {
            if (minimized) {
                RendererInstance().OnWindowGeometry(candidate, width, height, true, dpi);
            } else if (RendererInstance().OnWindowGeometry(candidate, width, height, false, dpi)) {
                Log("OVERLAY_WINDOW_GEOMETRY_UPDATED width=" +
                    std::to_string(width) + " height=" + std::to_string(height));
            }
        }
        ReconcileTerminalInputOwnership();
        UpdateRenderTimer();
        ReconcileOverlayInputReadiness();
    }

    DWORD WINAPI Worker(void*) noexcept
    {
        const DPI_AWARENESS_CONTEXT previousDpiContext =
            SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        overlay::RunOptionalOverlayWork([&]() {
        g_processId = GetCurrentProcessId();
        MSG message{};
        PeekMessageW(&message, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
        g_messageQueueReady.store(true, std::memory_order_release);
        g_uiThreadId.store(GetCurrentThreadId(), std::memory_order_release);
        OpenLog();
        Log("OVERLAY_MESSAGE_QUEUE_READY owner_thread=" +
            std::to_string(g_uiThreadId.load(std::memory_order_acquire)));
        auto* renderer = new (std::nothrow) overlay::Renderer{};
        if (!renderer) {
            Log("OVERLAY_RENDERER_ALLOCATION_FAILED");
            CloseLog();
            g_messageQueueReady.store(false, std::memory_order_release);
            g_workerStarted.store(false, std::memory_order_release);
            g_uiThreadId.store(0, std::memory_order_release);
            return;
        }
        g_renderer.store(renderer, std::memory_order_release);
        RendererInstance().SetLogger(&Log);
        RendererInstance().SetTerminalRequest(&RequestOverlayTerminal);
        Log(previousDpiContext ? "OVERLAY_DPI_THREAD_CONTEXT=PER_MONITOR_V2"
            : "OVERLAY_DPI_THREAD_CONTEXT=UNCHANGED_API_UNAVAILABLE");
        Log("OVERLAY_DCOMP_RUNTIME v=1 dxgi_factory_hooks=0 swapchain_hooks=0");
        const bool displayWindowReady = CreateOverlayDisplayWindow();
        if (!displayWindowReady) {
            Log("OVERLAY_DISPLAY_WINDOW_CREATE_FAILED");
            RequestOverlayTerminal();
        } else {
            Log("OVERLAY_DISPLAY_WINDOW_CREATED owner_thread=1 unowned=1");
            StartFrameClock();
        }
#if defined(OVERLAY_STARTUP_JOURNAL)
        diagnostics::startup_journal::MarkOnce(1, "OVERLAY_WORKER_START",
            "composition_owner_thread");
        diagnostics::startup_journal::Flush();
#endif
        if (displayWindowReady) {
            SetLastError(ERROR_SUCCESS);
            g_windowEvents = SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_SHOW,
                nullptr, &OnWindowEvent, g_processId, 0, WINEVENT_OUTOFCONTEXT);
            const DWORD windowEventsError = g_windowEvents
                ? ERROR_SUCCESS : GetLastError();
            SetLastError(ERROR_SUCCESS);
            g_locationEvents = SetWinEventHook(EVENT_OBJECT_LOCATIONCHANGE,
                EVENT_OBJECT_LOCATIONCHANGE, nullptr, &OnWindowEvent,
                g_processId, 0, WINEVENT_OUTOFCONTEXT);
            const DWORD locationEventsError = g_locationEvents
                ? ERROR_SUCCESS : GetLastError();
            Log("OVERLAY_WINDOW_EVENT_HOOK kind=create_show result=" +
                std::string(g_windowEvents ? "ok" : "failed") +
                " win32_error=" + std::to_string(windowEventsError));
            Log("OVERLAY_WINDOW_EVENT_HOOK kind=location_change result=" +
                std::string(g_locationEvents ? "ok" : "failed") +
                " win32_error=" + std::to_string(locationEventsError));
            Log(g_windowEvents && g_locationEvents ? "OVERLAY_WINDOW_EVENTS_ARMED"
                : "OVERLAY_WINDOW_EVENTS_PARTIAL");
            if (g_cameraCoreReady.load(std::memory_order_acquire))
                RefreshPresenter(nullptr, 0);
        }

        const HANDLE ownerWaitHandles[]{g_terminalWakeEvent,
            g_ownerCommandWakeEvent};
        bool terminalRequested{};
        while (!terminalRequested) {
            CompletePendingInputBinding();
            if (g_overlayInputReady.load(std::memory_order_acquire))
                g_ownerHeartbeat.Publish(GetTickCount64());
            DWORD ownerWaitTimeout = g_overlayInputReady.load(
                std::memory_order_acquire) ? OwnerHeartbeatIntervalMs : INFINITE;
            {
                std::scoped_lock lock{g_rendererMutex};
                const auto probeWait = RendererInstance().
                    StartupGameDataProbeWaitMilliseconds(GetTickCount64());
                if (probeWait != (std::numeric_limits<std::uint32_t>::max)())
                    ownerWaitTimeout = (std::min)(ownerWaitTimeout,
                        static_cast<DWORD>(probeWait));
            }
            const DWORD wait = MsgWaitForMultipleObjectsEx(2,
                ownerWaitHandles,
                ownerWaitTimeout,
                QS_ALLINPUT,
                MWMO_INPUTAVAILABLE);
            if (wait == WAIT_TIMEOUT) {
                overlay::RunOptionalOverlayWork([&]() {
                    std::scoped_lock lock{g_rendererMutex};
                    if (RendererInstance().ResolveStartupGameDataProbeTimeout()) {
                        if (!g_presenterMinimized.load(std::memory_order_acquire))
                            RendererInstance().Render();
                        UpdateRenderTimer();
                    }
                }, &DisableAfterOverlayException);
                ReconcileOverlayInputReadiness();
                continue;
            }
            if (wait == WAIT_OBJECT_0) break;
            if (wait == WAIT_FAILED) {
                RequestOverlayTerminal();
                break;
            }
            if (wait == WAIT_OBJECT_0 + 1) {
                ProcessOwnerInputCommands();
                if (g_inputCallbackAdmission.closed()) break;
                overlay::RunOptionalOverlayWork([&]() {
                    const bool refreshRequested =
                        g_presenterRefreshPending.Take();
                    bool discoveryNeeded{};
                    const HWND candidate = FindGameWindow();
                    {
                        std::scoped_lock lock{g_rendererMutex};
                        if (g_renderer.load(std::memory_order_acquire)) {
                            RendererInstance().RequestRedraw();
                            discoveryNeeded =
                                !RendererInstance().hasSurfaceGeneration() ||
                                (candidate && !RendererInstance().OwnsWindow(candidate));
                            if (!discoveryNeeded &&
                                !g_frameClockAvailable.load(std::memory_order_acquire) &&
                                !g_presenterMinimized.load(std::memory_order_acquire))
                                RendererInstance().Render();
                            if (!discoveryNeeded) UpdateRenderTimer();
                        }
                    }
                    if (refreshRequested || discoveryNeeded)
                        RefreshPresenter(candidate, refreshRequested
                            ? EVENT_OBJECT_LOCATIONCHANGE : EVENT_OBJECT_SHOW,
                            candidate, true);
                    if (refreshRequested && diagnostics::Enabled() &&
                        ConsumeDiagnosticBudget(g_refreshDispatchLogBudget))
                        LogDiscoveryDiagnostic("OVERLAY_REFRESH_DISPATCH source=owner_event pending_consumed=1 hwnd=" +
                            std::to_string(reinterpret_cast<std::uintptr_t>(candidate)));
                }, &DisableAfterOverlayException);
                ReconcileOverlayInputReadiness();
            }
            if (wait != WAIT_OBJECT_0 + 2) continue;

            for (unsigned dispatched = 0; dispatched < 64 &&
                PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE); ++dispatched) {
                if (message.message == WM_QUIT) {
                    RequestOverlayTerminal();
                    terminalRequested = true;
                    break;
                }
            if (message.message == WakeMessage) {
                g_redrawWakePending.BeginHandling();
                overlay::RunOptionalOverlayWork([&]() {
                    const bool refreshRequested =
                        g_presenterRefreshPending.Take();
                    HWND candidate = FindGameWindow();
                    bool discoveryNeeded{};
                    {
                        std::scoped_lock lock{g_rendererMutex};
                        RendererInstance().RequestRedraw();
                        discoveryNeeded = !RendererInstance().hasSurfaceGeneration() ||
                            (candidate && !RendererInstance().OwnsWindow(candidate));
                        if (!discoveryNeeded &&
                            !g_frameClockAvailable.load(std::memory_order_acquire) &&
                            !g_presenterMinimized.load(std::memory_order_acquire))
                            RendererInstance().Render();
                        if (!discoveryNeeded) UpdateRenderTimer();
                    }
                    if (refreshRequested || discoveryNeeded)
                        RefreshPresenter(candidate, refreshRequested
                            ? EVENT_OBJECT_LOCATIONCHANGE : EVENT_OBJECT_SHOW,
                            candidate, true);
                    if (refreshRequested && diagnostics::Enabled() &&
                        ConsumeDiagnosticBudget(g_refreshDispatchLogBudget))
                        LogDiscoveryDiagnostic("OVERLAY_REFRESH_DISPATCH source=thread_message pending_consumed=1 hwnd=" +
                            std::to_string(reinterpret_cast<std::uintptr_t>(candidate)));
                }, &DisableAfterOverlayException);
                ReconcileOverlayInputReadiness();
            } else if (message.message == FrameClockMessage) {
                g_frameClockTickPending.BeginHandling();
                overlay::RunOptionalOverlayWork([&]() {
                    std::scoped_lock lock{g_rendererMutex};
                    if (RendererInstance().ready() &&
                        !g_presenterMinimized.load(std::memory_order_acquire) &&
                        RendererInstance().needsRenderWork())
                        RendererInstance().Render();
                    ReconcileTerminalInputOwnership();
                    UpdateRenderTimer();
                }, &DisableAfterOverlayException);
                ReconcileOverlayInputReadiness();
            } else if (message.message == FrameClockUnavailableMessage) {
                Log("OVERLAY_COMPOSITOR_CLOCK_LOST status=" +
                    std::to_string(static_cast<std::uint32_t>(message.wParam)) +
                    " fallback=event_redraw_and_animation_timer");
                overlay::RunOptionalOverlayWork([&]() {
                    std::scoped_lock lock{g_rendererMutex};
                    if (!g_presenterMinimized.load(std::memory_order_acquire) &&
                        RendererInstance().needsRenderWork())
                        RendererInstance().Render();
                    UpdateRenderTimer();
                }, &DisableAfterOverlayException);
                ReconcileOverlayInputReadiness();
            } else if (message.message == WM_TIMER &&
                g_renderTimer.Matches(message.wParam)) {
                overlay::RunOptionalOverlayWork([&]() {
                    std::scoped_lock lock{g_rendererMutex};
                    const HWND activeWindow =
                        g_inputWindow.load(std::memory_order_acquire);
                    if (RendererInstance().ready() && (!activeWindow || !IsIconic(activeWindow)) &&
                        RendererInstance().needsRenderWork())
                        RendererInstance().Render();
                    if (RendererInstance().disabled()) {
                        ReconcileTerminalInputOwnership();
                    }
                    UpdateRenderTimer();
                }, &DisableAfterOverlayException);
                ReconcileOverlayInputReadiness();
            }
            TranslateMessage(&message);
            DispatchMessageW(&message);
                if (g_inputCallbackAdmission.closed()) {
                    terminalRequested = true;
                    break;
                }
            }
        }
        }, [&]() noexcept {
            auto* renderer = g_renderer.load(std::memory_order_acquire);
            if (renderer) renderer->Disable("overlay_worker_exception");
            else RequestOverlayTerminal();
        });

        RequestOverlayTerminal();
        if (g_renderer.load(std::memory_order_acquire)) {
            try {
                std::scoped_lock lock{g_rendererMutex};
                RendererInstance().SetVisible(false);
            } catch (...) {
                // Do not let an optional visibility update cross the native
                // worker entry; finalization will report any retained state.
            }
        }

        bool callbacksDrained = g_inputCallbackAdmission.inFlight() == 0;
        if (!callbacksDrained && g_callbackDrainEvent) {
            const DWORD drainResult = WaitForSingleObject(g_callbackDrainEvent,
                INFINITE);
            callbacksDrained = drainResult == WAIT_OBJECT_0 &&
                g_inputCallbackAdmission.inFlight() == 0;
        }

        bool observersRemoved = true;
        bool timerRemoved = true;
        bool frameClockStopped = true;
        bool inputHookRemoved = false;
        bool rendererReleased = false;
        bool displayWindowDestroyed = false;
        g_ownerFinalizing.store(true, std::memory_order_release);
        try {
            Log("OVERLAY_OWNER_FINALIZATION=STARTED outcome=PENDING callbacks_in_flight=" +
                std::to_string(g_inputCallbackAdmission.inFlight()) + " reason=" +
                (g_terminalReason.load(std::memory_order_acquire)
                    ? g_terminalReason.load(std::memory_order_acquire) : "unspecified"));
        } catch (...) {}
        const auto outcome = g_ownerFinalizer.Run([&]() {
            const auto* finalRenderer = g_renderer.load(std::memory_order_acquire);
            const bool rendererEmpty = !finalRenderer ||
                finalRenderer->OwnedResourcesReleased();
            const bool noWindow = !g_overlayDisplayWindow.load(
                std::memory_order_acquire);
            const bool noFrameClock = !g_frameClockThread &&
                !g_frameClockReady && !g_frameClockControl &&
                !g_frameClockStop;
            const bool complete = callbacksDrained && observersRemoved && timerRemoved &&
                frameClockStopped && inputHookRemoved && rendererReleased &&
                rendererEmpty && noWindow && noFrameClock;
            if (complete) return overlay::OwnerFinalizationOutcome::Complete;
            if ((!callbacksDrained &&
                    g_inputCallbackAdmission.inFlight() != 0) ||
                (finalRenderer && !rendererEmpty) || !noWindow ||
                !noFrameClock || !inputHookRemoved ||
                g_bindingInstallHook || g_windowEvents || g_locationEvents || g_renderTimer.active())
                return overlay::OwnerFinalizationOutcome::Retained;
            return overlay::OwnerFinalizationOutcome::Unverified;
        }, [&]() {
            if (g_bindingInstallHook) {
                const bool removed = UnhookWindowsHookEx(g_bindingInstallHook) != FALSE;
                observersRemoved = removed;
                if (removed) {
                    g_bindingInstallHook = nullptr;
                    g_pendingInputBinding.store(nullptr, std::memory_order_release);
                }
            }
            if (!g_bindingInstallHook)
                g_pendingInputBinding.store(nullptr, std::memory_order_release);
            if (g_windowEvents) {
                const bool removed = UnhookWinEvent(g_windowEvents) != FALSE;
                observersRemoved = observersRemoved && removed;
                if (removed) g_windowEvents = nullptr;
            }
            if (g_locationEvents) {
                const bool removed = UnhookWinEvent(g_locationEvents) != FALSE;
                observersRemoved = observersRemoved && removed;
                if (removed) g_locationEvents = nullptr;
            }
            return observersRemoved
                ? overlay::OwnerFinalizationOutcome::Complete
                : overlay::OwnerFinalizationOutcome::Retained;
        }, [&]() {
            const auto timerInterval = g_renderTimer.intervalMs();
            const auto timerId = g_renderTimer.TakeForCancellation();
            timerRemoved = !timerId ||
                KillTimer(nullptr, static_cast<UINT_PTR>(timerId)) != FALSE;
            if (!timerRemoved) g_renderTimer.Arm(timerId, timerInterval);
            return timerRemoved ? overlay::OwnerFinalizationOutcome::Complete
                : overlay::OwnerFinalizationOutcome::Unverified;
        }, [&]() {
            frameClockStopped = StopFrameClock();
            return frameClockStopped ? overlay::OwnerFinalizationOutcome::Complete
                : g_frameClockThread ? overlay::OwnerFinalizationOutcome::Retained
                                     : overlay::OwnerFinalizationOutcome::Unverified;
        }, [&]() {
            if (!callbacksDrained) return g_inputCallbackAdmission.inFlight()
                ? overlay::OwnerFinalizationOutcome::Retained
                : overlay::OwnerFinalizationOutcome::Unverified;
            inputHookRemoved = RemoveInputHook();
            overlay::GetInputEventBridge().Deactivate();
            return inputHookRemoved ? overlay::OwnerFinalizationOutcome::Complete
                                    : overlay::OwnerFinalizationOutcome::Retained;
        }, [&]() {
            if (!callbacksDrained) return g_inputCallbackAdmission.inFlight()
                ? overlay::OwnerFinalizationOutcome::Retained
                : overlay::OwnerFinalizationOutcome::Unverified;
            auto* renderer = g_renderer.load(std::memory_order_acquire);
            if (!renderer) {
                rendererReleased = true;
                return overlay::OwnerFinalizationOutcome::Complete;
            }
            std::scoped_lock lock{g_rendererMutex};
            rendererReleased = renderer->Shutdown() &&
                renderer->OwnedResourcesReleased();
            return rendererReleased ? overlay::OwnerFinalizationOutcome::Complete
                : renderer->OwnedResourcesReleased()
                    ? overlay::OwnerFinalizationOutcome::Unverified
                    : overlay::OwnerFinalizationOutcome::Retained;
        }, [&]() {
            if (!rendererReleased) return g_overlayDisplayWindow.load(
                    std::memory_order_acquire)
                ? overlay::OwnerFinalizationOutcome::Retained
                : overlay::OwnerFinalizationOutcome::Unverified;
            displayWindowDestroyed = DestroyOverlayDisplayWindow();
            return displayWindowDestroyed ? overlay::OwnerFinalizationOutcome::Complete
                : (g_overlayDisplayWindow.load(std::memory_order_acquire) ||
                    g_overlayDisplayClassRegistered)
                    ? overlay::OwnerFinalizationOutcome::Retained
                    : overlay::OwnerFinalizationOutcome::Unverified;
        });

        try {
            const char* outcomeName = outcome == overlay::OwnerFinalizationOutcome::Complete
                ? "COMPLETE"
                : outcome == overlay::OwnerFinalizationOutcome::Retained
                    ? "RETAINED" : "UNVERIFIED";
            auto* finalizedRenderer = g_renderer.load(std::memory_order_acquire);
            const bool rendererRefsReleased = !finalizedRenderer ||
                finalizedRenderer->OwnedResourcesReleased();
            Log(std::string("OVERLAY_RENDERER_OWNED_REFS_RELEASED=") +
                (rendererRefsReleased ? "1" : "0"));
            Log(std::string("OVERLAY_OWNER_FINALIZATION=") + outcomeName +
                " callbacks_drained=" + std::to_string(callbacksDrained ? 1 : 0) +
                " observers_removed=" + std::to_string(observersRemoved ? 1 : 0) +
                " timer_removed=" + std::to_string(timerRemoved ? 1 : 0) +
                " frame_clock_stopped=" + std::to_string(frameClockStopped ? 1 : 0) +
                " input_hook_removed=" + std::to_string(inputHookRemoved ? 1 : 0) +
                " renderer_released=" + std::to_string(rendererReleased ? 1 : 0) +
                " display_window_destroyed=" +
                    std::to_string(displayWindowDestroyed ? 1 : 0) +
                " display_class_registered=" +
                    std::to_string(g_overlayDisplayClassRegistered ? 1 : 0) +
                " pending_owner_commands=" +
                    std::to_string(g_ownerInputCommands.size()));
        } catch (...) {
            // Cleanup outcome remains in the one-shot owner record even if
            // optional log formatting cannot be completed.
        }
        CloseLog();
        if (previousDpiContext) SetThreadDpiAwarenessContext(previousDpiContext);
        g_messageQueueReady.store(false, std::memory_order_release);
        g_uiThreadId.store(0, std::memory_order_release);
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
    if (g_inputCallbackAdmission.closed()) return false;
    if (!g_terminalWakeEvent) {
        g_terminalWakeEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!g_terminalWakeEvent) return false;
    }
    if (!g_callbackDrainEvent) {
        g_callbackDrainEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!g_callbackDrainEvent) {
            CloseHandle(g_terminalWakeEvent);
            g_terminalWakeEvent = nullptr;
            return false;
        }
    }
    if (!g_ownerCommandWakeEvent) {
        g_ownerCommandWakeEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!g_ownerCommandWakeEvent) {
            CloseHandle(g_callbackDrainEvent);
            CloseHandle(g_terminalWakeEvent);
            g_callbackDrainEvent = nullptr;
            g_terminalWakeEvent = nullptr;
            return false;
        }
    }
    bool expected = false;
    if (!g_workerStarted.compare_exchange_strong(expected, true,
            std::memory_order_acq_rel))
        return true;
    const HANDLE thread = CreateThread(nullptr, 0, &Worker, nullptr, 0, nullptr);
    if (!thread) {
        g_workerStarted.store(false, std::memory_order_release);
        CloseHandle(g_ownerCommandWakeEvent);
        CloseHandle(g_callbackDrainEvent);
        CloseHandle(g_terminalWakeEvent);
        g_ownerCommandWakeEvent = nullptr;
        g_callbackDrainEvent = nullptr;
        g_terminalWakeEvent = nullptr;
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

extern "C" void NotifyOverlayCameraStatePublished() noexcept
{
    auto* renderer = g_renderer.load(std::memory_order_acquire);
    if (!renderer || !overlay::TryScheduleVisibleCameraStateRedraw(
            renderer->visible(), g_redrawWakePending))
        return;
    RequestOverlayRedraw();
}

extern "C" void RequestOverlayRedraw()
{
    const HANDLE wakeEvent = g_ownerCommandWakeEvent;
    const BOOL signaled = wakeEvent ? SetEvent(wakeEvent) : FALSE;
    const DWORD signalError = signaled ? ERROR_SUCCESS
        : wakeEvent ? GetLastError() : ERROR_INVALID_HANDLE;
    const DWORD threadId = g_uiThreadId.load(std::memory_order_acquire);
    const bool queueReady = g_messageQueueReady.load(std::memory_order_acquire);
    if (!threadId || !queueReady) {
        if (diagnostics::Enabled() &&
            ConsumeDiagnosticBudget(g_refreshRequestLogBudget)) {
            try {
                LogDiscoveryDiagnostic("OVERLAY_REDRAW_REQUEST owner_event_signaled=" +
                    std::to_string(signaled != FALSE) + " queue_ready=" +
                    std::to_string(queueReady) + " thread=" +
                    std::to_string(threadId) + " win32_error=" +
                    std::to_string(signalError));
            } catch (...) {}
        }
        return;
    }
    if (!g_redrawWakePending.TrySchedule()) return;
    SetLastError(ERROR_SUCCESS);
    const BOOL posted = PostThreadMessageW(threadId, WakeMessage, 0, 0);
    const DWORD postError = posted ? ERROR_SUCCESS : GetLastError();
    auto& postBudget = posted ? g_ownerPostSuccessLogBudget
                              : g_ownerPostFailureLogBudget;
    if (diagnostics::Enabled() && ConsumeDiagnosticBudget(postBudget)) {
        try {
            LogDiscoveryDiagnostic("OVERLAY_REDRAW_POST result=" +
                std::string(posted ? "ok" : "failed") + " thread=" +
                std::to_string(threadId) + " owner_event_signaled=" +
                std::to_string(signaled != FALSE) + " win32_error=" +
                std::to_string(postError));
        } catch (...) {}
    }
    if (!posted)
        g_redrawWakePending.CancelFailedPost();
}
