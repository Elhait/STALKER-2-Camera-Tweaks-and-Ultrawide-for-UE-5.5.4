#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace overlay
{
    // DXGI wrappers can synchronously route ResizeBuffers1 through
    // ResizeBuffers. Track that call stack per thread without holding a lock
    // across the native DXGI call. Concurrent resize calls on other threads
    // receive independent slots.
    class DxgiResizeNesting
    {
    public:
        static constexpr std::size_t Capacity = 16;

        bool Begin(std::uintptr_t identity) noexcept
        {
            if (!identity || entered_) return false;
            for (auto& slot : Slots()) {
                if (slot.identity == identity) {
                    ++slot.depth;
                    identity_ = identity;
                    entered_ = true;
                    outermost_ = false;
                    return true;
                }
            }
            for (auto& slot : Slots()) {
                if (!slot.identity) {
                    slot = {identity, 1};
                    identity_ = identity;
                    entered_ = true;
                    outermost_ = true;
                    return true;
                }
            }
            return false;
        }

        bool outermost() const noexcept { return entered_ && outermost_; }

        // Returns true only when this scope closed the outermost native call.
        bool End() noexcept
        {
            if (!entered_) return false;
            for (auto& slot : Slots()) {
                if (slot.identity != identity_) continue;
                entered_ = false;
                const bool complete = --slot.depth == 0;
                if (complete) slot = {};
                return complete;
            }
            entered_ = false;
            return false;
        }

        ~DxgiResizeNesting() { End(); }

    private:
        struct Slot { std::uintptr_t identity{}; std::size_t depth{}; };
        static std::array<Slot, Capacity>& Slots() noexcept
        {
            static thread_local std::array<Slot, Capacity> slots{};
            return slots;
        }

        std::uintptr_t identity_{};
        bool entered_{};
        bool outermost_{};
    };
}
