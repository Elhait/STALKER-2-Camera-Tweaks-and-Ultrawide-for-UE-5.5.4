#include "../../src/overlay/renderer_runtime.hpp"
#include "../../src/plugin/runtime.hpp"
#include "../../src/overlay/optional_overlay_boundary.hpp"
#define OVERLAY_PRODUCTION
#define OVERLAY_COMBINED
#include "../../src/overlay/discovery_runtime.cpp"
#include <array>
#include <iostream>
#include <stdexcept>
static_assert(!noexcept(DisableAfterOverlayException()),
    "mutex failure must reach the optional boundary, not std::terminate");

namespace plugin
{
    RuntimeSettingsApi& GetRuntimeSettingsApi() noexcept
    { static RuntimeSettingsApi api; return api; }
    RuntimeSettingsSnapshot GetRuntimeSettingsSnapshot() noexcept { return {}; }
    bool GetOverlaySemanticSnapshot(OverlaySemanticSnapshot&) noexcept { return false; }
    bool SetDetectedOverlayLocale(std::string_view) noexcept { return false; }
    void SetHotkeyRebindCaptureActive(bool) noexcept {}
    void MarkHotkeyRebindKeyConsumed(int) noexcept {}
    void DrainOverlayNotifications(std::vector<OverlayNotification>&) noexcept {}
    void PublishOverlayNotification(OverlayNotificationKind, OverlayNotificationAction,
        std::string, std::string, OverlayNotificationStatus, std::uint32_t) noexcept {}
}

