#pragma once

#include "../config/feature_config.hpp"

#include <cstddef>
#include <cstdint>

namespace cinematics
{
    // Established PC 21:9 framing policy. It intentionally uses the validated
    // 3440x1440 value, not the mathematical 21/9 ratio.
    inline constexpr float Forced21x9Aspect = 3440.0f / 1440.0f;

    using AutoAspectResolver = float (*)();
    using WritablePredicate = bool (*)(std::uintptr_t address, std::size_t size);
    using AspectValidator = bool (*)(float aspect);

    bool OverrideEnabled(config::CinematicAspectPolicy policy);
    inline float ResolveAspect(config::CinematicAspectPolicy policy, float nativeAspect,
        float cinemaAspect, float wideAspect, float autoAspect)
    {
        switch (policy) {
        case config::CinematicAspectPolicy::Forced16x9: return nativeAspect;
        case config::CinematicAspectPolicy::Forced21x9: return cinemaAspect;
        case config::CinematicAspectPolicy::Forced32x9: return wideAspect;
        case config::CinematicAspectPolicy::Auto: return autoAspect;
        case config::CinematicAspectPolicy::Native: return nativeAspect;
        }
        return nativeAspect;
    }
    float ResolveAspect(config::CinematicAspectPolicy policy, float nativeAspect,
        float cinemaAspect, float wideAspect, AutoAspectResolver autoResolver);

    struct AspectStoreResolution
    {
        std::uint8_t* store{};
        std::size_t matches{};
        bool imageValid{};
    };

    AspectStoreResolution ResolveAspectStore(void* executable, const char* signature,
        std::uintptr_t aspectOffset, const std::uint8_t* storePrefix,
        std::size_t storePrefixSize, const std::uint8_t* expectedImmediate,
        std::size_t immediateOffset, std::size_t immediateSize,
        std::size_t storeInstructionLength, std::uint32_t expectedImmediateValue);

    struct AspectStoreApplication
    {
        float aspect{};
        bool writable{};
    };

    AspectStoreApplication ApplyAspectStore(std::uintptr_t targetObject,
        float resolvedAspect, float nativeAspect, std::uintptr_t aspectOffset,
        WritablePredicate isWritable, AspectValidator isValidAspect);
}
