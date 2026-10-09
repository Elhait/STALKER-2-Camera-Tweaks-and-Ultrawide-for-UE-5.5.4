#pragma once

#include "../config/feature_config.hpp"

#include <cstdint>
#include <utility>

namespace cinematics
{
    struct CinematicSelectionSnapshot
    {
        config::CinematicAspectPolicy aspectPolicy{};
        config::CinematicFovMode fovMode{};
        std::uint64_t generation{};
        bool valid{};
    };

    CinematicSelectionSnapshot CaptureCinematicSelection(
        config::CinematicAspectPolicy aspectPolicy,
        config::CinematicFovMode fovMode,
        std::uint64_t generation) noexcept;

    bool TryCaptureCinematicSelection(
        bool activeSelectionValid,
        config::CinematicAspectPolicy aspectPolicy,
        config::CinematicFovMode fovMode,
        std::uint64_t generation,
        CinematicSelectionSnapshot& snapshot) noexcept;

    template <typename CommitAcceptedEnter>
    bool TryCaptureCinematicSelectionTransactional(
        bool activeSelectionValid,
        config::CinematicAspectPolicy aspectPolicy,
        config::CinematicFovMode fovMode,
        std::uint64_t generation,
        CinematicSelectionSnapshot& snapshot,
        CommitAcceptedEnter&& commitAcceptedEnter)
    {
        CinematicSelectionSnapshot candidate{};
        if (!TryCaptureCinematicSelection(activeSelectionValid, aspectPolicy,
                fovMode, generation, candidate))
            return false;

        std::forward<CommitAcceptedEnter>(commitAcceptedEnter)(candidate);
        snapshot = candidate;
        return true;
    }
}
