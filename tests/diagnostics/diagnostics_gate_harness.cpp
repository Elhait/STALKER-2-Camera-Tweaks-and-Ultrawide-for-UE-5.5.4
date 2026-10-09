#include "../../src/diagnostics/diagnostic_runtime.hpp"
#include "../../src/diagnostics/camera_log_probe.hpp"

#include <atomic>
#include <iostream>

int main()
{
    diagnostics::SetEnabled(false);
    const bool disabled = !diagnostics::Enabled();
    int emittedMarkers = 0;
    const bool disabledEmission = !diagnostics::EmitIfEnabled([&] {
        ++emittedMarkers;
    });
    const bool noDisabledMarker = emittedMarkers == 0;
    diagnostics::SetEnabled(true);
    const bool enabled = diagnostics::Enabled();
    const bool enabledEmission = diagnostics::EmitIfEnabled([&] {
        ++emittedMarkers;
    });
    const bool oneEnabledMarker = emittedMarkers == 1;
    diagnostics::SetEnabled(false);
    const bool reset = !diagnostics::Enabled();
    std::atomic<std::uint64_t> pendingGeneration{0};
    const auto offArm = diagnostics::ArmCinematicExitWriterProbe(
        pendingGeneration, false, 17);
    const bool offIsolation = offArm == 0 &&
        pendingGeneration.load(std::memory_order_acquire) == 0 &&
        diagnostics::TakeCinematicExitWriterProbe(pendingGeneration, false) == 0;
    const auto onArm = diagnostics::ArmCinematicExitWriterProbe(
        pendingGeneration, true, 23);
    const auto firstTake = diagnostics::TakeCinematicExitWriterProbe(
        pendingGeneration, true);
    const auto secondTake = diagnostics::TakeCinematicExitWriterProbe(
        pendingGeneration, true);
    const bool onOneShot = onArm == 0 && firstTake == 23 && secondTake == 0 &&
        pendingGeneration.load(std::memory_order_acquire) == 0;
    const bool pass = disabled && disabledEmission && noDisabledMarker && enabled &&
        enabledEmission && oneEnabledMarker && reset && offIsolation && onOneShot;
    std::cout << "diagnostics_gate=" << (pass ? "PASS" : "FAIL")
        << " off_suppresses_emission=" << (disabledEmission && noDisabledMarker ? "PASS" : "FAIL")
        << " on_emits=" << (enabledEmission && oneEnabledMarker ? "PASS" : "FAIL")
        << " off_probe_isolation=" << (offIsolation ? "PASS" : "FAIL")
        << " on_probe_one_shot=" << (onOneShot ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
