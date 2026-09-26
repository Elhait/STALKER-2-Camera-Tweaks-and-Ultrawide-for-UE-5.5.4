#pragma once

#include <cstdint>

namespace overlay
{
    enum class State : std::uint8_t
    {
        Unavailable,
        Discovering,
        Ready,
        Resizing,
        Failed,
        Disabled,
    };

    const char* StateName(State state) noexcept;

    class Lifecycle
    {
    public:
        State state() const noexcept { return state_; }
        bool BeginDiscovery() noexcept;
        bool MarkReady() noexcept;
        bool BeginResize() noexcept;
        bool CompleteResize(bool resourcesReady) noexcept;
        bool Fail() noexcept;
        bool Disable() noexcept;
        bool ResetForRecreation() noexcept;
        bool CanRender() const noexcept;

    private:
        State state_{State::Unavailable};
    };
}
