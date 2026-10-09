#pragma once

#include <utility>

namespace diagnostics
{
    void SetEnabled(bool enabled) noexcept;
    bool Enabled() noexcept;

    template <typename Emit>
    bool EmitIfEnabled(Emit&& emit)
    {
        if (!Enabled()) return false;
        std::forward<Emit>(emit)();
        return true;
    }
}
