#pragma once

#include <Windows.h>
#include <atomic>

namespace overlay
{
    // Run on the HWND thread: no native message can enter our new procedure
    // between replacement and publication of the actual returned predecessor.
    // The caller keeps record/property storage resident for foreign chains.
    inline bool InstallGameThreadSubclass(HWND window, const wchar_t* property,
        HANDLE record, WNDPROC procedure, std::atomic<WNDPROC>& predecessor) noexcept
    {
        if (!window || GetWindowThreadProcessId(window, nullptr) != GetCurrentThreadId() ||
            GetPropW(window, property) || !SetPropW(window, property, record))
            return false;
        SetLastError(0);
        const auto original = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
            window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(procedure)));
        if (!original) {
            RemovePropW(window, property);
            return false;
        }
        predecessor.store(original, std::memory_order_release);
        return true;
    }
}
