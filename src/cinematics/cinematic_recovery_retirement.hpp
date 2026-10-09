#pragma once

#include "cinematic_fov.hpp"

#include <cstdint>
#include <utility>

namespace cinematics
{
    // A completed owner may remain as metadata while the independent Dialogue
    // exclusion finishes, but it no longer gates Gameplay writer dispatch.
    constexpr bool ShouldRejectPostRecoveryForeignWriter(
        const CinematicRecoveryOwnerSnapshot& owner, bool exclusionActive,
        std::uintptr_t source) noexcept
    {
        return owner.HasRecoveryAuthority() && exclusionActive &&
            (source == 0 || source != owner.writerRsi);
    }

    // Retire only recovery tied to the previously observed receiver instance.
    // RBX and identityEpoch are intentionally metadata, not authorization.
    template<typename Retire>
    bool RetireRecoveryForWriterRsiChange(
        camera::CoordinatorState coordinator,
        const CinematicRecoveryOwnerSnapshot& owner,
        const SharedWriterPairTransition& transition,
        Retire&& retire)
    {
        if (coordinator != camera::CoordinatorState::CinematicExiting ||
            !owner.valid || owner.recoveryComplete || !transition.changed ||
            transition.previous.source == 0 ||
            transition.previous.source != owner.writerRsi ||
            transition.current.source == 0 ||
            transition.current.source == owner.writerRsi)
            return false;

        std::forward<Retire>(retire)();
        return true;
    }
}
