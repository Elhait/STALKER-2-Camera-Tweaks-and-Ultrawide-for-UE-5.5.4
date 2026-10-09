#include "../../src/camera/fov_observation.hpp"

#include <cmath>
#include <iostream>
#include <limits>

namespace
{
    bool Check(bool condition, const char* name)
    {
        if (!condition) std::cerr << name << ": FAIL\n";
        return condition;
    }
}

int main()
{
    using namespace camera;
    CameraFovObservationStore store;
    bool pass = true;
    pass &= Check(!store.ReadLatest(FovObservationBoundary::CameraWriter).has_value(),
        "empty_store");

    CameraFovObservation gameplay{};
    gameplay.boundary = FovObservationBoundary::CameraWriter;
    gameplay.inputFov = {112.623f, FovSpace::Native, FovProvenance::NativeRegisterInput, true};
    gameplay.resultFov = {143.132f, FovSpace::Transformed, FovProvenance::ModTransformResult, true};
    gameplay.aspect = {3.55556f, FovProvenance::ResolvedAspect, true};
    gameplay.writerSource = {0x1234, true};
    gameplay.writerFlags = 0x5;
    gameplay.writerFlagsValid = true;
    gameplay.observedDuringGameplay = true;
    const auto committedGameplay = store.Publish(gameplay);
    pass &= Check(committedGameplay.publicationSequence == 1, "first_sequence");
    pass &= Check(committedGameplay.inputFov.space == FovSpace::Native &&
        committedGameplay.resultFov.space == FovSpace::Transformed,
        "gameplay_spaces");

    CameraFovObservation enter{};
    enter.boundary = FovObservationBoundary::CinematicEnter;
    enter.inputFov = {90.0f, FovSpace::Native, FovProvenance::NativeRegisterInput, true};
    enter.resultFov = {143.132f, FovSpace::Transformed, FovProvenance::ModTransformResult, true};
    enter.aspect = {3.55556f, FovProvenance::ResolvedAspect, true};
    const auto committedEnter = store.Publish(enter);
    const auto latestGameplay = store.ReadLatest(FovObservationBoundary::CameraWriter);
    const auto latestEnter = store.ReadLatest(FovObservationBoundary::CinematicEnter);
    pass &= Check(committedEnter.publicationSequence == 2, "second_sequence");
    pass &= Check(latestGameplay.has_value() && latestGameplay->inputFov.value == 112.623f,
        "boundary_isolation_writer");
    pass &= Check(latestEnter.has_value() && latestEnter->inputFov.value == 90.0f,
        "boundary_isolation_enter");

    auto generationOne = BeginCameraTransitionBoundary({}, 1,
        latestGameplay, false);
    generationOne.cinematic = committedEnter;
    generationOne.cinematicValid = true;
    const auto generationTwo = BeginCameraTransitionBoundary(generationOne, 2,
        std::nullopt, true);
    pass &= Check(generationOne.accepted && generationOne.gameplayValid &&
        generationOne.cinematicValid && generationTwo.accepted &&
        generationTwo.lifecycleGeneration == 2 && generationTwo.gameplayValid &&
        !generationTwo.cinematicValid &&
        generationTwo.gameplay.publicationSequence ==
            generationOne.gameplay.publicationSequence,
        "confirmed_continuation_carries_only_gameplay_boundary_into_new_enter_generation");
    const auto unrelatedGeneration = BeginCameraTransitionBoundary(generationOne,
        2, std::nullopt, false);
    pass &= Check(unrelatedGeneration.accepted &&
        !unrelatedGeneration.gameplayValid,
        "unrelated_enter_does_not_reuse_old_gameplay_boundary");
    const auto refreshedGeneration = BeginCameraTransitionBoundary(generationOne,
        2, latestGameplay, true);
    pass &= Check(refreshedGeneration.gameplayValid &&
        refreshedGeneration.gameplay.publicationSequence ==
            latestGameplay->publicationSequence,
        "current_gameplay_boundary_takes_precedence_over_continued_evidence");

    CameraFovObservation cached{};
    cached.boundary = FovObservationBoundary::CameraWriter;
    cached.inputFov = {126.87f, FovSpace::Transformed, FovProvenance::CachedCinematicEnter, true};
    cached.resultFov = {126.87f, FovSpace::Transformed, FovProvenance::PassThroughResult, true};
    cached.aspect = {3.55556f, FovProvenance::ResolvedAspect, true};
    const auto committedCached = store.Publish(cached);
    pass &= Check(committedCached.inputFov.valid &&
        committedCached.inputFov.space == FovSpace::Transformed,
        "cached_not_native");
    pass &= Check(committedCached.inputFov.provenance == FovProvenance::CachedCinematicEnter,
        "cached_provenance");
    pass &= Check(committedCached.resultFov.provenance == FovProvenance::PassThroughResult,
        "cached_passthrough");
    pass &= Check(committedCached.publicationSequence == 3, "third_sequence");

    CameraFovObservation invalid{};
    invalid.boundary = FovObservationBoundary::CinematicEnter;
    const auto committedInvalid = store.Publish(invalid);
    pass &= Check(!committedInvalid.inputFov.valid &&
        std::isnan(committedInvalid.inputFov.value) &&
        committedInvalid.inputFov.space == FovSpace::Unknown &&
        committedInvalid.inputFov.provenance == FovProvenance::Unavailable,
        "canonical_invalid_fov");
    pass &= Check(!committedInvalid.aspect.valid && std::isnan(committedInvalid.aspect.value),
        "canonical_invalid_scalar");
    pass &= Check(committedInvalid.publicationSequence == 4, "fourth_sequence");

    std::cout << "FOV observation harness: " << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
