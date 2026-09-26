#pragma once

#include <Windows.h>

namespace overlay
{
    constexpr bool ShouldSynchronizeAutoLanguage(bool overlayVisible,
        bool autoLocaleConfigured) noexcept
    {
        return overlayVisible && autoLocaleConfigured;
    }

    // Synchronizes Auto mode when the overlay changes from closed to open.
    void SynchronizeAutoLanguageOnOverlayOpen(bool overlayVisible) noexcept;
    void SynchronizeAutoLanguageIfConfigured() noexcept;
    bool RequestAutoLanguageSynchronization(HWND window) noexcept;
    inline constexpr UINT AutoLanguageSyncMessage = WM_APP + 0x4D1;
}
