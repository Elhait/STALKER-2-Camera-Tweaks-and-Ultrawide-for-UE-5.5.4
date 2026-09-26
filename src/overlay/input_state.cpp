#include "input_state.hpp"

namespace overlay
{
    InputState& GetInputState() noexcept
    {
        static InputState input;
        return input;
    }

    void InputState::Close()
    {
        visible_.store(false, std::memory_order_release);
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
        if (!visible()) return false;
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

    bool InputState::ShouldCaptureRawInput(bool isMouse) const noexcept
    {
        return visible() && isMouse;
    }
}
