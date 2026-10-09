#pragma once

#include <cmath>
#include <cstdio>
#include <optional>
#include <string>

namespace overlay
{
    inline std::optional<std::string> FormatAspectRatioNine(float aspect)
    {
        if (!std::isfinite(aspect) || aspect <= 0.0f)
            return std::nullopt;

        char numerator[64]{};
        char rawAspect[64]{};
        std::snprintf(numerator, sizeof(numerator), "%.0f",
            std::round(static_cast<double>(aspect) * 9.0));
        std::snprintf(rawAspect, sizeof(rawAspect), "%.4f",
            static_cast<double>(aspect));
        return std::string(numerator) + ":9 (" + rawAspect + ")";
    }
}
