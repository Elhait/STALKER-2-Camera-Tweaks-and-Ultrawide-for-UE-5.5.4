#include "cinematic_continuation_trace.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>

namespace diagnostics
{
    namespace
    {
        int CompareToTarget(float value, float target) noexcept
        {
            if (value < target) return -1;
            if (value > target) return 1;
            return 0;
        }

        ContinuationTraceDirection DirectionForDelta(float delta) noexcept
        {
            if (delta < 0.0f) return ContinuationTraceDirection::Down;
            if (delta > 0.0f) return ContinuationTraceDirection::Up;
            return ContinuationTraceDirection::Flat;
        }
    }

    void CinematicContinuationTrace::Begin(bool diagnosticMode,
        std::uint64_t generation, std::uintptr_t expectedSource,
        float continuationStartFov, float cinematicTargetFov) noexcept
    {
        std::lock_guard lock(mutex_);
        ResetLocked();
        if (!diagnosticMode || generation == 0) return;
        state_.active = true;
        state_.generation = generation;
        state_.expectedSource = expectedSource;
        state_.continuationStartFov = continuationStartFov;
        state_.cinematicTargetFov = cinematicTargetFov;
    }

    bool CinematicContinuationTrace::Record(bool diagnosticMode,
        const ContinuationTraceInput& input) noexcept
    {
        std::lock_guard lock(mutex_);
        if (!diagnosticMode) {
            ResetLocked();
            return false;
        }
        if (!state_.active) return false;

        ContinuationTraceSample sample{};
        sample.sequence = ++state_.totalSamples;
        sample.generation = state_.generation;
        sample.source = input.source;
        sample.expectedSource = state_.expectedSource;
        sample.rawInputFov = input.rawInputFov;
        sample.outputFov = input.outputFov;
        sample.continuationStartFov = input.continuationStartFov;
        sample.cinematicTargetFov = input.cinematicTargetFov;
        sample.productionAction = input.productionAction;
        sample.distanceToTarget = std::isfinite(input.rawInputFov) &&
                std::isfinite(input.cinematicTargetFov)
            ? std::fabs(input.rawInputFov - input.cinematicTargetFov)
            : std::numeric_limits<float>::quiet_NaN();
        sample.sourceMatch = input.sourceMatch;
        sample.inProductionRange = input.inProductionRange;
        sample.inSemanticTargetRegion = input.inSemanticTargetRegion;

        if (!input.sourceMatch) {
            sample.events = sample.events | ContinuationTraceEvent::SourceMismatch;
            previousRawValid_ = false;
            previousSourceMatched_ = false;
            previousTargetRegion_ = false;
            previousNonFlatDirection_ = ContinuationTraceDirection::Unknown;
            previousTargetSide_ = 0;
        } else if (std::isfinite(input.rawInputFov)) {
            if (previousRawValid_ && previousSourceMatched_) {
                const auto direction = DirectionForDelta(input.rawInputFov - previousRawFov_);
                sample.direction = direction;
                if (direction != ContinuationTraceDirection::Flat &&
                    previousNonFlatDirection_ != ContinuationTraceDirection::Unknown &&
                    direction != previousNonFlatDirection_) {
                    sample.events = sample.events | ContinuationTraceEvent::DirectionReversal;
                }
                if (direction != ContinuationTraceDirection::Flat)
                    previousNonFlatDirection_ = direction;
            }

            if (input.inSemanticTargetRegion && !hasEnteredTargetRegion_) {
                sample.events = sample.events | ContinuationTraceEvent::FirstTargetRegionEntry;
                hasEnteredTargetRegion_ = true;
            } else if (hasEnteredTargetRegion_ && previousTargetRegion_ &&
                !input.inSemanticTargetRegion) {
                sample.events = sample.events | ContinuationTraceEvent::TargetRegionExit;
            }

            if (std::isfinite(input.cinematicTargetFov)) {
                const int targetSide = CompareToTarget(input.rawInputFov,
                    input.cinematicTargetFov);
                const bool continuationMovesUp =
                    input.cinematicTargetFov > input.continuationStartFov;
                const bool continuationMovesDown =
                    input.cinematicTargetFov < input.continuationStartFov;
                if ((continuationMovesUp && input.rawInputFov > input.cinematicTargetFov) ||
                    (continuationMovesDown && input.rawInputFov < input.cinematicTargetFov))
                    sample.events = sample.events | ContinuationTraceEvent::TargetOvershoot;
                if (previousRawValid_ && previousSourceMatched_ &&
                    previousTargetSide_ != 0 && previousTargetSide_ != targetSide) {
                    sample.events = sample.events | ContinuationTraceEvent::TargetCrossing;
                }
                previousTargetSide_ = targetSide;
            }

            previousRawFov_ = input.rawInputFov;
            previousRawValid_ = true;
            previousSourceMatched_ = true;
            previousTargetRegion_ = input.inSemanticTargetRegion;
        } else {
            previousRawValid_ = false;
            previousSourceMatched_ = false;
            previousTargetRegion_ = false;
            previousNonFlatDirection_ = ContinuationTraceDirection::Unknown;
            previousTargetSide_ = 0;
        }

        if (state_.storedSamples < state_.samples.size()) {
            state_.samples[state_.storedSamples++] = sample;
        } else {
            state_.overflow = true;
        }
        return true;
    }

