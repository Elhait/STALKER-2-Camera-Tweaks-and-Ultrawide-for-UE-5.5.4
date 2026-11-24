#pragma once

#include <cstddef>
#include <cstdint>

#include <Zydis.h>

#include "signature_scanner.hpp"

namespace hooks::validation
{
    inline bool PatchSpansOverlap(std::uintptr_t first, std::size_t firstSize,
        std::uintptr_t second, std::size_t secondSize) noexcept
    {
        if (!firstSize || !secondSize) return false;
        return first <= second ? second - first < firstSize : first - second < secondSize;
    }

    bool IsExecutable(std::uintptr_t address);
    bool DecodeInstruction(std::uint8_t* address, ZydisDecodedInstruction& instruction,
        ZydisDecodedOperand* operands, const ExecutableSpan& span);
    bool IsCallRel32(std::uint8_t* address, const ExecutableSpan& span);
    bool ContainsBytes(std::uint8_t* start, std::size_t length,
        const std::uint8_t* bytes, std::size_t byteCount, const ExecutableSpan& span);
    std::uint8_t* ResolveRel32CallTarget(std::uint8_t* address, const ExecutableSpan& span);
}
