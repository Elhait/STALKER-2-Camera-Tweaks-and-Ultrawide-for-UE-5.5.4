#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    constexpr DWORD TargetPid = 25960;
    constexpr std::size_t PageSize = 4096;
    constexpr std::size_t MaxSamplePages = 65536; // 256 MiB retained A baseline
    constexpr std::size_t PagesPerReadBlock = 8;
    constexpr std::size_t ContextBytes = 32;
    constexpr std::size_t MaxDiffRuns = 4096;

    enum class ReadStatus : std::uint8_t { Observed, NotObserved, Unmapped, LifetimeChanged };

    struct Region
    {
        std::uintptr_t begin{};
        std::uintptr_t end{};
        std::uintptr_t allocationBase{};
        DWORD type{};
        DWORD protection{};
        std::uint64_t pageCount{};
    };

    struct SamplePage
    {
        std::uintptr_t address{};
        std::uintptr_t allocationBase{};
        DWORD type{};
        DWORD initialProtection{};
        std::array<std::uint8_t, PageSize> phaseABytes{};
        std::array<ReadStatus, 3> status{ReadStatus::NotObserved, ReadStatus::NotObserved, ReadStatus::NotObserved};
        std::array<std::uintptr_t, 3> phaseAllocation{};
        std::array<DWORD, 3> phaseType{};
        bool phaseBDiffersFromA{};
        bool phaseCEqualsA{};
    };

    struct PhaseSummary
    {
        DWORD foregroundBegin{};
        DWORD foregroundEnd{};
        std::uint64_t observed{};
        std::uint64_t notObserved{};
        std::uint64_t unmapped{};
        std::uint64_t lifetimeChanged{};
    };

    std::vector<Region> g_regions;
    std::vector<SamplePage> g_samples;
    std::ofstream g_log;
    HANDLE g_process{};
    std::uint64_t g_diffRuns{};
    std::uint64_t g_suppressedDiffRuns{};

    std::vector<std::uint64_t> SelectSampleIndices(std::uint64_t eligiblePages,
        std::uint64_t cap)
    {
        const auto sampleCount = std::min(eligiblePages, cap);
        std::vector<std::uint64_t> result;
        result.reserve(static_cast<std::size_t>(sampleCount));
        for (std::uint64_t i = 0; i < sampleCount; ++i)
            result.push_back(((2 * i + 1) * eligiblePages) / (2 * sampleCount));
        return result;
    }

    std::string Hex(std::span<const std::uint8_t> bytes)
    {
        std::ostringstream out;
        out << std::hex << std::setfill('0');
        for (std::size_t i = 0; i < bytes.size(); ++i) {
            if (i) out << ' ';
            out << std::setw(2) << static_cast<unsigned>(bytes[i]);
        }
        return out.str();
    }

    void Log(std::string_view message)
    {
        SYSTEMTIME now{};
        GetLocalTime(&now);
        if (g_log.is_open()) {
            g_log << '[' << std::setfill('0') << std::setw(4) << now.wYear << '-'
                << std::setw(2) << now.wMonth << '-' << std::setw(2) << now.wDay << 'T'
                << std::setw(2) << now.wHour << ':' << std::setw(2) << now.wMinute << ':'
                << std::setw(2) << now.wSecond << '.' << std::setw(3) << now.wMilliseconds
                << "] " << message << "\r\n";
            g_log.flush();
        }
        std::cout << message << std::endl;
    }

    DWORD ForegroundPid() noexcept
    {
        const HWND window = GetForegroundWindow();
        if (!window) return 0;
        DWORD pid{};
        GetWindowThreadProcessId(window, &pid);
        return pid;
    }

    bool ReadableWritable(DWORD protection) noexcept
    {
        if ((protection & (PAGE_GUARD | PAGE_NOACCESS)) != 0) return false;
        switch (protection & 0xff) {
        case PAGE_READWRITE:
        case PAGE_WRITECOPY:
        case PAGE_EXECUTE_READWRITE:
        case PAGE_EXECUTE_WRITECOPY:
            return true;
        default: return false;
        }
    }

    const char* StatusName(ReadStatus status) noexcept
    {
        switch (status) {
        case ReadStatus::Observed: return "OBSERVED";
        case ReadStatus::NotObserved: return "NOT_OBSERVED";
        case ReadStatus::Unmapped: return "UNMAPPED_INVALIDATED";
        case ReadStatus::LifetimeChanged: return "ALLOCATION_LIFETIME_CHANGED";
        }
        return "UNKNOWN";
    }

    bool EnumerateEligibleRegions(std::uint64_t& eligiblePages, std::uint64_t& eligibleBytes)
    {
        g_regions.clear();
        eligiblePages = 0;
        eligibleBytes = 0;
        SYSTEM_INFO system{};
        GetSystemInfo(&system);
        const std::uintptr_t pageSize = system.dwPageSize ? system.dwPageSize : PageSize;
        if (pageSize != PageSize) {
            Log("CENSUS_ABORT reason=unexpected_page_size");
            return false;
        }
        auto address = reinterpret_cast<std::uintptr_t>(system.lpMinimumApplicationAddress);
        const auto maxAddress = reinterpret_cast<std::uintptr_t>(system.lpMaximumApplicationAddress);
        std::uint64_t queried{};
        while (address < maxAddress) {
            MEMORY_BASIC_INFORMATION info{};
            if (!VirtualQueryEx(g_process, reinterpret_cast<const void*>(address), &info, sizeof(info))) {
                if (address > UINTPTR_MAX - pageSize) break;
                address += pageSize;
                continue;
            }
            ++queried;
            const auto base = reinterpret_cast<std::uintptr_t>(info.BaseAddress);
            const auto end = base + info.RegionSize;
            if (end <= address) break;
            if (info.State == MEM_COMMIT && ReadableWritable(info.Protect) &&
                (info.Type == MEM_IMAGE || info.Type == MEM_PRIVATE || info.Type == MEM_MAPPED)) {
                const std::uint64_t pages = info.RegionSize / PageSize;
                if (pages != 0) {
                    g_regions.push_back({base, end,
                        reinterpret_cast<std::uintptr_t>(info.AllocationBase),
                        info.Type, info.Protect, pages});
                    eligiblePages += pages;
                    eligibleBytes += pages * PageSize;
                }
            }
            address = end;
        }
        Log("CENSUS queried_regions=" + std::to_string(queried) +
            " eligible_regions=" + std::to_string(g_regions.size()) +
            " eligible_pages=" + std::to_string(eligiblePages) +
            " eligible_bytes=" + std::to_string(eligibleBytes) +
            " metadata_only=true");
        return eligiblePages != 0 && !g_regions.empty();
    }

    bool BuildFixedSample(std::uint64_t eligiblePages)
    {
        std::uint64_t eligibleBlocks{};
        for (const auto& region : g_regions)
            eligibleBlocks += region.pageCount / PagesPerReadBlock;
        const std::uint64_t sampleBlocks = std::min<std::uint64_t>(eligibleBlocks,
            MaxSamplePages / PagesPerReadBlock);
        if (sampleBlocks == 0) return false;
        g_samples.clear();
        g_samples.reserve(static_cast<std::size_t>(sampleBlocks * PagesPerReadBlock));
        const auto selectedIndices = SelectSampleIndices(eligibleBlocks, sampleBlocks);

        // Systematic sampling across the full ordered set of eligible pages.
        // This list is materialized once, before A, and reused unchanged for B and C.
        std::uint64_t globalBlockIndex{};
        std::size_t nextSelected{};
        for (const auto& region : g_regions) {
            const auto blocksInRegion = region.pageCount / PagesPerReadBlock;
            for (std::uint64_t block = 0; block < blocksInRegion; ++block) {
                if (nextSelected < selectedIndices.size() &&
                    globalBlockIndex == selectedIndices[nextSelected]) {
                    const auto blockAddress = region.begin +
                        static_cast<std::uintptr_t>(block * PagesPerReadBlock * PageSize);
                    for (std::size_t page = 0; page < PagesPerReadBlock; ++page)
                        g_samples.push_back({blockAddress + page * PageSize,
                            region.allocationBase, region.type, region.protection});
                    ++nextSelected;
                }
                ++globalBlockIndex;
            }
        }
        if (g_samples.size() != sampleBlocks * PagesPerReadBlock) {
            Log("SAMPLE_ABORT reason=planned_count_mismatch planned=" +
                std::to_string(sampleBlocks * PagesPerReadBlock) +
                " selected=" + std::to_string(g_samples.size()));
            return false;
        }
        const std::uint64_t stride = (eligibleBlocks + sampleBlocks - 1) / sampleBlocks;
        Log("SAMPLE_SET method=deterministic-systematic-global-page-index"
            " cap_pages=" + std::to_string(MaxSamplePages) +
            " blocks=" + std::to_string(sampleBlocks) +
            " pages_per_block=" + std::to_string(PagesPerReadBlock) +
            " selected_pages=" + std::to_string(g_samples.size()) +
            " selected_bytes=" + std::to_string(g_samples.size() * PageSize) +
            " eligible_pages=" + std::to_string(eligiblePages) +
            " eligible_blocks=" + std::to_string(eligibleBlocks) +
            " approximate_stride=" + std::to_string(stride) +
            " address_span_start=0x" + [&] { std::ostringstream s; s << std::hex << g_samples.front().address; return s.str(); }() +
            " address_span_end=0x" + [&] { std::ostringstream s; s << std::hex << g_samples.back().address; return s.str(); }() +
            " fixed_for_all_phases=true");
        return true;
    }

    ReadStatus ReadBlock(std::span<SamplePage> samples,
        std::array<std::uint8_t, PageSize * PagesPerReadBlock>& bytes,
        std::uintptr_t& observedBase, DWORD& observedType, DWORD& observedProtection, DWORD& error)
    {
        const auto& first = samples.front();
        const auto blockBytes = samples.size() * PageSize;
        MEMORY_BASIC_INFORMATION info{};
        if (!VirtualQueryEx(g_process, reinterpret_cast<const void*>(first.address), &info, sizeof(info))) {
            error = GetLastError();
            return ReadStatus::NotObserved;
        }
        observedBase = reinterpret_cast<std::uintptr_t>(info.AllocationBase);
        observedType = info.Type;
        observedProtection = info.Protect;
        if (info.State != MEM_COMMIT) return ReadStatus::Unmapped;
        if (observedBase != first.allocationBase || observedType != first.type)
            return ReadStatus::LifetimeChanged;
        if (!ReadableWritable(info.Protect) ||
            reinterpret_cast<std::uintptr_t>(info.BaseAddress) > first.address ||
            first.address + blockBytes > reinterpret_cast<std::uintptr_t>(info.BaseAddress) + info.RegionSize) {
            error = ERROR_NOACCESS;
            return ReadStatus::NotObserved;
        }
        SIZE_T bytesRead{};
        if (!ReadProcessMemory(g_process, reinterpret_cast<const void*>(first.address),
            bytes.data(), blockBytes, &bytesRead) || bytesRead != blockBytes) {
            error = GetLastError();
            return ReadStatus::NotObserved;
        }
        return ReadStatus::Observed;
    }

    void LogAbContext(const SamplePage& sample, std::size_t pageOffset,
        std::size_t length, const std::array<std::uint8_t, PageSize>& bBytes)
    {
        if (g_diffRuns >= MaxDiffRuns) { ++g_suppressedDiffRuns; return; }
        const auto begin = pageOffset > ContextBytes ? pageOffset - ContextBytes : 0;
        const auto end = std::min(PageSize, pageOffset + length + ContextBytes);
        const auto size = end - begin;
        std::ostringstream row;
        row << "BYTE_CONTEXT_AB address=0x" << std::hex << sample.address
            << " offset=0x" << pageOffset << " changed_length=" << std::dec << length
            << " context_offset=0x" << std::hex << begin
            << " A={" << Hex(std::span(sample.phaseABytes).subspan(begin, size))
            << "} B={" << Hex(std::span(bBytes).subspan(begin, size)) << '}';
        Log(row.str());
        ++g_diffRuns;
    }

    void LogAcContext(const SamplePage& sample, std::size_t pageOffset,
        std::size_t length, const std::array<std::uint8_t, PageSize>& cBytes)
    {
        if (g_diffRuns >= MaxDiffRuns) { ++g_suppressedDiffRuns; return; }
        const auto begin = pageOffset > ContextBytes ? pageOffset - ContextBytes : 0;
        const auto end = std::min(PageSize, pageOffset + length + ContextBytes);
        const auto size = end - begin;
        std::ostringstream row;
        row << "BYTE_CONTEXT_AC address=0x" << std::hex << sample.address
            << " offset=0x" << pageOffset << " changed_length=" << std::dec << length
            << " context_offset=0x" << std::hex << begin
            << " A={" << Hex(std::span(sample.phaseABytes).subspan(begin, size))
            << "} C={" << Hex(std::span(cBytes).subspan(begin, size)) << '}';
        Log(row.str());
        ++g_diffRuns;
    }

    std::uint64_t LogChangedRuns(const SamplePage& sample,
        const std::array<std::uint8_t, PageSize>& bBytes)
    {
        std::uint64_t runs{};
        std::size_t i{};
        while (i < PageSize) {
            while (i < PageSize && sample.phaseABytes[i] == bBytes[i]) ++i;
            if (i == PageSize) break;
            const auto begin = i;
            while (i < PageSize && sample.phaseABytes[i] != bBytes[i]) ++i;
            ++runs;
            LogAbContext(sample, begin, i - begin, bBytes);
        }
        return runs;
    }

    void LogChangedRunsAC(const SamplePage& sample,
        const std::array<std::uint8_t, PageSize>& cBytes)
    {
        std::size_t i{};
        while (i < PageSize) {
            while (i < PageSize && sample.phaseABytes[i] == cBytes[i]) ++i;
            if (i == PageSize) break;
            const auto begin = i;
            while (i < PageSize && sample.phaseABytes[i] != cBytes[i]) ++i;
            LogAcContext(sample, begin, i - begin, cBytes);
        }
    }

    void CapturePhase(char phase, int phaseIndex, const char* locale)
    {
        PhaseSummary summary;
        summary.foregroundBegin = ForegroundPid();
        Log(std::string("MARKER phase=") + phase + " locale=" + locale +
            " target_pid=" + std::to_string(TargetPid) +
            " foreground_pid_at_begin=" + std::to_string(summary.foregroundBegin) + " status=begin");
        std::array<std::uint8_t, PageSize * PagesPerReadBlock> scratch{};
        std::uint64_t changedPages{};
        for (std::size_t blockStart = 0; blockStart < g_samples.size();
            blockStart += PagesPerReadBlock) {
            auto block = std::span(g_samples).subspan(blockStart, PagesPerReadBlock);
            std::uintptr_t observedBase{};
            DWORD observedType{}, observedProtection{}, error{};
            const auto status = ReadBlock(block, scratch, observedBase, observedType, observedProtection, error);
            for (std::size_t pageIndex = 0; pageIndex < block.size(); ++pageIndex) {
                auto& sample = block[pageIndex];
                sample.status[phaseIndex] = status;
                sample.phaseAllocation[phaseIndex] = observedBase;
                sample.phaseType[phaseIndex] = observedType;
            }
            const auto blockAddress = block.front().address;
            switch (status) {
            case ReadStatus::Observed:
                summary.observed += block.size();
                for (std::size_t pageIndex = 0; pageIndex < block.size(); ++pageIndex) {
                    auto& sample = block[pageIndex];
                    auto pageBytes = std::span(scratch).subspan(pageIndex * PageSize, PageSize);
                    if (phaseIndex == 0) {
                        std::copy(pageBytes.begin(), pageBytes.end(), sample.phaseABytes.begin());
                    } else if (phaseIndex == 1) {
                        sample.phaseBDiffersFromA = !std::equal(pageBytes.begin(), pageBytes.end(),
                            sample.phaseABytes.begin());
                        if (sample.phaseBDiffersFromA) {
                            ++changedPages;
                            std::array<std::uint8_t, PageSize> bPage{};
                            std::copy(pageBytes.begin(), pageBytes.end(), bPage.begin());
                            LogChangedRuns(sample, bPage);
                        }
                    } else {
                        sample.phaseCEqualsA = std::equal(pageBytes.begin(), pageBytes.end(),
                            sample.phaseABytes.begin());
                        if (!sample.phaseCEqualsA) {
                            std::array<std::uint8_t, PageSize> cPage{};
                            std::copy(pageBytes.begin(), pageBytes.end(), cPage.begin());
                            LogChangedRunsAC(sample, cPage);
                        }
                    }
                }
                break;
            case ReadStatus::NotObserved:
                summary.notObserved += block.size();
                Log("PAGE_BLOCK_STATUS phase=" + std::string(1, phase) + " address=0x" +
                    [&] { std::ostringstream s; s << std::hex << blockAddress; return s.str(); }() +
                    " pages=" + std::to_string(block.size()) +
                    " status=NOT_OBSERVED win32=" + std::to_string(error));
                break;
            case ReadStatus::Unmapped:
                summary.unmapped += block.size();
                Log("PAGE_BLOCK_STATUS phase=" + std::string(1, phase) + " address=0x" +
                    [&] { std::ostringstream s; s << std::hex << blockAddress; return s.str(); }() +
                    " pages=" + std::to_string(block.size()) + " status=UNMAPPED_INVALIDATED");
                break;
            case ReadStatus::LifetimeChanged:
                summary.lifetimeChanged += block.size();
                Log("PAGE_BLOCK_STATUS phase=" + std::string(1, phase) + " address=0x" +
                    [&] { std::ostringstream s; s << std::hex << blockAddress; return s.str(); }() +
                    " pages=" + std::to_string(block.size()) +
                    " status=ALLOCATION_LIFETIME_CHANGED expected_base=0x" +
                    [&] { std::ostringstream s; s << std::hex << block.front().allocationBase; return s.str(); }() +
                    " observed_base=0x" + [&] { std::ostringstream s; s << std::hex << observedBase; return s.str(); }());
                break;
            }
        }
        summary.foregroundEnd = ForegroundPid();
        Log("PHASE_SUMMARY phase=" + std::string(1, phase) +
            " target_pid=" + std::to_string(TargetPid) +
            " foreground_pid_at_begin=" + std::to_string(summary.foregroundBegin) +
            " foreground_pid_at_end=" + std::to_string(summary.foregroundEnd) +
            " planned=" + std::to_string(g_samples.size()) +
            " observed_full_pages=" + std::to_string(summary.observed) +
            " NOT_OBSERVED=" + std::to_string(summary.notObserved) +
            " UNMAPPED_INVALIDATED=" + std::to_string(summary.unmapped) +
            " ALLOCATION_LIFETIME_CHANGED=" + std::to_string(summary.lifetimeChanged) +
            " B_changed_pages=" + std::to_string(changedPages) +
            " byte_context_suppressed=" + std::to_string(g_suppressedDiffRuns) +
            " focus_status=" + ((summary.foregroundBegin == TargetPid && summary.foregroundEnd == TargetPid)
                ? "STABLE" : "CHANGED_OR_NOT_TARGET") +
            " status=SNAPSHOT_COMPLETE");
        MessageBeep(MB_OK);
    }

    bool SameAllocationAllPhases(const SamplePage& sample) noexcept
    {
        return sample.status[0] == ReadStatus::Observed &&
            sample.status[1] == ReadStatus::Observed &&
            sample.status[2] == ReadStatus::Observed &&
            sample.phaseAllocation[0] == sample.phaseAllocation[1] &&
            sample.phaseAllocation[0] == sample.phaseAllocation[2] &&
            sample.phaseType[0] == sample.phaseType[1] &&
            sample.phaseType[0] == sample.phaseType[2];
    }

    void Analyze()
    {
        std::uint64_t observedAba{}, observedOther{}, notObserved{}, unmapped{}, lifetimeChanged{};
        for (const auto& sample : g_samples) {
            const bool allReadsObserved = sample.status[0] == ReadStatus::Observed &&
                sample.status[1] == ReadStatus::Observed && sample.status[2] == ReadStatus::Observed;
            if (sample.status[0] == ReadStatus::Unmapped ||
                sample.status[1] == ReadStatus::Unmapped || sample.status[2] == ReadStatus::Unmapped) {
                ++unmapped;
            } else if (sample.status[0] == ReadStatus::LifetimeChanged ||
                sample.status[1] == ReadStatus::LifetimeChanged ||
                sample.status[2] == ReadStatus::LifetimeChanged) {
                ++lifetimeChanged;
            } else if (!allReadsObserved) {
                ++notObserved;
            } else if (!SameAllocationAllPhases(sample)) {
                ++lifetimeChanged;
            } else {
                if (sample.phaseBDiffersFromA && sample.phaseCEqualsA) {
                    ++observedAba;
                    std::ostringstream row;
                    row << "CANDIDATE_RESULT class=OBSERVED_A_B_A address=0x" << std::hex << sample.address
                        << " allocation_base=0x" << sample.allocationBase
                        << " type=" << sample.type
                        << " A_equals_C=verified-full-page-memcmp A_differs_B=verified-full-page-memcmp";
                    Log(row.str());
                } else {
                    ++observedOther;
                }
            }
        }
        Log("ANALYSIS OBSERVED_A_B_A=" + std::to_string(observedAba) +
            " OBSERVED_OTHER_TRANSITION=" + std::to_string(observedOther) +
            " NOT_OBSERVED=" + std::to_string(notObserved) +
            " UNMAPPED_INVALIDATED=" + std::to_string(unmapped) +
            " ALLOCATION_LIFETIME_CHANGED=" + std::to_string(lifetimeChanged) +
            " byte_context_runs_logged=" + std::to_string(g_diffRuns) +
            " byte_context_cap=" + std::to_string(MaxDiffRuns) +
            " byte_context_suppressed=" + std::to_string(g_suppressedDiffRuns) +
            " coverage_contract=fixed-address-set-and-full-4096-byte-read-in-A-B-C");
    }

    bool ProcessIdentity(std::string& details)
    {
        g_process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE,
            FALSE, TargetPid);
        if (!g_process) {
            details = "OpenProcess failed win32=" + std::to_string(GetLastError());
            return false;
        }
        FILETIME creation{}, exitTime{}, kernel{}, user{};
        if (!GetProcessTimes(g_process, &creation, &exitTime, &kernel, &user)) {
            details = "GetProcessTimes failed win32=" + std::to_string(GetLastError());
            return false;
        }
        wchar_t path[32768]{};
        DWORD length = static_cast<DWORD>(std::size(path));
        if (!QueryFullProcessImageNameW(g_process, 0, path, &length)) {
            details = "QueryFullProcessImageName failed win32=" + std::to_string(GetLastError());
            return false;
        }
        const std::wstring image(path, length);
        const auto name = std::filesystem::path(image).filename().wstring();
        if (_wcsicmp(name.c_str(), L"Stalker2-Win64-Shipping.exe") != 0) {
            details = "wrong_target_image_rejected";
            return false;
        }
        std::ostringstream out;
        out << "PROCESS_IDENTITY pid=" << TargetPid << " image=";
        const int count = WideCharToMultiByte(CP_UTF8, 0, image.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string utf8(count > 0 ? static_cast<std::size_t>(count) : 0, '\0');
        if (count > 1) {
            WideCharToMultiByte(CP_UTF8, 0, image.c_str(), -1, utf8.data(), count, nullptr, nullptr);
            utf8.resize(static_cast<std::size_t>(count - 1));
        }
        out << utf8 << " creation_filetime=" << creation.dwHighDateTime << ':' << creation.dwLowDateTime
            << " access=query+vm_read only";
        details = out.str();
        return true;
    }

    bool InitializeLog()
    {
        wchar_t local[MAX_PATH]{};
        const DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", local, MAX_PATH);
        if (length == 0 || length >= MAX_PATH) return false;
        const auto dir = std::filesystem::path(local) / L"STALKER2LanguageStateWatcher";
        std::error_code error;
        std::filesystem::create_directories(dir, error);
        if (error) return false;
        SYSTEMTIME now{};
        GetLocalTime(&now);
        std::wostringstream name;
        name << L"Stratified-" << now.wYear << std::setfill(L'0') << std::setw(2) << now.wMonth
            << std::setw(2) << now.wDay << L'-' << std::setw(2) << now.wHour
            << std::setw(2) << now.wMinute << std::setw(2) << now.wSecond << L".log";
        g_log.open(dir / name.str(), std::ios::out | std::ios::trunc);
        return g_log.is_open();
    }

    bool WaitForMarker(int key)
    {
        bool previous{};
        for (;;) {
            if (WaitForSingleObject(g_process, 0) != WAIT_TIMEOUT) return false;
            const bool chord = ForegroundPid() == TargetPid &&
                (GetAsyncKeyState(VK_CONTROL) & 0x8000) &&
                (GetAsyncKeyState(VK_MENU) & 0x8000) &&
                (GetAsyncKeyState(key) & 0x8000);
            if (chord && !previous) return true;
            previous = chord;
            Sleep(50);
        }
    }

    int SelfTest()
    {
        const std::array<Region, 3> regions{{
            {0x1000, 0x5000, 0x1000, MEM_PRIVATE, PAGE_READWRITE, 4},
            {0x9000, 0xb000, 0x9000, MEM_IMAGE, PAGE_READWRITE, 2},
            {0x15000, 0x16000, 0x15000, MEM_MAPPED, PAGE_READWRITE, 1}
        }};
        std::vector<std::uintptr_t> addresses;
        for (const auto& region : regions)
            for (auto address = region.begin; address + PageSize <= region.end; address += PageSize)
                addresses.push_back(address);
        const auto selected = SelectSampleIndices(addresses.size(), 4);
        const auto selectedAgain = SelectSampleIndices(addresses.size(), 4);
        bool pass = addresses.size() == 7;
        pass &= std::is_sorted(addresses.begin(), addresses.end());
        pass &= selected == selectedAgain;
        pass &= selected == std::vector<std::uint64_t>{0, 2, 4, 6};
        pass &= addresses[selected.front()] == 0x1000 && addresses[selected.back()] == 0x15000;
        SamplePage a{};
        a.status = {ReadStatus::Observed, ReadStatus::Observed, ReadStatus::Observed};
        a.phaseAllocation = {0x1000, 0x1000, 0x1000};
        a.phaseType = {MEM_PRIVATE, MEM_PRIVATE, MEM_PRIVATE};
        a.phaseBDiffersFromA = true;
        a.phaseCEqualsA = true;
        pass &= SameAllocationAllPhases(a) && a.phaseBDiffersFromA && a.phaseCEqualsA;
        a.status[1] = ReadStatus::NotObserved;
        pass &= !SameAllocationAllPhases(a);
        std::cout << "synthetic_regions=7 deterministic_uniform_indices=checked fixed_set_reused=checked"
            << " incomplete_phase_cannot_classify=checked "
            << (pass ? "PASS" : "FAIL") << std::endl;
        return pass ? 0 : 1;
    }
}

