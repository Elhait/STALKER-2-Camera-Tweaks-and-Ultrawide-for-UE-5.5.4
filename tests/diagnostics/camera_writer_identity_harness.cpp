#include "../../src/diagnostics/camera_writer_identity.hpp"
#include "../../src/diagnostics/writer_diagnostic_aggregation.hpp"
#include "../../src/diagnostics/post_exit_blend_observation.hpp"
#include "../../src/diagnostics/camera_state_writer_log.hpp"

#include <iostream>
#include <thread>
#include <vector>

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

    CameraStateWriterLogQueue<2> writerLog;
    CameraStateWriterEvent writerEvent{};
    writerEvent.writerTransactionId = 101;
    if (!Check(writerLog.TryPush(writerEvent), "Writer log rejected its first event")) return 1;
    writerEvent.writerTransactionId = 102;
    if (!Check(writerLog.TryPush(writerEvent), "Writer log rejected its second event")) return 1;
    writerEvent.writerTransactionId = 103;
    if (!Check(!writerLog.TryPush(writerEvent), "Writer log did not report full-buffer loss")) return 1;
    CameraStateWriterEvent readEvent{};
    if (!Check(writerLog.TryPop(readEvent) && readEvent.writerTransactionId == 101 &&
            readEvent.eventSequence == 1, "Writer log first event order changed")) return 1;
    if (!Check(writerLog.TryPop(readEvent) && readEvent.writerTransactionId == 102 &&
            readEvent.eventSequence == 2, "Writer log second event order changed")) return 1;
    if (!Check(!writerLog.TryPop(readEvent), "Writer log returned an event twice")) return 1;
    const auto writerLogCounters = writerLog.Counters();
    if (!Check(writerLogCounters.accepted == 2 && writerLogCounters.droppedFull == 1 &&
            writerLogCounters.droppedContended == 0,
            "Writer log loss accounting is inaccurate")) return 1;

    CameraStateWriterLogCoalescer coalescer;
    CameraStateWriterEventRun completedRun{};
    CameraStateWriterEvent stableSample{};
    stableSample.eventSequence = 4;
    stableSample.writerTransactionId = 1;
    stableSample.kind = static_cast<std::uint8_t>(CameraStateWriterEventKind::PostProcess);
    stableSample.receiver = 0x1000;
    stableSample.outputObject = 0x2000;
    stableSample.writerContext = 0;
    stableSample.inputFov = 90.0f;
    stableSample.plannedAfter = 127.119f;
    stableSample.actualAfter = 127.119f;
    if (!Check(coalescer.Observe(stableSample, completedRun) ==
            CameraStateWriterRunUpdate::Started && completedRun.count == 0,
            "First diagnostic sample did not start a coalesced run")) return 1;
    auto repeatedSample = stableSample;
    repeatedSample.eventSequence = 8;
    repeatedSample.writerTransactionId = 2;
    if (!Check(coalescer.Observe(repeatedSample, completedRun) ==
            CameraStateWriterRunUpdate::Coalesced && completedRun.count == 0,
            "An identical adjacent sample was not coalesced")) return 1;
    auto changedSample = repeatedSample;
    changedSample.eventSequence = 12;
    changedSample.writerTransactionId = 3;
    changedSample.plannedAfter = 128.0f;
    changedSample.actualAfter = 128.0f;
    if (!Check(coalescer.Observe(changedSample, completedRun) ==
            CameraStateWriterRunUpdate::Replaced && completedRun.count == 2 &&
            completedRun.first.eventSequence == 4 && completedRun.last.eventSequence == 8 &&
            completedRun.first.writerTransactionId == 1 &&
            completedRun.last.writerTransactionId == 2 &&
            SameCameraStateWriterPayload(completedRun.first, completedRun.last),
            "Changed writer output did not close the exact prior sample run")) return 1;
    if (!Check(coalescer.Flush(completedRun) && completedRun.count == 1 &&
            completedRun.first.writerTransactionId == 3 &&
            completedRun.last.writerTransactionId == 3,
            "Final diagnostic sample run did not flush with its exact identity")) return 1;
    changedSample.dialogueSettingsEpoch = 1;
    if (!Check(!SameCameraStateWriterPayload(stableSample, changedSample),
            "A settings-epoch change was incorrectly coalesced")) return 1;
    changedSample.dialogueSettingsEpoch = 0;
    changedSample.writerContext = 1;
    if (!Check(!SameCameraStateWriterPayload(stableSample, changedSample),
            "A camera context change was incorrectly coalesced")) return 1;

    CameraStateWriterLogQueue<128> concurrentWriterLog;
    std::vector<std::thread> producers;
    for (std::uint64_t producer = 0; producer < 4; ++producer) {
        producers.emplace_back([&concurrentWriterLog, producer] {
            for (std::uint64_t index = 0; index < 500; ++index) {
                CameraStateWriterEvent event{};
                event.writerTransactionId = producer * 1000 + index;
                concurrentWriterLog.TryPush(event);
            }
        });
    }
    for (auto& producer : producers) producer.join();
    const auto concurrentCounters = concurrentWriterLog.Counters();
    const std::uint64_t attempted = 4 * 500;
    if (!Check(concurrentCounters.accepted + concurrentCounters.droppedFull +
            concurrentCounters.droppedContended == attempted,
            "Concurrent writer log did not account for every attempted event")) return 1;
    std::uint64_t concurrentPopped = 0;
    std::uint64_t lastSequence = 0;
    while (concurrentWriterLog.TryPop(readEvent)) {
        if (!Check(readEvent.eventSequence > lastSequence,
                "Concurrent writer log event order was not monotonic")) return 1;
        lastSequence = readEvent.eventSequence;
        ++concurrentPopped;
    }
    if (!Check(concurrentPopped == concurrentCounters.accepted,
            "Concurrent writer log accepted count did not match drained events")) return 1;

    PostExitBlendObservation blend;
    PostExitBlendSample sample{};
    sample.context = 0x1000; sample.receiver = 0x2000; sample.thread = 7;
    sample.raw = sample.argument = sample.finalArgument = 70.0f;
    sample.start = 70.0f; sample.target = 90.3115f; sample.readable = 3;
    if (!Check(!blend.Observe(sample), "Unarmed blend trace accepted a sample")) return 1;
    blend.Begin(1, 0x2000, 90.3115f);
    if (!Check(blend.Observe(sample) != nullptr && !blend.Writer(0x2000, 8) &&
            !blend.Writer(0x3000, 7) && blend.Writer(0x2000, 7) != nullptr &&
            !blend.Writer(0x2000, 7), "Blend pairing lost camera/thread/pending isolation")) return 1;
    for (std::size_t i = 0; i < PostExitBlendObservation::PairLimit; ++i) {
        sample.context += 0x100;
        blend.Observe(sample);
    }
    if (!Check(blend.Rejected() > 0, "Changing identities escaped fixed storage")) return 1;
    if (!Check(blend.Writer(0x2000, 7) != nullptr && !blend.Writer(0x2000, 7),
            "A later writer reused a superseded pending blend instance")) return 1;
    blend.End();
    if (!Check(!blend.Active() && !blend.Observe(sample), "Retired episode accepted a sample")) return 1;
    for (std::uint64_t generation = 2; generation < 10002; ++generation) {
        blend.Begin(generation, sample.receiver, sample.target);
        if (blend.TakeRecord()) {
            blend.Observe(sample);
            blend.Writer(sample.receiver, sample.thread);
        }
    }
    if (!Check(blend.Records() == PostExitBlendObservation::RecordLimit &&
            !blend.Active() && !blend.TakeRecord(), "New generations reset the session log budget")) return 1;
    PostExitBlendObservation silent;
    silent.Begin(1, sample.receiver, sample.target);
    sample.context = 0;
    for (int i = 0; i < 20000; ++i) silent.Observe(sample);
    if (!Check(silent.Callbacks() == PostExitBlendObservation::CallbackLimit &&
            !silent.Active(), "Rejected callbacks escaped the observation work bound")) return 1;

    cinematics::CinematicWriterAssociationSnapshot empty{};
    cinematics::CinematicWriterAssociationSnapshot before{};
    before.latestWriter = { 40, 7, 0x1000, 0x2000 };
    before.generationActive = true;
    before.generation = 12;
    const cinematics::CinematicRecoveryOwnerSnapshot owner{
        true, 11, 0x1000, 0x2000, 39, false };

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
            owner.lastAdmittedWriterOutput == newOwner.lastAdmittedWriterOutput,
            "Diagnostic event creation must leave writer and recovery snapshots unchanged")) return 1;

    WriterDiagnosticAggregation aggregate;
    aggregate.Begin(12, 0x1000, 1);
    for (unsigned i = 0; i < 10000; ++i) {
        const auto from = i % 2 ? 0x2000 : 0x3000;
        const auto to = i % 2 ? 0x3000 : 0x2000;
        aggregate.RecordOutputTransition(from, to);
        aggregate.RecordSample(to, 90.0f + static_cast<float>(i) / 10000.0f,
            127.0f, i < 5000, i == 0 || i == 5000);
    }
    const auto summary = aggregate.Finish();
    if (!Check(summary.outputTransitions == 10000 && summary.samples == 10000 &&
            summary.applied == 5000 && summary.fullRecords == 2 &&
            summary.firstOutput == 0x3000 && summary.lastOutput == 0x3000 &&
            summary.minimumInput == 90.0f && summary.maximumInput > 90.99f &&
            summary.generation == 12 && summary.source == 0x1000,
            "Aggregation lost transitions, exact decisions or numeric control samples")) return 1;
    if (!Check(!aggregate.Finish().HasRecords(), "Aggregation flush duplicated counters")) return 1;
    aggregate.Begin(13, 0x4000, 2);
    aggregate.RecordSample(0x5000, 90.0f, 90.0f, false, true);
    const auto next = aggregate.Finish();
    if (!Check(next.generation == 13 && next.source == 0x4000 && next.samples == 1 &&
            next.outputTransitions == 0 && next.applied == 0 && next.fullRecords == 1,
            "Generation/source boundary retained old aggregation state")) return 1;

    std::cout << "PASS: Diagnostic OFF isolation, pair-change gating, context snapshot, and state neutrality\n";
    return 0;
}
