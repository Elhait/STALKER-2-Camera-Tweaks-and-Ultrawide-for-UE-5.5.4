#include "input_state.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace overlay
{
    InputEventBridge& GetInputEventBridge() noexcept
    {
        static InputEventBridge bridge;
        return bridge;
    }

    void VirtualCursorPosition::SetAbsolute(int x, int y, int width,
        int height) noexcept
    {
        position_.x = (std::clamp)(x, 0, (std::max)(0, width - 1));
        position_.y = (std::clamp)(y, 0, (std::max)(0, height - 1));
        known_ = true;
    }

    void VirtualCursorPosition::ApplyRelative(int x, int y, int width,
        int height) noexcept
    {
        if (!known_) SetAbsolute(width / 2, height / 2, width, height);
        const auto addAndClamp = [](LONG current, int delta, int extent) {
            const auto sum = static_cast<long long>(current) + delta;
            return static_cast<LONG>((std::clamp)(sum, 0ll,
                static_cast<long long>((std::max)(0, extent - 1))));
        };
        position_.x = addAndClamp(position_.x, x, width);
        position_.y = addAndClamp(position_.y, y, height);
        known_ = true;
    }

    void InputEventBridge::Activate(int initialX, int initialY)
    {
        std::lock_guard lock{mutex_};
        eventCount_ = 0;
        rawMouseActive_.store(false, std::memory_order_release);
        events_[eventCount_++] = {InputEventKind::Reset};
        events_[eventCount_++] = {InputEventKind::AbsoluteMousePosition, 0, 0, 0,
            initialX, initialY};
        events_[eventCount_++] = {InputEventKind::Focus, 0, TRUE};
        active_.store(true, std::memory_order_release);
    }

    void InputEventBridge::Deactivate() noexcept
    {
        active_.store(false, std::memory_order_release);
        rawMouseActive_.store(false, std::memory_order_release);
        std::lock_guard lock{mutex_};
        eventCount_ = 2;
        events_[0] = {InputEventKind::Reset};
        events_[1] = {InputEventKind::Focus, 0, FALSE};
    }

    bool InputEventBridge::Enqueue(InputEvent event, bool coalescible) noexcept
    {
        if (!active()) return false;
        try {
            std::lock_guard lock{mutex_};
            if (!active()) return false;
            if (coalescible && eventCount_ > 0 &&
                events_[eventCount_ - 1].kind == event.kind) {
                if (event.kind == InputEventKind::RelativeMouseMotion) {
                    const auto clampSum = [](int left, int right) {
                        const auto sum = static_cast<long long>(left) + right;
                        return static_cast<int>((std::clamp)(sum,
                            static_cast<long long>((std::numeric_limits<int>::min)()),
                            static_cast<long long>((std::numeric_limits<int>::max)())));
                    };
                    events_[eventCount_ - 1].x = clampSum(
                        events_[eventCount_ - 1].x, event.x);
                    events_[eventCount_ - 1].y = clampSum(
                        events_[eventCount_ - 1].y, event.y);
                } else {
                    events_[eventCount_ - 1] = event;
                }
                return true;
            }
            if (eventCount_ >= MaximumQueuedEvents) {
                std::size_t expendable = 0;
                while (expendable < eventCount_ &&
                    events_[expendable].kind != InputEventKind::AbsoluteMousePosition &&
                    events_[expendable].kind != InputEventKind::RelativeMouseMotion)
                    ++expendable;
                if (expendable < eventCount_) {
                    for (std::size_t index = expendable + 1;
                        index < eventCount_; ++index)
                        events_[index - 1] = events_[index];
                    --eventCount_;
                } else if (coalescible) {
                    return false;
                } else {
                    eventCount_ = 1;
                    events_[0] = {InputEventKind::Reset};
                }
            }
            events_[eventCount_++] = event;
            return true;
        } catch (...) {
            return false;
        }
    }

    bool InputEventBridge::PushAbsoluteMousePosition(int x, int y) noexcept
    {
        if (rawMouseActive_.load(std::memory_order_acquire)) return false;
        return Enqueue({InputEventKind::AbsoluteMousePosition, 0, 0, 0, x, y}, true);
    }

    bool InputEventBridge::PushRelativeMouseMotion(int x, int y) noexcept
    {
        // Button/key/raw packets may carry a zero mouse delta. They are not
        // evidence that this host is supplying the pointer-motion stream.
        // Keep absolute client-coordinate events eligible until real relative
        // movement arrives, otherwise one unrelated packet can freeze ImGui's
        // software cursor for the rest of the overlay session.
        if (!active() || (x == 0 && y == 0)) return false;
        rawMouseActive_.store(true, std::memory_order_release);
        return Enqueue({InputEventKind::RelativeMouseMotion, 0, 0, 0, x, y}, true);
    }

    bool InputEventBridge::PushMouseButton(UINT message, WPARAM wParam) noexcept
    {
        return Enqueue({InputEventKind::MouseButton, message, wParam}, false);
    }

    bool InputEventBridge::PushMouseWheel(UINT message, WPARAM wParam,
        LPARAM lParam) noexcept
    {
        return Enqueue({InputEventKind::MouseWheel, message, wParam, lParam}, false);
    }

    bool InputEventBridge::PushKeyboardMessage(UINT message, WPARAM wParam,
        LPARAM lParam) noexcept
    {
        return Enqueue({InputEventKind::NativeKeyboardMessage,
            message, wParam, lParam}, false);
    }

    void InputEventBridge::Drain(std::vector<InputEvent>& events)
    {
        std::lock_guard lock{mutex_};
        events.clear();
        events.reserve(eventCount_);
        events.insert(events.end(), events_.begin(),
            events_.begin() + static_cast<std::ptrdiff_t>(eventCount_));
        eventCount_ = 0;
    }

    InputState& GetInputState() noexcept
    {
        static InputState input;
        return input;
    }

    InputOwnershipSnapshot CaptureInputOwnership(const InputState& input,
        const InputEventBridge& bridge) noexcept
    {
        return {input.visible(), input.focused(), bridge.active()};
    }

    bool InputOwnershipUnchanged(const InputOwnershipSnapshot& before,
        const InputState& input, const InputEventBridge& bridge) noexcept
    {
        return before.Coherent() &&
            CaptureInputOwnership(input, bridge) == before;
    }

    void InputState::Close()
    {
        visible_.store(false, std::memory_order_release);
        focused_.store(false, std::memory_order_release);
        ResetEscapeDismissSequence();
        std::lock_guard lock(rebindMutex_);
        CancelRebindLocked();
        consumedKey_ = 0;
    }

    bool InputState::HandleToggleMessage(UINT message, WPARAM key, int toggleKey)
    {
        if (message != WM_KEYUP || key != static_cast<WPARAM>(toggleKey)) return false;
        CancelRebind();
        Toggle();
        return true;
    }

    bool InputState::ShouldDismissOnEscapeMessage(UINT message, WPARAM key,
        bool childConsumesEscape) noexcept
    {
        if (key != VK_ESCAPE) return false;
        if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) {
            if (childConsumesEscape) {
                escapeDismissPressOwnedByChild_.store(true, std::memory_order_release);
                // The window procedure cannot close a Combo outside its popup scope.
                escapePopupDismissRequested_.store(true, std::memory_order_release);
            }
            return false;
        }
        if (message != WM_KEYUP && message != WM_SYSKEYUP) return false;

        const bool childOwnedPress = escapeDismissPressOwnedByChild_.exchange(
            false, std::memory_order_acq_rel);
        return visible() && !childConsumesEscape && !childOwnedPress;
    }

    void InputState::BeginRebind(HotkeyAction action, int previousKey)
    {
        std::lock_guard lock(rebindMutex_);
        rebindAction_ = action;
        previousBinding_ = previousKey;
        lastAcceptedBinding_ = previousKey;
        rebindActive_ = true;
    }

    RebindMessageResult InputState::HandleRebindMessage(
        UINT message, WPARAM key, bool modifierDown)
    {
        if (!IsRebindMessage(message)) return RebindMessageResult::PassThrough;
        std::lock_guard lock(rebindMutex_);
        const bool keyDown = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
        const bool keyUp = message == WM_KEYUP || message == WM_SYSKEYUP;
        if (!keyDown && !keyUp) return RebindMessageResult::PassThrough;

        const int virtualKey = static_cast<int>(key);
        if (consumedKey_ != 0 && virtualKey == consumedKey_) {
            if (keyUp) consumedKey_ = 0;
            return RebindMessageResult::Consumed;
        }
        if (!rebindActive_ || !keyDown) return RebindMessageResult::PassThrough;
        if (virtualKey == VK_ESCAPE) {
            consumedKey_ = virtualKey;
            CancelRebindLocked();
            consumedKey_ = virtualKey;
            return RebindMessageResult::Cancelled;
        }
        if (modifierDown || !IsSupportedBindingKey(virtualKey))
            return RebindMessageResult::PassThrough;

        consumedKey_ = virtualKey;
        return RebindMessageResult::CapturedKey;
    }

    bool InputState::IsRebindMessage(UINT message) noexcept
    {
        return message == WM_KEYDOWN || message == WM_KEYUP ||
            message == WM_SYSKEYDOWN || message == WM_SYSKEYUP;
    }

    bool InputState::IsRebindKeyDownMessage(UINT message) noexcept
    {
        return message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
    }

    void InputState::CommitAcceptedRebind(int acceptedKey)
    {
        std::lock_guard lock(rebindMutex_);
        lastAcceptedBinding_ = acceptedKey;
        previousBinding_ = acceptedKey;
        rebindActive_ = false;
    }

    void InputState::CancelRebind()
    {
        std::lock_guard lock(rebindMutex_);
        CancelRebindLocked();
    }

    void InputState::CancelRebindLocked() noexcept
    {
        rebindActive_ = false;
        previousBinding_ = lastAcceptedBinding_;
    }

    bool InputState::rebindActive() const
    {
        std::lock_guard lock(rebindMutex_);
        return rebindActive_;
    }

    HotkeyAction InputState::rebindAction() const
    {
        std::lock_guard lock(rebindMutex_);
        return rebindAction_;
    }

    int InputState::previousBinding() const
    {
        std::lock_guard lock(rebindMutex_);
        return previousBinding_;
    }

    bool InputState::IsCapturing(HotkeyAction action) const
    {
        std::lock_guard lock(rebindMutex_);
        return rebindActive_ && rebindAction_ == action;
    }

    bool InputState::IsSupportedBindingKey(int key) noexcept
    {
        return config::IsSupportedHotkey(key);
    }

    bool InputState::ShouldCapture(UINT message) const noexcept
    {
        if (!ShouldOwnInput()) return false;
        switch (message) {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_SYSCHAR:
            return false;
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MBUTTONDBLCLK:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_XBUTTONDBLCLK:
            return true;
        default:
            return false;
        }
    }

}
