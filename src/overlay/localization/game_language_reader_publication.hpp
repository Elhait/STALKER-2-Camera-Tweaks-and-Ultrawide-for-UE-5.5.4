#pragma once

#include <atomic>

namespace overlay::detail
{
    // One callback may initialize native reader functions. Other GameHwnd
    // callbacks never wait: they either observe the complete immutable pair
    // or use the existing safe fallback while initialization is in progress.
    template <typename ReaderFunction, typename FreeFunction>
    class GameLanguageReaderPublication
    {
    public:
        struct FunctionBundle
        {
            ReaderFunction reader{};
            FreeFunction free{};
        };

        bool TryBeginInitialization() noexcept
        {
            auto expected = State::Uninitialized;
            return state_.compare_exchange_strong(expected, State::Initializing,
                std::memory_order_acq_rel, std::memory_order_acquire);
        }

        bool TryPublish(FunctionBundle functions) noexcept
        {
            if (!functions.reader || !functions.free ||
                state_.load(std::memory_order_relaxed) != State::Initializing)
                return false;
            functions_ = functions;
            state_.store(State::Ready, std::memory_order_release);
            return true;
        }

        void RetryAfterFailure() noexcept
        {
            state_.store(State::Uninitialized, std::memory_order_release);
        }

        bool TryRead(FunctionBundle& functions) const noexcept
        {
            if (state_.load(std::memory_order_acquire) != State::Ready)
                return false;
            functions = functions_;
            return functions.reader && functions.free;
        }

    private:
        enum class State : unsigned char { Uninitialized, Initializing, Ready };
        FunctionBundle functions_{};
        std::atomic<State> state_{State::Uninitialized};
    };
}
