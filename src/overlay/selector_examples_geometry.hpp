#pragma once

#include <imgui.h>

namespace overlay::selector_examples_geometry
{
    inline constexpr float BaseFontSizePixels = 14.0f;
    inline constexpr float HorizontalInnerInsetAtBaseScale = 10.0f;
    inline constexpr float VerticalInnerInsetAtBaseScale = 8.0f;

    constexpr float UiScale(float fontSize) noexcept
    {
        return fontSize / BaseFontSizePixels;
    }

    struct FrameGeometry
    {
        ImVec2 frameMin;
        ImVec2 frameMax;
        ImVec2 contentMin;
        ImVec2 contentMax;
    };

    inline FrameGeometry CalculateFrameGeometry(ImVec2 frameMin,
        float contentWidth, float contentHeight, float fontSize) noexcept
    {
        const float uiScale = UiScale(fontSize);
        const float horizontalInset = HorizontalInnerInsetAtBaseScale * uiScale;
        const float verticalInset = VerticalInnerInsetAtBaseScale * uiScale;
        return {
            frameMin,
            ImVec2(frameMin.x + contentWidth + horizontalInset * 2.0f,
                frameMin.y + contentHeight + verticalInset * 2.0f),
            ImVec2(frameMin.x + horizontalInset, frameMin.y + verticalInset),
            ImVec2(frameMin.x + horizontalInset + contentWidth,
                frameMin.y + verticalInset + contentHeight)
        };
    }
}
