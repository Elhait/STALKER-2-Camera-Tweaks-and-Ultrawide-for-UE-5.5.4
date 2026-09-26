#pragma once

#include <cstdint>
#include <limits>

namespace camera
{
    enum class EvidenceProvenance : std::uint8_t
    {
        Unavailable,
        NativeEvent,
        NativeNumericObservation,
        ModDerivedState,
        ClassifierHypothesis,
        ConfirmedModLifecycle,
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

    struct CameraEvidenceSnapshot
    {
        float nativeWriterFov{std::numeric_limits<float>::quiet_NaN()};
        float aspect{std::numeric_limits<float>::quiet_NaN()};
        std::uint8_t flags{};
        float transformedFov{std::numeric_limits<float>::quiet_NaN()};
        bool nativeWriterFovValid{};
        bool aspectValid{};
        bool transformedFovValid{};
        EvidenceProvenance nativeWriterFovProvenance{EvidenceProvenance::Unavailable};
        EvidenceProvenance aspectProvenance{EvidenceProvenance::Unavailable};
        EvidenceProvenance transformedFovProvenance{EvidenceProvenance::Unavailable};
        bool configuredGameplayFovKnown{};
        float retainedGameplayFov{std::numeric_limits<float>::quiet_NaN()};
        bool retainedGameplayFovValid{};
        EvidenceProvenance retainedGameplayFovProvenance{EvidenceProvenance::Unavailable};
    };

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
