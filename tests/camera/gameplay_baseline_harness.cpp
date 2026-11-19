#include "../../src/camera/gameplay_baseline.hpp"

#include <cmath>
#include <iostream>
#include <string_view>

namespace
{
    struct LegacyRetainedSubset
    {
        float nativeFov{std::numeric_limits<float>::quiet_NaN()};
        float aspect{std::numeric_limits<float>::quiet_NaN()};
        std::uintptr_t source{};
        bool aspectValid{};
    };

    bool Check(bool condition, const char* name)
    {
        if (!condition) std::cerr << name << ": FAIL\n";
        return condition;
    }

    camera::CameraFovObservation NativeHorPlus(
        float nativeFov, float horPlusFov, float aspect,
        std::uintptr_t source, std::uint64_t sequence)
    {
        camera::CameraFovObservation observation{};
        observation.boundary = camera::FovObservationBoundary::CameraWriter;
        observation.inputFov = { nativeFov, camera::FovSpace::Native,
            camera::FovProvenance::NativeRegisterInput, true };
        observation.resultFov = { horPlusFov, camera::FovSpace::Transformed,
            camera::FovProvenance::ModTransformResult, true };
        observation.aspect = { aspect, camera::FovProvenance::ResolvedAspect, true };
        observation.writerSource = { source, true };
        observation.publicationSequence = sequence;
        return observation;
    }

    camera::CameraFovObservation NativePassThrough(
        float nativeFov, float aspect, std::uintptr_t source, std::uint64_t sequence)
    {
        auto observation = NativeHorPlus(nativeFov, nativeFov, aspect, source, sequence);
        observation.resultFov = { nativeFov, camera::FovSpace::Native,
            camera::FovProvenance::PassThroughResult, true };
        return observation;
    }
}

