#include "../../src/plugin/camera_writer_transaction.hpp"

#include <cmath>
#include <iostream>

namespace
{
    bool Check(bool value, const char* label)
    {
        if (!value) std::cerr << "FAIL: " << label << '\n';
        return value;
    }

    void SelectMode(void* context, config::GameplayMode mode)
    {
        static_cast<std::atomic<config::GameplayMode>*>(context)->store(
            mode, std::memory_order_release);
    }
}

int main()
{
    plugin::CameraCorrectionGate gate;
    std::atomic<config::GameplayMode> gameplayMode{config::GameplayMode::HorPlus};
    std::atomic<bool> gameplayEnabled{true};
    std::atomic<std::uint64_t> gameplayEpoch{3};
    std::atomic<bool> gameplayEnablePending{};
    std::atomic<bool> gameplayDisablePending{};
    std::atomic<bool> gameplayTransitionPending{true};
    std::atomic<bool> gameplayTransitionDeferralLogged{true};
    std::atomic<bool> cinematicsEnabled{true};
    std::atomic<std::uint64_t> cinematicsEpoch{7};
    std::atomic<bool> activeCinematicsEnabled{true};
    std::atomic<std::uint64_t> activeCinematicsEpoch{7};
    std::atomic<bool> cinematicFovApplied{true};
    std::atomic<float> cinematicTarget{127.0f};
    std::atomic<float> exitCinematicTarget{126.0f};
    std::atomic<bool> continuationPending{true};
    std::atomic<bool> continuationCarryPending{true};
    std::atomic<bool> continuationInterpolation{true};
    std::atomic<std::uintptr_t> continuationSource{0xA};
    std::atomic<bool> dialogueEnabled{true};
    std::atomic<config::DialogueZoomPolicy> dialoguePolicy{
        config::DialogueZoomPolicy::Adaptive};
    std::atomic<std::uint64_t> dialogueEpoch{9};
    std::atomic<std::uint64_t> activeDialogueEpoch{9};
    bool dialogueNonNativeCapabilityAvailable{true};
    std::atomic<config::CinematicAspectPolicy> cinematicAspectPolicy{
        config::CinematicAspectPolicy::Auto};
    std::atomic<config::CinematicFovMode> cinematicFovMode{
        config::CinematicFovMode::NativeHorPlus};
    dialogue::CorrectionActivation dialogueActivation;
    dialogueActivation.AcceptedStart(true);

    auto view = [&] {
        return plugin::CameraSettingStateView{gameplayMode, gameplayEnabled, gameplayEpoch,
            gameplayEnablePending, gameplayDisablePending,
            gameplayTransitionPending, gameplayTransitionDeferralLogged,
            cinematicsEnabled, cinematicsEpoch, activeCinematicsEnabled,
            activeCinematicsEpoch, cinematicFovApplied, cinematicTarget,
            exitCinematicTarget, continuationPending, continuationCarryPending,
            continuationInterpolation,
            continuationSource, dialogueEnabled, dialoguePolicy, dialogueEpoch,
            activeDialogueEpoch, dialogueNonNativeCapabilityAvailable,
            cinematicAspectPolicy, cinematicFovMode,
            dialogueActivation, &gameplayMode, &SelectMode};
    };
    bool pass = true;
    bool traceFlushCalled = false;

    auto gameplayOff = plugin::ApplyCameraSetting(gate, view(),
        plugin::RuntimeSettingMutation::GameplayEnabled(false), true, [] {});
    pass &= Check(gameplayOff.handled && gameplayOff.mutation.accepted &&
        gameplayOff.mutation.changed && !gameplayEnabled.load() &&
        gameplayEpoch.load() == 4 && gameplayDisablePending.load() &&
        !gameplayEnablePending.load() && !gameplayTransitionPending.load(),
        "gameplay OFF uses shared publication transaction and disarms pending correction");
    auto gameplayOn = plugin::ApplyCameraSetting(gate, view(),
        plugin::RuntimeSettingMutation::GameplayEnabled(true), true, [] {});
    pass &= Check(gameplayOn.mutation.accepted && gameplayEnabled.load() &&
        gameplayEpoch.load() == 5 && gameplayEnablePending.load() &&
        !gameplayDisablePending.load(),
        "gameplay ON schedules a fresh writer-boundary application");

    auto cinematicOff = plugin::ApplyCameraSetting(gate, view(),
        plugin::RuntimeSettingMutation::CinematicsEnabled(false), true,
        [&] { traceFlushCalled = true; });
    pass &= Check(cinematicOff.mutation.changed && cinematicOff.flushCinematicTrace &&
        traceFlushCalled && !activeCinematicsEnabled.load() &&
        activeCinematicsEpoch.load() == 0 && !cinematicFovApplied.load() &&
        std::isnan(cinematicTarget.load()) && std::isnan(exitCinematicTarget.load()) &&
        !continuationPending.load() && !continuationCarryPending.load() &&
        !continuationInterpolation.load() &&
        continuationSource.load() == 0,
        "cinematics OFF invalidates correction state but reports lifecycle observation separately");
    auto cinematicOn = plugin::ApplyCameraSetting(gate, view(),
        plugin::RuntimeSettingMutation::CinematicsEnabled(true), true, [] {});
    pass &= Check(cinematicOn.mutation.changed && !activeCinematicsEnabled.load() &&
        !cinematicFovApplied.load(),
        "cinematics ON does not retroactively reactivate the current lifecycle");

    auto dialogueOff = plugin::ApplyCameraSetting(gate, view(),
        plugin::RuntimeSettingMutation::DialogueEnabled(false), true, [] {});
    auto dialogueOn = plugin::ApplyCameraSetting(gate, view(),
        plugin::RuntimeSettingMutation::DialogueEnabled(true), true, [] {});
    pass &= Check(dialogueOff.mutation.changed && dialogueOn.mutation.changed &&
        dialogueEpoch.load() == 11 && activeDialogueEpoch.load() == 0 &&
        !dialogueActivation.CanCorrect(true),
        "dialogue OFF then ON leaves current lifecycle correction disarmed");
    dialogueActivation.AcceptedStart(true);
    pass &= Check(dialogueActivation.CanCorrect(true),
        "next accepted dialogue lifecycle can activate after ON");

    const auto modeChanged = plugin::ApplyCameraSetting(gate, view(),
        plugin::RuntimeSettingMutation::Gameplay(config::GameplayMode::AspectRecalculation),
        true, [] {});
    const auto modeNoop = plugin::ApplyCameraSetting(gate, view(),
        plugin::RuntimeSettingMutation::Gameplay(config::GameplayMode::AspectRecalculation),
        true, [] {});
    const auto aspectChanged = plugin::ApplyCameraSetting(gate, view(),
        plugin::RuntimeSettingMutation::CinematicAspect(
            config::CinematicAspectPolicy::Forced21x9), true, [] {});
    const auto fovModeChanged = plugin::ApplyCameraSetting(gate, view(),
        plugin::RuntimeSettingMutation::CinematicFov(
            config::CinematicFovMode::GameplayHorPlus), true, [] {});
    const auto dialoguePolicyChanged = plugin::ApplyCameraSetting(gate, view(),
        plugin::RuntimeSettingMutation::Dialogue(config::DialogueZoomPolicy::Reduced),
        true, [] {});
    const auto dialogueNativeAccepted = plugin::ApplyCameraSetting(gate, view(),
        plugin::RuntimeSettingMutation::Dialogue(config::DialogueZoomPolicy::Native),
        true, [] {});
    pass &= Check(modeChanged.mutation.accepted && modeChanged.mutation.changed &&
        modeNoop.mutation.accepted && !modeNoop.mutation.changed &&
        gameplayMode.load() == config::GameplayMode::AspectRecalculation &&
        gameplayEpoch.load() == 5,
        "Gameplay mode publication preserves existing settings epoch semantics");
    pass &= Check(aspectChanged.mutation.changed && fovModeChanged.mutation.changed &&
        cinematicAspectPolicy.load() == config::CinematicAspectPolicy::Forced21x9 &&
        cinematicFovMode.load() == config::CinematicFovMode::GameplayHorPlus &&
        cinematicsEpoch.load() == 9,
        "Cinematic aspect and FOV updates preserve existing epoch semantics");
    pass &= Check(dialoguePolicyChanged.mutation.changed &&
        dialogueNativeAccepted.handled && dialogueNativeAccepted.mutation.accepted &&
        dialoguePolicy.load() == config::DialogueZoomPolicy::Native &&
        dialogueEpoch.load() == 11,
        "Dialogue Native remains accepted and policy settings do not alter epoch semantics");
    dialogueNonNativeCapabilityAvailable = false;
    const auto unsupportedDialoguePolicy = plugin::ApplyCameraSetting(gate, view(),
        plugin::RuntimeSettingMutation::Dialogue(config::DialogueZoomPolicy::Reduced),
        true, [] {});
    pass &= Check(unsupportedDialoguePolicy.handled &&
        !unsupportedDialoguePolicy.mutation.accepted &&
        dialoguePolicy.load() == config::DialogueZoomPolicy::Native,
        "non-native Dialogue policy remains rejected when boundary capability is unavailable");

    std::cout << (pass ? "PASS" : "FAIL") << ": camera setting transaction\n";
    return pass ? 0 : 1;
}
