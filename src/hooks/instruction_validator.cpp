#include "instruction_validator.hpp"

#include <cstring>
#include <windows.h>

namespace hooks::validation
{
    bool IsExecutable(std::uintptr_t address)
    {
        MEMORY_BASIC_INFORMATION info{};
        if (!address || !VirtualQuery(reinterpret_cast<const void*>(address), &info, sizeof(info)) ||
            info.State != MEM_COMMIT || (info.Protect & PAGE_GUARD)) return false;
        const auto protection = info.Protect & 0xFF;
        return protection == PAGE_EXECUTE || protection == PAGE_EXECUTE_READ ||
            protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
    }

    bool DecodeInstruction(std::uint8_t* address, ZydisDecodedInstruction& instruction,
        ZydisDecodedOperand* operands, const ExecutableSpan& span)
    {
        if (!span.Contains(address, 1)) return false;
        ZydisDecoder decoder{};
        const auto remaining = span.size - static_cast<std::size_t>(address - span.begin);
        return ZYAN_SUCCESS(ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64)) &&
            ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, address,
                remaining < 15 ? remaining : 15, &instruction, operands));
    }

    bool IsCallRel32(std::uint8_t* address, const ExecutableSpan& span)
    {
        ZydisDecodedInstruction instruction{};
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT]{};
        return span.Contains(address, 5) && DecodeInstruction(address, instruction, operands, span) &&
            address[0] == 0xE8 &&
            instruction.mnemonic == ZYDIS_MNEMONIC_CALL && instruction.length == 5 &&
            instruction.operand_count_visible == 1 &&
            operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE;
    }

    bool ContainsBytes(std::uint8_t* start, std::size_t length,
        const std::uint8_t* bytes, std::size_t byteCount, const ExecutableSpan& span)
    {
        if (length > span.size || !span.Contains(start, length)) return false;
        for (std::size_t offset = 0; offset + byteCount <= length; ++offset)
            if (std::memcmp(start + offset, bytes, byteCount) == 0) return true;
        return false;
    }

    std::uint8_t* ResolveRel32CallTarget(std::uint8_t* address, const ExecutableSpan& span)
    {
        if (!IsCallRel32(address, span)) return nullptr;
        std::int32_t displacement{};
        std::memcpy(&displacement, address + 1, sizeof(displacement));
        const auto target = reinterpret_cast<std::intptr_t>(address) + 5 + displacement;
        if (target < 0 || !span.Contains(reinterpret_cast<const std::uint8_t*>(target), 1)) return nullptr;
        return reinterpret_cast<std::uint8_t*>(target);
    }
}
