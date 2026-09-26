#pragma once

#include "../config/feature_config.hpp"

#include <Windows.h>

#include <atomic>
#include <mutex>

namespace overlay
{
    using HotkeyAction = config::HotkeyBindingId;

    enum class RebindMessageResult : unsigned char
    {
        PassThrough,
        Consumed,
        Cancelled,
        CapturedKey,
    };

    class InputState
    {
    public:
        bool visible() const noexcept { return visible_.load(std::memory_order_acquire); }
        void Close();
        bool Toggle() noexcept
        {
            const bool next = !visible();
            visible_.store(next, std::memory_order_release);
            return next;
        }

        bool HandleToggleMessage(UINT message, WPARAM key, int toggleKey = VK_DELETE);
        bool ShouldConsumeEscapeFromGame(UINT message, WPARAM key) const noexcept
        {
            return visible() && key == VK_ESCAPE && IsRebindMessage(message);
        }
        bool ShouldDismissOnEscapeMessage(UINT message, WPARAM key,
            bool childConsumesEscape) noexcept;
        bool ConsumeEscapePopupDismissRequest() noexcept
        {
            return escapePopupDismissRequested_.exchange(false,
                std::memory_order_acq_rel);
        }
        void ResetEscapeDismissSequence() noexcept
        {
            escapeDismissPressOwnedByChild_.store(false, std::memory_order_release);
            escapePopupDismissRequested_.store(false, std::memory_order_release);
        }
        void BeginRebind(HotkeyAction action, int previousKey);
        RebindMessageResult HandleRebindMessage(UINT message, WPARAM key,
            bool modifierDown = false);
        static bool IsRebindMessage(UINT message) noexcept;
        static bool IsRebindKeyDownMessage(UINT message) noexcept;
        void CommitAcceptedRebind(int acceptedKey);
        void CancelRebind();
        bool rebindActive() const;
        HotkeyAction rebindAction() const;
        int previousBinding() const;
        bool IsCapturing(HotkeyAction action) const;
        bool ShouldCapture(UINT message) const noexcept;
        bool ShouldCaptureRawInput(bool isMouse) const noexcept;

        static bool IsSupportedBindingKey(int key) noexcept;

    private:
        void CancelRebindLocked() noexcept;

        std::atomic<bool> visible_{};
        std::atomic<bool> escapeDismissPressOwnedByChild_{};
        std::atomic<bool> escapePopupDismissRequested_{};
        mutable std::mutex rebindMutex_;
        bool rebindActive_{};
        HotkeyAction rebindAction_{HotkeyAction::GameplayMode};
        int previousBinding_{};
        int lastAcceptedBinding_{};
        int consumedKey_{};
    };

    InputState& GetInputState() noexcept;
}
