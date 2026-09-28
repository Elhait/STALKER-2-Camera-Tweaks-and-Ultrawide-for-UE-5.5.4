#pragma once

// Opt-in chronology probe. It never participates in startup/readiness decisions.
#if defined(OVERLAY_STARTUP_TIMELINE)
#include <Windows.h>
#include <Psapi.h>
#include <winternl.h>
#include <intrin.h>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>

namespace diagnostics::startup
{
    constexpr unsigned Capacity = 128;
    constexpr unsigned PathCapacity = 384;
    struct Event
    {
        std::atomic<bool> published{false};
        const char* kind{};
        const char* stage{};
        unsigned long long tsc{}, qpc{}, utc{};
        DWORD thread{};
        const void* address{};
        const void* related{};
        unsigned long long value{};
        wchar_t path[PathCapacity]{};
        unsigned char bytes[16]{};
        SIZE_T byteCount{};
        bool truncated{};
    };
    static_assert(std::atomic<unsigned>::is_always_lock_free);
    static_assert(std::atomic<bool>::is_always_lock_free);
    inline Event events[Capacity];
    inline std::atomic<unsigned> next{};
    inline std::atomic<bool> factorySeen[3]{};
    inline std::atomic<bool> keyProbeTaken{};
    inline HMODULE selfModule{}, systemDxgi{};
    inline void* attachStack[24]{};
    inline USHORT attachDepth{};
    inline SRWLOCK flushLock = SRWLOCK_INIT;
    inline HANDLE file = INVALID_HANDLE_VALUE;
    inline unsigned drained{};
    inline void* notificationCookie{};
    inline const char* const exportNames[]{
        "CreateDXGIFactory", "CreateDXGIFactory1", "CreateDXGIFactory2"};

    // Also called while the loader owns its lock. Only inline atomics/intrinsics
    // and copying into our own static storage are permitted on this path.
    inline Event* Reserve(const char* kind, const char* stage) noexcept
    {
        const auto index = next.fetch_add(1, std::memory_order_relaxed);
        if (index >= Capacity) return nullptr;
        auto& event = events[index];
        event.kind = kind;
        event.stage = stage;
        event.tsc = __rdtsc();
        return &event;
    }
    inline void CopyPath(Event& event, const wchar_t* text, unsigned length) noexcept
    {
        if (!text) return;
        const unsigned count = length < PathCapacity - 1 ? length : PathCapacity - 1;
        for (unsigned i = 0; i < count; ++i) event.path[i] = text[i];
        event.path[count] = 0;
        event.truncated = count != length;
    }
    inline void Clock(Event& event) noexcept
    {
        LARGE_INTEGER qpc{};
        FILETIME utc{};
        QueryPerformanceCounter(&qpc);
        GetSystemTimeAsFileTime(&utc);
        event.qpc = static_cast<unsigned long long>(qpc.QuadPart);
        event.utc = (static_cast<unsigned long long>(utc.dwHighDateTime) << 32) |
            utc.dwLowDateTime;
        event.thread = GetCurrentThreadId();
    }
    inline void Mark(const char* stage, const void* address = nullptr,
        unsigned long long value = 0) noexcept
    {
        if (auto* event = Reserve("MARK", stage)) {
            Clock(*event);
            event->address = address;
            event->value = value;
            event->published.store(true, std::memory_order_release);
        }
    }
    inline void ProcessAttach(HMODULE module) noexcept
    {
        selfModule = module;
        Mark("dllmain_attach", module);
        // Addresses only: no symbol resolution, file I/O or hook work in DllMain.
        attachDepth = CaptureStackBackTrace(0, 24, attachStack, nullptr);
    }

