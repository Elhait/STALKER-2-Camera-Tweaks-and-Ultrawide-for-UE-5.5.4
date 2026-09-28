#include "../../src/plugin/optional_overlay_startup.hpp"

#include <iostream>
#include <stdexcept>

int main()
{
    int sequence = 0;
    bool coreReady = false;
    bool overlayHooksArmed = false;
    bool rendererActivationAllowed = false;
    bool overlayFailureReported = false;
    const int result = plugin::InitializeOptionalOverlayThenCore(
        [&]() {
            if (sequence != 0) throw std::logic_error("overlay arm order");
            sequence = 1;
            overlayHooksArmed = true;
            return true;
        },
        [&]() {
            if (sequence != 1 || !overlayHooksArmed || rendererActivationAllowed)
                return -1;
            sequence = 2;
            coreReady = true;
            return 42;
        },
        [&]() {
            if (!coreReady) return;
            rendererActivationAllowed = true;
        },
        [&]() {
            overlayFailureReported = coreReady;
        });
    const bool passed = result == 42 && coreReady && overlayHooksArmed &&
        rendererActivationAllowed && !overlayFailureReported && sequence == 2;
    bool thrownStartupReported = false;
    bool coreRanAfterOverlayFailure = false;
    bool failureReportedAfterCore = false;
    const int secondResult = plugin::InitializeOptionalOverlayThenCore(
        []() -> bool { throw std::runtime_error("overlay startup exception"); },
        [&]() { coreRanAfterOverlayFailure = true; return 7; },
        []() {},
        [&]() {
            thrownStartupReported = true;
            failureReportedAfterCore = coreRanAfterOverlayFailure;
        });
    const bool exceptionIsolated = secondResult == 7 && thrownStartupReported &&
        coreRanAfterOverlayFailure && failureReportedAfterCore;
    bool coreInitializationCompleted = false;
    bool readinessFailureReported = false;
    bool readinessExceptionEscaped = false;
    int readinessResult = -1;
    try {
        readinessResult = plugin::InitializeOptionalOverlayThenCore(
            [] { return true; },
            [&] { coreInitializationCompleted = true; return 19; },
            [] { throw std::runtime_error("optional readiness logging allocation"); },
            [&] { readinessFailureReported = coreInitializationCompleted; });
    } catch (...) {
        readinessExceptionEscaped = true;
    }
    const bool readinessIsolated = readinessResult == 19 &&
        coreInitializationCompleted && readinessFailureReported &&
        !readinessExceptionEscaped;
    std::cout << "overlay_hooks_armed_before_core=" << (overlayHooksArmed ? "PASS" : "FAIL")
        << " renderer_waits_for_core_ready=" << (rendererActivationAllowed && coreReady ? "PASS" : "FAIL")
        << " overlay_failure_isolated=" << (passed ? "PASS" : "FAIL")
        << " overlay_exception_isolated=" << (exceptionIsolated ? "PASS" : "FAIL")
        << " readiness_exception_isolated=" << (readinessIsolated ? "PASS" : "FAIL") << '\n';
    return passed && exceptionIsolated && readinessIsolated ? 0 : 1;
}