    ContinuationTraceSnapshot CinematicContinuationTrace::Finish(
        bool diagnosticMode, std::string_view clearReason) noexcept
    {
        std::lock_guard lock(mutex_);
        if (!diagnosticMode || !state_.active) {
            ResetLocked();
            return {};
        }
        auto snapshot = state_;
        const auto copyLength = (std::min)(clearReason.size(),
            snapshot.clearReason.size() - 1);
        std::memcpy(snapshot.clearReason.data(), clearReason.data(), copyLength);
        snapshot.clearReason[copyLength] = '\0';
        ResetLocked();
        return snapshot;
    }

    void CinematicContinuationTrace::Discard() noexcept
    {
        std::lock_guard lock(mutex_);
        ResetLocked();
    }

    void CinematicContinuationTrace::ResetLocked() noexcept
    {
        state_ = {};
        previousRawValid_ = false;
        previousSourceMatched_ = false;
        previousRawFov_ = 0.0f;
        hasEnteredTargetRegion_ = false;
        previousTargetRegion_ = false;
        previousNonFlatDirection_ = ContinuationTraceDirection::Unknown;
        previousTargetSide_ = 0;
    }

    const char* ContinuationTraceDirectionName(
        ContinuationTraceDirection direction) noexcept
    {
        switch (direction) {
        case ContinuationTraceDirection::Unknown: return "unknown";
        case ContinuationTraceDirection::Down: return "down";
        case ContinuationTraceDirection::Flat: return "flat";
        case ContinuationTraceDirection::Up: return "up";
        }
        return "unknown";
    }

    std::string_view ContinuationTraceEventNames(ContinuationTraceEvent events,
        char* buffer, std::size_t bufferSize) noexcept
    {
        if (buffer == nullptr || bufferSize == 0) return {};
        buffer[0] = '\0';
        const auto append = [&](const char* name) {
            const auto used = std::strlen(buffer);
            if (used >= bufferSize - 1) return;
            const auto separator = used == 0 ? "" : ",";
            std::snprintf(buffer + used, bufferSize - used, "%s%s", separator, name);
        };
        if (HasContinuationTraceEvent(events, ContinuationTraceEvent::FirstTargetRegionEntry))
            append("first_target_region_entry");
        if (HasContinuationTraceEvent(events, ContinuationTraceEvent::TargetRegionExit))
            append("target_region_exit");
        if (HasContinuationTraceEvent(events, ContinuationTraceEvent::DirectionReversal))
            append("direction_reversal");
        if (HasContinuationTraceEvent(events, ContinuationTraceEvent::TargetOvershoot))
            append("target_overshoot");
        if (HasContinuationTraceEvent(events, ContinuationTraceEvent::TargetCrossing))
            append("target_crossing");
        if (HasContinuationTraceEvent(events, ContinuationTraceEvent::SourceMismatch))
            append("source_mismatch");
        if (buffer[0] == '\0') return "none";
        return { buffer, std::strlen(buffer) };
    }
}
