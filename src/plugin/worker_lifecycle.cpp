#include "worker_lifecycle.hpp"

#include <new>

namespace plugin
{
    DWORD WINAPI WorkerLifecycle::ThreadStartThunk(void* parameter) noexcept
    {
        const auto request = static_cast<StartRequest*>(parameter);
        const auto entry = request ? request->entry : nullptr;
        void* context = request ? request->context : nullptr;
        delete request;
        if (!entry) return 0;
        try {
            return entry(context);
        } catch (...) {
            return 0;
        }
    }

    bool WorkerLifecycle::Initialize()
    {
        if (stopEvent_) return true;
        stopEvent_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        return stopEvent_ != nullptr;
    }

    bool WorkerLifecycle::Start(LPTHREAD_START_ROUTINE entry, void* context)
    {
        if (!stopEvent_ || !entry || workerCount_ == sizeof(workers_) / sizeof(workers_[0]))
            return false;
        const auto request = new (std::nothrow) StartRequest{ entry, context };
        if (!request) return false;
        // Lifecycle control is externally serialized; workers consume only the
        // already-published stop event and their request, never this handle list.
        const auto worker = CreateThread(nullptr, 0, ThreadStartThunk, request,
            0, nullptr);
        if (!worker) {
            delete request;
            return false;
        }
        workers_[workerCount_++] = worker;
        return true;
    }

    bool WorkerLifecycle::WaitForStop(DWORD timeout) const
    {
        return stopEvent_ && WaitForSingleObject(stopEvent_, timeout) == WAIT_OBJECT_0;
    }

    void WorkerLifecycle::SignalStop() const
    {
        if (stopEvent_) SetEvent(stopEvent_);
    }

    bool WorkerLifecycle::StopAndJoin()
    {
        if (!stopEvent_) return true;
        SignalStop();

        const DWORD currentThreadId = GetCurrentThreadId();
        bool joined = true;
        std::size_t remaining = 0;
        for (std::size_t index = 0; index < workerCount_; ++index) {
            const HANDLE worker = workers_[index];
            if (!worker) continue;
            if (GetThreadId(worker) == currentThreadId) {
                joined = false;
                workers_[remaining++] = worker;
                continue;
            }
            if (WaitForSingleObject(worker, INFINITE) != WAIT_OBJECT_0) {
                joined = false;
                workers_[remaining++] = worker;
                continue;
            }
            CloseHandle(worker);
        }
        workerCount_ = remaining;

        if (joined) {
            CloseHandle(stopEvent_);
            stopEvent_ = nullptr;
        }
        return joined;
    }

    std::size_t WorkerLifecycle::WorkerCount() const
    {
        return workerCount_;
    }
}
