#pragma once

#include <Windows.h>

#include <cstddef>

namespace overlay
{
    constexpr bool ShouldSynchronizeAutoLanguage(bool overlayVisible,
        bool autoLocaleConfigured) noexcept
    {
        return overlayVisible && autoLocaleConfigured;
    }

    // Synchronizes Auto mode when the overlay changes from closed to open.
    void SynchronizeAutoLanguageOnOverlayOpen(HWND window,
        bool overlayVisible) noexcept;
    bool ReadGameLanguageLocale(char* output, std::size_t capacity) noexcept;
    void ApplyDetectedGameLanguageLocale(const char* localeCode,
        bool readSucceeded) noexcept;
    bool RequestAutoLanguageSynchronization(HWND window) noexcept;
    bool RequestStartupGameDataProbe(HWND window) noexcept;
    inline constexpr UINT AutoLanguageSyncMessage = WM_APP + 0x4D1;
    inline constexpr UINT StartupGameDataProbeMessage = WM_APP + 0x4D2;
}
