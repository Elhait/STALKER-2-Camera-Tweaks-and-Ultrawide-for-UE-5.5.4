#include "../../src/plugin/dialogue_boundary_transaction.hpp"
#include "../../src/plugin/camera_writer_transaction.hpp"

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

namespace
{
    bool Check(bool value, const char* label)
    {
        if (!value) std::cerr << "FAIL: " << label << '\n';
        return value;
    }

    struct Fixture
    {
        std::mutex mutex;
        std::atomic<camera::CoordinatorState> coordinator{camera::CoordinatorState::Gameplay};
        dialogue::PostCinematicRecoveryExclusion exclusion;
        dialogue::Phase phase{dialogue::Phase::Inactive};
        dialogue::PolicySnapshot activePolicy;
        dialogue::CorrectionActivation activation;
        float baseline{NAN};
        float previous{NAN};
        float exitStart{NAN};
        dialogue::CandidateTracker candidate;
        dialogue::RecoveryRearm recovery;
        std::atomic<bool> enabled{true};
        std::atomic<config::DialogueZoomPolicy> selectedPolicy{
            config::DialogueZoomPolicy::Adaptive};
        std::atomic<std::uint64_t> settingsEpoch{1};
        std::atomic<std::uint64_t> activeEpoch{};
        bool dialogueNonNativeCapabilityAvailable{true};
        std::atomic<std::uintptr_t> snapshotSource{};
        std::atomic<float> snapshotTarget{NAN};
        std::atomic<bool> snapshotTargetValid{};

        plugin::DialogueBoundaryStateView View()
        {
            return {mutex, coordinator, exclusion, phase, activePolicy, activation,
                baseline, previous, exitStart, candidate, recovery, enabled,
                selectedPolicy, settingsEpoch, activeEpoch, snapshotSource,
                snapshotTarget, snapshotTargetValid};
        }

        plugin::DialogueBoundaryResult Event(float fov, std::uintptr_t source,
            float target = 70.0f, bool targetValid = true)
        {
            auto gate = correctionGate.Acquire();
            std::lock_guard lock(mutex);
            return plugin::ProcessDialogueBoundary(View(),
                {fov, source, target, targetValid, 0.01f, 0.02f, 3});
        }

        plugin::CameraCorrectionGate correctionGate;
    };

    void SelectGameplayMode(void*, config::GameplayMode) {}

    struct SettingsFixture
    {
        std::atomic<config::GameplayMode> gameplayMode{config::GameplayMode::HorPlus};
        std::atomic<bool> gameplayEnabled{true};
        std::atomic<std::uint64_t> gameplayEpoch{};
        std::atomic<bool> gameplayEnablePending{};
        std::atomic<bool> gameplayDisablePending{};
        std::atomic<bool> gameplayTransitionPending{};
        std::atomic<bool> gameplayDeferralLogged{};
        std::atomic<bool> cinematicsEnabled{true};
        std::atomic<std::uint64_t> cinematicsEpoch{};
        std::atomic<bool> activeCinematicsEnabled{};
        std::atomic<std::uint64_t> activeCinematicsEpoch{};
        std::atomic<bool> cinematicFovApplied{};
        std::atomic<float> cinematicTarget{NAN};
        std::atomic<float> exitTarget{NAN};
        std::atomic<bool> continuationPending{};
        std::atomic<bool> continuationCarryPending{};
        std::atomic<bool> continuationActive{};
        std::atomic<std::uintptr_t> continuationSource{};
        std::atomic<config::CinematicAspectPolicy> aspectPolicy{
            config::CinematicAspectPolicy::Auto};
        std::atomic<config::CinematicFovMode> fovMode{
            config::CinematicFovMode::NativeHorPlus};

        plugin::CameraSettingStateView View(Fixture& dialogue)
        {
            return {gameplayMode, gameplayEnabled, gameplayEpoch,
                gameplayEnablePending, gameplayDisablePending,
                gameplayTransitionPending, gameplayDeferralLogged,
                cinematicsEnabled, cinematicsEpoch, activeCinematicsEnabled,
                activeCinematicsEpoch, cinematicFovApplied, cinematicTarget,
                exitTarget, continuationPending, continuationCarryPending,
                continuationActive,
                continuationSource, dialogue.enabled, dialogue.selectedPolicy,
                dialogue.settingsEpoch, dialogue.activeEpoch,
                dialogue.dialogueNonNativeCapabilityAvailable, aspectPolicy,
                fovMode, dialogue.activation, nullptr, &SelectGameplayMode};
        }
    };
}