namespace
{
    struct FakeCom
    {
        void** table;
        ULONG references{1};
        IUnknown* identity{};
        IUnknown* device{};
    };
    ULONG STDMETHODCALLTYPE AddRef(IUnknown* object)
    { return ++reinterpret_cast<FakeCom*>(object)->references; }
    ULONG STDMETHODCALLTYPE Release(IUnknown* object)
    { return --reinterpret_cast<FakeCom*>(object)->references; }
    HRESULT STDMETHODCALLTYPE Query(IUnknown* object, REFIID, void** output)
    {
        const auto canonical = reinterpret_cast<FakeCom*>(object)->identity;
        *output = canonical ? canonical : object;
        AddRef(reinterpret_cast<IUnknown*>(*output));
        return S_OK;
    }
    std::array<void*, 3> basicComVtable{
        reinterpret_cast<void*>(&Query), reinterpret_cast<void*>(&AddRef),
        reinterpret_cast<void*>(&Release)};
    bool Check(bool value, const char* name)
    { std::cout << name << ": " << (value ? "PASS" : "FAIL") << '\n'; return value; }
    unsigned forwardedMessages{};
    ULONG oldChainRefsAtNative{}, oldBufferRefsAtNative{}, pendingChainRefsAtNative{};
    FakeCom* replacementOldChain{};
    FakeCom* replacementOldBuffer{};
    FakeCom* replacementPendingChain{};
    DXGI_SWAP_CHAIN_DESC resizeActualDesc{};
    IUnknown* resizeSwapchainDevice{};
    IUnknown* resizeQueues[2]{};
    UINT resizeNodeMasks[2]{1, 1};
    unsigned nativeResizeCalls{};
    HRESULT STDMETHODCALLTYPE GetSwapchainDevice(IDXGISwapChain*, REFIID, void** output)
    {
        if (!resizeSwapchainDevice) return E_NOINTERFACE;
        *output = resizeSwapchainDevice;
        resizeSwapchainDevice->AddRef();
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetSwapchainDesc(IDXGISwapChain*, DXGI_SWAP_CHAIN_DESC* desc)
    { *desc = resizeActualDesc; return S_OK; }
    HRESULT STDMETHODCALLTYPE GetQueueDevice(ID3D12CommandQueue* self, REFIID, void** output)
    {
        const auto device = reinterpret_cast<FakeCom*>(self)->device;
        if (!device) return E_NOINTERFACE;
        *output = device;
        device->AddRef();
        return S_OK;
    }
    D3D12_COMMAND_QUEUE_DESC* STDMETHODCALLTYPE GetQueueDesc(ID3D12CommandQueue*,
        D3D12_COMMAND_QUEUE_DESC* result)
    {
        *result = {D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 0};
        return result;
    }
    HRESULT STDMETHODCALLTYPE NativeResizeForFixture(IDXGISwapChain*, UINT, UINT, UINT,
        DXGI_FORMAT, UINT)
    { ++nativeResizeCalls; return S_OK; }
    HRESULT STDMETHODCALLTYPE NativeResize1ForFixture(IDXGISwapChain3*, UINT, UINT, UINT,
        DXGI_FORMAT, UINT, const UINT*, IUnknown* const*)
    { ++nativeResizeCalls; return S_OK; }
    HRESULT STDMETHODCALLTYPE NativeCreateForReplacement(IDXGIFactory2*, IUnknown*, HWND,
        const DXGI_SWAP_CHAIN_DESC1*, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*, IDXGIOutput*,
        IDXGISwapChain1**)
    {
        oldChainRefsAtNative = replacementOldChain->references;
        oldBufferRefsAtNative = replacementOldBuffer->references;
        pendingChainRefsAtNative = replacementPendingChain->references;
        return E_ACCESSDENIED;
    }
    LRESULT CALLBACK NativeWindow(HWND, UINT, WPARAM, LPARAM)
    { ++forwardedMessages; return 42; }
}

namespace overlay
{
    struct RendererLifetimeFixture
    {
        static void Seed(Renderer& renderer, FakeCom& swapchain, FakeCom& device,
            FakeCom& queue, FakeCom& backbuffer, HWND window)
        {
            renderer.swapchain_ = reinterpret_cast<IDXGISwapChain*>(&swapchain);
            renderer.swapchainIdentity_ = reinterpret_cast<std::uintptr_t>(&swapchain);
            renderer.device_ = reinterpret_cast<ID3D12Device*>(&device);
            renderer.queue_ = reinterpret_cast<ID3D12CommandQueue*>(&queue);
            renderer.backbuffers_.push_back(reinterpret_cast<ID3D12Resource*>(&backbuffer));
            renderer.window_ = window;
            renderer.resourcesReady_ = true;
            renderer.lifecycle_.BeginInitialization();
            renderer.lifecycle_.MarkReady();
        }
        static bool Detached(const Renderer& renderer)
        { return !renderer.swapchain_ && !renderer.device_ && !renderer.queue_ &&
            renderer.backbuffers_.empty() && !renderer.resourcesReady_; }
        static void EnterNestedResize(Renderer& renderer)
        { renderer.lifecycle_.BeginResize(); renderer.resizeDepth_ = 2; }
        static void UnprovenGpuWork(Renderer& renderer)
        { renderer.gpuWorkSubmitted_ = true; renderer.gpuWaitFailed_ = true; }
    };
}

int main()
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    std::cout << std::unitbuf;
    bool pass = true;
    const auto window = reinterpret_cast<HWND>(0x1234);
    FakeCom swapchain{basicComVtable.data()}, device{basicComVtable.data()}, queue{basicComVtable.data()}, buffer{basicComVtable.data()};
    overlay::Renderer renderer;
    overlay::RendererLifetimeFixture::Seed(renderer, swapchain, device, queue, buffer, window);
    renderer.SetVisible(true);
    FakeCom alias{basicComVtable.data(), 1, reinterpret_cast<IUnknown*>(&swapchain)};
    FakeCom unrelated{basicComVtable.data()};
    pass &= Check(renderer.OwnsSwapchain(reinterpret_cast<IDXGISwapChain*>(&alias)) &&
        !renderer.OwnsSwapchain(reinterpret_cast<IDXGISwapChain*>(&unrelated)),
        "renderer_matches_canonical_identity_not_interface_address");
    renderer.BeforeSwapchainReplacement(reinterpret_cast<HWND>(0x5678));
    pass &= Check(swapchain.references == 1 && buffer.references == 1,
        "unrelated_window_does_not_release_active_renderer");
    renderer.BeforeSwapchainReplacement(nullptr);
    pass &= Check(swapchain.references == 1, "null_window_does_not_release_active_renderer");
    renderer.BeforeSwapchainReplacement(window);
    // This is the actual Renderer teardown, with synthetic COM counters replacing
    // GPU objects. Native replacement is only allowed after old refs reach zero.
    const HRESULT nativeCreation = swapchain.references ? E_ACCESSDENIED : S_OK;
    pass &= Check(nativeCreation == S_OK && !device.references && !queue.references &&
        !buffer.references && overlay::RendererLifetimeFixture::Detached(renderer),
        "replacement_releases_swapchain_and_backbuffers_before_native_creation");
    pass &= Check(!renderer.disabled() && renderer.visible(),
        "replacement_is_not_terminal_disable_and_preserves_visibility");
    renderer.BeforeSwapchainReplacement(window);
    pass &= Check(!swapchain.references, "replacement_teardown_is_idempotent");

