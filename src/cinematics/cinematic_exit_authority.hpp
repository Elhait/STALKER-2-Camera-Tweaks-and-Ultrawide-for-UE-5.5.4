#pragma once

#include "../camera/presentation_state.hpp"

#include <utility>

namespace cinematics
{
    // EXIT hook invocation alone is not lifecycle authority. The current
    // callback contract exposes no native generation token, so acceptance is
    // limited to an active coordinator with a committed ENTER selection.
    constexpr bool IsAcceptedCinematicExit(
        camera::CoordinatorState coordinator, bool selectionValid) noexcept
    {
        return coordinator == camera::CoordinatorState::CinematicActive &&
            selectionValid;
    }

    template<typename ObserveRejected, typename ApplyAccepted>
    bool DispatchCinematicExit(camera::CoordinatorState coordinator,
        bool selectionValid, ObserveRejected&& observeRejected,
        ApplyAccepted&& applyAccepted)
    {
        if (!IsAcceptedCinematicExit(coordinator, selectionValid)) {
            std::forward<ObserveRejected>(observeRejected)();
            return false;
        }

        std::forward<ApplyAccepted>(applyAccepted)();
        return true;
    }
}
