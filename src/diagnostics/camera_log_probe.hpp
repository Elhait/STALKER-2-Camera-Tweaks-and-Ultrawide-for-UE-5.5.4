#pragma once

#include <atomic>
#include <cstdint>

namespace diagnostics
{
    // One-shot correlation state for rare cinematic log probes. Diagnostic OFF
    // does not publish or consume this state.
    inline std::uint64_t ArmCinematicExitWriterProbe(
        std::atomic<std::uint64_t>& pendingGeneration,
        bool diagnosticMode, std::uint64_t generation) noexcept
    {
        if (!diagnosticMode || generation == 0) return 0;
        return pendingGeneration.exchange(generation, std::memory_order_acq_rel);
    }

    inline std::uint64_t TakeCinematicExitWriterProbe(
        std::atomic<std::uint64_t>& pendingGeneration,
        bool diagnosticMode) noexcept
    {
        if (!diagnosticMode) return 0;
        return pendingGeneration.exchange(0, std::memory_order_acq_rel);
    }
}
