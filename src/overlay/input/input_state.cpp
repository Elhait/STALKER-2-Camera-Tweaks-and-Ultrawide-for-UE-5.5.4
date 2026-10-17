#include "input_state.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace overlay
{
    OwnerInputCommandQueue::OwnerInputCommandQueue() noexcept
    {
        for (std::uint64_t index = 0; index < Capacity; ++index)
            slots_[index].sequence.store(index, std::memory_order_relaxed);
    }

    bool OwnerInputCommandQueue::TryPush(
        const OwnerInputCommand& command) noexcept
    {
        auto position = writePosition_.load(std::memory_order_relaxed);
        for (unsigned attempt = 0; attempt < 8; ++attempt) {
            auto& slot = slots_[position % Capacity];
            const auto sequence = slot.sequence.load(std::memory_order_acquire);
            if (sequence == position) {
                if (writePosition_.compare_exchange_weak(position, position + 1,
                        std::memory_order_relaxed, std::memory_order_relaxed)) {
                    slot.command = command;
                    slot.sequence.store(position + 1, std::memory_order_release);
                    return true;
                }
            } else if (sequence < position) {
                return false;
            } else {
                position = writePosition_.load(std::memory_order_relaxed);
            }
        }
        return false;
    }

    bool OwnerInputCommandQueue::TryPop(
        OwnerInputCommand& command) noexcept
    {
        const auto position = readPosition_.load(std::memory_order_relaxed);
        auto& slot = slots_[position % Capacity];
        if (slot.sequence.load(std::memory_order_acquire) != position + 1)
            return false;
        command = slot.command;
        slot.sequence.store(position + Capacity, std::memory_order_release);
        readPosition_.store(position + 1, std::memory_order_relaxed);
        return true;
    }

    bool OwnerInputCommandQueue::empty() const noexcept
    {
        return readPosition_.load(std::memory_order_acquire) ==
            writePosition_.load(std::memory_order_acquire);
    }

    bool OwnerInputCommandQueue::full() const noexcept
    {
        return size() >= Capacity;
    }

    std::uint32_t OwnerInputCommandQueue::size() const noexcept
    {
        const auto write = writePosition_.load(std::memory_order_acquire);
        const auto read = readPosition_.load(std::memory_order_acquire);
        const auto count = write - read;
        return static_cast<std::uint32_t>((std::min<std::uint64_t>)(count,
            Capacity));
    }

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
        drainedResetGeneration_ = resetGeneration_.load(
            std::memory_order_acquire);
        active_.store(true, std::memory_order_release);
    }

    bool InputEventBridge::TryActivate(int initialX, int initialY) noexcept
    {
        std::unique_lock lock{mutex_, std::try_to_lock};
        if (!lock.owns_lock()) return false;
        eventCount_ = 0;
        rawMouseActive_.store(false, std::memory_order_release);
        events_[eventCount_++] = {InputEventKind::Reset};
        events_[eventCount_++] = {InputEventKind::AbsoluteMousePosition, 0, 0, 0,
            initialX, initialY};
        events_[eventCount_++] = {InputEventKind::Focus, 0, TRUE};
        drainedResetGeneration_ = resetGeneration_.load(
            std::memory_order_acquire);
        active_.store(true, std::memory_order_release);
        return true;
    }

    void InputEventBridge::Deactivate() noexcept
    {
        active_.store(false, std::memory_order_release);
        rawMouseActive_.store(false, std::memory_order_release);
        std::lock_guard lock{mutex_};
        eventCount_ = 2;
        events_[0] = {InputEventKind::Reset};
        events_[1] = {InputEventKind::Focus, 0, FALSE};
        drainedResetGeneration_ = resetGeneration_.load(
            std::memory_order_acquire);
    }

    bool InputEventBridge::TryDeactivate() noexcept
    {
        active_.store(false, std::memory_order_release);
        rawMouseActive_.store(false, std::memory_order_release);
        std::unique_lock lock{mutex_, std::try_to_lock};
        if (!lock.owns_lock()) {
            resetGeneration_.fetch_add(1, std::memory_order_acq_rel);
            return false;
        }
        eventCount_ = 2;
        events_[0] = {InputEventKind::Reset};
        events_[1] = {InputEventKind::Focus, 0, FALSE};
        drainedResetGeneration_ = resetGeneration_.load(
            std::memory_order_acquire);
        return true;
    }

    bool InputEventBridge::Enqueue(InputEvent event, bool coalescible) noexcept
    {
        if (!active()) return false;
        try {
            std::unique_lock lock{mutex_, std::try_to_lock};
            if (!lock.owns_lock()) {
                active_.store(false, std::memory_order_release);
                rawMouseActive_.store(false, std::memory_order_release);
                resetGeneration_.fetch_add(1, std::memory_order_acq_rel);
                return false;
            }
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
                active_.store(false, std::memory_order_release);
                rawMouseActive_.store(false, std::memory_order_release);
                eventCount_ = 0;
                resetGeneration_.fetch_add(1, std::memory_order_acq_rel);
                return false;
            }
            events_[eventCount_++] = event;
            return true;
        } catch (...) {
            active_.store(false, std::memory_order_release);
            rawMouseActive_.store(false, std::memory_order_release);
            resetGeneration_.fetch_add(1, std::memory_order_acq_rel);
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
        const auto resetGeneration = resetGeneration_.load(
            std::memory_order_acquire);
        if (!active() || resetGeneration != drainedResetGeneration_) {
            events.push_back({InputEventKind::Reset});
            events.push_back({InputEventKind::Focus, 0, FALSE});
            eventCount_ = 0;
            drainedResetGeneration_ = resetGeneration;
            return;
        }
        events.reserve(eventCount_);
        events.insert(events.end(), events_.begin(),
            events_.begin() + static_cast<std::ptrdiff_t>(eventCount_));
        eventCount_ = 0;
    }

    bool InputCallbackAdmissionGate::TryEnter() noexcept
    {
        auto state = state_.load(std::memory_order_acquire);
        for (;;) {
            if (state & ClosedBit) return false;
            if ((state & CountMask) == CountMask) return false;
            if (state_.compare_exchange_weak(state, state + 1,
                    std::memory_order_acq_rel, std::memory_order_acquire))
                return true;
        }
    }

    bool InputCallbackAdmissionGate::Leave() noexcept
    {
        auto state = state_.load(std::memory_order_acquire);
        for (;;) {
            const auto count = state & CountMask;
            if (!count) return (state & ClosedBit) != 0;
            const auto next = state - 1;
            if (state_.compare_exchange_weak(state, next,
                    std::memory_order_acq_rel, std::memory_order_acquire))
                return (next & ClosedBit) && (next & CountMask) == 0;
        }
    }

    bool InputCallbackAdmissionGate::Close() noexcept
    {
        const auto previous = state_.fetch_or(ClosedBit,
            std::memory_order_acq_rel);
        return (previous & ClosedBit) == 0;
    }

    bool InputCallbackAdmissionGate::closed() const noexcept
    {
        return (state_.load(std::memory_order_acquire) & ClosedBit) != 0;
    }

    std::uint64_t InputCallbackAdmissionGate::inFlight() const noexcept
    {
        return state_.load(std::memory_order_acquire) & CountMask;
    }

    void CloseInputAdmission(InputCallbackAdmissionGate& admission,
        InputEventBridge& bridge) noexcept
    {
        admission.Close();
        bridge.TryDeactivate();
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
        std::unique_lock lock{rebindMutex_, std::try_to_lock};
        if (!lock.owns_lock()) return false;
        CancelRebindLocked();
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
        rebindActivePublished_.store(true, std::memory_order_release);
        callbackRebindKey_.store(0, std::memory_order_release);
    }

    RebindMessageResult InputState::HandleRebindMessage(
        UINT message, WPARAM key, bool modifierDown,
        HotkeyAction* capturedAction, int* capturedPreviousKey)
    {
        if (!IsRebindMessage(message)) return RebindMessageResult::PassThrough;
        std::unique_lock lock{rebindMutex_, std::try_to_lock};
        if (!lock.owns_lock()) return RebindMessageResult::PassThrough;
        const bool keyDown = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
        const bool keyUp = message == WM_KEYUP || message == WM_SYSKEYUP;
        if (!keyDown && !keyUp) return RebindMessageResult::PassThrough;

        const int virtualKey = static_cast<int>(key);
        if (consumedKey_ != 0 && virtualKey == consumedKey_) {
            if (keyUp) {
                consumedKey_ = 0;
                callbackRebindKey_.store(0, std::memory_order_release);
            }
            return RebindMessageResult::Consumed;
        }
        if (!rebindActive_ || !keyDown) return RebindMessageResult::PassThrough;
        if (virtualKey == VK_ESCAPE) {
            consumedKey_ = virtualKey;
            CancelRebindLocked();
            consumedKey_ = virtualKey;
            callbackRebindKey_.store(virtualKey, std::memory_order_release);
            return RebindMessageResult::Cancelled;
        }
        if (modifierDown || !IsSupportedBindingKey(virtualKey))
            return RebindMessageResult::PassThrough;

        if (capturedAction) *capturedAction = rebindAction_;
        if (capturedPreviousKey) *capturedPreviousKey = previousBinding_;
        consumedKey_ = virtualKey;
        callbackRebindKey_.store(virtualKey, std::memory_order_release);
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
        rebindActivePublished_.store(false, std::memory_order_release);
        callbackRebindKey_.store(0, std::memory_order_release);
    }

    bool InputState::TryCommitAcceptedRebind(int acceptedKey) noexcept
    {
        std::unique_lock lock{rebindMutex_, std::try_to_lock};
        if (!lock.owns_lock()) return false;
        lastAcceptedBinding_ = acceptedKey;
        previousBinding_ = acceptedKey;
        rebindActive_ = false;
        rebindActivePublished_.store(false, std::memory_order_release);
        callbackRebindKey_.store(0, std::memory_order_release);
        return true;
    }

    void InputState::CancelRebind()
    {
        std::lock_guard lock(rebindMutex_);
        CancelRebindLocked();
    }

    bool InputState::TryCancelRebind() noexcept
    {
        std::unique_lock lock{rebindMutex_, std::try_to_lock};
        if (!lock.owns_lock()) return false;
        CancelRebindLocked();
        consumedKey_ = 0;
        return true;
    }

    void InputState::CancelRebindLocked() noexcept
    {
        rebindActive_ = false;
        rebindActivePublished_.store(false, std::memory_order_release);
        callbackRebindKey_.store(0, std::memory_order_release);
        previousBinding_ = lastAcceptedBinding_;
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
