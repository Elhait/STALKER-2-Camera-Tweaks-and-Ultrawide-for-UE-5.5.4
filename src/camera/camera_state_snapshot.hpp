#pragma once

#include <cstdint>
#include <limits>
#include <initializer_list>

namespace camera
{
    enum class EvidenceProvenance : std::uint8_t
    {
        Unavailable,
        NativeEvent,
        NativeNumericObservation,
        PassThroughResult,
        ClientViewportObservation,
        ModDerivedState,
        ClassifierHypothesis,
        ConfirmedModLifecycle,
    };

    // Freshness describes whether a value is current for its own provenance
    // group. Historical retains the last value for inspection without
    // presenting it as current camera state.
    enum class SnapshotFreshness : std::uint8_t
    {
        Unavailable,
        Current,
        Retained,
        Historical,
    };

    struct CameraStateFreshness
    {
        SnapshotFreshness writerObservation{SnapshotFreshness::Unavailable};
        SnapshotFreshness transformedResult{SnapshotFreshness::Unavailable};
        SnapshotFreshness presentation{SnapshotFreshness::Unavailable};
        SnapshotFreshness dialogue{SnapshotFreshness::Unavailable};
        SnapshotFreshness gameplayMode{SnapshotFreshness::Unavailable};
        SnapshotFreshness zoom{SnapshotFreshness::Unavailable};
    };

    enum class PresentationState : std::uint8_t
    {
        Unknown,
        Gameplay,
        CinematicActive,
        CinematicExiting,
    };

    enum class DialogueState : std::uint8_t
    {
        Inactive,
        Candidate,
        Active,
        Exiting,
        Recovery,
        Unknown,
    };

    enum class ZoomDirection : std::uint8_t
    {
        None,
        In,
        Out,
    };

    const char* EvidenceProvenanceName(EvidenceProvenance value) noexcept;
    const char* SnapshotFreshnessName(SnapshotFreshness value) noexcept;
    const char* PresentationStateName(PresentationState value) noexcept;
    const char* DialogueStateName(DialogueState value) noexcept;
    const char* ZoomDirectionName(ZoomDirection value) noexcept;

    struct GameplayWriterSourceToken
    {
        std::uintptr_t value{};
        bool valid{};
    };

    struct DialogueBlendSourceToken
    {
        std::uintptr_t value{};
        bool valid{};
    };

    struct ZoomSourceToken
    {
        std::uintptr_t value{};
        bool valid{};
    };

    struct PresentationSnapshot
    {
        PresentationState state{PresentationState::Unknown};
        EvidenceProvenance provenance{EvidenceProvenance::Unavailable};
        std::uint64_t epoch{};
        bool epochValid{};
    };

    struct ZoomEvidenceSnapshot
    {
        ZoomDirection direction{ZoomDirection::None};
        EvidenceProvenance provenance{EvidenceProvenance::Unavailable};
        std::uint64_t sequence{};
        bool valid{};
        float primaryWeight{std::numeric_limits<float>::quiet_NaN()};
        float secondaryWeight{std::numeric_limits<float>::quiet_NaN()};
        ZoomSourceToken source{};
        bool active{};
    };

    struct DialogueSnapshot
    {
        DialogueState state{DialogueState::Unknown};
        EvidenceProvenance provenance{EvidenceProvenance::Unavailable};
        bool activePolicyValid{};
        bool recoveryExclusionActive{};
        float nativeTarget{std::numeric_limits<float>::quiet_NaN()};
        bool nativeTargetValid{};
        DialogueBlendSourceToken source{};
    };

    struct GameplayModeSnapshot
    {
        std::uint8_t value{};
        EvidenceProvenance provenance{EvidenceProvenance::Unavailable};
        bool valid{};
    };

    // A semantic FOV fact is independently valid and can remain Retained
    // across camera-owner changes without being presented as a live sample.
    enum class CameraFovBoundary : std::uint8_t
    {
        Unavailable, GameplayWriter, DialogueSetter, CinematicEnter,
        PrimaryPostStore, PostHelper,
    };

    inline const char* CameraFovBoundaryName(CameraFovBoundary boundary) noexcept
    {
        switch (boundary) {
        case CameraFovBoundary::GameplayWriter: return "CameraWriter";
        case CameraFovBoundary::DialogueSetter: return "DialogueSetter";
        case CameraFovBoundary::CinematicEnter: return "CinematicEnter";
        case CameraFovBoundary::PrimaryPostStore: return "PrimaryPostStore";
        case CameraFovBoundary::PostHelper: return "PostHelper";
        default: return "Unavailable";
        }
    }

    struct CameraWriterTransactionIdentity
    {
        std::uint64_t id{};
        std::uintptr_t receiver{};
        std::uintptr_t output{};
        std::uint32_t thread{};
        std::uint64_t lifecycle{};
        bool operator==(const CameraWriterTransactionIdentity&) const = default;
        bool Valid() const noexcept { return id && receiver && output && thread; }
    };

