#pragma once

#include "composition_presenter_state.hpp"
#include "presenter_runtime_policy.hpp"
#include "placement_save_state.hpp"
#include "renderer_state.hpp"
#include "../presentation/presentation_contracts.hpp"
#include "../presentation/inline_hotkey_layout.hpp"
#include "../input/input_state.hpp"
#include "../localization/localization_manager.hpp"
#include "imgui_d3d11_renderer.hpp"
#include "../../plugin/runtime_settings.hpp"

#include <imgui.h>

#include <Windows.h>
#include <d3d11.h>
#include <dcomp.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <atomic>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace overlay
{
    class Renderer
    {
    public:
        using LogFunction = void(*)(const std::string&);
        using TerminalRequestFunction = void(*)() noexcept;

        void SetLogger(LogFunction logger) noexcept { logger_ = logger; }
        void SetTerminalRequest(TerminalRequestFunction request) noexcept
        {
            terminalRequest_ = request;
        }
        bool Initialize(HWND gameWindow, HWND overlayWindow, UINT width,
            UINT height, UINT dpi = 96) noexcept;
        bool OnWindowGeometry(HWND window, UINT width, UINT height,
            bool minimized, UINT dpi = 96) noexcept;
        bool OnHostActivation(HWND window, bool active) noexcept;
        void SetInputBindingReady(HWND window) noexcept;
        void CompleteStartupGameDataProbe(bool readSucceeded) noexcept;
        std::uint32_t StartupGameDataProbeWaitMilliseconds(
            std::uint64_t nowMs) const noexcept
        {
            return startupReadiness_.ProbeWaitMilliseconds(nowMs);
        }
        bool ResolveStartupGameDataProbeTimeout() noexcept;
        bool Render() noexcept;
        bool OwnsWindow(HWND window) const noexcept
        {
            return gameWindow_ == window && window != nullptr;
        }
        bool ready() const noexcept
        {
            return state_.phase() == PresenterPhase::Ready;
        }
        bool hasSurfaceGeneration() const noexcept
        {
            return state_.generation() != 0 &&
                state_.phase() != PresenterPhase::Disabled;
        }
        bool disabled() const noexcept
        {
            return state_.phase() == PresenterPhase::Disabled;
        }
        bool InputPresentationReady() const noexcept
        {
            return ready() && visible() && menuFrameAdmission_.ready() && visualAttached_ &&
                overlayWindowVisible_ && hostActive_ && !hostMinimized_ &&
                overlayWindow_ && IsWindowVisible(overlayWindow_) && DisplayWindowAboveHost();
        }
        bool OwnedResourcesReleased() const noexcept
        {
            return !device_ && !dxgiDevice_ && !compositionDevice_ &&
                !compositionTarget_ && !visual_ && !surface_ &&
                !imguiRenderer_.HasOwnedResources() && !imguiReady_ &&
                !imguiContextCreated_ && !ImGui::GetCurrentContext() &&
                !gameWindow_ && !overlayWindow_ &&
                !overlayWindowVisible_ && !visualAttached_;
        }
        void SetVisible(bool visible) noexcept
        {
            if (visible != this->visible()) forceFullDamage_ = true;
            menuFrameAdmission_.Request(visible);
            visible_.store(visible, std::memory_order_release);
            surfaceDirty_ = true;
        }
        bool visible() const noexcept
        {
            return visible_.load(std::memory_order_acquire);
        }
        bool needsRenderWork() const noexcept
        {
            return state_.phase() == PresenterPhase::Ready &&
                (surfaceDirty_ ||
                    (startupNotification_.ShouldWakePresenter(
                        state_.phase() == PresenterPhase::Ready, inputBindingReady_) &&
                        !startupReadiness_.probeStarted()) ||
                    !notifications_.empty());
        }
        bool notificationAnimationActive() const noexcept
        {
            return state_.phase() == PresenterPhase::Ready &&
                !notifications_.empty();
        }
        void RequestRedraw() noexcept { surfaceDirty_ = true; }
        bool RebindWindow(HWND gameWindow, UINT width, UINT height,
            UINT dpi = 96) noexcept;
        void Disable(const char* reason) noexcept;
        bool Shutdown() noexcept;

    private:
        friend struct RendererOwnershipTestAccess;
        bool InitializeImpl(HWND gameWindow, HWND overlayWindow, UINT width,
            UINT height, UINT dpi);
        bool CreateGraphicsDevice();
        bool CreateCompositionTarget(HWND window);
        bool CreateSurface(UINT width, UINT height,
            Microsoft::WRL::ComPtr<IDCompositionSurface>& surface) noexcept;
        bool RecoverGraphicsDevice(UINT width, UINT height) noexcept;
        bool BuildImGui();
        void BeginImGuiFrame(UINT width, UINT height) noexcept;
        bool RebuildFontAtlas(int fontSizePixels,
            std::string_view fontProfileCode);
        bool RenderImpl(IDCompositionSurface* targetSurface,
            UINT width, UINT height);
        bool DrawImGuiSurface(IDCompositionSurface* targetSurface,
            UINT width, UINT height, const ImDrawData* drawData,
            presentation::Rect updateRect) noexcept;
        bool ApplyWindowDpi(UINT dpi) noexcept;
        bool UpdateOverlayWindowBounds(bool forcePlacement = false) noexcept;
        bool DisplayWindowAboveHost() const noexcept;
        void ReconcileHostActivity() noexcept;
        bool SetOverlayWindowVisibility(bool visible) noexcept;
        void LogDisplayState() noexcept;
        void LogNotificationLifecycle(const char* event,
            const char* reason, std::uint64_t notificationId = 0) const noexcept;
        void LogStartupGate(const char* event, const char* reason) const noexcept;
        void LogStartupGateEligibility() noexcept;
        void FailAfterException() noexcept;
        void Log(const std::string& message) const noexcept;
        void Log(const char* message) const noexcept;
        void UpdateNotifications() noexcept;
        void PrepareStartupNotification() noexcept;
        void EnsureStartupNotification(
            const plugin::RuntimeSettingsSnapshot& settings) noexcept;
        void DrawNotifications();
        bool StartPendingNotificationLifetimes() noexcept;
        bool ReconcileCompositionAttachment() noexcept;
        void LogCompositionAttachment() const noexcept;
        bool ResetComposition() noexcept;
        void DetachImGui() noexcept;
        void ApplyInputEvents(UINT width, UINT height);

        struct ActiveNotification
        {
            plugin::OverlayNotification notification;
            NotificationLifetime lifetime;
            std::uintptr_t frameDrawListToken{};
        };

        CompositionPresenterState state_;
        LocalizationManager localization_;
        LogFunction logger_{};
        TerminalRequestFunction terminalRequest_{};
        Microsoft::WRL::ComPtr<ID3D11Device> device_;
        Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice_;
        Microsoft::WRL::ComPtr<IDCompositionDesktopDevice> compositionDevice_;
        Microsoft::WRL::ComPtr<IDCompositionTarget> compositionTarget_;
        Microsoft::WRL::ComPtr<IDCompositionVisual2> visual_;
        Microsoft::WRL::ComPtr<IDCompositionSurface> surface_;
        ImGuiD3D11Renderer imguiRenderer_;
        HWND gameWindow_{};
        HWND overlayWindow_{};
        UINT surfaceWidth_{};
        UINT surfaceHeight_{};
        UINT windowDpi_{96};
        float dpiScale_{1.0f};
        std::uint64_t surfaceGeneration_{};
        bool compositionReady_{};
        bool visualAttached_{};
        bool hostActive_{};
        bool inputBindingReady_{};
        MenuFrameAdmission menuFrameAdmission_;
        bool hostMinimized_{};
        bool overlayWindowVisible_{};
        DisplayPlacement overlayPlacement_{};
        unsigned diagnosticDisplayState_{~0u};
        bool imguiReady_{};
        bool imguiContextCreated_{};
        int activeFontSizePixels_{};
        int activeFontSetting_{};
        std::string_view activeFontProfileCode_{"base"};
        std::string activeLocaleCode_{"en"};
        ImGuiStyle baseStyle_{};
        bool baseStyleCaptured_{};
#ifdef OVERLAY_FONT_RASTERIZATION_EXPERIMENT
        int activeFontRasterizationMode_{};
#endif
#ifdef OVERLAY_FONT_PLAYGROUND_EXPERIMENT
        int activeFontChoice_{};
#endif
        std::wstring overlayConfigPath_;
        float overlayPositionX_{30.0f};
        float overlayPositionY_{30.0f};
        float compactOverlayWidthLogical_{};
        float compactOverlayHeightLogical_{};
        bool cameraStateExpanded_{};
        ImVec2 cameraStateCompactPosition_{};
        bool cameraStateCompactPositionValid_{};
        bool restoreCameraStateCompactPosition_{};
        float lastOverlayPositionX_{};
        float lastOverlayPositionY_{};
        bool overlayPlacementLoaded_{};
        bool overlayPositionObserved_{};
        PlacementSaveState placementSaveState_;
        StartupNotificationState startupNotification_;
        StartupToastReadinessGate startupReadiness_;
        bool startupGateEligibilityLogged_{};
        bool startupAutoLocaleWaitLogged_{};
        bool startupHintDrawSubmissionLogged_{};
        bool startupHintFrameLogged_{};
        bool firstRawMouseAppliedLogged_{};
        bool firstAbsoluteMouseAppliedLogged_{};
        bool firstFrameLogged_{};
        VirtualCursorPosition virtualCursorPosition_;
        std::chrono::steady_clock::time_point lastImGuiFrameTime_{};
        bool hasImGuiFrameTime_{};
        bool surfaceDirty_{true};
        bool forceFullDamage_{true};
        presentation::DamageTracker damageTracker_;
        int pendingFontSizePixels_{};
        std::atomic<bool> visible_{};
        std::vector<ActiveNotification> notifications_;
        std::vector<NotificationDrawListEvidence> submittedDrawLists_;
        std::array<std::uint64_t, 3> submittedNotificationIds_{};
        std::size_t submittedNotificationCount_{};
    };
}
