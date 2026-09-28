#pragma once

namespace overlay::layout_metrics
{
    struct Position
    {
        float x{};
        float y{};
    };

    float AvailableWindowWidth(float viewportWidth, float dpiScale = 1.0f) noexcept;
    float SafeMargin(float viewportExtent, float dpiScale = 1.0f) noexcept;
    Position ClampPosition(Position position, Position viewportPosition,
        Position viewportSize, Position windowSize,
        float dpiScale = 1.0f) noexcept;
}
