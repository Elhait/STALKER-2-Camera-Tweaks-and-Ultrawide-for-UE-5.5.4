#include "language_state_transition.hpp"

namespace diagnostics::language_state
{
    namespace
    {
        bool SameAllocation(const PageFingerprint& left,
            const PageFingerprint& right) noexcept
        {
            return left.address == right.address &&
                left.allocationBase == right.allocationBase &&
                left.memoryType == right.memoryType;
        }
    }

    bool IsReturnedPage(const PageFingerprint& phaseA,
        const PageFingerprint& phaseB, const PageFingerprint& phaseC) noexcept
    {
        return SameAllocation(phaseA, phaseB) &&
            SameAllocation(phaseA, phaseC) &&
            phaseA.hash != phaseB.hash &&
            phaseA.hash == phaseC.hash;
    }

    bool IsBOnlyAllocation(const PageFingerprint* phaseA,
        const PageFingerprint& phaseB, const PageFingerprint* phaseC) noexcept
    {
        return phaseA == nullptr && phaseC == nullptr && phaseB.address != 0;
    }

    bool IsMissingOnlyInB(const PageFingerprint& phaseA,
        const PageFingerprint* phaseB, const PageFingerprint& phaseC) noexcept
    {
        return phaseB == nullptr && SameAllocation(phaseA, phaseC) &&
            phaseA.hash == phaseC.hash;
    }
}
