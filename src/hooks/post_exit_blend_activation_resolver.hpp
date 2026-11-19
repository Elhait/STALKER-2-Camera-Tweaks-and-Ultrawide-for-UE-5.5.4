#pragma once

#include "instruction_validator.hpp"
#include "signatures/signature_definitions.hpp"

#include <cstring>

namespace hooks
{
    struct PostExitBlendActivationResolution
    {
        std::uint8_t* hook{};
        std::size_t matches{};
        const char* reason{"invalid-span"};
        explicit operator bool() const noexcept { return hook != nullptr; }
    };

    inline bool ValidatePostExitBlendStartCapture(
        std::uint8_t* sequence, const ExecutableSpan& span) noexcept
    {
        constexpr auto sequenceSize = sizeof(signatures::PostExitBlendStartCapture);
        if (!span.Contains(sequence, sequenceSize)) return false;

        ZydisDecodedInstruction instruction{};
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT]{};
        const auto decoded = [&](std::size_t offset, ZydisMnemonic mnemonic,
            std::uint8_t length) {
            return validation::DecodeInstruction(sequence + offset, instruction,
                operands, span) && instruction.mnemonic == mnemonic &&
                instruction.length == length;
        };
        const auto displacementIs = [&](const ZydisDecodedOperand& operand,
            ZydisRegister base, std::int64_t displacement) {
            return operand.type == ZYDIS_OPERAND_TYPE_MEMORY &&
                operand.mem.base == base && operand.mem.index == ZYDIS_REGISTER_NONE &&
                operand.mem.disp.has_displacement &&
                operand.mem.disp.value == displacement;
        };

        if (!decoded(0, ZYDIS_MNEMONIC_MOVSS, 5) ||
            instruction.operand_count_visible != 2 ||
            !displacementIs(operands[0], ZYDIS_REGISTER_RSI, 0x58) ||
            operands[1].type != ZYDIS_OPERAND_TYPE_REGISTER ||
            operands[1].reg.value != ZYDIS_REGISTER_XMM0)
            return false;

        if (!decoded(5, ZYDIS_MNEMONIC_LEA, 4) ||
            instruction.operand_count_visible != 2 ||
            operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER ||
            operands[0].reg.value != ZYDIS_REGISTER_RCX ||
            !displacementIs(operands[1], ZYDIS_REGISTER_RSI, 0x28))
            return false;

        if (!decoded(9, ZYDIS_MNEMONIC_MOV, 4) ||
            instruction.operand_count_visible != 2 ||
            operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER ||
            operands[0].reg.value != ZYDIS_REGISTER_RAX ||
            !displacementIs(operands[1], ZYDIS_REGISTER_RSI, 0x28))
            return false;

        return decoded(13, ZYDIS_MNEMONIC_CALL, 3) &&
            instruction.operand_count_visible == 1 &&
            displacementIs(operands[0], ZYDIS_REGISTER_RAX, 0x10);
    }

    // Search only the validated executable span. Any duplicate byte sequence
    // is ambiguous even if one instance happens to decode differently.
    inline PostExitBlendActivationResolution ResolvePostExitBlendStartCapture(
        const ExecutableSpan& span) noexcept
    {
        PostExitBlendActivationResolution result{};
        constexpr auto sequenceSize = sizeof(signatures::PostExitBlendStartCapture);
        if (!span.begin || sequenceSize > span.size) return result;

        std::uint8_t* candidate{};
        for (std::size_t offset = 0; offset <= span.size - sequenceSize; ++offset) {
            auto* at = span.begin + offset;
            if (std::memcmp(at, signatures::PostExitBlendStartCapture,
                    sequenceSize) != 0) continue;
            ++result.matches;
            candidate = at;
        }
        if (result.matches != 1) {
            result.reason = result.matches ? "ambiguous-signature" : "no-signature";
            return result;
        }
        if (!ValidatePostExitBlendStartCapture(candidate, span)) {
            result.reason = "instruction-contract";
            return result;
        }
        result.hook = candidate + 5;
        result.reason = "validated";
        return result;
    }
}
