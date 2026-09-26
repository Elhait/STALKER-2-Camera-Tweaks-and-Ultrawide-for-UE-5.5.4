#pragma once

#include "discovery_evidence.hpp"

#include <cstdint>

namespace overlay
{
    inline constexpr std::uint32_t StartupHintDurationMs = 10000;

    constexpr bool StartupLocaleReady(bool autoLocaleConfigured,
        bool autoLocaleSynchronized) noexcept
    {
        return !autoLocaleConfigured || autoLocaleSynchronized;
    }

    enum class RendererState
    {
        Uninitialized,
        Ready,
        Resizing,
        Failed,
        Disabled,
    };

    const char* RendererStateName(RendererState state) noexcept;

    class RendererLifecycle
    {
    public:
        RendererState state() const noexcept { return state_; }
        bool BeginInitialization() noexcept;
        bool MarkReady() noexcept;
        bool BeginResize() noexcept;
        bool CompleteResize(bool resourcesReady) noexcept;
        bool Fail() noexcept;
        bool Disable() noexcept;
        bool ResetForRecreation() noexcept;
        bool CanSubmit(AssociationState association) const noexcept;

    private:
        RendererState state_{RendererState::Uninitialized};
    };

    class StartupHintGate
    {
    public:
        bool ShouldReadSettings(bool rendererReady, bool validPresent) const noexcept;
        bool TryPublish(bool rendererReady, bool validPresent,
            bool settingsAvailable) noexcept;

    private:
        bool published_{};
    };

    bool NotificationExpired(std::uint64_t createdAtMs,
        std::uint64_t nowMs, std::uint32_t durationMs) noexcept;
}
