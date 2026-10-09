#pragma once

#include "../camera/gameplay_baseline.hpp"
#include "../cinematics/cinematic_fov.hpp"

namespace diagnostics
{
    inline bool ShouldEmitCinematicBaselineProvenance(bool diagnosticModeEnabled) noexcept
    {
        return diagnosticModeEnabled;
    }

    inline bool ShouldCaptureCinematicBaselineProvenance(bool diagnosticModeEnabled,
        camera::GameplayBaselineProjectionDisposition disposition) noexcept
    {
        return diagnosticModeEnabled &&
            disposition == camera::GameplayBaselineProjectionDisposition::Updated;
    }

    // Diagnostic-only values captured at baseline publication. These fields
    // preserve observations as recorded; they do not assert an object relation.
    struct BaselineWriterPublicationObservation
    {
        bool captured{};
        std::uint64_t baselinePublicationSequence{};
        std::uint64_t writerSequence{};
        std::uint64_t writerIdentityEpoch{};
        std::uintptr_t writerRsi{};
        std::uintptr_t writerRbx{};
    };

    struct CinematicEnterBaselineProvenanceRecord
    {
        std::uint64_t generation{};
        std::uintptr_t enterRcx{};
        std::uintptr_t enterRsi{};
        float authoredInputFov{};
        config::CinematicFovMode requestedFovMode{};
        config::CinematicFovMode effectiveFovMode{};
        camera::GameplayBaseline baseline{};
        bool baselineUsable{};
        BaselineWriterPublicationObservation baselinePublication{};
        cinematics::SharedWriterObservation latestWriterBeforeEnter{};
        bool selectionAvailable{};
        cinematics::CinematicBaselineSelection selectedBaseline{};
        float resultingTargetOutput{};
        bool transformApplied{};
    };

    inline CinematicEnterBaselineProvenanceRecord CaptureCinematicEnterBaselineProvenance(
        std::uint64_t generation, std::uintptr_t enterRcx, std::uintptr_t enterRsi,
        float authoredInputFov, config::CinematicFovMode requestedFovMode,
        config::CinematicFovMode effectiveFovMode,
        const camera::GameplayBaseline& baseline,
        const BaselineWriterPublicationObservation& baselinePublication,
        const cinematics::SharedWriterObservation& latestWriterBeforeEnter) noexcept
    {
        CinematicEnterBaselineProvenanceRecord record{};
        record.generation = generation;
        record.enterRcx = enterRcx;
        record.enterRsi = enterRsi;
        record.authoredInputFov = authoredInputFov;
        record.requestedFovMode = requestedFovMode;
        record.effectiveFovMode = effectiveFovMode;
        record.baseline = baseline;
        record.baselineUsable = camera::IsUsableGameplayBaseline(baseline);
        record.baselinePublication = baselinePublication;
        record.baselinePublication.captured = baselinePublication.captured &&
            baseline.valid && baselinePublication.baselinePublicationSequence ==
                baseline.observationSequence;
        record.latestWriterBeforeEnter = latestWriterBeforeEnter;
        return record;
    }

    inline void RecordCinematicEnterBaselineSelection(
        CinematicEnterBaselineProvenanceRecord& record,
        const cinematics::CinematicBaselineSelection& selection,
        float resultingTargetOutput, bool transformApplied) noexcept
    {
        record.selectionAvailable = true;
        record.selectedBaseline = selection;
        record.resultingTargetOutput = resultingTargetOutput;
        record.transformApplied = transformApplied;
    }
}
