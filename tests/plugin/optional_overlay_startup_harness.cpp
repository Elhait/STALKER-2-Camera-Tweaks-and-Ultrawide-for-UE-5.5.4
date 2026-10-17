#include "../../src/plugin/optional_overlay_startup.hpp"
#include "../../src/overlay/optional_overlay_boundary.hpp"

#include <iostream>
#include <stdexcept>

int main()
{
    const bool readinessPolicyIsFailClosed =
        plugin::ShouldPublishCameraCoreReady(true, true, false) &&
        !plugin::ShouldPublishCameraCoreReady(false, true, false) &&
        !plugin::ShouldPublishCameraCoreReady(true, false, false) &&
        !plugin::ShouldPublishCameraCoreReady(true, true, true);
    bool loggerThrowContained = false;
    bool loggerMessagePreserved = false;
    const bool throwingLoggerWasContained = !overlay::TryOptionalOverlayLog(
        [](const std::string&) { throw std::runtime_error("logger"); },
        "overlay failure report") &&
        overlay::TryOptionalOverlayLog([&](const std::string& message) {
            loggerMessagePreserved = message == "overlay failure report";
        }, "overlay failure report");
    loggerThrowContained = throwingLoggerWasContained && loggerMessagePreserved;

    bool workerFailureTransitionRequested = false;
    bool workerBoundaryEscaped = false;
    auto workerBoundaryResult = overlay::OptionalOverlayWorkResult::Completed;
    try {
        workerBoundaryResult = overlay::RunOptionalOverlayWork(
            [] { throw std::runtime_error("owner worker"); },
            [&]() noexcept { workerFailureTransitionRequested = true; });
    } catch (...) {
        workerBoundaryEscaped = true;
    }
    const bool workerBoundaryContained = !workerBoundaryEscaped &&
        workerBoundaryResult == overlay::OptionalOverlayWorkResult::Failed &&
        workerFailureTransitionRequested;

    bool disabledStartCallbackInvoked = false;
    const bool disabledSkipsInitialization = !plugin::TryStartConfiguredOverlay(false,
        [&]() { disabledStartCallbackInvoked = true; return true; }) &&
        !disabledStartCallbackInvoked;
    const bool enabledStarts = plugin::TryStartConfiguredOverlay(true,
        []() { return true; });
    const bool startupExceptionIsolated = !plugin::TryStartConfiguredOverlay(true,
        []() -> bool { throw std::runtime_error("startup"); });
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
    std::cout << "disabled_skips_overlay_initialization="
        << (disabledSkipsInitialization ? "PASS" : "FAIL")
        << " enabled_starts_overlay=" << (enabledStarts ? "PASS" : "FAIL")
        << " configured_start_exception_isolated="
        << (startupExceptionIsolated ? "PASS" : "FAIL") << '\n';
    std::cout << "throwing_overlay_logger_contained="
        << (loggerThrowContained ? "PASS" : "FAIL")
        << " worker_exception_requests_terminal_transition="
        << (workerBoundaryContained ? "PASS" : "FAIL") << '\n';
    std::cout << "camera_core_readiness_requires_success_complete_and_not_stopping="
        << (readinessPolicyIsFailClosed ? "PASS" : "FAIL") << '\n';
    return passed && exceptionIsolated && readinessIsolated &&
        disabledSkipsInitialization && enabledStarts && startupExceptionIsolated &&
        loggerThrowContained && workerBoundaryContained &&
        readinessPolicyIsFailClosed ? 0 : 1;
}
