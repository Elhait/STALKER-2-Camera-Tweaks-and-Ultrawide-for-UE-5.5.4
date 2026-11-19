#pragma once

#include "../cinematics/cinematic_fov.hpp"

namespace dialogue
{
    // A completed recovery owner no longer controls camera recovery, but while
    // the post-cinematic exclusion is still active it can identify the exact
    // receiver/generation for a provider handoff that arrives just afterward.
    inline bool IsPostExitProviderOwnerAuthorized(
        camera::CoordinatorState coordinator, bool exclusionActive,
        const cinematics::CinematicRecoveryOwnerSnapshot& owner,
        std::uint64_t generation, std::uintptr_t receiver) noexcept
    {
        if (!exclusionActive || !owner.valid || generation == 0 || receiver == 0 ||
            owner.generation != generation || owner.writerRsi != receiver)
            return false;

        if (coordinator == camera::CoordinatorState::CinematicExiting)
            return !owner.recoveryComplete;
        if (coordinator == camera::CoordinatorState::Gameplay)
            return owner.recoveryComplete;
        return false;
    }
}
