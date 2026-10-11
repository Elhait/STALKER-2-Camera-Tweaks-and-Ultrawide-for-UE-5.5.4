#pragma once

#include <cstdint>

namespace diagnostics::language_state
{
    struct PageFingerprint
    {
        std::uintptr_t address{};
        std::uintptr_t allocationBase{};
        std::uint64_t hash{};
        std::uint32_t memoryType{};
        std::uint32_t protection{};
    };

    enum class TransitionKind : std::uint8_t
    {
        StableAddressReturned,
        AllocationAppearedOnlyInB,
        AllocationMissingOnlyInB
    };

    // Classifies address-stable and allocation-lifecycle A -> B -> A evidence.
    // It deliberately does not infer what a page represents or who wrote it.
    bool IsReturnedPage(const PageFingerprint& phaseA,
        const PageFingerprint& phaseB, const PageFingerprint& phaseC) noexcept;

    bool IsBOnlyAllocation(const PageFingerprint* phaseA,
        const PageFingerprint& phaseB, const PageFingerprint* phaseC) noexcept;

    bool IsMissingOnlyInB(const PageFingerprint& phaseA,
        const PageFingerprint* phaseB, const PageFingerprint& phaseC) noexcept;
}