    inline bool RelevantName(const UNICODE_STRING* name) noexcept
    {
        if (!name || !name->Buffer) return false;
        const unsigned length = name->Length / sizeof(wchar_t);
        const wchar_t* const suffixes[]{L".asi", L".addon64", L"dxgi.dll",
            L"d3d12.dll", L"dsound.dll", L"winmm.dll", L"dwmapi.dll",
            L"ue4ss.dll", L"optiscaler.dll"};
        for (const auto* suffix : suffixes) {
            unsigned suffixLength = 0;
            while (suffix[suffixLength]) ++suffixLength;
            if (length < suffixLength) continue;
            bool match = true;
            for (unsigned i = 0; i < suffixLength; ++i) {
                auto character = name->Buffer[length - suffixLength + i];
                if (character >= L'A' && character <= L'Z') character += L'a' - L'A';
                if (character != suffix[i]) { match = false; break; }
            }
            if (match) return true;
        }
        return false;
    }
    // LdrDllNotification's documented SDK layout. Loaded and unloaded records
    // have the same fields. Registration is optional and done outside DllMain.
    struct NotificationData
    {
        ULONG flags;
        const UNICODE_STRING* fullName;
        const UNICODE_STRING* baseName;
        void* base;
        ULONG size;
    };
    inline void CALLBACK ModuleNotice(ULONG reason, const NotificationData* data,
        void*) noexcept
    {
        if ((reason != 1 && reason != 2) || !data || !RelevantName(data->baseName)) return;
        if (auto* event = Reserve(reason == 1 ? "MODULE_LOAD" : "MODULE_UNLOAD",
                "loader_notification")) {
            event->address = data->base;
            event->value = data->size;
            if (data->fullName)
                CopyPath(*event, data->fullName->Buffer,
                    data->fullName->Length / sizeof(wchar_t));
            event->published.store(true, std::memory_order_release);
        }
    }
    inline void BeginNotifications() noexcept
    {
        using Register = NTSTATUS(NTAPI*)(ULONG,
            void(CALLBACK*)(ULONG, const NotificationData*, void*), void*, void**);
        const auto ntdll = GetModuleHandleW(L"ntdll.dll");
        const auto registration = reinterpret_cast<Register>(
            ntdll ? GetProcAddress(ntdll, "LdrRegisterDllNotification") : nullptr);
        const NTSTATUS status = registration
            ? registration(0, &ModuleNotice, nullptr, &notificationCookie)
            : static_cast<NTSTATUS>(0xC0000002L);
        Mark("module_notifications_registered", nullptr, static_cast<ULONG>(status));
    }
    inline void SnapshotModules(const char* stage) noexcept
    {
        HMODULE modules[512]{};
        DWORD required{};
        if (!EnumProcessModulesEx(GetCurrentProcess(), modules, sizeof(modules),
                &required, LIST_MODULES_ALL)) {
            Mark("module_snapshot_failed", nullptr, GetLastError());
            return;
        }
        if (required > sizeof(modules)) Mark("module_snapshot_truncated", nullptr, required);
        const unsigned count = required < sizeof(modules)
            ? required / sizeof(HMODULE) : 512;
        for (unsigned i = 0; i < count; ++i) {
            wchar_t path[PathCapacity]{};
            const DWORD length = GetModuleFileNameW(modules[i], path, PathCapacity);
            if (!length || length >= PathCapacity) {
                Mark("module_path_unavailable", modules[i], GetLastError());
                continue;
            }
            const wchar_t* base = std::wcsrchr(path, L'\\');
            base = base ? base + 1 : path;
            UNICODE_STRING name{};
            name.Buffer = const_cast<wchar_t*>(base);
            name.Length = static_cast<USHORT>(std::wcslen(base) * sizeof(wchar_t));
            if (!RelevantName(&name)) continue;
            if (auto* event = Reserve("MODULE_PRESENT", stage)) {
                Clock(*event);
                event->address = modules[i];
                CopyPath(*event, path, length);
                event->published.store(true, std::memory_order_release);
            }
        }
    }
    inline void SnapshotExport(const char* stage, HMODULE module, const char* name) noexcept
    {
        const auto address = module ? GetProcAddress(module, name) : nullptr;
        if (auto* event = Reserve("EXPORT_ENTRY", stage)) {
            Clock(*event);
            event->address = reinterpret_cast<void*>(address);
            event->value = reinterpret_cast<std::uintptr_t>(module);
            // Preserve the name separately from stage, and bounded entry bytes
            // to detect later repatching without following or changing hooks.
            unsigned nameLength = 0;
            while (name[nameLength]) ++nameLength;
            for (unsigned i = 0; i < nameLength && i < PathCapacity - 1; ++i)
                event->path[i] = name[i];
            if (address) ReadProcessMemory(GetCurrentProcess(),
                reinterpret_cast<const void*>(address), event->bytes,
                sizeof(event->bytes), &event->byteCount);
            event->published.store(true, std::memory_order_release);
        }
        if (!address) return;

        // Decode only a short, read-only chain of direct/indirect x64 jumps.
        // Sampling happens at existing checkpoints, never from Present.
        auto* source = reinterpret_cast<const unsigned char*>(address);
        for (unsigned hop = 0; hop < 4; ++hop) {
            unsigned char instruction[16]{};
            SIZE_T read{};
            if (!ReadProcessMemory(GetCurrentProcess(), source, instruction,
                    sizeof(instruction), &read) || read < 2) break;
            const SIZE_T instructionBytes = read;

            const auto sourceAddress = reinterpret_cast<std::uintptr_t>(source);
            std::uintptr_t target{};
            if (instruction[0] == 0xE9 && read >= 5) {
                std::int32_t displacement{};
                std::memcpy(&displacement, instruction + 1, sizeof(displacement));
                target = sourceAddress + 5 + displacement;
            } else if (instruction[0] == 0xEB) {
                const auto displacement = static_cast<std::int8_t>(instruction[1]);
                target = sourceAddress + 2 + displacement;
            } else if (instruction[0] == 0xFF && instruction[1] == 0x25 && read >= 6) {
                std::int32_t displacement{};
                std::memcpy(&displacement, instruction + 2, sizeof(displacement));
                const auto pointerAddress = sourceAddress + 6 + displacement;
                if (!ReadProcessMemory(GetCurrentProcess(),
                        reinterpret_cast<const void*>(pointerAddress), &target,
                        sizeof(target), &read) || read != sizeof(target)) break;
            } else {
                break;
            }

            if (auto* event = Reserve("EXPORT_CHAIN", stage)) {
                Clock(*event);
                event->address = source;
                event->related = reinterpret_cast<const void*>(target);
                event->value = hop;
                event->byteCount = instructionBytes < sizeof(event->bytes)
                    ? instructionBytes : sizeof(event->bytes);
                std::memcpy(event->bytes, instruction, event->byteCount);
                char label[64]{};
                std::snprintf(label, sizeof(label), "%s#%u", name, hop);
                unsigned i = 0;
                for (; label[i] && i < PathCapacity - 1; ++i)
                    event->path[i] = static_cast<unsigned char>(label[i]);
                event->path[i] = 0;
                event->published.store(true, std::memory_order_release);
            }
            source = reinterpret_cast<const unsigned char*>(target);
        }
    }
    inline void SnapshotExports(const char* stage, HMODULE module) noexcept
    {
        for (const auto* name : exportNames) SnapshotExport(stage, module, name);
    }
    inline bool FactoryEnter(unsigned index, const void* caller) noexcept
    {
        if (factorySeen[index].exchange(true, std::memory_order_relaxed)) return false;
        if (auto* event = Reserve("FACTORY_ENTER", exportNames[index])) {
            Clock(*event);
            event->related = caller;
            event->published.store(true, std::memory_order_release);
        }
        return true;
    }
    inline void FactoryReturn(unsigned index, HRESULT result, const void* object) noexcept
    {
        if (auto* event = Reserve("FACTORY_RETURN", exportNames[index])) {
            Clock(*event);
            event->address = object;
            event->value = static_cast<ULONG>(result);
            event->published.store(true, std::memory_order_release);
        }
    }
    inline void Write(const char* text, DWORD length) noexcept
    {
        DWORD written{};
        WriteFile(file, text, length, &written, nullptr);
    }
    inline void Utf8Path(const wchar_t* path, char* output, int capacity) noexcept
    {
        if (!WideCharToMultiByte(CP_UTF8, 0, path, -1, output, capacity,
                nullptr, nullptr)) {
            const char unavailable[] = "path_encoding_unavailable";
            for (unsigned i = 0; i < sizeof(unavailable) && i < static_cast<unsigned>(capacity); ++i)
                output[i] = unavailable[i];
        }
    }
    inline void Flush() noexcept
    {
        AcquireSRWLockExclusive(&flushLock);
        if (file == INVALID_HANDLE_VALUE) {
            wchar_t path[32768]{};
            const auto length = GetModuleFileNameW(selfModule, path, 32768);
            auto* base = length && length < 32768 ? std::wcsrchr(path, L'\\') : nullptr;
            if (base && (base - path) + 36 < 32768) {
                wcscpy_s(base + 1, static_cast<SIZE_T>(32768 - (base - path) - 1),
                    L"STALKER2CameraTweaksStartup.log");
                file = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ,
                    nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            }
            if (file != INVALID_HANDLE_VALUE) {
                LARGE_INTEGER frequency{};
                QueryPerformanceFrequency(&frequency);
                FILETIME created{}, exited{}, kernel{}, user{};
                GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user);
                const unsigned long long processUtc =
                    (static_cast<unsigned long long>(created.dwHighDateTime) << 32) |
                        created.dwLowDateTime;
                char header[320]{};
                const int size = std::snprintf(header, sizeof(header),
                    "STARTUP_TIMELINE v=2 pid=%lu qpcFrequency=%lld capacity=%u "
                    "processCreateUtc=%llu attachStackAtCapacity=%u "
                    "moduleTime=notification_tsc_only moduleLoad=before_dynamic_linking "
                    "preRegistrationLoads=unknown\n",
                    GetCurrentProcessId(), frequency.QuadPart, Capacity, processUtc,
                    attachDepth == 24 ? 1u : 0u);
                if (size > 0) Write(header, static_cast<DWORD>(size));
                for (USHORT i = 0; i < attachDepth; ++i) {
                    wchar_t modulePath[PathCapacity]{};
                    HMODULE module{};
                    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                        reinterpret_cast<LPCWSTR>(attachStack[i]), &module);
                    if (module) GetModuleFileNameW(module, modulePath, PathCapacity);
                    modulePath[PathCapacity - 1] = 0;
                    char utf8[PathCapacity * 3]{};
                    Utf8Path(modulePath, utf8, sizeof(utf8));
                    char line[1600]{};
                    const int count = std::snprintf(line, sizeof(line),
                        "ATTACH_STACK frame=%u address=%p module=%p rva=0x%llx path=%s\n",
                        i, attachStack[i], module,
                        module ? reinterpret_cast<std::uintptr_t>(attachStack[i]) -
                            reinterpret_cast<std::uintptr_t>(module) : 0, utf8);
                    if (count > 0 && count < static_cast<int>(sizeof(line)))
                        Write(line, static_cast<DWORD>(count));
                }
            }
        }
        if (file != INVALID_HANDLE_VALUE) {
            const unsigned end = next.load(std::memory_order_relaxed);
            while (drained < end && drained < Capacity) {
                const auto& event = events[drained];
                if (!event.published.load(std::memory_order_acquire)) break;
                char bytes[33]{};
                for (SIZE_T i = 0; i < event.byteCount && i < 16; ++i)
                    std::snprintf(bytes + i * 2, 3, "%02x", event.bytes[i]);
                FILETIME utc{static_cast<DWORD>(event.utc), static_cast<DWORD>(event.utc >> 32)};
                FILETIME local{};
                SYSTEMTIME wall{};
                if (event.utc && FileTimeToLocalFileTime(&utc, &local))
                    FileTimeToSystemTime(&local, &wall);
                char utf8[PathCapacity * 3]{};
                Utf8Path(event.path, utf8, sizeof(utf8));
                char line[1800]{};
                const int size = std::snprintf(line, sizeof(line),
                    "seq=%u kind=%s stage=%s qpc=%llu tsc=%llu "
                    "wall=%04u-%02u-%02uT%02u:%02u:%02u.%03u tid=%lu "
                    "address=%p related=%p value=0x%llx bytes=%s truncated=%u path=%s\n",
                    drained, event.kind, event.stage, event.qpc, event.tsc,
                    wall.wYear, wall.wMonth, wall.wDay, wall.wHour, wall.wMinute,
                    wall.wSecond, wall.wMilliseconds, event.thread,
                    event.address, event.related, event.value, bytes,
                    event.truncated ? 1u : 0u, utf8);
                if (size > 0 && size < static_cast<int>(sizeof(line)))
                    Write(line, static_cast<DWORD>(size));
                if (event.related && std::strcmp(event.kind, "FACTORY_ENTER") == 0) {
                    HMODULE owner{};
                    wchar_t ownerPath[PathCapacity]{};
                    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(event.related), &owner))
                        GetModuleFileNameW(owner, ownerPath, PathCapacity);
                    ownerPath[PathCapacity - 1] = 0;
                    Utf8Path(ownerPath, utf8, sizeof(utf8));
                    const int ownerSize = std::snprintf(line, sizeof(line),
                        "CALLER_OWNER seq=%u address=%p module=%p path=%s\n",
                        drained, event.related, owner, utf8);
                    if (ownerSize > 0 && ownerSize < static_cast<int>(sizeof(line)))
                        Write(line, static_cast<DWORD>(ownerSize));
                } else if (event.related && std::strcmp(event.kind, "EXPORT_CHAIN") == 0) {
                    HMODULE owner{};
                    wchar_t ownerPath[PathCapacity]{};
                    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(event.related), &owner))
                        GetModuleFileNameW(owner, ownerPath, PathCapacity);
                    ownerPath[PathCapacity - 1] = 0;
                    MEMORY_BASIC_INFORMATION memory{};
                    VirtualQuery(event.related, &memory, sizeof(memory));
                    Utf8Path(ownerPath, utf8, sizeof(utf8));
                    const int ownerSize = std::snprintf(line, sizeof(line),
                        "CHAIN_TARGET_OWNER seq=%u target=%p module=%p path=%s "
                        "allocationBase=%p state=0x%lx protect=0x%lx type=0x%lx\n",
                        drained, event.related, owner, utf8, memory.AllocationBase,
                        memory.State, memory.Protect, memory.Type);
                    if (ownerSize > 0 && ownerSize < static_cast<int>(sizeof(line)))
                        Write(line, static_cast<DWORD>(ownerSize));
                }
                ++drained;
            }
            if (end > Capacity) {
                char overflow[80]{};
                const int size = std::snprintf(overflow, sizeof(overflow),
                    "TIMELINE_OVERFLOW dropped=%u\n", end - Capacity);
                if (size > 0) Write(overflow, static_cast<DWORD>(size));
            }
            FlushFileBuffers(file);
        }
        ReleaseSRWLockExclusive(&flushLock);
    }
    inline void ToggleProbe() noexcept
    {
        if (keyProbeTaken.exchange(true, std::memory_order_relaxed)) return;
        Mark("first_toggle_probe");
        SnapshotModules("toggle_probe");
        SnapshotExports("toggle_probe", systemDxgi);
        Flush();
    }
}
#endif