int main()
{
    bool pass = true;
    Fixture state;
    const auto candidate = state.Event(100.0f, 0xA);
    const auto active = state.Event(99.8f, 0xA);
    const bool activationArmedAtStart = state.activation.CanCorrect(true);
    const auto activeB = state.Event(89.7f, 0xB);
    const auto exitingB = state.Event(100.2f, 0xB);
    const auto rearmedB = state.Event(70.0f, 0xB);
    if (active.action != plugin::DialogueBoundaryAction::AcceptedStart ||
        active.phaseAfter != dialogue::Phase::Active || !active.mutateXmm1)
        std::cerr << "dialogue_active action=" << static_cast<int>(active.action)
            << " phase=" << static_cast<int>(active.phaseAfter)
            << " mutate=" << active.mutateXmm1 << " output=" << active.outputFov
            << " policy=" << static_cast<int>(active.activePolicy)
        << " armed=" << activationArmedAtStart << '\n';
    pass &= Check(candidate.action == plugin::DialogueBoundaryAction::CandidateCaptured &&
        candidate.phaseAfter == dialogue::Phase::Candidate && !candidate.mutateXmm1,
        "candidate event captures lifecycle without changing XMM1");
    pass &= Check(active.action == plugin::DialogueBoundaryAction::AcceptedStart &&
        active.phaseAfter == dialogue::Phase::Active && active.mutateXmm1 &&
        activationArmedAtStart,
        "confirmed descent accepts Dialogue and returns production correction output");
    pass &= Check(activeB.phaseAfter == dialogue::Phase::Active && activeB.mutateXmm1 &&
        std::isfinite(activeB.outputFov) && activeB.outputFov != 89.7f,
        "active callback may move from accepted A to observed B without fixed-source gate");
    pass &= Check(exitingB.action == plugin::DialogueBoundaryAction::RecoveryStarted &&
        exitingB.phaseAtCallback == dialogue::Phase::Exiting &&
        exitingB.phaseAfter == dialogue::Phase::RearmPending &&
        exitingB.recoverySource == 0xB && !exitingB.mutateXmm1,
        "native A-to-B transition preserves Exiting then rearms on observed B");
    pass &= Check(rearmedB.action == plugin::DialogueBoundaryAction::RecoveryCompleted &&
        rearmedB.phaseAfter == dialogue::Phase::Inactive &&
        !rearmedB.mutateXmm1,
        "RecoveryRearm completion clears lifecycle without changing native XMM1");

    Fixture offStarted;
    offStarted.enabled.store(false);
    const auto offCandidate = offStarted.Event(90.0f, 0xA);
    const auto offActive = offStarted.Event(89.8f, 0xA);
    offStarted.enabled.store(true);
    const auto onMidLifecycle = offStarted.Event(89.0f, 0xA);
    pass &= Check(offCandidate.action == plugin::DialogueBoundaryAction::CandidateCaptured &&
        offActive.action == plugin::DialogueBoundaryAction::AcceptedStart &&
        !offActive.mutateXmm1 && onMidLifecycle.phaseAfter == dialogue::Phase::Active &&
        !onMidLifecycle.mutateXmm1,
        "OFF observation remains active and ON does not retroactively arm lifecycle");

    Fixture candidateMismatch;
    static_cast<void>(candidateMismatch.Event(90.0f, 0xA));
    const auto rejectedCandidate = candidateMismatch.Event(89.8f, 0xB);
    if (rejectedCandidate.action != plugin::DialogueBoundaryAction::CandidateCancelled)
        std::cerr << "dialogue_mismatch action=" << static_cast<int>(rejectedCandidate.action)
            << " phase=" << static_cast<int>(rejectedCandidate.phaseAfter) << '\n';
    pass &= Check(rejectedCandidate.action == plugin::DialogueBoundaryAction::CandidateCancelled &&
        candidateMismatch.phase == dialogue::Phase::Inactive &&
        !rejectedCandidate.mutateXmm1,
        "candidate context mismatch cancels only pending Candidate state");

    Fixture invalidTarget;
    const auto rejectedTarget = invalidTarget.Event(90.0f, 0xA, NAN, false);
    pass &= Check(rejectedTarget.action == plugin::DialogueBoundaryAction::CandidateRejected &&
        invalidTarget.phase == dialogue::Phase::Inactive &&
        !invalidTarget.snapshotTargetValid.load(),
        "unreadable native target fails closed without candidate ownership");

    // Dialogue OFF-first: use the production setting transaction, then the
    // same event decision path. Lifecycle observation is retained; XMM1 is not.
    Fixture offFirst;
    SettingsFixture settings;
    auto off = plugin::ApplyCameraSetting(offFirst.correctionGate, settings.View(offFirst),
        plugin::RuntimeSettingMutation::DialogueEnabled(false), true, [] {});
    const auto offFirstCandidate = offFirst.Event(90.0f, 0xA);
    const auto offFirstActive = offFirst.Event(89.8f, 0xA);
    pass &= Check(off.mutation.accepted && !offFirst.enabled.load() &&
        offFirstCandidate.action == plugin::DialogueBoundaryAction::CandidateCaptured &&
        offFirstActive.action == plugin::DialogueBoundaryAction::AcceptedStart &&
        !offFirstActive.mutateXmm1 && offFirst.phase == dialogue::Phase::Active,
        "Dialogue OFF-first is observation-only while real lifecycle phase advances");

    // Deterministic writer-first ordering: pause after the actual correction
    // transaction owns the production gate; OFF cannot return until XMM1 and
    // lifecycle output have committed.
    Fixture writerFirst;
    static_cast<void>(writerFirst.Event(100.0f, 0xA));
    static_cast<void>(writerFirst.Event(99.8f, 0xA));
    SettingsFixture writerFirstSettings;
    std::mutex rendezvousMutex;
    std::condition_variable rendezvous;
    bool writerPaused = false;
    bool releaseWriter = false;
    bool offAttempted = false;
    std::atomic<bool> offReturned{};
    plugin::DialogueBoundaryResult inFlight{};
    float committedXmm1 = 89.0f;
    std::thread writer([&] {
        auto gate = writerFirst.correctionGate.Acquire();
        std::lock_guard dialogueLock(writerFirst.mutex);
        {
            std::unique_lock lock(rendezvousMutex);
            writerPaused = true;
            rendezvous.notify_all();
            rendezvous.wait(lock, [&] { return releaseWriter; });
        }
        inFlight = plugin::ProcessDialogueBoundary(writerFirst.View(),
            {89.0f, 0xA, 70.0f, true, 0.01f, 0.02f, 3});
        if (inFlight.mutateXmm1) committedXmm1 = inFlight.outputFov;
    });
    {
        std::unique_lock lock(rendezvousMutex);
        rendezvous.wait(lock, [&] { return writerPaused; });
    }
    std::thread disable([&] {
        {
            std::lock_guard lock(rendezvousMutex);
            offAttempted = true;
            rendezvous.notify_all();
        }
        const auto result = plugin::ApplyCameraSetting(writerFirst.correctionGate,
            writerFirstSettings.View(writerFirst),
            plugin::RuntimeSettingMutation::DialogueEnabled(false), true, [] {});
        offReturned.store(result.mutation.accepted, std::memory_order_release);
    });
    {
        std::unique_lock lock(rendezvousMutex);
        rendezvous.wait(lock, [&] { return offAttempted; });
        pass &= Check(!offReturned.load(std::memory_order_acquire),
            "OFF cannot return while Dialogue production transaction holds correction gate");
        releaseWriter = true;
        rendezvous.notify_all();
    }
    writer.join();
    disable.join();
    if (!inFlight.mutateXmm1 || committedXmm1 == 89.0f ||
        !offReturned.load() || writerFirst.activation.CanCorrect(true))
        std::cerr << "dialogue_writer_first mutate=" << inFlight.mutateXmm1
            << " committed=" << committedXmm1 << " off=" << offReturned.load()
            << " armed=" << writerFirst.activation.CanCorrect(true)
            << " phase=" << static_cast<int>(writerFirst.phase) << '\n';
    pass &= Check(inFlight.mutateXmm1 && committedXmm1 != 89.0f &&
        offReturned.load() && !writerFirst.activation.CanCorrect(true),
        "writer-first correction commits before OFF, and OFF disarms before returning");

    // Cinematics setting publication shares the same correction gate and
    // retains the already-established deferred-activation semantics.
    Fixture cinematicSettingState;
    SettingsFixture cinematicSettings;
    const auto cinematicOff = plugin::ApplyCameraSetting(
        cinematicSettingState.correctionGate, cinematicSettings.View(cinematicSettingState),
        plugin::RuntimeSettingMutation::CinematicsEnabled(false), true, [] {});
    const auto cinematicOn = plugin::ApplyCameraSetting(
        cinematicSettingState.correctionGate, cinematicSettings.View(cinematicSettingState),
        plugin::RuntimeSettingMutation::CinematicsEnabled(true), true, [] {});
    pass &= Check(cinematicOff.mutation.changed && cinematicOn.mutation.changed &&
        !cinematicSettings.activeCinematicsEnabled.load() &&
        !cinematicSettings.cinematicFovApplied.load(),
        "Cinematics OFF/ON does not retroactively activate the old generation");

    std::cout << (pass ? "PASS" : "FAIL") << ": Dialogue boundary transaction\n";
    return pass ? 0 : 1;
}
