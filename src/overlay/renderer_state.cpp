#include "renderer_state.hpp"

namespace overlay
{
    const char* RendererStateName(RendererState state) noexcept
    {
        switch (state) {
        case RendererState::Uninitialized: return "Uninitialized";
        case RendererState::Ready: return "Ready";
        case RendererState::Resizing: return "Resizing";
        case RendererState::Failed: return "Failed";
        case RendererState::Disabled: return "Disabled";
        }
        return "Uninitialized";
    }

    bool RendererLifecycle::BeginInitialization() noexcept
    {
        if (state_ == RendererState::Disabled || state_ == RendererState::Ready ||
            state_ == RendererState::Resizing)
            return false;
        state_ = RendererState::Uninitialized;
        return true;
    }

    bool RendererLifecycle::MarkReady() noexcept
    {
        if (state_ != RendererState::Uninitialized && state_ != RendererState::Resizing)
            return false;
        state_ = RendererState::Ready;
        return true;
    }

    bool RendererLifecycle::BeginResize() noexcept
    {
        if (state_ != RendererState::Ready) return false;
        state_ = RendererState::Resizing;
        return true;
    }

    bool RendererLifecycle::CompleteResize(bool resourcesReady) noexcept
    {
        if (state_ != RendererState::Resizing) return false;
        state_ = resourcesReady ? RendererState::Ready : RendererState::Failed;
        return resourcesReady;
    }

    bool RendererLifecycle::Fail() noexcept
    {
        if (state_ == RendererState::Disabled) return false;
        state_ = RendererState::Failed;
        return true;
    }

    bool RendererLifecycle::Disable() noexcept
    {
        if (state_ == RendererState::Disabled) return false;
        state_ = RendererState::Disabled;
        return true;
    }

    bool RendererLifecycle::ResetForRecreation() noexcept
    {
        if (state_ == RendererState::Disabled || state_ == RendererState::Ready ||
            state_ == RendererState::Resizing)
            return false;
        state_ = RendererState::Uninitialized;
        return true;
    }

    bool RendererLifecycle::CanSubmit(AssociationState association) const noexcept
    {
        return state_ == RendererState::Ready && association == AssociationState::Supported;
    }

    bool StartupHintGate::TryPublish(bool rendererReady, bool validPresent,
        bool settingsAvailable) noexcept
    {
        if (published_ || !rendererReady || !validPresent || !settingsAvailable)
            return false;
        published_ = true;
        return true;
    }

    bool StartupHintGate::ShouldReadSettings(bool rendererReady,
        bool validPresent) const noexcept
    {
        return !published_ && rendererReady && validPresent;
    }

    bool NotificationExpired(std::uint64_t createdAtMs,
        std::uint64_t nowMs, std::uint32_t durationMs) noexcept
    {
        return nowMs >= createdAtMs && nowMs - createdAtMs >= durationMs;
    }
}