    FakeCom other{basicComVtable.data()};
    pass &= Check(!renderer.OwnsSwapchain(reinterpret_cast<IDXGISwapChain*>(&other)),
        "detached_renderer_does_not_own_unrelated_swapchain");
    renderer.OnSwapchainCreationFailure(reinterpret_cast<HWND>(0x5678));
    pass &= Check(!renderer.disabled(), "unrelated_creation_failure_does_not_disable_overlay");
    renderer.OnSwapchainCreationFailure(window);
    pass &= Check(renderer.disabled() && !renderer.visible(),
        "failed_replacement_closes_overlay_without_stale_capture");
    renderer.Disable("fixture");
    renderer.BeforeSwapchainReplacement(window);
    pass &= Check(renderer.disabled() && !renderer.visible(), "terminal_failure_stays_closed");
    overlay::Renderer failed;
    FakeCom failedSwap{basicComVtable.data()}, failedDevice{basicComVtable.data()}, failedQueue{basicComVtable.data()}, failedBuffer{basicComVtable.data()};
    overlay::RendererLifetimeFixture::Seed(failed, failedSwap, failedDevice, failedQueue, failedBuffer, window);
    failed.SetVisible(true);
    failed.SetLogger([](const std::string&) { throw std::runtime_error("fixture logging failure"); });
    const auto failure = overlay::RunOptionalOverlayWork(
        [&]() { failed.BeforeSwapchainReplacement(window); },
        [&]() noexcept { failed.Disable("fixture_exception"); });
    pass &= Check(failure == overlay::OptionalOverlayWorkResult::Failed && failed.disabled() &&
        !failed.visible() && !failedSwap.references && !failedBuffer.references &&
        overlay::RendererLifetimeFixture::Detached(failed),
        "replacement_exception_still_releases_resources_and_closes_capture");
    overlay::Renderer resizing;
    FakeCom resizeSwap{basicComVtable.data()}, resizeDevice{basicComVtable.data()}, resizeQueue{basicComVtable.data()}, resizeBuffer{basicComVtable.data()};
    overlay::RendererLifetimeFixture::Seed(resizing, resizeSwap, resizeDevice, resizeQueue, resizeBuffer, window);
    pass &= Check(resizing.ready() && resizing.needsRenderWork(),
        "ready_renderer_is_eligible_for_present_render_work");
    overlay::RendererLifetimeFixture::EnterNestedResize(resizing);
    pass &= Check(!resizing.ready() && resizing.needsRenderWork(),
        "successful_resize_rebuild_remains_eligible_on_next_present");
    resizing.OnResizeResult(reinterpret_cast<IDXGISwapChain*>(&resizeSwap), false, 0, 0);
    pass &= Check(!resizing.disabled() && resizeSwap.references == 1,
        "inner_resize_failure_defers_to_outer_native_call");
    resizing.OnResizeResult(reinterpret_cast<IDXGISwapChain*>(&resizeSwap), false, 0, 0);
    pass &= Check(resizing.disabled() && !resizeSwap.references && !resizeBuffer.references,
        "outer_resize_failure_disables_and_releases_resources");
    pass &= Check(!resizing.needsRenderWork(),
        "disabled_renderer_is_not_eligible_for_present_render_work");
    overlay::Renderer pending;
    FakeCom pendingSwap{basicComVtable.data()}, pendingDevice{basicComVtable.data()}, pendingQueue{basicComVtable.data()}, pendingBuffer{basicComVtable.data()};
    overlay::RendererLifetimeFixture::Seed(pending, pendingSwap, pendingDevice, pendingQueue, pendingBuffer, window);
    overlay::RendererLifetimeFixture::UnprovenGpuWork(pending);
    pending.Disable("fixture_gpu_completion_unknown");
    pass &= Check(pending.disabled() && pendingSwap.references == 1 && pendingBuffer.references == 1 &&
        pendingDevice.references == 1 && pendingQueue.references == 1 && overlay::RendererLifetimeFixture::Detached(pending),
        "unproven_gpu_frame_detaches_but_retains_last_resource_refs");
    pending.Shutdown();
    pass &= Check(pendingBuffer.references == 1, "terminal_retention_not_released_by_repeat_shutdown");

