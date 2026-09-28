#pragma once

#include <Windows.h>

namespace overlay
{
    inline constexpr DWORD GpuFenceWaitTimeoutMs = 1000;

    constexpr bool GpuFenceWaitCompleted(DWORD result) noexcept
    {
        return result == WAIT_OBJECT_0;
    }
}
