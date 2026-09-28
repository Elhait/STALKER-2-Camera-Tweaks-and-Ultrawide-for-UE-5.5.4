#include "renderer_state.hpp"

namespace overlay
{
    bool StartupNotificationState::MarkCreated(bool presenterReady,
        bool settingsAvailable) noexcept
    {
        if (!pending_ || !presenterReady || !settingsAvailable) return false;
        pending_ = false;
        return true;
    }

    bool NotificationExpired(std::uint64_t createdAtMs,
        std::uint64_t nowMs, std::uint32_t durationMs) noexcept
    {
        return nowMs >= createdAtMs && nowMs - createdAtMs >= durationMs;
    }

    bool NotificationLifetime::IsDrawable(std::uint64_t nowMs,
        std::uint32_t durationMs) const noexcept
    {
        return awaitingFirstCommit() || !NotificationExpired(
            visibleAtMs_, nowMs, durationMs);
    }

    bool NotificationLifetime::IsExpired(std::uint64_t nowMs,
        std::uint32_t durationMs) const noexcept
    {
        return !awaitingFirstCommit() && NotificationExpired(
            visibleAtMs_, nowMs, durationMs);
    }

    std::uint64_t NotificationLifetime::AgeMs(std::uint64_t nowMs) const noexcept
    {
        if (awaitingFirstCommit()) return 220;
        return nowMs >= visibleAtMs_ ? nowMs - visibleAtMs_ : 0;
    }

    bool NotificationLifetime::StartAfterCommit(std::uint64_t nowMs) noexcept
    {
        if (!awaitingFirstCommit()) return false;
        visibleAtMs_ = nowMs ? nowMs : 1;
        return true;
    }

    bool NotificationDrawWasSubmitted(std::uint64_t notificationId,
        std::span<const std::uint64_t> submittedNotificationIds) noexcept
    {
        for (const auto submittedId : submittedNotificationIds)
            if (submittedId == notificationId) return true;
        return false;
    }

    bool NotificationDrawListWasSubmitted(std::uintptr_t candidateDrawList,
        std::span<const NotificationDrawListEvidence> submittedDrawLists) noexcept
    {
        if (!candidateDrawList) return false;
        for (const auto& submitted : submittedDrawLists)
            if (submitted.drawListToken == candidateDrawList &&
                submitted.vertexCount > 0)
                return true;
        return false;
    }

    bool StartNotificationLifetimeAfterCommit(NotificationLifetime& lifetime,
        std::uint64_t notificationId,
        std::span<const std::uint64_t> submittedNotificationIds,
        bool compositionCommitted, std::uint64_t committedAtMs) noexcept
    {
        return compositionCommitted &&
            NotificationDrawWasSubmitted(notificationId,
                submittedNotificationIds) &&
            lifetime.StartAfterCommit(committedAtMs);
    }
}