    // Exercise the production ResizeBuffers1 callback with a pending target.
    // Successful native resize must refresh actual descriptor values and a
    // validated replacement queue; ambiguous per-buffer queues fail closed.
    auto resizeTable = std::array<void*, 40>{};
    resizeTable.fill(reinterpret_cast<void*>(&Query));
    resizeTable[1] = reinterpret_cast<void*>(&AddRef);
    resizeTable[2] = reinterpret_cast<void*>(&Release);
    resizeTable[7] = reinterpret_cast<void*>(&GetSwapchainDevice);
    resizeTable[12] = reinterpret_cast<void*>(&GetSwapchainDesc);
    resizeTable[13] = reinterpret_cast<void*>(&NativeResizeForFixture);
    resizeTable[39] = reinterpret_cast<void*>(&NativeResize1ForFixture);
    FakeCom resizeChain{resizeTable.data()};
    FakeCom resizeDeviceOwner{basicComVtable.data()};
    std::array<void*, 20> queueTable{};
    queueTable.fill(reinterpret_cast<void*>(&Query));
    queueTable[1] = reinterpret_cast<void*>(&AddRef);
    queueTable[2] = reinterpret_cast<void*>(&Release);
    queueTable[7] = reinterpret_cast<void*>(&GetQueueDevice);
    queueTable[18] = reinterpret_cast<void*>(&GetQueueDesc);
    FakeCom originalQueue{queueTable.data(), 1, nullptr,
        reinterpret_cast<IUnknown*>(&resizeDeviceOwner)};
    FakeCom updatedQueue{queueTable.data(), 1, nullptr,
        reinterpret_cast<IUnknown*>(&resizeDeviceOwner)};
    FakeCom ambiguousQueue{queueTable.data(), 1, nullptr,
        reinterpret_cast<IUnknown*>(&resizeDeviceOwner)};
    g_swapchainHooks.Install(&resizeChain, 40, [](overlay::DxgiHookRecord& record) {
        record.replacement[13] = reinterpret_cast<void*>(&HookResizeBuffers);
        record.replacement[39] = reinterpret_cast<void*>(&HookResizeBuffers1);
    });
    const auto resizeIdentity = reinterpret_cast<std::uintptr_t>(&resizeChain);
    g_pendingRendererTarget.emplace();
    g_pendingRendererTarget->swapchain = reinterpret_cast<IDXGISwapChain*>(&resizeChain);
    g_pendingRendererTarget->device = reinterpret_cast<ID3D12Device*>(&resizeDeviceOwner);
    g_pendingRendererTarget->queue = reinterpret_cast<ID3D12CommandQueue*>(&originalQueue);
    g_pendingRendererTarget->identity = resizeIdentity;
    g_pendingRendererTarget->window = window;
    g_pendingRendererTarget->bufferCount = 2;
    g_pendingRendererTarget->format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resizeSwapchainDevice = reinterpret_cast<IUnknown*>(&resizeDeviceOwner);
    resizeActualDesc.BufferDesc.Width = 1920;
    resizeActualDesc.BufferDesc.Height = 1080;
    resizeActualDesc.BufferDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
    resizeActualDesc.BufferCount = 3;
    g_evidence.BeginSwapchain(resizeIdentity);
    g_evidence.ObserveCandidate(resizeIdentity, 1, 2, 0);
    auto resize1 = reinterpret_cast<ResizeBuffers1Fn>(resizeChain.table[39]);
    nativeResizeCalls = 0;
    pass &= Check(resize1(reinterpret_cast<IDXGISwapChain3*>(&resizeChain), 0, 0, 0,
        DXGI_FORMAT_UNKNOWN, 0, nullptr, nullptr) == S_OK && nativeResizeCalls == 1 &&
        g_pendingRendererTarget && g_pendingRendererTarget->bufferCount == 3 &&
        g_pendingRendererTarget->format == DXGI_FORMAT_R10G10B10A2_UNORM &&
        g_pendingRendererTarget->queue.Get() == reinterpret_cast<ID3D12CommandQueue*>(&originalQueue),
        "successful_zero_preservation_resize_refreshes_actual_descriptor_and_retains_queue");
    resizeActualDesc.BufferCount = 2;
    resizeActualDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resizeQueues[0] = reinterpret_cast<IUnknown*>(&updatedQueue);
    resizeQueues[1] = reinterpret_cast<IUnknown*>(&updatedQueue);
    nativeResizeCalls = 0;
    pass &= Check(resize1(reinterpret_cast<IDXGISwapChain3*>(&resizeChain), 2, 1920, 1080,
        DXGI_FORMAT_R8G8B8A8_UNORM, 0, resizeNodeMasks, resizeQueues) == S_OK &&
        nativeResizeCalls == 1 && g_pendingRendererTarget &&
        g_pendingRendererTarget->bufferCount == 2 &&
        g_pendingRendererTarget->format == DXGI_FORMAT_R8G8B8A8_UNORM &&
        g_pendingRendererTarget->queue.Get() == reinterpret_cast<ID3D12CommandQueue*>(&updatedQueue),
        "successful_resize1_rebinds_validated_same_device_present_queue");
    resizeQueues[1] = reinterpret_cast<IUnknown*>(&ambiguousQueue);
    nativeResizeCalls = 0;
    pass &= Check(resize1(reinterpret_cast<IDXGISwapChain3*>(&resizeChain), 2, 1920, 1080,
        DXGI_FORMAT_R8G8B8A8_UNORM, 0, resizeNodeMasks, resizeQueues) == S_OK &&
        nativeResizeCalls == 1 && !g_pendingRendererTarget,
        "ambiguous_per_buffer_queue_topology_keeps_native_resize_but_discards_overlay_target");

