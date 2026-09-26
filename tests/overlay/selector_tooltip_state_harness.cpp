#include "../../src/overlay/selector_tooltip_state.hpp"

#include <array>
#include <iostream>

namespace
{
    bool Check(bool condition, const char* name)
    {
        if (!condition) std::cerr << "FAIL: " << name << "\n";
        return condition;
    }
}

int main()
{
    bool pass = true;
    constexpr std::array cases{
        overlay::SelectorInteractionState{false, false, false},
        overlay::SelectorInteractionState{false, false, true},
        overlay::SelectorInteractionState{false, true, false},
        overlay::SelectorInteractionState{false, true, true},
        overlay::SelectorInteractionState{true, false, false},
        overlay::SelectorInteractionState{true, false, true},
        overlay::SelectorInteractionState{true, true, false},
        overlay::SelectorInteractionState{true, true, true}
    };
    for (const auto state : cases) {
        const bool expected = state.hovered && !state.active && !state.popupOpen;
        pass &= Check(overlay::IsSelectorTooltipEligible(state) == expected,
            "selector_tooltip_truth_table");
    }

    std::cout << "Selector tooltip state harness: "
        << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
