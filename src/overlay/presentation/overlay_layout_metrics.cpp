#include "overlay_layout_metrics.hpp"

#include <algorithm>
#include <cmath>

namespace overlay::layout_metrics
{
    namespace
    {
        float NonnegativeFinite(float value) noexcept
        {
            return std::isfinite(value) && value > 0.0f ? value : 0.0f;
        }
    }

    float SafeMargin(float viewportExtent, float dpiScale) noexcept
    {
        const float scale = std::isfinite(dpiScale) && dpiScale > 0.0f
            ? dpiScale : 1.0f;
        return std::min(30.0f * scale,
            NonnegativeFinite(viewportExtent) * 0.05f);
    }

    float AvailableWindowWidth(float viewportWidth, float dpiScale) noexcept
    {
        const float viewport = NonnegativeFinite(viewportWidth);
        return std::max(0.0f, viewport - 2.0f * SafeMargin(viewport, dpiScale));
    }

    Position ClampPosition(Position position, Position viewportPosition,
        Position viewportSize, Position windowSize, float dpiScale) noexcept
    {
        const float width = NonnegativeFinite(viewportSize.x);
        const float height = NonnegativeFinite(viewportSize.y);
        const float marginX = SafeMargin(width, dpiScale);
        const float marginY = SafeMargin(height, dpiScale);
        const float left = viewportPosition.x + marginX;
        const float top = viewportPosition.y + marginY;
        const float right = std::max(left,
            viewportPosition.x + width - marginX - NonnegativeFinite(windowSize.x));
        const float bottom = std::max(top,
            viewportPosition.y + height - marginY - NonnegativeFinite(windowSize.y));
        const float x = std::isfinite(position.x) ? position.x : left;
        const float y = std::isfinite(position.y) ? position.y : top;
        return {std::clamp(x, left, right), std::clamp(y, top, bottom)};
    }
}
