#include "composition_presenter_state.hpp"

namespace overlay
{
    namespace
    {
        constexpr std::uint32_t MaximumSurfaceDimension = 16384;

        bool IsValidExtent(SurfaceExtent extent) noexcept
        {
            return extent.width > 0 && extent.height > 0 &&
                extent.width <= MaximumSurfaceDimension &&
                extent.height <= MaximumSurfaceDimension;
        }
    }

    void CompositionPresenterState::MarkCameraCoreReady() noexcept
    {
        if (phase_ == PresenterPhase::Disabled) return;
        cameraCoreReady_ = true;
        if (phase_ == PresenterPhase::WaitingForCore)
            phase_ = PresenterPhase::WaitingForWindow;
    }

    bool CompositionPresenterState::MarkWindowAvailable(
        SurfaceExtent extent) noexcept
    {
        if (phase_ == PresenterPhase::Disabled || !cameraCoreReady_ ||
            !IsValidExtent(extent))
            return false;
        if (phase_ == PresenterPhase::WaitingForCore)
            phase_ = PresenterPhase::WaitingForWindow;
        return phase_ == PresenterPhase::WaitingForWindow ||
            phase_ == PresenterPhase::Ready || phase_ == PresenterPhase::Minimized;
    }

    bool CompositionPresenterState::BeginSurfaceUpdate(SurfaceExtent extent,
        SurfaceUpdateToken& token) const noexcept
    {
        if (!cameraCoreReady_ || phase_ == PresenterPhase::Disabled ||
            phase_ == PresenterPhase::RecoveringDevice || !IsValidExtent(extent) ||
            phase_ == PresenterPhase::Minimized ||
            (phase_ != PresenterPhase::WaitingForWindow &&
                phase_ != PresenterPhase::Ready))
            return false;
        token = {generation_, generation_ + 1, extent};
        return true;
    }

    bool CompositionPresenterState::PublishSurfaceUpdate(
        const SurfaceUpdateToken& token, bool fullSurfaceDrawn,
        bool compositionCommitted) noexcept
    {
        if (phase_ == PresenterPhase::Disabled ||
            (phase_ != PresenterPhase::WaitingForWindow &&
                phase_ != PresenterPhase::Ready) ||
            token.expectedGeneration != generation_ ||
            token.nextGeneration != generation_ + 1 ||
            !IsValidExtent(token.extent) || !fullSurfaceDrawn ||
            !compositionCommitted)
            return false;
        generation_ = token.nextGeneration;
        extent_ = token.extent;
        phase_ = PresenterPhase::Ready;
        return true;
    }

    void CompositionPresenterState::SetMinimized(bool minimized) noexcept
    {
        if (phase_ == PresenterPhase::Disabled ||
            phase_ == PresenterPhase::RecoveringDevice || generation_ == 0)
            return;
        minimized_ = minimized;
        phase_ = minimized ? PresenterPhase::Minimized : PresenterPhase::Ready;
    }

    bool CompositionPresenterState::BeginDeviceRecovery() noexcept
    {
        if (phase_ != PresenterPhase::Ready && phase_ != PresenterPhase::Minimized)
            return false;
        if (deviceRecoveryAttempted_) {
            DisableOverlay();
            return false;
        }
        deviceRecoveryAttempted_ = true;
        phase_ = PresenterPhase::RecoveringDevice;
        return true;
    }

    bool CompositionPresenterState::CompleteDeviceRecovery(bool surfaceCommitted,
        SurfaceExtent extent) noexcept
    {
        if (phase_ != PresenterPhase::RecoveringDevice) return false;
        if (!surfaceCommitted || !IsValidExtent(extent)) {
            DisableOverlay();
            return false;
        }
        ++generation_;
        extent_ = extent;
        minimized_ = false;
        phase_ = PresenterPhase::Ready;
        return true;
    }

    void CompositionPresenterState::DisableOverlay() noexcept
    {
        phase_ = PresenterPhase::Disabled;
    }

    RedrawReason SelectRedrawReason(bool ready, bool dirty, bool visible,
        bool notificationActive) noexcept
    {
        if (!ready) return RedrawReason::None;
        if (notificationActive) return RedrawReason::Notification;
        if (visible) return RedrawReason::Visible;
        return dirty ? RedrawReason::Dirty : RedrawReason::None;
    }
}
