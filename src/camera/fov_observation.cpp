#include "fov_observation.hpp"
#include "../diagnostics/performance_telemetry.hpp"

namespace camera
{
    const char* FovObservationBoundaryName(FovObservationBoundary value) noexcept
    {
        switch (value) {
        case FovObservationBoundary::CameraWriter: return "CameraWriter";
        case FovObservationBoundary::CinematicEnter: return "CinematicEnter";
        }
        return "CameraWriter";
    }

    const char* FovSpaceName(FovSpace value) noexcept
    {
        switch (value) {
        case FovSpace::Unknown: return "Unknown";
        case FovSpace::Native: return "Native";
        case FovSpace::Transformed: return "Transformed";
        }
        return "Unknown";
    }

    const char* FovProvenanceName(FovProvenance value) noexcept
    {
        switch (value) {
        case FovProvenance::Unavailable: return "Unavailable";
        case FovProvenance::NativeRegisterInput: return "NativeRegisterInput";
        case FovProvenance::NativeObjectField: return "NativeObjectField";
        case FovProvenance::ModTransformResult: return "ModTransformResult";
        case FovProvenance::PassThroughResult: return "PassThroughResult";
        case FovProvenance::CachedCinematicEnter: return "CachedCinematicEnter";
        case FovProvenance::ResolvedAspect: return "ResolvedAspect";
        }
        return "Unavailable";
    }

    std::size_t CameraFovObservationStore::Index(FovObservationBoundary boundary) noexcept
    {
        return static_cast<std::size_t>(boundary);
    }

    CameraFovObservation CameraFovObservationStore::Publish(CameraFovObservation observation)
    {
        diagnostics::ScopedRuntimeMutex lock(mutex_, diagnostics::RuntimeLockPath::FovObservation);
        observation.publicationSequence = ++nextSequence_;
        const auto index = Index(observation.boundary);
        latest_[index] = observation;
        valid_[index] = true;
        return observation;
    }

    std::optional<CameraFovObservation> CameraFovObservationStore::ReadLatest(
        FovObservationBoundary boundary) const
    {
        diagnostics::ScopedRuntimeMutex lock(mutex_, diagnostics::RuntimeLockPath::FovObservation);
        const auto index = Index(boundary);
        if (!valid_[index]) return std::nullopt;
        return latest_[index];
    }
}
