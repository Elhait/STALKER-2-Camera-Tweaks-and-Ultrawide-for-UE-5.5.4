#include "../../src/plugin/worker_lifecycle.hpp"
#include "../../src/platform/process_lifetime.hpp"
BOOL WINAPI TestNativeFileWrite(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
#define NATIVE_FILE_TEST_WRITE TestNativeFileWrite
#include "../../src/platform/win32/native_log_sink.hpp"
#undef NATIVE_FILE_TEST_WRITE
#include <spdlog/logger.h>

#include <atomic>
#include <cstdio>
#include <filesystem>
#include <sstream>
#include <thread>
#include <type_traits>
#include <vector>

namespace {
    std::atomic<unsigned> writeFaultPhase{};
    std::atomic<unsigned> nativeWriteCalls{};
}

BOOL WINAPI TestNativeFileWrite(HANDLE file, LPCVOID data, DWORD size,
    LPDWORD written, LPOVERLAPPED overlapped)
{
    nativeWriteCalls.fetch_add(1);
    if (writeFaultPhase.load() == 1) {
        writeFaultPhase.store(2);
        return WriteFile(file, data, (std::min)(size, DWORD{8}), written, overlapped);
    }
    if (writeFaultPhase.load() == 2) {
        writeFaultPhase.store(0);
        *written = 0;
        SetLastError(ERROR_WRITE_FAULT);
        return FALSE;
    }
    return WriteFile(file, data, size, written, overlapped);
}

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

    bool TestImmediatelyCompletingWorkers()
    {
        std::atomic<unsigned> completions{};
        const auto entry = +[](void* context) -> DWORD {
            static_cast<std::atomic<unsigned>*>(context)->fetch_add(1,
                std::memory_order_release);
            return 0;
        };
        plugin::WorkerLifecycle lifecycle;
        for (unsigned round = 0; round != 64; ++round)
            if (!lifecycle.Initialize() || !lifecycle.Start(entry, &completions) ||
                !lifecycle.StopAndJoin() || lifecycle.WorkerCount()) return false;
        return completions.load(std::memory_order_acquire) == 64;
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

    struct LifetimeProbe
    {
        unsigned* destructions;
        ~LifetimeProbe() { ++*destructions; }
    };

    bool TestNativeLogsAndStorage(const std::filesystem::path& directory)
    {
        static_assert(std::is_trivially_destructible_v<
            platform::ProcessLifetime<LifetimeProbe>>);
        unsigned destructions{};
        {
            platform::ProcessLifetime<LifetimeProbe> resident(&destructions);
            if (resident.get().destructions != &destructions) return false;
        }
        if (destructions != 0) return false;

        const auto cappedPath = directory / L"bounded-log.txt";
        {
            platform::win32::NativeLogFile capped(256);
            if (!capped.Open(cappedPath.c_str())) return false;
            for (unsigned record = 0; record != 100; ++record)
                if (!capped.Write("one complete bounded log record\r\n")) return false;
            if (!capped.Close()) return false;
        }
        std::string bounded;
        if (!platform::win32::ReadNativeFile(cappedPath.c_str(), bounded) ||
            bounded.size() > 256 || bounded.find("LOG_LIMIT_REACHED") == std::string::npos ||
            bounded.find("LOG_LIMIT_REACHED") != bounded.rfind("LOG_LIMIT_REACHED") ||
            !DeleteFileW(cappedPath.c_str())) return false;

        for (const bool failMarker : {false, true}) {
            platform::win32::NativeLogFile capped(256);
            if (!capped.Open(cappedPath.c_str())) return false;
            if (failMarker && !capped.Write(std::string(160, 'x'))) return false;
            const auto beforeFailure = nativeWriteCalls.load();
            writeFaultPhase.store(1);
            if (capped.Write(std::string(100, 'y'))) return false;
            const auto afterFailure = nativeWriteCalls.load();
            if (afterFailure != beforeFailure + 2) return false;
            for (unsigned retry = 0; retry != 100; ++retry)
                if (!capped.Write("must not issue another OS write")) return false;
            if (nativeWriteCalls.load() != afterFailure || !capped.Close()) return false;
            if (!platform::win32::ReadNativeFile(cappedPath.c_str(), bounded) ||
                bounded.size() != (failMarker ? 168u : 8u) ||
                !DeleteFileW(cappedPath.c_str())) return false;
        }
        std::printf("partial_log_write_budget=PASS record_and_limit_marker=PASS\n");

        const auto path = directory / L"shutdown-log-\u0436\u0443\u0440\u043d\u0430\u043b.txt";
        for (unsigned round = 0; round != 2; ++round) {
            auto sink = std::make_shared<platform::win32::NativeLogSink>(path.c_str());
            spdlog::logger logger("same-unregistered-name", sink);
            logger.set_pattern("%v");
            std::vector<std::thread> writers;
            for (unsigned worker = 0; worker != 4; ++worker)
                writers.emplace_back([&, worker] {
                    for (unsigned item = 0; item != 128; ++item)
                        logger.info("worker={} item={} UTF8=\xD0\xA2\xD0\xB5\xD1\x81\xD1\x82", worker, item);
                });
            for (auto& writer : writers) writer.join();
            logger.flush();
            std::string text;
            if (!platform::win32::ReadNativeFile(path.c_str(), text)) return false;
            std::vector<bool> seen(512);
            std::istringstream lines(text);
            std::string line;
            unsigned count{};
            while (std::getline(lines, line)) {
                unsigned worker{}, item{};
                if (sscanf_s(line.c_str(), "worker=%u item=%u", &worker, &item) != 2 ||
                    worker >= 4 || item >= 128 || seen[worker * 128 + item] ||
                    line.find("UTF8=\xD0\xA2\xD0\xB5\xD1\x81\xD1\x82\r") == std::string::npos)
                    return false;
                seen[worker * 128 + item] = true;
                ++count;
            }
            if (count != 512) return false;
        }
        platform::win32::NativeFile file;
        if (file.Write("closed") || !file.Close() || !file.Open(path.c_str(), true) ||
            !file.Write("a\r\nb\n") || !file.FlushToDisk() ||
            !file.Close() || !file.Close()) return false;
        std::string text = "preserved";
        if (platform::win32::ReadNativeFile((directory / L"missing").c_str(), text) ||
            text != "preserved" || !platform::win32::ReadNativeFile(path.c_str(), text) ||
            text != "a\r\nb\n") return false;
        return DeleteFileW(path.c_str()) != FALSE;
    }

    int ExitChild(const wchar_t* path, unsigned automatic)
    {
        wchar_t executable[MAX_PATH]{};
        if (!GetModuleFileNameW(nullptr, executable, MAX_PATH)) return 2;
        const auto dll = std::filesystem::path(executable).parent_path() /
            L"shutdown_storage_probe.dll";
        const HMODULE module = LoadLibraryW(dll.c_str());
        using Prepare = HANDLE(WINAPI*)(const wchar_t*, HANDLE, HANDLE, BOOL);
        using Pin = BOOL(WINAPI*)(HMODULE);
        const auto pin = module ? reinterpret_cast<Pin>(GetProcAddress(module,
            "PinShutdownProbe")) : nullptr;
        const auto prepare = module ? reinterpret_cast<Prepare>(GetProcAddress(
            module, "PrepareShutdownProbe")) : nullptr;
        if (!pin || pin(nullptr) || !pin(module) || !pin(module) || !FreeLibrary(module) ||
            GetModuleHandleW(dll.c_str()) != module) ExitProcess(2);
        const HANDLE ready = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        const HANDLE hold = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        const HANDLE worker = prepare && ready && hold
            ? prepare(path, ready, hold, static_cast<BOOL>(automatic)) : nullptr;
        if (!worker || WaitForSingleObject(ready, 3000) != WAIT_OBJECT_0)
            ExitProcess(2);
        // Deliberately exercise real DLL CRT detach while a worker owns its
        // mutex. The externally supervised process, not a thread, is isolated.
        ExitProcess(7);
    }

    bool TestRealDllExit(const std::filesystem::path& path)
    {
        wchar_t executable[MAX_PATH]{};
        if (!GetModuleFileNameW(nullptr, executable, MAX_PATH)) return false;
        std::wstring command = L"\"" + std::wstring(executable) +
            L"\" --exit-child \"" + path.wstring() + L"\"";
        STARTUPINFOW startup{};
        startup.cb = sizeof(startup);
        PROCESS_INFORMATION child{};
        if (!CreateProcessW(executable, command.data(), nullptr, nullptr, FALSE,
                CREATE_NO_WINDOW, nullptr, nullptr, &startup, &child)) return false;
        CloseHandle(child.hThread);
        DWORD code{};
        const bool exited = WaitForSingleObject(child.hProcess, 5000) == WAIT_OBJECT_0 &&
            GetExitCodeProcess(child.hProcess, &code) && code == 7;
        CloseHandle(child.hProcess);
        // On a regression, the outer process-tree supervisor owns the blocked
        // child and must report/clean it rather than allowing a false PASS.
        std::string text;
        const bool content = exited && platform::win32::ReadNativeFile(path.c_str(), text) &&
            text == "before_exit\r\n";
        if (exited) DeleteFileW(path.c_str());
        return content;
    }
}

int wmain(int argc, wchar_t** argv)
{
    if (argc == 3 && std::wstring_view(argv[1]) == L"--exit-child")
        return ExitChild(argv[2], false);
    if (argc == 3 && std::wstring_view(argv[1]) == L"--automatic-exit-child")
        return ExitChild(argv[2], 1);
    if (argc == 3 && std::wstring_view(argv[1]) == L"--crt-exit-child")
        return ExitChild(argv[2], 2);
    wchar_t temporary[MAX_PATH]{};
    if (!GetTempPathW(MAX_PATH, temporary)) return 1;
    const auto directory = std::filesystem::path(temporary) /
        (L"CameraTweaks-shutdown-" + std::to_wstring(GetCurrentProcessId()));
    if (!CreateDirectoryW(directory.c_str(), nullptr)) return 1;
    const bool normal = TestNormalStartupAndControlledStop();
    const bool immediate = TestImmediatelyCompletingWorkers();
    std::printf("immediate_worker_publication=%s\n", immediate ? "PASS" : "FAIL");
    const bool partial = TestPartialStartupFailure();
    const bool beforeStartup = TestStopBeforeStartup();
    const bool workerCapacity = TestHotkeyPlusDiagnosticWorkerCapacity();
    const bool logsAndStorage = TestNativeLogsAndStorage(directory);
    const bool dllExit = TestRealDllExit(directory / L"dll-exit.txt");
    const bool filesRemoved = RemoveDirectoryW(directory.c_str()) != FALSE;
    std::printf("normal=%s partial=%s startup_stop_observation=%s stop_before_start=%s worker_capacity=%s\n",
        normal ? "PASS" : "FAIL", partial ? "PASS" : "FAIL",
        (normal && partial) ? "PASS" : "FAIL", beforeStartup ? "PASS" : "FAIL",
        workerCapacity ? "PASS" : "FAIL");
    std::printf("native_logs_and_storage=%s real_dll_exit=%s files_removed=%s\n",
        logsAndStorage ? "PASS" : "FAIL", dllExit ? "PASS" : "FAIL",
        filesRemoved ? "PASS" : "FAIL");
    return normal && immediate && partial && beforeStartup && workerCapacity &&
        logsAndStorage && dllExit && filesRemoved ? 0 : 1;
}
