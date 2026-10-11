#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <windows.h>

int main(int argc, char** argv) {
    if (argc >= 2 && std::strcmp(argv[1], "--crash") == 0) {
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
        RaiseException(EXCEPTION_ACCESS_VIOLATION, EXCEPTION_NONCONTINUABLE, 0, nullptr);
        return 66;
    }
    if (argc == 3 && std::strcmp(argv[1], "--sleep") == 0) {
        const long milliseconds = std::strtol(argv[2], nullptr, 10);
        if (milliseconds < 0 || milliseconds > 120000) return 65;
        Sleep(static_cast<DWORD>(milliseconds));
        return 0;
    }
    if (argc != 2) return 64;

    errno = 0;
    char* end = nullptr;
    const long value = std::strtol(argv[1], &end, 10);
    if (errno != 0 || end == argv[1] || *end != '\0' || value < INT_MIN || value > INT_MAX) {
        return 65;
    }
    return static_cast<int>(value);
}
