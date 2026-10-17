#pragma once

#include <cstdint>
#include <cstddef>
#include <limits>
#include <span>

namespace overlay
{
    inline constexpr std::uint32_t StartupHintDurationMs = 10000;
    inline constexpr std::uint32_t StartupGameDataProbeTimeoutMs = 3000;

    class StartupToastReadinessGate
    {
    public:
        bool MarkRuntimeInitialized() noexcept
        {
            const bool changed = !runtimeInitialized_;
            runtimeInitialized_ = true;
            return changed;
        }
        bool MarkOverlayReady() noexcept
        {
            const bool changed = !overlayReady_;
            overlayReady_ = true;
            return changed;
        }
        bool BeginGameDataProbe(std::uint64_t nowMs,
            std::uint32_t timeoutMs) noexcept
        {
            if (!runtimeInitialized_ || !overlayReady_ || probeStarted_ ||
                gameDataObserved_ || fallbackResolved_ || !timeoutMs)
                return false;
            probeStarted_ = true;
            probePending_ = true;
            probeDeadlineMs_ = nowMs <=
                ((std::numeric_limits<std::uint64_t>::max)() -
                    static_cast<std::uint64_t>(timeoutMs))
                ? nowMs + timeoutMs
                : (std::numeric_limits<std::uint64_t>::max)();
            return true;
        }
        bool ResolveGameDataProbe(bool observed) noexcept
        {
            if (observed) {
                const bool changed = !gameDataObserved_;
                gameDataObserved_ = true;
                probePending_ = false;
                return changed;
            }
            return ResolveFallback();
        }
        bool ResolveFallback() noexcept
        {
            if (fallbackResolved_ || gameDataObserved_) return false;
            fallbackResolved_ = true;
            probePending_ = false;
            return true;
        }
        bool ResolveProbeTimeout(std::uint64_t nowMs) noexcept
        {
            return probePending_ && nowMs >= probeDeadlineMs_ && ResolveFallback();
        }
        bool runtimeInitialized() const noexcept { return runtimeInitialized_; }
        bool overlayReady() const noexcept { return overlayReady_; }
        bool gameDataObserved() const noexcept { return gameDataObserved_; }
        bool fallbackResolved() const noexcept { return fallbackResolved_; }
        bool probeStarted() const noexcept { return probeStarted_; }
        bool probePending() const noexcept { return probePending_; }
        bool eligible() const noexcept
        {
            return runtimeInitialized_ && overlayReady_ &&
                (gameDataObserved_ || fallbackResolved_);
        }
        std::uint32_t ProbeWaitMilliseconds(std::uint64_t nowMs) const noexcept
        {
            constexpr auto NoDeadline = (std::numeric_limits<std::uint32_t>::max)();
            if (!probePending_) return NoDeadline;
            if (nowMs >= probeDeadlineMs_) return 0;
            const auto remaining = probeDeadlineMs_ - nowMs;
            return remaining >= NoDeadline ? NoDeadline - 1
                : static_cast<std::uint32_t>(remaining);
        }

    private:
        bool runtimeInitialized_{};
        bool overlayReady_{};
        bool probeStarted_{};
        bool probePending_{};
        bool gameDataObserved_{};
        bool fallbackResolved_{};
        std::uint64_t probeDeadlineMs_{};
    };

    constexpr bool StartupHintMustWaitForAutoLocale(bool autoLocale,
        bool autoLocaleSynchronized, bool syncRequestAccepted) noexcept
    {
        return autoLocale && !autoLocaleSynchronized && syncRequestAccepted;
    }

    constexpr bool CompositionRootRequired(bool menuRequested,
        std::size_t activeNotificationCount, bool startupNotificationPending) noexcept
    {
        return menuRequested || activeNotificationCount != 0 ||
            startupNotificationPending;
    }

    class StartupNotificationState
    {
    public:
        bool pending() const noexcept { return pending_; }
        bool ShouldWakePresenter(bool presenterReady,
            bool inputBindingReady = true) const noexcept
        {
            return pending_ && presenterReady && inputBindingReady;
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
        std::uint64_t visibleAtMs() const noexcept { return visibleAtMs_; }
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
