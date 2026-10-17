#pragma once

#include "../../config/feature_config.hpp"

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

    struct OwnerInputCommand
    {
        HWND window{};
        std::uint64_t generation{};
        UINT message{};
        WPARAM wParam{};
        LPARAM lParam{};
        std::uintptr_t bindingIdentity{};
        bool finishAfterContinuation{};
        bool hasLocaleResult{};
        bool localeReadSucceeded{};
        bool startupGameDataProbe{};
        std::array<char, 64> localeCode{};
        bool rebindModifierDown{};
    };

    // Bounded multi-producer queue for overlapping old/new GameHwnd callbacks
    // during rebind. The single Overlay owner is the only consumer. Producers
    // have a bounded number of CAS attempts and never wait for the owner.
    class OwnerInputCommandQueue
    {
    public:
        OwnerInputCommandQueue() noexcept;
        bool TryPush(const OwnerInputCommand& command) noexcept;
        bool TryPop(OwnerInputCommand& command) noexcept;
        bool empty() const noexcept;
        bool full() const noexcept;
        std::uint32_t size() const noexcept;

    private:
        static constexpr std::uint32_t Capacity = 256;
        struct Slot
        {
            std::atomic<std::uint64_t> sequence{};
            OwnerInputCommand command{};
        };
        std::array<Slot, Capacity> slots_{};
        std::atomic<std::uint64_t> writePosition_{};
        std::atomic<std::uint64_t> readPosition_{};
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
        bool TryActivate(int initialX, int initialY) noexcept;
        void Deactivate() noexcept;
        bool TryDeactivate() noexcept;
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
        std::atomic<std::uint64_t> resetGeneration_{};
        std::uint64_t drainedResetGeneration_{};
        std::mutex mutex_;
        std::array<InputEvent, MaximumQueuedEvents> events_{};
        std::size_t eventCount_{};
    };

    // Admission for callbacks that may touch owner-managed overlay state.
    // Closing is terminal; callers already admitted may finish, but new calls
    // are rejected. The owner can use inFlight() to drain admitted callbacks.
    class InputCallbackAdmissionGate
    {
    public:
        bool TryEnter() noexcept;
        bool Leave() noexcept;
        bool Close() noexcept;
        bool closed() const noexcept;
        std::uint64_t inFlight() const noexcept;

    private:
        static constexpr std::uint64_t ClosedBit = std::uint64_t{1} << 63;
        static constexpr std::uint64_t CountMask = ~ClosedBit;
        std::atomic<std::uint64_t> state_{};
    };

    // Shared terminal transition used by the production owner request and its
    // failure-path harness. It closes admission before the try-only bridge
    // deactivation; it never waits for the Overlay owner.
    void CloseInputAdmission(InputCallbackAdmissionGate& admission,
        InputEventBridge& bridge) noexcept;

    // A nonblocking lease used by the game-window callback to fail open when
    // the Overlay owner stops servicing a visible interactive surface.
    class InputOwnerHeartbeat
    {
    public:
        void Publish(std::uint64_t monotonicMilliseconds) noexcept
        {
            lastProgress_.store(monotonicMilliseconds,
                std::memory_order_release);
        }
        void Reset() noexcept
        {
            lastProgress_.store(0, std::memory_order_release);
        }
        bool IsFresh(std::uint64_t nowMilliseconds,
            std::uint64_t maximumAgeMilliseconds) const noexcept
        {
            const auto last = lastProgress_.load(std::memory_order_acquire);
            return last != 0 && nowMilliseconds >= last &&
                nowMilliseconds - last <= maximumAgeMilliseconds;
        }

    private:
        std::atomic<std::uint64_t> lastProgress_{};
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
            bool modifierDown = false, HotkeyAction* capturedAction = nullptr,
            int* capturedPreviousKey = nullptr);
        static bool IsRebindMessage(UINT message) noexcept;
        static bool IsRebindKeyDownMessage(UINT message) noexcept;
        void CommitAcceptedRebind(int acceptedKey);
        bool TryCommitAcceptedRebind(int acceptedKey) noexcept;
        void CancelRebind();
        bool TryCancelRebind() noexcept;
        bool rebindActive() const noexcept
        {
            return rebindActivePublished_.load(std::memory_order_acquire);
        }
        void PublishCallbackRebindKey(int key) noexcept
        {
            callbackRebindKey_.store(key, std::memory_order_release);
        }
        int callbackRebindKey() const noexcept
        {
            return callbackRebindKey_.load(std::memory_order_acquire);
        }
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
        std::atomic<bool> rebindActivePublished_{};
        std::atomic<int> callbackRebindKey_{};
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
