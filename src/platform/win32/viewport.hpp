#pragma once

namespace platform::win32
{
    bool IsUsableClientViewport(long width, long height) noexcept;
    float ReadCurrentProcessClientViewportAspect();
    float ReadClientViewportAspect();
}
