#include "../../src/plugin/worker_lifecycle.hpp"

#include <atomic>
#include <cstdio>

namespace
{
    struct WorkerContext
    {
        plugin::WorkerLifecycle* lifecycle{};
        std::atomic<bool>* started{};
    };

    DWORD WINAPI WaitingWorker(void* parameter)
    {
        const auto context = static_cast<WorkerContext*>(parameter);
        context->started->store(true, std::memory_order_release);
        while (!context->lifecycle->WaitForStop(25)) {}
        return 0;
    }

    bool WaitUntilStarted(const std::atomic<bool>& started)
    {
        for (int attempt = 0; attempt != 100 && !started.load(std::memory_order_acquire); ++attempt)
            Sleep(1);
        return started.load(std::memory_order_acquire);
    }

    bool TestNormalStartupAndControlledStop()
    {
        plugin::WorkerLifecycle lifecycle;
        std::atomic<bool> started{};
        WorkerContext context{ &lifecycle, &started };
        if (!lifecycle.Initialize() || !lifecycle.Start(WaitingWorker, &context)) return false;
        if (!WaitUntilStarted(started) || lifecycle.WorkerCount() != 1) return false;
        if (!lifecycle.StopAndJoin() || lifecycle.WorkerCount() != 0) return false;
        return lifecycle.StopAndJoin();
    }

    bool TestPartialStartupFailure()
    {
        plugin::WorkerLifecycle lifecycle;
        std::atomic<bool> started{};
        WorkerContext context{ &lifecycle, &started };
        if (!lifecycle.Initialize() || !lifecycle.Start(WaitingWorker, &context)) return false;
        if (lifecycle.Start(nullptr, nullptr)) return false;
        return lifecycle.StopAndJoin() && lifecycle.WorkerCount() == 0;
    }

    bool TestStopBeforeStartup()
    {
        plugin::WorkerLifecycle lifecycle;
        if (!lifecycle.StopAndJoin()) return false;
        return lifecycle.Initialize() && lifecycle.StopAndJoin();
    }

    bool TestHotkeyPlusDiagnosticWorkerCapacity()
    {
        plugin::WorkerLifecycle lifecycle;
        std::atomic<bool> started[4]{};
        WorkerContext contexts[4]{};
        if (!lifecycle.Initialize()) return false;
        for (int index = 0; index < 4; ++index) {
            contexts[index] = { &lifecycle, &started[index] };
            if (!lifecycle.Start(WaitingWorker, &contexts[index])) return false;
        }
        for (const auto& workerStarted : started)
            if (!WaitUntilStarted(workerStarted)) return false;
        return lifecycle.WorkerCount() == 4 && lifecycle.StopAndJoin() &&
            lifecycle.WorkerCount() == 0;
    }
}

int main()
{
    const bool normal = TestNormalStartupAndControlledStop();
    const bool partial = TestPartialStartupFailure();
    const bool beforeStartup = TestStopBeforeStartup();
    const bool workerCapacity = TestHotkeyPlusDiagnosticWorkerCapacity();
    std::printf("normal=%s partial=%s startup_stop_observation=%s stop_before_start=%s worker_capacity=%s\n",
        normal ? "PASS" : "FAIL", partial ? "PASS" : "FAIL",
        (normal && partial) ? "PASS" : "FAIL", beforeStartup ? "PASS" : "FAIL",
        workerCapacity ? "PASS" : "FAIL");
    return normal && partial && beforeStartup && workerCapacity ? 0 : 1;
}
