#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>

namespace plugin
{
    // RuntimeState-owned admission/drain barrier for native camera callbacks.
    // Admission itself is lock-free; only the reset owner waits, with a finite
    // deadline. A timed-out or self-requested drain leaves the gate closed so
    // callers can safely retain hook resources and retry from a control thread.
    class NativeCallbackAdmission
    {
        static constexpr std::uint32_t ClosedBit = 0x80000000u;
        static constexpr std::uint32_t ActiveMask = ~ClosedBit;

    public:
        enum class DrainResult { Drained, CalledFromAdmittedCallback, TimedOut };

        class Guard
        {
        public:
            Guard(const Guard&) = delete;
            Guard& operator=(const Guard&) = delete;
            Guard(Guard&&) = delete;
            Guard& operator=(Guard&&) = delete;
            ~Guard() { if (owner_) owner_->Leave(); }

            explicit operator bool() const noexcept { return owner_ != nullptr; }

        private:
            friend class NativeCallbackAdmission;
            explicit Guard(NativeCallbackAdmission* owner) noexcept : owner_(owner)
            {
                if (currentOwner_ == owner_) ++currentDepth_;
                else { currentOwner_ = owner_; currentDepth_ = 1; }
            }
            Guard() noexcept = default;
            NativeCallbackAdmission* owner_{};
        };

        NativeCallbackAdmission() = default;
        NativeCallbackAdmission(const NativeCallbackAdmission&) = delete;
        NativeCallbackAdmission& operator=(const NativeCallbackAdmission&) = delete;

        Guard TryEnter() noexcept
        {
            auto state = state_.load(std::memory_order_acquire);
            for (;;) {
                if ((state & ClosedBit) != 0 || (state & ActiveMask) == ActiveMask)
                    return Guard{};
                if (state_.compare_exchange_weak(state, state + 1,
                        std::memory_order_acq_rel, std::memory_order_acquire))
                    return Guard{this};
            }
        }

        bool Open() noexcept
        {
            auto expected = ClosedBit;
            return state_.compare_exchange_strong(expected, 0,
                std::memory_order_release, std::memory_order_relaxed);
        }

        void Close() noexcept
        {
            state_.fetch_or(ClosedBit, std::memory_order_acq_rel);
        }

        DrainResult CloseAndDrain(std::uint32_t timeoutMilliseconds) noexcept
        {
            Close();
            if (currentOwner_ == this && currentDepth_ != 0)
                return DrainResult::CalledFromAdmittedCallback;

            const auto started = std::chrono::steady_clock::now();
            while ((state_.load(std::memory_order_acquire) & ActiveMask) != 0) {
                if (std::chrono::steady_clock::now() - started >=
                    std::chrono::milliseconds(timeoutMilliseconds))
                    return DrainResult::TimedOut;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            return DrainResult::Drained;
        }

        bool IsClosed() const noexcept
        {
            return (state_.load(std::memory_order_acquire) & ClosedBit) != 0;
        }

        bool IsEnteredByCurrentThread() const noexcept
        {
            return currentOwner_ == this && currentDepth_ != 0;
        }

    private:
        void Leave() noexcept
        {
            if (currentOwner_ == this && currentDepth_ != 0 && --currentDepth_ == 0)
                currentOwner_ = nullptr;
            state_.fetch_sub(1, std::memory_order_release);
        }

        std::atomic<std::uint32_t> state_{ClosedBit};
        inline static thread_local NativeCallbackAdmission* currentOwner_{};
        inline static thread_local std::uint32_t currentDepth_{};
    };
}
