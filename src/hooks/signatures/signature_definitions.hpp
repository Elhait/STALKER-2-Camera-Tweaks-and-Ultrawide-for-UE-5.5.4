#pragma once

#include <cstddef>
#include <cstdint>

namespace hooks::signatures
{
    // This sequence identifies the complete camera-view copy path, not merely
    // a generic MOVSS instruction. The JNZ displacement may change between builds.
    inline constexpr std::uint8_t CameraWriter[] = {
        0xF6, 0x86, 0x62, 0x02, 0x00, 0x00, 0x10,
        0xF3, 0x0F, 0x10, 0x86, 0x30, 0x02, 0x00, 0x00,
        0x0F, 0x85, 0x00, 0x00, 0x00, 0x00,
        0x48, 0x8D, 0x4B, 0x30,
        0xF3, 0x0F, 0x11, 0x43, 0x30,
        0xF3, 0x0F, 0x10, 0x86, 0x54, 0x02, 0x00, 0x00,
        0xF3, 0x0F, 0x11, 0x43, 0x5C,
        0x0F, 0xB6, 0x96, 0x59, 0x02, 0x00, 0x00,
        0x8B, 0x43, 0x68, 0x83, 0xE2, 0x01, 0x83, 0xE0, 0xFE,
        0x09, 0xD0, 0x89, 0x43, 0x68,
        0x0F, 0xB6, 0x96, 0x59, 0x02, 0x00, 0x00,
        0x83, 0xE2, 0x04, 0x83, 0xE0, 0xFB, 0x09, 0xD0, 0x89,
        0x43, 0x68, 0x8A, 0x96, 0x63, 0x02, 0x00, 0x00, 0x88,
        0x53, 0x6C,
    };
    inline constexpr std::size_t FovWriteOffsetInCameraWriter = 25;

    inline constexpr char DialogueBoundary[] =
        "48 8B 0A 48 8B 01 0F 28 CE FF 90 08 06 00 00 "
        "0F 2E 76 2C 75 02 7B 1A";

    inline constexpr char ZoomIn[] =
        "F3 0F 10 40 4C F3 0F 10 8E 38 01 00 00 0F 2E C8";
    inline constexpr char ZoomOut[] =
        "F3 0F 10 40 50 F3 0F 10 8E 3C 01 00 00 0F 2E C8";

    // Ghidra-verified language reader entry signature and a distinctive
    // interior anchor in the matching game-allocator release function. Resolve
    // the anchor through the PE exception table to its owning function entry;
    // compiler changes move the anchor within the function across game builds.
    inline constexpr char GameLanguageReader[] =
        "41 56 56 57 53 48 83 EC 28 48 89 CE E8 ?? ?? ?? ?? "
        "48 8B 48 28 48 8B 78 30 48 85 FF 74 ?? F0 FF 47 08 "
        "E8 ?? ?? ?? ?? 48 C7 06 00 00 00 00 48 63 58 08 4C 8B 30 "
        "89 5E 08 48 85 DB 74 ?? 48 89 F1 89 DA 45 31 C0 "
        "E8 ?? ?? ?? ?? 48 8B 0E 48 01 DB 4C 89 F2 49 89 D8 E8 ?? ?? ?? ??";
    inline constexpr char GameLanguageBufferFreeAnchor[] =
        "48 89 C1 48 C1 E1 04 49 89 34 0E 48 C1 E0 04 "
        "4D 89 6C 06 08 4C 01 AF 38 15 00 00 FF 87 40 15 00 00";

    // These signatures describe the 2.0.3 -> 2.0.4 transition topology, not
    // fixed addresses. Relative call displacements and the ENTER RIP-relative
    // scalar displacement are intentionally wildcarded.
    inline constexpr char CinematicAspectSetter[] =
        "48 8B 41 18 48 8B 88 F8 00 00 00 80 89 59 02 00 00 01 "
        "48 8B 80 F8 00 00 00 C7 80 54 02 00 00 39 8E E3 3F B0 01 C3";
    inline constexpr char CinematicEnter[] =
        "F3 0F 10 05 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 89 F1 E8 ?? ?? ?? ?? "
        "48 89 C1 31 D2 E8 ?? ?? ?? ??";
    inline constexpr char CinematicExit[] =
        "F3 0F 10 47 38 E8 ?? ?? ?? ?? 48 89 F1 E8 ?? ?? ?? ?? "
        "48 89 C1 31 D2 E8 ?? ?? ?? ??";
    // Current post-update EXIT topology: the FOV sample is loaded through an
    // indexed source expression before entering the same consumer chain.
    inline constexpr char CinematicExitIndexed[] =
        "40 0F B6 C7 F3 0F 10 44 83 38 E8 ?? ?? ?? ?? "
        "48 89 F1 E8 ?? ?? ?? ?? 48 89 C1 31 D2 E8 ?? ?? ?? ?? "
        "48 85 C0 74 ?? 48 89 C7 48 8B 00 48 89 F9 "
        "FF 90 80 08 00 00 48 8B 07 48 89 F9 FF 90 68 08 00 00";
    inline constexpr std::uint8_t EnterVcallPair[] = {
        0xFF, 0x90, 0x78, 0x08, 0x00, 0x00, 0x48, 0x8B, 0x07, 0x48, 0x89, 0xF9,
        0xB2, 0x01, 0xFF, 0x90, 0x60, 0x08, 0x00, 0x00,
    };
    inline constexpr std::uint8_t ExitVcallPair[] = {
        0xFF, 0x90, 0x80, 0x08, 0x00, 0x00, 0x48, 0x8B, 0x07, 0x48, 0x89, 0xF9,
        0xFF, 0x90, 0x68, 0x08, 0x00, 0x00,
    };
}
