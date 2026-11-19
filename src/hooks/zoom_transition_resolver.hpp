#pragma once

#include "instruction_validator.hpp"
#include "signatures/signature_definitions.hpp"

namespace hooks
{
    struct ZoomTransitionResolution
    {
        std::uint8_t* in{};
        std::uint8_t* out{};
        std::size_t inMatches{};
        std::size_t outMatches{};
        const char* reason{"image-span-unavailable"};
        explicit operator bool() const noexcept { return in && out; }
    };

    // The two loads establish the callback's RAX/RSI and XMM contracts.
    // The callback runs before UCOMISS and preserves the native comparison.
    inline bool ValidateZoomTransition(std::uint8_t* match, bool entering,
        const ExecutableSpan& span)
    {
        if (!span.Contains(match, 16) ||
            !validation::IsExecutable(reinterpret_cast<std::uintptr_t>(match))) return false;
        ZydisDecodedInstruction instruction{};
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT]{};
        const auto load = [&](std::size_t offset, unsigned length,
            ZydisRegister destination, ZydisRegister source, std::int64_t displacement) {
            return validation::DecodeInstruction(match + offset, instruction, operands, span) &&
                instruction.mnemonic == ZYDIS_MNEMONIC_MOVSS && instruction.length == length &&
                instruction.operand_count_visible == 2 &&
                operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER && operands[0].reg.value == destination &&
                operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY && operands[1].mem.base == source &&
                operands[1].mem.index == ZYDIS_REGISTER_NONE &&
                operands[1].mem.disp.has_displacement && operands[1].mem.disp.value == displacement;
        };
        return load(0, 5, ZYDIS_REGISTER_XMM0, ZYDIS_REGISTER_RAX, entering ? 0x4C : 0x50) &&
            load(5, 8, ZYDIS_REGISTER_XMM1, ZYDIS_REGISTER_RSI, entering ? 0x138 : 0x13C) &&
            validation::DecodeInstruction(match + 13, instruction, operands, span) &&
            instruction.mnemonic == ZYDIS_MNEMONIC_UCOMISS && instruction.length == 3 &&
            instruction.operand_count_visible == 2 &&
            operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER && operands[0].reg.value == ZYDIS_REGISTER_XMM1 &&
            operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER && operands[1].reg.value == ZYDIS_REGISTER_XMM0;
    }

    // No hash, patch number or disk-file availability participates in admission.
    inline ZoomTransitionResolution ResolveZoomTransitions(void* executable)
    {
        ZoomTransitionResolution result;
        ExecutableSpan span{};
        if (!executable || !FindSectionSpan(executable, ".text", span)) return result;
        const auto in = FindAll(executable, signatures::ZoomIn);
        const auto out = FindAll(executable, signatures::ZoomOut);
        result.inMatches = in.size();
        result.outMatches = out.size();
        if (in.size() != 1 || out.size() != 1) {
            result.reason = "signature-cardinality";
            return result;
        }
        if (!ValidateZoomTransition(in.front(), true, span) ||
            !ValidateZoomTransition(out.front(), false, span)) {
            result.reason = "instruction-contract";
            return result;
        }
        result.in = in.front() + 13;
        result.out = out.front() + 13;
        result.reason = "validated";
        return result;
    }
}