    // Exercise replacement ordering through the actual factory callback rather
    // than invoking Renderer::BeforeSwapchainReplacement directly.
    FakeCom oldChain{basicComVtable.data()}, oldDevice{basicComVtable.data()}, oldQueue{basicComVtable.data()}, oldBuffer{basicComVtable.data()};
    overlay::RendererLifetimeFixture::Seed(g_renderer, oldChain, oldDevice,
        oldQueue, oldBuffer, window);
    g_renderer.SetVisible(true);
    overlay::GetInputState().Toggle();
    FakeCom pendingChain{basicComVtable.data()}, replacementDevice{basicComVtable.data()}, replacementQueue{basicComVtable.data()};
    g_pendingRendererTarget.emplace();
    g_pendingRendererTarget->swapchain = reinterpret_cast<IDXGISwapChain*>(&pendingChain);
    g_pendingRendererTarget->device = reinterpret_cast<ID3D12Device*>(&replacementDevice);
    g_pendingRendererTarget->queue = reinterpret_cast<ID3D12CommandQueue*>(&replacementQueue);
    g_pendingRendererTarget->identity = reinterpret_cast<std::uintptr_t>(&pendingChain);
    g_pendingRendererTarget->window = window;
    std::array<void*, 25> factoryTable{};
    factoryTable.fill(reinterpret_cast<void*>(&Query));
    factoryTable[1] = reinterpret_cast<void*>(&AddRef);
    factoryTable[2] = reinterpret_cast<void*>(&Release);
    factoryTable[15] = reinterpret_cast<void*>(&NativeCreateForReplacement);
    FakeCom factory{factoryTable.data()};
    InstallFactoryHooks(reinterpret_cast<IUnknown*>(&factory), 25, "FS01_fixture");
    replacementOldChain = &oldChain;
    replacementOldBuffer = &oldBuffer;
    replacementPendingChain = &pendingChain;
    IDXGISwapChain1* replacementResult{};
    pass &= Check(HookCreateSwapChainForHwnd(reinterpret_cast<IDXGIFactory2*>(&factory),
        reinterpret_cast<IUnknown*>(&replacementQueue), reinterpret_cast<HWND>(0x5678),
        nullptr, nullptr, nullptr, &replacementResult) == E_ACCESSDENIED &&
        oldChainRefsAtNative == 1 && oldBufferRefsAtNative == 1 &&
        pendingChainRefsAtNative == 2 && g_pendingRendererTarget &&
        !g_renderer.disabled() && overlay::GetInputState().visible(),
        "unrelated_factory_replacement_preserves_active_and_pending_overlay_owners");
    pass &= Check(HookCreateSwapChainForHwnd(reinterpret_cast<IDXGIFactory2*>(&factory),
        reinterpret_cast<IUnknown*>(&replacementQueue), window, nullptr, nullptr, nullptr,
        &replacementResult) == E_ACCESSDENIED && oldChainRefsAtNative == 0 &&
        oldBufferRefsAtNative == 0 && pendingChainRefsAtNative == 1 &&
        !g_pendingRendererTarget && g_renderer.disabled() && !g_renderer.visible() &&
        !overlay::GetInputState().visible(),
        "actual_factory_replacement_releases_matching_active_and_pending_owners_before_native_and_closes_on_failure");

