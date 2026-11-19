#include "../../src/dialogue/dialogue_fov.hpp"
#include "../../src/config/feature_config.hpp"

#include <cmath>
#include <cstdio>
#include <limits>

namespace
{
    bool Near(float actual, float expected, float tolerance = 0.02f)
    {
        return std::isfinite(actual) && std::fabs(actual - expected) <= tolerance;
    }

    bool Check(bool condition, const char* name)
    {
        if (!condition) std::fprintf(stderr, "%s: FAIL\n", name);
        return condition;
    }
}

int main()
{
    bool pass = true;
    constexpr double pi = 3.14159265358979323846;
    const auto independentReduced = [=](double baseline) {
        const double nativeZoom = std::tan(90.0 * pi / 360.0) /
            std::tan(70.0 * pi / 360.0);
        return 2.0 * std::atan(std::tan(baseline * pi / 360.0) /
            std::sqrt(nativeZoom)) * 180.0 / pi;
    };
    const auto independentAdaptive = [=](double baseline) {
        const double nativeZoom = std::tan(90.0 * pi / 360.0) /
            std::tan(70.0 * pi / 360.0);
        return 2.0 * std::atan(std::tan(baseline * pi / 360.0) /
            nativeZoom) * 180.0 / pi;
    };
    // Independently recorded/validated dialogue endpoints for the native 90°
    // gameplay reference and the 110° gameplay reference.
    pass &= Check(Near(dialogue::AdaptiveTarget(90.0f), 70.0f), "adaptive_90_endpoint");
    pass &= Check(Near(dialogue::ReducedTarget(90.0f), 79.84411f), "reduced_90_endpoint");
    pass &= Check(Near(dialogue::AdaptiveTarget(110.0f), 90.0f), "adaptive_110_endpoint");
    pass &= Check(Near(dialogue::ReducedTarget(110.0f), 100.15589f), "reduced_110_endpoint");
    constexpr float baselineSweep[] = { 1.01f, 70.0f, 90.0f, 100.0f,
        120.0f, 130.0f, 178.99f };
    for (const float baselineValue : baselineSweep) {
        pass &= Check(Near(dialogue::ReducedTarget(baselineValue),
                static_cast<float>(independentReduced(baselineValue)), 0.025f) &&
            Near(dialogue::AdaptiveTarget(baselineValue),
                static_cast<float>(independentAdaptive(baselineValue)), 0.025f),
            "dialogue_targets_match_independent_boundary_oracle");
    }
    const float policyBaseline = 90.3115f;
    pass &= Check(Near(dialogue::ResolveDialogueDestinationTarget(
            config::DialogueZoomPolicy::Native, policyBaseline, 70.0f), 70.0f) &&
        Near(dialogue::ResolveDialogueDestinationTarget(
            config::DialogueZoomPolicy::Adaptive, policyBaseline, 70.0f), 70.292987f) &&
        Near(dialogue::ResolveDialogueDestinationTarget(
            config::DialogueZoomPolicy::Reduced, policyBaseline, 70.0f), 80.150877f) &&
        Near(dialogue::ResolveDialogueDestinationTarget(
            config::DialogueZoomPolicy::Disabled, policyBaseline, 70.0f), policyBaseline),
        "destination_policy_endpoints_remain_distinct");
    pass &= Check(std::isnan(dialogue::ResolveDialogueDestinationTarget(
            config::DialogueZoomPolicy::Reduced,
            std::numeric_limits<float>::quiet_NaN(), 70.0f)) &&
        std::isnan(dialogue::ResolveDialogueDestinationTarget(
            config::DialogueZoomPolicy::Native, policyBaseline,
            std::numeric_limits<float>::infinity())),
        "destination_policy_rejects_invalid_endpoints");

    const float baseline = 110.0f;
    const float target = dialogue::ReducedTarget(baseline);
    const float enter = dialogue::TransformProjectionSample(baseline, baseline, target);
    const float exit = dialogue::TransformExitSample(target, baseline, target, target);
    pass &= Check(Near(enter, baseline) && Near(
        dialogue::TransformProjectionSample(70.0f, baseline, target), target),
        "projection_endpoints");
    if (!Near(exit, target)) std::fprintf(stderr, "exit=%f target=%f\n", exit, target);
    pass &= Check(Near(exit, target), "exit_start_endpoint");
    pass &= Check(Near(dialogue::TransformExitSample(baseline, baseline, baseline, target), baseline),
        "exit_baseline_endpoint");

    const float p1 = dialogue::TransformProjectionSample(100.0f, baseline, target);
    const float p2 = dialogue::TransformProjectionSample(90.0f, baseline, target);
    pass &= Check(p1 > p2 && p2 > target, "projection_monotonicity");
    pass &= Check(dialogue::TransformProjectionSample(40.0f, baseline, target) == 40.0f,
        "out_of_range_passthrough");
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    pass &= Check(std::isnan(dialogue::TransformProjectionSample(nan, baseline, target)),
        "nan_passthrough");
    const float infinityExit = dialogue::TransformExitSample(inf, baseline, target, target);
    if (!std::isinf(infinityExit)) std::fprintf(stderr,
        "infinity_exit=%f\n", infinityExit);
    pass &= Check(std::isinf(infinityExit), "infinity_passthrough");

    std::printf("dialogue_fov=%s\n", pass ? "PASS" : "FAIL");
    return pass ? 0 : 1;
}
