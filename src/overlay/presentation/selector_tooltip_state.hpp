#pragma once

namespace overlay
{
    struct SelectorInteractionState
    {
        bool hovered{};
        bool active{};
        bool popupOpen{};
    };

    [[nodiscard]] constexpr bool IsSelectorTooltipEligible(
        SelectorInteractionState state) noexcept
    {
        return state.hovered && !state.active && !state.popupOpen;
    }
}
