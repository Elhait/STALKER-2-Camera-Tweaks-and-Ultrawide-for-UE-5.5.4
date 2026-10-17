#pragma once

namespace overlay::layout_metrics
{
    struct Position
    {
        float x{};
        float y{};
    };

    struct CameraStatePanelLayout
    {
        float windowWidth{};
        float windowHeight{};
        float existingColumnsWidth{};
        float cameraStateWidth{};
    };

    float AvailableWindowWidth(float viewportWidth, float dpiScale = 1.0f) noexcept;
    CameraStatePanelLayout ComputeCameraStatePanelLayout(
        float viewportWidth, float compactWindowWidth, float compactWindowHeight,
        float preferredPanelWidth,
        float windowPaddingWidth, bool expanded,
        float dpiScale = 1.0f) noexcept;
    float SafeMargin(float viewportExtent, float dpiScale = 1.0f) noexcept;
    Position ClampPosition(Position position, Position viewportPosition,
        Position viewportSize, Position windowSize,
        float dpiScale = 1.0f) noexcept;
}