int wmain(int argc, wchar_t** argv)
{
    if (argc == 2 && _wcsicmp(argv[1], L"--self-test") == 0) return SelfTest();
    std::string identity;
    if (!ProcessIdentity(identity)) {
        std::cerr << "TARGET_REJECTED " << identity << std::endl;
        return 2;
    }
    if (argc == 2 && _wcsicmp(argv[1], L"--preflight") == 0) {
        std::cout << identity << std::endl;
        std::uint64_t pages{}, bytes{};
        if (!EnumerateEligibleRegions(pages, bytes) || !BuildFixedSample(pages)) return 3;
        std::cout << "PREFLIGHT_COMPLETE metadata_only=true selected_pages=" << g_samples.size()
            << " selected_bytes=" << g_samples.size() * PageSize << std::endl;
        CloseHandle(g_process);
        return 0;
    }
    if (!InitializeLog()) {
        std::cerr << "log_open_failed" << std::endl;
        return 2;
    }
    Log("RUN_START");
    Log(identity);
    Log("OBSERVATION_BOUNDARY external read-only VirtualQueryEx/ReadProcessMemory; no writes, hooks, PAGE_GUARD, or executable-callsite tracing");
    Log("SAFETY process handle rights=PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_VM_READ|SYNCHRONIZE");
    std::uint64_t eligiblePages{}, eligibleBytes{};
    if (!EnumerateEligibleRegions(eligiblePages, eligibleBytes) || !BuildFixedSample(eligiblePages)) {
        Log("RUN_ABORT reason=fixed_sample_creation_failed");
        CloseHandle(g_process);
        return 3;
    }
    Log("CONTROL Ctrl+Alt+F6=A(Ukrainian stable), Ctrl+Alt+F7=B(English applied/stable), Ctrl+Alt+F8=C(Ukrainian applied/stable)");
    Log("WATCHER_READY sample_fixed=true phases=A/B/C foreground_gated=true");
    if (!WaitForMarker(VK_F6)) { Log("RUN_ABORT reason=target_exited_before_A"); CloseHandle(g_process); return 4; }
    CapturePhase('A', 0, "uk");
    if (!WaitForMarker(VK_F7)) { Log("RUN_ABORT reason=target_exited_before_B"); CloseHandle(g_process); return 4; }
    CapturePhase('B', 1, "en");
    if (!WaitForMarker(VK_F8)) { Log("RUN_ABORT reason=target_exited_before_C"); CloseHandle(g_process); return 4; }
    CapturePhase('C', 2, "uk");
    Analyze();
    Log("RUN_COMPLETE phases=A(uk),B(en),C(uk); process_memory_read_only=true");
    CloseHandle(g_process);
    return 0;
}
