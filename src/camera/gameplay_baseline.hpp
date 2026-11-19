#pragma once

#include "fov_observation.hpp"

#include <cstdint>
#include <limits>
#include <mutex>

namespace camera
{
    struct GameplayBaseline
    {
        FovSample nativeFov{};
        FovSample horPlusFov{};
        float aspect{std::numeric_limits<float>::quiet_NaN()};
        FovWriterSourceToken source{};
        std::uint64_t observationSequence{};
        bool valid{};
        bool current{true};
    };

    enum class GameplayBaselineProjectionDisposition : std::uint8_t
    {
        Updated,
        Retained,
        Invalidated,
    };

    struct GameplayBaselineProjectionResult
    {
        GameplayBaselineProjectionDisposition disposition{
            GameplayBaselineProjectionDisposition::Retained};
        GameplayBaseline baseline{};
        bool eligible{};
    };

    const char* GameplayBaselineProjectionDispositionName(
        GameplayBaselineProjectionDisposition value) noexcept;
    bool IsUsableGameplayBaseline(const GameplayBaseline& baseline) noexcept;

    class GameplayBaselineStore
    {
    public:
        GameplayBaselineProjectionResult Project(
            const CameraFovObservation& observation, bool gameplayEligible);
        GameplayBaselineProjectionResult Invalidate();
        GameplayBaseline Read() const;

    private:
        static bool IsEligibleObservation(const CameraFovObservation& observation,
            bool gameplayEligible) noexcept;

        mutable std::mutex mutex_;
        GameplayBaseline baseline_{};
    };
}
