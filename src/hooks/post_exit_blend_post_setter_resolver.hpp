#pragma once

#include "instruction_validator.hpp"

#include <cstring>

namespace hooks
{
    struct PostExitBlendPostSetterResolution
    {
        std::uint8_t* hook{};
        std::size_t matches{};
        const char* reason{"invalid-span"};
        explicit operator bool() const noexcept { return hook != nullptr; }
    };

    inline bool ValidatePostExitBlendPostSetterCall(
        std::uint8_t* call, const ExecutableSpan& span) noexcept
    {
        constexpr auto& expected = signatures::PostExitBlendSetterReturn;
        constexpr auto expectedSize = sizeof(expected);
        if (!span.Contains(call, expectedSize) ||
            std::memcmp(call, expected, expectedSize) != 0)
            return false;

        ZydisDecodedInstruction instruction{};
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT]{};
        if (!validation::DecodeInstruction(call, instruction, operands, span) ||
            instruction.mnemonic != ZYDIS_MNEMONIC_CALL || instruction.length != 6 ||
            instruction.operand_count_visible != 1 ||
            operands[0].type != ZYDIS_OPERAND_TYPE_MEMORY ||
            operands[0].mem.base != ZYDIS_REGISTER_RAX ||
            operands[0].mem.index != ZYDIS_REGISTER_NONE ||
            !operands[0].mem.disp.has_displacement ||
            operands[0].mem.disp.value != 0x608)
            return false;

        if (!validation::DecodeInstruction(call + 6, instruction, operands, span) ||
            instruction.mnemonic != ZYDIS_MNEMONIC_UCOMISS || instruction.length != 4 ||
            instruction.operand_count_visible != 2 ||
            operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER ||
            operands[0].reg.value != ZYDIS_REGISTER_XMM6 ||
            operands[1].type != ZYDIS_OPERAND_TYPE_MEMORY ||
            operands[1].mem.base != ZYDIS_REGISTER_RSI ||
            operands[1].mem.index != ZYDIS_REGISTER_NONE ||
            !operands[1].mem.disp.has_displacement ||
            operands[1].mem.disp.value != 0x2C)
            return false;

        return true;
    }

    inline PostExitBlendPostSetterResolution ResolvePostExitBlendPostSetter(
        const ExecutableSpan& span) noexcept
    {
        constexpr auto& signature = signatures::PostExitBlendSetterReturn;
        PostExitBlendPostSetterResolution result{};
        constexpr auto signatureSize = sizeof(signature);
        if (!span.begin || signatureSize > span.size) return result;
        std::uint8_t* candidate{};
        for (std::size_t offset = 0; offset <= span.size - signatureSize; ++offset) {
            auto* at = span.begin + offset;
            if (std::memcmp(at, signature, signatureSize) != 0) continue;
            ++result.matches;
            candidate = at;
        }
        if (result.matches != 1) {
            result.reason = result.matches ? "ambiguous-signature" : "no-signature";
            return result;
        }
        if (!ValidatePostExitBlendPostSetterCall(candidate, span)) {
            result.reason = "instruction-contract";
            return result;
        }
        result.hook = candidate + 6;
        result.reason = "validated";
        return result;
    }
}
