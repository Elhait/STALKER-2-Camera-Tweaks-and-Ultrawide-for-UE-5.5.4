#pragma once

#include <Windows.h>

#include <cstddef>

namespace plugin
{
    // Externally serialized lifecycle primitive. Initialize, Start and
    // StopAndJoin belong to one logically serialized Runtime control path;
    // they need not run on one physical thread and must not overlap. Workers
    // only consume the stop event through WaitForStop. SignalStop is const and
    // may be called by the minimal DLL detach notification path. This is not a
    // general concurrent thread-management abstraction.
    class WorkerLifecycle
    {
    public:
        WorkerLifecycle() = default;
        WorkerLifecycle(const WorkerLifecycle&) = delete;
        WorkerLifecycle& operator=(const WorkerLifecycle&) = delete;

        ~WorkerLifecycle() = default;

        bool Initialize();
        bool Start(LPTHREAD_START_ROUTINE entry, void* context = nullptr);
        bool WaitForStop(DWORD timeout) const;
        void SignalStop() const;
        bool StopAndJoin();
        std::size_t WorkerCount() const;

    private:
        struct StartRequest
        {
            LPTHREAD_START_ROUTINE entry{};
            void* context{};
        };

        static DWORD WINAPI ThreadStartThunk(void* parameter) noexcept;

        HANDLE stopEvent_{};
        HANDLE workers_[4]{};
        std::size_t workerCount_{};
    };
}
