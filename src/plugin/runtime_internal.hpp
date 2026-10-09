#pragma once

// Private integration boundary for plugin translation units. The concrete
// RuntimeState layout and its sole owner remain in runtime.cpp.
namespace plugin::detail
{
    struct RuntimeState;

    RuntimeState& GetRuntimeState() noexcept;
}
