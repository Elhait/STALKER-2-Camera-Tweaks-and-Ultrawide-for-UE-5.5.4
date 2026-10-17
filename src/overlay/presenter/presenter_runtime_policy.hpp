#pragma once

#include <cstdint>
#include <atomic>
#include <utility>

namespace overlay
{
    constexpr bool ResolveHostDisplaySnapshot(bool validHost,
        bool foregroundIsHost, bool minimized) noexcept
    {
        return validHost && foregroundIsHost && !minimized;
    }

    // Owner-thread state: a visible toast is not proof of a drawn menu.
    class MenuFrameAdmission
    {
    public:
        void Request(bool visible) noexcept
        {
            if (requested_ != visible) Invalidate();
            requested_ = visible;
        }
        void Invalidate() noexcept { drawn_ = committed_ = false; }
        void DrawCompleted(bool menuDrawn) noexcept { drawn_ = requested_ && menuDrawn; }
        void Commit(bool displayReady) noexcept
        {
            committed_ = requested_ && drawn_ && displayReady;
        }
        bool ready() const noexcept { return committed_; }
    private:
        bool requested_{};
        bool drawn_{};
        bool committed_{};
    };

    struct DisplayPlacement
    {
        int x{};
        int y{};
        int width{};
        int height{};
        unsigned dpi{96};
        bool topmost{};
        bool valid{};
    };

    enum class HostDisplayActivityEvent : std::uint8_t
    {
        KeyboardFocusLost,
        ApplicationDeactivated,
        KeyboardFocusGained,
        ApplicationActivated,
    };

    enum class HostDisplayActivityAction : std::uint8_t
    {
        Preserve,
        Deactivate,
        Activate,
    };

    constexpr HostDisplayActivityAction ResolveHostDisplayActivity(
        HostDisplayActivityEvent event) noexcept
    {
        switch (event) {
        case HostDisplayActivityEvent::KeyboardFocusLost:
            return HostDisplayActivityAction::Preserve;
        case HostDisplayActivityEvent::ApplicationDeactivated:
            return HostDisplayActivityAction::Deactivate;
        case HostDisplayActivityEvent::KeyboardFocusGained:
        case HostDisplayActivityEvent::ApplicationActivated:
            return HostDisplayActivityAction::Activate;
        }
        return HostDisplayActivityAction::Preserve;
    }

    constexpr bool RequiresDisplayPlacementUpdate(
        const DisplayPlacement& current,
        const DisplayPlacement& requested) noexcept
    {
        return !current.valid || current.x != requested.x ||
            current.y != requested.y || current.width != requested.width ||
            current.height != requested.height || current.dpi != requested.dpi ||
            current.topmost != requested.topmost;
    }

    template <typename ApplyChange, typename ObserveState>
    bool CommitDisplayWindowVisibility(bool requestedVisible,
        ApplyChange&& applyChange, ObserveState&& observeState)
    {
        if (!std::forward<ApplyChange>(applyChange)()) return false;
        return static_cast<bool>(std::forward<ObserveState>(observeState)()) ==
            requestedVisible;
    }

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

    // Coalesces owner-thread presenter reconciliation requests independently
    // from the message queue; the owner event remains the durable wake source.
    class PresenterRefreshSignal
    {
    public:
        void Request() noexcept
        {
            pending_.store(true, std::memory_order_release);
        }

        bool Take() noexcept
        {
            return pending_.exchange(false, std::memory_order_acq_rel);
        }

        bool pending() const noexcept
        {
            return pending_.load(std::memory_order_acquire);
        }

    private:
        std::atomic<bool> pending_{};
    };

    inline bool TryScheduleVisibleCameraStateRedraw(bool overlayVisible,
        PresenterWakeGate& wakeGate) noexcept
    {
        return overlayVisible && wakeGate.TrySchedule();
    }
}
