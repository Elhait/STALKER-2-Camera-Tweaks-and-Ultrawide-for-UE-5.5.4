#include "cinematic_aspect.hpp"

#include "../hooks/instruction_validator.hpp"
#include "../hooks/signature_scanner.hpp"

#include <cstring>
#include <limits>
#include <windows.h>

namespace cinematics
{
    AspectStoreApplication ApplyAspectStore(std::uintptr_t targetObject,
        float resolvedAspect, float nativeAspect, std::uintptr_t aspectOffset,
        WritablePredicate isWritable, AspectValidator isValidAspect)
    {
        const float aspect = isValidAspect && isValidAspect(resolvedAspect)
            ? resolvedAspect : nativeAspect;
        const bool writable = targetObject &&
            targetObject <= (std::numeric_limits<std::uintptr_t>::max)() - aspectOffset &&
            isWritable && isWritable(targetObject + aspectOffset, sizeof(aspect));
        if (writable)
            std::memcpy(reinterpret_cast<void*>(targetObject + aspectOffset), &aspect, sizeof(aspect));
        return { aspect, writable };
    }

    AspectStoreResolution ResolveAspectStore(void* executable, const char* signature,
        std::uintptr_t aspectOffset, const std::uint8_t* storePrefix,
        std::size_t storePrefixSize, const std::uint8_t* expectedImmediate,
        std::size_t immediateOffset, std::size_t immediateSize,
        std::size_t storeInstructionLength, std::uint32_t expectedImmediateValue)
    {
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(executable);
        if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE) return {};
        const auto* base = reinterpret_cast<const std::uint8_t*>(executable);
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) return {};

        const auto matches = hooks::FindAll(executable, signature);
        hooks::ExecutableSpan textSpan{};
        if (!hooks::FindSectionSpan(executable, ".text", textSpan)) return {};
        if (matches.size() != 1) return { nullptr, matches.size(), true };
        auto* candidate = matches.front() + 0x19;
        if (!textSpan.Contains(candidate, storeInstructionLength) ||
            !textSpan.Contains(candidate + immediateOffset, immediateSize) ||
            !hooks::validation::IsExecutable(reinterpret_cast<std::uintptr_t>(candidate)) ||
            std::memcmp(candidate, storePrefix, storePrefixSize) != 0 ||
            std::memcmp(candidate + immediateOffset, expectedImmediate, immediateSize) != 0)
            return { nullptr, matches.size(), true };

        ZydisDecodedInstruction instruction{};
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT]{};
        const bool valid = hooks::validation::DecodeInstruction(candidate, instruction, operands, textSpan) &&
            instruction.mnemonic == ZYDIS_MNEMONIC_MOV &&
            instruction.length == storeInstructionLength &&
            instruction.operand_count_visible >= 2 &&
            operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY &&
            operands[0].mem.base == ZYDIS_REGISTER_RAX &&
            operands[0].mem.disp.has_displacement &&
            operands[0].mem.disp.value == static_cast<std::int64_t>(aspectOffset) &&
            operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
            operands[1].imm.value.u == expectedImmediateValue;
        return valid ? AspectStoreResolution{ candidate, matches.size(), true }
                     : AspectStoreResolution{ nullptr, matches.size(), true };
    }

    bool OverrideEnabled(config::CinematicAspectPolicy policy)
    {
        return policy != config::CinematicAspectPolicy::Native;
    }

    float ResolveAspect(config::CinematicAspectPolicy policy, float nativeAspect,
        float cinemaAspect, float wideAspect, AutoAspectResolver autoResolver)
    {
        return ResolveAspect(policy, nativeAspect, cinemaAspect, wideAspect,
            policy == config::CinematicAspectPolicy::Auto && autoResolver ? autoResolver() : nativeAspect);
    }
}
