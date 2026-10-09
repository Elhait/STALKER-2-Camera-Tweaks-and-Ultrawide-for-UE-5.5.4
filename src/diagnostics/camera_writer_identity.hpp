#pragma once

#include "../cinematics/cinematic_fov.hpp"

namespace diagnostics
{
    enum class CameraWriterIdentityChangeKind : std::uint8_t
    {
        RsiChanged,
        RbxOnlyChanged,
        RsiAndRbxChanged,
    };

    struct CameraWriterIdentityChange
    {
        bool valid{};
        bool initialization{};
        std::uint64_t oldSequence{};
        std::uint64_t newSequence{};
        std::uint64_t oldIdentityEpoch{};
        std::uint64_t newIdentityEpoch{};
        std::uintptr_t oldRsi{};
        std::uintptr_t oldRbx{};
        std::uintptr_t newRsi{};
        std::uintptr_t newRbx{};
        CameraWriterIdentityChangeKind changeKind{
            CameraWriterIdentityChangeKind::RsiChanged};
        camera::CoordinatorState coordinator{camera::CoordinatorState::Gameplay};
        bool cinematicGenerationActive{};
        std::uint64_t cinematicGeneration{};
        cinematics::CinematicRecoveryOwnerSnapshot recoveryOwner{};
    };

    // Builds an observational record only. The shared writer pair remains the
    // production observation authority; Diagnostic Mode controls only whether
    // this additional record is materialized and emitted.
    inline CameraWriterIdentityChange MakeCameraWriterIdentityChange(
        bool diagnosticEnabled,
        const cinematics::CinematicWriterAssociationSnapshot& before,
        const cinematics::CinematicWriterAssociationSnapshot& after,
        camera::CoordinatorState coordinator,
        const cinematics::CinematicRecoveryOwnerSnapshot& recoveryOwner) noexcept
    {
        CameraWriterIdentityChange event{};
        const auto& oldWriter = before.latestWriter;
        const auto& newWriter = after.latestWriter;
        if (!diagnosticEnabled || newWriter.sequence == 0)
            return event;

        if (oldWriter.sequence != 0 &&
            oldWriter.source == newWriter.source &&
            oldWriter.output == newWriter.output)
            return event;

        event.valid = true;
        event.initialization = oldWriter.sequence == 0;
        event.oldSequence = oldWriter.sequence;
        event.newSequence = newWriter.sequence;
        event.oldIdentityEpoch = oldWriter.identityEpoch;
        event.newIdentityEpoch = newWriter.identityEpoch;
        event.oldRsi = oldWriter.source;
        event.oldRbx = oldWriter.output;
        event.newRsi = newWriter.source;
        event.newRbx = newWriter.output;
        const bool rsiChanged = oldWriter.source != newWriter.source;
        const bool rbxChanged = oldWriter.output != newWriter.output;
        event.changeKind = rsiChanged && rbxChanged
            ? CameraWriterIdentityChangeKind::RsiAndRbxChanged
            : rbxChanged ? CameraWriterIdentityChangeKind::RbxOnlyChanged
            : CameraWriterIdentityChangeKind::RsiChanged;
        event.coordinator = coordinator;
        event.cinematicGenerationActive = after.generationActive;
        event.cinematicGeneration = after.generation;
        event.recoveryOwner = recoveryOwner;
        return event;
    }

    inline const char* CameraWriterIdentityChangeKindName(
        CameraWriterIdentityChangeKind kind) noexcept
    {
        switch (kind) {
        case CameraWriterIdentityChangeKind::RsiChanged:
            return "RsiChanged";
        case CameraWriterIdentityChangeKind::RbxOnlyChanged:
            return "RbxOnlyChanged";
        case CameraWriterIdentityChangeKind::RsiAndRbxChanged:
            return "RsiAndRbxChanged";
        }
        return "Unknown";
    }
}