int main(int argc, char** argv)
{
    using namespace camera;
    if (argc == 2 && std::string_view(argv[1]) == "--native-origin-contract") {
        // This unit check exercises context admission; the corresponding
        // transaction harness derives context from a real Dialogue lifecycle.
        GameplayBaselineStore affected;
        affected.Project(NativeHorPlus(70.0005f, 108.941f, 3.55556f, 0x1000, 1), true);
        auto dialogueObservation = NativeHorPlus(60.9227f, 99.2615f, 3.55556f, 0x1000, 2);
        dialogueObservation.writerContext = WriterContext::DialogueLifecycleObserved;
        const auto derived = affected.Project(dialogueObservation, true);
        const bool rejectsDerived = derived.disposition ==
                GameplayBaselineProjectionDisposition::Retained &&
            derived.baseline.nativeFov.value == 70.0005f;
        std::cout << "recorded_mod_result_must_not_replace_native_reference: "
            << (rejectsDerived ? "PASS" : "FAIL")
            << " native=" << derived.baseline.nativeFov.value << '\n';

        // The identical number remains eligible under independent Gameplay context.
        GameplayBaselineStore gameChange;
        gameChange.Project(NativeHorPlus(70.0005f, 108.941f, 3.55556f, 0x1000, 1), true);
        const auto changed = gameChange.Project(
            NativeHorPlus(60.9227f, 99.2615f, 3.55556f, 0x1000, 2), true);
        const bool acceptsGameChange = changed.disposition ==
                GameplayBaselineProjectionDisposition::Updated &&
            changed.baseline.nativeFov.value == 60.9227f;
        std::cout << "independent_game_change_must_update_native_reference: "
            << (acceptsGameChange ? "PASS" : "FAIL") << '\n';
        return rejectsDerived && acceptsGameChange ? 0 : 1;
    }
    bool pass = true;
    GameplayBaselineStore store;
    LegacyRetainedSubset legacy{};

    const auto startup = store.Read();
    pass &= Check(!startup.valid && !startup.nativeFov.valid &&
        std::isnan(startup.nativeFov.value), "startup_invalid");

    const auto first = store.Project(NativeHorPlus(112.623f, 143.132f, 3.55556f,
        0x1000, 7), true);
    legacy.nativeFov = 112.623f;
    legacy.aspect = 3.55556f;
    legacy.source = 0x1000;
    legacy.aspectValid = true;
    pass &= Check(first.disposition == GameplayBaselineProjectionDisposition::Updated,
        "valid_update");
    pass &= Check(first.baseline.valid && first.baseline.nativeFov.valid &&
        first.baseline.nativeFov.value == 112.623f && first.baseline.horPlusFov.valid &&
        first.baseline.horPlusFov.value == 143.132f && first.baseline.aspect == 3.55556f &&
        first.baseline.source.value == 0x1000 && first.baseline.observationSequence == 7,
        "coherent_first_pair");
    pass &= Check(first.baseline.nativeFov.value == legacy.nativeFov &&
        first.baseline.aspect == legacy.aspect && first.baseline.source.value == legacy.source,
        "legacy_subset_equivalence");

    const auto second = store.Project(NativeHorPlus(101.25f, 129.0f, 2.4f, 0x2000, 8), true);
    pass &= Check(second.disposition == GameplayBaselineProjectionDisposition::Updated &&
        second.baseline.nativeFov.value == 101.25f && second.baseline.horPlusFov.valid &&
        second.baseline.horPlusFov.value == 129.0f &&
        second.baseline.aspect == 2.4f && second.baseline.source.value == 0x2000 &&
        second.baseline.observationSequence == 8, "consecutive_update");

    const auto ads = store.Project(NativeHorPlus(73.5f, 101.0f, 2.4f, 0x2000, 9), true);
    legacy.nativeFov = 73.5f;
    legacy.aspect = 2.4f;
    legacy.source = 0x2000;
    pass &= Check(ads.disposition == GameplayBaselineProjectionDisposition::Updated &&
        ads.baseline.nativeFov.valid && ads.baseline.nativeFov.value == 73.5f &&
        ads.baseline.horPlusFov.valid && ads.baseline.horPlusFov.value == 101.0f,
        "transient_gameplay_matches_legacy_retention");

    const auto passThrough = store.Project(NativePassThrough(80.0f, 2.4f, 0x2000, 10), true);
    legacy.nativeFov = 80.0f;
    legacy.aspect = 2.4f;
    legacy.source = 0x2000;
    pass &= Check(passThrough.disposition == GameplayBaselineProjectionDisposition::Updated &&
        passThrough.baseline.valid && passThrough.baseline.nativeFov.valid &&
        passThrough.baseline.nativeFov.value == 80.0f &&
        !passThrough.baseline.horPlusFov.valid,
        "valid_passthrough_updates_native_only");
    pass &= Check(passThrough.baseline.nativeFov.value == legacy.nativeFov &&
        passThrough.baseline.aspect == legacy.aspect &&
        passThrough.baseline.source.value == legacy.source,
        "passthrough_legacy_subset_equivalence");

    auto invalid = NativePassThrough(75.0f, 2.4f, 0x2000, 11);
    invalid.inputFov.valid = false;
    const auto retainedInvalid = store.Project(invalid, true);
    pass &= Check(retainedInvalid.disposition == GameplayBaselineProjectionDisposition::Retained &&
        retainedInvalid.baseline.nativeFov.value == 80.0f,
        "invalid_retains");
    pass &= Check(retainedInvalid.baseline.nativeFov.value == legacy.nativeFov &&
        retainedInvalid.baseline.aspect == legacy.aspect &&
        retainedInvalid.baseline.source.value == legacy.source,
        "invalid_preserves_legacy_subset");

    const auto cinematic = store.Project(NativeHorPlus(90.0f, 126.87f, 3.55556f,
        0x3000, 11), false);
    pass &= Check(cinematic.disposition == GameplayBaselineProjectionDisposition::Retained &&
        cinematic.baseline.nativeFov.value == 80.0f, "cinematic_retains");

    CameraFovObservation cachedObservation{};
    cachedObservation.boundary = FovObservationBoundary::CameraWriter;
    cachedObservation.inputFov = { 126.87f, FovSpace::Transformed,
        FovProvenance::CachedCinematicEnter, true };
    cachedObservation.resultFov = { 126.87f, FovSpace::Transformed,
        FovProvenance::PassThroughResult, true };
    cachedObservation.aspect = { 3.55556f, FovProvenance::ResolvedAspect, true };
    cachedObservation.writerSource = { 0x3000, true };
    cachedObservation.publicationSequence = 12;
    const auto cachedResult = store.Project(cachedObservation, false);
    pass &= Check(cachedResult.disposition == GameplayBaselineProjectionDisposition::Retained &&
        cachedResult.baseline.nativeFov.value == 80.0f, "cached_transformed_retains");

    const auto noEligibility = store.Project(NativeHorPlus(84.0f, 118.0f, 2.4f,
        0x4000, 13), false);
    pass &= Check(noEligibility.disposition == GameplayBaselineProjectionDisposition::Retained &&
        noEligibility.baseline.nativeFov.value == 80.0f, "semantic_eligibility_is_explicit");

    GameplayBaselineStore passThroughStartupStore;
    const auto passThroughStartup = passThroughStartupStore.Project(
        NativePassThrough(88.0f, 2.4f, 0x4100, 14), true);
    pass &= Check(passThroughStartup.disposition == GameplayBaselineProjectionDisposition::Updated &&
        passThroughStartup.baseline.valid && passThroughStartup.baseline.nativeFov.value == 88.0f &&
        !passThroughStartup.baseline.horPlusFov.valid, "passthrough_only_startup");
    GameplayBaselineStore aspectRecalculationStore;
    const auto aspectRecalculation = aspectRecalculationStore.Project(
        NativePassThrough(96.0f, 1.77778f, 0x4200, 14), true);
    pass &= Check(aspectRecalculation.disposition == GameplayBaselineProjectionDisposition::Updated &&
        aspectRecalculation.baseline.valid && aspectRecalculation.baseline.nativeFov.value == 96.0f &&
        !aspectRecalculation.baseline.horPlusFov.valid,
        "aspect_recalculation_native_update");
    const auto transformedAfterPassThrough = passThroughStartupStore.Project(
        NativeHorPlus(91.0f, 130.0f, 2.4f, 0x4100, 15), true);
    pass &= Check(transformedAfterPassThrough.disposition ==
        GameplayBaselineProjectionDisposition::Updated &&
        transformedAfterPassThrough.baseline.nativeFov.value == 91.0f &&
        transformedAfterPassThrough.baseline.horPlusFov.valid &&
        transformedAfterPassThrough.baseline.horPlusFov.value == 130.0f,
        "transformed_after_passthrough");

    auto malformed = NativeHorPlus(84.0f, 118.0f, 2.4f, 0x4000, 14);
    malformed.writerSource.valid = false;
    const auto malformedResult = store.Project(malformed, true);
    pass &= Check(malformedResult.disposition == GameplayBaselineProjectionDisposition::Retained &&
        malformedResult.baseline.nativeFov.value == 80.0f, "malformed_retains");

    const auto invalidated = store.Invalidate();
    legacy = {};
    legacy.aspectValid = false;
    pass &= Check(invalidated.disposition == GameplayBaselineProjectionDisposition::Invalidated &&
        !invalidated.baseline.valid && !invalidated.baseline.nativeFov.valid &&
        std::isnan(invalidated.baseline.nativeFov.value),
        "explicit_invalidate");
    pass &= Check(!legacy.aspectValid && std::isnan(legacy.nativeFov),
        "legacy_transition_invalidation_model");

    const auto afterInvalidate = store.Project(NativeHorPlus(90.0f, 126.87f, 3.55556f,
        0x5000, 15), true);
    pass &= Check(afterInvalidate.disposition == GameplayBaselineProjectionDisposition::Updated &&
        afterInvalidate.baseline.observationSequence == 15, "update_after_invalidate");

    // Regression from the 90 -> receiver replacement/reload -> 130 runtime
    // sequence. A non-Gameplay sample from a new receiver must not leave the
    // old receiver's Gameplay baseline usable for subsequent Cinematic or
    // provider handoff decisions. Expected HorPlus is computed independently
    // in double precision instead of calling production camera::HorPlus.
    constexpr std::uintptr_t oldReceiver = 0x22707d56700;
    constexpr std::uintptr_t newReceiver = 0x2279302d9c0;
    constexpr float nativeAspect = 16.0f / 9.0f;
    constexpr float runtimeAspect = 32.0f / 9.0f;
    constexpr float replacementNativeFov = 130.3115f;
    GameplayBaselineStore receiverReplacement;
    const auto oldReceiverBaseline = receiverReplacement.Project(
        NativeHorPlus(90.3115f, 127.119f, runtimeAspect, oldReceiver, 100), true);
    auto replacementDuringAmbiguousTransition = NativeHorPlus(
        replacementNativeFov, 153.931f, runtimeAspect, newReceiver, 101);
    replacementDuringAmbiguousTransition.writerContext = WriterContext::TransitionAmbiguous;
    const auto replacementProjection = receiverReplacement.Project(
        replacementDuringAmbiguousTransition, false);
    const auto independentHorPlus = static_cast<float>(
        2.0 * std::atan(std::tan(static_cast<double>(replacementNativeFov) *
            3.14159265358979323846 / 360.0) *
            (static_cast<double>(runtimeAspect) / nativeAspect)) *
        180.0 / 3.14159265358979323846);
    pass &= Check(oldReceiverBaseline.baseline.source.value == oldReceiver &&
        replacementProjection.baseline.source.value != newReceiver &&
        !replacementProjection.baseline.current &&
        !IsUsableGameplayBaseline(replacementProjection.baseline),
        "receiver_replacement_ambiguous_sample_invalidates_old_gameplay_baseline");
    pass &= Check(std::fabs(independentHorPlus - 153.931) < 0.01,
        "receiver_replacement_130_fov_independent_horplus_oracle");
    const auto trustedNewGameplay = receiverReplacement.Project(
        NativeHorPlus(replacementNativeFov, independentHorPlus, runtimeAspect,
            newReceiver, 102), true);
    pass &= Check(trustedNewGameplay.disposition ==
            GameplayBaselineProjectionDisposition::Updated &&
        trustedNewGameplay.baseline.source.value == newReceiver &&
        trustedNewGameplay.baseline.nativeFov.value == replacementNativeFov &&
        trustedNewGameplay.baseline.current &&
        IsUsableGameplayBaseline(trustedNewGameplay.baseline),
        "receiver_replacement_fresh_gameplay_reestablishes_current_baseline");

    // Neighbor cases derived from the runtime failure class. Receiver identity,
    // not the direction or magnitude of the FOV change, determines whether an
    // ambiguous sample can preserve the old receiver's baseline.
    struct ReceiverReplacementCase
    {
        const char* name;
        float oldFov;
        float replacementFov;
    };
    constexpr ReceiverReplacementCase replacementCases[] = {
        { "receiver_replacement_130_to_90_invalidates_old_baseline", 130.0f, 90.0f },
        { "receiver_replacement_same_fov_invalidates_old_baseline", 90.0f, 90.0f },
    };
    std::uint64_t replacementSequence = 200;
    for (const auto& testCase : replacementCases) {
        GameplayBaselineStore storeForReplacement;
        const auto seeded = storeForReplacement.Project(NativeHorPlus(
            testCase.oldFov, testCase.oldFov, runtimeAspect, oldReceiver,
            replacementSequence++), true);
        auto ambiguousReplacement = NativeHorPlus(testCase.replacementFov,
            testCase.replacementFov, runtimeAspect, newReceiver,
            replacementSequence++);
        ambiguousReplacement.writerContext = WriterContext::TransitionAmbiguous;
        const auto projected = storeForReplacement.Project(ambiguousReplacement, false);
        pass &= Check(seeded.baseline.source.value == oldReceiver &&
            projected.baseline.source.value != newReceiver &&
            !projected.baseline.current &&
            !IsUsableGameplayBaseline(projected.baseline), testCase.name);
    }

    // Generic same-receiver ambiguity remains fail-closed. Explicitly
    // identified provider/recovery transitions have a separate retention path.
    GameplayBaselineStore sameReceiverFovChange;
    sameReceiverFovChange.Project(NativeHorPlus(90.0f, 127.0f, runtimeAspect,
        oldReceiver, replacementSequence++), true);
    auto sameReceiverAmbiguous = NativeHorPlus(130.0f, 153.0f, runtimeAspect,
        oldReceiver, replacementSequence++);
    sameReceiverAmbiguous.writerContext = WriterContext::TransitionAmbiguous;
    const auto sameReceiverProjection = sameReceiverFovChange.Project(
        sameReceiverAmbiguous, false);
    pass &= Check(!sameReceiverProjection.baseline.current &&
        sameReceiverProjection.baseline.source.value == oldReceiver &&
        sameReceiverProjection.baseline.nativeFov.value == 90.0f &&
        !IsUsableGameplayBaseline(sameReceiverProjection.baseline),
        "same_receiver_unclassified_fov_change_remains_excluded");

    GameplayBaselineStore knownTransition;
    knownTransition.Project(NativeHorPlus(130.311f, 153.931f, runtimeAspect,
        oldReceiver, replacementSequence++), true);
    auto providerSample = NativeHorPlus(70.0f, 118.5f, runtimeAspect,
        oldReceiver, replacementSequence++);
    providerSample.writerContext = WriterContext::ProviderTransitionObserved;
    const auto providerProjection = knownTransition.Project(providerSample, false);
    auto recoverySample = NativeHorPlus(70.0f, 118.5f, runtimeAspect,
        oldReceiver, replacementSequence++);
    recoverySample.writerContext = WriterContext::PostCinematicRecoveryExcluded;
    const auto recoveryProjection = knownTransition.Project(recoverySample, false);
    pass &= Check(providerProjection.disposition == GameplayBaselineProjectionDisposition::Retained &&
        recoveryProjection.disposition == GameplayBaselineProjectionDisposition::Retained &&
        IsUsableGameplayBaseline(recoveryProjection.baseline) &&
        recoveryProjection.baseline.source.value == oldReceiver &&
        std::fabs(recoveryProjection.baseline.nativeFov.value - 130.311f) < 0.001f &&
        std::fabs(recoveryProjection.baseline.horPlusFov.value - 153.931f) < 0.001f,
        "known provider and post-cinematic recovery contexts retain prior Gameplay reference");

    GameplayBaselineStore confirmedDialogueRetention;
    confirmedDialogueRetention.Project(NativeHorPlus(130.311f, 153.931f,
        runtimeAspect, oldReceiver, replacementSequence++), true);
    auto confirmedDialogueSample = NativeHorPlus(70.0f, 118.5f,
        runtimeAspect, oldReceiver, replacementSequence++);
    confirmedDialogueSample.writerContext =
        WriterContext::ConfirmedProviderDialogueLifecycleObserved;
    const auto confirmedDialogueProjection = confirmedDialogueRetention.Project(
        confirmedDialogueSample, false);
    auto genericDialogueSample = NativeHorPlus(69.9f, 118.4f,
        runtimeAspect, oldReceiver, replacementSequence++);
    genericDialogueSample.writerContext = WriterContext::DialogueLifecycleObserved;
    const auto genericDialogueProjection = confirmedDialogueRetention.Project(
        genericDialogueSample, false);
    pass &= Check(confirmedDialogueProjection.baseline.current &&
        IsUsableGameplayBaseline(confirmedDialogueProjection.baseline) &&
        confirmedDialogueProjection.baseline.nativeFov.value == 130.311f &&
        !genericDialogueProjection.baseline.current &&
        !IsUsableGameplayBaseline(genericDialogueProjection.baseline),
        "only_confirmed_provider_dialogue_retains_gameplay_baseline_currency");

    // Repeated replacement must not resurrect the first receiver's baseline
    // after another ambiguous receiver appears.
    GameplayBaselineStore repeatedReplacement;
    repeatedReplacement.Project(NativeHorPlus(90.0f, 127.0f, runtimeAspect,
        oldReceiver, replacementSequence++), true);
    auto firstReplacement = NativeHorPlus(130.0f, 153.0f, runtimeAspect,
        newReceiver, replacementSequence++);
    firstReplacement.writerContext = WriterContext::TransitionAmbiguous;
    const auto afterFirstReplacement = repeatedReplacement.Project(
        firstReplacement, false);
    constexpr std::uintptr_t thirdReceiver = 0x227A0000100;
    auto secondReplacement = NativeHorPlus(100.0f, 140.0f, runtimeAspect,
        thirdReceiver, replacementSequence++);
    secondReplacement.writerContext = WriterContext::TransitionAmbiguous;
    const auto afterSecondReplacement = repeatedReplacement.Project(
        secondReplacement, false);
    pass &= Check(!IsUsableGameplayBaseline(afterFirstReplacement.baseline) &&
        !afterSecondReplacement.eligible && !afterSecondReplacement.baseline.current &&
        !IsUsableGameplayBaseline(afterSecondReplacement.baseline),
        "repeated_receiver_replacements_keep_stale_baseline_unusable");

    std::cout << "Gameplay baseline harness: " << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
