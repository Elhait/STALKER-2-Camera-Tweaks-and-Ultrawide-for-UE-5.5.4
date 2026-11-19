#pragma once
#include "instruction_validator.hpp"
#include <cstring>

namespace hooks {
struct BlendProducerResolution {
    std::uintptr_t returnAddress{};
    std::size_t matches{};
    float operand{};
    const char* reason{"invalid-span"};
};

// Identify the provider-resolution / constant-argument / wrapper-CALL path.
// The literal's numeric value is reported, never used to classify the caller.
inline BlendProducerResolution ResolveBlendProducer(const ExecutableSpan& text,
    const ExecutableSpan& data, std::uint8_t* wrapper,
    const ExecutableSpan& alternateData = {}) noexcept {
    BlendProducerResolution out{};
    constexpr std::size_t size = 34;
    if (!text.begin || text.size < size || !text.Contains(wrapper, 1)) return out;
    constexpr std::uint8_t prefix[]{0x48,0x8D,0x74,0x24,0x20,0x48,0x89,0xF2,0xE8};
    constexpr std::uint8_t middle[]{0x48,0x8B,0x0E,0x48,0x85,0xC9,0x74,0x0D,0xF3,0x0F,0x10,0x0D};
    for (std::size_t i=0; i<=text.size-size; ++i) {
        auto* p=text.begin+i;
        if (std::memcmp(p,prefix,sizeof(prefix)) ||
            std::memcmp(p+13,middle,sizeof(middle)) || p[29]!=0xE8) continue;
        ++out.matches;
        bool valid=validation::ResolveRel32CallTarget(p+8,text) &&
            validation::ResolveRel32CallTarget(p+29,text)==wrapper;
        std::size_t offset{};
        while(valid && offset<size) {
            ZydisDecodedInstruction ix{}; ZydisDecodedOperand ops[ZYDIS_MAX_OPERAND_COUNT]{};
            valid=validation::DecodeInstruction(p+offset,ix,ops,text) && ix.length &&
                ix.length<=size-offset;
            if(valid) offset+=ix.length;
        }
        // RIP-relative operand ends at p+29; validate the referenced data span.
        std::int32_t d{}; std::memcpy(&d,p+25,4);
        const auto end=reinterpret_cast<std::uintptr_t>(p+29);
        const auto magnitude=d<0 ? static_cast<std::uint64_t>(-static_cast<std::int64_t>(d)) : static_cast<std::uint64_t>(d);
        if ((d<0 && end<magnitude) || (d>=0 && end>UINTPTR_MAX-magnitude)) valid=false;
        const auto operand=d<0 ? end-magnitude : end+magnitude;
        if (!data.Contains(reinterpret_cast<std::uint8_t*>(operand),sizeof(float)) &&
            !alternateData.Contains(reinterpret_cast<std::uint8_t*>(operand),sizeof(float))) valid=false;
        if(valid) {
            std::memcpy(&out.operand,reinterpret_cast<const void*>(operand),sizeof(float));
            out.returnAddress=reinterpret_cast<std::uintptr_t>(p+size);
        }
    }
    if(out.matches!=1 || !out.returnAddress) {
        out.returnAddress=0;
        out.reason=out.matches>1 ? "ambiguous" : (out.matches ? "invalid-linked-contract" : "not-found");
    } else out.reason="validated-provider-call-path";
    return out;
}
}
