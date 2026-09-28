#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace overlay
{
    class FactoryCreationScope
    {
    public:
        FactoryCreationScope() noexcept
        {
            if (state_.depth++ == 0) state_.count = 0;
        }

        FactoryCreationScope(const FactoryCreationScope&) = delete;
        FactoryCreationScope& operator=(const FactoryCreationScope&) = delete;

        ~FactoryCreationScope()
        {
            if (state_.depth && --state_.depth == 0) state_.count = 0;
        }

        // Returns true only when the same canonical COM identity was already
        // observed in this nested factory-creation call chain. At capacity we
        // fail open and allow another observation instead of dropping a target.
        static bool AlreadyObserved(std::uintptr_t identity) noexcept
        {
            if (!identity || !state_.depth) return false;
            for (std::size_t index = 0; index < state_.count; ++index)
                if (state_.identities[index] == identity) return true;
            if (state_.count < state_.identities.size())
                state_.identities[state_.count++] = identity;
            return false;
        }

    private:
        struct State
        {
            std::array<std::uintptr_t, 16> identities{};
            std::size_t count{};
            std::size_t depth{};
        };
        inline static thread_local State state_{};
    };
}
