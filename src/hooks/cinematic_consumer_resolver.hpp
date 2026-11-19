#pragma once
#include "instruction_validator.hpp"
#include <array>
#include <cstring>

namespace hooks {
struct CinematicConsumerSites {
    // Return, resolver result, guard result, shared wrapper epilogue.
    std::array<std::uint8_t*, 4> points{};
    std::uint8_t* wrapper{};
    const char* reason{"invalid-contract"};
    explicit operator bool() const noexcept { return points[0] != nullptr; }
};
inline CinematicConsumerSites ResolveCinematicConsumerSites(
    std::uint8_t* enter, const ExecutableSpan& span) noexcept {
    CinematicConsumerSites out;
    const auto same = [&](std::uint8_t* at, const auto& bytes) {
        return span.Contains(at, bytes.size()) && !std::memcmp(at, bytes.data(), bytes.size());
    };
    const auto find = [&](std::uint8_t* begin, std::size_t size, const auto& bytes) {
        std::uint8_t* match{};
        if (!span.Contains(begin, size) || bytes.size() > size) return match;
        for (std::size_t i = 0; i <= size - bytes.size(); ++i) {
            if (!same(begin + i, bytes)) continue;
            if (match) return static_cast<std::uint8_t*>(nullptr);
            match = begin + i;
        }
        return match;
    };
    auto* consumer = validation::ResolveRel32CallTarget(enter, span);
    if (!consumer) return out;
    constexpr std::array<std::uint8_t, 16> tail{
        0x48,0x89,0xF1,0x0F,0x28,0xCE,0x0F,0x28,0x74,0x24,0x20,0x48,0x83,0xC4,0x30,0x5E};
    auto* forward = find(consumer, 0x60, tail);
    if (!forward || !span.Contains(forward + tail.size(), 5) || forward[tail.size()] != 0xE9) return out;
    std::int32_t displacement{};
    std::memcpy(&displacement, forward + tail.size() + 1, 4);
    const auto address = reinterpret_cast<std::uintptr_t>(forward + tail.size() + 5);
    if (displacement < 0 && address < static_cast<std::uint64_t>(-static_cast<std::int64_t>(displacement))) return out;
    auto* wrapper = reinterpret_cast<std::uint8_t*>(address + displacement);
    if (!span.Contains(wrapper, 0x150)) return out;
    constexpr std::array<std::uint8_t, 15> prologue{
        0x56,0x57,0x53,0x48,0x83,0xEC,0x30,0x0F,0x29,0x74,0x24,0x20,0x0F,0x28,0xF1};
    if (!same(wrapper, prologue)) return out;
    // Match instruction identities after the preceding CALL instead of its displacement.
    constexpr std::array<std::uint8_t, 3> testRax{0x48,0x85,0xC0};
    constexpr std::array<std::uint8_t, 2> testAl{0x84,0xC0};
    constexpr std::array<std::uint8_t, 8> comparison{0xF3,0x0F,0x10,0x41,0x54,0x0F,0x2E,0xC6};
    auto* resolved = find(wrapper, 0x40, testRax);
    auto* guarded = find(wrapper, 0x60, testAl);
    auto* compared = find(wrapper, 0x130, comparison);
    if (!resolved || !guarded || !compared ||
        !validation::IsCallRel32(resolved - 5, span) ||
        !validation::IsCallRel32(guarded - 5, span)) return out;
    auto* compareBranch = compared + comparison.size();
    if (!span.Contains(resolved, 12) || resolved[3] != 0x0F || resolved[4] != 0x84 ||
        !span.Contains(guarded, 11) || guarded[2] != 0x0F || guarded[3] != 0x84 ||
        !span.Contains(compareBranch, 9) || compareBranch[0] != 0x75 || compareBranch[2] != 0x7A) return out;
    const auto rel32 = [](std::uint8_t* instruction, std::size_t length, std::size_t disp) {
        std::int32_t d{}; std::memcpy(&d, instruction + disp, 4);
        return reinterpret_cast<std::uintptr_t>(instruction + length) + d;
    };
    const auto epilogue = reinterpret_cast<std::uintptr_t>(compareBranch + 4);
    if (rel32(resolved + 3, 6, 2) != epilogue || rel32(guarded + 2, 6, 2) != epilogue ||
        compareBranch + 2 + static_cast<std::int8_t>(compareBranch[1]) !=
        compareBranch + 4 + static_cast<std::int8_t>(compareBranch[3])) return out;
    constexpr std::array<std::uint8_t, 9> flagsCheck{
        0x8B,0x48,0x08,0x0F,0xBA,0xE1,0x1E,0x0F,0x82};
    if (!same(resolved + 9, flagsCheck) || !span.Contains(resolved + 16, 6) ||
        rel32(resolved + 16, 6, 2) != epilogue) return out;
    auto* dispatch = compareBranch + 2 + static_cast<std::int8_t>(compareBranch[1]);
    constexpr std::array<std::uint8_t,16> initializerTail{
        0x0F,0x28,0xCE,0x0F,0x28,0x74,0x24,0x20,0x48,0x83,0xC4,0x30,0x5B,0x5F,0x5E,0xE9};
    if (!same(dispatch, initializerTail) || !span.Contains(dispatch,20) ||
        !span.Contains(reinterpret_cast<std::uint8_t*>(rel32(dispatch+15,5,1)),1)) return out;
    constexpr std::array<std::uint8_t, 3> afterEnter{0x48,0x89,0xF1};
    if (!same(enter + 5, afterEnter) || !validation::IsCallRel32(enter + 8, span)) return out;
    out.points = {enter + 5, resolved, guarded, compareBranch + 4};
    // Decode each relocated instruction span; SafetyHook must use the same boundaries.
    for (auto* point : out.points) {
        std::size_t total{};
        while (total < 5) {
            ZydisDecodedInstruction ix{}; ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT]{};
            if (!validation::DecodeInstruction(point + total, ix, operands, span) || !ix.length) return {};
            total += ix.length;
        }
        if (total > 16 || !span.Contains(point, total)) return {};
    }
    out.reason = "validated-linked-call-branches";
    out.wrapper = wrapper;
    return out;
}
}
