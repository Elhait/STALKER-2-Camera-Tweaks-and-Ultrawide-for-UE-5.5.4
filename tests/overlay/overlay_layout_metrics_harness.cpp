#include "../../src/overlay/overlay_layout_metrics.hpp"

#include <cmath>
#include <iostream>

namespace
{
    bool Check(bool condition, const char* name)
    {
        if (!condition) std::cerr << "FAIL: " << name << "\n";
        return condition;
    }

    bool Near(float a, float b, float epsilon = 0.02f)
    {
        return std::fabs(a - b) <= epsilon;
    }
}

int main()
{
    using namespace overlay::layout_metrics;
    bool pass = true;

    pass &= Check(Near(AvailableWindowWidth(1920.0f), 1860.0f) &&
        Near(AvailableWindowWidth(640.0f), 580.0f),
        "window_width_is_limited_only_by_viewport_safe_margins");
    pass &= Check(AvailableWindowWidth(20.0f) == 18.0f &&
        AvailableWindowWidth(-1.0f) == 0.0f,
        "degenerate_viewport_never_produces_negative_width");

    const Position viewport{100.0f, 50.0f};
    const Position viewportSize{640.0f, 480.0f};
    const Position windowSize{580.0f, 300.0f};
    const Position clamped = ClampPosition({700.0f, -100.0f}, viewport,
        viewportSize, windowSize);
    pass &= Check(Near(clamped.x, 130.0f) && Near(clamped.y, 74.0f),
        "position_reclamped_inside_viewport_safe_margins");

    std::cout << "overlay_layout_metrics=" << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
