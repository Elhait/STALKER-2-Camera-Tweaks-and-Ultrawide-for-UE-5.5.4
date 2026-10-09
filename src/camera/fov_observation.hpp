#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>

namespace camera
{
    enum class FovObservationBoundary : std::uint8_t
    {
        CameraWriter,
        CinematicEnter,
    };

    enum class FovSpace : std::uint8_t
    {
        Unknown,
        Native,
        Transformed,
    };

    enum class FovProvenance : std::uint8_t
    {
        Unavailable,
        NativeRegisterInput,
        NativeObjectField,
        ModTransformResult,
        PassThroughResult,
        CachedCinematicEnter,
        ResolvedAspect,
        NativeWriterAspect,
        ClientViewportAspect,
    };

    struct FovSample
    {
        float value{std::numeric_limits<float>::quiet_NaN()};
        FovSpace space{FovSpace::Unknown};
        FovProvenance provenance{FovProvenance::Unavailable};
        bool valid{};
    };

    struct ScalarSample
    {
        float value{std::numeric_limits<float>::quiet_NaN()};
        FovProvenance provenance{FovProvenance::Unavailable};
        bool valid{};
    };

    struct FovWriterSourceToken
    {
        std::uintptr_t value{};
        bool valid{};
    };

    struct CameraFovObservation
    {
        FovObservationBoundary boundary{FovObservationBoundary::CameraWriter};
        FovSample inputFov{};
        FovSample resultFov{};
        ScalarSample aspect{};
        FovWriterSourceToken writerSource{};
        std::uint8_t writerFlags{};
        bool writerFlagsValid{};
        std::uint64_t publicationSequence{};
        ScalarSample policyAspect{};
        ScalarSample currentViewportAspect{};
        bool observedDuringGameplay{};
    };

    // The two production observations captured on one accepted camera
    // boundary. Gameplay is sampled immediately before ENTER; Cinematic is
    // sampled after the ENTER transaction has produced its actual output.
    struct CameraTransitionBoundaryObservation
    {
        CameraFovObservation gameplay{};
        CameraFovObservation cinematic{};
        std::uint64_t lifecycleGeneration{};
        bool gameplayValid{};
        bool cinematicValid{};
        bool accepted{};
    };

    inline CameraTransitionBoundaryObservation BeginCameraTransitionBoundary(
        const CameraTransitionBoundaryObservation& previous,
        std::uint64_t lifecycleGeneration,
        const std::optional<CameraFovObservation>& gameplayObservation,
        bool continuedOwnership) noexcept
    {
        CameraTransitionBoundaryObservation next{};
        next.lifecycleGeneration = lifecycleGeneration;
        next.accepted = lifecycleGeneration != 0;
        if (gameplayObservation && gameplayObservation->observedDuringGameplay &&
            gameplayObservation->boundary == FovObservationBoundary::CameraWriter) {
            next.gameplay = *gameplayObservation;
            next.gameplayValid = true;
        } else if (continuedOwnership && previous.accepted &&
            previous.lifecycleGeneration != 0 && previous.gameplayValid &&
            previous.cinematicValid &&
            previous.gameplay.boundary == FovObservationBoundary::CameraWriter) {
            // A confirmed ownership continuation keeps the original Gameplay
            // boundary for this cinematic chain. It is transition evidence,
            // not a live camera sample; the new ENTER operand is published
            // separately after its production transaction completes.
            next.gameplay = previous.gameplay;
            next.gameplayValid = true;
        }
        return next;
    }

    const char* FovObservationBoundaryName(FovObservationBoundary value) noexcept;
    const char* FovSpaceName(FovSpace value) noexcept;
    const char* FovProvenanceName(FovProvenance value) noexcept;

    class CameraFovObservationStore
    {
    public:
        CameraFovObservation Publish(CameraFovObservation observation);
        std::optional<CameraFovObservation> ReadLatest(
            FovObservationBoundary boundary) const;

    private:
        static std::size_t Index(FovObservationBoundary boundary) noexcept;

        mutable std::mutex mutex_;
        std::uint64_t nextSequence_{};
        std::array<CameraFovObservation, 2> latest_{};
        std::array<bool, 2> valid_{};
    };
}
