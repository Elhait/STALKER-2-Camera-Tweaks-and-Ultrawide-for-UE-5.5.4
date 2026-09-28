#include "dialogue_state.hpp"

#include <cmath>

namespace dialogue
{
    const char* PhaseName(Phase phase)
    {
        switch (phase) {
        case Phase::Inactive: return "Inactive";
        case Phase::Candidate: return "Candidate";
        case Phase::Active: return "Active";
        case Phase::Exiting: return "Exiting";
        case Phase::RearmPending: return "RearmPending";
        }
        return "Unknown";
    }

    bool CandidatePromotionAllowed(config::DialogueZoomPolicy selectedPolicy,
        bool coordinatorGameplay, bool postCinematicExclusionInactive) noexcept
    {
        return selectedPolicy != config::DialogueZoomPolicy::Native &&
            coordinatorGameplay && postCinematicExclusionInactive;
    }

    void CandidateTracker::Reset() noexcept
    {
        pending_ = false;
        source_ = 0;
        nativeTarget_ = std::numeric_limits<float>::quiet_NaN();
        baseline_ = std::numeric_limits<float>::quiet_NaN();
        previous_ = std::numeric_limits<float>::quiet_NaN();
        cumulativeDescent_ = 0.0f;
        stableSamples_ = 0;
    }

    void CandidateTracker::Begin(float initialSample) noexcept
    {
        pending_ = std::isfinite(initialSample);
        source_ = 0;
        nativeTarget_ = std::numeric_limits<float>::quiet_NaN();
        baseline_ = initialSample;
        previous_ = initialSample;
        cumulativeDescent_ = 0.0f;
        stableSamples_ = 0;
    }

    void CandidateTracker::Begin(float initialSample, std::uintptr_t source,
        float nativeTarget) noexcept
    {
        pending_ = source != 0 && std::isfinite(initialSample) &&
            std::isfinite(nativeTarget);
        source_ = source;
        nativeTarget_ = nativeTarget;
        baseline_ = initialSample;
        previous_ = initialSample;
        cumulativeDescent_ = 0.0f;
        stableSamples_ = 0;
    }

    CandidateDecision CandidateTracker::Observe(float sample, float reversalEpsilon,
        float stabilityEpsilon, std::uint32_t stableSampleLimit) noexcept
    {
        if (!pending_ || !std::isfinite(sample) || !std::isfinite(previous_) ||
            !std::isfinite(reversalEpsilon) || !std::isfinite(stabilityEpsilon) ||
            reversalEpsilon < 0.0f || stabilityEpsilon < 0.0f ||
            stableSampleLimit == 0) {
            Reset();
            return CandidateDecision::Cancel;
        }

        const float delta = sample - previous_;
        if (delta > reversalEpsilon) {
            Reset();
            return CandidateDecision::Cancel;
        }
        if (delta < 0.0f) {
            cumulativeDescent_ += -delta;
            stableSamples_ = 0;
        } else if (std::fabs(delta) <= stabilityEpsilon) {
            ++stableSamples_;
        } else {
            stableSamples_ = 0;
        }
        previous_ = sample;

        if (cumulativeDescent_ > reversalEpsilon) {
            pending_ = false;
            return CandidateDecision::Activate;
        }
        if (stableSamples_ >= stableSampleLimit) {
            Reset();
            return CandidateDecision::Cancel;
        }
        return CandidateDecision::Pending;
    }

    CandidateDecision CandidateTracker::Observe(float sample, std::uintptr_t source,
        float nativeTarget, float sourceTargetEpsilon, float reversalEpsilon,
        float stabilityEpsilon, std::uint32_t stableSampleLimit) noexcept
    {
        if (!pending_ || source_ == 0 || source != source_ ||
            !std::isfinite(nativeTarget_) || !std::isfinite(nativeTarget) ||
            !std::isfinite(sourceTargetEpsilon) || sourceTargetEpsilon < 0.0f ||
            std::fabs(nativeTarget - nativeTarget_) > sourceTargetEpsilon) {
            Reset();
            return CandidateDecision::Cancel;
        }
        return Observe(sample, reversalEpsilon, stabilityEpsilon, stableSampleLimit);
    }

    bool CandidateTracker::IsPending() const noexcept
    {
        return pending_;
    }

    float CandidateTracker::Baseline() const noexcept
    {
        return baseline_;
    }

    bool CandidateTracker::HasContext() const noexcept
    {
        return pending_ && source_ != 0 && std::isfinite(nativeTarget_);
    }

    void PolicySnapshot::Clear() noexcept
    {
        value_ = config::DialogueZoomPolicy::Native;
        valid_ = false;
    }

    void PolicySnapshot::Capture(config::DialogueZoomPolicy policy) noexcept
    {
        value_ = policy;
        valid_ = true;
    }

    bool PolicySnapshot::IsValid() const noexcept
    {
        return valid_;
    }