    struct CameraFovFact
    {
        float value{std::numeric_limits<float>::quiet_NaN()};
        bool valid{};
        EvidenceProvenance provenance{EvidenceProvenance::Unavailable};
        SnapshotFreshness freshness{SnapshotFreshness::Unavailable};
        std::uintptr_t source{};
        bool sourceValid{};
        std::uint64_t sequence{};
        CameraFovBoundary boundary{CameraFovBoundary::Unavailable};
        std::uint64_t writerTransactionId{};
    };

    struct CameraEvidenceSnapshot
    {
        CameraFovFact gameplayFov{};
        CameraFovFact nativeFov{};
        CameraFovFact horPlusTargetFov{};
        CameraFovFact currentOutputFov{};
        // currentOutputFov is the existing planned-output fact, not readback.
        CameraFovFact neutralGameplayFov{};
        CameraFovFact writerInputFov{};
        CameraFovFact postStoreFov{};
        CameraFovFact postHelperFov{};
        CameraWriterTransactionIdentity writerTransaction{};
        bool gameplayReferenceUsable{};
        float aspect{std::numeric_limits<float>::quiet_NaN()};
        std::uint8_t flags{};
        bool aspectValid{};
        // Retains whether a full writer publication supplied any usable FOV
        // or aspect evidence, independently of transaction Native FOV.
        bool writerObservationHasEvidence{};
        EvidenceProvenance aspectProvenance{EvidenceProvenance::Unavailable};
        bool configuredGameplayFovKnown{};
        float gameplayPolicyAspect{std::numeric_limits<float>::quiet_NaN()};
        bool gameplayPolicyAspectValid{};
        EvidenceProvenance gameplayPolicyAspectProvenance{EvidenceProvenance::Unavailable};
        float currentViewportAspect{std::numeric_limits<float>::quiet_NaN()};
        bool currentViewportAspectValid{};
        EvidenceProvenance currentViewportAspectProvenance{EvidenceProvenance::Unavailable};
        bool runtimeCombineValid{};
        float runtimeCombineAspectBefore{std::numeric_limits<float>::quiet_NaN()};
        float runtimeCombineTargetAspect{std::numeric_limits<float>::quiet_NaN()};
        float runtimeCombineAspectAfter{std::numeric_limits<float>::quiet_NaN()};
        float runtimeCombineFovInput{std::numeric_limits<float>::quiet_NaN()};
        float runtimeCombineFovOutput{std::numeric_limits<float>::quiet_NaN()};
        std::uint8_t runtimeCombineFlagsBefore{};
        std::uint8_t runtimeCombineFlagsAfter{};
        bool runtimeCombineMismatch{};
        bool runtimeCombineWriteAttempted{};
        bool runtimeCombineWriteSucceeded{};
        bool runtimeCombineReadbackValid{};
        bool runtimeCombineConfirmed{};
        bool runtimeCombineFovApplied{};
    };

    // Select the latest named observation of the current planned transaction.
    // Both underlying readbacks remain inspectable; neither is rendered FOV.
    inline CameraFovFact EffectiveCameraFov(const CameraEvidenceSnapshot& evidence) noexcept
    {
        const auto& identity = evidence.writerTransaction;
        if (!identity.Valid() || !evidence.currentOutputFov.valid ||
            evidence.currentOutputFov.freshness != SnapshotFreshness::Current ||
            evidence.currentOutputFov.source != identity.receiver ||
            evidence.currentOutputFov.writerTransactionId != identity.id)
            return {};
        for (const auto* fact : {&evidence.postHelperFov, &evidence.postStoreFov}) {
            if (fact->valid && fact->freshness == SnapshotFreshness::Current &&
                fact->writerTransactionId == identity.id &&
                fact->sourceValid && fact->source == identity.receiver)
                return *fact;
        }
        return {};
    }

    struct SourceProvenance
    {
        GameplayWriterSourceToken gameplayWriter{};
    };

    struct GenerationProvenance
    {
        std::uint64_t presentationEpoch{};
        bool presentationEpochValid{};
        std::uint64_t eventSequence{};
        bool eventSequenceValid{};
    };

    struct CameraStateSnapshot
    {
        PresentationSnapshot presentation{};
        ZoomEvidenceSnapshot zoom{};
        DialogueSnapshot dialogue{};
        GameplayModeSnapshot gameplayMode{};
        CameraEvidenceSnapshot evidence{};
        SourceProvenance source{};
        GenerationProvenance generation{};
        CameraStateFreshness freshness{};
    };

    struct CameraStateInput
    {
        PresentationSnapshot presentation{};
        ZoomEvidenceSnapshot zoom{};
        DialogueSnapshot dialogue{};
        GameplayModeSnapshot gameplayMode{};
        CameraEvidenceSnapshot evidence{};
        SourceProvenance source{};
        GenerationProvenance generation{};
    };

    CameraStateSnapshot BuildCameraStateSnapshot(const CameraStateInput& input) noexcept;
    bool SnapshotChanged(const CameraStateSnapshot& current,
        const CameraStateSnapshot& previous) noexcept;
    bool SnapshotSemanticsChanged(const CameraStateSnapshot& current,
        const CameraStateSnapshot& previous) noexcept;
}
