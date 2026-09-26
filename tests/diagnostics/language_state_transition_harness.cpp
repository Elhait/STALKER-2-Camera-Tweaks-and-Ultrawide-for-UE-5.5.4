#include "../../src/diagnostics/language_state_transition.hpp"

#include <cassert>
#include <cstdint>

using diagnostics::language_state::IsBOnlyAllocation;
using diagnostics::language_state::IsMissingOnlyInB;
using diagnostics::language_state::IsReturnedPage;
using diagnostics::language_state::PageFingerprint;

int main()
{
    constexpr std::uint32_t privateMemory = 0x20000;
    constexpr std::uint32_t readWrite = 0x04;
    const PageFingerprint a{0x1000, 0x1000, 10, privateMemory, readWrite};
    const PageFingerprint b{0x1000, 0x1000, 20, privateMemory, readWrite};
    const PageFingerprint c{0x1000, 0x1000, 10, privateMemory, readWrite};
    assert(IsReturnedPage(a, b, c));

    const PageFingerprint sameB{0x1000, 0x1000, 10, privateMemory, readWrite};
    assert(!IsReturnedPage(a, sameB, c));
    const PageFingerprint wrongC{0x1000, 0x1000, 21, privateMemory, readWrite};
    assert(!IsReturnedPage(a, b, wrongC));
    const PageFingerprint remappedB{0x1000, 0x9000, 20, privateMemory, readWrite};
    assert(!IsReturnedPage(a, remappedB, c));

    const PageFingerprint newB{0x3000, 0x3000, 31, privateMemory, readWrite};
    assert(IsBOnlyAllocation(nullptr, newB, nullptr));
    assert(!IsBOnlyAllocation(&a, newB, nullptr));
    assert(IsMissingOnlyInB(a, nullptr, c));
    assert(!IsMissingOnlyInB(a, &b, c));
}
