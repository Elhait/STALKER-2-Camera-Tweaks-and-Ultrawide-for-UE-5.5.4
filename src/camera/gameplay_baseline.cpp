#include "gameplay_baseline.hpp"
#include "../diagnostics/performance_telemetry.hpp"

#include <cmath>

namespace camera
{
    const char* GameplayBaselineProjectionDispositionName(
        GameplayBaselineProjectionDisposition value) noexcept
    {
        switch (value) {
        case GameplayBaselineProjectionDisposition::Updated: return "UPDATE";
        case GameplayBaselineProjectionDisposition::Retained: return "RETAIN";
        case GameplayBaselineProjectionDisposition::Invalidated: return "INVALIDATE";
        }
        return "RETAIN";
    }

    bool IsUsableGameplayBaseline(const GameplayBaseline& baseline) noexcept
    {
        return baseline.valid && baseline.nativeFov.valid &&
            baseline.nativeFov.space == FovSpace::Native &&
            baseline.nativeFov.provenance == FovProvenance::NativeRegisterInput &&
            std::isfinite(baseline.nativeFov.value) && baseline.nativeFov.value > 1.0f &&
            baseline.nativeFov.value < 179.0f && std::isfinite(baseline.aspect) &&
            baseline.aspect > 0.0f;
    }

    bool GameplayBaselineStore::IsEligibleObservation(
        const CameraFovObservation& observation, bool gameplayEligible) noexcept
    {
        const bool validNativeInput = observation.inputFov.valid &&
            observation.inputFov.space == FovSpace::Native &&
            observation.inputFov.provenance == FovProvenance::NativeRegisterInput &&
            std::isfinite(observation.inputFov.value);
        const bool validTransformedResult = observation.resultFov.valid &&
            observation.resultFov.space == FovSpace::Transformed &&
            observation.resultFov.provenance == FovProvenance::ModTransformResult &&
            std::isfinite(observation.resultFov.value);
        const bool validPassThroughResult = observation.resultFov.valid &&
            observation.resultFov.space == FovSpace::Native &&
            observation.resultFov.provenance == FovProvenance::PassThroughResult &&
            std::isfinite(observation.resultFov.value);
        return gameplayEligible &&
            observation.boundary == FovObservationBoundary::CameraWriter &&
            validNativeInput && (validTransformedResult || validPassThroughResult) &&
            observation.aspect.valid &&
            std::isfinite(observation.aspect.value) &&
            observation.writerSource.valid &&
            observation.publicationSequence != 0;
    }

    GameplayBaselineProjectionResult GameplayBaselineStore::Project(
        const CameraFovObservation& observation, bool gameplayEligible)
    {
        diagnostics::ScopedRuntimeMutex lock(mutex_, diagnostics::RuntimeLockPath::GameplayBaseline);
        GameplayBaselineProjectionResult result{};
        result.baseline = baseline_;
        result.eligible = IsEligibleObservation(observation, gameplayEligible);
        if (!result.eligible) return result;

        baseline_.nativeFov = observation.inputFov;
        baseline_.horPlusFov = observation.resultFov.space == FovSpace::Transformed
            ? observation.resultFov : FovSample{};
        baseline_.aspect = observation.aspect.value;
        baseline_.source = observation.writerSource;
        baseline_.observationSequence = observation.publicationSequence;
        baseline_.valid = true;
        result.disposition = GameplayBaselineProjectionDisposition::Updated;
        result.baseline = baseline_;
        return result;
    }

    GameplayBaselineProjectionResult GameplayBaselineStore::Invalidate()
    {
        diagnostics::ScopedRuntimeMutex lock(mutex_, diagnostics::RuntimeLockPath::GameplayBaseline);
        baseline_ = {};
        GameplayBaselineProjectionResult result{};
        result.disposition = GameplayBaselineProjectionDisposition::Invalidated;
        result.baseline = baseline_;
        return result;
    }

    GameplayBaseline GameplayBaselineStore::Read() const
    {
        diagnostics::ScopedRuntimeMutex lock(mutex_, diagnostics::RuntimeLockPath::GameplayBaseline);
        return baseline_;
    }
}
