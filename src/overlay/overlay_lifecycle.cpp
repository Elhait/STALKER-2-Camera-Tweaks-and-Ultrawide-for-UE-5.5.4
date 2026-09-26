#include "overlay_lifecycle.hpp"

namespace overlay
{
    const char* StateName(State state) noexcept
    {
        switch (state) {
        case State::Unavailable: return "Unavailable";
        case State::Discovering: return "Discovering";
        case State::Ready: return "Ready";
        case State::Resizing: return "Resizing";
        case State::Failed: return "Failed";
        case State::Disabled: return "Disabled";
        }
        return "Unavailable";
    }

    bool Lifecycle::BeginDiscovery() noexcept
    {
        if (state_ == State::Disabled || state_ == State::Discovering ||
            state_ == State::Resizing)
            return false;
        state_ = State::Discovering;
        return true;
    }

    bool Lifecycle::MarkReady() noexcept
    {
        if (state_ != State::Discovering && state_ != State::Resizing)
            return false;
        state_ = State::Ready;
        return true;
    }

    bool Lifecycle::BeginResize() noexcept
    {
        if (state_ != State::Ready) return false;
        state_ = State::Resizing;
        return true;
    }

    bool Lifecycle::CompleteResize(bool resourcesReady) noexcept
    {
        if (state_ != State::Resizing) return false;
        state_ = resourcesReady ? State::Ready : State::Failed;
        return resourcesReady;
    }

    bool Lifecycle::Fail() noexcept
    {
        if (state_ == State::Disabled) return false;
        state_ = State::Failed;
        return true;
    }

    bool Lifecycle::Disable() noexcept
    {
        if (state_ == State::Disabled) return false;
        state_ = State::Disabled;
        return true;
    }

    bool Lifecycle::ResetForRecreation() noexcept
    {
        if (state_ == State::Disabled || state_ == State::Ready ||
            state_ == State::Resizing)
            return false;
        state_ = State::Discovering;
        return true;
    }

    bool Lifecycle::CanRender() const noexcept
    {
        return state_ == State::Ready;
    }
}
