#pragma once

#include <cstdint>
#include <atomic>
#include <limits>

#include "../config/feature_config.hpp"

namespace dialogue
{
    enum class Phase : std::uint32_t
    {
        Inactive,
        Candidate,
        Active,
        Exiting,
        RearmPending,
    };

    const char* PhaseName(Phase phase);
    bool CandidatePromotionAllowed(config::DialogueZoomPolicy selectedPolicy,
        bool coordinatorGameplay, bool postCinematicExclusionInactive) noexcept;

    enum class CandidateDecision : std::uint32_t
    {
        Pending,
        Activate,
        Cancel,
    };

    class CandidateTracker
    {
    public:
        void Reset() noexcept;
        void Begin(float initialSample) noexcept;
        void Begin(float initialSample, std::uintptr_t source,
            float nativeTarget) noexcept;
        CandidateDecision Observe(float sample, float reversalEpsilon,
            float stabilityEpsilon, std::uint32_t stableSampleLimit = 3) noexcept;
        CandidateDecision Observe(float sample, std::uintptr_t source,
            float nativeTarget,
            float reversalEpsilon, float stabilityEpsilon,
            std::uint32_t stableSampleLimit = 3) noexcept;
        bool IsPending() const noexcept;
        float Baseline() const noexcept;
        bool HasContext() const noexcept;

    private:
        bool pending_{};
        std::uintptr_t source_{};
        float nativeTarget_{std::numeric_limits<float>::quiet_NaN()};
        float baseline_{std::numeric_limits<float>::quiet_NaN()};
        float previous_{std::numeric_limits<float>::quiet_NaN()};
        float cumulativeDescent_{};
        std::uint32_t stableSamples_{};
    };

    class PolicySnapshot
    {
    public:
        void Clear() noexcept;
        void Capture(config::DialogueZoomPolicy policy) noexcept;
        bool IsValid() const noexcept;
        config::DialogueZoomPolicy Value() const noexcept;

    private:
        config::DialogueZoomPolicy value_{config::DialogueZoomPolicy::Native};
        bool valid_{};
    };

    // Correction activation is captured only at an accepted dialogue start.
    // Disabling takes effect immediately; enabling never retroactively arms
    // an already active lifecycle.
    class CorrectionActivation
    {
    public:
        void Reset() noexcept;
        void AcceptedStart(bool moduleEnabled) noexcept;
        void ModuleEnabledChanged(bool enabled) noexcept;
        bool CanCorrect(bool moduleEnabled) const noexcept;

    private:
        std::atomic<bool> armed_{};
    };

    class RecoveryRearm
    {
    public:
        enum class Decision : std::uint32_t
        {
            Pending,
            Complete,
            Cancel,
        };

        void Reset() noexcept;
        void Begin(std::uintptr_t source, float initialSample, float nativeTarget) noexcept;
        bool IsPending() const noexcept;
        std::uint32_t StableSamples() const noexcept;
        Decision Observe(std::uintptr_t source, float sample, float nativeTarget,
            float endpointToleranceDegrees,
            std::uint32_t requiredStableSamples = 2) noexcept;

    private:
        bool pending_{};
        std::uintptr_t source_{};
        float previous_{std::numeric_limits<float>::quiet_NaN()};
        float nativeTarget_{std::numeric_limits<float>::quiet_NaN()};
        std::uint32_t stableSamples_{};
    };

    class PostCinematicRecoveryExclusion
    {
    public:
        enum class Observation : std::uint8_t
        {
            Inactive,
            Suppressed,
            Recovered,
            Cancelled,
        };

        void Arm(float targetFov) noexcept;
        bool Reset() noexcept;
        bool IsActive() const noexcept;
        Observation ObserveValidated(std::uintptr_t source, float currentFov,
            float endpointToleranceDegrees) noexcept;
        bool Observe(std::uintptr_t source, float currentFov,
            float endpointToleranceDegrees) noexcept;

    private:
        std::atomic<bool> active_{false};
        std::atomic<bool> recoveryStarted_{false};
        std::atomic<std::uintptr_t> source_{};
        std::atomic<float> targetFov_{std::numeric_limits<float>::quiet_NaN()};
    };
}
