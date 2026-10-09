#pragma once

#include <cmath>

namespace camera
{
    // Runtime FOV calculations can vary slightly without representing a new
    // camera intent. This is semantic equivalence, not a math/interpolation epsilon.
    inline constexpr float kSemanticFovToleranceDegrees = 1.0f;

    inline bool AreFovValuesSemanticallyEquivalent(float left, float right) noexcept
    {
        return std::isfinite(left) && std::isfinite(right) &&
            std::fabs(left - right) <= kSemanticFovToleranceDegrees;
    }
}
