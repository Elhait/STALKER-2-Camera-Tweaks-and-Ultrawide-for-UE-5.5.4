#include "../../src/cinematics/cinematic_aspect.hpp"

#include <Windows.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>

namespace
{
    bool Check(bool condition, const char* name)
    {
        if (!condition) std::fprintf(stderr, "%s: FAIL\n", name);
        return condition;
    }

    bool Writable(std::uintptr_t address, std::size_t size)
    {
        return address != 0 && size != 0;
    }

    bool NeverWritable(std::uintptr_t, std::size_t) { return false; }
    bool ValidAspect(float value) { return std::isfinite(value) && value > 1.0f && value < 5.0f; }

    struct PeFixture
    {
        std::uint8_t* base{};
        std::size_t size{0x4000};
        std::uint8_t* text{};

        PeFixture()
        {
            base = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, size,
                MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));
            if (!base) return;
            auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
            dos->e_magic = IMAGE_DOS_SIGNATURE;
            dos->e_lfanew = 0x100;
            auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
            nt->Signature = IMAGE_NT_SIGNATURE;
            nt->FileHeader.NumberOfSections = 1;
            nt->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER64);
            auto* section = IMAGE_FIRST_SECTION(nt);
            std::memcpy(section->Name, ".text", 5);
            section->VirtualAddress = 0x1000;
            section->Misc.VirtualSize = 0x180;
            section->Characteristics = IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
            text = base + section->VirtualAddress;
        }

        ~PeFixture()
        {
            if (base) VirtualFree(base, 0, MEM_RELEASE);
        }
    };

    void PlaceValidStore(PeFixture& fixture, std::size_t matchOffset)
    {
        const std::uint8_t signature[] = { 0xAA, 0xBB, 0xCC };
        const std::uint8_t instruction[] = {
            0xC7, 0x80, 0x54, 0x02, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F };
        std::memcpy(fixture.text + matchOffset, signature, sizeof(signature));
        std::memcpy(fixture.text + matchOffset + 0x19, instruction, sizeof(instruction));
    }
}

int main()
{
    bool pass = true;
    const float forced21Aspect = cinematics::ResolveAspect(
        config::CinematicAspectPolicy::Forced21x9, 16.0f / 9.0f,
        cinematics::Forced21x9Aspect, 32.0f / 9.0f, nullptr);
    pass &= Check(std::fabs(forced21Aspect - (3440.0f / 1440.0f)) < 0.0001f,
        "forced21_policy_preserves_production_3440x1440_value");

    std::uint8_t bytes[16]{};
    const auto target = reinterpret_cast<std::uintptr_t>(bytes);
    const auto applied = cinematics::ApplyAspectStore(target, 3.55556f, 1.77778f,
        4, Writable, ValidAspect);
    float stored = 0.0f;
    std::memcpy(&stored, bytes + 4, sizeof(stored));
    pass &= Check(applied.writable && std::fabs(stored - 3.55556f) < 0.00001f,
        "writable_aspect_store");
    const auto fallback = cinematics::ApplyAspectStore(target, 0.0f, 1.77778f,
        4, Writable, ValidAspect);
    std::memcpy(&stored, bytes + 4, sizeof(stored));
    pass &= Check(fallback.writable && std::fabs(stored - 1.77778f) < 0.00001f,
        "invalid_aspect_native_fallback");
    pass &= Check(!cinematics::ApplyAspectStore(target, 3.5f, 1.77778f,
        4, NeverWritable, ValidAspect).writable, "unwritable_refusal");
    pass &= Check(!cinematics::ApplyAspectStore(0, 3.5f, 1.77778f,
        4, Writable, ValidAspect).writable, "null_target_refusal");
    pass &= Check(!cinematics::ApplyAspectStore((std::numeric_limits<std::uintptr_t>::max)(),
        3.5f, 1.77778f, 4, Writable, ValidAspect).writable, "offset_overflow_refusal");

    PeFixture valid;
    if (!valid.base) return 1;
    PlaceValidStore(valid, 0x40);
    const std::uint8_t prefix[] = { 0xC7, 0x80 };
    const std::uint8_t immediate[] = { 0x00, 0x00, 0x80, 0x3F };
    const auto one = cinematics::ResolveAspectStore(valid.base, "AA BB CC", 0x254,
        prefix, sizeof(prefix), immediate, 6, sizeof(immediate), 10, 0x3F800000);
    pass &= Check(one.matches == 1 && one.store != nullptr, "resolver_one_valid_match");
    PlaceValidStore(valid, 0x90);
    const auto many = cinematics::ResolveAspectStore(valid.base, "AA BB CC", 0x254,
        prefix, sizeof(prefix), immediate, 6, sizeof(immediate), 10, 0x3F800000);
    pass &= Check(many.matches == 2 && many.store == nullptr, "resolver_multiple_refusal");
    const auto none = cinematics::ResolveAspectStore(valid.base, "DE AD BE EF", 0x254,
        prefix, sizeof(prefix), immediate, 6, sizeof(immediate), 10, 0x3F800000);
    pass &= Check(none.matches == 0 && none.store == nullptr, "resolver_zero_match");

    PeFixture malformed;
    PlaceValidStore(malformed, 0x40);
    malformed.text[0x40 + 0x19] = 0x90;
    const auto malformedCandidate = cinematics::ResolveAspectStore(malformed.base, "AA BB CC", 0x254,
        prefix, sizeof(prefix), immediate, 6, sizeof(immediate), 10, 0x3F800000);
    pass &= Check(malformedCandidate.matches == 1 && malformedCandidate.store == nullptr,
        "resolver_malformed_candidate_refusal");

    PeFixture truncated;
    PlaceValidStore(truncated, 0x40);
    auto* truncatedNt = reinterpret_cast<IMAGE_NT_HEADERS64*>(truncated.base + 0x100);
    IMAGE_FIRST_SECTION(truncatedNt)->Misc.VirtualSize = 0x60;
    const auto truncatedCandidate = cinematics::ResolveAspectStore(truncated.base, "AA BB CC", 0x254,
        prefix, sizeof(prefix), immediate, 6, sizeof(immediate), 10, 0x3F800000);
    pass &= Check(truncatedCandidate.matches == 1 && truncatedCandidate.store == nullptr,
        "resolver_truncated_candidate_refusal");

    std::printf("cinematic_aspect=%s\n", pass ? "PASS" : "FAIL");
    return pass ? 0 : 1;
}
