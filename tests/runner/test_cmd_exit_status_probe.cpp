#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cwchar>
#include <string>
#include <windows.h>

namespace {
std::wstring QuoteArgument(const std::wstring& argument) {
    std::wstring result = L"\"";
    std::size_t slashes = 0;
    for (wchar_t value : argument) {
        if (value == L'\\') { ++slashes; continue; }
        if (value == L'\"') {
            result.append(slashes * 2 + 1, L'\\');
            result.push_back(value);
            slashes = 0;
            continue;
        }
        result.append(slashes, L'\\');
        result.push_back(value);
        slashes = 0;
    }
    result.append(slashes * 2, L'\\');
    result.push_back(L'\"');
    return result;
}

bool WritePidMarker(const wchar_t* path) {
    HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    const std::string value = std::to_string(GetCurrentProcessId());
    DWORD written = 0;
    const BOOL writeOk = WriteFile(file, value.data(), static_cast<DWORD>(value.size()), &written, nullptr);
    const BOOL closeOk = CloseHandle(file);
    return writeOk && written == value.size() && closeOk;
}

bool StartFixtureChild(const wchar_t* mode, const wchar_t* firstMarker, const wchar_t* secondMarker) {
    wchar_t executable[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, executable, MAX_PATH);
    if (!length || length >= MAX_PATH) return false;
    std::wstring command = QuoteArgument(executable) + L" " + mode + L" " + QuoteArgument(firstMarker);
    if (secondMarker != nullptr) command += L" " + QuoteArgument(secondMarker);
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(executable, command.data(), nullptr, nullptr, FALSE,
            CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process)) return false;
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return true;
}

int RunJobCloseFixture(const wchar_t* pidMarker) {
    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    if (job == nullptr) return 71;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation,
            &limits, sizeof(limits))) {
        CloseHandle(job);
        return 72;
    }

    wchar_t executable[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, executable, MAX_PATH);
    if (!length || length >= MAX_PATH) { CloseHandle(job); return 73; }
    std::wstring command = QuoteArgument(executable) + L" --sleep 120000";
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION child{};
    if (!CreateProcessW(executable, command.data(), nullptr, nullptr, FALSE,
            CREATE_SUSPENDED | CREATE_NO_WINDOW, nullptr, nullptr, &startup, &child)) {
        CloseHandle(job);
        return 74;
    }
    if (!AssignProcessToJobObject(job, child.hProcess) || !WritePidMarker(pidMarker)) {
        TerminateProcess(child.hProcess, 75);
        WaitForSingleObject(child.hProcess, 5000);
        CloseHandle(child.hThread);
        CloseHandle(child.hProcess);
        CloseHandle(job);
        return 75;
    }
    if (ResumeThread(child.hThread) == static_cast<DWORD>(-1)) {
        TerminateJobObject(job, 76);
        CloseHandle(child.hThread);
        CloseHandle(child.hProcess);
        CloseHandle(job);
        return 76;
    }
    CloseHandle(child.hThread);
    CloseHandle(job);
    const DWORD wait = WaitForSingleObject(child.hProcess, 5000);
    CloseHandle(child.hProcess);
    return wait == WAIT_OBJECT_0 ? 0 : 77;
}

int RunDescendantFixture(int argc, wchar_t** argv) {
    if (argc == 5 && std::wcscmp(argv[1], L"--spawn-descendants") == 0) {
        if (!WritePidMarker(argv[2]) || !StartFixtureChild(L"--spawn-middle", argv[3], argv[4])) return 67;
        Sleep(120000);
        return 0;
    }
    if (argc == 5 && std::wcscmp(argv[1], L"--spawn-orphans") == 0) {
        if (!WritePidMarker(argv[2]) || !StartFixtureChild(L"--spawn-middle", argv[3], argv[4])) return 67;
        return 0;
    }
    if (argc == 4 && std::wcscmp(argv[1], L"--spawn-middle") == 0) {
        if (!WritePidMarker(argv[2]) || !StartFixtureChild(L"--spawn-leaf", argv[3], nullptr)) return 68;
        Sleep(120000);
        return 0;
    }
    if (argc == 3 && std::wcscmp(argv[1], L"--spawn-leaf") == 0) {
        if (!WritePidMarker(argv[2])) return 69;
        Sleep(120000);
        return 0;
    }
    if (argc == 3 && std::wcscmp(argv[1], L"--job-close-descendant") == 0) {
        return RunJobCloseFixture(argv[2]);
    }
    return -1;
}
}

int wmain(int argc, wchar_t** argv) {
    if (argc >= 2 && std::wcscmp(argv[1], L"--crash") == 0) {
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
        RaiseException(EXCEPTION_ACCESS_VIOLATION, EXCEPTION_NONCONTINUABLE, 0, nullptr);
        return 66;
    }
    if (argc == 3 && std::wcscmp(argv[1], L"--sleep") == 0) {
        const long milliseconds = std::wcstol(argv[2], nullptr, 10);
        if (milliseconds < 0 || milliseconds > 120000) return 65;
        Sleep(static_cast<DWORD>(milliseconds));
        return 0;
    }
    const int descendantResult = RunDescendantFixture(argc, argv);
    if (descendantResult >= 0) return descendantResult;
    if (argc != 2) return 64;

    errno = 0;
    wchar_t* end = nullptr;
    const long value = std::wcstol(argv[1], &end, 10);
    if (errno != 0 || end == argv[1] || *end != L'\0' || value < INT_MIN || value > INT_MAX) {
        return 65;
    }
    return static_cast<int>(value);
}
