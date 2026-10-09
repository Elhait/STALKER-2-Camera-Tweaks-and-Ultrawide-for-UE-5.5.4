#include "../../src/gameplay/gameplay_camera.hpp"

#include <cmath>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <iterator>
#include <limits>

namespace
{
    std::uint8_t* g_memory = nullptr;
    bool Writable(std::uintptr_t address, std::size_t size)
    {
        const auto begin = reinterpret_cast<std::uintptr_t>(g_memory);
        return address >= begin && address - begin + size <= 16;
    }

    bool Check(bool condition, const char* name)
    {
        if (!condition) std::fprintf(stderr, "%s: FAIL\n", name);
        return condition;
    }
}

int main()
{
    std::uint8_t bytes[16]{};
    g_memory = bytes;
    const auto source = reinterpret_cast<std::uintptr_t>(bytes);
    const float aspect = 3.55556f;
    bool pass = true;
    pass &= Check(gameplay::WriteAspectOnly(source, 4, aspect, Writable),
        "aspect_only_write_succeeds");
    float written = 0.0f;
    std::memcpy(&written, bytes + 4, sizeof(written));
    pass &= Check(std::fabs(written - aspect) < 0.00001f,
        "aspect_only_value_written");
    pass &= Check(bytes[0] == 0 && bytes[8] == 0,
        "aspect_only_does_not_touch_flags_or_neighbors");
    std::fill(std::begin(bytes), std::end(bytes), static_cast<std::uint8_t>(0));
    pass &= Check(gameplay::WriteAspectAndFlags(source, 4, 8, aspect, 0x4, Writable),
        "aspect_and_flags_complete_write");
    pass &= Check(bytes[8] == 0x4, "flags_written_with_aspect");
    std::fill(std::begin(bytes), std::end(bytes), static_cast<std::uint8_t>(0xA5));
    pass &= Check(!gameplay::WriteAspectAndFlags(source, 4, 20, aspect, 0x4, Writable),
        "invalid_flags_refuse_write");
    pass &= Check(bytes[4] == 0xA5, "invalid_flags_no_partial_aspect_write");
    pass &= Check(!gameplay::WriteAspectAndFlags(
        (std::numeric_limits<std::uintptr_t>::max)(), 4, 8, aspect, 0x4, Writable),
        "write_offset_overflow_refusal");

    const auto sourceChange = gameplay::EvaluateGameplayContextChange(
        source, 90.0f, source + 32, 90.0f, 5.0f);
    const auto fovChange = gameplay::EvaluateGameplayContextChange(
        source, 90.0f, source, 100.0f, 5.0f);
    const auto stable = gameplay::EvaluateGameplayContextChange(
        source, 90.0f, source, 90.1f, 5.0f);
    const auto nativeVariance = gameplay::EvaluateGameplayContextChange(
        source, 90.0f, source, 90.6557f, 5.0f);
    pass &= Check(sourceChange.InvalidatesDialogue() && sourceChange.sourceChanged,
        "source_change_invalidates");
    pass &= Check(fovChange.InvalidatesDialogue() && fovChange.materialFovJump,
        "material_fov_jump_invalidates");
    pass &= Check(!stable.InvalidatesDialogue(), "stable_context_is_retained");
    pass &= Check(!nativeVariance.InvalidatesDialogue(),
        "subdegree_native_fov_variance_does_not_invalidate_camera_intent");

    std::printf("gameplay_camera_safety=%s\n", pass ? "PASS" : "FAIL");
    return pass ? 0 : 1;
}
