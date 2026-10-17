#pragma once

#include <Windows.h>

namespace platform::win32
{
    // Call from the initialization worker, before publishing callbacks. A
    // resident owner/VEH must not outlive the code containing its callbacks.
    inline bool PinModuleForProcessLifetime(HMODULE module) noexcept
    {
        if (!module) return false;
        HMODULE pinned{};
        return GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_PIN, reinterpret_cast<LPCWSTR>(module),
            &pinned) != FALSE && pinned == module;
    }
}
