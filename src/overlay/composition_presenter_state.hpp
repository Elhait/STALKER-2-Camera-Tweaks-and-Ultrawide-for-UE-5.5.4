#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>

namespace overlay
{
    struct SurfaceExtent
    {
        std::uint32_t width{};
        std::uint32_t height{};
        friend bool operator==(const SurfaceExtent&, const SurfaceExtent&) = default;
    };

    struct SurfaceUpdateToken
    {
        std::uint64_t expectedGeneration{};
        std::uint64_t nextGeneration{};
        SurfaceExtent extent{};
    };

    enum class PresenterPhase : std::uint8_t
    {
        WaitingForCore,
        WaitingForWindow,
        Ready,
        Minimized,
        RecoveringDevice,
        Disabled,
    };

    class CompositionPresenterState
    {
    public:
        PresenterPhase phase() const noexcept { return phase_; }
        std::uint64_t generation() const noexcept { return generation_; }
        SurfaceExtent extent() const noexcept { return extent_; }
        bool cameraCoreReady() const noexcept { return cameraCoreReady_; }

        void MarkCameraCoreReady() noexcept;
        bool MarkWindowAvailable(SurfaceExtent extent) noexcept;
        bool BeginSurfaceUpdate(SurfaceExtent extent,
            SurfaceUpdateToken& token) const noexcept;
        bool PublishSurfaceUpdate(const SurfaceUpdateToken& token,
            bool fullSurfaceDrawn, bool compositionCommitted) noexcept;
        void SetMinimized(bool minimized) noexcept;
        bool BeginDeviceRecovery() noexcept;
        bool CompleteDeviceRecovery(bool surfaceCommitted,
            SurfaceExtent extent) noexcept;
        void DisableOverlay() noexcept;

    private:
        PresenterPhase phase_{PresenterPhase::WaitingForCore};
        std::uint64_t generation_{};
        SurfaceExtent extent_{};
        bool cameraCoreReady_{};
        bool minimized_{};
        bool deviceRecoveryAttempted_{};
    };

    template <typename Resource>
    bool PublishCommittedSurfaceResource(CompositionPresenterState& state,
        const SurfaceUpdateToken& token, Resource& active, Resource&& staged,
        bool fullSurfaceDrawn, bool compositionCommitted) noexcept(
            std::is_nothrow_move_assignable_v<Resource>)
    {
        if (!state.PublishSurfaceUpdate(token, fullSurfaceDrawn,
                compositionCommitted))
            return false;
        active = std::move(staged);
        return true;
    }

    enum class RedrawReason : std::uint8_t
    {
        None,
        Dirty,
        Visible,
        Notification,
    };

    RedrawReason SelectRedrawReason(bool ready, bool dirty, bool visible,
        bool notificationActive) noexcept;
}
