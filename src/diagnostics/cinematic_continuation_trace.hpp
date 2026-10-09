#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string_view>

namespace diagnostics
{
    enum class ContinuationTraceEvent : std::uint16_t
    {
        None = 0,
        FirstTargetRegionEntry = 1 << 0,
        TargetRegionExit = 1 << 1,
        DirectionReversal = 1 << 2,
        TargetOvershoot = 1 << 3,
        TargetCrossing = 1 << 4,
        SourceMismatch = 1 << 5,
    };

    constexpr ContinuationTraceEvent operator|(ContinuationTraceEvent left,
        ContinuationTraceEvent right) noexcept
    {
        return static_cast<ContinuationTraceEvent>(
            static_cast<std::uint16_t>(left) | static_cast<std::uint16_t>(right));
    }

    constexpr bool HasContinuationTraceEvent(ContinuationTraceEvent value,
        ContinuationTraceEvent event) noexcept
    {
        return (static_cast<std::uint16_t>(value) &
            static_cast<std::uint16_t>(event)) != 0;
    }

    enum class ContinuationTraceDirection : std::int8_t
    {
        Unknown = 0,
        Down = -1,
        Flat = 1,
        Up = 2,
    };

    struct ContinuationTraceInput
    {
        std::uintptr_t source{};
        bool sourceMatch{};
        float rawInputFov{};
        float outputFov{};
        float continuationStartFov{};
        float cinematicTargetFov{};
        std::uint8_t productionAction{};
        bool inProductionRange{};
        bool inSemanticTargetRegion{};
    };

    struct ContinuationTraceSample
    {
        std::uint64_t sequence{};
        std::uint64_t generation{};
        std::uintptr_t source{};
        std::uintptr_t expectedSource{};
        float rawInputFov{};
        float outputFov{};
        float continuationStartFov{};
        float cinematicTargetFov{};
        float distanceToTarget{};
        std::uint8_t productionAction{};
        ContinuationTraceDirection direction{ContinuationTraceDirection::Unknown};
        bool sourceMatch{};
        bool inProductionRange{};
        bool inSemanticTargetRegion{};
        ContinuationTraceEvent events{ContinuationTraceEvent::None};
    };

    inline constexpr std::size_t kContinuationTraceCapacity = 512;

    struct ContinuationTraceSnapshot
    {
        std::array<ContinuationTraceSample, kContinuationTraceCapacity> samples{};
        std::array<char, 48> clearReason{};
        std::uint64_t generation{};
        std::uint64_t totalSamples{};
        std::size_t storedSamples{};
        std::uintptr_t expectedSource{};
        float continuationStartFov{};
        float cinematicTargetFov{};
        bool active{};
        bool overflow{};
    };

    class CinematicContinuationTrace
    {
    public:
        void Begin(bool diagnosticMode, std::uint64_t generation,
            std::uintptr_t expectedSource, float continuationStartFov,
            float cinematicTargetFov) noexcept;
        bool Record(bool diagnosticMode, const ContinuationTraceInput& input) noexcept;
        ContinuationTraceSnapshot Finish(bool diagnosticMode,
            std::string_view clearReason) noexcept;
        void Discard() noexcept;

    private:
        void ResetLocked() noexcept;

        std::mutex mutex_;
        ContinuationTraceSnapshot state_{};
        bool previousRawValid_{};
        bool previousSourceMatched_{};
        float previousRawFov_{};
        bool hasEnteredTargetRegion_{};
        bool previousTargetRegion_{};
        ContinuationTraceDirection previousNonFlatDirection_{
            ContinuationTraceDirection::Unknown};
        int previousTargetSide_{};
    };

    const char* ContinuationTraceDirectionName(
        ContinuationTraceDirection direction) noexcept;
    std::string_view ContinuationTraceEventNames(
        ContinuationTraceEvent events, char* buffer, std::size_t bufferSize) noexcept;
}
