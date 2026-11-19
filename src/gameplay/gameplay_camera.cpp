#include "gameplay_camera.hpp"

#include "../hooks/instruction_validator.hpp"
#include "../hooks/signatures/signature_definitions.hpp"

#include <windows.h>
#include <cstring>
#include <limits>
#include <cmath>

namespace gameplay
{
    bool WriteAspectOnly(std::uintptr_t source, std::uintptr_t aspectOffset,
        float aspect, WritablePredicate isWritable)
    {
        if (!isWritable || source > (std::numeric_limits<std::uintptr_t>::max)() - aspectOffset)
            return false;
        const auto aspectAddress = source + aspectOffset;
        if (!isWritable(aspectAddress, sizeof(aspect))) return false;
        std::memcpy(reinterpret_cast<void*>(aspectAddress), &aspect, sizeof(aspect));
        return true;
    }

    GameplayContextChange EvaluateGameplayContextChange(
        std::uintptr_t previousSource, float previousFov,
        std::uintptr_t source, float fov, float fovJumpThreshold) noexcept
    {
        const bool sourceChanged = previousSource != 0 && source != 0 && previousSource != source;
        const bool materialFovJump = previousSource == source && std::isfinite(previousFov) &&
            std::isfinite(fov) && std::fabs(fov - previousFov) > fovJumpThreshold;
        return { sourceChanged, materialFovJump };
    }

    bool WriteAspectAndFlags(std::uintptr_t source, std::uintptr_t aspectOffset,
        std::uintptr_t flagsOffset, float aspect, std::uint8_t flags,
        WritablePredicate isWritable)
    {
        if (!isWritable || source > (std::numeric_limits<std::uintptr_t>::max)() - aspectOffset ||
            source > (std::numeric_limits<std::uintptr_t>::max)() - flagsOffset) return false;
        const auto aspectAddress = source + aspectOffset;
        const auto flagsAddress = source + flagsOffset;
        if (!isWritable(aspectAddress, sizeof(aspect)) ||
            !isWritable(flagsAddress, sizeof(flags))) return false;
        std::memcpy(reinterpret_cast<void*>(aspectAddress), &aspect, sizeof(aspect));
        std::memcpy(reinterpret_cast<void*>(flagsAddress), &flags, sizeof(flags));
        return true;
    }

