#include "overlay_lifecycle.hpp"
#include "discovery_evidence.hpp"
#include "game_language_reader.hpp"
#include "optional_overlay_boundary.hpp"
#include "dxgi_abi.hpp"
#include "dxgi_hook_registry.hpp"
#include "dxgi_resize_nesting.hpp"
#include "dxgi_module_selection.hpp"
#include "factory_observation_coverage.hpp"
#include "nested_factory_creation.hpp"
#include "../diagnostics/diagnostic_runtime.hpp"
#include "../diagnostics/startup_journal.hpp"
#include "../diagnostics/startup_timeline.hpp"
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
#include <wrl/client.h>

#include <Windows.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#if defined(OVERLAY_FACTORY_CALLER_TRACE) || defined(OVERLAY_STARTUP_TIMELINE)
#include <intrin.h>
#endif

namespace
{
    using CreateFactoryFn = HRESULT(WINAPI*)(REFIID, void**);
    using CreateSwapChainFn = overlay::ComCallbackType<decltype(&IDXGIFactory::CreateSwapChain)>;
    using CreateSwapChainForHwndFn = overlay::ComCallbackType<decltype(&IDXGIFactory2::CreateSwapChainForHwnd)>;
    using CreateSwapChainForCoreWindowFn = overlay::ComCallbackType<decltype(&IDXGIFactory2::CreateSwapChainForCoreWindow)>;
    using CreateSwapChainForCompositionFn = overlay::ComCallbackType<decltype(&IDXGIFactory2::CreateSwapChainForComposition)>;
    using PresentFn = overlay::ComCallbackType<decltype(&IDXGISwapChain::Present)>;
    using Present1Fn = overlay::ComCallbackType<decltype(&IDXGISwapChain1::Present1)>;
    using ResizeBuffersFn = overlay::ComCallbackType<decltype(&IDXGISwapChain::ResizeBuffers)>;
    using ResizeBuffers1Fn = overlay::ComCallbackType<decltype(&IDXGISwapChain3::ResizeBuffers1)>;

    std::mutex g_mutex;
    std::ofstream g_log;
#if defined(OVERLAY_NO_GPU_SUBMISSION_DIAGNOSTIC)
    constexpr const char* OverlayGpuMode = "no_gpu_submission_diagnostic";
#else
    constexpr const char* OverlayGpuMode = "enabled";
#endif
    // Process-resident table ownership; no retained COM object or stale object key.
    auto& g_factoryHooks = *new overlay::DxgiHookRegistry;
    auto& g_swapchainHooks = *new overlay::DxgiHookRegistry;
    std::recursive_mutex g_rendererMutex;
    overlay::FactoryEvidenceStore g_factoryEvidence;
    // Loaded-until-exit contract: no static destructor may suspend threads or
    // remove executable hooks while CRT DLL teardown holds the loader lock.
    auto& g_createFactory = *new safetyhook::InlineHook;
    auto& g_createFactory1 = *new safetyhook::InlineHook;
    auto& g_createFactory2 = *new safetyhook::InlineHook;
    HMODULE g_systemDxgiModule{}; // Process-resident exact System32 module identity.
    thread_local std::uint32_t g_factoryBootstrapDepth{};
    overlay::Lifecycle g_lifecycle;
    overlay::ObservationWindow g_presentWindow;
    overlay::QueueEvidenceStore g_evidence;
#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
    overlay::Renderer g_renderer;
    struct PendingRendererTarget
    {
        Microsoft::WRL::ComPtr<IDXGISwapChain> swapchain;
        Microsoft::WRL::ComPtr<ID3D12Device> device;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
        std::uintptr_t identity{};
        HWND window{};
        UINT bufferCount{};
        DXGI_FORMAT format{DXGI_FORMAT_R8G8B8A8_UNORM};
    };
    std::optional<PendingRendererTarget> g_pendingRendererTarget;
    HWND g_inputWindow{};
    WNDPROC g_originalWindowProc{};
    bool g_terminalCursorRestorePosted{}; // Renderer mutex; one owner-thread restoration.
#endif
    std::atomic<bool> g_stopping{};
#if defined(OVERLAY_COMBINED)
    std::atomic<bool> g_cameraCoreReady{};
#else
    std::atomic<bool> g_cameraCoreReady{true};
#endif

    void DisableAfterOverlayException();

    class FactoryBootstrapScope
    {
    public:
        FactoryBootstrapScope() noexcept { ++g_factoryBootstrapDepth; }
        FactoryBootstrapScope(const FactoryBootstrapScope&) = delete;
        FactoryBootstrapScope& operator=(const FactoryBootstrapScope&) = delete;
        ~FactoryBootstrapScope() { --g_factoryBootstrapDepth; }
    };

