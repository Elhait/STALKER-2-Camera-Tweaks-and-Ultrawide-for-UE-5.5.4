#include "../../src/diagnostics/diagnostic_runtime.hpp"
#include "../../src/diagnostics/performance_telemetry.hpp"

#include <iostream>
#include <mutex>
#include <string>

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
    std::mutex mutex;
    std::string summary;
    diagnostics::SetEnabled(false);
    {
        diagnostics::ScopedRuntimeMutex lock(mutex,
            diagnostics::RuntimeLockPath::Dialogue);
        diagnostics::ScopedRuntimeLog log;
        diagnostics::RecordRuntimeEvent(
            diagnostics::RuntimeEvent::DialogueCameraSample);
    }
    diagnostics::RequestRuntimePerformanceSummary();
    pass &= Check(!diagnostics::TakeRuntimePerformanceSummary(summary),
        "disabled_telemetry_emits_no_summary");

    diagnostics::SetEnabled(true);
    {
        diagnostics::ScopedRuntimeMutex lock(mutex,
            diagnostics::RuntimeLockPath::Dialogue);
        {
            diagnostics::ScopedRuntimeLog log;
        }
        diagnostics::RecordRuntimeEvent(
            diagnostics::RuntimeEvent::DialogueCameraSample);
    }
    diagnostics::RequestRuntimePerformanceSummary();
    pass &= Check(diagnostics::TakeRuntimePerformanceSummary(summary),
        "enabled_telemetry_emits_requested_summary");
    pass &= Check(summary.find("PERF_RUNTIME_SUMMARY diagnostics=on") !=
            std::string::npos &&
        summary.find("elapsed_ms=") != std::string::npos &&
        summary.find("dialogue-camera-samples=1") != std::string::npos &&
        summary.find("lock=dialogue acquisitions=1") != std::string::npos &&
        summary.find("log_calls=1") != std::string::npos,
        "summary_contains_bounded_event_lock_and_log_metrics");
    pass &= Check(!diagnostics::TakeRuntimePerformanceSummary(summary),
        "summary_is_emitted_only_once");
    std::cout << "Performance telemetry harness: " << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
