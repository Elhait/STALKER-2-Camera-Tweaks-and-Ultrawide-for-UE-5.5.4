#pragma once

#include "../config/feature_config.hpp"

#include <Windows.h>

#include <atomic>
#include <array>
#include <cstdint>
#include <mutex>
#include <vector>
#include <cstddef>

namespace overlay
{
    using HotkeyAction = config::HotkeyBindingId;

    enum class InputEventKind : unsigned char
    {
        Reset,
        AbsoluteMousePosition,
        RelativeMouseMotion,
        MouseButton,
        MouseWheel,
        NativeKeyboardMessage,
        Focus,
    };

    struct InputEvent
    {
        InputEventKind kind{InputEventKind::Reset};
        UINT message{};
        WPARAM wParam{};
        LPARAM lParam{};
        int x{};
        int y{};
    };

    class VirtualCursorPosition
    {
    public:
        bool known() const noexcept { return known_; }
        POINT point() const noexcept { return position_; }
        void Reset() noexcept { known_ = false; }
        void SetAbsolute(int x, int y, int width, int height) noexcept;
        void ApplyRelative(int x, int y, int width, int height) noexcept;

    private:
        POINT position_{};
        bool known_{};
    };

    class InputEventBridge
    {
    public:
        void Activate(int initialX, int initialY);
        void Deactivate() noexcept;
        bool active() const noexcept
        {
            return active_.load(std::memory_order_acquire);
        }
        bool PushAbsoluteMousePosition(int x, int y) noexcept;
        bool PushRelativeMouseMotion(int x, int y) noexcept;
        bool PushMouseButton(UINT message, WPARAM wParam) noexcept;
        bool PushMouseWheel(UINT message, WPARAM wParam, LPARAM lParam) noexcept;
        bool PushKeyboardMessage(UINT message, WPARAM wParam,
            LPARAM lParam) noexcept;
        void Drain(std::vector<InputEvent>& events);

    private:
        bool Enqueue(InputEvent event, bool coalescible) noexcept;

        static constexpr std::size_t MaximumQueuedEvents = 256;
        std::atomic<bool> active_{};
        std::atomic<bool> rawMouseActive_{};
        std::mutex mutex_;
        std::array<InputEvent, MaximumQueuedEvents> events_{};
        std::size_t eventCount_{};
    };

    InputEventBridge& GetInputEventBridge() noexcept;

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
        bool focused() const noexcept { return focused_.load(std::memory_order_acquire); }
        bool ShouldOwnInput() const noexcept { return visible() && focused(); }
        void SetFocused(bool focused) noexcept
        {
            focused_.store(focused, std::memory_order_release);
        }
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

        static bool IsSupportedBindingKey(int key) noexcept;

    private:
        void CancelRebindLocked() noexcept;

        std::atomic<bool> visible_{};
        std::atomic<bool> focused_{};
        std::atomic<bool> escapeDismissPressOwnedByChild_{};
        std::atomic<bool> escapePopupDismissRequested_{};
        mutable std::mutex rebindMutex_;
        bool rebindActive_{};
        HotkeyAction rebindAction_{HotkeyAction::GameplayMode};
        int previousBinding_{};
        int lastAcceptedBinding_{};
        int consumedKey_{};
    };

    struct InputOwnershipSnapshot
    {
        bool panelVisible{};
        bool hostFocused{};
        bool bridgeActive{};

        bool Coherent() const noexcept
        {
            return bridgeActive == (panelVisible && hostFocused);
        }
        friend bool operator==(const InputOwnershipSnapshot&,
            const InputOwnershipSnapshot&) = default;
    };

    InputOwnershipSnapshot CaptureInputOwnership(const InputState& input,
        const InputEventBridge& bridge) noexcept;
    bool InputOwnershipUnchanged(const InputOwnershipSnapshot& before,
        const InputState& input, const InputEventBridge& bridge) noexcept;

    InputState& GetInputState() noexcept;
}
