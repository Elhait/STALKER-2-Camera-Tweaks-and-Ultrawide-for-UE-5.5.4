#pragma once

#include "discovery_evidence.hpp"
#include "renderer_state.hpp"
#include "placement_save_state.hpp"
#include "localization_manager.hpp"
#include "../plugin/runtime_settings.hpp"

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>

#include <atomic>
#include <cstdint>
#include <chrono>
#include <string>
#include <string_view>
#include <vector>

struct ImGui_ImplDX12_InitInfo;

namespace overlay
{
    class Renderer
    {
    public:
        using LogFunction = void(*)(const std::string&);

        void SetLogger(LogFunction logger) noexcept { logger_ = logger; }
        bool Initialize(IDXGISwapChain* swapchain, ID3D12Device* device,
            ID3D12CommandQueue* queue, HWND window, UINT bufferCount,
            DXGI_FORMAT format) noexcept;
        void BeforeSwapchainReplacement(HWND window);
        void OnSwapchainCreationFailure(HWND window);
        bool OwnsSwapchain(IDXGISwapChain* swapchain) const noexcept;
        bool OwnsWindow(HWND window) const noexcept { return window_ == window && window != nullptr; }
        bool ready() const noexcept
        {
            return lifecycle_.state() == RendererState::Ready;
        }
        bool needsRenderWork() const noexcept
        {
            const auto state = lifecycle_.state();
            return state == RendererState::Ready || state == RendererState::Resizing;
        }
        void BeforeResize(IDXGISwapChain* swapchain);
        void OnResizeResult(IDXGISwapChain* swapchain, bool success,
            double originalResizeMs, double totalResizeHookMs,
            UINT queueCount = 0, IUnknown* const* presentQueues = nullptr);
        bool Render(IDXGISwapChain* swapchain, AssociationState association) noexcept;
        void SetVisible(bool visible) noexcept
        {
            visible_.store(visible, std::memory_order_release);
        }
        bool visible() const noexcept
        {
            return visible_.load(std::memory_order_acquire);
        }
        bool disabled() const noexcept
        {
            return lifecycle_.state() == RendererState::Disabled;
        }
        void Disable(const char* reason) noexcept;
        void Shutdown() noexcept;

    private:
#ifdef OVERLAY_RENDERER_LIFETIME_TEST
        friend struct RendererLifetimeFixture;
#endif
        struct FrameContext
        {
            ID3D12CommandAllocator* allocator{};
            std::uint64_t fenceValue{};
        };

        bool InitializeImpl(IDXGISwapChain* swapchain, ID3D12Device* device,
            ID3D12CommandQueue* queue, HWND window, UINT bufferCount,
            DXGI_FORMAT format);
        void RenderImpl(IDXGISwapChain* swapchain, AssociationState association,
            bool& commandListSubmitted);
        void FailAfterException() noexcept;
        bool BuildResources(IDXGISwapChain* swapchain);
        bool BuildImGui();
        bool RebuildFontAtlas(int fontSizePixels,
            std::string_view fontProfileCode);
        bool WaitForGpu() noexcept;
        bool WaitForFrame(FrameContext& frame) noexcept;
        void ReleaseBackbuffers() noexcept;
        void Log(const std::string& message) const;
        void UpdateNotifications() noexcept;
        void DrawNotifications();
        static void AllocateSrv(ImGui_ImplDX12_InitInfo* info,
            D3D12_CPU_DESCRIPTOR_HANDLE* cpu,
            D3D12_GPU_DESCRIPTOR_HANDLE* gpu);
        static void FreeSrv(ImGui_ImplDX12_InitInfo* info,
            D3D12_CPU_DESCRIPTOR_HANDLE cpu,
            D3D12_GPU_DESCRIPTOR_HANDLE gpu);

        RendererLifecycle lifecycle_;
        LocalizationManager localization_;
        LogFunction logger_{};
        IDXGISwapChain* swapchain_{};
        std::uintptr_t swapchainIdentity_{};
        ID3D12Device* device_{};
        ID3D12CommandQueue* queue_{};
        HWND window_{};
        DXGI_FORMAT format_{DXGI_FORMAT_R8G8B8A8_UNORM};
        UINT bufferCount_{};
        UINT rtvStride_{};
        ID3D12DescriptorHeap* rtvHeap_{};
        ID3D12DescriptorHeap* srvHeap_{};
        ID3D12GraphicsCommandList* commandList_{};
        ID3D12Fence* fence_{};
        HANDLE fenceEvent_{};
        std::uint64_t nextFenceValue_{};
        bool gpuWaitFailed_{};
        bool gpuWorkSubmitted_{};
        std::vector<ID3D12Resource*> backbuffers_;
        std::vector<FrameContext> frames_;
        bool imguiReady_{};
        bool imguiContextCreated_{};
        int activeFontSizePixels_{};
        int pendingFontSizePixels_{};
        std::string_view activeFontProfileCode_{"base"};
#ifdef OVERLAY_FONT_RASTERIZATION_EXPERIMENT
        int activeFontRasterizationMode_{};
#endif
#ifdef OVERLAY_FONT_PLAYGROUND_EXPERIMENT
        int activeFontChoice_{};
#endif
        std::wstring overlayConfigPath_;
        float overlayPositionX_{30.0f};
        float overlayPositionY_{30.0f};
        float lastOverlayPositionX_{};
        float lastOverlayPositionY_{};
        bool overlayPlacementLoaded_{};
        bool overlayPositionObserved_{};
        PlacementSaveState placementSaveState_;
        StartupHintGate startupHintGate_;
        bool startupAutoLanguageSyncRequested_{};
        bool startupAutoLanguageSyncRequestFailureLogged_{};
        bool resourcesReady_{};
        UINT resizeDepth_{};
        std::atomic<bool> visible_{};
        double resizePreWaitMs_{};
        double resizeReleaseMs_{};
        double originalResizeMs_{};
        double totalResizeHookMs_{};
        std::chrono::steady_clock::time_point resizeResultTime_{};
        std::vector<plugin::OverlayNotification> notifications_;
    };
}
