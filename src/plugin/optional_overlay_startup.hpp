#pragma once

#include <utility>

namespace plugin
{
template <typename OverlayStartup, typename CoreInitialization,
    typename CoreReady, typename OverlayFailure>
auto InitializeOptionalOverlayThenCore(OverlayStartup&& startOverlay,
    CoreInitialization&& initializeCore, CoreReady&& markCoreReady,
    OverlayFailure&& onOverlayFailure)
        -> decltype(std::forward<CoreInitialization>(initializeCore)())
    {
        bool overlayStarted = false;
        try {
            overlayStarted = std::forward<OverlayStartup>(startOverlay)();
        } catch (...) {
            overlayStarted = false;
        }
        auto result = std::forward<CoreInitialization>(initializeCore)();
        if (!overlayStarted) {
            try {
                std::forward<OverlayFailure>(onOverlayFailure)();
            } catch (...) {
                // Optional UI startup cannot escape into the core startup thread.
            }
        }
        try {
            std::forward<CoreReady>(markCoreReady)();
        } catch (...) {
            try {
                std::forward<OverlayFailure>(onOverlayFailure)();
            } catch (...) {
                // Core initialization is already complete; readiness-reporting
                // failure remains local to the optional Overlay transition.
            }
        }
        return result;
    }
}
