#pragma once

#include <cstdint>
#include <cstddef>
#include <span>

namespace overlay
{
    inline constexpr std::uint32_t StartupHintDurationMs = 10000;

    constexpr bool StartupHintMustWaitForAutoLocale(bool autoLocale,
        bool autoLocaleSynchronized, bool syncRequestAccepted) noexcept
    {
        return autoLocale && !autoLocaleSynchronized && syncRequestAccepted;
    }

    class StartupNotificationState
    {
    public:
        bool pending() const noexcept { return pending_; }
        bool ShouldWakePresenter(bool presenterReady) const noexcept
        {
            return pending_ && presenterReady;
        }
        bool MarkCreated(bool presenterReady,
            bool settingsAvailable) noexcept;

    private:
        bool pending_{true};
    };

    bool NotificationExpired(std::uint64_t createdAtMs,
        std::uint64_t nowMs, std::uint32_t durationMs) noexcept;

    class NotificationLifetime
    {
    public:
        bool awaitingFirstCommit() const noexcept { return !visibleAtMs_; }
        bool IsDrawable(std::uint64_t nowMs,
            std::uint32_t durationMs) const noexcept;
        bool IsExpired(std::uint64_t nowMs,
            std::uint32_t durationMs) const noexcept;
        std::uint64_t AgeMs(std::uint64_t nowMs) const noexcept;
        bool StartAfterCommit(std::uint64_t nowMs) noexcept;

    private:
        std::uint64_t visibleAtMs_{};
    };

    bool NotificationDrawWasSubmitted(std::uint64_t notificationId,
        std::span<const std::uint64_t> submittedNotificationIds) noexcept;

    struct NotificationDrawListEvidence
    {
        std::uintptr_t drawListToken{};
        std::size_t vertexCount{};
    };

    bool NotificationDrawListWasSubmitted(std::uintptr_t candidateDrawList,
        std::span<const NotificationDrawListEvidence> submittedDrawLists) noexcept;

    bool StartNotificationLifetimeAfterCommit(NotificationLifetime& lifetime,
        std::uint64_t notificationId,
        std::span<const std::uint64_t> submittedNotificationIds,
        bool compositionCommitted, std::uint64_t committedAtMs) noexcept;
}
