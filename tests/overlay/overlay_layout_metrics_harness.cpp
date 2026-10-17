#include "../../src/overlay/presentation/overlay_layout_metrics.hpp"

#include <algorithm>
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

    for (const auto viewportWidth : {1920.0f, 2560.0f, 5120.0f}) {
        for (const auto dpi : {1.0f, 1.25f, 1.5f}) {
            const auto available = AvailableWindowWidth(viewportWidth, dpi);
            const auto compact = std::min(1024.0f * dpi, available);
            const auto layout = ComputeCameraStatePanelLayout(viewportWidth,
                compact, 720.0f * dpi, 640.0f * dpi, 16.0f * dpi, true, dpi);
            const auto closedLayout = ComputeCameraStatePanelLayout(viewportWidth,
                compact, 720.0f * dpi, 640.0f * dpi, 16.0f * dpi, false, dpi);
            pass &= Check(layout.windowWidth <= available + 0.02f &&
                layout.windowWidth >= 0.0f && layout.existingColumnsWidth >= 0.0f &&
                layout.cameraStateWidth >= 0.0f &&
                Near(layout.windowHeight, 720.0f * dpi),
                "expanded_panel_stays_inside_each_aspect_and_dpi_viewport");
            pass &= Check(Near(closedLayout.windowWidth, compact) &&
                Near(closedLayout.windowHeight, 720.0f * dpi) &&
                Near(closedLayout.cameraStateWidth, 0.0f) &&
                Near(closedLayout.existingColumnsWidth,
                    std::max(0.0f, compact - 32.0f * dpi)),
                "collapsed_panel_selects_compact_size_and_hides_third_column");
            if (available >= compact + 640.0f * dpi) {
                pass &= Check(Near(layout.existingColumnsWidth,
                    compact - 32.0f * dpi) &&
                    Near(layout.cameraStateWidth, 640.0f * dpi),
                    "roomy_viewport_preserves_compact_columns_and_adds_panel");
            } else {
                pass &= Check(Near(layout.windowWidth, available) &&
                    layout.cameraStateWidth > 0.0f &&
                    layout.existingColumnsWidth > 0.0f,
                    "constrained_viewport_adapts_without_dropping_panel_columns");
            }

            bool expanded = false;
            float currentWidth = closedLayout.windowWidth;
            for (int toggle = 0; toggle < 20; ++toggle) {
                expanded = !expanded;
                const auto nextLayout = ComputeCameraStatePanelLayout(viewportWidth,
                    compact, 720.0f * dpi, 640.0f * dpi, 16.0f * dpi,
                    expanded, dpi);
                currentWidth = nextLayout.windowWidth;
                pass &= Check(Near(currentWidth, layout.windowWidth),
                    "rapid_toggle_expands_to_deterministic_width");
                expanded = !expanded;
                const auto closedAgain = ComputeCameraStatePanelLayout(viewportWidth,
                    compact, 720.0f * dpi, 640.0f * dpi, 16.0f * dpi,
                    expanded, dpi);
                currentWidth = closedAgain.windowWidth;
                pass &= Check(Near(currentWidth, compact) &&
                    Near(currentWidth, closedLayout.windowWidth) &&
                    Near(closedAgain.cameraStateWidth, 0.0f),
                    "rapid_toggle_restores_compact_width_after_panel_hides");
            }
            const Position topLeft{SafeMargin(viewportWidth, dpi),
                SafeMargin(viewportWidth, dpi)};
            const auto expandedPosition = ClampPosition(topLeft, {0.0f, 0.0f},
                {viewportWidth, 1440.0f * dpi},
                {layout.windowWidth, layout.windowHeight}, dpi);
            const auto compactPosition = ClampPosition(topLeft, {0.0f, 0.0f},
                {viewportWidth, 1440.0f * dpi},
                {closedLayout.windowWidth, closedLayout.windowHeight}, dpi);
            pass &= Check(Near(expandedPosition.x, compactPosition.x),
                "left_edge_remains_stable_when_viewport_can_contain_both_sizes");
        }
    }
    const auto tinyLayout = ComputeCameraStatePanelLayout(320.0f, 280.0f,
        300.0f, 640.0f, 16.0f, true);
    pass &= Check(tinyLayout.windowWidth <= AvailableWindowWidth(320.0f) + 0.02f &&
        tinyLayout.existingColumnsWidth >= 0.0f && tinyLayout.cameraStateWidth >= 0.0f,
        "very_narrow_viewport_remains_finite_and_nonnegative");

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