    CameraWriterResolution ResolveCameraWriter(void* executable,
        bool resolvePostStoreObservation)
    {
        const auto* base = reinterpret_cast<const std::uint8_t*>(executable);
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE) return {};
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) return {};

        const auto* section = IMAGE_FIRST_SECTION(nt);
        const std::uint8_t* textStart = nullptr;
        std::size_t textSize = 0;
        for (std::uint16_t i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section) {
            if (std::memcmp(section->Name, ".text", 5) == 0) {
                textStart = base + section->VirtualAddress;
                textSize = section->Misc.VirtualSize;
                break;
            }
        }
        if (!textStart || textSize < sizeof(hooks::signatures::CameraWriter))
            return { nullptr, 0, true };

        std::uint8_t* match = nullptr;
        std::size_t matches = 0;
        for (std::size_t offset = 0; offset <= textSize - sizeof(hooks::signatures::CameraWriter); ++offset) {
            const auto* candidate = textStart + offset;
            bool matchesSignature = true;
            for (std::size_t i = 0; i < sizeof(hooks::signatures::CameraWriter); ++i) {
                // The 32-bit relative target of JNZ may move between builds.
                if ((i < 17 || i >= 21) && candidate[i] != hooks::signatures::CameraWriter[i]) {
                    matchesSignature = false;
                    break;
                }
            }
            if (matchesSignature) {
                match = const_cast<std::uint8_t*>(candidate);
                ++matches;
            }
        }
        if (matches != 1) return { nullptr, matches, true };

        auto* target = match + hooks::signatures::FovWriteOffsetInCameraWriter;
        const hooks::ExecutableSpan textSpan{ const_cast<std::uint8_t*>(textStart), textSize };
        ZydisDecodedInstruction instruction{};
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT]{};
        const bool verified = hooks::validation::DecodeInstruction(target, instruction, operands, textSpan) &&
            instruction.mnemonic == ZYDIS_MNEMONIC_MOVSS && instruction.operand_count_visible >= 2 &&
            operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY && operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER &&
            operands[0].mem.base == ZYDIS_REGISTER_RBX && operands[0].mem.disp.has_displacement &&
            operands[0].mem.disp.value == 0x30 && operands[1].reg.value == ZYDIS_REGISTER_XMM0;
        if (!verified) return { nullptr, matches, true };

        if (!resolvePostStoreObservation)
            return { target, matches, true, nullptr, nullptr, nullptr };

        auto* postStore = target + instruction.length;
        ZydisDecodedInstruction nextInstruction{};
        ZydisDecodedOperand nextOperands[ZYDIS_MAX_OPERAND_COUNT]{};
        const bool postStoreVerified = hooks::validation::DecodeInstruction(
                postStore, nextInstruction, nextOperands, textSpan) &&
            nextInstruction.mnemonic == ZYDIS_MNEMONIC_MOVSS &&
            nextInstruction.operand_count_visible >= 2 &&
            nextOperands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
            nextOperands[0].reg.value == ZYDIS_REGISTER_XMM0 &&
            nextOperands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
            nextOperands[1].mem.base == ZYDIS_REGISTER_RSI &&
            nextOperands[1].mem.disp.has_displacement &&
            nextOperands[1].mem.disp.value == 0x254;

        if (!postStoreVerified)
            return { target, matches, true, nullptr, nullptr, nullptr };

        std::uint8_t* postProcessObservation = nullptr;
        std::uint8_t* postProcessHelper = nullptr;
        std::uint8_t* postProcessMatch = nullptr;
        std::size_t postProcessMatches = 0;
        constexpr auto signatureSize = sizeof(hooks::signatures::CameraWriterPostProcessCall);
        if (textSize >= signatureSize) {
            for (std::size_t offset = 0; offset <= textSize - signatureSize; ++offset) {
                const auto* candidate = textStart + offset;
                bool signatureMatches = true;
                for (std::size_t i = 0; i < signatureSize; ++i) {
                    const bool relativeCallDisplacement =
                        i > hooks::signatures::CameraWriterPostProcessCallOffset &&
                        i < hooks::signatures::CameraWriterPostProcessCallOffset + 5;
                    if (!relativeCallDisplacement &&
                        candidate[i] != hooks::signatures::CameraWriterPostProcessCall[i]) {
                        signatureMatches = false;
                        break;
                    }
                }
                if (signatureMatches) {
                    postProcessMatch = const_cast<std::uint8_t*>(candidate);
                    ++postProcessMatches;
                }
            }
        }

        if (postProcessMatches == 1 && postProcessMatch) {
            auto* callSite = postProcessMatch +
                hooks::signatures::CameraWriterPostProcessCallOffset;
            ZydisDecodedInstruction callInstruction{};
            ZydisDecodedOperand callOperands[ZYDIS_MAX_OPERAND_COUNT]{};
            const bool callVerified = hooks::validation::DecodeInstruction(
                    callSite, callInstruction, callOperands, textSpan) &&
                callInstruction.mnemonic == ZYDIS_MNEMONIC_CALL &&
                callInstruction.length == 5 &&
                hooks::validation::IsCallRel32(callSite, textSpan);
            auto* helper = callVerified
                ? hooks::validation::ResolveRel32CallTarget(callSite, textSpan)
                : nullptr;
            const auto textBegin = reinterpret_cast<std::uintptr_t>(textStart);
            const auto textEnd = textBegin + textSize;
            const bool helperInText = helper &&
                reinterpret_cast<std::uintptr_t>(helper) >= textBegin &&
                reinterpret_cast<std::uintptr_t>(helper) < textEnd;

            auto* afterCall = callSite + callInstruction.length;
            ZydisDecodedInstruction continuationInstruction{};
            ZydisDecodedOperand continuationOperands[ZYDIS_MAX_OPERAND_COUNT]{};
            const bool continuationVerified = helperInText &&
                hooks::validation::DecodeInstruction(afterCall,
                    continuationInstruction, continuationOperands, textSpan) &&
                continuationInstruction.mnemonic == ZYDIS_MNEMONIC_CMP &&
                continuationInstruction.operand_count_visible >= 2 &&
                continuationOperands[0].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                continuationOperands[0].mem.base == ZYDIS_REGISTER_RSI &&
                continuationOperands[0].mem.disp.has_displacement &&
                continuationOperands[0].mem.disp.value == 0x240 &&
                continuationOperands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
                continuationOperands[1].imm.value.u == 0;
            if (continuationVerified) {
                postProcessObservation = afterCall;
                postProcessHelper = helper;
            }
        }

        return { target, matches, true, postStore, postProcessObservation,
            postProcessHelper };
    }
}
