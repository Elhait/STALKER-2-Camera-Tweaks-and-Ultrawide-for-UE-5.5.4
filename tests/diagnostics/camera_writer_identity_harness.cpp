#include "../../src/diagnostics/camera_writer_identity.hpp"

#include <iostream>

namespace
{
    bool Check(bool condition, const char* message)
    {
        if (!condition) std::cerr << message << '\n';
        return condition;
    }
}

int main()
{
    using namespace diagnostics;
    using camera::CoordinatorState;

    cinematics::CinematicWriterAssociationSnapshot empty{};
    cinematics::CinematicWriterAssociationSnapshot before{};
    before.latestWriter = { 40, 7, 0x1000, 0x2000 };
    before.generationActive = true;
    before.generation = 12;
    const cinematics::CinematicRecoveryOwnerSnapshot owner{
        true, 11, 0x1000, 0x2000, false };

    auto initialized = before;
    initialized.latestWriter = { 1, 1, 0x1000, 0x2000 };
    const auto initialObservation = MakeCameraWriterIdentityChange(true,
        empty, initialized, CoordinatorState::Gameplay, {});
    if (!Check(initialObservation.valid && initialObservation.initialization &&
            initialObservation.oldSequence == 0 &&
            initialObservation.newSequence == 1 &&
            initialObservation.newRsi == 0x1000 &&
            initialObservation.newRbx == 0x2000,
            "The first writer observation must be classified as initialization")) return 1;

    const auto diagnosticOff = MakeCameraWriterIdentityChange(false,
        empty, initialized, CoordinatorState::CinematicExiting, owner);
    if (!Check(!diagnosticOff.valid,
            "Diagnostic OFF must not materialize a writer identity event")) return 1;

    const auto unchangedPair = MakeCameraWriterIdentityChange(true,
        initialized, initialized, CoordinatorState::CinematicActive, owner);
    if (!Check(!unchangedPair.valid,
            "An unchanged RSI/RBX pair must not emit an event")) return 1;

    auto epochOnly = initialized;
    epochOnly.latestWriter.sequence = 2;
    epochOnly.latestWriter.identityEpoch = 2;
    const auto epochOnlyChange = MakeCameraWriterIdentityChange(true,
        initialized, epochOnly, CoordinatorState::Gameplay, {});
    if (!Check(!epochOnlyChange.valid,
            "An epoch-only difference must not count as a writer-pair transition")) return 1;

    auto after = before;
    after.latestWriter.sequence = 41;
    after.latestWriter.identityEpoch = 8;
    after.latestWriter.source = 0x3000;
    const auto rsiChange = MakeCameraWriterIdentityChange(true,
        before, after, CoordinatorState::CinematicExiting, owner);
    if (!Check(rsiChange.valid && rsiChange.oldRsi == 0x1000 &&
            rsiChange.oldRbx == 0x2000 && rsiChange.newRsi == 0x3000 &&
            rsiChange.newRbx == 0x2000 && rsiChange.oldSequence == 40 &&
            rsiChange.newSequence == 41 && rsiChange.oldIdentityEpoch == 7 &&
            rsiChange.newIdentityEpoch == 8 &&
            !rsiChange.initialization &&
            rsiChange.changeKind == CameraWriterIdentityChangeKind::RsiChanged &&
            rsiChange.coordinator == CoordinatorState::CinematicExiting &&
            rsiChange.cinematicGenerationActive &&
            rsiChange.cinematicGeneration == 12 &&
            rsiChange.recoveryOwner.valid &&
            rsiChange.recoveryOwner.generation == 11 &&
            rsiChange.recoveryOwner.writerRsi == 0x1000 &&
            !rsiChange.recoveryOwner.recoveryComplete,
            "RSI change event must carry the old/new pair and runtime context")) return 1;

    auto rbxAfter = after;
    rbxAfter.latestWriter.sequence = 42;
    rbxAfter.latestWriter.identityEpoch = 9;
    rbxAfter.latestWriter.output = 0x4000;
    const auto rbxChange = MakeCameraWriterIdentityChange(true,
        after, rbxAfter, CoordinatorState::Gameplay, {});
    if (!Check(rbxChange.valid && rbxChange.oldRsi == 0x3000 &&
            rbxChange.newRsi == 0x3000 && rbxChange.oldRbx == 0x2000 &&
            rbxChange.newRbx == 0x4000 && !rbxChange.recoveryOwner.valid &&
            rbxChange.changeKind == CameraWriterIdentityChangeKind::RbxOnlyChanged,
            "RBX-only changes must be recorded without inventing a recovery owner")) return 1;

    auto pairAfter = before;
    pairAfter.latestWriter.sequence = 43;
    pairAfter.latestWriter.identityEpoch = 10;
    pairAfter.latestWriter.source = 0x3000;
    pairAfter.latestWriter.output = 0x4000;
    const auto pairChange = MakeCameraWriterIdentityChange(true,
        before, pairAfter, CoordinatorState::CinematicExiting, owner);
    if (!Check(pairChange.valid && !pairChange.initialization &&
            pairChange.changeKind == CameraWriterIdentityChangeKind::RsiAndRbxChanged,
            "Simultaneous RSI/RBX changes must receive their distinct classification")) return 1;

    const auto oldSource = before;
    const auto newSource = after;
    const auto oldOwner = owner;
    const auto newOwner = owner;
    (void)MakeCameraWriterIdentityChange(true, before, after,
        CoordinatorState::CinematicExiting, owner);
    if (!Check(before.latestWriter.source == oldSource.latestWriter.source &&
            before.latestWriter.output == oldSource.latestWriter.output &&
            after.latestWriter.source == newSource.latestWriter.source &&
            after.latestWriter.output == newSource.latestWriter.output &&
            owner.writerRsi == oldOwner.writerRsi &&
            owner.observedWriterRbx == newOwner.observedWriterRbx,
            "Diagnostic event creation must leave writer and recovery snapshots unchanged")) return 1;

    std::cout << "PASS: Diagnostic OFF isolation, pair-change gating, context snapshot, and state neutrality\n";
    return 0;
}