    void RestoreInputAfterTerminalFailure() noexcept
    {
        try {
#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
        HWND window{};
        {
            std::scoped_lock rendererLock{g_rendererMutex};
            if (!g_renderer.disabled() || g_terminalCursorRestorePosted) return;
            window = g_inputWindow;
            if (window) g_terminalCursorRestorePosted = true;
        }
        plugin::SetHotkeyRebindCaptureActive(false);
        plugin::MarkHotkeyRebindKeyConsumed(0);
        // Do not invoke a foreign WndProc on the DXGI/render thread or read its
        // mutable chain outside the mutex. The window thread forwards this message.
        if (window) PostMessageW(window, WM_SETCURSOR,
            reinterpret_cast<WPARAM>(window), MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
#endif
        } catch (...) {
            try { DisableAfterOverlayException(); } catch (...) {}
        }
    }

    void PrepareForSwapchainReplacement(HWND window)
    {
#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
        if (!window) return;
        std::scoped_lock rendererLock{g_rendererMutex};
        if (g_pendingRendererTarget && g_pendingRendererTarget->window == window)
            g_pendingRendererTarget.reset();
        if (g_renderer.OwnsWindow(window))
            g_renderer.BeforeSwapchainReplacement(window);
#else
        (void)window;
#endif
    }

    void CompleteSwapchainCreation(HWND window, HRESULT result)
    {
#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
        if (!window || SUCCEEDED(result)) return;
        std::scoped_lock rendererLock{g_rendererMutex};
        if (g_pendingRendererTarget && g_pendingRendererTarget->window == window)
            g_pendingRendererTarget.reset();
        g_renderer.OnSwapchainCreationFailure(window);
#else
        (void)window;
        (void)result;
#endif
    }

    // The outer optional-work boundary also contains failure-transition errors.
    // Mutex acquisition may throw; noexcept here would terminate before that catch.
    void DisableAfterOverlayException()
    {
        std::scoped_lock rendererLock{g_rendererMutex};
        {
            std::scoped_lock lock{g_mutex};
            g_lifecycle.Fail();
        }
#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
        g_renderer.Disable("overlay_callback_exception");
        g_pendingRendererTarget.reset();
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

    void LogFactoryTrace(const std::string& message)
    {
#if defined(OVERLAY_FACTORY_CALLER_TRACE)
        Log(message);
#else
        LogDiagnostic(message);
#endif
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

#if defined(OVERLAY_FACTORY_CALLER_TRACE)
    std::string CallerModuleEvidence(const void* address)
    {
        HMODULE module{};
        const auto resolved = address && GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(address), &module);
        char moduleName[MAX_PATH * 3] = "unknown";
        if (resolved) {
            wchar_t path[MAX_PATH]{};
            const DWORD pathLength = GetModuleFileNameW(module, path, MAX_PATH);
            if (pathLength && pathLength < MAX_PATH) {
                const wchar_t* basename = std::wcsrchr(path, L'\\');
                basename = basename ? basename + 1 : path;
                WideCharToMultiByte(CP_UTF8, 0, basename, -1,
                    moduleName, static_cast<int>(std::size(moduleName)), nullptr, nullptr);
            }
        }
        return std::string(" caller=") + Hex(address) +
            " callerModule=" + (resolved ? Hex(module) : "unknown") +
            " callerModuleName=" + moduleName;
    }
#endif

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

    bool IsCurrentProcessWindow(HWND window) noexcept
    {
        if (!window || !IsWindow(window)) return false;
        DWORD processId{};
        if (!GetWindowThreadProcessId(window, &processId)) return false;
        return processId == GetCurrentProcessId();
    }

    bool SameComIdentity(IUnknown* left, IUnknown* right)
    {
        if (!left || !right) return false;
        Microsoft::WRL::ComPtr<IUnknown> leftIdentity;
        Microsoft::WRL::ComPtr<IUnknown> rightIdentity;
        return SUCCEEDED(left->QueryInterface(IID_PPV_ARGS(&leftIdentity))) &&
            SUCCEEDED(right->QueryInterface(IID_PPV_ARGS(&rightIdentity))) &&
            leftIdentity.Get() == rightIdentity.Get();
    }

    HMODULE LoadExactSystemDxgiModule()
    {
        std::vector<wchar_t> directory(32768);
        const UINT directoryLength = GetSystemDirectoryW(
            directory.data(), static_cast<UINT>(directory.size()));
        if (!directoryLength || directoryLength >= directory.size()) {
            Log("DXGI_MODULE_RESOLUTION_FAILED reason=system_directory_unavailable");
            return nullptr;
        }
        std::wstring expectedPath(directory.data(), directoryLength);
        expectedPath += L"\\dxgi.dll";

        // The process can load a wrapper and the OS DXGI runtime under the same
        // basename. Resolve the canonical system path explicitly; never let
        // GetModuleHandle(basename) pick a timing-dependent wrapper instead.
        HMODULE module = LoadLibraryExW(expectedPath.c_str(), nullptr,
            LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!module) {
            Log("DXGI_MODULE_RESOLUTION_FAILED reason=system_dxgi_load_failed");
            return nullptr;
        }
        std::vector<wchar_t> loadedPath(32768);
        const DWORD loadedLength = GetModuleFileNameW(module,
            loadedPath.data(), static_cast<DWORD>(loadedPath.size()));
        if (!loadedLength || loadedLength >= loadedPath.size() ||
            !overlay::IsExactSystemDxgiPath(
                std::wstring_view(loadedPath.data(), loadedLength), expectedPath)) {
            FreeLibrary(module);
            Log("DXGI_MODULE_RESOLUTION_FAILED reason=loaded_path_mismatch");
            return nullptr;
        }
        g_systemDxgiModule = module; // Deliberately retained for process lifetime.
        Log("DXGI_MODULE_SELECTED source=exact_system_path module=" + Hex(module));
        return module;
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

    void LogPresentFailure(IDXGISwapChain* swapchain, std::uintptr_t identity,
        HRESULT result, UINT syncInterval, UINT flags, std::uint32_t sequence,
        std::uintptr_t device, std::uintptr_t queue,
        overlay::AssociationState stateBefore, overlay::AssociationState stateAfter,
        bool overlayRenderCalled, bool overlayCommandListSubmitted,
        const char* path) noexcept
    {
        // Failure-only telemetry is useful even with Diagnostics.Enabled=false.
        // Logging must never change the native Present result or escape its ABI.
        try {
            char resultHex[16]{};
            std::snprintf(resultHex, sizeof(resultHex), "0x%08X",
                static_cast<unsigned int>(static_cast<std::uint32_t>(result)));
            HRESULT getDeviceResult = E_POINTER;
            HRESULT deviceRemovedReason = E_UNEXPECTED;
            Microsoft::WRL::ComPtr<ID3D12Device> removedDevice;
            if (swapchain) {
                getDeviceResult = swapchain->GetDevice(
                    IID_PPV_ARGS(removedDevice.GetAddressOf()));
                if (SUCCEEDED(getDeviceResult) && removedDevice)
                    deviceRemovedReason = removedDevice->GetDeviceRemovedReason();
            }
            char getDeviceResultHex[16]{};
            char deviceRemovedReasonHex[16]{};
            std::snprintf(getDeviceResultHex, sizeof(getDeviceResultHex), "0x%08X",
                static_cast<unsigned int>(static_cast<std::uint32_t>(getDeviceResult)));
            std::snprintf(deviceRemovedReasonHex, sizeof(deviceRemovedReasonHex), "0x%08X",
                static_cast<unsigned int>(static_cast<std::uint32_t>(deviceRemovedReason)));
            std::string dredSummary = "unavailable";
            if (SUCCEEDED(getDeviceResult) && removedDevice) {
                Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedData1> dred;
                const HRESULT dredQuery = removedDevice.As(&dred);
                if (SUCCEEDED(dredQuery) && dred) {
                    D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 breadcrumbs{};
                    D3D12_DRED_PAGE_FAULT_OUTPUT1 pageFault{};
                    const HRESULT breadcrumbsResult = dred->GetAutoBreadcrumbsOutput1(&breadcrumbs);
                    const HRESULT pageFaultResult = dred->GetPageFaultAllocationOutput1(&pageFault);
                    dredSummary = "query=available breadcrumbsHr=" +
                        std::to_string(static_cast<std::uint32_t>(breadcrumbsResult)) +
                        " pageFaultHr=" + std::to_string(static_cast<std::uint32_t>(pageFaultResult));
                    const D3D12_AUTO_BREADCRUMB_NODE1* node =
                        SUCCEEDED(breadcrumbsResult) ? breadcrumbs.pHeadAutoBreadcrumbNode : nullptr;
                    for (unsigned int index = 0; node && index < 12; ++index, node = node->pNext) {
                        const UINT last = node->pLastBreadcrumbValue
                            ? *node->pLastBreadcrumbValue : (std::numeric_limits<UINT>::max)();
                        dredSummary += " node" + std::to_string(index) + "={listNamePtr=" +
                            Hex(node->pCommandListDebugNameA) +
                            ",queueNamePtr=" + Hex(node->pCommandQueueDebugNameA) +
                            ",breadcrumbCount=" + std::to_string(node->BreadcrumbCount) +
                            ",lastBreadcrumb=" + std::to_string(last) + "}";
                    }
                    if (SUCCEEDED(pageFaultResult))
                        dredSummary += " pageFaultVA=" + std::to_string(pageFault.PageFaultVA) +
                            " existingAllocPtr=" +
                                Hex(pageFault.pHeadExistingAllocationNode
                                    ? pageFault.pHeadExistingAllocationNode->ObjectNameA : nullptr) +
                            " recentlyFreedAllocPtr=" +
                                Hex(pageFault.pHeadRecentFreedAllocationNode
                                    ? pageFault.pHeadRecentFreedAllocationNode->ObjectNameA : nullptr);
                } else {
                    dredSummary = "queryHr=" + std::to_string(static_cast<std::uint32_t>(dredQuery));
                }
            }
            Log(std::string("PRESENT_FAILED path=") + path +
                " swapchain=" + Hex(swapchain) +
                " identity=" + (identity ? Hex(reinterpret_cast<void*>(identity)) : "unqueried") +
                " hr=" + resultHex +
                " getDeviceHr=" + getDeviceResultHex +
                " deviceRemovedReason=" + deviceRemovedReasonHex +
                " dred=" + dredSummary +
                " syncInterval=" + std::to_string(syncInterval) +
                " flags=" + std::to_string(flags) +
                " sequence=" + std::to_string(sequence) +
                " device=" + (device ? Hex(reinterpret_cast<void*>(device)) : "unknown") +
                " queue=" + (queue ? Hex(reinterpret_cast<void*>(queue)) : "unknown") +
                " associationBefore=" + AssociationStateName(stateBefore) +
                " associationAtPresent=" + AssociationStateName(stateAfter) +
                " overlayGpuMode=" + OverlayGpuMode +
                " overlayRenderCalled=" + (overlayRenderCalled ? "true" : "false") +
                " overlayCommandListSubmitted=" +
                    (overlayCommandListSubmitted ? "true" : "false"));
#if defined(OVERLAY_BREAK_ON_PRESENT_FAILURE)
            // Diagnostic-only build: stop only when a debugger is attached, after
            // failure telemetry is flushed and before UE handles the failed Present.
            if (IsDebuggerPresent()) __debugbreak();
#endif
        } catch (...) {
            // Native Present pass-through has priority over optional diagnostics.
        }
    }

    overlay::DxgiHookRegistry::Lease FindSwapchain(void* object)
    {
        return g_swapchainHooks.Find(object);
    }

    overlay::DxgiHookRegistry::Lease FindFactory(void* object)
    {
        return g_factoryHooks.Find(object);
    }

    std::uintptr_t SwapchainIdentity(IUnknown* self)
    {
        Microsoft::WRL::ComPtr<IUnknown> identity;
        return SUCCEEDED(self->QueryInterface(IID_PPV_ARGS(&identity)))
            ? reinterpret_cast<std::uintptr_t>(identity.Get()) : 0;
    }

#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
    void TryActivateRenderer(IDXGISwapChain* presentedSwapchain,
        std::uintptr_t identity);
#endif

    const char* FactoryInterfaceName(REFIID riid)
    {
        if (riid == __uuidof(IDXGIFactory7)) return "IDXGIFactory7";
        if (riid == __uuidof(IDXGIFactory6)) return "IDXGIFactory6";
        if (riid == __uuidof(IDXGIFactory5)) return "IDXGIFactory5";
        if (riid == __uuidof(IDXGIFactory4)) return "IDXGIFactory4";
        if (riid == __uuidof(IDXGIFactory3)) return "IDXGIFactory3";
        if (riid == __uuidof(IDXGIFactory2)) return "IDXGIFactory2";
        if (riid == __uuidof(IDXGIFactory1)) return "IDXGIFactory1";
        if (riid == __uuidof(IDXGIFactory)) return "IDXGIFactory";
        if (riid == __uuidof(IUnknown)) return "IUnknown";
        return "Other";
    }

    const char* HookInstallStatusName(overlay::DxgiHookInstallStatus status)
    {
        using Status = overlay::DxgiHookInstallStatus;
        switch (status) {
        case Status::Installed: return "installed";
        case Status::InvalidArgument: return "invalid_argument";
        case Status::ObjectUnreadable: return "object_unreadable";
        case Status::DuplicateVtable: return "duplicate_vtable";
        case Status::VtableUnreadable: return "vtable_unreadable";
        case Status::NullMethod: return "null_method";
        case Status::ReplacementMismatch: return "replacement_mismatch";
        case Status::RegionUnavailable: return "region_unavailable";
        case Status::VtableOutsideRegion: return "vtable_outside_region";
        case Status::ProtectionChangeFailed: return "protection_change_failed";
        case Status::ProtectionRestoreFailed: return "protection_restore_failed";
        }
        return "unknown";
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

#if defined(OVERLAY_STARTUP_JOURNAL)
    void MarkRealFactoryObserved(const char* source, const void* factory,
        bool flush = false) noexcept
    {
        const bool recorded = diagnostics::startup_journal::MarkOnce(2,
            "FACTORY_FIRST_OBSERVED", source,
            static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(factory)));
        if (recorded && flush)
            diagnostics::startup_journal::Flush();
    }

    void MarkFirstSwapchainCreated(const char* method, IDXGISwapChain* swapchain) noexcept
    {
        if (diagnostics::startup_journal::MarkOnce(1, "SWAPCHAIN_FIRST_CREATED", method,
                static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(swapchain))))
            diagnostics::startup_journal::Flush();
    }
#endif

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
        // Terminal failure is authoritative: Toggle must not reopen invisible
        // input capture after the ImGui context and renderer have been torn down.
        if (g_renderer.disabled()) return decision;
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
        WNDPROC original{};
        overlay::RunOptionalOverlayWork([&]() {
            std::scoped_lock rendererLock{g_rendererMutex};
            original = g_originalWindowProc;
            decision = ProcessOverlayWindowMessage(window, message, wParam, lParam);
        }, &DisableAfterOverlayException);
        RestoreInputAfterTerminalFailure();
        if (decision.restoreGameCursor && original) {
            CallWindowProcW(original, window, WM_SETCURSOR,
                reinterpret_cast<WPARAM>(window), MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
        }
        if (decision.handled) {
            if (decision.callDefaultProcedure)
                return DefWindowProcW(window, message, wParam, lParam);
            return decision.result;
        }
        const LRESULT result = original
            ? CallWindowProcW(original, window, message, wParam, lParam)
            : DefWindowProcW(window, message, wParam, lParam);
        overlay::RunOptionalOverlayWork([&]() {
            std::scoped_lock rendererLock{g_rendererMutex};
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
#if defined(OVERLAY_STARTUP_JOURNAL)
        diagnostics::startup_journal::MarkOnce(8, "INPUT_ACTIVATED", "window_proc_installed");
        diagnostics::startup_journal::Flush();
#endif
    }
#endif

    void TraceFactoryMethod(overlay::FactoryMethod method, HRESULT result,
        IDXGISwapChain* swapchain, IUnknown* factory, const void* caller)
    {
#if !defined(OVERLAY_FACTORY_CALLER_TRACE)
        (void)factory;
        (void)caller;
#endif
        const auto message = std::string("FACTORY_METHOD_INVOKED method=") + FactoryMethodName(method) +
            " result=" + std::to_string(result) +
#if defined(OVERLAY_FACTORY_CALLER_TRACE)
            " factory=" + Hex(factory) +
            " returnedSwapchain=" + Hex(swapchain)
            + CallerModuleEvidence(caller)
#else
            " returnedSwapchain=" + Hex(swapchain)
#endif
            ;
        if (FAILED(result)) Log(message);
        else LogFactoryTrace(message);
    }

    void TraceSwapchainIdentity(IDXGISwapChain* swapchain, IUnknown* creationDevice,
        const char* source, HWND window, UINT width, UINT height, UINT buffers)
    {
        DXGI_SWAP_CHAIN_DESC desc{};
        if (FAILED(swapchain->GetDesc(&desc))) {
            Log(std::string("SWAPCHAIN_DESC_FAILED source=") + source);
            return;
        }

        Microsoft::WRL::ComPtr<ID3D12Device> deviceOwner;
        const HRESULT deviceResult = swapchain->GetDevice(
            __uuidof(ID3D12Device), reinterpret_cast<void**>(deviceOwner.GetAddressOf()));
        auto* device = deviceOwner.Get();

        Microsoft::WRL::ComPtr<ID3D12CommandQueue> queueOwner;
        D3D12_COMMAND_LIST_TYPE queueType = static_cast<D3D12_COMMAND_LIST_TYPE>(-1);
        HRESULT queueResult = E_NOINTERFACE;
        if (creationDevice)
            queueResult = creationDevice->QueryInterface(
                __uuidof(ID3D12CommandQueue), reinterpret_cast<void**>(queueOwner.GetAddressOf()));
        auto* queue = queueOwner.Get();
        if (queue) {
            const auto queueDesc = queue->GetDesc();
            queueType = queueDesc.Type;
        }
        Microsoft::WRL::ComPtr<ID3D12Device> queueDeviceOwner;
        const HRESULT queueDeviceResult = queue
            ? queue->GetDevice(IID_PPV_ARGS(&queueDeviceOwner)) : E_NOINTERFACE;
        const bool deviceQueueMatch = SUCCEEDED(deviceResult) &&
            SUCCEEDED(queueDeviceResult) &&
            SameComIdentity(device, queueDeviceOwner.Get());

        Microsoft::WRL::ComPtr<IDXGISwapChain3> swapchain3Owner;
        UINT backBufferIndex = 0;
        const bool hasSwapchain3 = SUCCEEDED(swapchain->QueryInterface(
            __uuidof(IDXGISwapChain3), reinterpret_cast<void**>(swapchain3Owner.GetAddressOf())));
        auto* swapchain3 = swapchain3Owner.Get();
        if (hasSwapchain3 && swapchain3)
            backBufferIndex = swapchain3->GetCurrentBackBufferIndex();

        bool candidateAdded = false;
        std::size_t candidateCount = 0;
        const auto identity = SwapchainIdentity(swapchain);
        const bool eligibleWindow = IsCurrentProcessWindow(window) &&
            desc.BufferDesc.Width != 0 && desc.BufferDesc.Height != 0;
        if (identity && queue && device && deviceQueueMatch && eligibleWindow &&
            desc.BufferCount != 0 && desc.BufferCount <= 16 &&
            queueType == D3D12_COMMAND_LIST_TYPE_DIRECT) {
            std::scoped_lock lock{g_mutex};
            candidateAdded = g_evidence.ObserveCandidate(
                identity,
                reinterpret_cast<std::uintptr_t>(device),
                reinterpret_cast<std::uintptr_t>(queue),
                static_cast<std::uint32_t>(queueType));
            candidateCount = g_evidence.CandidateCount(
                identity);
        }

        LogDiagnostic(std::string("SWAPCHAIN_DISCOVERED source=") + source +
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
            " queueDeviceHr=" + std::to_string(queueDeviceResult) +
            " deviceQueueMatch=" + (deviceQueueMatch ? "true" : "false") +
            " processWindow=" + (eligibleWindow ? "true" : "false") +
            " actualBuffers=" + std::to_string(desc.BufferCount) +
            " backBufferIndex=" + std::to_string(backBufferIndex) +
            " queueCandidateAdded=" + (candidateAdded ? "true" : "false") +
            " queueCandidateCount=" + std::to_string(candidateCount));

#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
        if (identity && queue && device && deviceQueueMatch && eligibleWindow &&
            desc.BufferCount != 0 && desc.BufferCount <= 16 &&
            queueType == D3D12_COMMAND_LIST_TYPE_DIRECT) {
            Microsoft::WRL::ComPtr<IUnknown> identityOwner;
            if (SUCCEEDED(swapchain->QueryInterface(IID_PPV_ARGS(&identityOwner))) &&
                identityOwner) {
                std::scoped_lock rendererLock{g_rendererMutex};
                if (!g_renderer.disabled() &&
                    (!g_renderer.ready() || g_renderer.OwnsWindow(window))) {
                    PendingRendererTarget target{};
                    target.swapchain = swapchain;
                    target.device = device;
                    target.queue = queue;
                    target.identity = reinterpret_cast<std::uintptr_t>(identityOwner.Get());
                    target.window = window;
                    target.bufferCount = desc.BufferCount;
                    target.format = desc.BufferDesc.Format;
                    // Prefer the most recently created eligible chain while the
                    // candidate set is still forming; no GPU work starts here.
                    g_pendingRendererTarget = std::move(target);
#if defined(OVERLAY_STARTUP_JOURNAL)
                    diagnostics::startup_journal::TrackPendingTarget(
                        g_pendingRendererTarget->identity);
#endif
                    LogDiagnostic("OVERLAY_RENDER_TARGET_PENDING identity=" + Hex(identityOwner.Get()) +
                        " reason=awaiting_successful_presents");
                }
            }
        } else {
            LogDiagnostic("OVERLAY_RENDER_TARGET_REJECTED reason=unvalidated_device_queue_or_window");
        }
#endif

    }

    HRESULT STDMETHODCALLTYPE HookPresent(IDXGISwapChain* self, UINT syncInterval, UINT flags)
    {
        const auto hooks = FindSwapchain(self);
        PresentFn original = hooks ? hooks->Original<PresentFn>(8) : nullptr;
        if (!original) return E_FAIL;
        // TEST is a visibility probe, not a frame. DO_NOT_WAIT must not acquire
        // an Overlay GPU wait or submit work for a native nonblocking attempt.
        if (flags & (DXGI_PRESENT_TEST | DXGI_PRESENT_DO_NOT_WAIT)) {
            const HRESULT result = original(self, syncInterval, flags);
            if (FAILED(result) &&
                !(flags & DXGI_PRESENT_DO_NOT_WAIT &&
                    result == DXGI_ERROR_WAS_STILL_DRAWING)) {
                overlay::RunOptionalOverlayWork([&]() {
                    LogPresentFailure(self, 0, result, syncInterval, flags, 0, 0, 0,
                        overlay::AssociationState::Unknown,
                        overlay::AssociationState::Unknown, false, false,
                        "probe_passthrough");
                }, &DisableAfterOverlayException);
            }
            return result;
        }
        std::uintptr_t identity{};
        std::uint32_t sequence = 0;
        bool shouldLog = false;
        bool revalidated = false;
        overlay::AssociationState stateBefore = overlay::AssociationState::Unknown;
        overlay::AssociationState stateAfter = overlay::AssociationState::Unknown;
        std::uintptr_t device = 0;
        std::uintptr_t queue = 0;
        bool overlayRenderCalled = false;
        bool overlayCommandListSubmitted = false;
        overlay::RunOptionalOverlayWork([&]() {
        identity = SwapchainIdentity(self);
        {
            std::scoped_lock lock{g_mutex};
            if (hooks) {
                sequence = ++hooks->presentObservations;
                stateBefore = g_evidence.State(identity);
                shouldLog = diagnostics::Enabled() && g_presentWindow.Consume();
                stateAfter = stateBefore;
                g_evidence.GetSingleCandidate(identity,
                    device, queue);
            }
        }
#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
#if !defined(OVERLAY_NO_GPU_SUBMISSION_DIAGNOSTIC)
        if (hooks && stateAfter == overlay::AssociationState::Supported) {
            std::scoped_lock rendererLock{g_rendererMutex};
            // A successful native resize leaves the renderer in Resizing until
            // RenderImpl rebuilds its backbuffers on the next validated Present.
            // Do not gate that recovery path on ready(), which is false by design.
            if (g_renderer.needsRenderWork() && g_renderer.OwnsSwapchain(self)) {
                overlayRenderCalled = true;
                overlayCommandListSubmitted = g_renderer.Render(self, stateAfter);
            }
        }
#endif
#endif
        }, &DisableAfterOverlayException);
        RestoreInputAfterTerminalFailure();
        const HRESULT result = original(self, syncInterval, flags);
        // Only S_OK represents a presented frame. Positive DXGI status codes
        // such as OCCLUDED must not count toward renderer activation evidence.
        if (overlay::IsSuccessfulPresentResult(result)) {
#if defined(OVERLAY_STARTUP_JOURNAL)
            const unsigned journalPresent =
                diagnostics::startup_journal::RecordSuccessfulPendingPresent(identity);
            if (journalPresent == 1) {
                diagnostics::startup_journal::Mark("FIRST_SUCCESSFUL_PRESENT",
                    "pending_target", static_cast<std::uint64_t>(identity));
            } else if (journalPresent == 2) {
                diagnostics::startup_journal::Mark("SECOND_SUCCESSFUL_PRESENT",
                    "pending_target", static_cast<std::uint64_t>(identity));
                diagnostics::startup_journal::Flush();
            }
#endif
            overlay::RunOptionalOverlayWork([&]() {
                {
                    std::scoped_lock lock{g_mutex};
                    if (hooks) {
                        g_evidence.ObservePresent(identity);
                        stateAfter = g_evidence.State(identity);
                        revalidated = stateBefore ==
                            overlay::AssociationState::ResizeRevalidationRequired &&
                            stateAfter == overlay::AssociationState::Supported;
                    }
                }
#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
                if (hooks) TryActivateRenderer(self, identity);
#endif
            }, &DisableAfterOverlayException);
        }
        overlay::RunOptionalOverlayWork([&]() {
        if (hooks && shouldLog) {
            IDXGISwapChain3* swapchain3 = nullptr;
            UINT index = 0;
            if (SUCCEEDED(self->QueryInterface(__uuidof(IDXGISwapChain3),
                reinterpret_cast<void**>(&swapchain3))) && swapchain3) {
                index = swapchain3->GetCurrentBackBufferIndex();
                swapchain3->Release();
            }
            std::scoped_lock lock{g_mutex};
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
                  << " nativePresentSucceeded=" <<
                        (overlay::IsSuccessfulPresentResult(result) ? "true" : "false")
                  << '\n';
            g_log.flush();
        }
        }, &DisableAfterOverlayException);
        if (FAILED(result))
            overlay::RunOptionalOverlayWork([&]() {
                LogPresentFailure(self, identity, result, syncInterval, flags, sequence,
                    device, queue, stateBefore, stateAfter, overlayRenderCalled,
                    overlayCommandListSubmitted, "frame");
            }, &DisableAfterOverlayException);
        return result;
    }

    // Present1 is diagnostic-only: preserve every native argument/result and
    // do not render Overlay work from this alternate presentation entry point.
    HRESULT STDMETHODCALLTYPE HookPresent1(IDXGISwapChain1* self, UINT syncInterval,
        UINT flags, const DXGI_PRESENT_PARAMETERS* presentParameters)
    {
        const auto hooks = FindSwapchain(self);
        const Present1Fn original = hooks ? hooks->Original<Present1Fn>(22) : nullptr;
        if (!original) return E_FAIL;

        const HRESULT result = original(self, syncInterval, flags, presentParameters);
        if (FAILED(result) &&
            !(flags & DXGI_PRESENT_DO_NOT_WAIT &&
                result == DXGI_ERROR_WAS_STILL_DRAWING)) {
            overlay::RunOptionalOverlayWork([&]() {
                LogPresentFailure(static_cast<IDXGISwapChain*>(self), 0,
                    result, syncInterval, flags, 0, 0, 0,
                    overlay::AssociationState::Unknown,
                    overlay::AssociationState::Unknown, false, false, "present1");
            }, &DisableAfterOverlayException);
        }
        return result;
    }

    bool BeginResizeObservation(const overlay::DxgiHookRegistry::Lease& hooks,
        IDXGISwapChain* self, std::uintptr_t identity,
        overlay::DxgiResizeNesting& nesting, UINT count, UINT width, UINT height,
        DXGI_FORMAT format, UINT flags)
    {
        if (!nesting.Begin(identity)) {
            DisableAfterOverlayException();
            return false;
        }
        if (!nesting.outermost()) return true;
#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
#if !defined(OVERLAY_NO_GPU_SUBMISSION_DIAGNOSTIC)
        {
            std::scoped_lock rendererLock{g_rendererMutex};
            g_renderer.BeforeResize(self);
        }
#endif
#endif
        if (!hooks) return true;
        std::uint32_t sequence{};
        overlay::AssociationState association{};
        std::uintptr_t device{}, queue{};
        {
            std::scoped_lock lock{g_mutex};
            sequence = ++hooks->resizeObservations;
            hooks->presentObservations = 0;
            association = g_evidence.State(identity);
            g_evidence.BeginResize(identity);
            g_presentWindow.Arm(16);
            g_lifecycle.BeginResize();
            g_evidence.GetSingleCandidate(identity, device, queue);
        }
        LogDiagnostic(std::string("OVERLAY_RESIZE_BEGIN sequence=") +
            std::to_string(sequence) + " width=" + std::to_string(width) +
            " height=" + std::to_string(height) + " buffers=" +
            std::to_string(count) + " format=" +
            std::to_string(static_cast<unsigned>(format)) + " flags=" +
            std::to_string(flags));
        LogDiagnostic(std::string("OVERLAY_RESIZE_ASSOCIATION before=") +
            AssociationStateName(association) + " device=" +
            Hex(reinterpret_cast<void*>(device)) + " queue=" +
            Hex(reinterpret_cast<void*>(queue)));
        return true;
    }

#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
    bool RefreshPendingTargetAfterResize(std::uintptr_t identity,
        IDXGISwapChain* swapchain, bool success, UINT queueCount,
        IUnknown* const* presentQueues);
#endif

    void CompleteResizeObservation(const overlay::DxgiHookRegistry::Lease& hooks,
        IDXGISwapChain* self, std::uintptr_t identity,
        overlay::DxgiResizeNesting& nesting, HRESULT result,
        double nativeMs, double totalMs, bool resizeBuffers1,
        UINT queueCount = 0, IUnknown* const* presentQueues = nullptr)
    {
        if (!nesting.End()) return;
        if (!hooks) return;
        const bool succeeded = SUCCEEDED(result);
        {
            std::scoped_lock lock{g_mutex};
            g_evidence.CompleteResize(identity, succeeded);
            if (succeeded) g_lifecycle.MarkReady();
            else g_lifecycle.CompleteResize(false);
        }
#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
#if !defined(OVERLAY_NO_GPU_SUBMISSION_DIAGNOSTIC)
        {
            std::scoped_lock rendererLock{g_rendererMutex};
            if (resizeBuffers1)
                g_renderer.OnResizeResult(self, succeeded, nativeMs, totalMs,
                    queueCount, presentQueues);
            else
                g_renderer.OnResizeResult(self, succeeded, nativeMs, totalMs);
            if (g_renderer.disabled() || !RefreshPendingTargetAfterResize(
                    identity, self, succeeded, queueCount, presentQueues)) {
                std::scoped_lock lock{g_mutex};
                g_evidence.InvalidateSwapchain(identity);
            }
        }
#endif
#else
        (void)queueCount;
        (void)presentQueues;
#endif
        if (FAILED(result))
            Log(std::string(resizeBuffers1 ? "OVERLAY_RESIZE1_FAILED result=" :
                "OVERLAY_RESIZE_FAILED result=") + std::to_string(result));
        LogDiagnostic(std::string(resizeBuffers1 ? "OVERLAY_RESIZE1_END result=" :
            "OVERLAY_RESIZE_END result=") + std::to_string(result) +
            " nativeMs=" + std::to_string(nativeMs) +
            " hookMs=" + std::to_string(totalMs));
        std::string state;
        {
            std::scoped_lock lock{g_mutex};
            state = std::string(overlay::StateName(g_lifecycle.state())) +
                " association=" + AssociationStateName(g_evidence.State(identity));
        }
        LogDiagnostic("OVERLAY_RESIZE_STATE lifecycle=" + state);
    }

    HRESULT STDMETHODCALLTYPE HookResizeBuffers(IDXGISwapChain* self, UINT count,
        UINT width, UINT height, DXGI_FORMAT format, UINT flags)
    {
        const auto hooks = FindSwapchain(self);
        const auto original = hooks ? hooks->Original<ResizeBuffersFn>(13) : nullptr;
        if (!original) return E_FAIL;
        const auto start = std::chrono::steady_clock::now();
        overlay::DxgiResizeNesting nesting;
        std::uintptr_t identity{};
        overlay::RunOptionalOverlayWork([&]() {
            identity = SwapchainIdentity(self);
            BeginResizeObservation(hooks, self, identity, nesting,
                count, width, height, format, flags);
        }, &DisableAfterOverlayException);
        RestoreInputAfterTerminalFailure();
        const auto nativeStart = std::chrono::steady_clock::now();
        const HRESULT result = original(self, count, width, height, format, flags);
        const auto nativeMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - nativeStart).count();
        const auto totalMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        overlay::RunOptionalOverlayWork([&]() {
            CompleteResizeObservation(hooks, self, identity, nesting, result,
                nativeMs, totalMs, false);
        }, &DisableAfterOverlayException);
        RestoreInputAfterTerminalFailure();
        return result;
    }

    HRESULT STDMETHODCALLTYPE HookResizeBuffers1(IDXGISwapChain3* self, UINT count,
        UINT width, UINT height, DXGI_FORMAT format, UINT flags,
        const UINT* creationNodeMask, IUnknown* const* presentQueues)
    {
        const auto hooks = FindSwapchain(self);
        const auto original = hooks ? hooks->Original<ResizeBuffers1Fn>(39) : nullptr;
        if (!original) return E_FAIL;
        const auto start = std::chrono::steady_clock::now();
        overlay::DxgiResizeNesting nesting;
        std::uintptr_t identity{};
        overlay::RunOptionalOverlayWork([&]() {
            identity = SwapchainIdentity(self);
            BeginResizeObservation(hooks, self, identity, nesting,
                count, width, height, format, flags);
        }, &DisableAfterOverlayException);
        RestoreInputAfterTerminalFailure();
        const auto nativeStart = std::chrono::steady_clock::now();
        const HRESULT result = original(self, count, width, height, format, flags,
            creationNodeMask, presentQueues);
        const auto nativeMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - nativeStart).count();
        const auto totalMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        overlay::RunOptionalOverlayWork([&]() {
            CompleteResizeObservation(hooks, self, identity, nesting, result,
                nativeMs, totalMs, true, count, presentQueues);
        }, &DisableAfterOverlayException);
        RestoreInputAfterTerminalFailure();
        return result;
    }

#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
    void TryActivateRenderer(IDXGISwapChain* presentedSwapchain,
        std::uintptr_t identity)
    {
#if !defined(OVERLAY_NO_GPU_SUBMISSION_DIAGNOSTIC)
        if (!g_cameraCoreReady.load(std::memory_order_acquire)) return;
        PendingRendererTarget target{};
        {
            std::scoped_lock rendererLock{g_rendererMutex};
            if (!g_pendingRendererTarget ||
                g_pendingRendererTarget->identity != identity) return;
            target = *g_pendingRendererTarget;
        }
        {
            std::scoped_lock lock{g_mutex};
            if (!g_evidence.HasStableAssociation(identity, 2)) return;
        }
        if (!target.swapchain || !target.device || !target.queue) return;
        Microsoft::WRL::ComPtr<IUnknown> currentIdentity;
        Microsoft::WRL::ComPtr<IUnknown> targetIdentity;
        if (!presentedSwapchain ||
            FAILED(presentedSwapchain->QueryInterface(IID_PPV_ARGS(&currentIdentity))) ||
            FAILED(target.swapchain->QueryInterface(IID_PPV_ARGS(&targetIdentity))) ||
            currentIdentity.Get() != targetIdentity.Get()) return;

        std::scoped_lock rendererLock{g_rendererMutex};
        if (!g_pendingRendererTarget ||
            g_pendingRendererTarget->identity != identity) return;
        if (g_renderer.ready() && g_renderer.OwnsSwapchain(presentedSwapchain)) {
            g_pendingRendererTarget.reset();
            return;
        }
        if (g_renderer.ready()) {
            if (!g_renderer.OwnsWindow(target.window)) return;
            g_renderer.BeforeSwapchainReplacement(target.window);
            if (g_renderer.disabled()) return;
        }
        if (g_renderer.Initialize(target.swapchain.Get(), target.device.Get(),
            target.queue.Get(), target.window, target.bufferCount, target.format)) {
#if defined(OVERLAY_STARTUP_JOURNAL)
            diagnostics::startup_journal::MarkOnce(7, "RENDERER_ACTIVATED",
                "validated_target_after_two_successful_presents", identity);
            diagnostics::startup_journal::Flush();
#endif
            g_renderer.SetVisible(overlay::GetInputState().visible());
            InstallInputHook(target.window);
            g_pendingRendererTarget.reset();
            Log("OVERLAY_RENDER_TARGET_ACTIVATED reason=two_successful_presents");
        }
#else
        (void)presentedSwapchain;
        (void)identity;
#endif
    }
#endif

    bool IsEligibleSwapchainTarget(IDXGISwapChain* swapchain,
        IUnknown* creationDevice, HWND window)
    {
        if (!swapchain || !creationDevice || !IsCurrentProcessWindow(window))
            return false;
        DXGI_SWAP_CHAIN_DESC desc{};
        if (FAILED(swapchain->GetDesc(&desc)) ||
            !desc.BufferDesc.Width || !desc.BufferDesc.Height ||
            !desc.BufferCount || desc.BufferCount > 16)
            return false;

        Microsoft::WRL::ComPtr<ID3D12Device> swapchainDevice;
        if (FAILED(swapchain->GetDevice(__uuidof(ID3D12Device),
                reinterpret_cast<void**>(swapchainDevice.GetAddressOf()))))
            return false;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
        if (FAILED(creationDevice->QueryInterface(__uuidof(ID3D12CommandQueue),
                reinterpret_cast<void**>(queue.GetAddressOf()))) || !queue ||
            queue->GetDesc().Type != D3D12_COMMAND_LIST_TYPE_DIRECT)
            return false;
        Microsoft::WRL::ComPtr<ID3D12Device> queueDevice;
        if (FAILED(queue->GetDevice(IID_PPV_ARGS(&queueDevice)))) return false;
        return SameComIdentity(swapchainDevice.Get(), queueDevice.Get());
    }

#if defined(OVERLAY_PRODUCTION) || defined(OVERLAY_RENDERING_POC)
    bool RefreshPendingTargetAfterResize(std::uintptr_t identity,
        IDXGISwapChain* swapchain, bool success, UINT queueCount,
        IUnknown* const* presentQueues)
    {
        if (!g_pendingRendererTarget || g_pendingRendererTarget->identity != identity)
            return true;
        if (!success) {
            g_pendingRendererTarget.reset();
            return false;
        }
        DXGI_SWAP_CHAIN_DESC desc{};
        if (!swapchain || FAILED(swapchain->GetDesc(&desc)) ||
            !desc.BufferCount || desc.BufferCount > 16 ||
            !desc.BufferDesc.Width || !desc.BufferDesc.Height) {
            g_pendingRendererTarget.reset();
            return false;
        }
        if (presentQueues) {
            if (!queueCount || queueCount != desc.BufferCount) {
                g_pendingRendererTarget.reset();
                return false;
            }
            Microsoft::WRL::ComPtr<IUnknown> selectedIdentity;
            Microsoft::WRL::ComPtr<ID3D12Device> selectedDevice;
            Microsoft::WRL::ComPtr<ID3D12CommandQueue> selectedQueue;
            if (!presentQueues[0] ||
                FAILED(presentQueues[0]->QueryInterface(IID_PPV_ARGS(&selectedQueue))) ||
                !selectedQueue || selectedQueue->GetDesc().Type != D3D12_COMMAND_LIST_TYPE_DIRECT ||
                FAILED(selectedQueue->QueryInterface(IID_PPV_ARGS(&selectedIdentity))) ||
                FAILED(selectedQueue->GetDevice(IID_PPV_ARGS(&selectedDevice))) ||
                !SameComIdentity(g_pendingRendererTarget->device.Get(), selectedDevice.Get())) {
                g_pendingRendererTarget.reset();
                return false;
            }
            for (UINT index = 1; index < queueCount; ++index) {
                Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
                Microsoft::WRL::ComPtr<IUnknown> queueIdentity;
                Microsoft::WRL::ComPtr<ID3D12Device> queueDevice;
                if (!presentQueues[index] ||
                    FAILED(presentQueues[index]->QueryInterface(IID_PPV_ARGS(&queue))) ||
                    !queue || queue->GetDesc().Type != D3D12_COMMAND_LIST_TYPE_DIRECT ||
                    FAILED(queue->QueryInterface(IID_PPV_ARGS(&queueIdentity))) ||
                    queueIdentity.Get() != selectedIdentity.Get() ||
                    FAILED(queue->GetDevice(IID_PPV_ARGS(&queueDevice))) ||
                    !SameComIdentity(selectedDevice.Get(), queueDevice.Get())) {
                    g_pendingRendererTarget.reset();
                    return false;
                }
            }
            g_pendingRendererTarget->queue = std::move(selectedQueue);
        }
        g_pendingRendererTarget->bufferCount = desc.BufferCount;
        g_pendingRendererTarget->format = desc.BufferDesc.Format;
        return true;
    }
#endif

    void InstallSwapchainHooks(IDXGISwapChain* swapchain, IUnknown* creationDevice,
        const char* source, HWND window, UINT width, UINT height, UINT buffers)
    {
        if (!swapchain) return;
        if (!IsEligibleSwapchainTarget(swapchain, creationDevice, window)) {
            Log("SWAPCHAIN_HOOK_SKIPPED reason=target_not_validated");
            return;
        }
        Microsoft::WRL::ComPtr<IUnknown> identity;
        if (FAILED(swapchain->QueryInterface(IID_PPV_ARGS(&identity)))) return;
        const auto identityValue = reinterpret_cast<std::uintptr_t>(identity.Get());
        if (overlay::FactoryCreationScope::AlreadyObserved(identityValue)) {
            LogFactoryTrace("SWAPCHAIN_CREATION_DEDUPLICATED reason=nested_same_iunknown_identity");
            return;
        }
        for (const auto& extent : overlay::SwapchainInterfaces) {
            Microsoft::WRL::ComPtr<IUnknown> pointer;
            if (FAILED(swapchain->QueryInterface(*extent.iid,
                    reinterpret_cast<void**>(pointer.GetAddressOf())))) continue;
            if (extent.methods < 18) continue;
            g_swapchainHooks.Install(pointer.Get(), extent.methods,
                [&](overlay::DxgiHookRecord& record) {
                    if (extent.methods >= 18) {
                        record.replacement[8] = reinterpret_cast<void*>(&HookPresent);
                        record.replacement[13] = reinterpret_cast<void*>(&HookResizeBuffers);
                    }
                    if (extent.methods >= 23)
                        record.replacement[22] = reinterpret_cast<void*>(&HookPresent1);
                    if (extent.methods >= 40)
                        record.replacement[39] = reinterpret_cast<void*>(&HookResizeBuffers1);
                });
        }
        const auto installed = FindSwapchain(swapchain);
        if (!installed || !installed->active.load(std::memory_order_acquire)) {
            Log("SWAPCHAIN_HOOK_FAILED reason=unrecognized_interface");
            return;
        }
        {
            std::scoped_lock lock{g_mutex};
            g_evidence.BeginSwapchain(reinterpret_cast<std::uintptr_t>(identity.Get()));
            g_presentWindow.Arm(32);
            g_lifecycle.MarkReady();
        }
        TraceSwapchainIdentity(swapchain, creationDevice, source, window, width, height, buffers);
    }

    HRESULT STDMETHODCALLTYPE HookCreateSwapChain(IDXGIFactory* self, IUnknown* device,
        DXGI_SWAP_CHAIN_DESC* desc, IDXGISwapChain** outSwapchain)
    {
        overlay::FactoryCreationScope creationScope;
#if defined(OVERLAY_FACTORY_CALLER_TRACE)
        const auto caller = _ReturnAddress();
#endif
        const auto hooks = FindFactory(self);
        const auto original = hooks ? hooks->Original<CreateSwapChainFn>(10) : nullptr;
        if (!original) return E_FAIL;
#if defined(OVERLAY_STARTUP_JOURNAL)
        MarkRealFactoryObserved("shared_factory_table", self);
#endif
        const HWND window = desc ? desc->OutputWindow : nullptr;
        overlay::RunOptionalOverlayWork([&]() {
            PrepareForSwapchainReplacement(window);
        }, &DisableAfterOverlayException);
        const HRESULT result = original(self, device, desc, outSwapchain);
#if defined(OVERLAY_STARTUP_JOURNAL)
        if (SUCCEEDED(result) && outSwapchain && *outSwapchain)
            MarkFirstSwapchainCreated("CreateSwapChain", *outSwapchain);
#endif
        overlay::RunOptionalOverlayWork([&]() {
            CompleteSwapchainCreation(window, result);
            TraceFactoryMethod(overlay::FactoryMethod::CreateSwapChain, result,
                (SUCCEEDED(result) && outSwapchain) ? *outSwapchain : nullptr, self,
#if defined(OVERLAY_FACTORY_CALLER_TRACE)
                caller
#else
                nullptr
#endif
                );
            if (SUCCEEDED(result) && outSwapchain && *outSwapchain)
                InstallSwapchainHooks(*outSwapchain, device, "CreateSwapChain",
                    window, desc ? desc->BufferDesc.Width : 0,
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
        overlay::FactoryCreationScope creationScope;
#if defined(OVERLAY_FACTORY_CALLER_TRACE)
        const auto caller = _ReturnAddress();
#endif
        const auto hooks = FindFactory(self);
        const auto original = hooks ? hooks->Original<CreateSwapChainForHwndFn>(15) : nullptr;
        if (!original) return E_FAIL;
#if defined(OVERLAY_STARTUP_JOURNAL)
        MarkRealFactoryObserved("shared_factory_table", self);
#endif
        overlay::RunOptionalOverlayWork([&]() {
            PrepareForSwapchainReplacement(window);
        }, &DisableAfterOverlayException);
        const HRESULT result = original(self, device, window, desc, fullscreen, output, outSwapchain);
#if defined(OVERLAY_STARTUP_JOURNAL)
        if (SUCCEEDED(result) && outSwapchain && *outSwapchain)
            MarkFirstSwapchainCreated("CreateSwapChainForHwnd", *outSwapchain);
#endif
        overlay::RunOptionalOverlayWork([&]() {
            CompleteSwapchainCreation(window, result);
            TraceFactoryMethod(overlay::FactoryMethod::CreateSwapChainForHwnd, result,
                (SUCCEEDED(result) && outSwapchain) ? *outSwapchain : nullptr, self,
#if defined(OVERLAY_FACTORY_CALLER_TRACE)
                caller
#else
                nullptr
#endif
                );
            if (SUCCEEDED(result) && outSwapchain && *outSwapchain)
                InstallSwapchainHooks(*outSwapchain, device, "CreateSwapChainForHwnd", window,
                    desc ? desc->Width : 0, desc ? desc->Height : 0, desc ? desc->BufferCount : 0);
        }, &DisableAfterOverlayException);
        RestoreInputAfterTerminalFailure();
        return result;
    }

    HRESULT STDMETHODCALLTYPE HookCreateSwapChainForCoreWindow(IDXGIFactory2* self,
        IUnknown* device, IUnknown* window, const DXGI_SWAP_CHAIN_DESC1* desc,
        IDXGIOutput* output, IDXGISwapChain1** outSwapchain)
    {
        overlay::FactoryCreationScope creationScope;
#if defined(OVERLAY_FACTORY_CALLER_TRACE)
        const auto caller = _ReturnAddress();
#endif
        const auto hooks = FindFactory(self);
        const auto original = hooks ? hooks->Original<CreateSwapChainForCoreWindowFn>(16) : nullptr;
        if (!original) return E_FAIL;
#if defined(OVERLAY_STARTUP_JOURNAL)
        MarkRealFactoryObserved("shared_factory_table", self);
#endif
        const HRESULT result = original(self, device, window, desc, output, outSwapchain);
#if defined(OVERLAY_STARTUP_JOURNAL)
        if (SUCCEEDED(result) && outSwapchain && *outSwapchain)
            MarkFirstSwapchainCreated("CreateSwapChainForCoreWindow", *outSwapchain);
#endif
        overlay::RunOptionalOverlayWork([&]() {
            TraceFactoryMethod(overlay::FactoryMethod::CreateSwapChainForCoreWindow, result,
                (SUCCEEDED(result) && outSwapchain) ? *outSwapchain : nullptr, self,
#if defined(OVERLAY_FACTORY_CALLER_TRACE)
                caller
#else
                nullptr
#endif
                );
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
        overlay::FactoryCreationScope creationScope;
#if defined(OVERLAY_FACTORY_CALLER_TRACE)
        const auto caller = _ReturnAddress();
#endif
        const auto hooks = FindFactory(self);
        const auto original = hooks ? hooks->Original<CreateSwapChainForCompositionFn>(24) : nullptr;
        if (!original) return E_FAIL;
#if defined(OVERLAY_STARTUP_JOURNAL)
        MarkRealFactoryObserved("shared_factory_table", self);
#endif
        const HRESULT result = original(self, device, desc, output, outSwapchain);
#if defined(OVERLAY_STARTUP_JOURNAL)
        if (SUCCEEDED(result) && outSwapchain && *outSwapchain)
            MarkFirstSwapchainCreated("CreateSwapChainForComposition", *outSwapchain);
#endif
        overlay::RunOptionalOverlayWork([&]() {
            TraceFactoryMethod(overlay::FactoryMethod::CreateSwapChainForComposition, result,
                (SUCCEEDED(result) && outSwapchain) ? *outSwapchain : nullptr, self,
#if defined(OVERLAY_FACTORY_CALLER_TRACE)
                caller
#else
                nullptr
#endif
                );
            if (SUCCEEDED(result) && outSwapchain && *outSwapchain)
                InstallSwapchainHooks(*outSwapchain, device, "CreateSwapChainForComposition",
                    nullptr, desc ? desc->Width : 0, desc ? desc->Height : 0,
                    desc ? desc->BufferCount : 0);
        }, &DisableAfterOverlayException);
        return result;
    }

    bool InstallFactoryHooks(IUnknown* factory, std::size_t methodCount,
        const char* interfaceName)
    {
        if (methodCount < 12) {
            LogFactoryTrace(std::string("FACTORY_HOOK_SKIPPED interface=") + interfaceName +
                " reason=interface_has_no_swapchain_methods");
            return false;
        }
        overlay::DxgiHookInstallStatus installStatus{};
        const bool installed = g_factoryHooks.Install(factory, methodCount,
            [methodCount](overlay::DxgiHookRecord& record) {
                if (methodCount >= 12)
                    record.replacement[10] = reinterpret_cast<void*>(&HookCreateSwapChain);
                if (methodCount >= 25) {
                    record.replacement[15] = reinterpret_cast<void*>(&HookCreateSwapChainForHwnd);
                    record.replacement[16] = reinterpret_cast<void*>(&HookCreateSwapChainForCoreWindow);
                    record.replacement[24] = reinterpret_cast<void*>(&HookCreateSwapChainForComposition);
                }
            }, &installStatus);
        if (!installed) {
            const auto existing = FindFactory(factory);
            if (existing && existing->active.load(std::memory_order_acquire)) {
                std::string message = std::string("FACTORY_HOOK_REUSED interface=") + interfaceName +
                    " object=" + Hex(factory) + " reason=shared_vtable_already_hooked";
#if defined(OVERLAY_FACTORY_CALLER_TRACE)
                message += " vtable=" + Hex(existing->table);
                const auto appendSlot = [&](std::size_t slot, void* expected) {
                    void* current{};
                    SIZE_T copied{};
                    const bool readable = ReadProcessMemory(GetCurrentProcess(),
                        existing->table + slot, &current, sizeof(current), &copied) &&
                        copied == sizeof(current);
                    message += " slot" + std::to_string(slot) + "=" +
                        (readable ? Hex(current) : "unreadable") +
                        " expected" + std::to_string(slot) + "=" + Hex(expected);
                };
                if (existing->replacement.size() > 10)
                    appendSlot(10, reinterpret_cast<void*>(&HookCreateSwapChain));
                if (existing->replacement.size() > 24) {
                    appendSlot(15, reinterpret_cast<void*>(&HookCreateSwapChainForHwnd));
                    appendSlot(16, reinterpret_cast<void*>(&HookCreateSwapChainForCoreWindow));
                    appendSlot(24, reinterpret_cast<void*>(&HookCreateSwapChainForComposition));
                }
#endif
                LogFactoryTrace(message);
                return existing->original.size() >= 25;
            }
            Log(std::string("FACTORY_HOOK_FAILED interface=") + interfaceName +
                " object=" + Hex(factory) +
                " reason=" + HookInstallStatusName(installStatus));
            return false;
        }
        {
            std::scoped_lock lock{g_mutex};
            g_factoryEvidence.ObserveFactory(reinterpret_cast<std::uintptr_t>(factory));
            g_factoryEvidence.RecordMethod(reinterpret_cast<std::uintptr_t>(factory),
                overlay::FactoryMethod::CreateSwapChain);
            if (methodCount >= 25) {
                g_factoryEvidence.RecordMethod(reinterpret_cast<std::uintptr_t>(factory),
                    overlay::FactoryMethod::CreateSwapChainForHwnd);
                g_factoryEvidence.RecordMethod(reinterpret_cast<std::uintptr_t>(factory),
                    overlay::FactoryMethod::CreateSwapChainForCoreWindow);
                g_factoryEvidence.RecordMethod(reinterpret_cast<std::uintptr_t>(factory),
                    overlay::FactoryMethod::CreateSwapChainForComposition);
            }
        }
        LogFactoryTrace(std::string("FACTORY_HOOKED interface=") + interfaceName +
            " factory=" + Hex(factory) +
            " tableMethods=" + std::to_string(methodCount));
#if defined(OVERLAY_FACTORY_CALLER_TRACE)
        const auto installedRecord = FindFactory(factory);
        if (installedRecord && installedRecord->active.load(std::memory_order_acquire)) {
            std::string message = std::string("FACTORY_HOOK_TABLE interface=") + interfaceName +
                " factory=" + Hex(factory) + " vtable=" + Hex(installedRecord->table);
            const auto appendSlot = [&](std::size_t slot, void* expected) {
                void* current{};
                SIZE_T copied{};
                const bool readable = ReadProcessMemory(GetCurrentProcess(),
                    installedRecord->table + slot, &current, sizeof(current), &copied) &&
                    copied == sizeof(current);
                message += " slot" + std::to_string(slot) + "=" +
                    (readable ? Hex(current) : "unreadable") +
                    " expected" + std::to_string(slot) + "=" + Hex(expected);
            };
            if (methodCount > 10)
                appendSlot(10, reinterpret_cast<void*>(&HookCreateSwapChain));
            if (methodCount > 24) {
                appendSlot(15, reinterpret_cast<void*>(&HookCreateSwapChainForHwnd));
                appendSlot(16, reinterpret_cast<void*>(&HookCreateSwapChainForCoreWindow));
                appendSlot(24, reinterpret_cast<void*>(&HookCreateSwapChainForComposition));
            }
            LogFactoryTrace(message);
        }
#endif
        return methodCount >= 25;
    }

    bool PinImageCodeOwner(const void* address)
    {
        if (!address) return false;
        MEMORY_BASIC_INFORMATION region{};
        if (VirtualQuery(address, &region, sizeof(region)) != sizeof(region) ||
            region.Type != MEM_IMAGE ||
            !(region.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ |
                PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
            return false;
        HMODULE owner{};
        return GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(address), &owner) != FALSE && owner != nullptr;
    }

    bool PinFactoryTableOwner(IUnknown* factory, std::size_t methodCount)
    {
        void** table{};
        SIZE_T copied{};
        if (!factory || methodCount < 25 ||
            !ReadProcessMemory(GetCurrentProcess(), factory, &table,
                sizeof(table), &copied) || copied != sizeof(table) || !table)
            return false;

        MEMORY_BASIC_INFORMATION region{};
        if (VirtualQuery(table, &region, sizeof(region)) != sizeof(region) ||
            region.Type != MEM_IMAGE)
            return false;
        const auto begin = reinterpret_cast<std::uintptr_t>(table);
        const auto regionBegin = reinterpret_cast<std::uintptr_t>(region.BaseAddress);
        if (begin < regionBegin || begin > regionBegin + region.RegionSize ||
            methodCount * sizeof(void*) > regionBegin + region.RegionSize - begin)
            return false;

        HMODULE tableOwner{};
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_PIN,
                reinterpret_cast<LPCWSTR>(table), &tableOwner) ||
            !tableOwner)
            return false;

        constexpr std::size_t creationSlots[]{10, 15, 16, 24};
        for (const auto slot : creationSlots) {
            if (slot >= methodCount) continue;
            void* method{};
            if (!ReadProcessMemory(GetCurrentProcess(), table + slot,
                    &method, sizeof(method), &copied) ||
                copied != sizeof(method) || !PinImageCodeOwner(method))
                return false;
        }
        return true;
    }

    bool InstallFactoryInterfaces(IUnknown* factory, const char* origin,
        bool requirePinnedSharedTable)
    {
        if (!factory) return false;
        bool modernCoverage = false;
        for (const auto& candidate : overlay::FactoryInterfaces) {
            if (candidate.methods < 12) continue;
            if (requirePinnedSharedTable && candidate.methods < 25) continue;
            Microsoft::WRL::ComPtr<IUnknown> queried;
            if (FAILED(factory->QueryInterface(*candidate.iid,
                    reinterpret_cast<void**>(queried.GetAddressOf()))) || !queried)
                continue;
            if (requirePinnedSharedTable &&
                !PinFactoryTableOwner(queried.Get(), candidate.methods)) {
                Log("FACTORY_BOOTSTRAP_REJECTED reason=factory_table_not_image_owned");
                continue;
            }
            const auto name = std::string(origin) + "/" + FactoryInterfaceName(*candidate.iid);
            const bool installed = InstallFactoryHooks(queried.Get(),
                candidate.methods, name.c_str());
            modernCoverage |= candidate.methods >= 25 && installed;
        }
        return modernCoverage;
    }

    bool InstallBootstrapFactoryTable(IUnknown* seedFactory)
    {
        const bool installed = InstallFactoryInterfaces(seedFactory,
            "bootstrap", true);
        Log(installed
            ? "FACTORY_BOOTSTRAP_READY coverage=pinned_shared_factory2_table"
            : "FACTORY_BOOTSTRAP_FAILED reason=no_pinned_shared_factory2_table");
#if defined(OVERLAY_STARTUP_JOURNAL)
        diagnostics::startup_journal::Mark(
            installed ? "FACTORY_BOOTSTRAP_READY" : "FACTORY_BOOTSTRAP_UNAVAILABLE",
            installed ? "pinned_shared_factory2_table" : "no_pinned_shared_factory2_table");
#endif
        return installed;
    }

    void ObserveFactoryResult(HRESULT result, REFIID riid, void** output,
        const char* exportName, const void* caller)
    {
#if !defined(OVERLAY_FACTORY_CALLER_TRACE)
        (void)caller;
#endif
#if defined(OVERLAY_FACTORY_CALLER_TRACE)
        std::string returnMessage = std::string("FACTORY_EXPORT_RETURN export=") + exportName +
            " result=" + std::to_string(result) +
            " object=" + ((output && SUCCEEDED(result)) ? Hex(*output) : "null") +
            " interface=" + FactoryInterfaceName(riid)
            + CallerModuleEvidence(caller)
            ;
        LogFactoryTrace(returnMessage);
#else
        (void)exportName;
        (void)caller;
#endif
        if (FAILED(result) || !output || !*output) return;
        const auto extent = std::find_if(std::begin(overlay::FactoryInterfaces),
            std::end(overlay::FactoryInterfaces),
            [&riid](const overlay::DxgiInterfaceExtent& candidate) {
                return *candidate.iid == riid;
            });
        if (extent == std::end(overlay::FactoryInterfaces)) {
            LogFactoryTrace("FACTORY_IGNORED reason=requested_interface_not_dxgi_factory");
            return;
        }
        const auto object = reinterpret_cast<std::uintptr_t>(*output);
        bool firstObservation = false;
        {
            std::scoped_lock lock{g_mutex};
            firstObservation = g_factoryEvidence.ObserveFactory(object);
        }
        if (firstObservation) {
            LogFactoryTrace(std::string("FACTORY_OBSERVED result=") + std::to_string(result) +
                " object=" + Hex(*output) + " interface=" + FactoryInterfaceName(riid));
#if defined(OVERLAY_STARTUP_JOURNAL)
            MarkRealFactoryObserved(FactoryInterfaceName(riid), *output, true);
#endif
        }
        InstallFactoryInterfaces(reinterpret_cast<IUnknown*>(*output),
            "export", false);
    }

    bool BootstrapFactoryObservation()
    {
#if defined(OVERLAY_STARTUP_JOURNAL)
        diagnostics::startup_journal::Mark("FACTORY_BOOTSTRAP_BEGIN", "CreateDXGIFactory1");
#endif
        auto create = g_createFactory1.original<CreateFactoryFn>();
        if (!create && g_systemDxgiModule) {
            create = reinterpret_cast<CreateFactoryFn>(
                GetProcAddress(g_systemDxgiModule, "CreateDXGIFactory1"));
        }
        if (!create) {
            Log("FACTORY_BOOTSTRAP_FAILED reason=factory1_entry_unavailable");
#if defined(OVERLAY_STARTUP_JOURNAL)
            diagnostics::startup_journal::Mark("FACTORY_BOOTSTRAP_UNAVAILABLE",
                "factory1_entry_unavailable");
#endif
            return false;
        }

        void* output{};
        HRESULT result{};
        {
            FactoryBootstrapScope scope;
            result = create(__uuidof(IDXGIFactory2), &output);
        }
        if (FAILED(result) || !output) {
            Log("FACTORY_BOOTSTRAP_FAILED reason=seed_factory_creation_failed");
#if defined(OVERLAY_STARTUP_JOURNAL)
            diagnostics::startup_journal::Mark("FACTORY_BOOTSTRAP_UNAVAILABLE",
                "seed_factory_creation_failed", static_cast<std::uint64_t>(result));
#endif
            return false;
        }

        Microsoft::WRL::ComPtr<IUnknown> seedFactory;
        seedFactory.Attach(reinterpret_cast<IUnknown*>(output));
        return InstallBootstrapFactoryTable(seedFactory.Get());
    }

    HRESULT WINAPI HookCreateFactory(REFIID riid, void** output)
    {
#if defined(OVERLAY_STARTUP_TIMELINE)
        const bool firstTimelineCall = diagnostics::startup::FactoryEnter(0, _ReturnAddress());
#endif
#if defined(OVERLAY_FACTORY_CALLER_TRACE)
        const auto caller = _ReturnAddress();
#endif
        const auto original = g_createFactory.original<CreateFactoryFn>();
        const HRESULT result = original ? original(riid, output) : E_FAIL;
#if defined(OVERLAY_STARTUP_TIMELINE)
        if (firstTimelineCall) diagnostics::startup::FactoryReturn(0, result,
            output && SUCCEEDED(result) ? *output : nullptr);
#endif
        if (g_factoryBootstrapDepth == 0) overlay::RunOptionalOverlayWork([&]() { ObserveFactoryResult(result, riid, output,
            "CreateDXGIFactory",
#if defined(OVERLAY_FACTORY_CALLER_TRACE)
            caller
#else
            nullptr
#endif
            ); },
            &DisableAfterOverlayException);
        RestoreInputAfterTerminalFailure();
        return result;
    }

    static_assert(std::is_same_v<decltype(&HookCreateSwapChain), CreateSwapChainFn>);
    static_assert(std::is_same_v<decltype(&HookCreateSwapChainForHwnd), CreateSwapChainForHwndFn>);
    static_assert(std::is_same_v<decltype(&HookCreateSwapChainForCoreWindow), CreateSwapChainForCoreWindowFn>);
    static_assert(std::is_same_v<decltype(&HookCreateSwapChainForComposition), CreateSwapChainForCompositionFn>);
    static_assert(std::is_same_v<decltype(&HookPresent), PresentFn>);
    static_assert(std::is_same_v<decltype(&HookPresent1), Present1Fn>);
    static_assert(std::is_same_v<decltype(&HookResizeBuffers), ResizeBuffersFn>);
    static_assert(std::is_same_v<decltype(&HookResizeBuffers1), ResizeBuffers1Fn>);

    HRESULT WINAPI HookCreateFactory1(REFIID riid, void** output)
    {
#if defined(OVERLAY_STARTUP_TIMELINE)
        const bool firstTimelineCall = diagnostics::startup::FactoryEnter(1, _ReturnAddress());
#endif
#if defined(OVERLAY_FACTORY_CALLER_TRACE)
        const auto caller = _ReturnAddress();
#endif
        const auto original = g_createFactory1.original<CreateFactoryFn>();
        const HRESULT result = original ? original(riid, output) : E_FAIL;
#if defined(OVERLAY_STARTUP_TIMELINE)
        if (firstTimelineCall) diagnostics::startup::FactoryReturn(1, result,
            output && SUCCEEDED(result) ? *output : nullptr);
#endif
        if (g_factoryBootstrapDepth == 0) overlay::RunOptionalOverlayWork([&]() { ObserveFactoryResult(result, riid, output,
            "CreateDXGIFactory1",
#if defined(OVERLAY_FACTORY_CALLER_TRACE)
            caller
#else
            nullptr
#endif
            ); },
            &DisableAfterOverlayException);
        RestoreInputAfterTerminalFailure();
        return result;
    }

    HRESULT WINAPI HookCreateFactory2(UINT flags, REFIID riid, void** output)
    {
#if defined(OVERLAY_STARTUP_TIMELINE)
        const bool firstTimelineCall = diagnostics::startup::FactoryEnter(2, _ReturnAddress());
#endif
#if defined(OVERLAY_FACTORY_CALLER_TRACE)
        const auto caller = _ReturnAddress();
#endif
        using Function = HRESULT(WINAPI*)(UINT, REFIID, void**);
        const auto original = g_createFactory2.original<Function>();
        const HRESULT result = original ? original(flags, riid, output) : E_FAIL;
#if defined(OVERLAY_STARTUP_TIMELINE)
        if (firstTimelineCall) diagnostics::startup::FactoryReturn(2, result,
            output && SUCCEEDED(result) ? *output : nullptr);
#endif
        if (g_factoryBootstrapDepth == 0) overlay::RunOptionalOverlayWork([&]() { ObserveFactoryResult(result, riid, output,
            "CreateDXGIFactory2",
#if defined(OVERLAY_FACTORY_CALLER_TRACE)
            caller
#else
            nullptr
#endif
            ); },
            &DisableAfterOverlayException);
        RestoreInputAfterTerminalFailure();
        return result;
    }

    bool InstallExportHook(HMODULE module, const char* name,
        safetyhook::InlineHook& hook, void* destination)
    {
        const auto address = module ? GetProcAddress(module, name) : nullptr;
#if defined(OVERLAY_STARTUP_TIMELINE)
        diagnostics::startup::SnapshotExport("pre_arm", module, name);
#endif
        if (!address) {
            Log(std::string("EXPORT_NOT_FOUND name=") + name);
            return false;
        }
        auto result = safetyhook::InlineHook::create(address, destination,
            safetyhook::InlineHook::StartDisabled);
        if (!result) {
            Log(std::string("EXPORT_HOOK_FAILED name=") + name);
            return false;
        }
        hook = std::move(*result);
        if (!hook.enable()) {
            hook.reset();
            Log(std::string("EXPORT_HOOK_FAILED name=") + name);
            return false;
        }
#if defined(OVERLAY_STARTUP_TIMELINE)
        diagnostics::startup::SnapshotExport("export_armed", module, name);
#endif
        return true;
    }

    bool InitializeImpl()
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
#if defined(OVERLAY_NO_GPU_SUBMISSION_DIAGNOSTIC)
        Log("OVERLAY_GPU_MODE=no_gpu_submission_diagnostic");
#endif
        g_lifecycle.BeginDiscovery();
        HWND window = GetForegroundWindow();
        LogDiagnostic(std::string("PROCESS_HWND hwnd=") + Hex(window));

        std::string installedExports;
        const auto recordInstalled = [&installedExports](const char* name, bool success) {
            if (!success) return;
            if (!installedExports.empty()) installedExports += ',';
            installedExports += name;
        };
#if defined(OVERLAY_STARTUP_TIMELINE)
        diagnostics::startup::Mark("system_dxgi_load_enter");
#endif
        g_systemDxgiModule = LoadExactSystemDxgiModule();
#if defined(OVERLAY_STARTUP_TIMELINE)
        diagnostics::startup::systemDxgi = g_systemDxgiModule;
        diagnostics::startup::Mark("system_dxgi_selected", g_systemDxgiModule);
#endif
        if (!g_systemDxgiModule) {
            g_lifecycle.Fail();
            Log("OVERLAY_DISCOVERY_FAILED reason=canonical_system_dxgi_unavailable");
#if defined(OVERLAY_STARTUP_JOURNAL)
            diagnostics::startup_journal::Mark("FACTORY_BOOTSTRAP_UNAVAILABLE",
                "canonical_system_dxgi_unavailable");
            diagnostics::startup_journal::Mark("DXGI_DISCOVERY_UNAVAILABLE",
                "canonical_system_dxgi_unavailable");
            diagnostics::startup_journal::Flush();
#endif
            return false;
        }
        const bool factoryInstalled = InstallExportHook(g_systemDxgiModule,
            "CreateDXGIFactory", g_createFactory,
            reinterpret_cast<void*>(&HookCreateFactory));
        recordInstalled("CreateDXGIFactory", factoryInstalled);
        const bool factory1Installed = InstallExportHook(g_systemDxgiModule,
            "CreateDXGIFactory1", g_createFactory1,
            reinterpret_cast<void*>(&HookCreateFactory1));
        recordInstalled("CreateDXGIFactory1", factory1Installed);
        const bool factory2Installed = InstallExportHook(g_systemDxgiModule,
            "CreateDXGIFactory2", g_createFactory2,
            reinterpret_cast<void*>(&HookCreateFactory2));
        recordInstalled("CreateDXGIFactory2", factory2Installed);

#if defined(OVERLAY_STARTUP_JOURNAL)
        const std::uint64_t exportMask = (factoryInstalled ? 1ull : 0ull) |
            (factory1Installed ? 2ull : 0ull) | (factory2Installed ? 4ull : 0ull);
        diagnostics::startup_journal::Mark(
            exportMask ? "EXPORT_OBSERVATION_ARMED" : "EXPORT_OBSERVATION_UNAVAILABLE",
            "supplemental", exportMask);
#endif

        const bool bootstrapInstalled = BootstrapFactoryObservation();
        const bool observationReady = overlay::HasFactoryObservationCoverage(
            bootstrapInstalled);
        if (!observationReady) {
            g_lifecycle.Fail();
            Log("OVERLAY_DISCOVERY_FAILED reason=no_factory_observation_path");
#if defined(OVERLAY_STARTUP_JOURNAL)
            diagnostics::startup_journal::Mark("DXGI_DISCOVERY_UNAVAILABLE",
                "no_factory_observation_path", exportMask);
#endif
        } else {
            Log("DXGI_DISCOVERY_READY exports=" +
                (installedExports.empty() ? std::string("none") : installedExports) +
                " bootstrap=" + (bootstrapInstalled ? "shared_table" : "unavailable"));
#if defined(OVERLAY_STARTUP_JOURNAL)
            diagnostics::startup_journal::Mark("DXGI_DISCOVERY_READY",
                "coverage=shared_table", exportMask);
#endif
        }
#if defined(OVERLAY_STARTUP_JOURNAL)
        diagnostics::startup_journal::Flush();
#endif
#if defined(OVERLAY_STARTUP_TIMELINE)
        diagnostics::startup::Mark("early_arm_complete", g_systemDxgiModule,
            observationReady);
        diagnostics::startup::SnapshotModules("arm_complete");
        diagnostics::startup::Flush();
#endif
        return observationReady;
    }

    DWORD WINAPI Initialize(void*) noexcept
    {
        overlay::RunOptionalOverlayWork([]() { InitializeImpl(); },
            &DisableAfterOverlayException);
        return 0;
    }
}

#ifdef OVERLAY_COMBINED
extern "C" bool StartOverlayDiscovery(HMODULE module)
{
#if defined(OVERLAY_STARTUP_TIMELINE)
    diagnostics::startup::Mark("early_arm_enter", module);
#endif
    g_module = module;
    DisableThreadLibraryCalls(module);
    bool hooksArmed = false;
    overlay::RunOptionalOverlayWork([&]() {
        hooksArmed = InitializeImpl();
    }, &DisableAfterOverlayException);
    return hooksArmed;
}

extern "C" void NotifyOverlayCameraCoreReady()
{
    overlay::RunOptionalOverlayWork([&]() {
        g_cameraCoreReady.store(true, std::memory_order_release);
        Log("OVERLAY_CAMERA_CORE_READY");
#if defined(OVERLAY_STARTUP_JOURNAL)
        diagnostics::startup_journal::MarkOnce(3, "CAMERA_CORE_READY", "runtime_initialized");
        diagnostics::startup_journal::Flush();
#endif
#if defined(OVERLAY_STARTUP_TIMELINE)
        diagnostics::startup::Mark("camera_core_ready");
        diagnostics::startup::SnapshotExports("core_ready", g_systemDxgiModule);
        diagnostics::startup::Flush();
#endif
    }, &DisableAfterOverlayException);
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
