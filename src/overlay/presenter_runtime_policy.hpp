#pragma once

#include <cstdint>
#include <atomic>

namespace overlay
{
    enum class PresenterRoute : std::uint8_t
    {
        WaitForWindow,
        Initialize,
        RebindWindow,
        UpdateGeometry,
        Idle,
    };

    enum class PresenterFrameState : std::uint8_t
    {
        Idle,
        NotificationAnimation,
        InteractiveUi,
        EventDrivenRedraw,
    };

    constexpr PresenterFrameState SelectPresenterFrameState(bool ready,
        bool minimized, bool notificationActive, bool redrawPending,
        bool interactive) noexcept
    {
        if (!ready || minimized) return PresenterFrameState::Idle;
        if (notificationActive) return PresenterFrameState::NotificationAnimation;
        if (redrawPending) return PresenterFrameState::EventDrivenRedraw;
        if (interactive) return PresenterFrameState::InteractiveUi;
        return PresenterFrameState::Idle;
    }

    constexpr bool NeedsCompositorClock(PresenterFrameState state) noexcept
    {
        return state == PresenterFrameState::NotificationAnimation ||
            state == PresenterFrameState::EventDrivenRedraw;
    }

    constexpr bool NeedsFallbackAnimationTimer(
        PresenterFrameState state) noexcept
    {
        return state == PresenterFrameState::NotificationAnimation;
    }

    constexpr PresenterRoute SelectPresenterRoute(bool hasSurfaceGeneration,
        bool ownsCandidateWindow, bool minimized,
        bool windowGeometryEvent) noexcept
    {
        if (!hasSurfaceGeneration)
            return minimized ? PresenterRoute::WaitForWindow
                             : PresenterRoute::Initialize;
        if (!ownsCandidateWindow) return PresenterRoute::RebindWindow;
        if (minimized || windowGeometryEvent)
            return PresenterRoute::UpdateGeometry;
        return PresenterRoute::Idle;
    }

    class PresenterTimerIdentity
    {
    public:
        constexpr bool active() const noexcept { return id_ != 0; }
        constexpr std::uintptr_t id() const noexcept { return id_; }
        constexpr std::uint32_t intervalMs() const noexcept { return intervalMs_; }

        constexpr bool NeedsArm(std::uint32_t intervalMs) const noexcept
        {
            return !active() || intervalMs_ != intervalMs;
        }

        constexpr bool Matches(std::uintptr_t messageId) const noexcept
        {
            return active() && id_ == messageId;
        }

        constexpr void Arm(std::uintptr_t returnedId,
            std::uint32_t intervalMs) noexcept
        {
            id_ = returnedId;
            intervalMs_ = returnedId ? intervalMs : 0;
        }

        constexpr std::uintptr_t TakeForCancellation() noexcept
        {
            const auto previous = id_;
            id_ = 0;
            intervalMs_ = 0;
            return previous;
        }

    private:
        std::uintptr_t id_{};
        std::uint32_t intervalMs_{};
    };

    class PresenterWakeGate
    {
    public:
        bool TrySchedule() noexcept
        {
            bool expected = false;
            return pending_.compare_exchange_strong(expected, true,
                std::memory_order_acq_rel);
        }

        void BeginHandling() noexcept
        {
            pending_.store(false, std::memory_order_release);
        }

        void CancelFailedPost() noexcept
        {
            pending_.store(false, std::memory_order_release);
        }

        bool pending() const noexcept
        {
            return pending_.load(std::memory_order_acquire);
        }

    private:
        std::atomic<bool> pending_{};
    };
}
