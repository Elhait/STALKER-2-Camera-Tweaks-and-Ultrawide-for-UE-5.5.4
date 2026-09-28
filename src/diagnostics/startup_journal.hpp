#pragma once

// Bounded production startup milestones. This deliberately does not inspect
// modules or decode instructions, and records no per-frame graphics events.
#if defined(OVERLAY_STARTUP_JOURNAL)
#include <Windows.h>
#include <intrin.h>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cwchar>

namespace diagnostics::startup_journal
{
    constexpr unsigned Capacity = 48;
    constexpr unsigned OnceCapacity = 16;
    constexpr unsigned PathCapacity = 512;

    struct Event
    {
        std::atomic<bool> published{false};
        const char* name{};
        const char* detail{};
        std::uint64_t tsc{}, qpc{}, utc{}, value{};
        DWORD thread{};
    };

    static_assert(std::atomic<unsigned>::is_always_lock_free);
    static_assert(std::atomic<bool>::is_always_lock_free);
    inline Event events[Capacity];
    inline std::atomic<unsigned> next{};
    inline std::atomic<unsigned> onceBits{};
    inline HMODULE module{};
    inline SRWLOCK fileLock = SRWLOCK_INIT;
    inline HANDLE file = INVALID_HANDLE_VALUE;
    inline unsigned drained{};
    inline bool headerWritten{};

    inline Event* Reserve(const char* name, const char* detail,
        std::uint64_t value) noexcept
    {
        const auto index = next.fetch_add(1, std::memory_order_relaxed);
        if (index >= Capacity) return nullptr;
        auto& event = events[index];
        event.name = name;
        event.detail = detail;
        event.value = value;
        return &event;
    }

    inline void Publish(Event& event, bool includeClock) noexcept
    {
        event.tsc = __rdtsc();
        if (includeClock) {
            LARGE_INTEGER qpc{};
            FILETIME utc{};
            QueryPerformanceCounter(&qpc);
            GetSystemTimeAsFileTime(&utc);
            event.qpc = static_cast<std::uint64_t>(qpc.QuadPart);
            event.utc = (static_cast<std::uint64_t>(utc.dwHighDateTime) << 32) |
                utc.dwLowDateTime;
            event.thread = GetCurrentThreadId();
        }
        event.published.store(true, std::memory_order_release);
    }

    inline void ProcessAttach(HMODULE self) noexcept
    {
        module = self;
        if (auto* event = Reserve("DLL_PROCESS_ATTACH", "loader_lock", 0))
            Publish(*event, false);
    }

    inline void Mark(const char* name, const char* detail = "", std::uint64_t value = 0) noexcept
    {
        if (auto* event = Reserve(name, detail, value)) Publish(*event, true);
    }

    inline bool MarkOnce(unsigned id, const char* name, const char* detail = "",
        std::uint64_t value = 0) noexcept
    {
        if (id >= OnceCapacity) return false;
        const unsigned bit = 1u << id;
        if (onceBits.fetch_or(bit, std::memory_order_relaxed) & bit) return false;
        Mark(name, detail, value);
        return true;
    }

    inline bool OpenFile() noexcept
    {
        if (file != INVALID_HANDLE_VALUE) return true;
        wchar_t path[PathCapacity]{};
        const DWORD length = GetModuleFileNameW(module, path, PathCapacity);
        if (!length || length >= PathCapacity) return false;
        wchar_t* slash = wcsrchr(path, L'\\');
        wchar_t* forwardSlash = wcsrchr(path, L'/');
        if (!slash || (forwardSlash && forwardSlash > slash)) slash = forwardSlash;
        if (!slash) return false;
        slash[1] = L'\0';
        constexpr wchar_t filename[] = L"STALKER2CameraTweaksStartup.log";
        const auto directoryLength = wcslen(path);
        if (directoryLength + _countof(filename) > PathCapacity) return false;
        wcscat_s(path, PathCapacity, filename);
        file = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        return file != INVALID_HANDLE_VALUE;
    }

    inline bool Write(const char* data, unsigned length) noexcept
    {
        DWORD written{};
        return WriteFile(file, data, length, &written, nullptr) && written == length;
    }

    inline void Flush() noexcept
    {
        AcquireSRWLockExclusive(&fileLock);
        if (!OpenFile()) {
            ReleaseSRWLockExclusive(&fileLock);
            return;
        }
        char line[512]{};
        if (!headerWritten) {
            LARGE_INTEGER frequency{};
            QueryPerformanceFrequency(&frequency);
            const int count = std::snprintf(line, sizeof(line),
                "STARTUP_JOURNAL v=1 pid=%lu qpcFrequency=%lld capacity=%u\r\n",
                GetCurrentProcessId(), frequency.QuadPart, Capacity);
            if (count > 0 && Write(line, static_cast<unsigned>(count)))
                headerWritten = true;
        }
        const unsigned available = next.load(std::memory_order_acquire);
        const unsigned limit = available < Capacity ? available : Capacity;
        while (drained < limit) {
            auto& event = events[drained];
            if (!event.published.load(std::memory_order_acquire)) break;
            const int count = std::snprintf(line, sizeof(line),
                "qpc=%llu utc=%llu tsc=%llu tid=%lu event=%s detail=%s value=%llu\r\n",
                static_cast<unsigned long long>(event.qpc),
                static_cast<unsigned long long>(event.utc),
                static_cast<unsigned long long>(event.tsc), event.thread,
                event.name ? event.name : "unknown",
                event.detail ? event.detail : "",
                static_cast<unsigned long long>(event.value));
            if (count <= 0 || static_cast<unsigned>(count) >= sizeof(line) ||
                !Write(line, static_cast<unsigned>(count))) break;
            ++drained;
        }
        ReleaseSRWLockExclusive(&fileLock);
    }
}
#endif
