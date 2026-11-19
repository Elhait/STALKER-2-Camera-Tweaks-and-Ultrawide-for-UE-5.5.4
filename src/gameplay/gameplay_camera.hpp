#pragma once

#include <cstddef>
#include <cstdint>

namespace gameplay
{
    using WritablePredicate = bool (*)(std::uintptr_t address, std::size_t size);

    struct CameraWriterResolution
    {
        std::uint8_t* writeAddress{};
        std::size_t matches{};
        bool imageValid{};
        std::uint8_t* postStoreObservationAddress{};
        std::uint8_t* postProcessObservationAddress{};
        std::uint8_t* postProcessHelperAddress{};
    };

    CameraWriterResolution ResolveCameraWriter(void* executable,
        bool resolvePostStoreObservation = false);

    bool WriteAspectAndFlags(std::uintptr_t source, std::uintptr_t aspectOffset,
        std::uintptr_t flagsOffset, float aspect, std::uint8_t flags,
        WritablePredicate isWritable);

    bool WriteAspectOnly(std::uintptr_t source, std::uintptr_t aspectOffset,
        float aspect, WritablePredicate isWritable);

    struct GameplayContextChange
    {
        bool sourceChanged{};
        bool materialFovJump{};

        bool InvalidatesDialogue() const noexcept
        {
            return sourceChanged || materialFovJump;
        }
    };

    GameplayContextChange EvaluateGameplayContextChange(
        std::uintptr_t previousSource, float previousFov,
        std::uintptr_t source, float fov, float fovJumpThreshold) noexcept;
}