    g_renderer.Disable("fixture_terminal_input");
    auto& input = overlay::GetInputState();
    const auto toggle = ProcessOverlayWindowMessage(nullptr, WM_KEYUP, VK_DELETE, 0);
    const auto escape = ProcessOverlayWindowMessage(nullptr, WM_KEYUP, VK_ESCAPE, 0);
    const auto mouse = ProcessOverlayWindowMessage(nullptr, WM_MOUSEMOVE, 0, 0);
    pass &= Check(!toggle.handled && !escape.handled && !mouse.handled && !input.visible(),
        "actual_terminal_wndproc_policy_cannot_reopen_or_capture");
    g_originalWindowProc = &NativeWindow;
    pass &= Check(OverlayWindowProc(nullptr, WM_KEYUP, VK_DELETE, 0) == 42 && forwardedMessages == 1 && !input.visible(),
        "terminal_wndproc_preserves_native_message_result");
    g_inputWindow = window;
    pass &= Check(OverlayWindowProc(window, WM_NCDESTROY, 0, 0) == 42 &&
        !g_inputWindow && !g_originalWindowProc, "destroyed_window_discards_subclass_owner_after_native_forwarding");
    pass &= Check(overlay::RunOptionalOverlayWork([]() { throw 1; }, []() { throw 2; }) ==
        overlay::OptionalOverlayWorkResult::Failed, "throwing_failure_transition_cannot_escape_native_boundary");
    return pass ? 0 : 1;
}
