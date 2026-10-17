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

    CameraStatePanelLayout ComputeCameraStatePanelLayout(float viewportWidth,
        float compactWindowWidth, float compactWindowHeight,
        float preferredPanelWidth,
        float windowPaddingWidth, bool expanded, float dpiScale) noexcept
    {
        const float available = AvailableWindowWidth(viewportWidth, dpiScale);
        if (available <= 0.0f) return {};
        const float height = NonnegativeFinite(compactWindowHeight);

        const float compact = std::clamp(NonnegativeFinite(compactWindowWidth),
            0.0f, available);
        const float preferred = NonnegativeFinite(preferredPanelWidth);
        const float padding = std::min(NonnegativeFinite(windowPaddingWidth),
            available * 0.5f);
        if (!expanded) {
            return {compact, height,
                std::max(0.0f, compact - 2.0f * padding), 0.0f};
        }
        const float remaining = available - compact;
        if (remaining >= preferred) {
            return {compact + preferred, height,
                std::max(0.0f, compact - 2.0f * padding), preferred};
        }

        // On a constrained viewport, preserve the three-column structure and
        // allocate a useful share to the diagnostics panel without exceeding
        // the viewport. At very small widths both regions scale down safely.
        const float panel = std::clamp(available * 0.36f, 0.0f, available);
        const float baseWindow = available - panel;
        return {available, height,
            std::max(0.0f, baseWindow - 2.0f * padding), panel};
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
