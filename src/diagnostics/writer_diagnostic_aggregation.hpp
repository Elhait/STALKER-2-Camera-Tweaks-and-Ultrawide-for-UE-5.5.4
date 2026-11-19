#pragma once

#include <cmath>
#include <cstdint>
#include <limits>

namespace diagnostics
{
    struct WriterDiagnosticSummary
    {
        bool active{};
        std::uint64_t generation{};
        std::uintptr_t source{};
        std::uint8_t lifecycle{};
        std::uint64_t outputTransitions{};
        std::uint64_t samples{};
        std::uint64_t applied{};
        std::uint64_t fullRecords{};
        std::uintptr_t firstOutput{};
        std::uintptr_t lastOutput{};
        float firstInput{std::numeric_limits<float>::quiet_NaN()};
        float lastInput{std::numeric_limits<float>::quiet_NaN()};
        float lastResult{std::numeric_limits<float>::quiet_NaN()};
        float minimumInput{std::numeric_limits<float>::quiet_NaN()};
        float maximumInput{std::numeric_limits<float>::quiet_NaN()};
        float minimumResult{std::numeric_limits<float>::quiet_NaN()};
        float maximumResult{std::numeric_limits<float>::quiet_NaN()};
        bool HasRecords() const noexcept { return outputTransitions || samples; }
    };

    // Observational, fixed-size state. Runtime serializes it with the existing
    // correction gate; callers skip it entirely when diagnostics are disabled.
    // No output-address cache or numeric delta controls emission.
    class WriterDiagnosticAggregation
    {
    public:
        bool Matches(std::uint64_t generation, std::uintptr_t source,
            std::uint8_t lifecycle) const noexcept
        {
            return state_.active && state_.generation == generation &&
                state_.source == source && state_.lifecycle == lifecycle;
        }
        void Begin(std::uint64_t generation, std::uintptr_t source,
            std::uint8_t lifecycle) noexcept
        {
            state_ = {};
            state_.active = true;
            state_.generation = generation;
            state_.source = source;
            state_.lifecycle = lifecycle;
        }
        void RecordOutputTransition(std::uintptr_t from, std::uintptr_t to) noexcept
        {
            if (!state_.outputTransitions && !state_.samples) state_.firstOutput = from;
            ++state_.outputTransitions;
            state_.lastOutput = to;
        }
        void RecordSample(std::uintptr_t output, float input, float result,
            bool applied, bool fullRecord) noexcept
        {
            if (!state_.samples) {
                state_.firstInput = input;
                if (!state_.outputTransitions) state_.firstOutput = output;
            }
            ++state_.samples;
            state_.applied += applied ? 1 : 0;
            state_.fullRecords += fullRecord ? 1 : 0;
            state_.lastOutput = output;
            state_.lastInput = input;
            state_.lastResult = result;
            ObserveRange(input, state_.minimumInput, state_.maximumInput);
            ObserveRange(result, state_.minimumResult, state_.maximumResult);
        }
        WriterDiagnosticSummary Finish() noexcept
        {
            const auto result = state_;
            state_ = {};
            return result;
        }
    private:
        static void ObserveRange(float value, float& minimum, float& maximum) noexcept
        {
            if (!std::isfinite(value)) return;
            if (!std::isfinite(minimum) || value < minimum) minimum = value;
            if (!std::isfinite(maximum) || value > maximum) maximum = value;
        }
        WriterDiagnosticSummary state_{};
    };
}
