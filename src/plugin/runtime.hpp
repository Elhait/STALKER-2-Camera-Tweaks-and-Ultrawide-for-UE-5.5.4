#pragma once

#include <Windows.h>

#include "worker_lifecycle.hpp"
#include "feature_status.hpp"
#include "runtime_settings.hpp"

#include <string_view>

namespace plugin
{
    void SetModuleHandle(HMODULE module);
    DWORD WINAPI InitializeThread(void* parameter);
    void Shutdown();
    void NotifyProcessDetach(bool processTerminating);
    RuntimeSettingsApi& GetRuntimeSettingsApi() noexcept;
    RuntimeSettingsSnapshot GetRuntimeSettingsSnapshot() noexcept;
    bool GetOverlaySemanticSnapshot(OverlaySemanticSnapshot& snapshot) noexcept;
    float GetRuntimeViewportAspect() noexcept;
    bool SetDetectedOverlayLocale(std::string_view localeCode) noexcept;
}