    config::DialogueZoomPolicy PolicySnapshot::Value() const noexcept
    {
        return value_;
    }

    void RecoveryRearm::Reset() noexcept
    {
        pending_ = false;
        source_ = 0;
        previous_ = std::numeric_limits<float>::quiet_NaN();
        nativeTarget_ = std::numeric_limits<float>::quiet_NaN();
        stableSamples_ = 0;
    }

    void RecoveryRearm::Begin(std::uintptr_t source, float initialSample,
        float nativeTarget) noexcept
    {
        pending_ = source != 0 && std::isfinite(initialSample);
        source_ = source;
        previous_ = initialSample;
        nativeTarget_ = nativeTarget;
        stableSamples_ = 0;
    }

    bool RecoveryRearm::IsPending() const noexcept
    {
        return pending_;
    }

    std::uint32_t RecoveryRearm::StableSamples() const noexcept
    {
        return stableSamples_;
    }

    RecoveryRearm::Decision RecoveryRearm::Observe(std::uintptr_t source, float sample,
        float nativeTarget, float targetEpsilon, float stabilityEpsilon,
        std::uint32_t requiredStableSamples) noexcept
    {
        if (!pending_) return Decision::Cancel;
        if (source == 0 || source != source_) {
            Reset();
            return Decision::Cancel;
        }
        if (!std::isfinite(sample) || !std::isfinite(nativeTarget) ||
            !std::isfinite(previous_) || !std::isfinite(targetEpsilon) ||
            !std::isfinite(stabilityEpsilon) || targetEpsilon < 0.0f ||
            stabilityEpsilon < 0.0f || requiredStableSamples == 0) {
            // An unavailable target is not evidence of completion. Keep the
            // pending lifecycle fail-closed until a valid native sample arrives.
            return Decision::Pending;
        }
        if (!std::isfinite(nativeTarget_)) {
            nativeTarget_ = nativeTarget;
        } else if (std::fabs(nativeTarget - nativeTarget_) > stabilityEpsilon) {
            nativeTarget_ = nativeTarget;
            stableSamples_ = 0;
        }

        if (std::fabs(sample - nativeTarget_) <= targetEpsilon) {
            Reset();
            return Decision::Complete;
        }
        previous_ = sample;
        // Completion is intentionally target-based and occurs above on the
        // first valid converged sample. Keep the counter for diagnostics only.
        stableSamples_ = 0;
        return Decision::Pending;
    }

    void PostCinematicRecoveryExclusion::Arm(float targetFov) noexcept
    {
        recoveryStarted_.store(false, std::memory_order_release);
        source_.store(0, std::memory_order_release);
        targetFov_.store(targetFov, std::memory_order_release);
        active_.store(true, std::memory_order_release);
    }

    bool PostCinematicRecoveryExclusion::Reset() noexcept
    {
        const bool wasActive = active_.exchange(false, std::memory_order_acq_rel);
        recoveryStarted_.store(false, std::memory_order_release);
        source_.store(0, std::memory_order_release);
        targetFov_.store(std::numeric_limits<float>::quiet_NaN(), std::memory_order_release);
        return wasActive;
    }

    bool PostCinematicRecoveryExclusion::IsActive() const noexcept
    {
        return active_.load(std::memory_order_acquire);
    }

    PostCinematicRecoveryExclusion::Observation
    PostCinematicRecoveryExclusion::ObserveValidated(
        std::uintptr_t source, float currentFov, float epsilon) noexcept
    {
        if (!IsActive()) return Observation::Inactive;

        const float targetFov = targetFov_.load(std::memory_order_acquire);
        if (!source || !std::isfinite(currentFov) || !std::isfinite(targetFov) ||
            !std::isfinite(epsilon) || epsilon < 0.0f) {
            Reset();
            return Observation::Cancelled;
        }

        auto expectedSource = source_.load(std::memory_order_acquire);
        if (expectedSource == 0) {
            source_.compare_exchange_strong(expectedSource, source,
                std::memory_order_acq_rel);
            expectedSource = source_.load(std::memory_order_acquire);
        }
        if (expectedSource != source) {
            Reset();
            return Observation::Cancelled;
        }

        const float distanceFromTarget = std::fabs(currentFov - targetFov);
        if (!recoveryStarted_.load(std::memory_order_acquire)) {
            if (distanceFromTarget > epsilon)
                recoveryStarted_.store(true, std::memory_order_release);
            return Observation::Suppressed;
        }
        if (distanceFromTarget <= epsilon) {
            Reset();
            return Observation::Recovered;
        }
        return Observation::Suppressed;
    }

    bool PostCinematicRecoveryExclusion::Observe(
        std::uintptr_t source, float currentFov, float epsilon) noexcept
    {
        return ObserveValidated(source, currentFov, epsilon) == Observation::Suppressed;
    }
}
