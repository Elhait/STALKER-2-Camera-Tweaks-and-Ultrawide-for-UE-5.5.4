#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

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
    constexpr std::size_t ContextBytes = 32;
    constexpr std::size_t MaxDiffRuns = 1024;

    struct Candidate
    {
        std::uintptr_t address;
        std::uintptr_t expectedAllocation;
        bool priorityCluster;
    };

    constexpr Candidate SourceCandidates[] = {
        {0xb1859fd000ull, 0xb185900000ull, false},
        {0x267841a9000ull, 0x267841a0000ull, false},
        {0x26784879000ull, 0x26784870000ull, false},
        {0x26784938000ull, 0x26784930000ull, false},
        {0x26784e45000ull, 0x26784e40000ull, false},
        {0x26790950000ull, 0x26790950000ull, false},
        {0x26792a79000ull, 0x26792990000ull, false},
        {0x26795188000ull, 0x26795150000ull, false},
        {0x2679769f000ull, 0x26797690000ull, false},
        {0x26798e6b000ull, 0x26798990000ull, false},
        {0x267999a5000ull, 0x267999a0000ull, false},
        {0x2679ca95000ull, 0x2679ca90000ull, false},
        {0x2679cbf4000ull, 0x2679cbf0000ull, false},
        {0x2679cbf5000ull, 0x2679cbf0000ull, false},
        {0x2679f222000ull, 0x2679f220000ull, false},
        {0x2679f22b000ull, 0x2679f220000ull, false},
        {0x2679f2db000ull, 0x2679f2d0000ull, false},
        {0x267a680a000ull, 0x267a6000000ull, false},
        {0x267a89e0000ull, 0x267a8200000ull, false},
        {0x267b4ce4000ull, 0x267b4ce0000ull, false},
        {0x267b521c000ull, 0x267b5210000ull, false},
        {0x267b521d000ull, 0x267b5210000ull, false},
        {0x267b521e000ull, 0x267b5210000ull, false},
        {0x267b5551000ull, 0x267b5550000ull, false},
        {0x268097b2000ull, 0x26809290000ull, true},
        {0x2680a259000ull, 0x26809290000ull, true},
        {0x2680a328000ull, 0x26809290000ull, true},
        {0x2680a363000ull, 0x26809290000ull, true},
        {0x2680a3c5000ull, 0x26809290000ull, true},
        {0x2680a51b000ull, 0x26809290000ull, true},
        {0x2680a539000ull, 0x26809290000ull, true},
        {0x2680a81c000ull, 0x26809290000ull, true},
        {0x2680bf45000ull, 0x26809290000ull, true},
        {0x2680c1a7000ull, 0x26809290000ull, true},
        {0x2680de4d000ull, 0x26809290000ull, true},
        {0x2680de4e000ull, 0x26809290000ull, true},
        {0x268104ca000ull, 0x26809290000ull, true},
        {0x268112e3000ull, 0x26809290000ull, true},
        {0x268112e7000ull, 0x26809290000ull, true},
        {0x26811315000ull, 0x26809290000ull, true},
        {0x26811318000ull, 0x26809290000ull, true},
        {0x268113d2000ull, 0x26809290000ull, true},
        {0x268113f6000ull, 0x26809290000ull, true},
        {0x26811447000ull, 0x26809290000ull, true},
        {0x26811594000ull, 0x26809290000ull, true},
        {0x268117c7000ull, 0x26809290000ull, true},
        {0x268117cf000ull, 0x26809290000ull, true},
        {0x26811894000ull, 0x26809290000ull, true},
        {0x26811a67000ull, 0x26809290000ull, true},
        {0x26811a68000ull, 0x26809290000ull, true},
        {0x26811a76000ull, 0x26809290000ull, true},
        {0x26811a7c000ull, 0x26809290000ull, true},
        {0x26811a7f000ull, 0x26809290000ull, true},
        {0x26812b07000ull, 0x26809290000ull, true},
        {0x26812b08000ull, 0x26809290000ull, true},
        {0x26812b17000ull, 0x26809290000ull, true},
        {0x26812b1f000ull, 0x26809290000ull, true},
        {0x26816763000ull, 0x26809290000ull, true},
        {0x26816769000ull, 0x26809290000ull, true},
        {0x2681676d000ull, 0x26809290000ull, true},
        {0x268186a8000ull, 0x26809290000ull, true},
        {0x268186c5000ull, 0x26809290000ull, true},
        {0x268186d8000ull, 0x26809290000ull, true},
        {0x26818a92000ull, 0x26809290000ull, true},
        {0x26818e15000ull, 0x26809290000ull, true},
        {0x2681c317000ull, 0x26809290000ull, true},
        {0x2681c318000ull, 0x26809290000ull, true},
        {0x2681c31d000ull, 0x26809290000ull, true},
        {0x2681c326000ull, 0x26809290000ull, true},
        {0x2681cd18000ull, 0x26809290000ull, true},
        {0x2681cd2c000ull, 0x26809290000ull, true},
        {0x2681cd2f000ull, 0x26809290000ull, true},
        {0x2681dfb4000ull, 0x26809290000ull, true},
        {0x2681e3c1000ull, 0x26809290000ull, true},
        {0x2684136a000ull, 0x26809290000ull, true},
        {0x2684136b000ull, 0x26809290000ull, true},
        {0x2684138a000ull, 0x26809290000ull, true},
        {0x2684138f000ull, 0x26809290000ull, true},
        {0x26842197000ull, 0x26809290000ull, true},
        {0x26842198000ull, 0x26809290000ull, true},
        {0x2684219d000ull, 0x26809290000ull, true},
        {0x268421a6000ull, 0x26809290000ull, true},
        {0x268421df000ull, 0x26809290000ull, true},
        {0x2684285d000ull, 0x26809290000ull, true},
        {0x26842870000ull, 0x26809290000ull, true},
        {0x26842d77000ull, 0x26809290000ull, true},
        {0x26842d78000ull, 0x26809290000ull, true},
        {0x26842d7d000ull, 0x26809290000ull, true},
        {0x26842d86000ull, 0x26809290000ull, true},
        {0x26842ef7000ull, 0x26809290000ull, true},
        {0x2684305f000ull, 0x26809290000ull, true},
        {0x268430bf000ull, 0x26809290000ull, true}
    };

    constexpr std::size_t SourceCandidateCount = std::size(SourceCandidates);
    std::vector<Candidate> g_candidates;

    struct Observation
    {
        enum class Status { Observed, NotObserved, Unmapped, LifetimeChanged } status{Status::NotObserved};
        std::uintptr_t allocationBase{};
        DWORD type{};
        DWORD protection{};
        DWORD error{};
        std::array<std::uint8_t, PageSize> bytes{};
    };

    struct PhaseSnapshot
    {
        std::vector<Observation> pages;
        DWORD foregroundPidAtBegin{};
        DWORD foregroundPidAtEnd{};
        bool complete{};
    };

    std::ofstream logFile;
    HANDLE targetProcess{};
    FILETIME targetCreation{};
    std::uint64_t diffRuns{};

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

    void Log(std::string_view text)
    {
        SYSTEMTIME now{};
        GetLocalTime(&now);
        if (logFile.is_open()) {
            logFile << '[' << std::setfill('0') << std::setw(4) << now.wYear << '-'
                << std::setw(2) << now.wMonth << '-' << std::setw(2) << now.wDay << 'T'
                << std::setw(2) << now.wHour << ':' << std::setw(2) << now.wMinute << ':'
                << std::setw(2) << now.wSecond << '.' << std::setw(3) << now.wMilliseconds
                << "] " << text << "\r\n";
            logFile.flush();
        }
        std::cout << text << std::endl;
    }

    DWORD ForegroundPid() noexcept
    {
        const HWND window = GetForegroundWindow();
        if (!window) return 0;
        DWORD pid{};
        GetWindowThreadProcessId(window, &pid);
        return pid;
    }

    bool ForegroundIsTarget() noexcept { return ForegroundPid() == TargetPid; }

    bool ReadableProtection(DWORD protect) noexcept
    {
        if ((protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0) return false;
        switch (protect & 0xff) {
        case PAGE_READONLY:
        case PAGE_READWRITE:
        case PAGE_WRITECOPY:
        case PAGE_EXECUTE_READ:
        case PAGE_EXECUTE_READWRITE:
        case PAGE_EXECUTE_WRITECOPY:
            return true;
        default: return false;
        }
    }

    const char* TypeName(DWORD type) noexcept
    {
        switch (type) {
        case MEM_PRIVATE: return "private";
        case MEM_MAPPED: return "mapped";
        case MEM_IMAGE: return "image";
        default: return "other";
        }
    }

    const char* StatusName(Observation::Status status) noexcept
    {
        switch (status) {
        case Observation::Status::Observed: return "OBSERVED";
        case Observation::Status::NotObserved: return "NOT_OBSERVED";
        case Observation::Status::Unmapped: return "UNMAPPED_INVALIDATED";
        case Observation::Status::LifetimeChanged: return "ALLOCATION_LIFETIME_CHANGED";
        }
        return "UNKNOWN";
    }

    Observation Observe(const Candidate& candidate)
    {
        Observation result;
        MEMORY_BASIC_INFORMATION info{};
        if (!VirtualQueryEx(targetProcess, reinterpret_cast<const void*>(candidate.address),
            &info, sizeof(info))) {
            result.error = GetLastError();
            return result;
        }
        result.allocationBase = reinterpret_cast<std::uintptr_t>(info.AllocationBase);
        result.type = info.Type;
        result.protection = info.Protect;
        if (info.State != MEM_COMMIT) {
            result.status = Observation::Status::Unmapped;
            return result;
        }
        if (result.allocationBase != candidate.expectedAllocation ||
            info.Type != MEM_PRIVATE) {
            result.status = Observation::Status::LifetimeChanged;
            return result;
        }
        if (reinterpret_cast<std::uintptr_t>(info.BaseAddress) > candidate.address ||
            candidate.address + PageSize >
                reinterpret_cast<std::uintptr_t>(info.BaseAddress) + info.RegionSize ||
            !ReadableProtection(info.Protect)) {
            result.status = Observation::Status::NotObserved;
            result.error = ERROR_NOACCESS;
            return result;
        }
        SIZE_T read{};
        if (!ReadProcessMemory(targetProcess, reinterpret_cast<const void*>(candidate.address),
            result.bytes.data(), PageSize, &read) || read != PageSize) {
            result.error = GetLastError();
            return result;
        }
        result.status = Observation::Status::Observed;
        return result;
    }

    PhaseSnapshot Capture(char phase, const char* locale)
    {
        PhaseSnapshot snapshot;
        snapshot.pages.reserve(g_candidates.size());
        snapshot.foregroundPidAtBegin = ForegroundPid();
        Log(std::string("MARKER phase=") + phase + " locale=" + locale +
            " target_pid=" + std::to_string(TargetPid) +
            " foreground_pid_at_begin=" + std::to_string(snapshot.foregroundPidAtBegin) +
            " status=begin");
        std::size_t observed{}, notObserved{}, unmapped{}, lifetimeChanged{};
        for (const auto& candidate : g_candidates) {
            Observation observation = Observe(candidate);
            switch (observation.status) {
            case Observation::Status::Observed: ++observed; break;
            case Observation::Status::NotObserved: ++notObserved; break;
            case Observation::Status::Unmapped: ++unmapped; break;
            case Observation::Status::LifetimeChanged: ++lifetimeChanged; break;
            }
            std::ostringstream row;
            row << "TARGET_PAGE phase=" << phase << " address=0x" << std::hex << candidate.address
                << " priority=" << (candidate.priorityCluster ? "68-page-cluster" : "other")
                << " expected_allocation=0x" << candidate.expectedAllocation
                << " status=" << StatusName(observation.status)
                << " observed_allocation=0x" << observation.allocationBase
                << " type=" << TypeName(observation.type)
                << " protection=0x" << observation.protection
                << " error=" << std::dec << observation.error
                << " bytes=" << (observation.status == Observation::Status::Observed ? PageSize : 0);
            Log(row.str());
            snapshot.pages.push_back(std::move(observation));
        }
        snapshot.foregroundPidAtEnd = ForegroundPid();
        snapshot.complete = snapshot.pages.size() == g_candidates.size();
        std::ostringstream summary;
        summary << "PHASE_SUMMARY phase=" << phase << " target_pid=" << TargetPid
            << " foreground_pid_at_begin=" << snapshot.foregroundPidAtBegin
            << " foreground_pid_at_end=" << snapshot.foregroundPidAtEnd
            << " planned=" << g_candidates.size()
            << " observed=" << observed << " not_observed=" << notObserved
            << " unmapped_invalidated=" << unmapped
            << " allocation_lifetime_changed=" << lifetimeChanged
            << " status=" << (snapshot.complete ? "COMPLETE" : "INCOMPLETE");
        Log(summary.str());
        return snapshot;
    }

    bool SamePage(const Observation& a, const Observation& b) noexcept
    {
        return std::equal(a.bytes.begin(), a.bytes.end(), b.bytes.begin());
    }

    void LogChangedRuns(const Candidate& candidate, const Observation& a,
        const Observation& b, const Observation& c)
    {
        std::size_t i{};
        while (i < PageSize && diffRuns < MaxDiffRuns) {
            while (i < PageSize && a.bytes[i] == b.bytes[i] && b.bytes[i] == c.bytes[i]) ++i;
            if (i == PageSize) break;
            const std::size_t begin = i;
            while (i < PageSize && (a.bytes[i] != b.bytes[i] || b.bytes[i] != c.bytes[i])) ++i;
            const std::size_t end = i;
            const std::size_t contextBegin = begin > ContextBytes ? begin - ContextBytes : 0;
            const std::size_t contextEnd = std::min(PageSize, end + ContextBytes);
            const auto context = [contextBegin, contextEnd](const Observation& observation) {
                return Hex(std::span(observation.bytes).subspan(contextBegin, contextEnd - contextBegin));
            };
            std::ostringstream row;
            row << "BYTE_CONTEXT address=0x" << std::hex << candidate.address
                << " offset=0x" << begin << " length=" << std::dec << (end - begin)
                << " context_offset=0x" << std::hex << contextBegin
                << " A={" << context(a) << "} B={" << context(b)
                << "} C={" << context(c) << '}';
            Log(row.str());
            ++diffRuns;
        }
    }

    bool AllObservedSameLifetime(const Observation& a, const Observation& b,
        const Observation& c) noexcept
    {
        return a.status == Observation::Status::Observed &&
            b.status == Observation::Status::Observed &&
            c.status == Observation::Status::Observed &&
            a.allocationBase == b.allocationBase && a.allocationBase == c.allocationBase &&
            a.type == b.type && a.type == c.type;
    }

    bool IsObservedAba(const Observation& a, const Observation& b,
        const Observation& c) noexcept
    {
        return AllObservedSameLifetime(a, b, c) && SamePage(a, c) && !SamePage(a, b);
    }

    void Analyze(const PhaseSnapshot& a, const PhaseSnapshot& b, const PhaseSnapshot& c)
    {
        std::size_t aba{}, observedOther{}, notObserved{}, unmapped{}, lifetimeChanged{};
        for (std::size_t i = 0; i < g_candidates.size(); ++i) {
            const Observation& oa = a.pages[i];
            const Observation& ob = b.pages[i];
            const Observation& oc = c.pages[i];
            if (AllObservedSameLifetime(oa, ob, oc)) {
                if (IsObservedAba(oa, ob, oc)) {
                    ++aba;
                    Log("CANDIDATE_RESULT class=OBSERVED_A_B_A address=0x" +
                        [&] { std::ostringstream s; s << std::hex << g_candidates[i].address; return s.str(); }() +
                        " priority=" + (g_candidates[i].priorityCluster ? "68-page-cluster" : "other"));
                } else {
                    ++observedOther;
                }
                LogChangedRuns(g_candidates[i], oa, ob, oc);
            } else if (oa.status == Observation::Status::Unmapped ||
                ob.status == Observation::Status::Unmapped || oc.status == Observation::Status::Unmapped) {
                ++unmapped;
            } else if (oa.status == Observation::Status::LifetimeChanged ||
                ob.status == Observation::Status::LifetimeChanged ||
                oc.status == Observation::Status::LifetimeChanged) {
                ++lifetimeChanged;
            } else {
                ++notObserved;
            }
        }
        std::ostringstream summary;
        summary << "ANALYSIS observed_A_B_A=" << aba << " observed_other_transition=" << observedOther
            << " NOT_OBSERVED=" << notObserved << " UNMAPPED_INVALIDATED=" << unmapped
            << " ALLOCATION_LIFETIME_CHANGED=" << lifetimeChanged
            << " byte_context_runs=" << diffRuns << " byte_context_cap=" << MaxDiffRuns
            << " comparison_rule=all-three-full-page-reads-and-same-allocation-identity";
        Log(summary.str());
    }

    bool ProcessIdentity(std::string& details)
    {
        HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE,
            FALSE, TargetPid);
        if (!process) {
            details = "OpenProcess failed win32=" + std::to_string(GetLastError());
            return false;
        }
        targetProcess = process;
        FILETIME exitTime{}, kernel{}, user{};
        if (!GetProcessTimes(process, &targetCreation, &exitTime, &kernel, &user)) {
            details = "GetProcessTimes failed win32=" + std::to_string(GetLastError());
            return false;
        }
        wchar_t imagePath[32768]{};
        DWORD pathLength = static_cast<DWORD>(std::size(imagePath));
        if (!QueryFullProcessImageNameW(process, 0, imagePath, &pathLength)) {
            details = "QueryFullProcessImageName failed win32=" + std::to_string(GetLastError());
            return false;
        }
        const std::wstring image(imagePath, pathLength);
        const std::wstring basename = std::filesystem::path(image).filename().wstring();
        if (_wcsicmp(basename.c_str(), L"Stalker2-Win64-Shipping.exe") != 0) {
            details = "wrong_target_image_rejected";
            return false;
        }
        std::ostringstream row;
        row << "PROCESS_IDENTITY pid=" << TargetPid << " image=";
        const int n = WideCharToMultiByte(CP_UTF8, 0, image.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string utf8(n > 0 ? static_cast<std::size_t>(n) : 0, '\0');
        if (n > 1) {
            WideCharToMultiByte(CP_UTF8, 0, image.c_str(), -1, utf8.data(), n, nullptr, nullptr);
            utf8.resize(static_cast<std::size_t>(n - 1));
        }
        row << utf8 << " creation_filetime=" << targetCreation.dwHighDateTime << ':'
            << targetCreation.dwLowDateTime << " access=query+vm_read only";
        details = row.str();
        return true;
    }

    bool ResolveCandidates()
    {
        std::map<std::uintptr_t, std::vector<Candidate>> profiles;
        for (const auto& source : SourceCandidates)
            profiles[source.expectedAllocation].push_back(source);

        SYSTEM_INFO system{};
        GetSystemInfo(&system);
        const std::uintptr_t pageSize = system.dwPageSize ? system.dwPageSize : PageSize;
        std::uintptr_t address = reinterpret_cast<std::uintptr_t>(system.lpMinimumApplicationAddress);
        const std::uintptr_t maxAddress = reinterpret_cast<std::uintptr_t>(system.lpMaximumApplicationAddress);
        std::set<std::uintptr_t> privateAllocationBases;
        std::uint64_t queriedRegions{};
        while (address < maxAddress) {
            MEMORY_BASIC_INFORMATION info{};
            if (!VirtualQueryEx(targetProcess, reinterpret_cast<const void*>(address), &info, sizeof(info))) {
                if (address > UINTPTR_MAX - pageSize) break;
                address += pageSize;
                continue;
            }
            ++queriedRegions;
            const auto base = reinterpret_cast<std::uintptr_t>(info.BaseAddress);
            const auto end = base + info.RegionSize;
            if (end <= address) break;
            if (info.State == MEM_COMMIT && info.Type == MEM_PRIVATE)
                privateAllocationBases.insert(reinterpret_cast<std::uintptr_t>(info.AllocationBase));
            address = end;
        }
        Log("RESOLUTION_METADATA_SCAN queried_regions=" + std::to_string(queriedRegions) +
            " private_allocation_bases=" + std::to_string(privateAllocationBases.size()) +
            " memory_bytes_read=0");

        bool primaryResolved{};
        std::size_t resolvedProfiles{};
        for (const auto& [sourceBase, sourcePages] : profiles) {
            if (sourcePages.size() < 2) continue;
            const bool primary = sourcePages.front().priorityCluster;
            std::vector<std::uintptr_t> matches;
            for (const auto candidateBase : privateAllocationBases) {
                bool profileMatches = true;
                for (const auto& source : sourcePages) {
                    const std::uintptr_t offset = source.address - sourceBase;
                    if (candidateBase > UINTPTR_MAX - offset) {
                        profileMatches = false;
                        break;
                    }
                    MEMORY_BASIC_INFORMATION info{};
                    if (!VirtualQueryEx(targetProcess,
                        reinterpret_cast<const void*>(candidateBase + offset), &info, sizeof(info)) ||
                        info.State != MEM_COMMIT || info.Type != MEM_PRIVATE ||
                        reinterpret_cast<std::uintptr_t>(info.AllocationBase) != candidateBase ||
                        !ReadableProtection(info.Protect)) {
                        profileMatches = false;
                        break;
                    }
                }
                if (profileMatches) matches.push_back(candidateBase);
            }

            std::ostringstream result;
            result << "RESOLUTION_PROFILE source_base=0x" << std::hex << sourceBase
                << " pages=" << std::dec << sourcePages.size()
                << " priority=" << (primary ? "68-page-cluster" : "secondary")
                << " unique_matches=" << matches.size();
            Log(result.str());
            if (matches.size() != 1) {
                if (primary) return false;
                continue;
            }

            const auto targetBase = matches.front();
            for (const auto& source : sourcePages) {
                const auto offset = source.address - sourceBase;
                g_candidates.push_back({targetBase + offset, targetBase, primary});
            }
            ++resolvedProfiles;
            if (primary) primaryResolved = true;
        }
        std::sort(g_candidates.begin(), g_candidates.end(),
            [](const Candidate& a, const Candidate& b) { return a.address < b.address; });
        const bool uniqueAddresses = std::adjacent_find(g_candidates.begin(), g_candidates.end(),
            [](const Candidate& a, const Candidate& b) { return a.address == b.address; }) == g_candidates.end();
        Log("RESOLUTION_SUMMARY source_pages=" + std::to_string(SourceCandidateCount) +
            " rebaseable_cluster_pages=75 resolved_pages=" + std::to_string(g_candidates.size()) +
            " resolved_profiles=" + std::to_string(resolvedProfiles) +
            " primary_cluster_unique=" + (primaryResolved ? "true" : "false") +
            " unique_target_addresses=" + (uniqueAddresses ? "true" : "false"));
        return primaryResolved && uniqueAddresses && g_candidates.size() >= 68;
    }

    bool WaitForMarker(int virtualKey)
    {
        bool previous{};
        for (;;) {
            if (WaitForSingleObject(targetProcess, 0) != WAIT_TIMEOUT) return false;
            const bool chord = ForegroundIsTarget() &&
                (GetAsyncKeyState(VK_CONTROL) & 0x8000) &&
                (GetAsyncKeyState(VK_MENU) & 0x8000) &&
                (GetAsyncKeyState(virtualKey) & 0x8000);
            if (chord && !previous) return true;
            previous = chord;
            Sleep(50);
        }
    }

    int SelfTest()
    {
        bool pass = SourceCandidateCount == 92;
        std::size_t cluster{};
        std::map<std::uintptr_t, std::size_t> profileSizes;
        for (std::size_t i = 0; i < SourceCandidateCount; ++i) {
            pass &= (SourceCandidates[i].address % PageSize) == 0;
            pass &= SourceCandidates[i].expectedAllocation != 0;
            ++profileSizes[SourceCandidates[i].expectedAllocation];
            if (SourceCandidates[i].priorityCluster) {
                ++cluster;
                pass &= SourceCandidates[i].expectedAllocation == 0x26809290000ull;
            }
            for (std::size_t j = i + 1; j < SourceCandidateCount; ++j)
                pass &= SourceCandidates[i].address != SourceCandidates[j].address;
        }
        std::size_t rebaseablePages{};
        for (const auto& [base, count] : profileSizes) {
            (void)base;
            if (count >= 2) rebaseablePages += count;
        }
        pass &= cluster == 68;
        pass &= rebaseablePages == 75;
        Observation read;
        read.status = Observation::Status::Observed;
        read.allocationBase = 0x1000;
        read.type = MEM_PRIVATE;
        Observation changed = read;
        changed.bytes[0] = 1;
        Observation returned = read;
        Observation missing;
        missing.status = Observation::Status::NotObserved;
        Observation unmapped;
        unmapped.status = Observation::Status::Unmapped;
        Observation otherLife;
        otherLife.status = Observation::Status::Observed;
        otherLife.allocationBase = 0x2000;
        otherLife.type = MEM_PRIVATE;
        pass &= IsObservedAba(read, changed, returned);
        pass &= !IsObservedAba(read, missing, returned);
        pass &= !IsObservedAba(read, unmapped, returned);
        pass &= !IsObservedAba(read, otherLife, returned);
        pass &= !AllObservedSameLifetime(read, missing, read);
        pass &= !AllObservedSameLifetime(read, unmapped, read);
        pass &= !AllObservedSameLifetime(read, otherLife, read);
        std::cout << "source_candidate_pages=" << SourceCandidateCount
            << " priority_cluster_pages=" << cluster
            << " repeatable_allocation_profile_pages=" << rebaseablePages
            << " unique_page_aligned=checked"
            << " exact_A_equals_C_and_A_differs_B=checked"
            << " partial_or_unmapped_or_lifetime_change_never_comparable=checked "
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
    if (argc == 2 && _wcsicmp(argv[1], L"--resolve-only") == 0) {
        std::cout << identity << std::endl;
        const bool resolved = ResolveCandidates();
        std::cout << "PREFLIGHT_COMPLETE metadata_only=true language_markers_captured=0" << std::endl;
        CloseHandle(targetProcess);
        return resolved ? 0 : 4;
    }
    wchar_t localBuffer[MAX_PATH]{};
    const DWORD localLength = GetEnvironmentVariableW(L"LOCALAPPDATA", localBuffer, MAX_PATH);
    if (localLength == 0 || localLength >= MAX_PATH) {
        std::cerr << "LOCALAPPDATA unavailable" << std::endl;
        return 2;
    }
    std::error_code ec;
    const auto directory = std::filesystem::path(localBuffer) / L"STALKER2LanguageStateWatcher";
    std::filesystem::create_directories(directory, ec);
    if (ec) {
        std::cerr << "log_directory_error=" << ec.message() << std::endl;
        return 2;
    }
    SYSTEMTIME now{};
    GetLocalTime(&now);
    std::wostringstream filename;
    filename << L"Targeted-" << now.wYear << std::setfill(L'0') << std::setw(2) << now.wMonth
        << std::setw(2) << now.wDay << L'-' << std::setw(2) << now.wHour
        << std::setw(2) << now.wMinute << std::setw(2) << now.wSecond << L".log";
    logFile.open(directory / filename.str(), std::ios::out | std::ios::trunc);
    if (!logFile) {
        std::cerr << "log_open_failed" << std::endl;
        return 2;
    }
    Log("RUN_START");
    Log(identity);
    Log("OBSERVATION_BOUNDARY external ReadProcessMemory only; no process writes, hooks, page guards, executable-page callsite tracing, or UE APIs");
    Log("TARGET_SET source=prior fully observed A/B/A pages count=92; rebase only repeated allocation profiles; singleton profiles excluded");
    if (!ResolveCandidates()) {
        Log("RUN_ABORT reason=primary_candidate_allocation_not_uniquely_resolved; no language markers captured");
        CloseHandle(targetProcess);
        return 4;
    }
    if (argc == 2 && _wcsicmp(argv[1], L"--resolve-only") == 0) {
        Log("PREFLIGHT_COMPLETE metadata_only=true language_markers_captured=0");
        CloseHandle(targetProcess);
        return 0;
    }
    Log("CONTROL Ctrl+Alt+F6=A(Ukrainian stable), Ctrl+Alt+F7=B(English applied/stable), Ctrl+Alt+F8=C(Ukrainian applied/stable)");
    Log("SAFETY process handle rights=PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_VM_READ|SYNCHRONIZE");
    Log("WATCHER_READY wait_for_markers=true focus_must_remain_on_target_game");
    const PhaseSnapshot a = [&] {
        while (!WaitForMarker(VK_F6)) {
            Log("RUN_ABORT reason=target_process_exited_before_A");
            CloseHandle(targetProcess);
            return PhaseSnapshot{};
        }
        return Capture('A', "uk");
    }();
    if (a.pages.empty()) return 3;
    const PhaseSnapshot b = [&] {
        while (!WaitForMarker(VK_F7)) {
            Log("RUN_ABORT reason=target_process_exited_before_B");
            CloseHandle(targetProcess);
            return PhaseSnapshot{};
        }
        return Capture('B', "en");
    }();
    if (b.pages.empty()) return 3;
    const PhaseSnapshot c = [&] {
        while (!WaitForMarker(VK_F8)) {
            Log("RUN_ABORT reason=target_process_exited_before_C");
            CloseHandle(targetProcess);
            return PhaseSnapshot{};
        }
        return Capture('C', "uk");
    }();
    if (c.pages.empty()) return 3;
    Analyze(a, b, c);
    Log("WATCHER_COMPLETE phases=A(uk),B(en),C(uk); no game launch or process modification");
    Log("RUN_STOP");
    CloseHandle(targetProcess);
    return 0;
}
