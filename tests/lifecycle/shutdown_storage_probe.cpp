#include "../../src/platform/process_lifetime.hpp"
#include "../../src/platform/win32/native_file.hpp"
#include "../../src/platform/win32/module_residency.hpp"

#include <mutex>
#include <vector>
#include <fstream>

namespace
{
    struct Probe
    {
        std::mutex mutex;
        platform::win32::NativeFile file;
        std::vector<unsigned> values;
        HANDLE ready{};
        HANDLE hold{};
        ~Probe()
        {
            file.Write("DESTRUCTOR_BEGIN\r\n");
            // Negative control: CRT detach must block if this destructor is
            // registered after ExitProcess kills the worker holding the lock.
            std::scoped_lock lock{mutex};
            file.Write("DESTRUCTOR_RAN\r\n");
        }
    };
    Probe& Resident()
    {
        static platform::ProcessLifetime<Probe> owner;
        return owner.get();
    }
    Probe& Automatic()
    {
        static Probe owner;
        return owner;
    }
    struct LegacyFileProbe
    {
        std::ofstream file;
        platform::win32::NativeFile witness;
        HANDLE ready{}, hold{};
        ~LegacyFileProbe()
        {
            witness.Write("CRT_DESTRUCTOR_BEGIN\r\n");
            file.close();
            witness.Write("CRT_DESTRUCTOR_END\r\n");
        }
    };
    DWORD WINAPI HoldLegacyFile(void* parameter)
    {
        auto& probe = *static_cast<LegacyFileProbe*>(parameter);
        probe.file.rdbuf()->_Lock();
        SetEvent(probe.ready);
        WaitForSingleObject(probe.hold, INFINITE);
        probe.file.rdbuf()->_Unlock();
        return 0;
    }
    DWORD WINAPI Hold(void* parameter)
    {
        auto& probe = *static_cast<Probe*>(parameter);
        std::scoped_lock lock{probe.mutex};
        probe.values.push_back(42);
        SetEvent(probe.ready);
        WaitForSingleObject(probe.hold, INFINITE);
        return 0;
    }
}

extern "C" __declspec(dllexport) HANDLE WINAPI PrepareShutdownProbe(
    const wchar_t* path, HANDLE ready, HANDLE hold, BOOL automatic)
{
    try {
        if (automatic == 2) {
            // Exact old ofstream/FILE-lock path, used only as an externally
            // timed negative control. No graphics or game process is involved.
            static LegacyFileProbe legacy;
            legacy.ready = ready;
            legacy.hold = hold;
            legacy.file.open(path, std::ios::out | std::ios::trunc);
            if (!legacy.file) return nullptr;
            legacy.file << "before_exit\n";
            legacy.file.flush();
            const auto witness = std::wstring(path) + L".witness";
            if (!legacy.witness.Open(witness.c_str(), true)) return nullptr;
            return CreateThread(nullptr, 0, HoldLegacyFile, &legacy, 0, nullptr);
        }
        auto& probe = automatic ? Automatic() : Resident();
        probe.ready = ready;
        probe.hold = hold;
        if (!probe.file.Open(path, true) || !probe.file.Write("before_exit\r\n"))
            return nullptr;
        return CreateThread(nullptr, 0, Hold, &probe, 0, nullptr);
    } catch (...) { return nullptr; }
}

extern "C" __declspec(dllexport) BOOL WINAPI PinShutdownProbe(HMODULE module)
{
    return platform::win32::PinModuleForProcessLifetime(module);
}

BOOL WINAPI DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }
