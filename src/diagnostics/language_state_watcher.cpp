#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>

#include "language_state_transition.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    using diagnostics::language_state::IsBOnlyAllocation;
    using diagnostics::language_state::IsMissingOnlyInB;
    using diagnostics::language_state::IsReturnedPage;
    using diagnostics::language_state::PageFingerprint;

    constexpr std::size_t PageBufferSize = 4096;
    constexpr std::uint64_t MaxScannedBytes = 2ull * 1024 * 1024 * 1024;
    constexpr std::size_t MaxRetainedPhaseBPages = 16 * 1024;
    constexpr std::size_t MaxMissingPhaseBRecords = 131072;
    constexpr std::size_t MaxLoggedDiffRuns = 4096;
    struct ModuleRecord
    {
        std::uintptr_t base{};
        std::uintptr_t size{};
        std::wstring path;
    };

    struct CapturedPage
    {
        PageFingerprint fingerprint{};
        std::array<std::uint8_t, PageBufferSize> bytes{};
    };

    struct Snapshot
    {
        std::vector<PageFingerprint> pages;
        std::uint64_t eligibleBytes{};
        std::uint64_t scannedBytes{};
        std::uint64_t unreadablePages{};
        std::uint64_t eligibleRegions{};
        bool truncated{};
    };

    enum class Phase : std::uint8_t { None, A, B, Complete, Failed };

    HANDLE g_stopEvent{};
    std::mutex g_logMutex;
    std::ofstream g_log;
    std::filesystem::path g_logPath;
    std::vector<ModuleRecord> g_modules;
    std::vector<PageFingerprint> g_phaseA;
    std::vector<PageFingerprint> g_phaseB;
    std::vector<CapturedPage> g_retainedBPages;
    std::vector<PageFingerprint> g_missingBPages;
    std::uint64_t g_changedBPages{};
    std::uint64_t g_randomState{0xA71A5EEDu};
    Phase g_phase{Phase::None};
    bool g_previousPhaseAKey{};
    bool g_previousPhaseBKey{};
    bool g_previousPhaseCKey{};

    std::string Narrow(std::wstring_view value)
    {
        if (value.empty()) return {};
        const int required = WideCharToMultiByte(CP_UTF8, 0, value.data(),
            static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        if (required <= 0) return "<utf8-conversion-failed>";
        std::string result(static_cast<std::size_t>(required), '\0');
        WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
            result.data(), required, nullptr, nullptr);
        return result;
    }

    std::string Timestamp()
    {
        SYSTEMTIME value{};
        GetLocalTime(&value);
        std::ostringstream stream;
        stream << std::setfill('0') << std::setw(4) << value.wYear << '-'
            << std::setw(2) << value.wMonth << '-' << std::setw(2) << value.wDay
            << 'T' << std::setw(2) << value.wHour << ':' << std::setw(2) << value.wMinute
            << ':' << std::setw(2) << value.wSecond << '.' << std::setw(3) << value.wMilliseconds;
        return stream.str();
    }

    void Log(std::string_view message)
    {
        std::lock_guard lock(g_logMutex);
        if (!g_log.is_open()) return;
        g_log << '[' << Timestamp() << "] " << message << "\r\n";
        g_log.flush();
    }

    std::uint64_t HashPage(const std::uint8_t* bytes) noexcept
    {
        std::uint64_t hash = 14695981039346656037ull;
        for (std::size_t index = 0; index < PageBufferSize; ++index) {
            hash ^= bytes[index];
            hash *= 1099511628211ull;
        }
        return hash;
    }

    std::uint64_t NextRandom() noexcept
    {
        g_randomState = g_randomState * 6364136223846793005ull + 1442695040888963407ull;
        return g_randomState;
    }

    bool IsReadableWritable(DWORD protection) noexcept
    {
        if ((protection & (PAGE_GUARD | PAGE_NOACCESS)) != 0) return false;
        switch (protection & 0xFF) {
        case PAGE_READWRITE:
        case PAGE_WRITECOPY:
        case PAGE_EXECUTE_READWRITE:
        case PAGE_EXECUTE_WRITECOPY:
            return true;
        default:
            return false;
        }
    }

    const char* MemoryTypeName(DWORD type) noexcept
    {
        switch (type) {
        case MEM_IMAGE: return "image";
        case MEM_PRIVATE: return "private";
        case MEM_MAPPED: return "mapped";
        default: return "other";
        }
    }

    const char* ProtectionName(DWORD protection) noexcept
    {
        switch (protection & 0xFF) {
        case PAGE_READWRITE: return "rw";
        case PAGE_WRITECOPY: return "copy-on-write";
        case PAGE_EXECUTE_READWRITE: return "xrw";
        case PAGE_EXECUTE_WRITECOPY: return "x-copy-on-write";
        default: return "other";
        }
    }

    void EnumerateModules()
    {
        g_modules.clear();
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
            GetCurrentProcessId());
        if (snapshot == INVALID_HANDLE_VALUE) {
            Log("MODULE_MAP unavailable win32=" + std::to_string(GetLastError()));
            return;
        }

        MODULEENTRY32W entry{};
        entry.dwSize = sizeof(entry);
        if (Module32FirstW(snapshot, &entry)) {
            do {
                g_modules.push_back({
                    reinterpret_cast<std::uintptr_t>(entry.modBaseAddr),
                    static_cast<std::uintptr_t>(entry.modBaseSize), entry.szExePath});
            } while (Module32NextW(snapshot, &entry));
        }
        CloseHandle(snapshot);
        Log("MODULE_MAP count=" + std::to_string(g_modules.size()));
    }

    const ModuleRecord* ModuleFor(std::uintptr_t allocationBase) noexcept
    {
        for (const auto& module : g_modules) {
            if (module.base == allocationBase) return &module;
        }
        return nullptr;
    }

    std::vector<PageFingerprint>::const_iterator FindPage(
        const std::vector<PageFingerprint>& pages, std::uintptr_t address)
    {
        return std::lower_bound(pages.begin(), pages.end(), address,
            [](const PageFingerprint& page, std::uintptr_t target) {
                return page.address < target;
            });
    }

    const PageFingerprint* FindPagePointer(
        const std::vector<PageFingerprint>& pages, std::uintptr_t address)
    {
        const auto found = FindPage(pages, address);
        return found != pages.end() && found->address == address ? &*found : nullptr;
    }

    bool IsSameAllocation(const PageFingerprint& left,
        const PageFingerprint& right) noexcept
    {
        return left.address == right.address && left.allocationBase == right.allocationBase &&
            left.memoryType == right.memoryType;
    }

    void RetainChangedPhaseBPage(const PageFingerprint& fingerprint,
        const std::array<std::uint8_t, PageBufferSize>& bytes)
    {
        ++g_changedBPages;
        CapturedPage captured{fingerprint, bytes};
        if (g_retainedBPages.size() < MaxRetainedPhaseBPages) {
            g_retainedBPages.push_back(std::move(captured));
            return;
        }

        const std::uint64_t selected = NextRandom() % g_changedBPages;
        if (selected < MaxRetainedPhaseBPages)
            g_retainedBPages[static_cast<std::size_t>(selected)] = std::move(captured);
    }

    Snapshot CaptureSnapshot(const std::vector<PageFingerprint>* reference, bool retainChanges)
    {
        Snapshot result;
        SYSTEM_INFO systemInfo{};
        GetSystemInfo(&systemInfo);
        const std::uintptr_t pageSize = systemInfo.dwPageSize ? systemInfo.dwPageSize : PageBufferSize;
        if (pageSize != PageBufferSize) {
            result.truncated = true;
            Log("SNAPSHOT_ABORT reason=unexpected-system-page-size page_size=" + std::to_string(pageSize) +
                " expected=" + std::to_string(PageBufferSize));
            return result;
        }
        const std::uintptr_t maxAddress = reinterpret_cast<std::uintptr_t>(
            systemInfo.lpMaximumApplicationAddress);
        std::uintptr_t address = reinterpret_cast<std::uintptr_t>(systemInfo.lpMinimumApplicationAddress);
        std::array<std::uint8_t, PageBufferSize> pageBytes{};
        result.pages.reserve(65536);

        while (address < maxAddress) {
            MEMORY_BASIC_INFORMATION info{};
            if (!VirtualQuery(reinterpret_cast<const void*>(address), &info, sizeof(info))) {
                const std::uintptr_t next = address + pageSize;
                if (next <= address) break;
                address = next;
                continue;
            }

            const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(info.BaseAddress);
            const std::uintptr_t regionEnd = base + info.RegionSize;
            if (regionEnd <= address) break;

            if (info.State == MEM_COMMIT && IsReadableWritable(info.Protect) &&
                (info.Type == MEM_IMAGE || info.Type == MEM_PRIVATE || info.Type == MEM_MAPPED)) {
                ++result.eligibleRegions;
                result.eligibleBytes += info.RegionSize;
                std::uintptr_t pageAddress = base;
                while (pageAddress < regionEnd) {
                    if (result.scannedBytes + pageSize > MaxScannedBytes) {
                        result.truncated = true;
                        break;
                    }

                    SIZE_T bytesRead{};
                    const BOOL copied = ReadProcessMemory(GetCurrentProcess(),
                        reinterpret_cast<const void*>(pageAddress), pageBytes.data(),
                        pageSize, &bytesRead);
                    result.scannedBytes += pageSize;
                    if (copied && bytesRead == pageSize) {
                        PageFingerprint fingerprint{
                            pageAddress, reinterpret_cast<std::uintptr_t>(info.AllocationBase),
                            HashPage(pageBytes.data()), info.Type, info.Protect};
                        result.pages.push_back(fingerprint);

                        if (retainChanges) {
                            const PageFingerprint* previous = reference
                                ? FindPagePointer(*reference, pageAddress) : nullptr;
                            const bool changed = !previous || !IsSameAllocation(*previous, fingerprint) ||
                                previous->hash != fingerprint.hash;
                            if (changed) RetainChangedPhaseBPage(fingerprint, pageBytes);
                        }
                    } else {
                        ++result.unreadablePages;
                    }

                    if (pageAddress > UINTPTR_MAX - pageSize) break;
                    pageAddress += pageSize;
                }
            }

            if (result.truncated) break;
            address = regionEnd;
        }
        return result;
    }

    std::string Hex(std::span<const std::uint8_t> bytes)
    {
        std::ostringstream stream;
        stream << std::hex << std::setfill('0');
        for (std::size_t index = 0; index < bytes.size(); ++index) {
            if (index) stream << ' ';
            stream << std::setw(2) << static_cast<unsigned>(bytes[index]);
        }
        return stream.str();
    }

    std::string TextPreview(const std::uint8_t* bytes, std::size_t size)
    {
        std::ostringstream stream;
        stream << "ascii=\"";
        for (std::size_t index = 0; index < size; ++index) {
            const unsigned char value = bytes[index];
            stream << ((value >= 0x20 && value <= 0x7E) ? static_cast<char>(value) : '.');
        }
        stream << "\" utf16=\"" << std::hex << std::setfill('0');
        for (std::size_t index = 0; index + 1 < size; index += 2) {
            const std::uint16_t value = static_cast<std::uint16_t>(bytes[index]) |
                (static_cast<std::uint16_t>(bytes[index + 1]) << 8);
            if (value >= 0x20 && value <= 0x7E) stream << static_cast<char>(value);
            else if (value == 0) stream << ' ';
            else stream << "\\u" << std::setw(4) << value;
        }
        stream << '"';
        return stream.str();
    }

    void LogPageIdentity(std::string_view kind, const PageFingerprint& page)
    {
        std::ostringstream line;
        line << "CANDIDATE_PAGE kind=" << kind << " address=0x" << std::hex << page.address
            << " allocation_base=0x" << page.allocationBase << std::dec
            << " type=" << MemoryTypeName(page.memoryType)
            << " protection=" << ProtectionName(page.protection)
            << " fingerprint=0x" << std::hex << page.hash << std::dec;
        if (const ModuleRecord* module = ModuleFor(page.allocationBase)) {
            line << " module=\"" << Narrow(module->path) << "\" rva=0x" << std::hex
                << (page.address - module->base);
        } else {
            line << " module=<none>";
        }
        Log(line.str());
    }

    void LogTextRuns(std::string_view kind, const PageFingerprint& page,
        const std::uint8_t* bytes)
    {
        std::size_t emitted{};
        for (std::size_t index = 0; index + 8 < PageBufferSize && emitted < 8;) {
            const bool ascii = bytes[index] >= 0x20 && bytes[index] <= 0x7E;
            const bool wide = index + 1 < PageBufferSize && bytes[index] >= 0x20 &&
                bytes[index] <= 0x7E && bytes[index + 1] == 0;
            if (!ascii && !wide) { ++index; continue; }

            const std::size_t start = index;
            std::size_t count{};
            while (index + (wide ? 1 : 0) < PageBufferSize && count < 160) {
                const bool next = wide
                    ? bytes[index] >= 0x20 && bytes[index] <= 0x7E && bytes[index + 1] == 0
                    : bytes[index] >= 0x20 && bytes[index] <= 0x7E;
                if (!next) break;
                index += wide ? 2 : 1;
                ++count;
            }
            if (count >= 4) {
                std::ostringstream line;
                line << "CANDIDATE_TEXT kind=" << kind << " address=0x" << std::hex
                    << (page.address + start) << std::dec << " encoding=" << (wide ? "utf16-ascii" : "ascii")
                    << " text=\"";
                for (std::size_t offset = 0; offset < count && offset < 160; ++offset)
                    line << static_cast<char>(bytes[start + offset * (wide ? 2 : 1)]);
                line << '"';
                Log(line.str());
                ++emitted;
            }
            if (index == start) ++index;
        }
    }

    void LogDiffRuns(const CapturedPage& phaseB, const std::uint8_t* phaseC,
        std::uint64_t& loggedRuns)
    {
        std::size_t index{};
        while (index < PageBufferSize && loggedRuns < MaxLoggedDiffRuns) {
            if (phaseB.bytes[index] == phaseC[index]) { ++index; continue; }
            const std::size_t start = index;
            while (index < PageBufferSize && phaseB.bytes[index] != phaseC[index] && index - start < 32)
                ++index;
            const std::size_t count = index - start;
            const std::size_t contextStart = start > 16 ? start - 16 : 0;
            const std::size_t contextLength = std::min<std::size_t>(64, PageBufferSize - contextStart);
            std::ostringstream line;
            line << "CANDIDATE_DIFF address=0x" << std::hex << (phaseB.fingerprint.address + start)
                << std::dec << " length=" << count
                << " B={" << Hex(std::span(phaseB.bytes.data() + start, count)) << "}"
                << " A_C={" << Hex(std::span(phaseC + start, count)) << "}"
                << " B_context={" << Hex(std::span(phaseB.bytes.data() + contextStart, contextLength)) << "}"
                << ' ' << TextPreview(phaseB.bytes.data() + contextStart, contextLength)
                << " C_context={" << Hex(std::span(phaseC + contextStart, contextLength)) << "}"
                << ' ' << TextPreview(phaseC + contextStart, contextLength);
            Log(line.str());
            ++loggedRuns;
        }
    }

    void LogSnapshotSummary(std::string_view phase, const Snapshot& snapshot)
    {
        DWORD foregroundPid{};
        const HWND foregroundWindow = GetForegroundWindow();
        if (foregroundWindow) GetWindowThreadProcessId(foregroundWindow, &foregroundPid);
        std::ostringstream line;
        line << "SNAPSHOT phase=" << phase << " pid=" << GetCurrentProcessId()
            << " foreground_pid=" << foregroundPid << " pages=" << snapshot.pages.size()
            << " scanned_bytes=" << snapshot.scannedBytes
            << " eligible_bytes=" << snapshot.eligibleBytes
            << " eligible_regions=" << snapshot.eligibleRegions
            << " unreadable_pages=" << snapshot.unreadablePages
            << " coverage=" << (snapshot.truncated ? "partial-capped" : "all-eligible-regions")
            << " limit_bytes=" << MaxScannedBytes;
        Log(line.str());
    }

    void TakePhaseA()
    {
        if (g_phase != Phase::None) {
            Log("MARKER_REJECTED requested=A reason=phase-order");
            return;
        }
        Log("MARKER phase=A locale=uk status=begin");
        const Snapshot snapshot = CaptureSnapshot(nullptr, false);
        g_phaseA = snapshot.pages;
        LogSnapshotSummary("A", snapshot);
        if (g_phaseA.empty()) {
            g_phase = Phase::Failed;
            Log("WATCHER_FAILED phase=A reason=no-readable-writable-pages");
            return;
        }
        g_phase = Phase::A;
        Log("MARKER phase=A status=complete next=apply-English-then-press-Ctrl-Alt-F7");
    }

    void TakePhaseB()
    {
        if (g_phase != Phase::A) {
            Log("MARKER_REJECTED requested=B reason=phase-order");
            return;
        }
        Log("MARKER phase=B locale=en status=begin");
        g_changedBPages = 0;
        g_retainedBPages.clear();
        g_missingBPages.clear();
        const Snapshot snapshot = CaptureSnapshot(&g_phaseA, true);
        g_phaseB = snapshot.pages;
        for (const auto& pageA : g_phaseA) {
            if (!FindPagePointer(g_phaseB, pageA.address) &&
                g_missingBPages.size() < MaxMissingPhaseBRecords)
                g_missingBPages.push_back(pageA);
        }
        LogSnapshotSummary("B", snapshot);
        std::ostringstream line;
        line << "PHASE_B_CHANGESET changed_pages=" << g_changedBPages
            << " retained_pages=" << g_retainedBPages.size()
            << " retained_page_budget=" << MaxRetainedPhaseBPages
            << " missing_A_pages_recorded=" << g_missingBPages.size()
            << " missing_A_record_limit=" << MaxMissingPhaseBRecords
            << " sampling=" << (g_changedBPages > g_retainedBPages.size() ? "uniform-reservoir" : "complete");
        Log(line.str());
        g_phase = Phase::B;
        Log("MARKER phase=B status=complete next=apply-Ukrainian-then-press-Ctrl-Alt-F8");
    }

    void TakePhaseC()
    {
        if (g_phase != Phase::B) {
            Log("MARKER_REJECTED requested=C reason=phase-order");
            return;
        }
        Log("MARKER phase=C locale=uk status=begin");
        const Snapshot snapshot = CaptureSnapshot(nullptr, false);
        LogSnapshotSummary("C", snapshot);

        std::uint64_t returnedPages{};
        std::uint64_t bOnlyPages{};
        std::uint64_t missingThenReturnedPages{};
        std::uint64_t loggedRuns{};
        std::array<std::uint8_t, PageBufferSize> currentBytes{};

        for (const auto& phaseB : g_retainedBPages) {
            const auto* phaseA = FindPagePointer(g_phaseA, phaseB.fingerprint.address);
            const auto* phaseC = FindPagePointer(snapshot.pages, phaseB.fingerprint.address);

            if (phaseA && phaseC && IsReturnedPage(*phaseA, phaseB.fingerprint, *phaseC)) {
                SIZE_T bytesRead{};
                if (!ReadProcessMemory(GetCurrentProcess(),
                    reinterpret_cast<const void*>(phaseC->address), currentBytes.data(),
                    currentBytes.size(), &bytesRead) || bytesRead != currentBytes.size()) continue;
                ++returnedPages;
                LogPageIdentity("A-B-A", phaseB.fingerprint);
                LogDiffRuns(phaseB, currentBytes.data(), loggedRuns);
            } else if (!phaseA && !phaseC) {
                ++bOnlyPages;
                LogPageIdentity("B-only-allocation", phaseB.fingerprint);
                LogTextRuns("B-only-allocation", phaseB.fingerprint, phaseB.bytes.data());
            } else if (phaseA && phaseC && !IsSameAllocation(*phaseA, phaseB.fingerprint) &&
                IsSameAllocation(*phaseA, *phaseC) && phaseA->hash == phaseC->hash) {
                ++returnedPages;
                LogPageIdentity("A-B-replaced-allocation-A", phaseB.fingerprint);
                SIZE_T bytesRead{};
                if (ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(phaseC->address),
                    currentBytes.data(), currentBytes.size(), &bytesRead) && bytesRead == currentBytes.size())
                    LogDiffRuns(phaseB, currentBytes.data(), loggedRuns);
            }
        }

        for (const auto& phaseA : g_missingBPages) {
            const auto* phaseC = FindPagePointer(snapshot.pages, phaseA.address);
            if (!phaseC || !IsMissingOnlyInB(phaseA, nullptr, *phaseC)) continue;
            ++missingThenReturnedPages;
            LogPageIdentity("A-missing-in-B-returned-in-C", phaseA);
        }

        std::ostringstream summary;
        summary << "AB_A_ANALYSIS returned_pages=" << returnedPages
            << " B_only_allocations=" << bOnlyPages
            << " A_pages_missing-in-B_returned-in-C=" << missingThenReturnedPages
            << " changed_runs_logged=" << loggedRuns
            << " changed_run_cap=" << MaxLoggedDiffRuns
            << " retained_phase_B_sample=" << g_retainedBPages.size() << '/' << g_changedBPages
            << " static_callsite_or_writer_observation=not-performed"
            << " caveat=page-hash-identifies-candidates-not-semantic-language-state";
        Log(summary.str());
        g_phase = Phase::Complete;
        Log("WATCHER_COMPLETE phases=A(uk),B(en),C(uk); no-memory-writes-or-hooks-installed");
    }

    bool InitializeLog()
    {
        wchar_t localAppData[MAX_PATH]{};
        const DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
        if (length == 0 || length >= MAX_PATH) return false;
        g_logPath = std::filesystem::path(localAppData) / L"STALKER2LanguageStateWatcher" /
            L"LanguageStateWatcher.log";
        std::error_code error;
        std::filesystem::create_directories(g_logPath.parent_path(), error);
        if (error) return false;
        g_log.open(g_logPath, std::ios::out | std::ios::app);
        if (!g_log.is_open()) return false;
        Log("RUN_START pid=" + std::to_string(GetCurrentProcessId()) +
            " log=\"" + Narrow(g_logPath.wstring()) + "\"");
        Log("OBSERVATION_BOUNDARY writable-committed-MEM_IMAGE/MEM_PRIVATE/MEM_MAPPED; "
            "read-only pages and executed-callsite tracing are not observed; no UE/UE4SS APIs or signatures used");
        Log("CONTROL Ctrl+Alt+F6=A(Ukrainian stable), Ctrl+Alt+F7=B(English applied and stable), "
            "Ctrl+Alt+F8=C(Ukrainian applied and stable)");
        Log("SAFETY read-only VirtualQuery/ReadProcessMemory snapshots; markers use non-exclusive "
            "50ms foreground-process key-state checks; no PAGE_GUARD, breakpoints, ProcessEvent hooks, "
            "settings writes, or per-frame memory polling");
        return true;
    }

    bool IsForegroundProcess() noexcept
    {
        const HWND foregroundWindow = GetForegroundWindow();
        if (!foregroundWindow) return false;
        DWORD foregroundPid{};
        GetWindowThreadProcessId(foregroundWindow, &foregroundPid);
        return foregroundPid == GetCurrentProcessId();
    }

    bool IsKeyDown(int virtualKey) noexcept
    {
        return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
    }

    DWORD WINAPI WatcherThread(void*)
    {
        if (!InitializeLog()) return 2;
        EnumerateModules();
        Log("WATCHER_READY trigger=foreground-only-key-state; no-hotkey-registration; "
            "wait-for-user-markers=enabled");

        bool running = true;
        while (running) {
            if (WaitForSingleObject(g_stopEvent, 50) != WAIT_TIMEOUT) break;
            const bool focused = IsForegroundProcess();
            const bool chord = focused && IsKeyDown(VK_CONTROL) && IsKeyDown(VK_MENU);
            const bool phaseA = chord && IsKeyDown(VK_F6);
            const bool phaseB = chord && IsKeyDown(VK_F7);
            const bool phaseC = chord && IsKeyDown(VK_F8);

            if (phaseA && !g_previousPhaseAKey) TakePhaseA();
            if (phaseB && !g_previousPhaseBKey) TakePhaseB();
            if (phaseC && !g_previousPhaseCKey) {
                TakePhaseC();
                running = false;
            }
            g_previousPhaseAKey = phaseA;
            g_previousPhaseBKey = phaseB;
            g_previousPhaseCKey = phaseC;
        }

        Log("RUN_STOP");
        return 0;
    }
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        g_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!g_stopEvent) return FALSE;
        HANDLE thread = CreateThread(nullptr, 0, WatcherThread, nullptr, 0, nullptr);
        if (!thread) {
            CloseHandle(g_stopEvent);
            g_stopEvent = nullptr;
            return FALSE;
        }
        CloseHandle(thread);
    } else if (reason == DLL_PROCESS_DETACH && g_stopEvent) {
        SetEvent(g_stopEvent);
    }
    return TRUE;
}
