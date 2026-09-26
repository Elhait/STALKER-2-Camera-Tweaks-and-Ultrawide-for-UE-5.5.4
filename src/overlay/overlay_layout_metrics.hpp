#pragma once

namespace overlay::layout_metrics
{
    struct Position
    {
        float x{};
        float y{};
    };

    float AvailableWindowWidth(float viewportWidth) noexcept;
    float SafeMargin(float viewportExtent) noexcept;
    Position ClampPosition(Position position, Position viewportPosition,
        Position viewportSize, Position windowSize) noexcept;
}
