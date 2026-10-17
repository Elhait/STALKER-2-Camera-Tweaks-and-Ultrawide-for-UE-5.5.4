#include "runtime.hpp"
#include "runtime_internal.hpp"
#include "../overlay/composition/composition_runtime.hpp"
#include "optional_overlay_startup.hpp"
#include "runtime_settings.hpp"
#include "camera_correction_gate.hpp"
#include "native_callback_admission.hpp"
#include "camera_writer_transaction.hpp"
#include "dialogue_boundary_transaction.hpp"
#include "cinematic_lifecycle_transaction.hpp"
#include "../config/feature_config.hpp"
#include "../config/config_repository.hpp"
#include "../config/config_template.hpp"
#include "../hooks/signatures/signature_definitions.hpp"
#include "../hooks/signature_scanner.hpp"
#include "../hooks/instruction_validator.hpp"
#include "../hooks/hook_set.hpp"
#include "../gameplay/gameplay_state.hpp"
#include "../gameplay/gameplay_camera.hpp"
#include "../gameplay/horplus_gameplay.hpp"
#include "../gameplay/aspect_policy.hpp"
#include "../cinematics/cinematic_initialization.hpp"
#include "../cinematics/cinematic_fov.hpp"
#include "../cinematics/cinematic_exit_authority.hpp"
#include "../cinematics/cinematic_recovery_retirement.hpp"
#include "../cinematics/cinematic_selection.hpp"
#include "../diagnostics/matchgameplay_prediction.hpp"
#include "../cinematics/cinematic_aspect.hpp"
#include "../dialogue/dialogue_fov.hpp"
#include "../dialogue/dialogue_state.hpp"
#include "../diagnostics/diagnostic_runtime.hpp"
#include "../diagnostics/camera_writer_identity.hpp"
#include "../diagnostics/camera_log_probe.hpp"
#include "../diagnostics/cinematic_continuation_trace.hpp"
#include "../diagnostics/cinematic_baseline_provenance.hpp"
#include "../diagnostics/dialogue_source_provenance.hpp"
#include "../diagnostics/cinematic_lifecycle_context.hpp"
#include "../diagnostics/startup_journal.hpp"
#include "../diagnostics/startup_timeline.hpp"
#include "../diagnostics/performance_telemetry.hpp"
#include "../camera/fov_observation.hpp"
#include "../camera/gameplay_baseline.hpp"
#include "../camera/gameplay_aspect_restoration.hpp"
#include "../camera/horplus.hpp"
#include "../camera/presentation_state.hpp"
#include "../camera/camera_state_snapshot.hpp"
#include "../camera/camera_state_publication.hpp"
#include "../camera/fov_semantics.hpp"
#include "../platform/win32/memory.hpp"
#include "../platform/win32/sha256.hpp"
#include "../platform/win32/viewport.hpp"
#include "../platform/win32/window.hpp"
#include "../platform/win32/native_log_sink.hpp"
#include "../platform/win32/module_residency.hpp"
#include <safetyhook.hpp>
#include <Zydis.h>
#include <bcrypt.h>
#include <spdlog/logger.h>

#include <atomic>
#include <array>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <deque>
#include <filesystem>
#include <initializer_list>
#include <limits>
#include <memory>
#include <new>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

#ifdef OVERLAY_COMBINED
extern "C" bool StartOverlayDiscovery(HMODULE module);
extern "C" void NotifyOverlayCameraCoreReady();
extern "C" void NotifyOverlayCameraStatePublished() noexcept;
#endif

#if defined(POST_EXIT_RAW_TRACE_DIAGNOSTIC) || \
    defined(POST_EXIT_INTERPOLATED_FOV_PRODUCER_TRACE) || \
    defined(POST_EXIT_FOV_PRODUCER_STORE_TRACE) || \
    defined(POST_EXIT_CAMERA_FOV_WRITE_OWNER_TRACE) || \
    defined(POST_EXIT_FOV_STATE_CONSUMER_TRACE) || \
    defined(POST_EXIT_GAMEPLAY_OBSERVER_DIAGNOSTIC) || \
    defined(NATIVE_GAMEPLAY_BASELINE_DIAGNOSTIC) || \
    defined(POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST)
#define POST_EXIT_TRACE_STATE
#endif

namespace
{
    thread_local bool g_initializationFailureCleanupAttempted{};
    void CloseInitializationReadiness() noexcept;
    bool TryInitializationFailureCleanup() noexcept;

    void MarkCameraStateFreshness(
        std::initializer_list<camera::CameraStateFreshnessGroup> groups);
    void MarkCameraPresentationTransition(bool includeDialogue);

    plugin::RuntimeSettingsApi g_runtimeSettings;
    using plugin::FeatureStatus;
    using plugin::FeatureStatusName;
    using config::CinematicAspectPolicy;
    using config::DialogueZoomPolicy;
    using config::FeatureConfig;
    using config::CinematicAspectPolicyName;
    using config::DialogueZoomPolicyName;
    using config::HotkeyName;
    using config::NextCinematicAspectPolicy;
    using config::NextDialogueZoomPolicy;
    using config::ParseBool;
    using config::ParseCinematicAspectPolicy;
    using config::ParseDialogueZoomPolicy;
    using config::ParseHotkey;
    using config::Trim;
    using namespace hooks::signatures;
    using camera::CoordinatorState;
    using camera::CoordinatorStateName;
    using gameplay::ReplayState;
    using gameplay::ReplayStateName;
    using dialogue::Phase;
    using dialogue::PhaseName;
    using DialoguePhase = dialogue::Phase;
    constexpr auto DialoguePhaseName = dialogue::PhaseName;

    // Global integration copy; stable gameplay source remains untouched.
    constexpr std::uintptr_t kAspectOffset = 0x254;
    constexpr std::uintptr_t kFlagsOffset = 0x259;
    constexpr std::uintptr_t kCameraWorldFovOffset = 0x230;
    constexpr float kWideAspect = 32.0f / 9.0f;
    constexpr float kNativeAspect = 16.0f / 9.0f;
    constexpr std::uint8_t kCinematicStorePrefix[] = { 0xC7, 0x80, 0x54, 0x02, 0x00, 0x00 };
    constexpr std::uint8_t kCinematicOriginalImmediate[] = { 0x39, 0x8E, 0xE3, 0x3F };
    constexpr std::size_t kCinematicImmediateOffset = 6;
    constexpr std::size_t kCinematicStoreInstructionLength = 10;
    constexpr float kRecoveryEndpointToleranceDegrees = 0.01f;
    constexpr float kRecoveryInterpolationRangeEpsilon = 0.01f;
    constexpr std::size_t kDialogueBoundaryHookOffset = 9;
    constexpr float kDialogueTransformEpsilon = 0.01f;
    constexpr std::uint32_t kDialogueRecoveryStableSamples = 2;
    constexpr std::uint32_t kDialogueCandidateStableSamples = 3;
    constexpr float kDialogueContextFovJump = 5.0f;


}

namespace plugin::detail
{
    // RuntimeState remains defined only here; runtime_internal.hpp exposes
    // only a private, typed reference to the single process-resident owner.
    struct RuntimeState
    {
        HMODULE module{};
        HMODULE executable = GetModuleHandle(nullptr);
        plugin::WorkerLifecycle workers;
        std::uint8_t* fovWriteAddress{};
        hooks::HookSet hooks;
        std::shared_ptr<spdlog::logger> logger;
        std::mutex notificationMutex;
        std::deque<plugin::OverlayNotification> pendingNotifications;
        std::uint64_t nextNotificationId{};
        config::FeatureConfig config{};
        std::mutex hotkeyConfigMutex;
        // Serializes camera correction callbacks with camera-setting commits.
        // Lock order is cameraCorrectionGate -> dialogueMutex -> cameraStateMutex.
        plugin::CameraCorrectionGate cameraCorrectionGate;
        plugin::NativeCallbackAdmission nativeCallbackAdmission;
        std::atomic<std::uint64_t> gameplaySettingsEpoch{};
        std::atomic<std::uint64_t> cinematicsSettingsEpoch{};
        std::atomic<std::uint64_t> dialogueSettingsEpoch{};
        std::atomic<std::uint64_t> activeCinematicSettingsEpoch{};
        std::atomic<std::uint64_t> activeDialogueSettingsEpoch{};
        std::atomic<bool> hotkeyRebindCaptureActive{};
        std::atomic<int> hotkeyRebindConsumedKey{};
        std::atomic<bool> gameplayRecoveryObservationAvailable{false};
        std::atomic<bool> runtimeInitializationComplete{false};
        std::atomic<bool> overlayStartupSucceeded{false};
        std::atomic<bool> runtimeGameplayEnabled{true};
        std::atomic<bool> gameplayEnableApplyPending{};
        std::atomic<bool> gameplayDisableRestorePending{};
        std::atomic<bool> gameplayEnabledTransitionDeferralLogged{};
        std::atomic<config::GameplayMode> runtimeGameplayMode{
            config::GameplayMode::HorPlus};
        std::atomic<bool> runtimeCinematicsEnabled{true};
        std::atomic<bool> activeCinematicsEnabled{true};
        std::atomic<bool> gameplayModeTransitionPending{};
        std::atomic<bool> gameplayModeTransitionDeferralLogged{};
        std::atomic<config::GameplayMode> gameplayModeTransitionTarget{
            config::GameplayMode::HorPlus};
        std::atomic<config::CinematicAspectPolicy> runtimeCinematicPolicy{
            config::CinematicAspectPolicy::Auto};
        std::atomic<config::CinematicFovMode> runtimeCinematicFovMode{
            config::CinematicFovMode::GameplayHorPlus};
        std::atomic<config::CinematicAspectPolicy> activeCinematicAspectPolicy{
            config::CinematicAspectPolicy::Auto};
        std::atomic<config::CinematicFovMode> activeCinematicFovMode{
            config::CinematicFovMode::GameplayHorPlus};
        std::atomic<std::uint64_t> cinematicSelectionGeneration{};
        std::atomic<bool> cinematicSelectionValid{};
        std::atomic<bool> cinematicExitIndexed{};
        std::atomic<std::uint64_t> cinematicExitLogProbeGeneration{};
        std::atomic<bool> cameraWriterHookGate{};
        std::atomic<bool> cinematicHookGate{};
        std::atomic<bool> stopping{};
        std::atomic<config::DialogueZoomPolicy> runtimeDialoguePolicy{
            config::DialogueZoomPolicy::Adaptive};
        std::atomic<bool> runtimeDialogueEnabled{true};
        std::atomic<const localization::LocaleDescriptor*> runtimeOverlayLocale{
            localization::CanonicalLocaleDescriptor()};
        std::atomic<bool> runtimeOverlayLocaleAuto{true};
        std::atomic<bool> runtimeOverlayAutoLocaleSynchronized{};
        std::atomic<int> runtimeOverlayFontSize{config::OverlayFontSizeDefault};
        std::atomic<bool> cinematicLifecycleObservationAvailable{};
        std::atomic<bool> cinematicAspectComponentAvailable{};
        std::atomic<bool> dialogueBoundaryHookAvailable{};
        std::atomic<bool> dialogueNonNativeCapabilityAvailable{};
        std::filesystem::path configPath;
        std::atomic<gameplay::ReplayState> state{
            gameplay::ReplayState::WaitingForAutomaticUpdate};
        std::atomic<std::uint32_t> lastCameraMode{};
        std::atomic<std::uint64_t> transitionTraceSequence{0};
        std::atomic<camera::CoordinatorState> coordinator{
            camera::CoordinatorState::Gameplay};
        camera::CameraFovObservationStore fovObservationStore{};
        camera::CameraTransitionBoundaryObservation cameraTransitionBoundary{};
        camera::GameplayBaselineStore gameplayBaselineStore{};
        cinematics::CinematicWriterAssociation cinematicWriterAssociation{};
        cinematics::CinematicRecoveryOwner cinematicRecoveryOwner{};
        std::unique_ptr<diagnostics::CinematicContinuationTrace> cinematicContinuationTrace{};
        camera::GameplayAspectRestorationStore gameplayAspectRestorationStore{};
        std::mutex cameraStateMutex;
        camera::CameraStateSnapshot cameraStateSnapshot{};
        bool cameraStateSnapshotValid{};
        float neutralGameplayFovCandidate{std::numeric_limits<float>::quiet_NaN()};
        std::uint32_t neutralGameplayFovSamples{};
        std::uintptr_t neutralGameplayFovCandidateSource{};
        std::atomic<bool> cinematicFovApplied{false};
        std::atomic<float> cinematicTransformedFov{std::numeric_limits<float>::quiet_NaN()};
        std::atomic<float> exitCinematicTransformedFov{std::numeric_limits<float>::quiet_NaN()};
        std::atomic<float> matchGameplayEnterNativeFov{std::numeric_limits<float>::quiet_NaN()};
        std::atomic<float> matchGameplayPreEnterNativeFov{std::numeric_limits<float>::quiet_NaN()};
        std::atomic<float> matchGameplayPreEnterHorPlusFov{std::numeric_limits<float>::quiet_NaN()};
        std::atomic<float> matchGameplayPreEnterAspect{std::numeric_limits<float>::quiet_NaN()};
        // Retained only for existing continuation/recovery source validation.
        // Initial target retention is authorized by the generation-scoped
        // Cinematics association, never by this Gameplay baseline source.
        std::atomic<std::uintptr_t> gameplayBaselineContinuationSource{};
        std::atomic<bool> cinematicContinuationPending{};
        std::atomic<bool> cinematicContinuationCarryPending{};
        std::atomic<std::uintptr_t> cinematicContinuationSource{};
        std::atomic<float> cinematicContinuationExitNativeTarget{
            std::numeric_limits<float>::quiet_NaN()};
        std::atomic<float> cinematicContinuationPreviousTarget{
            std::numeric_limits<float>::quiet_NaN()};
        std::atomic<bool> cinematicContinuationInterpolationActive{};
        std::atomic<float> cinematicContinuationStartFov{
            std::numeric_limits<float>::quiet_NaN()};
        std::atomic<bool> matchGameplayPreEnterPairValid{};
        std::atomic<float> cinematicEnterAspect{std::numeric_limits<float>::quiet_NaN()};
        std::atomic<float> exitTargetFov{0.0f};
        dialogue::PostCinematicRecoveryExclusion postCinematicDialogueExclusion{};
#ifdef POST_EXIT_TRACE_STATE
        std::atomic<bool> postExitTraceArmed{false};
        std::atomic<std::uint32_t> postExitTraceWriterCount{0};
        std::atomic<std::int64_t> postExitTraceStartNs{0};
        std::atomic<std::uint64_t> postExitTraceSequence{0};
#endif
#ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST
        std::atomic<bool> deferredGameplayReplayPending{false};
        std::atomic<std::uintptr_t> deferredGameplaySource{};
        std::atomic<std::uint32_t> deferredGameplayStableSamples{0};
        std::atomic<float> deferredGameplayLastFov{std::numeric_limits<float>::quiet_NaN()};
        std::atomic<std::int64_t> deferredGameplayStableSinceNs{0};
#endif
        std::atomic<bool> atomicExitHandoffPending{false};
        std::atomic<std::uintptr_t> atomicExitHandoffSource{};
        std::atomic<float> atomicExitHandoffPreviousFov{std::numeric_limits<float>::quiet_NaN()};
#ifdef POST_EXIT_DELAYED_ASPECT_DIAGNOSTIC
        std::atomic<bool> delayedAspectDiagnosticApplied{false};
#endif
#ifdef HORPLUS_GAMEPLAY_DIAGNOSTIC
        std::atomic<std::uint64_t> horPlusDiagnosticWriterCallbacks{0};
        std::atomic<std::uint64_t> horPlusDiagnosticApplications{0};
        std::atomic<std::uint64_t> horPlusDiagnosticChanges{0};
#endif
#ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC
        std::atomic<bool> dialogueDiscoveryActive{false};
        std::atomic<std::int64_t> dialogueDiscoveryStartNs{0};
        std::atomic<std::uint64_t> dialogueDiscoverySamples{0};
        std::atomic<std::uint64_t> dialogueDiscoveryChanges{0};
        std::atomic<std::uint64_t> dialogueDiscoveryResetGeneration{0};
#endif
        SafetyHookMid zoomInHook;
        SafetyHookMid zoomOutHook;
        std::atomic<std::uint64_t> zoomInHits{0};
        std::atomic<std::uint64_t> zoomOutHits{0};
        std::atomic<std::uint64_t> zoomLoggedEdges{0};
        std::atomic<std::uint8_t> snapshotZoomDirection{};
        std::atomic<std::uint64_t> snapshotZoomSequence{};
        std::atomic<std::uintptr_t> snapshotZoomSource{};
        std::atomic<float> snapshotZoomPrimary{std::numeric_limits<float>::quiet_NaN()};
        std::atomic<float> snapshotZoomSecondary{std::numeric_limits<float>::quiet_NaN()};
        std::atomic<bool> snapshotZoomActive{};
        std::atomic<bool> snapshotZoomObservationValid{};
        std::atomic<bool> snapshotZoomHold{};
        std::atomic<std::uint64_t> snapshotEventSequence{};
        std::atomic<std::uintptr_t> snapshotDialogueSource{};
        std::atomic<float> snapshotDialogueTarget{std::numeric_limits<float>::quiet_NaN()};
        std::atomic<bool> snapshotDialogueTargetValid{};
        std::mutex horPlusFovTelemetryMutex;
        bool horPlusFovTelemetryValid{};
        bool horPlusGameplayCacheValid{};
        std::uintptr_t stableObservationSource{};
        float stableObservationFov{std::numeric_limits<float>::quiet_NaN()};
        std::uint32_t stableObservationSamples{};
        bool stableObservationReported{};
        float horPlusTelemetryNativeFov{std::numeric_limits<float>::quiet_NaN()};
        float horPlusTelemetryAspect{std::numeric_limits<float>::quiet_NaN()};
        float horPlusTelemetryResult{std::numeric_limits<float>::quiet_NaN()};
        std::uint8_t horPlusTelemetryFlags{};
        config::GameplayMode horPlusTelemetryMode{config::GameplayMode::AspectRecalculation};
        std::uint8_t horPlusTelemetryOwner{};
#ifdef POST_EXIT_CAMERA_FOV_WRITE_OWNER_TRACE
        std::atomic<std::uint32_t> postExitWriteOwnerHitCount{0};
#endif
#ifdef POST_EXIT_FOV_PRODUCER_STORE_TRACE
        std::atomic<std::uint32_t> postExitProducerStoreHitCount{0};
#endif
        std::uint8_t* cinematicAspectStore{};
        std::uint8_t cinematicOriginalImmediate[sizeof(std::uint8_t) * 4]{};
        bool cinematicAspectPatched{};
        std::atomic<std::uintptr_t> lastAutoRestoreSource{};
        std::atomic<std::uintptr_t> lastGameplayCameraSource{};
        std::atomic<float> lastGameplayCameraFov{std::numeric_limits<float>::quiet_NaN()};
        std::uintptr_t lastLoggedFovSource{};
        float lastLoggedWriterInputFov = std::numeric_limits<float>::quiet_NaN();
        float lastLoggedCameraWorldFov = std::numeric_limits<float>::quiet_NaN();
        float lastLoggedCameraFirstPersonFov = std::numeric_limits<float>::quiet_NaN();
        std::mutex dialogueMutex;
        dialogue::Phase dialoguePhase{dialogue::Phase::Inactive};
        dialogue::PolicySnapshot activeDialoguePolicy{};
        dialogue::CorrectionActivation dialogueCorrectionActivation{};
        float dialogueBaseline = std::numeric_limits<float>::quiet_NaN();
        float dialoguePrevious = std::numeric_limits<float>::quiet_NaN();
        float dialogueExitIncomingStart{std::numeric_limits<float>::quiet_NaN()};
        dialogue::CandidateTracker dialogueCandidate{};
        dialogue::RecoveryRearm dialogueRecoveryRearm{};
    // The runtime is process-resident by contract. Deliberately allocate it
    // without registering a C++ static destructor so normal DLL teardown
    // cannot destroy hooks, mutexes or filesystem state under loader lock.
    };

}

namespace
{
    using plugin::detail::RuntimeState;

    RuntimeState& g_runtime = *new RuntimeState{};
    auto& g_notificationMutex = g_runtime.notificationMutex;
    auto& g_pendingNotifications = g_runtime.pendingNotifications;
    auto& g_nextNotificationId = g_runtime.nextNotificationId;

    diagnostics::CinematicContinuationTrace* GetCinematicContinuationTrace() noexcept
    {
        return g_runtime.cinematicContinuationTrace.get();
    }

    auto& g_module = g_runtime.module;
    auto& g_executable = g_runtime.executable;
    auto& g_workers = g_runtime.workers;
    auto& g_fovWriteAddress = g_runtime.fovWriteAddress;
    auto& g_hooks = g_runtime.hooks;
    auto& g_hook = g_hooks.gameplay;
    auto& g_logger = g_runtime.logger;
    auto& g_config = g_runtime.config;
    auto& g_hotkeyConfigMutex = g_runtime.hotkeyConfigMutex;
    auto& g_cameraCorrectionGate = g_runtime.cameraCorrectionGate;
    auto& g_nativeCallbackAdmission = g_runtime.nativeCallbackAdmission;
    auto& g_gameplaySettingsEpoch = g_runtime.gameplaySettingsEpoch;
    auto& g_cinematicsSettingsEpoch = g_runtime.cinematicsSettingsEpoch;
    auto& g_dialogueSettingsEpoch = g_runtime.dialogueSettingsEpoch;
    auto& g_activeCinematicSettingsEpoch = g_runtime.activeCinematicSettingsEpoch;
    auto& g_activeDialogueSettingsEpoch = g_runtime.activeDialogueSettingsEpoch;
    auto& g_hotkeyRebindCaptureActive = g_runtime.hotkeyRebindCaptureActive;
    auto& g_hotkeyRebindConsumedKey = g_runtime.hotkeyRebindConsumedKey;
    auto& g_gameplayRecoveryObservationAvailable =
        g_runtime.gameplayRecoveryObservationAvailable;
    auto& g_runtimeGameplayEnabled = g_runtime.runtimeGameplayEnabled;
    auto& g_gameplayEnableApplyPending = g_runtime.gameplayEnableApplyPending;
    auto& g_gameplayDisableRestorePending = g_runtime.gameplayDisableRestorePending;
    auto& g_gameplayEnabledTransitionDeferralLogged =
        g_runtime.gameplayEnabledTransitionDeferralLogged;
    auto& g_runtimeGameplayMode = g_runtime.runtimeGameplayMode;
    auto& g_runtimeCinematicsEnabled = g_runtime.runtimeCinematicsEnabled;
    auto& g_activeCinematicsEnabled = g_runtime.activeCinematicsEnabled;
    auto& g_gameplayModeTransitionPending = g_runtime.gameplayModeTransitionPending;
    auto& g_gameplayModeTransitionDeferralLogged = g_runtime.gameplayModeTransitionDeferralLogged;
    auto& g_gameplayModeTransitionTarget = g_runtime.gameplayModeTransitionTarget;
    auto& g_runtimeCinematicPolicy = g_runtime.runtimeCinematicPolicy;
    auto& g_runtimeCinematicFovMode = g_runtime.runtimeCinematicFovMode;
    auto& g_activeCinematicAspectPolicy = g_runtime.activeCinematicAspectPolicy;
    auto& g_activeCinematicFovMode = g_runtime.activeCinematicFovMode;
    auto& g_cinematicSelectionGeneration = g_runtime.cinematicSelectionGeneration;
    auto& g_cinematicSelectionValid = g_runtime.cinematicSelectionValid;
    auto& g_cinematicExitIndexed = g_runtime.cinematicExitIndexed;
    auto& g_cinematicExitLogProbeGeneration =
        g_runtime.cinematicExitLogProbeGeneration;
    auto& g_cameraWriterHookGate = g_runtime.cameraWriterHookGate;
    auto& g_cinematicHookGate = g_runtime.cinematicHookGate;
    auto& g_stopping = g_runtime.stopping;
    auto& g_runtimeDialoguePolicy = g_runtime.runtimeDialoguePolicy;
    auto& g_runtimeDialogueEnabled = g_runtime.runtimeDialogueEnabled;
    auto& g_runtimeOverlayLocale = g_runtime.runtimeOverlayLocale;
    auto& g_runtimeOverlayLocaleAuto = g_runtime.runtimeOverlayLocaleAuto;
    auto& g_runtimeOverlayAutoLocaleSynchronized =
        g_runtime.runtimeOverlayAutoLocaleSynchronized;
    auto& g_runtimeOverlayFontSize = g_runtime.runtimeOverlayFontSize;
    auto& g_cinematicLifecycleObservationAvailable =
        g_runtime.cinematicLifecycleObservationAvailable;
    auto& g_cinematicAspectComponentAvailable =
        g_runtime.cinematicAspectComponentAvailable;
    auto& g_dialogueBoundaryHookAvailable =
        g_runtime.dialogueBoundaryHookAvailable;
    auto& g_runtimeInitializationComplete = g_runtime.runtimeInitializationComplete;
    auto& g_dialogueNonNativeCapabilityAvailable =
        g_runtime.dialogueNonNativeCapabilityAvailable;
    auto& g_horPlusFovTelemetryMutex = g_runtime.horPlusFovTelemetryMutex;
    auto& g_configPath = g_runtime.configPath;
    auto& g_state = g_runtime.state;
    auto& g_lastCameraMode = g_runtime.lastCameraMode;
    auto& g_transitionTraceSequence = g_runtime.transitionTraceSequence;
    auto& g_coordinator = g_runtime.coordinator;
    auto& g_coordinatorState = g_runtime.coordinator;
    auto& g_fovObservationStore = g_runtime.fovObservationStore;
    auto& g_gameplayBaselineStore = g_runtime.gameplayBaselineStore;
    auto& g_cinematicWriterAssociation = g_runtime.cinematicWriterAssociation;
    auto& g_cinematicRecoveryOwner = g_runtime.cinematicRecoveryOwner;
    auto& g_gameplayAspectRestorationStore = g_runtime.gameplayAspectRestorationStore;
    auto& g_cameraStateMutex = g_runtime.cameraStateMutex;
    auto& g_cameraStateSnapshot = g_runtime.cameraStateSnapshot;
    auto& g_cameraStateSnapshotValid = g_runtime.cameraStateSnapshotValid;
    auto& g_neutralGameplayFovCandidate = g_runtime.neutralGameplayFovCandidate;
    auto& g_neutralGameplayFovSamples = g_runtime.neutralGameplayFovSamples;
    auto& g_neutralGameplayFovCandidateSource = g_runtime.neutralGameplayFovCandidateSource;
    auto& g_cinematicFovApplied = g_runtime.cinematicFovApplied;
    auto& g_cinematicTransformedFov = g_runtime.cinematicTransformedFov;
    auto& g_exitCinematicTransformedFov = g_runtime.exitCinematicTransformedFov;
    auto& g_cinematicEnterAspect = g_runtime.cinematicEnterAspect;
    auto& g_matchGameplayEnterNativeFov = g_runtime.matchGameplayEnterNativeFov;
    auto& g_matchGameplayPreEnterNativeFov = g_runtime.matchGameplayPreEnterNativeFov;
    auto& g_matchGameplayPreEnterHorPlusFov = g_runtime.matchGameplayPreEnterHorPlusFov;
    auto& g_matchGameplayPreEnterAspect = g_runtime.matchGameplayPreEnterAspect;
    auto& g_gameplayBaselineContinuationSource =
        g_runtime.gameplayBaselineContinuationSource;
    auto& g_cinematicContinuationPending = g_runtime.cinematicContinuationPending;
    auto& g_cinematicContinuationCarryPending =
        g_runtime.cinematicContinuationCarryPending;
    auto& g_cinematicContinuationSource = g_runtime.cinematicContinuationSource;
    auto& g_cinematicContinuationExitNativeTarget =
        g_runtime.cinematicContinuationExitNativeTarget;
    auto& g_cinematicContinuationPreviousTarget =
        g_runtime.cinematicContinuationPreviousTarget;
    auto& g_cinematicContinuationInterpolationActive =
        g_runtime.cinematicContinuationInterpolationActive;
    auto& g_cinematicContinuationStartFov = g_runtime.cinematicContinuationStartFov;
    auto& g_matchGameplayPreEnterPairValid = g_runtime.matchGameplayPreEnterPairValid;
    auto& g_exitTargetFov = g_runtime.exitTargetFov;
    auto& g_postCinematicDialogueExclusion = g_runtime.postCinematicDialogueExclusion;
#ifdef POST_EXIT_TRACE_STATE
    auto& g_postExitTraceArmed = g_runtime.postExitTraceArmed;
    auto& g_postExitTraceWriterCount = g_runtime.postExitTraceWriterCount;
    auto& g_postExitTraceStartNs = g_runtime.postExitTraceStartNs;
    auto& g_postExitTraceSequence = g_runtime.postExitTraceSequence;
#endif
#ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST
#ifndef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_DELAY_MS
#define POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_DELAY_MS 120
#endif
    constexpr std::uint32_t kDeferredGameplayStableSamples = 3;
    constexpr std::int64_t kDeferredGameplayDelayNs =
        static_cast<std::int64_t>(POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_DELAY_MS) * 1000000LL;
    auto& g_deferredGameplayReplayPending = g_runtime.deferredGameplayReplayPending;
    auto& g_deferredGameplaySource = g_runtime.deferredGameplaySource;
    auto& g_deferredGameplayStableSamples = g_runtime.deferredGameplayStableSamples;
    auto& g_deferredGameplayLastFov = g_runtime.deferredGameplayLastFov;
    auto& g_deferredGameplayStableSinceNs = g_runtime.deferredGameplayStableSinceNs;
#endif
    auto& g_atomicExitHandoffPending = g_runtime.atomicExitHandoffPending;
    auto& g_atomicExitHandoffSource = g_runtime.atomicExitHandoffSource;
    auto& g_atomicExitHandoffPreviousFov = g_runtime.atomicExitHandoffPreviousFov;
#ifdef POST_EXIT_DELAYED_ASPECT_DIAGNOSTIC
    auto& g_delayedAspectDiagnosticApplied = g_runtime.delayedAspectDiagnosticApplied;
#endif
#ifdef HORPLUS_GAMEPLAY_DIAGNOSTIC
    auto& g_horPlusDiagnosticWriterCallbacks = g_runtime.horPlusDiagnosticWriterCallbacks;
    auto& g_horPlusDiagnosticApplications = g_runtime.horPlusDiagnosticApplications;
    auto& g_horPlusDiagnosticChanges = g_runtime.horPlusDiagnosticChanges;
#endif
#ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC
    auto& g_dialogueDiscoveryActive = g_runtime.dialogueDiscoveryActive;
    auto& g_dialogueDiscoveryStartNs = g_runtime.dialogueDiscoveryStartNs;
    auto& g_dialogueDiscoverySamples = g_runtime.dialogueDiscoverySamples;
    auto& g_dialogueDiscoveryChanges = g_runtime.dialogueDiscoveryChanges;
    constexpr std::int64_t kDialogueDiscoveryDurationNs = 10'000'000'000LL;
#endif
    auto& g_zoomInHook = g_runtime.zoomInHook;
    auto& g_zoomOutHook = g_runtime.zoomOutHook;
    auto& g_zoomInHits = g_runtime.zoomInHits;
    auto& g_zoomOutHits = g_runtime.zoomOutHits;
    auto& g_zoomLoggedEdges = g_runtime.zoomLoggedEdges;
    auto& g_snapshotZoomActive = g_runtime.snapshotZoomActive;
    auto& g_snapshotZoomHold = g_runtime.snapshotZoomHold;
#ifdef POST_EXIT_CAMERA_FOV_WRITE_OWNER_TRACE
    auto& g_postExitWriteOwnerHitCount = g_runtime.postExitWriteOwnerHitCount;
#endif
#ifdef POST_EXIT_FOV_PRODUCER_STORE_TRACE
    auto& g_postExitProducerStoreHitCount = g_runtime.postExitProducerStoreHitCount;
#endif
    auto& g_cinematicEnterHook = g_hooks.cinematicEnter;
    auto& g_cinematicExitHook = g_hooks.cinematicExit;
    auto& g_cinematicAspectStore = g_runtime.cinematicAspectStore;
    auto& g_cinematicOriginalImmediate = g_runtime.cinematicOriginalImmediate;
    auto& g_cinematicAspectPatched = g_runtime.cinematicAspectPatched;
    auto& g_cinematicAspectStoreHook = g_hooks.cinematicAspectStore;
    auto& g_dialogueBoundaryHook = g_hooks.dialogueBoundary;
#ifdef POST_EXIT_GAMEPLAY_OBSERVER_DIAGNOSTIC
    auto& g_postExitGameplayObserverHook = g_hooks.postExitGameplayObserver;
#endif
#ifdef POST_EXIT_FOV_STATE_CONSUMER_TRACE
    auto& g_postExitConsumerHook = g_hooks.postExitConsumer;
    constexpr std::uintptr_t kPostExitConsumerRva = 0x318DCD4;
    constexpr char kPostExitConsumerGameSha256[] =
        "e7b481a97c02d80581fab0bece940214a88ebe30211088a00129845a039f9293";
#endif
#ifdef POST_EXIT_CAMERA_FOV_WRITE_OWNER_TRACE
    auto& g_postExitWriteOwnerHook = g_hooks.postExitWriteOwner;
    constexpr std::uintptr_t kPostExitWriteOwnerRva = 0x3DB2CE7;
    constexpr char kPostExitWriteOwnerGameSha256[] =
        "e7b481a97c02d80581fab0bece940214a88ebe30211088a00129845a039f9293";
#endif

#ifdef POST_EXIT_FOV_PRODUCER_STORE_TRACE
    auto& g_postExitProducerStoreHook = g_hooks.postExitProducerStore;
    constexpr std::uintptr_t kPostExitProducerStoreRva = 0x32B779D;
    constexpr char kPostExitProducerStoreGameSha256[] =
        "e7b481a97c02d80581fab0bece940214a88ebe30211088a00129845a039f9293";
#endif
    auto& g_lastAutoRestoreSource = g_runtime.lastAutoRestoreSource;
    auto& g_lastGameplayCameraSource = g_runtime.lastGameplayCameraSource;
    auto& g_lastGameplayCameraFov = g_runtime.lastGameplayCameraFov;
    auto& g_lastLoggedFovSource = g_runtime.lastLoggedFovSource;
    auto& g_lastLoggedWriterInputFov = g_runtime.lastLoggedWriterInputFov;
    auto& g_lastLoggedCameraWorldFov = g_runtime.lastLoggedCameraWorldFov;
    auto& g_lastLoggedCameraFirstPersonFov = g_runtime.lastLoggedCameraFirstPersonFov;
    auto& g_dialogueMutex = g_runtime.dialogueMutex;
    auto& g_dialoguePhase = g_runtime.dialoguePhase;
    auto& g_activeDialoguePolicy = g_runtime.activeDialoguePolicy;
    auto& g_dialogueCorrectionActivation = g_runtime.dialogueCorrectionActivation;
    auto& g_dialogueBaseline = g_runtime.dialogueBaseline;
    auto& g_dialoguePrevious = g_runtime.dialoguePrevious;
    auto& g_dialogueExitIncomingStart = g_runtime.dialogueExitIncomingStart;
    auto& g_dialogueCandidate = g_runtime.dialogueCandidate;
    auto& g_dialogueRecoveryRearm = g_runtime.dialogueRecoveryRearm;

    template <typename... Args>
    void Log(Args&&... args) noexcept;

    void InitializeCinematicContinuationDiagnostics(bool enabled)
    {
        if (!enabled) return;
        g_runtime.cinematicContinuationTrace.reset(
            new (std::nothrow) diagnostics::CinematicContinuationTrace{});
        if (!g_runtime.cinematicContinuationTrace)
            Log("Cinematic continuation diagnostics unavailable: trace storage allocation failed.");
    }

    bool WaitForWorkerStop(DWORD timeout)
    {
        return g_workers.WaitForStop(timeout);
    }

    bool StartWorker(LPTHREAD_START_ROUTINE entry, const char* name)
    {
        if (!g_workers.Start(entry)) {
            Log("Worker startup failed: ", name, ".");
            return false;
        }
        Log("Worker started: ", name, ".");
        return true;
    }

    bool StopWorkers(bool wait)
    {
        if (!wait) {
            g_workers.SignalStop();
            return true;
        }
        return g_workers.StopAndJoin();
    }

    bool CreateWorkerStopEvent()
    {
        return g_workers.Initialize();
    }

    diagnostics::DialogueSourceProvenanceTrace* GetDialogueSourceProvenanceTrace() noexcept
    {
        if (!diagnostics::Enabled()) return nullptr;
        // Diagnostic-only process-lifetime storage: no trace object or heap
        // allocation is created unless Diagnostic Mode is explicitly enabled.
        static auto* trace = new (std::nothrow)
            diagnostics::DialogueSourceProvenanceTrace{};
        return trace;
    }

    diagnostics::DialogueWriterSnapshot ReadDialogueWriterSnapshot()
    {
        const auto writer = g_cinematicWriterAssociation.Read().latestWriter;
        return { writer.sequence != 0, writer.sequence, writer.identityEpoch,
            writer.source, writer.output };
    }

    const char* DialogueSourceStageName(diagnostics::DialogueSourceStage stage) noexcept
    {
        switch (stage) {
        case diagnostics::DialogueSourceStage::Candidate: return "CandidateObservation";
        case diagnostics::DialogueSourceStage::Active: return "Active";
        case diagnostics::DialogueSourceStage::Exiting: return "Exiting";
        case diagnostics::DialogueSourceStage::Recovery: return "RecoveryRearm";
        case diagnostics::DialogueSourceStage::End: return "End";
        }
        return "Unknown";
    }

    void EmitDialogueSourceEvent(
        const std::optional<diagnostics::DialogueSourceEvent>& event)
    {
        if (!event) return;
        diagnostics::EmitIfEnabled([&] {
            Log(event->marker, " generation=", event->generation,
                " stage=", DialogueSourceStageName(event->stage),
                " owner=", event->owner ? event->owner : "unknown",
                " lifecycle_source_known=", event->lifecycleSourceKnown ? 1 : 0,
                " observed_lifecycle_RSI=0x", std::hex, event->lifecycleSource,
                " callback_or_recovery_RSI=0x", event->callbackSource, std::dec,
                " pointer_mismatch_known=", event->pointerMismatchKnown ? 1 : 0,
                " pointer_mismatch=", event->pointerMismatch ? 1 : 0,
                " relation=raw_pointer_comparison_only",
                " shared_writer_valid=", event->latestWriter.valid ? 1 : 0,
                " shared_writer_sequence=", event->latestWriter.sequence,
                " shared_writer_identity_epoch=", event->latestWriter.identityEpoch,
                " shared_writer_RSI=0x", std::hex, event->latestWriter.rsi,
                " shared_writer_RBX=0x", event->latestWriter.rbx, std::dec,
                " reason=", event->reason ? event->reason : "unspecified", ".");
        });
    }

    void RecordDialogueSourceCallback(DialoguePhase phase, std::uintptr_t callbackSource)
    {
        if (!diagnostics::Enabled()) return;
        auto* trace = GetDialogueSourceProvenanceTrace();
        if (!trace) return;
        const auto stage = phase == DialoguePhase::Active
            ? diagnostics::DialogueSourceStage::Active
            : phase == DialoguePhase::Exiting
                ? diagnostics::DialogueSourceStage::Exiting
                : phase == DialoguePhase::RearmPending
                    ? diagnostics::DialogueSourceStage::Recovery
                    : diagnostics::DialogueSourceStage::Candidate;
        const auto owner = CoordinatorStateName(
            g_coordinator.load(std::memory_order_acquire));
        EmitDialogueSourceEvent(trace->Callback(true, stage, callbackSource,
            owner, ReadDialogueWriterSnapshot()));
    }

    void RecordDialogueRecoverySource(std::uintptr_t recoverySource)
    {
        if (!diagnostics::Enabled()) return;
        auto* trace = GetDialogueSourceProvenanceTrace();
        if (!trace) return;
        const auto owner = CoordinatorStateName(
            g_coordinator.load(std::memory_order_acquire));
        EmitDialogueSourceEvent(trace->RecoveryStarted(true, recoverySource,
            owner, ReadDialogueWriterSnapshot()));
    }

    void RecordDialogueSourceEnd(const char* reason)
    {
        if (!diagnostics::Enabled()) return;
        auto* trace = GetDialogueSourceProvenanceTrace();
        if (!trace) return;
        const auto owner = CoordinatorStateName(
            g_coordinator.load(std::memory_order_acquire));
        EmitDialogueSourceEvent(trace->Ended(true, reason, owner,
            ReadDialogueWriterSnapshot()));
    }

    void RestoreCinematicAspect() noexcept;

    plugin::DialogueBoundaryStateView GetDialogueBoundaryStateView() noexcept
    {
        return {g_dialogueMutex, g_coordinator, g_postCinematicDialogueExclusion,
            g_dialoguePhase, g_activeDialoguePolicy, g_dialogueCorrectionActivation,
            g_dialogueBaseline, g_dialoguePrevious, g_dialogueExitIncomingStart,
            g_dialogueCandidate, g_dialogueRecoveryRearm, g_runtimeDialogueEnabled,
            g_runtimeDialoguePolicy, g_dialogueSettingsEpoch,
            g_activeDialogueSettingsEpoch, g_runtime.snapshotDialogueSource,
            g_runtime.snapshotDialogueTarget, g_runtime.snapshotDialogueTargetValid};
    }

    void ResetDialogueRuntimeState()
    {
        RecordDialogueSourceEnd("runtime-reset");
        if (g_activeDialoguePolicy.IsValid())
            Log("Dialogue active policy released: policy=", DialogueZoomPolicyName(g_activeDialoguePolicy.Value()), ".");
        plugin::ResetDialogueBoundaryState(GetDialogueBoundaryStateView());
    }

    void ResetPostCinematicDialogueExclusion(const char* reason)
    {
        const bool wasActive = g_postCinematicDialogueExclusion.Reset();
        if (wasActive) Log("Post-cinematic Dialogue exclusion cleared: reason=", reason, ".");
    }

    void ArmPostCinematicDialogueExclusion()
    {
        g_postCinematicDialogueExclusion.Arm(g_exitTargetFov.load(std::memory_order_acquire));
        Log("Post-cinematic Dialogue exclusion armed: release=validated-native-recovery.");
    }

    dialogue::PostCinematicRecoveryExclusion::Observation
    ObservePostCinematicDialogueRecovery(SafetyHookContext& context)
    {
        const auto source = static_cast<std::uintptr_t>(context.rsi);
        const float currentFov = context.xmm0.f32[0];
        const auto observation = g_postCinematicDialogueExclusion.ObserveValidated(
            source, currentFov, kRecoveryEndpointToleranceDegrees);
        if (observation == dialogue::PostCinematicRecoveryExclusion::Observation::Recovered) {
            Log("Post-cinematic Dialogue exclusion cleared: reason=native-recovery-converged.");
        }
        return observation;
    }

    void LogDialogueDiagnosticSummary();

    void ResetAllRuntimeResources()
    {
        g_stopping.store(true, std::memory_order_release);
        g_runtimeInitializationComplete.store(false, std::memory_order_release);
        g_hotkeyRebindCaptureActive.store(false, std::memory_order_release);
        g_hotkeyRebindConsumedKey.store(0, std::memory_order_release);
        g_cameraWriterHookGate.store(false, std::memory_order_release);
        g_cinematicHookGate.store(false, std::memory_order_release);
        g_workers.SignalStop();
        const auto callbackDrain = g_nativeCallbackAdmission.CloseAndDrain(5000);
        if (callbackDrain != plugin::NativeCallbackAdmission::DrainResult::Drained) {
            Log(callbackDrain == plugin::NativeCallbackAdmission::DrainResult::CalledFromAdmittedCallback
                ? "Runtime reset deferred: requested from an admitted native callback; hook resources retained."
                : "Runtime reset deferred: native callback drain timed out; hook resources retained.");
            return;
        }
        if (!StopWorkers(true)) {
            Log("Runtime reset deferred: required worker join did not complete; hook resources retained.");
            return;
        }
        auto correctionLock = g_cameraCorrectionGate.Acquire();
        g_cinematicRecoveryOwner.Clear();
        LogDialogueDiagnosticSummary();
        ResetPostCinematicDialogueExclusion("runtime-reset");
#ifdef POST_EXIT_GAMEPLAY_OBSERVER_DIAGNOSTIC
        g_postExitGameplayObserverHook.reset();
#endif
#ifdef POST_EXIT_CAMERA_FOV_WRITE_OWNER_TRACE
        g_postExitWriteOwnerHook.reset();
#endif
#ifdef POST_EXIT_FOV_PRODUCER_STORE_TRACE
        g_postExitProducerStoreHook.reset();
#endif
#ifdef POST_EXIT_FOV_STATE_CONSUMER_TRACE
        g_postExitConsumerHook.reset();
#endif
        g_zoomInHook.reset();
        g_zoomOutHook.reset();
        if (diagnostics::Enabled() && (g_zoomInHits.load(std::memory_order_relaxed) != 0 ||
            g_zoomOutHits.load(std::memory_order_relaxed) != 0))
            Log("ZOOM_TRANSITION SUMMARY inHits=", g_zoomInHits.load(std::memory_order_relaxed),
                " outHits=", g_zoomOutHits.load(std::memory_order_relaxed),
                " loggedEdges=", g_zoomLoggedEdges.load(std::memory_order_relaxed), ".");
        g_hook.reset();
        g_dialogueBoundaryHook.reset();
        g_cinematicExitHook.reset();
        g_cinematicEnterHook.reset();
        RestoreCinematicAspect();
        g_gameplayRecoveryObservationAvailable.store(false, std::memory_order_release);
        g_cinematicLifecycleObservationAvailable.store(false, std::memory_order_release);
        g_cinematicAspectComponentAvailable.store(false, std::memory_order_release);
        g_dialogueBoundaryHookAvailable.store(false, std::memory_order_release);
        g_cinematicSelectionValid.store(false, std::memory_order_release);
    }
#ifdef GAMEPLAY_ONE_SHOT_CINEMATIC_TRIGGER
    std::atomic_bool g_oneShotCinematicTriggerArmed{false};
#endif
#ifdef COMBINED_GAMEPLAY_DIAGNOSTIC
    std::atomic<std::uintptr_t> g_lastCameraSource{};
    std::atomic_bool g_suppressRearmAfterManualRestore{false};
    std::atomic_bool g_readyForOneShot{};
    std::atomic_bool g_readyMarkerLogged{};
    thread_local bool g_logCombinedOutputForInvocation{};

    struct CombinedOutputSnapshot
    {
        std::uintptr_t source{};
        std::uintptr_t output{};
        float sourceAspect{};
        std::uint8_t sourceFlags{};
        float outputAspect{};
        std::uint64_t outputField40{};
        std::uint64_t outputField58{};
        std::uint64_t outputField68{};
        bool valid{};
    };
    thread_local CombinedOutputSnapshot g_lastCombinedOutput{};

    struct ResolutionSnapshot
    {
        DWORD displayWidth{};
        DWORD displayHeight{};
        LONG windowWidth{};
        LONG windowHeight{};
        LONG clientWidth{};
        LONG clientHeight{};
        bool valid{};
    };
    ResolutionSnapshot g_lastResolutionSnapshot{};
#endif
#ifdef FOV_SETTINGS_TRACE_DIAGNOSTIC
    float g_lastLoggedGameplayFov = std::numeric_limits<float>::quiet_NaN();
    std::uintptr_t g_lastLoggedGameplayFovSource{};
#endif

    bool IsValidAspect(float aspect)
    {
        return std::isfinite(aspect) && aspect > 0.0f;
    }

    void TraceGameplayAspectRestorationState(
        camera::GameplayAspectRestorationDisposition disposition,
        const camera::GameplayAspectRestorationState& shadow)
    {
        if (!diagnostics::Enabled()) return;
        Log("RESTORATION_STATE action=",
            camera::GameplayAspectRestorationDispositionName(disposition),
            " aspect=", shadow.aspect, " valid=", shadow.valid ? "true" : "false",
            " source=0x", std::hex, shadow.source.value, std::dec,
            " sequence=", shadow.observationSequence, ".");
    }

    void UpdateGameplayAspectRestorationState(float aspect, std::uintptr_t source)
    {
        const auto result = g_gameplayAspectRestorationStore.Update(
            aspect, { source, source != 0 });
        if (result.changed)
            TraceGameplayAspectRestorationState(result.disposition, result.state);
    }

    void InvalidateGameplayAspectRestorationState()
    {
        const auto result = g_gameplayAspectRestorationStore.Invalidate();
        if (result.changed)
            TraceGameplayAspectRestorationState(result.disposition, result.state);
    }

    template <typename... Args>
    void LogImmediate(Args&&... args) noexcept
    {
        try {
            if (!g_logger) return;
            diagnostics::ScopedRuntimeLog measuredLog;
            std::ostringstream message;
            (message << ... << args);
            g_logger->info("{}", message.str());
        } catch (...) {
            // Logging is auxiliary at native callback/thread boundaries.
        }
    }

    template <typename... Args>
    void Log(Args&&... args) noexcept
    {
        try {
            if (!g_cameraCorrectionGate.IsHeldByCurrentThread()) {
                LogImmediate(std::forward<Args>(args)...);
                return;
            }
            using Values = std::tuple<std::decay_t<Args>...>;
            Values values(std::forward<Args>(args)...);
            static_cast<void>(g_cameraCorrectionGate.DeferUntilReleased(
                [values = std::move(values)]() mutable {
                    std::apply([](auto&&... captured) {
                        LogImmediate(std::forward<decltype(captured)>(captured)...);
                    }, std::move(values));
                }));
        } catch (...) {
            // Logging remains auxiliary at native callback/thread boundaries.
        }
    }

    void OutputCameraLogProbeDebug(const char* stage, std::uint64_t generation,
        std::uint64_t sequence, std::uintptr_t source, std::uintptr_t output,
        CoordinatorState coordinator) noexcept
    {
        if (!diagnostics::Enabled()) return;
        char line[384]{};
        const int written = std::snprintf(line, sizeof(line),
            "CAMERA_LOG_PROBE stage=%s generation=%llu sequence=%llu rsi=0x%llx "
            "rbx=0x%llx coordinator=%s logger_present=%u\n",
            stage != nullptr ? stage : "unspecified",
            static_cast<unsigned long long>(generation),
            static_cast<unsigned long long>(sequence),
            static_cast<unsigned long long>(source),
            static_cast<unsigned long long>(output),
            CoordinatorStateName(coordinator), g_logger ? 1u : 0u);
        if (written > 0 && static_cast<std::size_t>(written) < sizeof(line))
            OutputDebugStringA(line);
        else
            OutputDebugStringA("CAMERA_LOG_PROBE stage=format-failed\n");
    }

    void FlushCameraLogProbe(const char* stage, std::uint64_t generation,
        std::uint64_t sequence, std::uintptr_t source, std::uintptr_t output,
        CoordinatorState coordinator) noexcept
    {
        if (!diagnostics::Enabled()) return;
        OutputCameraLogProbeDebug(stage, generation, sequence, source, output,
            coordinator);
        LogImmediate("CAMERA_LOG_PROBE stage=", stage,
            " generation=", generation, " sequence=", sequence,
            " rsi=0x", std::hex, source, " rbx=0x", output, std::dec,
            " coordinator=", CoordinatorStateName(coordinator), ".");
        bool flushReturned = false;
        try {
            const bool loggerPresent = static_cast<bool>(g_logger);
            if (loggerPresent) {
                g_logger->flush();
                flushReturned = true;
            }
            OutputCameraLogProbeDebug(loggerPresent ?
                (flushReturned ? "file-flush-returned" : "file-flush-failed") :
                "file-flush-no-logger", generation, sequence, source, output,
                coordinator);
        } catch (...) {
            OutputCameraLogProbeDebug("file-flush-exception", generation,
                sequence, source, output, coordinator);
        }
    }

    void DeferCameraLogProbeFlush(const char* stage, std::uint64_t generation,
        std::uint64_t sequence, std::uintptr_t source, std::uintptr_t output,
        CoordinatorState coordinator) noexcept
    {
        if (!diagnostics::Enabled()) return;
        if (!g_cameraCorrectionGate.IsHeldByCurrentThread()) {
            FlushCameraLogProbe(stage, generation, sequence, source, output,
                coordinator);
            return;
        }
        try {
            const bool queued = g_cameraCorrectionGate.DeferUntilReleased(
                [stage, generation, sequence, source, output, coordinator] {
                    FlushCameraLogProbe(stage, generation, sequence, source,
                        output, coordinator);
                });
            if (!queued)
                OutputCameraLogProbeDebug("defer-rejected", generation, sequence,
                    source, output, coordinator);
        } catch (...) {
            OutputCameraLogProbeDebug("defer-exception", generation, sequence,
                source, output, coordinator);
        }
    }

    struct CinematicDiagnosticWriterState
    {
        bool valid{};
        std::uint64_t generation{};
        std::uintptr_t source{};
        std::uintptr_t output{};
        std::uintptr_t associatedSource{};
        bool associationValid{};
        bool associationConflict{};
        bool initialEstablishmentActive{};
        bool initialEstablishmentEnded{};
        cinematics::InitialEstablishmentEndReason initialEstablishmentEndReason{
            cinematics::InitialEstablishmentEndReason::None};
        bool initialEstablishmentAuthorized{};
        bool continuationSourceMatch{};
        CoordinatorState lifecycle{CoordinatorState::Gameplay};
        cinematics::CinematicWriterAction action{
            cinematics::CinematicWriterAction::PassThroughUnclassified};
        bool dispatchHandled{};
        bool rawFovValid{};
        float rawFov{std::numeric_limits<float>::quiet_NaN()};
        float inputXmm0{std::numeric_limits<float>::quiet_NaN()};
        float outputXmm0{std::numeric_limits<float>::quiet_NaN()};
    };

    std::mutex g_cinematicDiagnosticMutex;
    CinematicDiagnosticWriterState g_lastCinematicDiagnosticWriterState{};
    std::uint64_t g_lastCinematicDiagnosticAspectGeneration{};
    std::uintptr_t g_lastCinematicDiagnosticAspectTarget{};
    float g_lastCinematicDiagnosticAspect{std::numeric_limits<float>::quiet_NaN()};
    std::uint8_t g_lastCinematicDiagnosticFlags{};
    bool g_lastCinematicDiagnosticAspectValid{};
    bool g_lastCinematicDiagnosticFlagsValid{};

    const char* CinematicDiagnosticActionName(
        cinematics::CinematicWriterAction action) noexcept
    {
        switch (action) {
        case cinematics::CinematicWriterAction::RetainTransformedTarget:
            return "RetainTransformedTarget";
        case cinematics::CinematicWriterAction::RetainContinuationTarget:
            return "RetainContinuationTarget";
        case cinematics::CinematicWriterAction::PassThroughInterpolation:
            return "PassThroughInterpolation";
        case cinematics::CinematicWriterAction::PassThroughUnclassified:
            return "PassThroughUnclassified";
        }
        return "Unknown";
    }

    void LogCompletedCinematicContinuationTrace(
        const diagnostics::ContinuationTraceSnapshot& trace)
    {
        if (!trace.active) return;
        std::ostringstream block;
        block << "CINEMATIC_CONTINUATION_TRACE_BEGIN generation=" << trace.generation
            << " expected_source=0x" << std::hex << trace.expectedSource << std::dec
            << " start_fov=" << trace.continuationStartFov
            << " target_fov=" << trace.cinematicTargetFov
            << " stored_samples=" << trace.storedSamples
            << " total_samples=" << trace.totalSamples
            << " overflow=" << (trace.overflow ? 1 : 0)
            << " clear_reason=" << trace.clearReason.data() << ".";
        for (std::size_t i = 0; i < trace.storedSamples; ++i) {
            const auto& sample = trace.samples[i];
            char eventBuffer[128]{};
            const auto events = diagnostics::ContinuationTraceEventNames(
                sample.events, eventBuffer, sizeof(eventBuffer));
            block << "\nCINEMATIC_CONTINUATION_TRACE_SAMPLE generation="
                << sample.generation << " sequence=" << sample.sequence
                << " source=0x" << std::hex << sample.source
                << " expected_source=0x" << sample.expectedSource << std::dec
                << " source_match=" << (sample.sourceMatch ? 1 : 0)
                << " raw_input_fov=" << sample.rawInputFov
                << " output_fov=" << sample.outputFov
                << " production_action=" << CinematicDiagnosticActionName(
                    static_cast<cinematics::CinematicWriterAction>(sample.productionAction))
                << " continuation_start_fov=" << sample.continuationStartFov
                << " target_fov=" << sample.cinematicTargetFov
                << " in_production_range=" << (sample.inProductionRange ? 1 : 0)
                << " distance_to_target=" << sample.distanceToTarget
                << " in_target_region=" << (sample.inSemanticTargetRegion ? 1 : 0)
                << " direction=" << diagnostics::ContinuationTraceDirectionName(
                    sample.direction)
                << " events=" << events << ".";
        }
        block << "\nCINEMATIC_CONTINUATION_TRACE_END generation=" << trace.generation
            << " overflow=" << (trace.overflow ? 1 : 0) << ".";
        Log(block.str());
    }

    void FlushCinematicContinuationTrace(const char* clearReason)
    {
        if (!diagnostics::Enabled()) return;
        if (auto* traceState = GetCinematicContinuationTrace()) {
            const auto trace = traceState->Finish(true,
                clearReason != nullptr ? clearReason : "unspecified");
            LogCompletedCinematicContinuationTrace(trace);
        }
    }

    void RecordCinematicContinuationTrace(bool associationDiagnosticsActive,
        bool continuationInterpolationActive, std::uintptr_t source,
        bool continuationSourceMatch, float writerInputFov, float outputFov,
        float continuationStartFov, std::uint8_t productionAction)
    {
        if (!diagnostics::Enabled()) return;
        auto* traceState = GetCinematicContinuationTrace();
        if (traceState == nullptr ||
            !(continuationInterpolationActive || !associationDiagnosticsActive)) return;

        const float targetFov = g_cinematicTransformedFov.load(std::memory_order_acquire);
        const float lower = (std::min)(continuationStartFov, targetFov);
        const float upper = (std::max)(continuationStartFov, targetFov);
        const bool rangeValuesValid = std::isfinite(writerInputFov) &&
            std::isfinite(continuationStartFov) && std::isfinite(targetFov);
        const diagnostics::ContinuationTraceInput traceInput{
            source,
            continuationSourceMatch,
            writerInputFov,
            outputFov,
            continuationStartFov,
            targetFov,
            productionAction,
            continuationInterpolationActive && rangeValuesValid &&
                writerInputFov >= lower && writerInputFov <= upper,
            std::isfinite(writerInputFov) && std::isfinite(targetFov) &&
                camera::AreFovValuesSemanticallyEquivalent(writerInputFov, targetFov)
        };
        static_cast<void>(traceState->Record(true, traceInput));
    }

    const char* InitialEstablishmentEndReasonName(
        cinematics::InitialEstablishmentEndReason reason) noexcept
    {
        switch (reason) {
        case cinematics::InitialEstablishmentEndReason::None: return "none";
        case cinematics::InitialEstablishmentEndReason::AuthoredBaselineDeparture:
            return "authored_baseline_departure";
        case cinematics::InitialEstablishmentEndReason::AssociationConflict:
            return "association_conflict";
        case cinematics::InitialEstablishmentEndReason::CinematicExit:
            return "cinematic_exit";
        }
        return "unknown";
    }

    bool DiagnosticFovChanged(float current, float previous) noexcept
    {
        if (std::isfinite(current) != std::isfinite(previous)) return true;
        if (!std::isfinite(current)) return false;
        return !camera::AreFovValuesSemanticallyEquivalent(current, previous);
    }

    void LogCinematicDiagnosticEnter(std::uint64_t generation, float authoredFov,
        float targetFov, const cinematics::EffectiveCinematicSettings& settings)
    {
        {
            std::lock_guard lock(g_cinematicDiagnosticMutex);
            g_lastCinematicDiagnosticWriterState = {};
            g_lastCinematicDiagnosticWriterState.generation = generation;
            g_lastCinematicDiagnosticAspectGeneration = generation;
            g_lastCinematicDiagnosticAspectTarget = 0;
            g_lastCinematicDiagnosticAspect = std::numeric_limits<float>::quiet_NaN();
            g_lastCinematicDiagnosticFlags = 0;
            g_lastCinematicDiagnosticAspectValid = false;
            g_lastCinematicDiagnosticFlagsValid = false;
        }
        Log("CINEMATIC_ASSOCIATION_ENTER generation=", generation,
            " authored_fov=", authoredFov, " selected_target_fov=", targetFov,
            " enabled=", settings.enabled ? 1 : 0,
            " aspect_policy=", CinematicAspectPolicyName(settings.aspectPolicy),
            " fov_mode=", config::CinematicFovModeName(settings.fovMode), ".");
    }

    void LogCinematicDiagnosticAspectStore(std::uint64_t generation,
        std::uintptr_t targetObject, float aspect, bool aspectValid,
        std::uint8_t flags, bool flagsValid,
        const cinematics::CinematicWriterAssociationSnapshot& association)
    {
        bool changed{};
        {
            std::lock_guard lock(g_cinematicDiagnosticMutex);
            changed = generation != g_lastCinematicDiagnosticAspectGeneration ||
                targetObject != g_lastCinematicDiagnosticAspectTarget ||
                aspectValid != g_lastCinematicDiagnosticAspectValid ||
                flagsValid != g_lastCinematicDiagnosticFlagsValid ||
                (aspectValid && aspect != g_lastCinematicDiagnosticAspect) ||
                (flagsValid && flags != g_lastCinematicDiagnosticFlags);
            if (!changed) return;
            g_lastCinematicDiagnosticAspectGeneration = generation;
            g_lastCinematicDiagnosticAspectTarget = targetObject;
            g_lastCinematicDiagnosticAspect = aspect;
            g_lastCinematicDiagnosticFlags = flags;
            g_lastCinematicDiagnosticAspectValid = aspectValid;
            g_lastCinematicDiagnosticFlagsValid = flagsValid;
        }
        Log("CINEMATIC_ASSOCIATION_ASPECT_STORE generation=", generation,
            " targetObject=0x", std::hex, targetObject, std::dec,
            " aspect=", aspect, " aspect_valid=", aspectValid ? 1 : 0,
            " flags=0x", std::hex, static_cast<unsigned int>(flags), std::dec,
            " flags_valid=", flagsValid ? 1 : 0,
            " association_target=0x", std::hex, association.aspectStoreTargetObject,
            " conflicting_target=0x", association.conflictingAspectStoreTargetObject,
            std::dec, " association_valid=", association.associationValid ? 1 : 0,
            " association_conflict=", association.associationConflict ? 1 : 0, ".");
    }

    void LogCinematicDiagnosticAssociationTransition(
        const cinematics::CinematicWriterAssociationSnapshot& before,
        const cinematics::CinematicWriterAssociationSnapshot& after)
    {
        if (!before.associationValid && after.associationValid) {
            Log("CINEMATIC_ASSOCIATION_SET generation=", after.generation,
                " writer_RSI=0x", std::hex, after.associatedWriterRsi,
                " writer_RBX=0x", after.observedWriterRbx, std::dec,
                " targetObject=0x", std::hex, after.aspectStoreTargetObject,
                std::dec, " observation_sequence=", after.latestWriter.sequence,
                " identity_epoch=", after.latestWriter.identityEpoch,
                " basis=exact_RSI_equals_aspect_target.");
        } else if (!before.associationConflict && after.associationConflict) {
            Log("CINEMATIC_ASSOCIATION_CONFLICT generation=", after.generation,
                " targetObject=0x", std::hex, after.aspectStoreTargetObject,
                " conflicting_targetObject=0x", after.conflictingAspectStoreTargetObject,
                " observed_RSI=0x", after.latestWriter.source,
                " observed_RBX=0x", after.latestWriter.output, std::dec,
                " association_valid=0 reason=conflicting_identity.");
        }
    }

    void LogCinematicDiagnosticWriterState(
        const cinematics::CinematicWriterAssociationSnapshot& association,
        CoordinatorState lifecycle, std::uintptr_t source, std::uintptr_t output,
        float rawFov, bool rawFovValid, float inputXmm0, float outputXmm0,
        cinematics::CinematicWriterAction action, bool dispatchHandled,
        bool initialEstablishmentAuthorized, bool continuationSourceMatch)
    {
        bool changed{};
        {
            std::lock_guard lock(g_cinematicDiagnosticMutex);
            const auto& previous = g_lastCinematicDiagnosticWriterState;
            changed = !previous.valid || previous.generation != association.generation ||
                previous.source != source || previous.output != output ||
                previous.associatedSource != association.associatedWriterRsi ||
                previous.associationValid != association.associationValid ||
                previous.associationConflict != association.associationConflict ||
                previous.initialEstablishmentActive !=
                    association.initialEstablishmentActive ||
                previous.initialEstablishmentEnded !=
                    association.initialEstablishmentEnded ||
                previous.initialEstablishmentEndReason !=
                    association.initialEstablishmentEndReason ||
                previous.initialEstablishmentAuthorized !=
                    initialEstablishmentAuthorized ||
                previous.continuationSourceMatch != continuationSourceMatch ||
                previous.lifecycle != lifecycle ||
                previous.action != action || previous.dispatchHandled != dispatchHandled ||
                previous.rawFovValid != rawFovValid ||
                (rawFovValid && DiagnosticFovChanged(rawFov, previous.rawFov)) ||
                DiagnosticFovChanged(inputXmm0, previous.inputXmm0) ||
                DiagnosticFovChanged(outputXmm0, previous.outputXmm0);
            if (!changed) return;
            g_lastCinematicDiagnosticWriterState = {};
            auto& current = g_lastCinematicDiagnosticWriterState;
            current.valid = true;
            current.generation = association.generation;
            current.source = source;
            current.output = output;
            current.associatedSource = association.associatedWriterRsi;
            current.associationValid = association.associationValid;
            current.associationConflict = association.associationConflict;
            current.initialEstablishmentActive =
                association.initialEstablishmentActive;
            current.initialEstablishmentEnded = association.initialEstablishmentEnded;
            current.initialEstablishmentEndReason =
                association.initialEstablishmentEndReason;
            current.initialEstablishmentAuthorized = initialEstablishmentAuthorized;
            current.continuationSourceMatch = continuationSourceMatch;
            current.lifecycle = lifecycle;
            current.action = action;
            current.dispatchHandled = dispatchHandled;
            current.rawFovValid = rawFovValid;
            current.rawFov = rawFov;
            current.inputXmm0 = inputXmm0;
            current.outputXmm0 = outputXmm0;
        }
        Log("CINEMATIC_ASSOCIATION_WRITER_STATE generation=", association.generation,
            " observation_sequence=", association.latestWriter.sequence,
            " identity_epoch=", association.latestWriter.identityEpoch,
            " RSI=0x", std::hex, source, " RBX=0x", output, std::dec,
            " raw_writer_fov=", rawFov, " raw_fov_valid=", rawFovValid ? 1 : 0,
            " input_XMM0=", inputXmm0, " output_XMM0=", outputXmm0,
            " association_valid=", association.associationValid ? 1 : 0,
            " association_conflict=", association.associationConflict ? 1 : 0,
            " associated_RSI=0x", std::hex, association.associatedWriterRsi,
            " aspect_targetObject=0x", association.aspectStoreTargetObject,
            std::dec, " initial_establishment_active=",
            association.initialEstablishmentActive ? 1 : 0,
            " initial_establishment_ended=",
            association.initialEstablishmentEnded ? 1 : 0,
            " initial_establishment_end_reason=",
            InitialEstablishmentEndReasonName(
                association.initialEstablishmentEndReason),
            " initial_establishment_authorized=",
            initialEstablishmentAuthorized ? 1 : 0,
            " continuation_source_match=", continuationSourceMatch ? 1 : 0,
            " production_policy_reason=", CinematicDiagnosticActionName(action),
            " dispatch_handled=", dispatchHandled ? 1 : 0,
            " policy_changed_XMM0=", outputXmm0 != inputXmm0 ? 1 : 0,
            " lifecycle=", CoordinatorStateName(lifecycle), ".");
    }

    void LogCinematicDiagnosticExit(std::uint64_t generation, bool invalidated,
        const cinematics::CinematicWriterAssociationSnapshot& association)
    {
        Log("CINEMATIC_ASSOCIATION_EXIT generation=", generation,
            " association_invalidated=", invalidated ? 1 : 0,
            " association_valid_after=", association.associationValid ? 1 : 0,
            " association_conflict_after=", association.associationConflict ? 1 : 0,
            " initial_establishment_active=",
            association.initialEstablishmentActive ? 1 : 0,
            " initial_establishment_ended=",
            association.initialEstablishmentEnded ? 1 : 0,
            " initial_establishment_end_reason=",
            InitialEstablishmentEndReasonName(
                association.initialEstablishmentEndReason),
            " targetObject=0x", std::hex, association.aspectStoreTargetObject,
            " associated_RSI=0x", association.associatedWriterRsi, std::dec,
            " lifecycle=", CoordinatorStateName(g_coordinator.load(
                std::memory_order_acquire)), ".");
    }

    struct CinematicBaselineWriterPublicationState
    {
        std::mutex mutex;
        diagnostics::BaselineWriterPublicationObservation observation{};
    };

    CinematicBaselineWriterPublicationState* GetCinematicBaselineWriterPublicationState() noexcept
    {
        // Allocate only on the first Diagnostic-Mode observation; intentionally
        // process-lifetime storage avoids changing RuntimeState's layout.
        static auto* state = new (std::nothrow) CinematicBaselineWriterPublicationState{};
        return state;
    }

    diagnostics::BaselineWriterPublicationObservation ReadCinematicBaselineWriterPublication()
    {
        auto* state = GetCinematicBaselineWriterPublicationState();
        if (!state) return {};
        std::lock_guard lock(state->mutex);
        return state->observation;
    }

    void CaptureCinematicBaselineWriterPublication(
        const camera::GameplayBaselineProjectionResult& projection)
    {
        if (!diagnostics::ShouldCaptureCinematicBaselineProvenance(
                diagnostics::Enabled(), projection.disposition))
            return;

        const auto writer = g_cinematicWriterAssociation.Read().latestWriter;
        diagnostics::BaselineWriterPublicationObservation snapshot{};
        snapshot.captured = true;
        snapshot.baselinePublicationSequence = projection.baseline.observationSequence;
        snapshot.writerSequence = writer.sequence;
        snapshot.writerIdentityEpoch = writer.identityEpoch;
        snapshot.writerRsi = writer.source;
        snapshot.writerRbx = writer.output;
        auto* state = GetCinematicBaselineWriterPublicationState();
        if (!state) return;
        std::lock_guard lock(state->mutex);
        state->observation = snapshot;
    }

    const char* CinematicBaselineOriginName(
        cinematics::CinematicBaselineValueOrigin origin) noexcept
    {
        switch (origin) {
        case cinematics::CinematicBaselineValueOrigin::AuthoredEnter:
            return "AuthoredEnterFallback";
        case cinematics::CinematicBaselineValueOrigin::GameplayNativeNumericBaseline:
            return "GameplayBaseline";
        }
        return "unknown";
    }

    void LogCinematicBaselineProvenance(
        const diagnostics::CinematicEnterBaselineProvenanceRecord& record)
    {
        const auto& baseline = record.baseline;
        const auto& publication = record.baselinePublication;
        const auto& writer = record.latestWriterBeforeEnter;
        Log("CINEMATIC_BASELINE_PROVENANCE generation=", record.generation,
            " enter_RCX=0x", std::hex, record.enterRcx,
            " enter_RSI=0x", record.enterRsi, std::dec,
            " authored_input_fov=", record.authoredInputFov,
            " requested_fov_mode=", config::CinematicFovModeName(record.requestedFovMode),
            " effective_fov_mode=", config::CinematicFovModeName(record.effectiveFovMode),
            " baseline_valid=", baseline.valid ? 1 : 0,
            " baseline_usable=", record.baselineUsable ? 1 : 0,
            " baseline_native_fov=", baseline.nativeFov.value,
            " baseline_native_valid=", baseline.nativeFov.valid ? 1 : 0,
            " baseline_native_space=", camera::FovSpaceName(baseline.nativeFov.space),
            " baseline_native_provenance=",
                camera::FovProvenanceName(baseline.nativeFov.provenance),
            " baseline_horplus_fov=", baseline.horPlusFov.value,
            " baseline_horplus_valid=", baseline.horPlusFov.valid ? 1 : 0,
            " baseline_horplus_space=", camera::FovSpaceName(baseline.horPlusFov.space),
            " baseline_horplus_provenance=",
                camera::FovProvenanceName(baseline.horPlusFov.provenance),
            " baseline_aspect=", baseline.aspect,
            " baseline_source=0x", std::hex, baseline.source.value, std::dec,
            " baseline_source_valid=", baseline.source.valid ? 1 : 0,
            " baseline_publication_sequence=", baseline.observationSequence,
            " baseline_projection_writer_snapshot_captured=", publication.captured ? 1 : 0,
            " baseline_projection_writer_sequence=", publication.writerSequence,
            " baseline_projection_writer_identity_epoch=", publication.writerIdentityEpoch,
            " baseline_projection_writer_RSI=0x", std::hex, publication.writerRsi,
            " baseline_projection_writer_RBX=0x", publication.writerRbx, std::dec,
            " pre_enter_latest_writer_sequence=", writer.sequence,
            " pre_enter_latest_writer_identity_epoch=", writer.identityEpoch,
            " pre_enter_latest_writer_RSI=0x", std::hex, writer.source,
            " pre_enter_latest_writer_RBX=0x", writer.output, std::dec,
            " pre_enter_latest_raw_fov=", writer.rawFov,
            " pre_enter_latest_input_fov=", writer.rawFov,
            " pre_enter_latest_aspect=", writer.aspect,
            " pre_enter_latest_aspect_valid=", writer.aspectValid ? 1 : 0,
            " pre_enter_latest_flags=0x", std::hex,
                static_cast<unsigned int>(writer.flags), std::dec,
            " pre_enter_latest_flags_valid=", writer.flagsValid ? 1 : 0,
            " pointer_equality_is_relation=0.");
    }

    void LogCinematicBaselineSelection(
        std::uint64_t generation,
        const cinematics::CinematicBaselineSelection& selection,
        float resultingTargetOutput, bool transformApplied)
    {
        Log("CINEMATIC_BASELINE_SELECTION generation=", generation,
            " selected_baseline_origin=",
                CinematicBaselineOriginName(selection.valueOrigin),
            " selected_numeric_baseline=", selection.targetBaselineFov,
            " used_gameplay_baseline_value=",
                selection.usedGameplayBaselineValue ? 1 : 0,
            " resulting_cinematic_target_output=", resultingTargetOutput,
            " transform_applied=", transformApplied ? 1 : 0, ".");
    }

    void PassThroughCinematicAspectStore(SafetyHookContext& context) noexcept;

    template <void (*Callback)(SafetyHookContext&),
        void (*Rejected)(SafetyHookContext&) = nullptr>
    void SafeMidHookEntry(SafetyHookContext& context) noexcept
    {
        auto admission = g_nativeCallbackAdmission.TryEnter();
        if (!admission) {
            if constexpr (Rejected != nullptr) Rejected(context);
            return;
        }
        try {
            Callback(context);
        } catch (...) {
            // Native hook boundaries fail closed by preserving the generated
            // trampoline's pass-through continuation.
        }
    }

#ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST
    std::int64_t NowSteadyNs()
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    void CancelDeferredGameplayReplay(const char* reason, std::uintptr_t source = 0,
        float fov = std::numeric_limits<float>::quiet_NaN(), float aspect = 0.0f,
        std::uint8_t flags = 0)
    {
        const bool wasPending = g_deferredGameplayReplayPending.exchange(false,
            std::memory_order_acq_rel);
        const auto samples = g_deferredGameplayStableSamples.exchange(0,
            std::memory_order_acq_rel);
        g_deferredGameplaySource.store(0, std::memory_order_release);
        g_deferredGameplayLastFov.store(std::numeric_limits<float>::quiet_NaN(),
            std::memory_order_release);
        g_deferredGameplayStableSinceNs.store(0, std::memory_order_release);
        if (wasPending || samples != 0)
            Log("DeferCancelled: reason=", reason, " source=0x", std::hex, source,
                std::dec, " fov=", fov, " aspect=", aspect, " flags=0x", std::hex,
                static_cast<unsigned>(flags), std::dec, " stableSamples=", samples, ".");
    }

    void ArmDeferredGameplayReplay(float targetFov)
    {
        g_deferredGameplaySource.store(0, std::memory_order_release);
        g_deferredGameplayStableSamples.store(0, std::memory_order_release);
        g_deferredGameplayLastFov.store(std::numeric_limits<float>::quiet_NaN(),
            std::memory_order_release);
        g_deferredGameplayStableSinceNs.store(0, std::memory_order_release);
        g_deferredGameplayReplayPending.store(true, std::memory_order_release);
        Log("DeferArmed: targetFov=", targetFov, " requiredStableSamples=3 delayMs=",
            POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_DELAY_MS, ".");
    }
#endif

    void ResetAtomicExitHandoff(const char* reason)
    {
        const bool wasPending = g_atomicExitHandoffPending.exchange(false,
            std::memory_order_acq_rel);
        g_atomicExitHandoffSource.store(0, std::memory_order_release);
        g_atomicExitHandoffPreviousFov.store(std::numeric_limits<float>::quiet_NaN(),
            std::memory_order_release);
        if (wasPending) Log("AtomicExitHandoffCancelled: reason=", reason, ".");
    }

    plugin::CameraWriterRecoveryStateView CameraWriterRecoveryStateView()
    {
        return {
            g_coordinator, g_cinematicWriterAssociation, g_cinematicRecoveryOwner,
            g_postCinematicDialogueExclusion, g_atomicExitHandoffPending,
            g_atomicExitHandoffSource, g_atomicExitHandoffPreviousFov,
            g_cinematicContinuationPending, g_cinematicContinuationCarryPending,
            g_cinematicContinuationSource,
            g_cinematicContinuationExitNativeTarget,
            g_cinematicContinuationPreviousTarget,
            g_cinematicContinuationInterpolationActive,
            g_cinematicContinuationStartFov, g_gameplayBaselineContinuationSource,
            g_exitTargetFov, g_exitCinematicTransformedFov
        };
    }

    plugin::CameraWriterCinematicStateView GetCameraWriterCinematicStateView()
    {
        return {
            g_activeCinematicsEnabled, g_cinematicSelectionValid,
            g_cinematicSelectionGeneration, g_cinematicFovApplied,
            g_matchGameplayEnterNativeFov, g_cinematicTransformedFov,
            g_cinematicContinuationSource, g_cinematicContinuationStartFov,
            g_cinematicContinuationInterpolationActive,
            g_cinematicWriterAssociation
        };
    }

    plugin::CameraWriterGameplayStateView GetCameraWriterGameplayStateView()
    {
        return {
            g_gameplayEnableApplyPending,
            g_gameplayDisableRestorePending,
            g_gameplayModeTransitionPending,
            g_gameplayEnabledTransitionDeferralLogged,
            g_gameplayModeTransitionDeferralLogged,
            g_gameplayModeTransitionTarget,
            g_state,
            g_lastAutoRestoreSource,
            g_lastGameplayCameraSource,
            g_lastGameplayCameraFov,
            g_gameplayAspectRestorationStore,
            g_fovObservationStore,
            g_gameplayBaselineStore
        };
    }

    plugin::CinematicLifecycleStateView GetCinematicLifecycleStateView()
    {
        return {
            g_coordinator, g_cinematicSelectionValid, g_cinematicSelectionGeneration,
            g_activeCinematicsEnabled, g_activeCinematicSettingsEpoch,
            g_activeCinematicAspectPolicy, g_activeCinematicFovMode,
            g_cinematicFovApplied, g_cinematicTransformedFov,
            g_exitTargetFov, g_exitCinematicTransformedFov,
            g_matchGameplayEnterNativeFov, g_matchGameplayPreEnterNativeFov,
            g_matchGameplayPreEnterHorPlusFov, g_matchGameplayPreEnterAspect,
            g_matchGameplayPreEnterPairValid, g_gameplayBaselineContinuationSource,
            g_cinematicContinuationPending, g_cinematicContinuationCarryPending,
            g_cinematicContinuationSource,
            g_cinematicContinuationExitNativeTarget,
            g_cinematicContinuationPreviousTarget,
            g_cinematicContinuationInterpolationActive,
            g_cinematicContinuationStartFov,
            g_cinematicWriterAssociation, g_cinematicRecoveryOwner,
            g_postCinematicDialogueExclusion, g_atomicExitHandoffPending,
            g_atomicExitHandoffSource, g_atomicExitHandoffPreviousFov
        };
    }

    void ClearCinematicRecoveryAuxiliaryState(const char* reason,
        bool logNoOwner = true, bool resetDialogueLifecycle = true)
    {
        const auto cleanup = plugin::ClearCameraWriterRecoveryState(
            CameraWriterRecoveryStateView(), false);
        if (cleanup.handoffCancelled)
            Log("AtomicExitHandoffCancelled: reason=", reason, ".");
        if (cleanup.exclusionCleared)
            Log("Post-cinematic Dialogue exclusion cleared: reason=", reason, ".");
        if (resetDialogueLifecycle) {
            diagnostics::ScopedRuntimeMutex dialogueLock(
                g_dialogueMutex, diagnostics::RuntimeLockPath::Dialogue);
            ResetDialogueRuntimeState();
        }
        // Publish the non-recovery owner only after all dependent state is cleared.
        g_coordinator.store(CoordinatorState::Gameplay, std::memory_order_release);
        if (logNoOwner && diagnostics::Enabled())
            Log("CINEMATIC_RECOVERY_NO_OWNER reason=", reason,
                " coordinator=",
                CoordinatorStateName(g_coordinator.load(std::memory_order_acquire)),
                " recovery_armed=0.");
    }

    void CompleteCinematicRecovery(std::uint64_t generation, std::uintptr_t source,
        const char* path)
    {
        const auto completion = plugin::CompleteCameraWriterRecovery(
            CameraWriterRecoveryStateView(), generation);
        if (!completion.completed) return;
        if (diagnostics::Enabled())
            Log("CINEMATIC_RECOVERY_COMPLETED generation=", generation,
                " source=0x", std::hex, source, std::dec,
                " path=", path, " exclusion_active=", completion.exclusionActive ? 1 : 0,
                " recovery_owner_authority=none", ".");
    }

    void LogForeignRecoverySource(std::uintptr_t source,
        const cinematics::CinematicRecoveryOwnerSnapshot& owner,
        CoordinatorState coordinator)
    {
        if (!diagnostics::Enabled()) return;
        Log("CINEMATIC_RECOVERY_FOREIGN_SOURCE_REJECTED generation=",
            owner.generation, " owner_rsi=0x", std::hex, owner.writerRsi,
            " current_rsi=0x", source, std::dec, " coordinator=",
            camera::CoordinatorStateName(coordinator), ".");
    }

    void ArmAtomicExitHandoff()
    {
        g_atomicExitHandoffSource.store(0, std::memory_order_release);
        g_atomicExitHandoffPreviousFov.store(std::numeric_limits<float>::quiet_NaN(),
            std::memory_order_release);
        g_atomicExitHandoffPending.store(true, std::memory_order_release);
        Log("AtomicExitHandoffArmed: trigger=first-downward-fov-sample.");
    }

    bool WriteAspectAndFlags(std::uintptr_t source, float aspect, std::uint8_t flags);

#ifdef POST_EXIT_DELAYED_ASPECT_DIAGNOSTIC
    bool ApplyDelayedAspectDiagnostic(std::uintptr_t source, float currentFov,
        float currentAspect, std::uint8_t currentFlags)
    {
        bool expectedApplied = false;
        if (!g_delayedAspectDiagnosticApplied.compare_exchange_strong(
                expectedApplied, true, std::memory_order_acq_rel)) return false;
        if (!std::isfinite(currentAspect) ||
            std::fabs(currentAspect - kWideAspect) > 0.001f || currentFlags != 0x4) {
            g_delayedAspectDiagnosticApplied.store(false, std::memory_order_release);
            Log("DELAYED_ASPECT B REFUSED: unexpected source state; aspect=", currentAspect,
                " flags=0x", std::hex, static_cast<unsigned>(currentFlags), std::dec, ".");
            return false;
        }
        Log("DELAYED_ASPECT B BEFORE: source=0x", std::hex, source, std::dec,
            " fov=", currentFov, " aspect=", currentAspect, " flags=0x", std::hex,
            static_cast<unsigned>(currentFlags), std::dec, ".");
        if (!WriteAspectAndFlags(source, kNativeAspect, 0x4)) {
            g_delayedAspectDiagnosticApplied.store(false, std::memory_order_release);
            Log("DELAYED_ASPECT B REFUSED: validated aspect/flags write was not writable.");
            return false;
        }
        g_state.store(ReplayState::Complete, std::memory_order_release);
        g_lastAutoRestoreSource.store(source, std::memory_order_release);
        Log("DELAYED_ASPECT B AFTER: source=0x", std::hex, source, std::dec,
            " fov=", currentFov, " aspect=", kNativeAspect, " flags=0x4.");
        return true;
    }
#endif

    void LogGameplayFovSourceChange(std::uintptr_t source, float writerInputXmm0,
        float cameraWorldFov, float cameraFirstPersonFov, float aspect, std::uint8_t flags)
    {
        if (!diagnostics::Enabled()) return;
        const bool sourceChanged = g_lastLoggedFovSource != source;
        const auto changed = [](float current, float previous) {
            return std::isfinite(current) != std::isfinite(previous) ||
                (std::isfinite(current) && std::isfinite(previous) &&
                    std::fabs(current - previous) > 0.01f);
        };
        if (!sourceChanged && !changed(writerInputXmm0, g_lastLoggedWriterInputFov) &&
            !changed(cameraWorldFov, g_lastLoggedCameraWorldFov) &&
            !changed(cameraFirstPersonFov, g_lastLoggedCameraFirstPersonFov)) return;

        const auto coordinator = g_coordinator.load(std::memory_order_acquire);
        DialoguePhase dialoguePhase = DialoguePhase::Inactive;
        {
        diagnostics::ScopedRuntimeMutex dialogueLock(g_dialogueMutex, diagnostics::RuntimeLockPath::Dialogue);
            dialoguePhase = g_dialoguePhase;
        }
        Log("Gameplay FOV source change: event=", CoordinatorStateName(coordinator),
            " sourceChanged=", sourceChanged, " source=0x", std::hex, source, std::dec,
            " writerInputXmm0=", writerInputXmm0,
            " cameraWorldFov(+0x230)=", cameraWorldFov,
            " cameraFirstPersonFov(+0x234)=", cameraFirstPersonFov,
            " aspect=", aspect, " flags=0x", std::hex, static_cast<int>(flags), std::dec,
            " replayState=", ReplayStateName(g_state.load(std::memory_order_acquire)),
            " coordinator=", CoordinatorStateName(coordinator),
            " dialoguePhase=", DialoguePhaseName(dialoguePhase), ".");
        g_lastLoggedFovSource = source;
        g_lastLoggedWriterInputFov = writerInputXmm0;
        g_lastLoggedCameraWorldFov = cameraWorldFov;
        g_lastLoggedCameraFirstPersonFov = cameraFirstPersonFov;
    }

    template <typename T>
    bool SafeRead(std::uintptr_t address, T& value);

#if defined(POST_EXIT_RAW_TRACE_DIAGNOSTIC) || defined(POST_EXIT_INTERPOLATED_FOV_PRODUCER_TRACE)
    struct PostExitTraceSnapshot
    {
        bool valid{};
        std::uint64_t sequence{};
        std::uintptr_t source{};
        std::uintptr_t output{};
        float writerInputXmm0{};
        float cameraWorldFov{};
        float cameraFirstPersonFov{};
        float aspect{};
        std::uint8_t flags{};
        std::uint8_t selector{};
        float outputFov{};
        float outputAspect{};
        CoordinatorState coordinator{CoordinatorState::Gameplay};
        ReplayState replayState{ReplayState::WaitingForAutomaticUpdate};
        DialoguePhase dialoguePhase{DialoguePhase::Inactive};
    };

    thread_local PostExitTraceSnapshot g_previousPostExitTrace{};

    void LogPostExitWriterObservation(std::uintptr_t source, std::uintptr_t output,
        float writerInputXmm0, float cameraWorldFov, float cameraFirstPersonFov,
        float aspect, std::uint8_t flags, std::uint8_t selector,
        float outputFov, float outputAspect)
    {
        if (!g_postExitTraceArmed.load(std::memory_order_acquire) || !g_logger) return;
        const auto ordinal = g_postExitTraceWriterCount.fetch_add(1, std::memory_order_acq_rel) + 1;
        const auto startNs = g_postExitTraceStartNs.load(std::memory_order_acquire);
        const auto nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        const auto elapsedUs = startNs > 0 ? (nowNs - startNs) / 1000 : -1;
        DialoguePhase dialoguePhase = DialoguePhase::Inactive;
        {
        diagnostics::ScopedRuntimeMutex dialogueLock(g_dialogueMutex, diagnostics::RuntimeLockPath::Dialogue);
            dialoguePhase = g_dialoguePhase;
        }

        const auto sequence = g_postExitTraceSequence.load(std::memory_order_acquire);
        const auto currentCoordinator = g_coordinator.load(std::memory_order_acquire);
        const auto replayState = g_state.load(std::memory_order_acquire);
        const auto floatChanged = [](float current, float previous) {
            return std::isfinite(current) != std::isfinite(previous) ||
                (std::isfinite(current) && std::isfinite(previous) &&
                    std::fabs(current - previous) > 0.01f);
        };
        const bool newSequence = !g_previousPostExitTrace.valid ||
            g_previousPostExitTrace.sequence != sequence;
        std::array<char, 256> changedFields{};
        std::size_t changedFieldLength = 0;
        const auto addChanged = [&changedFields, &changedFieldLength](const char* name) {
            const auto nameLength = std::strlen(name);
            const auto separatorLength = changedFieldLength == 0 ? 0u : 1u;
            if (changedFieldLength + separatorLength + nameLength >= changedFields.size()) return;
            if (separatorLength != 0) changedFields[changedFieldLength++] = '|';
            std::memcpy(changedFields.data() + changedFieldLength, name, nameLength);
            changedFieldLength += nameLength;
        };
        if (newSequence || g_previousPostExitTrace.source != source) addChanged("source");
        if (newSequence || g_previousPostExitTrace.output != output) addChanged("output");
        if (newSequence || floatChanged(writerInputXmm0, g_previousPostExitTrace.writerInputXmm0))
            addChanged("fovIn");
        if (newSequence || floatChanged(cameraWorldFov, g_previousPostExitTrace.cameraWorldFov))
            addChanged("cameraWorldFov");
        if (newSequence || floatChanged(cameraFirstPersonFov,
            g_previousPostExitTrace.cameraFirstPersonFov)) addChanged("cameraFirstPersonFov");
        if (newSequence || floatChanged(aspect, g_previousPostExitTrace.aspect)) addChanged("aspect");
        if (newSequence || flags != g_previousPostExitTrace.flags) addChanged("flags");
        if (newSequence || selector != g_previousPostExitTrace.selector) addChanged("selector");
        if (newSequence || floatChanged(outputFov, g_previousPostExitTrace.outputFov))
            addChanged("outputFov");
        if (newSequence || floatChanged(outputAspect, g_previousPostExitTrace.outputAspect))
            addChanged("outputAspect");
        if (newSequence || coordinator != g_previousPostExitTrace.coordinator)
            addChanged("coordinator");
        if (newSequence || replayState != g_previousPostExitTrace.replayState)
            addChanged("replay");
        if (newSequence || dialoguePhase != g_previousPostExitTrace.dialoguePhase)
            addChanged("dialogue");

        if (changedFieldLength != 0) {
            changedFields[changedFieldLength] = '\0';
            Log("POST_EXIT seq=", sequence,
            " ordinal=", ordinal, " elapsedUs=", elapsedUs,
            " changed=", changedFields.data(),
            " source=0x", std::hex, source, " output=0x", output, std::dec,
            " writerInputXmm0=", writerInputXmm0,
            " cameraWorldFov(+0x230)=", cameraWorldFov,
            " cameraFirstPersonFov(+0x234)=", cameraFirstPersonFov,
            " aspect=", aspect, " flags=0x", std::hex, static_cast<int>(flags),
            " selector=0x", static_cast<int>(selector), std::dec,
            " outputFOV(before)=", outputFov, " outputAspect(before)=", outputAspect,
            " coordinator=", CoordinatorStateName(coordinator),
            " replayState=", ReplayStateName(replayState),
            " dialoguePhase=", DialoguePhaseName(dialoguePhase), ".");
        }

        g_previousPostExitTrace = PostExitTraceSnapshot{
            true, sequence, source, output, writerInputXmm0, cameraWorldFov,
            cameraFirstPersonFov, aspect, flags, selector, outputFov, outputAspect,
            coordinator, replayState, dialoguePhase };
    }
#endif

#if defined(POST_EXIT_RAW_TRACE_DIAGNOSTIC) || defined(POST_EXIT_INTERPOLATED_FOV_PRODUCER_TRACE)
    void ObservePostExitRawWriterEntry(SafetyHookContext& context)
    {
        if (!g_postExitTraceArmed.load(std::memory_order_acquire)) return;
        const auto source = static_cast<std::uintptr_t>(context.rsi);
        const auto output = static_cast<std::uintptr_t>(context.rbx);
        float aspect = 0.0f;
        std::uint8_t flags = 0;
        float cameraWorldFov = 0.0f;
        float cameraFirstPersonFov = 0.0f;
        float outputFov = 0.0f;
        float outputAspect = 0.0f;
        std::uint8_t selector = 0;
        if (!SafeRead(source + kAspectOffset, aspect) ||
            !SafeRead(source + kFlagsOffset, flags) ||
            !SafeRead(source + 0x230, cameraWorldFov) ||
            !SafeRead(source + 0x234, cameraFirstPersonFov) ||
            !SafeRead(source + 0x262, selector) ||
            !SafeRead(output + 0x30, outputFov) ||
            !SafeRead(output + 0x5C, outputAspect)) return;
        LogPostExitWriterObservation(source, output, context.xmm0.f32[0],
            cameraWorldFov, cameraFirstPersonFov, aspect, flags, selector,
            outputFov, outputAspect);
#ifdef POST_EXIT_INTERPOLATED_FOV_PRODUCER_TRACE
        std::uintptr_t returnAddress = 0;
        SafeRead(static_cast<std::uintptr_t>(context.rsp), returnAddress);
        Log("Post-EXIT interpolated FOV producer marker: seq=",
            g_postExitTraceSequence.load(std::memory_order_acquire),
            " elapsedUs=", [&]() {
                const auto startNs = g_postExitTraceStartNs.load(std::memory_order_acquire);
                const auto nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
                return startNs > 0 ? (nowNs - startNs) / 1000 : -1;
            }(),
            " writerRIP=0x", std::hex,
            reinterpret_cast<std::uintptr_t>(g_fovWriteAddress),
            " return=0x", returnAddress, " source=0x", source, std::dec,
            " writerInputXmm0=", context.xmm0.f32[0],
            " cameraWorldFov=", cameraWorldFov,
            " threadId=", GetCurrentThreadId(), ".");
#endif
    }
#endif

#ifdef POST_EXIT_FOV_PRODUCER_STORE_TRACE
    void ObservePostExitProducerStore(SafetyHookContext& context)
    {
        if (!g_logger) return;
        const auto camera = static_cast<std::uintptr_t>(context.rsi);
        const auto destination = camera + 0x230;
        float previousFov = 0.0f;
        const bool readable = camera != 0 && SafeRead(destination, previousFov);
        const bool armed = g_postExitTraceArmed.load(std::memory_order_acquire);
        const auto coordinator = g_coordinator.load(std::memory_order_acquire);
        const auto startNs = g_postExitTraceStartNs.load(std::memory_order_acquire);
        const auto nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        const auto elapsedUs = startNs > 0 ? (nowNs - startNs) / 1000 : -1;
        const auto hitOrdinal = g_postExitProducerStoreHitCount.fetch_add(
            1, std::memory_order_acq_rel) + 1;
        Log("Post-EXIT FOV producer store HIT: hitOrdinal=", hitOrdinal,
            " armed=", armed, " elapsedUs=", elapsedUs,
            " storeRIP=0x", std::hex,
            reinterpret_cast<std::uintptr_t>(g_executable) + kPostExitProducerStoreRva,
            " camera=0x", camera, " destination=0x", destination, std::dec,
            " previousFov=", previousFov, " writtenFov=", context.xmm0.f32[0],
            " readable=", readable, " coordinator=", CoordinatorStateName(coordinator),
            " sequence=", g_postExitTraceSequence.load(std::memory_order_acquire),
            " threadId=", GetCurrentThreadId(), ".");
        if (hitOrdinal >= 256) g_postExitProducerStoreHitCount.store(0, std::memory_order_release);
    }

    bool InstallPostExitProducerStoreTrace(const std::string& gameHash)
    {
        if (_stricmp(gameHash.c_str(), kPostExitProducerStoreGameSha256) != 0) {
            Log("Post-EXIT FOV producer store trace refused: game identity mismatch.");
            return false;
        }
        auto* target = reinterpret_cast<std::uint8_t*>(
            reinterpret_cast<std::uintptr_t>(g_executable) + kPostExitProducerStoreRva);
        constexpr std::uint8_t expected[] = {
            0xF3, 0x0F, 0x11, 0x86, 0x30, 0x02, 0x00, 0x00
        };
        if (!hooks::validation::IsExecutable(reinterpret_cast<std::uintptr_t>(target)) ||
            std::memcmp(target, expected, sizeof(expected)) != 0) {
            Log("Post-EXIT FOV producer store trace refused: 2.0.5 store bytes mismatch.");
            return false;
        }
        g_postExitProducerStoreHook = safetyhook::create_mid(target, ObservePostExitProducerStore);
        if (!g_postExitProducerStoreHook) {
            Log("Post-EXIT FOV producer store trace refused: hook creation failed.");
            return false;
        }
        Log("Post-EXIT FOV producer store trace installed: RVA=0x32B779D; observation only.");
        return true;
    }
#endif

#ifdef POST_EXIT_CAMERA_FOV_WRITE_OWNER_TRACE
    void ObservePostExitFovWriteOwner(SafetyHookContext& context)
    {
        if (!g_logger) return;

        const auto camera = static_cast<std::uintptr_t>(context.rax);
        const auto destination = camera + 0x230;
        float previousFov = 0.0f;
        const bool readable = camera != 0 && SafeRead(camera + 0x230, previousFov);
        const bool armed = g_postExitTraceArmed.load(std::memory_order_acquire);
        const auto coordinator = g_coordinator.load(std::memory_order_acquire);
        const auto startNs = g_postExitTraceStartNs.load(std::memory_order_acquire);
        const auto nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        const auto elapsedUs = startNs > 0 ? (nowNs - startNs) / 1000 : -1;
        std::uintptr_t returnAddress = 0;
        SafeRead(static_cast<std::uintptr_t>(context.rsp), returnAddress);
        const auto hitOrdinal = g_postExitWriteOwnerHitCount.fetch_add(1, std::memory_order_acq_rel) + 1;
        Log("Post-EXIT FOV store candidate HIT: hitOrdinal=", hitOrdinal,
            " armed=", armed, " elapsedUs=", elapsedUs,
            " writeRIP=0x", std::hex,
            reinterpret_cast<std::uintptr_t>(g_executable) + kPostExitWriteOwnerRva,
            " return=0x", returnAddress, " camera=0x", camera, std::dec,
            " destination=0x", std::hex, destination, std::dec,
            " previousFov=", previousFov,
            " writtenFov=", context.xmm0.f32[0],
            " readable=", readable,
            " coordinator=", CoordinatorStateName(coordinator),
            " sequence=", g_postExitTraceSequence.load(std::memory_order_acquire),
            " threadId=", GetCurrentThreadId(), ".");
        if (hitOrdinal >= 256) g_postExitWriteOwnerHitCount.store(0, std::memory_order_release);
    }

    bool InstallPostExitFovWriteOwnerTrace(const std::string& gameHash)
    {
        if (_stricmp(gameHash.c_str(), kPostExitWriteOwnerGameSha256) != 0) {
            Log("Post-EXIT FOV physical-write trace refused: game identity mismatch.");
            return false;
        }
        auto* target = reinterpret_cast<std::uint8_t*>(
            reinterpret_cast<std::uintptr_t>(g_executable) + kPostExitWriteOwnerRva);
        constexpr std::uint8_t expected[] = {
            0xF3, 0x0F, 0x11, 0x80, 0x30, 0x02, 0x00, 0x00
        };
        if (!hooks::validation::IsExecutable(reinterpret_cast<std::uintptr_t>(target)) ||
            std::memcmp(target, expected, sizeof(expected)) != 0) {
            Log("Post-EXIT FOV physical-write trace refused: 2.0.5 store bytes mismatch.");
            return false;
        }
        g_postExitWriteOwnerHook = safetyhook::create_mid(target, ObservePostExitFovWriteOwner);
        if (!g_postExitWriteOwnerHook) {
            Log("Post-EXIT FOV physical-write trace refused: hook creation failed.");
            return false;
        }
        Log("Post-EXIT FOV physical-write trace installed: RVA=0x3DB2CE7; observation only.");
        return true;
    }
#endif

#ifdef POST_EXIT_FOV_STATE_CONSUMER_TRACE
    void ObservePostExitFovConsumer(SafetyHookContext& context)
    {
        const auto coordinator = g_coordinator.load(std::memory_order_acquire);
#ifdef CINEMATIC_FOV_TRANSITION_TRACE
        if (!g_logger || (coordinator != CoordinatorState::CinematicActive &&
            coordinator != CoordinatorState::CinematicExiting)) return;
#else
        if (!g_postExitTraceArmed.load(std::memory_order_acquire) ||
            coordinator != CoordinatorState::CinematicExiting || !g_logger) return;
#endif
        const auto state = static_cast<std::uintptr_t>(context.rcx);
        const auto callerAddress = [&]() {
            std::uintptr_t value{};
            SafeRead(static_cast<std::uintptr_t>(context.rsp), value);
            return value;
        }();
        float state50 = 0.0f;
        float state54 = 0.0f;
        float state58 = 0.0f;
        const bool reads = state != 0 &&
            SafeRead(state + 0x50, state50) && SafeRead(state + 0x54, state54) &&
            SafeRead(state + 0x58, state58);
        const auto startNs = g_postExitTraceStartNs.load(std::memory_order_acquire);
        const auto nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        const auto elapsedUs = coordinator == CoordinatorState::CinematicExiting && startNs > 0
            ? (nowNs - startNs) / 1000 : -1;
        Log("Cinematic FOV consumer: phase=", CoordinatorStateName(coordinator),
            " seq=", g_postExitTraceSequence.load(std::memory_order_acquire),
            " elapsedUs=", elapsedUs, " incomingFov=", context.xmm1.f32[0],
            " state=0x", std::hex, state, " caller=0x", callerAddress, std::dec,
            " threadId=", GetCurrentThreadId(), " stateRead=", reads,
            " state+0x50=", state50, " state+0x54(before)=", state54,
            " state+0x58=", state58, ".");
    }

    bool InstallPostExitFovConsumerTrace(const std::string& gameHash)
    {
        if (_stricmp(gameHash.c_str(), kPostExitConsumerGameSha256) != 0) {
            Log("Post-EXIT FOV consumer trace refused: game identity mismatch.");
            return false;
        }
        auto* target = reinterpret_cast<std::uint8_t*>(
            reinterpret_cast<std::uintptr_t>(g_executable) + kPostExitConsumerRva);
        constexpr std::uint8_t expected[] = {
            0x56, 0x57, 0x48, 0x83, 0xEC, 0x28, 0x48, 0x89, 0xCE,
            0xF3, 0x0F, 0x11
        };
        if (!hooks::validation::IsExecutable(reinterpret_cast<std::uintptr_t>(target)) ||
            std::memcmp(target, expected, sizeof(expected)) != 0) {
            Log("Post-EXIT FOV consumer trace refused: 2.0.5 prologue mismatch.");
            return false;
        }
        g_postExitConsumerHook = safetyhook::create_mid(target, ObservePostExitFovConsumer);
        if (!g_postExitConsumerHook) {
            Log("Post-EXIT FOV consumer trace refused: hook creation failed.");
            return false;
        }
        Log("Post-EXIT FOV consumer trace installed: RVA=0x318DCD4; observation only.");
        return true;
    }
#endif

#ifdef FOV_SETTINGS_TRACE_DIAGNOSTIC
    void LogGameplayFovChange(std::uintptr_t source, float fov)
    {
        const auto coordinator = g_coordinator.load(std::memory_order_acquire);
        const auto replayState = g_state.load(std::memory_order_acquire);
        if (source != g_lastLoggedGameplayFovSource ||
            !std::isfinite(g_lastLoggedGameplayFov) ||
            std::fabs(fov - g_lastLoggedGameplayFov) > 0.01f) {
            Log("Gameplay FOV trace: source=0x", std::hex, source, std::dec,
                " writerInputXmm0=", fov,
                " coordinator=", static_cast<std::uint32_t>(coordinator),
                " replayState=", static_cast<std::uint32_t>(replayState), ".");
            g_lastLoggedGameplayFovSource = source;
            g_lastLoggedGameplayFov = fov;
        }
    }
#endif

    bool ComputeSha256(const std::filesystem::path& path, std::string& result)
    {
        return platform::win32::ComputeSha256(path, result);
    }

    bool CinematicAspectOverrideEnabled()
    {
        return cinematics::OverrideEnabled(g_runtimeCinematicPolicy.load(std::memory_order_acquire));
    }

    bool IsWritable(std::uintptr_t address, std::size_t size)
    {
        return platform::win32::IsWritable(address, size);
    }

    bool IsValidFovValue(float value)
    {
        return std::isfinite(value) && value > 1.0f && value < 179.0f;
    }

    template <typename T>
    bool SafeRead(std::uintptr_t address, T& value)
    {
        return platform::win32::ReadMemory(address, &value, sizeof(value));
    }

#ifndef RUNTIME_ADAPTER_ZOOM_SAFE_READ
#define RUNTIME_ADAPTER_ZOOM_SAFE_READ(address, value) SafeRead((address), (value))
#define RUNTIME_ADAPTER_ZOOM_SAFE_READ_DEFAULT
#endif

    struct EnterWriterRawObservation
    {
        bool valid{};
        std::uint64_t sequence{};
        std::uint64_t identityEpoch{};
        std::int64_t qpc{};
        std::uintptr_t source{};
        std::uintptr_t output{};
        float inputFov{std::numeric_limits<float>::quiet_NaN()};
        float aspect{std::numeric_limits<float>::quiet_NaN()};
        bool aspectValid{};
        std::uint8_t flags{};
        bool flagsValid{};
    };

    struct EnterWriterCaptureState
    {
        std::mutex mutex;
        EnterWriterRawObservation latestWriter{};
        std::uint64_t nextSequence{};
        std::uint64_t identityEpoch{};
        std::uintptr_t lastSource{};
        std::uintptr_t lastOutput{};
        bool lastIdentityValid{};
        bool active{};
        bool enterDetailsReady{};
        bool firstPostValid{};
        std::uint64_t generation{};
        std::int64_t enterQpc{};
        EnterWriterRawObservation preEnter{};
        EnterWriterRawObservation firstPost{};
        std::uintptr_t enterRcx{};
        std::uintptr_t enterRsi{};
        float enterNativeFov{std::numeric_limits<float>::quiet_NaN()};
        config::CinematicFovMode fovMode{config::CinematicFovMode::NativeHorPlus};
        bool gameplayEnabled{};
        config::GameplayMode gameplayMode{config::GameplayMode::RuntimeCombine};
    };

    EnterWriterCaptureState g_enterWriterCapture;

    void LogEnterWriterObservation(const char* label, std::uint64_t generation,
        const EnterWriterRawObservation& observation)
    {
        Log("CINEMATIC_ENTER_WRITER_CAPTURE_", label,
            " generation=", generation,
            " valid=", observation.valid ? 1 : 0,
            " sequence=", observation.sequence,
            " identity_epoch=", observation.identityEpoch,
            " qpc=", observation.qpc,
            " RSI_source=0x", std::hex, observation.source,
            " RBX_output=0x", observation.output, std::dec,
            " raw_input_fov=", observation.inputFov,
            " aspect=", observation.aspect,
            " aspect_valid=", observation.aspectValid ? 1 : 0,
            " flags=0x", std::hex, static_cast<unsigned int>(observation.flags), std::dec,
            " flags_valid=", observation.flagsValid ? 1 : 0, ".");
    }

    void BeginEnterWriterCapture(std::uint64_t generation, std::uintptr_t enterRcx,
        std::uintptr_t enterRsi, float nativeFov, config::CinematicFovMode fovMode,
        bool gameplayEnabled, config::GameplayMode gameplayMode)
    {
        std::uint64_t supersededGeneration{};
        EnterWriterRawObservation preEnter{};
        std::int64_t enterQpc{};
        std::int64_t qpcFrequency{};
        {
            std::lock_guard lock(g_enterWriterCapture.mutex);
            if (g_enterWriterCapture.active)
                supersededGeneration = g_enterWriterCapture.generation;
            LARGE_INTEGER counter{};
            LARGE_INTEGER frequency{};
            QueryPerformanceCounter(&counter);
            QueryPerformanceFrequency(&frequency);
            enterQpc = counter.QuadPart;
            qpcFrequency = frequency.QuadPart;
            preEnter = g_enterWriterCapture.latestWriter;
            g_enterWriterCapture.active = true;
            g_enterWriterCapture.enterDetailsReady = false;
            g_enterWriterCapture.firstPostValid = false;
            g_enterWriterCapture.generation = generation;
            g_enterWriterCapture.enterQpc = enterQpc;
            g_enterWriterCapture.preEnter = preEnter;
            g_enterWriterCapture.enterRcx = enterRcx;
            g_enterWriterCapture.enterRsi = enterRsi;
            g_enterWriterCapture.enterNativeFov = nativeFov;
            g_enterWriterCapture.fovMode = fovMode;
            g_enterWriterCapture.gameplayEnabled = gameplayEnabled;
            g_enterWriterCapture.gameplayMode = gameplayMode;
        }
        if (supersededGeneration != 0)
            Log("CINEMATIC_ENTER_WRITER_CAPTURE_END generation=", supersededGeneration,
                " reason=SUPERSEDED_BY_ACCEPTED_ENTER first_post_writer=missing.");
        Log("CINEMATIC_ENTER_WRITER_CAPTURE_BEGIN generation=", generation,
            " qpc=", enterQpc, " qpc_frequency=", qpcFrequency,
            " pre_enter_sequence=", preEnter.sequence,
            " pre_enter_identity_epoch=", preEnter.identityEpoch, ".");
        LogEnterWriterObservation("PRE_ENTER_WRITER", generation, preEnter);
    }

    void CompleteEnterWriterCapture(std::uint64_t generation, float resultFov)
    {
        EnterWriterRawObservation post{};
        bool hasPost{};
        std::uintptr_t enterRcx{};
        std::uintptr_t enterRsi{};
        float nativeFov{};
        config::CinematicFovMode fovMode{};
        bool gameplayEnabled{};
        config::GameplayMode gameplayMode{};
        std::int64_t enterQpc{};
        std::uint64_t preSequence{};
        {
            std::lock_guard lock(g_enterWriterCapture.mutex);
            if (!g_enterWriterCapture.active ||
                g_enterWriterCapture.generation != generation) return;
            enterRcx = g_enterWriterCapture.enterRcx;
            enterRsi = g_enterWriterCapture.enterRsi;
            nativeFov = g_enterWriterCapture.enterNativeFov;
            fovMode = g_enterWriterCapture.fovMode;
            gameplayEnabled = g_enterWriterCapture.gameplayEnabled;
            gameplayMode = g_enterWriterCapture.gameplayMode;
            enterQpc = g_enterWriterCapture.enterQpc;
            preSequence = g_enterWriterCapture.preEnter.sequence;
            hasPost = g_enterWriterCapture.firstPostValid;
            if (hasPost) {
                post = g_enterWriterCapture.firstPost;
                g_enterWriterCapture.active = false;
            }
        }
        Log("CINEMATIC_ENTER_WRITER_CAPTURE_ACCEPTED_ENTER generation=", generation,
            " qpc=", enterQpc,
            " RCX=0x", std::hex, enterRcx, " RSI=0x", enterRsi, std::dec,
            " native_input_fov=", nativeFov,
            " result_cinematic_fov=", resultFov,
            " fov_mode=", config::CinematicFovModeName(fovMode),
            " pre_enter_sequence=", preSequence,
            " gameplay_enabled_context=", gameplayEnabled ? 1 : 0,
            " gameplay_mode_context=", config::GameplayModeName(gameplayMode), ".");
        if (!hasPost) {
            std::lock_guard lock(g_enterWriterCapture.mutex);
            if (!g_enterWriterCapture.active ||
                g_enterWriterCapture.generation != generation) return;
            g_enterWriterCapture.enterDetailsReady = true;
            hasPost = g_enterWriterCapture.firstPostValid;
            if (hasPost) {
                post = g_enterWriterCapture.firstPost;
                g_enterWriterCapture.active = false;
            }
        }
        if (hasPost) {
            LogEnterWriterObservation("FIRST_POST_ENTER_WRITER", generation, post);
            Log("CINEMATIC_ENTER_WRITER_CAPTURE_POST_DELTA generation=", generation,
                " delta_sequence=", post.sequence - preSequence, ".");
            Log("CINEMATIC_ENTER_WRITER_CAPTURE_END generation=", generation,
                " reason=FIRST_POST_ENTER_WRITER.");
        }
    }

    void RecordEnterWriterObservation(SafetyHookContext& context)
    {
        EnterWriterRawObservation observation{};
        std::uint64_t generation{};
        std::uint64_t preSequence{};
        {
            std::lock_guard lock(g_enterWriterCapture.mutex);
            LARGE_INTEGER counter{};
            QueryPerformanceCounter(&counter);
            observation.qpc = counter.QuadPart;
            observation.source = static_cast<std::uintptr_t>(context.rsi);
            observation.output = static_cast<std::uintptr_t>(context.rbx);
            observation.inputFov = context.xmm0.f32[0];
            observation.aspectValid = observation.source != 0 &&
                SafeRead(observation.source + kAspectOffset, observation.aspect);
            observation.flagsValid = observation.source != 0 &&
                SafeRead(observation.source + kFlagsOffset, observation.flags);
            observation.sequence = ++g_enterWriterCapture.nextSequence;
            if (!g_enterWriterCapture.lastIdentityValid ||
                g_enterWriterCapture.lastSource != observation.source ||
                g_enterWriterCapture.lastOutput != observation.output) {
                ++g_enterWriterCapture.identityEpoch;
                g_enterWriterCapture.lastSource = observation.source;
                g_enterWriterCapture.lastOutput = observation.output;
                g_enterWriterCapture.lastIdentityValid = true;
            }
            observation.identityEpoch = g_enterWriterCapture.identityEpoch;
            observation.valid = true;
            g_enterWriterCapture.latestWriter = observation;
            if (g_enterWriterCapture.active && !g_enterWriterCapture.firstPostValid) {
                g_enterWriterCapture.firstPost = observation;
                g_enterWriterCapture.firstPostValid = true;
                if (g_enterWriterCapture.enterDetailsReady) {
                    g_enterWriterCapture.active = false;
                    generation = g_enterWriterCapture.generation;
                    preSequence = g_enterWriterCapture.preEnter.sequence;
                } else {
                    return;
                }
            } else {
                return;
            }
        }
        LogEnterWriterObservation("FIRST_POST_ENTER_WRITER", generation, observation);
        Log("CINEMATIC_ENTER_WRITER_CAPTURE_POST_DELTA generation=", generation,
            " delta_sequence=", observation.sequence - preSequence, ".");
        Log("CINEMATIC_ENTER_WRITER_CAPTURE_END generation=", generation,
            " reason=FIRST_POST_ENTER_WRITER.");
    }

    float ReadClientViewportAspect()
    {
        return platform::win32::ReadClientViewportAspect();
    }

    float ResolveAutoAspect()
    {
        const float aspect = ReadClientViewportAspect();
        if (IsValidAspect(aspect)) {
            Log("Auto cinematic aspect resolved from client/display viewport: aspect=", aspect, ".");
            return aspect;
        }
        Log("Auto cinematic aspect resolution unavailable; native aspect fallback=", kNativeAspect, ".");
        return kNativeAspect;
    }

    float ReadAutoAspectForCinematicStoreLog()
    {
        diagnostics::RecordRuntimeEvent(
            diagnostics::RuntimeEvent::CinematicViewportQuery);
        return ReadClientViewportAspect();
    }

    float ResolveAutoAspectForCinematicStore()
    {
        diagnostics::RecordRuntimeEvent(
            diagnostics::RuntimeEvent::CinematicViewportQuery);
        return ResolveAutoAspect();
    }

    float ResolveCinematicAspect()
    {
        return cinematics::ResolveAspect(g_activeCinematicAspectPolicy.load(std::memory_order_acquire),
            kNativeAspect, cinematics::Forced21x9Aspect, kWideAspect, ResolveAutoAspect);
    }

    float ResolveCinematicAspect(config::CinematicAspectPolicy policy)
    {
        return cinematics::ResolveAspect(policy, kNativeAspect, cinematics::Forced21x9Aspect,
            kWideAspect, ResolveAutoAspect);
    }

    bool WriteAspectAndFlags(std::uintptr_t source, float aspect, std::uint8_t flags)
    {
        return gameplay::WriteAspectAndFlags(source, kAspectOffset, kFlagsOffset,
            aspect, flags, IsWritable);
    }

    bool WriteAspectOnly(std::uintptr_t source, float aspect)
    {
        return gameplay::WriteAspectOnly(source, kAspectOffset, aspect, IsWritable);
    }

    plugin::CameraWriterEngineOperations GetCameraWriterEngineOperations()
    {
        return {nullptr, +[](void*, std::uintptr_t source, bool includeFlags) {
            plugin::CameraWriterFreshAspectFlags fields{};
            if (source == 0 ||
                !SafeRead(source + kAspectOffset, fields.aspect))
                return fields;
            fields.aspectReadable = true;
            if (includeFlags)
                fields.flagsReadable = SafeRead(source + kFlagsOffset, fields.flags);
            return fields;
        },
            +[](void*, std::uintptr_t source, float aspect) {
                return WriteAspectOnly(source, aspect);
            },
            +[](void*, std::uintptr_t source, float aspect, std::uint8_t flags) {
                return WriteAspectAndFlags(source, aspect, flags);
            }};
    }

    bool ApplyPendingGameplayModeTransition(std::uintptr_t source)
    {
        auto gameplayState = GetCameraWriterGameplayStateView();
        plugin::CameraWriterInput input{};
        input.sourceIdentity = source;
        input.gameplayEnabled = g_runtimeGameplayEnabled.load(std::memory_order_acquire);
        input.gameplayMode = g_runtimeGameplayMode.load(std::memory_order_acquire);
        input.nativeAspect = kNativeAspect;
        static_cast<void>(plugin::ProcessPendingHorPlusModeTransition(
            gameplayState, input, g_coordinator.load(std::memory_order_acquire),
            GetCameraWriterEngineOperations()));
        return false;
    }

    bool ApplyGameplayAspectFixAtomic(std::uintptr_t source, const char* phase, float fov,
        float previousAspect, std::uint8_t previousFlags)
    {
        if (!WriteAspectAndFlags(source, kNativeAspect, 0x4)) {
            Log("AtomicReplayRefused: phase=", phase,
                " final aspect/flags fields were not writable.");
            return false;
        }
        g_state.store(ReplayState::Complete, std::memory_order_release);
        g_lastAutoRestoreSource.store(source, std::memory_order_release);
        Log("AtomicReplayApplied: phase=", phase, " source=0x", std::hex, source,
            std::dec, " fov=", fov, " aspect=", kNativeAspect,
            " flags=0x4 previousAspect=", previousAspect, " previousFlags=0x",
            std::hex, static_cast<unsigned>(previousFlags), std::dec, ".");
        if (g_gameplayEnableApplyPending.load(std::memory_order_acquire) &&
            g_runtimeGameplayEnabled.load(std::memory_order_acquire) &&
            g_runtimeGameplayMode.load(std::memory_order_acquire) ==
                config::GameplayMode::AspectRecalculation &&
            g_coordinator.load(std::memory_order_acquire) == CoordinatorState::Gameplay) {
            g_gameplayEnableApplyPending.store(false, std::memory_order_release);
            g_gameplayEnabledTransitionDeferralLogged.store(false, std::memory_order_release);
            Log("Gameplay.Enabled re-enable applied: mode=AspectRecalculation",
                " result=native_transition_applied boundary=GameplayCameraWriter.");
        }
        return true;
    }

    void ApplyCinematicAspectStore(SafetyHookContext& context)
    {
        diagnostics::RecordRuntimeEvent(diagnostics::RuntimeEvent::CinematicAspectCallback);
        auto correctionLock = g_cameraCorrectionGate.Acquire();
        // Complete the replaced ten-byte store's control-flow adjustment
        // before any auxiliary processing can throw.
        context.rip += kCinematicStoreInstructionLength;
        if (g_stopping.load(std::memory_order_acquire) ||
            !g_cinematicHookGate.load(std::memory_order_acquire)) {
            static_cast<void>(WriteAspectOnly(
                static_cast<std::uintptr_t>(context.rax), kNativeAspect));
            return;
        }
        const auto targetObject = static_cast<std::uintptr_t>(context.rax);
        const bool associationDiagnosticsActive = diagnostics::Enabled();
        float diagnosticAspect = 0.0f;
        std::uint8_t diagnosticFlags = 0;
        const bool diagnosticAspectValid = associationDiagnosticsActive &&
            targetObject != 0 && SafeRead(targetObject + kAspectOffset, diagnosticAspect) &&
            IsValidAspect(diagnosticAspect);
        const bool diagnosticFlagsValid = associationDiagnosticsActive &&
            targetObject != 0 && SafeRead(targetObject + kFlagsOffset, diagnosticFlags);
        const auto aspectStore = plugin::ObserveCinematicAspectStore(
            GetCinematicLifecycleStateView(),
            {targetObject,
                g_cinematicHookGate.load(std::memory_order_acquire),
                g_cinematicSelectionValid.load(std::memory_order_acquire),
                g_stopping.load(std::memory_order_acquire),
                g_runtimeCinematicsEnabled.load(std::memory_order_acquire),
                g_activeCinematicsEnabled.load(std::memory_order_acquire),
                kNativeAspect, cinematics::Forced21x9Aspect, kWideAspect,
                &ReadAutoAspectForCinematicStoreLog,
                &ResolveAutoAspectForCinematicStore});
        const auto aspectGeneration = aspectStore.association.generation;
        if (aspectStore.accepted && associationDiagnosticsActive)
            diagnostics::EmitIfEnabled([&] {
                LogCinematicDiagnosticAspectStore(aspectGeneration, targetObject,
                    diagnosticAspect, diagnosticAspectValid, diagnosticFlags,
                    diagnosticFlagsValid, aspectStore.association);
            });
        if (!aspectStore.correctionPolicyActive) {
            static_cast<void>(WriteAspectOnly(targetObject, aspectStore.aspectToStore));
            return;
        }
        const auto policy = aspectStore.effectivePolicy;
        // For Auto, the native store may already have written 16:9 into the
        // object by the time this boundary is observed. The resolved Auto
        // value comes from the client/display viewport, not this camera field.
        const auto application = cinematics::ApplyAspectStore(targetObject,
            aspectStore.aspectToStore,
            kNativeAspect, kAspectOffset, IsWritable, IsValidAspect);
        if (application.writable) {
            diagnostics::RecordRuntimeEvent(diagnostics::RuntimeEvent::CinematicAspectLog);
            Log("Cinematic aspect store: object=0x", std::hex, targetObject, std::dec,
                " aspect=", application.aspect, " policy=", CinematicAspectPolicyName(policy),
                " source=", policy == CinematicAspectPolicy::Auto
                    ? (IsValidAspect(aspectStore.autoAspectForLog)
                        ? "client-display-viewport" : "native-fallback")
                    : "configured", ".");
        } else {
            diagnostics::RecordRuntimeEvent(diagnostics::RuntimeEvent::CinematicAspectLog);
            Log("Cinematic aspect store refused: target object was not writable; native store skipped.");
        }
        // The hook replaces C7 80 [disp32] [imm32]. The original store has no
        // control-flow side effects, so resuming after its ten-byte encoding
        // preserves the native lifecycle and following instructions.
    }

    void PassThroughCinematicAspectStore(SafetyHookContext& context) noexcept
    {
        context.rip += kCinematicStoreInstructionLength;
        static_cast<void>(WriteAspectOnly(
            static_cast<std::uintptr_t>(context.rax), kNativeAspect));
    }

    bool LogCameraModeChange(std::uintptr_t source, std::uintptr_t output, float writerInputXmm0,
        float aspect, std::uint8_t flags)
    {
        if (!diagnostics::Enabled()) return false;
        float cameraWorldFov = 0.0f;
        float cameraFirstPersonFov = 0.0f;
        float outputFov = 0.0f;
        float outputAspect = 0.0f;
        float rawSourceAspect = 0.0f;
        std::uint8_t rawSourceFlags = 0;
        std::uint8_t selector = 0;
        SafeRead(source + 0x230, cameraWorldFov);
        SafeRead(source + 0x234, cameraFirstPersonFov);
        SafeRead(source + kAspectOffset, rawSourceAspect);
        SafeRead(source + kFlagsOffset, rawSourceFlags);
        SafeRead(source + 0x262, selector);
        SafeRead(output + 0x30, outputFov);
        SafeRead(output + 0x5C, outputAspect);
        LogGameplayFovSourceChange(source, writerInputXmm0, cameraWorldFov,
            cameraFirstPersonFov, aspect, flags);

#ifdef COMBINED_GAMEPLAY_DIAGNOSTIC
        std::uint64_t outputField40 = 0;
        std::uint64_t outputField58 = 0;
        std::uint64_t outputField68 = 0;
        if (!SafeRead(output + 0x40, outputField40) || !SafeRead(output + 0x58, outputField58) ||
            !SafeRead(output + 0x68, outputField68))
            return false;
        const CombinedOutputSnapshot snapshot{
            source, output, aspect, flags, outputAspect,
            outputField40, outputField58, outputField68, true };
        const bool outputChanged = !g_lastCombinedOutput.valid ||
            snapshot.source != g_lastCombinedOutput.source ||
            snapshot.output != g_lastCombinedOutput.output ||
            snapshot.sourceAspect != g_lastCombinedOutput.sourceAspect ||
            snapshot.sourceFlags != g_lastCombinedOutput.sourceFlags ||
            snapshot.outputAspect != g_lastCombinedOutput.outputAspect ||
            snapshot.outputField58 != g_lastCombinedOutput.outputField58;
        g_lastCombinedOutput = snapshot;
#endif

        std::uint32_t aspectBits{};
        std::memcpy(&aspectBits, &aspect, sizeof(aspectBits));
        const std::uint32_t mode = aspectBits ^ (static_cast<std::uint32_t>(flags) << 1) ^
            (static_cast<std::uint32_t>(selector) << 9);
        const bool sourceModeChanged = g_lastCameraMode.exchange(mode, std::memory_order_relaxed) != mode;
#ifdef COMBINED_GAMEPLAY_DIAGNOSTIC
        g_logCombinedOutputForInvocation = sourceModeChanged || outputChanged;
        if (!g_logCombinedOutputForInvocation)
            return false;
#else
        if (!sourceModeChanged)
            return false;
#endif

        Log("Camera mode: source=0x", std::hex, source, std::dec,
            " rawSourceAspect=", rawSourceAspect, " rawSourceFlags=0x", std::hex,
            static_cast<int>(rawSourceFlags), std::dec,
            " writerInputXmm0=", writerInputXmm0,
            " cameraWorldFov(+0x230)=", cameraWorldFov,
            " cameraFirstPersonFov(+0x234)=", cameraFirstPersonFov,
            " aspect=", aspect, " flags=0x", std::hex, static_cast<int>(flags),
            " selector=0x", static_cast<int>(selector), std::dec,
            " outputFOV(before)=", outputFov, " outputAspect(before)=", outputAspect, ".");
#ifdef COMBINED_GAMEPLAY_DIAGNOSTIC
        Log("Combined output PRE: source=0x", std::hex, source, " output=0x", output, std::dec,
            " outputFov=", outputFov, " outputAspect=", outputAspect,
            " output+0x40=0x", std::hex, outputField40,
            " output+0x58=0x", outputField58, " output+0x68=0x", outputField68, std::dec, ".");
#endif
        return true;
    }

#ifdef GAMEPLAY_ONE_SHOT_CINEMATIC_TRIGGER
    bool WriteFlagsOnly(std::uintptr_t source, std::uint8_t flags)
    {
        if (!IsWritable(source + kFlagsOffset, sizeof(flags))) return false;
        std::memcpy(reinterpret_cast<void*>(source + kFlagsOffset), &flags, sizeof(flags));
        return true;
    }

    DWORD WINAPI OneShotTriggerLoop(void*)
    {
        bool previousF7 = false;
#ifdef COMBINED_GAMEPLAY_DIAGNOSTIC
        bool previousF6 = false;
        bool previousF8 = false;
        bool previousF9 = false;
#endif
        while (!WaitForWorkerStop(10)) {
#ifdef COMBINED_GAMEPLAY_DIAGNOSTIC
            const bool f6 = (GetAsyncKeyState(VK_F6) & 0x8000) != 0;
#endif
            const bool f7 = (GetAsyncKeyState(VK_F7) & 0x8000) != 0;
#ifdef COMBINED_GAMEPLAY_DIAGNOSTIC
            const bool f8 = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
            const bool f9 = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
            if (f6 && !previousF6)
                Log("Combined diagnostic marker: native-wrong cinematic state.");
#endif
            if (f7 && !previousF7) {
                if (g_readyForOneShot.load(std::memory_order_acquire)) {
                    g_oneShotCinematicTriggerArmed.store(true, std::memory_order_release);
                    Log("Diagnostic one-shot cinematic trigger armed from validated READY state.");
                } else {
                    Log("Diagnostic F7 refused: validated 32:9/0x5 Waiting state is not ready.");
                }
            }
#ifdef COMBINED_GAMEPLAY_DIAGNOSTIC
            if (f8 && !previousF8) {
                const auto source = g_lastCameraSource.load(std::memory_order_acquire);
                float currentAspect = 0.0f;
                DEVMODE display{ .dmSize = sizeof(DEVMODE) };
                const bool displayReady = EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &display) && display.dmPelsHeight != 0;
                const float displayAspect = displayReady
                    ? static_cast<float>(display.dmPelsWidth) / static_cast<float>(display.dmPelsHeight) : 0.0f;
                const bool applied = source && displayReady &&
                    g_state.load(std::memory_order_acquire) == ReplayState::Complete &&
                    SafeRead(source + kAspectOffset, currentAspect) &&
                    std::fabs(currentAspect - kNativeAspect) <= 0.001f &&
                    IsWritable(source + kAspectOffset, sizeof(displayAspect));
                if (applied) {
                    std::memcpy(reinterpret_cast<void*>(source + kAspectOffset), &displayAspect, sizeof(displayAspect));
                    g_suppressRearmAfterManualRestore.store(true, std::memory_order_release);
                    Log("Combined diagnostic F8: native aspect restore source=0x", std::hex, source,
                        std::dec, " aspect=", currentAspect, " -> ", displayAspect,
                        " flags preserved.");
                } else {
                    Log("Combined diagnostic F8 refused: Complete + constrained 16:9 state was not confirmed.");
                }
            }
            if (f9 && !previousF9)
                Log("Combined diagnostic marker: observation window.");
            previousF6 = f6;
#endif
            previousF7 = f7;
#ifdef COMBINED_GAMEPLAY_DIAGNOSTIC
            previousF8 = f8;
            previousF9 = f9;
#endif
        }
    }
#endif

    void LogTransitionSnapshot(const char* stage, std::uint64_t sequence, std::uintptr_t source)
    {
        float aspect = 0.0f;
        std::uint8_t flags = 0;
        if (!SafeRead(source + kAspectOffset, aspect) || !SafeRead(source + kFlagsOffset, flags)) {
            Log("Gameplay transition seq=", sequence, " stage=", stage,
                " source=0x", std::hex, source, std::dec, " readable=false");
            return;
        }

        Log("Gameplay transition seq=", sequence, " stage=", stage,
            " source=0x", std::hex, source, std::dec,
            " aspect=", aspect, " flags=0x", std::hex, static_cast<int>(flags), std::dec,
            " transitionState=", ReplayStateName(g_state.load(std::memory_order_relaxed)), ".");
    }

#ifdef COMBINED_GAMEPLAY_DIAGNOSTIC
    void LogResolutionChange()
    {
        DEVMODE display{ .dmSize = sizeof(DEVMODE) };
        const bool displayReady = EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &display) != FALSE;
        HWND window = platform::win32::FindCurrentProcessWindow();
        RECT windowRect{};
        RECT clientRect{};
        const bool windowReady = window && GetWindowRect(window, &windowRect) && GetClientRect(window, &clientRect);
        ResolutionSnapshot current{
            displayReady ? display.dmPelsWidth : 0,
            displayReady ? display.dmPelsHeight : 0,
            windowReady ? windowRect.right - windowRect.left : 0,
            windowReady ? windowRect.bottom - windowRect.top : 0,
            windowReady ? clientRect.right - clientRect.left : 0,
            windowReady ? clientRect.bottom - clientRect.top : 0,
            displayReady || windowReady };
        if (!current.valid || (g_lastResolutionSnapshot.valid &&
            current.displayWidth == g_lastResolutionSnapshot.displayWidth &&
            current.displayHeight == g_lastResolutionSnapshot.displayHeight &&
            current.windowWidth == g_lastResolutionSnapshot.windowWidth &&
            current.windowHeight == g_lastResolutionSnapshot.windowHeight &&
            current.clientWidth == g_lastResolutionSnapshot.clientWidth &&
            current.clientHeight == g_lastResolutionSnapshot.clientHeight))
            return;

        g_lastResolutionSnapshot = current;
        Log("Resolution observation: display=", current.displayWidth, "x", current.displayHeight,
            " window=", current.windowWidth, "x", current.windowHeight,
            " client=", current.clientWidth, "x", current.clientHeight,
            " source=Win32 display/window bounds; engine render resolution not established.");
    }

    DWORD WINAPI ResolutionMonitorLoop(void*)
    {
        while (!WaitForWorkerStop(250)) {
            LogResolutionChange();
        }
    }

    void LogCombinedOutputPost(std::uint64_t sequence, std::uintptr_t source, std::uintptr_t output)
    {
        float outputFov = 0.0f;
        float outputAspect = 0.0f;
        std::uint64_t outputField40 = 0;
        std::uint64_t outputField58 = 0;
        std::uint64_t outputField68 = 0;
        if (!SafeRead(output + 0x30, outputFov) || !SafeRead(output + 0x5C, outputAspect) ||
            !SafeRead(output + 0x40, outputField40) || !SafeRead(output + 0x58, outputField58) ||
            !SafeRead(output + 0x68, outputField68)) {
            Log("Combined output POST seq=", sequence, " read-refused.");
            return;
        }
        Log("Combined output POST seq=", sequence, ": source=0x", std::hex, source,
            " output=0x", output, std::dec, " outputFov=", outputFov,
            " outputAspect=", outputAspect, " output+0x40=0x", std::hex, outputField40,
            " output+0x58=0x", outputField58, " output+0x68=0x", outputField68, std::dec, ".");
    }
#endif

#ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST
    // The deferred-replay experiment is a non-production build profile. The
    // regular writer hook now routes AspectRecalculation through
    // ProcessCameraWriter; keep this legacy experiment adapter out of the
    // production ASI until/if that research profile is explicitly revived.
    void ReplayManualTransitionOriginal(SafetyHookContext& context)
    {
        const auto source = context.rsi;
#ifdef COMBINED_GAMEPLAY_DIAGNOSTIC
        g_lastCameraSource.store(source, std::memory_order_release);
#endif
        const float fov = context.xmm0.f32[0];
        const auto previousSource = g_lastGameplayCameraSource.exchange(source, std::memory_order_acq_rel);
        const float previousFov = g_lastGameplayCameraFov.exchange(fov, std::memory_order_acq_rel);
        const auto contextChange = gameplay::EvaluateGameplayContextChange(
            previousSource, previousFov, source, fov, kDialogueContextFovJump);
        if (contextChange.InvalidatesDialogue()) {
        diagnostics::ScopedRuntimeMutex dialogueLock(g_dialogueMutex, diagnostics::RuntimeLockPath::Dialogue);
            if (g_dialoguePhase != DialoguePhase::Inactive) {
                Log("Dialogue runtime invalidated by gameplay camera context change: sourceChanged=",
                    contextChange.sourceChanged, " previousSource=0x", std::hex, previousSource,
                    " source=0x", source, std::dec, " previousFOV=", previousFov,
                    " currentFOV=", fov, ". Native pass-through until a new descent.");
                ResetDialogueRuntimeState();
            }
        }
#ifdef FOV_SETTINGS_TRACE_DIAGNOSTIC
        if (std::isfinite(fov) && fov > 1.0f && fov < 179.0f)
            LogGameplayFovChange(source, fov);
#endif
        float aspect = 0.0f;
        std::uint8_t flags = 0;
        if (!SafeRead(source + kAspectOffset, aspect) || !SafeRead(source + kFlagsOffset, flags)) return;
        const auto stateBeforeObservation = g_state.load(std::memory_order_acquire);
        const bool isOwnAutoRestore = stateBeforeObservation == ReplayState::Complete &&
            std::fabs(aspect - kNativeAspect) <= 0.001f &&
            g_lastAutoRestoreSource.load(std::memory_order_acquire) == source;
        // The restore observation arrives while the replay state is still
        // AppliedConstrainPass; g_lastAutoRestoreSource is assigned only after
        // the restore write below. Use the lifecycle state as the ownership
        // marker so the fix-owned native value cannot replace the last
        // externally observed ultrawide Auto aspect.
        const bool isPendingOwnAutoRestore = stateBeforeObservation == ReplayState::AppliedConstrainPass &&
            std::fabs(aspect - kNativeAspect) <= 0.001f;
        if (!isOwnAutoRestore && !isPendingOwnAutoRestore && IsValidAspect(aspect))
        {
            UpdateGameplayAspectRestorationState(aspect, source);
        }
        else if (isPendingOwnAutoRestore)
            Log("Skipped fix-owned Auto restore in authoritative aspect cache; aspect=", aspect, ".");

        if (g_runtimeGameplayMode.load(std::memory_order_acquire) ==
                config::GameplayMode::AspectRecalculation &&
            g_coordinator.load(std::memory_order_acquire) == CoordinatorState::Gameplay &&
            !isOwnAutoRestore && !isPendingOwnAutoRestore &&
            IsValidFovValue(fov) && IsValidAspect(aspect)) {
            camera::CameraFovObservation observation{};
            observation.boundary = camera::FovObservationBoundary::CameraWriter;
            observation.inputFov = {
                fov, camera::FovSpace::Native,
                camera::FovProvenance::NativeRegisterInput, true };
            observation.resultFov = {
                fov, camera::FovSpace::Native,
                camera::FovProvenance::PassThroughResult, true };
            observation.aspect = { aspect, camera::FovProvenance::ResolvedAspect, true };
            observation.writerSource = { source, source != 0 };
            observation.writerFlags = flags;
            observation.writerFlagsValid = true;
            observation.observedDuringGameplay = true;
            const auto committed = g_fovObservationStore.Publish(observation);
            const auto projection = g_gameplayBaselineStore.Project(committed, true);
            CaptureCinematicBaselineWriterPublication(projection);
        }
#ifdef GAMEPLAY_ONE_SHOT_CINEMATIC_TRIGGER
        if (g_oneShotCinematicTriggerArmed.load(std::memory_order_acquire) &&
            g_state.load(std::memory_order_relaxed) == ReplayState::WaitingForAutomaticUpdate &&
            gameplay::IsConstrainedUltrawideAspect(aspect, flags, kNativeAspect)) {
            if (WriteFlagsOnly(source, 0x4)) {
                g_oneShotCinematicTriggerArmed.store(false, std::memory_order_release);
#ifdef COMBINED_GAMEPLAY_DIAGNOSTIC
                g_readyForOneShot.store(false, std::memory_order_release);
#endif
                flags = 0x4;
                Log("Diagnostic one-shot cinematic trigger consumed at camera-writer boundary: source=0x",
                    std::hex, source, std::dec, " aspect=", aspect, " flags=0x5 -> 0x4.");
            } else {
                Log("Diagnostic one-shot cinematic trigger refused: flags field was not writable.");
            }
        }
#endif
#ifdef COMBINED_GAMEPLAY_DIAGNOSTIC
        const auto currentState = g_state.load(std::memory_order_acquire);
        const bool ready = currentState == ReplayState::WaitingForAutomaticUpdate &&
            gameplay::IsConstrainedUltrawideAspect(aspect, flags, kNativeAspect);
        g_readyForOneShot.store(ready, std::memory_order_release);
        if (ready && !g_readyMarkerLogged.exchange(true, std::memory_order_acq_rel))
            Log("Combined diagnostic READY FOR F7: source=0x", std::hex, source,
                std::dec, " aspect=", aspect, " flags=0x5 transitionState=WaitingForAutomaticUpdate.");
        if (!ready)
            g_readyMarkerLogged.store(false, std::memory_order_release);
#endif
        const bool modeChanged = LogCameraModeChange(source, context.rbx, fov, aspect, flags);
        const auto traceSequence = modeChanged
            ? g_transitionTraceSequence.fetch_add(1, std::memory_order_relaxed) + 1
            : 0;
        if (modeChanged) LogTransitionSnapshot("PRE", traceSequence, source);
        const auto logPost = [&]() {
            if (modeChanged) LogTransitionSnapshot("POST", traceSequence, source);
#ifdef COMBINED_GAMEPLAY_DIAGNOSTIC
            if (modeChanged && g_logCombinedOutputForInvocation)
                LogCombinedOutputPost(traceSequence, source, context.rbx);
#endif
        };

        if (gameplay::IsConstrainedUltrawideAspect(aspect, flags, kNativeAspect)) {
            // Preserve constrained aspect ratio for a real 21:9 output. The FOV source
            // is already correct; only the camera aspect state needs normalization.
            if (WriteAspectAndFlags(source, kNativeAspect, 0x5))
                Log("Normalized 21:9 camera aspect while preserving constrained aspect ratio.");
            logPost();
            return;
        }

        auto state = g_state.load(std::memory_order_relaxed);
        if (state == ReplayState::Complete && gameplay::IsUltrawideAspect(aspect, kNativeAspect) && flags == 0x4) {
            // Cutscenes can rebuild the gameplay camera and restore the same broken
            // Auto state seen during startup. Arm the proven two-pass transition again.
            {
        diagnostics::ScopedRuntimeMutex dialogueLock(g_dialogueMutex, diagnostics::RuntimeLockPath::Dialogue);
                ResetDialogueRuntimeState();
            }
#ifdef COMBINED_GAMEPLAY_DIAGNOSTIC
            if (g_suppressRearmAfterManualRestore.exchange(false, std::memory_order_acq_rel)) {
                Log("Combined diagnostic: manual native-aspect restore observed; automatic re-arm suppressed.");
            } else {
#endif
                g_state.store(ReplayState::WaitingForAutomaticUpdate, std::memory_order_relaxed);
                state = ReplayState::WaitingForAutomaticUpdate;
                Log("Gameplay camera was rebuilt; re-arming aspect transition.");
#ifdef COMBINED_GAMEPLAY_DIAGNOSTIC
            }
#endif
        }

        if (state == ReplayState::WaitingForAutomaticUpdate) {
            // The game has performed its own startup update. Recreate the observed
            // temporary constrained pass that occurs when the user applies 16:9.
            if (!gameplay::IsUltrawideAspect(aspect, kNativeAspect) || flags != 0x4) {
                logPost();
                return;
            }
            ApplyGameplayAspectFixAtomic(source, "GameplayDetected", fov, aspect, flags);
            logPost();
            return;
        }
        logPost();
    }
#endif

#ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST
    bool TryDeferredGameplayReplay(SafetyHookContext& context)
    {
        if (!g_deferredGameplayReplayPending.load(std::memory_order_acquire)) return false;

        const auto source = static_cast<std::uintptr_t>(context.rsi);
        const float fov = context.xmm0.f32[0];
        const float targetFov = g_exitTargetFov.load(std::memory_order_acquire);
        float aspect = 0.0f;
        std::uint8_t flags = 0;
        if (!SafeRead(source + kAspectOffset, aspect) || !SafeRead(source + kFlagsOffset, flags)) {
            CancelDeferredGameplayReplay("camera-state-unreadable", source, fov, aspect, flags);
            return true;
        }

        bool dialogueActive = false;
        {
        diagnostics::ScopedRuntimeMutex dialogueLock(g_dialogueMutex, diagnostics::RuntimeLockPath::Dialogue);
            dialogueActive = g_dialoguePhase != DialoguePhase::Inactive;
        }
        if (g_coordinator.load(std::memory_order_acquire) != CoordinatorState::Gameplay) {
            CancelDeferredGameplayReplay("coordinator-not-gameplay", source, fov, aspect, flags);
            return true;
        }
        if (dialogueActive) {
            CancelDeferredGameplayReplay("dialogue-active", source, fov, aspect, flags);
            return true;
        }

        const auto expectedSource = g_deferredGameplaySource.load(std::memory_order_acquire);
        if (expectedSource != 0 && expectedSource != source) {
            CancelDeferredGameplayReplay("source-changed", source, fov, aspect, flags);
            return true;
        }
        if (!std::isfinite(fov) || !std::isfinite(targetFov) ||
            std::fabs(fov - targetFov) > kRecoveryEndpointToleranceDegrees) {
            CancelDeferredGameplayReplay("fov-left-target-tolerance", source, fov, aspect, flags);
            return true;
        }
        if (!gameplay::IsUltrawideAspect(aspect, kNativeAspect) || flags != 0x4) {
            CancelDeferredGameplayReplay("gameplay-replay-not-applicable", source, fov, aspect, flags);
            return true;
        }

        const float previousFov = g_deferredGameplayLastFov.load(std::memory_order_acquire);
        if (std::isfinite(previousFov) && std::fabs(fov - previousFov) > kDialogueTransformEpsilon) {
            CancelDeferredGameplayReplay("fov-not-stable", source, fov, aspect, flags);
            return true;
        }
        if (expectedSource == 0)
            g_deferredGameplaySource.store(source, std::memory_order_release);
        g_deferredGameplayLastFov.store(fov, std::memory_order_release);

        auto samples = g_deferredGameplayStableSamples.load(std::memory_order_acquire);
        if (samples < kDeferredGameplayStableSamples) {
            samples = g_deferredGameplayStableSamples.fetch_add(1,
                std::memory_order_acq_rel) + 1;
            Log("StableSample ", samples, "/3: source=0x", std::hex, source, std::dec,
                " fov=", fov, " targetFov=", targetFov, " aspect=", aspect,
                " flags=0x", std::hex, static_cast<unsigned>(flags), std::dec,
                " elapsedUs=", (NowSteadyNs() - g_postExitTraceStartNs.load(std::memory_order_acquire)) / 1000, ".");
            if (samples < kDeferredGameplayStableSamples) return true;

            const auto stableSince = NowSteadyNs();
            g_deferredGameplayStableSinceNs.store(stableSince, std::memory_order_release);
            Log("StableStateConfirmed: source=0x", std::hex, source, std::dec,
                " fov=", fov, " aspect=", aspect, " flags=0x", std::hex,
                static_cast<unsigned>(flags), std::dec, ".");
        }
        const auto stableSince = g_deferredGameplayStableSinceNs.load(std::memory_order_acquire);
        if (NowSteadyNs() - stableSince < kDeferredGameplayDelayNs) return true;

        bool expectedPending = true;
        if (!g_deferredGameplayReplayPending.compare_exchange_strong(expectedPending, false,
            std::memory_order_acq_rel)) return true;
        g_deferredGameplayStableSamples.store(0, std::memory_order_release);
        Log("DeferredReplayApplied: source=0x", std::hex, source, std::dec,
            " fov=", fov, " aspect=", aspect, " flags=0x", std::hex,
            static_cast<unsigned>(flags), std::dec, " delayMs=",
            POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_DELAY_MS,
#ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_ATOMICITY_TEST
            " mode=atomic.");
#else
            " mode=legacy.");
#endif
#ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_ATOMICITY_TEST
        if (WriteAspectAndFlags(source, kNativeAspect, 0x4)) {
            g_state.store(ReplayState::Complete, std::memory_order_release);
            g_lastAutoRestoreSource.store(source, std::memory_order_release);
            Log("AtomicReplayApplied: source=0x", std::hex, source, std::dec,
                " fov=", fov, " aspect=", kNativeAspect, " flags=0x4 delayMs=",
                POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_DELAY_MS, ".");
        } else {
            Log("AtomicReplayRefused: final aspect/flags fields were not writable.");
        }
#else
        g_state.store(ReplayState::WaitingForAutomaticUpdate, std::memory_order_release);
        ReplayManualTransitionOriginal(context);
#endif
        return true;
    }
#endif

    bool ValidateEnterBoundary(std::uint8_t* match, std::uint8_t*& callsite)
    {
        hooks::ExecutableSpan textSpan{};
        if (!hooks::FindSectionSpan(g_executable, ".text", textSpan)) return false;
        ZydisDecodedInstruction instruction{};
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT]{};
        if (!hooks::validation::DecodeInstruction(match, instruction, operands, textSpan) ||
            instruction.mnemonic != ZYDIS_MNEMONIC_MOVSS || instruction.length != 8 ||
            instruction.operand_count_visible < 2 ||
            operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER ||
            operands[0].reg.value != ZYDIS_REGISTER_XMM0 ||
            operands[1].type != ZYDIS_OPERAND_TYPE_MEMORY ||
            operands[1].mem.base != ZYDIS_REGISTER_RIP) return false;
        callsite = match + 8;
        return hooks::validation::IsCallRel32(callsite, textSpan) &&
            hooks::validation::ContainsBytes(match, 96, EnterVcallPair, sizeof(EnterVcallPair), textSpan);
    }

    bool ValidateExitBoundary(std::uint8_t* match, std::uint8_t*& callsite)
    {
        hooks::ExecutableSpan textSpan{};
        if (!hooks::FindSectionSpan(g_executable, ".text", textSpan)) return false;
        ZydisDecodedInstruction instruction{};
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT]{};
        if (!hooks::validation::DecodeInstruction(match, instruction, operands, textSpan) ||
            instruction.mnemonic != ZYDIS_MNEMONIC_MOVSS || instruction.length != 5 ||
            instruction.operand_count_visible < 2 ||
            operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER ||
            operands[0].reg.value != ZYDIS_REGISTER_XMM0 ||
            operands[1].type != ZYDIS_OPERAND_TYPE_MEMORY ||
            operands[1].mem.base != ZYDIS_REGISTER_RDI ||
            !operands[1].mem.disp.has_displacement || operands[1].mem.disp.value != 0x38) return false;
        callsite = match + 5;
        return hooks::validation::IsCallRel32(callsite, textSpan) &&
            hooks::validation::ContainsBytes(match, 96, ExitVcallPair, sizeof(ExitVcallPair), textSpan);
    }

    bool ValidateIndexedExitBoundary(std::uint8_t* match, std::uint8_t*& callsite)
    {
        hooks::ExecutableSpan textSpan{};
        if (!hooks::FindSectionSpan(g_executable, ".text", textSpan)) return false;
        constexpr std::size_t kMovssOffset = 4;
        constexpr std::size_t kCallOffset = 10;
        ZydisDecodedInstruction instruction{};
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT]{};
        if (!hooks::validation::DecodeInstruction(match + kMovssOffset, instruction, operands, textSpan) ||
            instruction.mnemonic != ZYDIS_MNEMONIC_MOVSS || instruction.length != 6 ||
            instruction.operand_count_visible < 2 ||
            operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER ||
            operands[0].reg.value != ZYDIS_REGISTER_XMM0 ||
            operands[1].type != ZYDIS_OPERAND_TYPE_MEMORY ||
            operands[1].mem.base != ZYDIS_REGISTER_RBX ||
            operands[1].mem.index != ZYDIS_REGISTER_RAX ||
            operands[1].mem.scale != 4 ||
            !operands[1].mem.disp.has_displacement ||
            operands[1].mem.disp.value != 0x38) return false;
        callsite = match + kCallOffset;
        return hooks::validation::IsCallRel32(callsite, textSpan) &&
            hooks::validation::ContainsBytes(match, 96, ExitVcallPair, sizeof(ExitVcallPair), textSpan);
    }

    bool ResolveCinematicAspectStore(std::uint8_t*& store)
    {
        const auto resolution = cinematics::ResolveAspectStore(g_executable, CinematicAspectSetter,
            kAspectOffset, kCinematicStorePrefix, sizeof(kCinematicStorePrefix),
            kCinematicOriginalImmediate, kCinematicImmediateOffset,
            sizeof(kCinematicOriginalImmediate), kCinematicStoreInstructionLength, 0x3FE38E39);
        if (!resolution.imageValid) return false;
        if (resolution.matches != 1) {
            Log("Cinematic aspect signature rejected: matches=", resolution.matches, ".");
            return false;
        }
        if (!resolution.store) return false;
        store = resolution.store;
        Log("Cinematic aspect signature validated at RVA=0x", std::hex,
            reinterpret_cast<std::uintptr_t>(store) - reinterpret_cast<std::uintptr_t>(g_executable), std::dec, ".");
        return true;
    }

    bool ResolveCinematicFovCallsites(std::uint8_t*& enter, std::uint8_t*& exit)
    {
        const auto enterMatches = hooks::FindAll(g_executable, CinematicEnter);
        auto exitMatches = hooks::FindAll(g_executable, CinematicExit);
        bool indexedExit = false;
        Log("Cinematic FOV resolver: ENTER matches=", enterMatches.size(),
            " EXIT legacy matches=", exitMatches.size(), ".");
        if (exitMatches.empty()) {
            exitMatches = hooks::FindAll(g_executable, CinematicExitIndexed);
            indexedExit = true;
            Log("Cinematic FOV resolver: EXIT indexed fallback=ACTIVE matches=", exitMatches.size(), ".");
        } else {
            Log("Cinematic FOV resolver: EXIT indexed fallback=NOT_USED.");
        }
        if (enterMatches.size() != 1 || exitMatches.size() != 1) {
            Log("Cinematic FOV signatures rejected: enterMatches=", enterMatches.size(),
                " exitMatches=", exitMatches.size(), ".");
            return false;
        }
        const bool enterValid = ValidateEnterBoundary(enterMatches.front(), enter);
        Log("Cinematic FOV ENTER structural/decode/operand validation=",
            enterValid ? "PASS" : "FAIL", ".");
        if (!enterValid) {
            return false;
        }
        const bool exitValid = indexedExit
            ? ValidateIndexedExitBoundary(exitMatches.front(), exit)
            : ValidateExitBoundary(exitMatches.front(), exit);
        Log("Cinematic FOV EXIT structural/decode/operand validation=",
            exitValid ? "PASS" : "FAIL", " mode=", indexedExit ? "indexed" : "legacy", ".");
        if (!exitValid) {
            return false;
        }
        hooks::ExecutableSpan textSpan{};
        if (!hooks::FindSectionSpan(g_executable, ".text", textSpan)) return false;
        const auto enterTarget = hooks::validation::ResolveRel32CallTarget(enter, textSpan);
        const auto exitTarget = hooks::validation::ResolveRel32CallTarget(exit, textSpan);
        const bool enterTargetValid = enterTarget != nullptr &&
            hooks::validation::IsExecutable(reinterpret_cast<std::uintptr_t>(enterTarget));
        const bool exitTargetValid = exitTarget != nullptr &&
            hooks::validation::IsExecutable(reinterpret_cast<std::uintptr_t>(exitTarget));
        const bool sameTarget = enterTarget != nullptr && enterTarget == exitTarget;
        Log("Cinematic FOV call-target validation: ENTER=", enterTargetValid ? "PASS" : "FAIL",
            " EXIT=", exitTargetValid ? "PASS" : "FAIL",
            " sameTarget=", sameTarget ? "PASS" : "FAIL", ".");
        if (!enterTargetValid || !exitTargetValid || !sameTarget) {
            Log("Cinematic FOV consumer target validation failed: ENTER=0x", std::hex,
                reinterpret_cast<std::uintptr_t>(enterTarget), " EXIT=0x",
                reinterpret_cast<std::uintptr_t>(exitTarget), std::dec, ".");
            return false;
        }
        g_cinematicExitIndexed.store(indexedExit, std::memory_order_release);
        Log("Cinematic FOV signatures validated at ENTER RVA=0x", std::hex,
            reinterpret_cast<std::uintptr_t>(enter) - reinterpret_cast<std::uintptr_t>(g_executable),
            " EXIT RVA=0x", reinterpret_cast<std::uintptr_t>(exit) - reinterpret_cast<std::uintptr_t>(g_executable),
            " consumer=0x", reinterpret_cast<std::uintptr_t>(enterTarget), std::dec, ".");
        return true;
    }

    bool InstallCinematicAspect(std::uint8_t* store)
    {
        g_cinematicAspectStore = store;
        if (!hooks::validation::IsExecutable(reinterpret_cast<std::uintptr_t>(g_cinematicAspectStore)) ||
            std::memcmp(g_cinematicAspectStore, kCinematicStorePrefix, sizeof(kCinematicStorePrefix)) != 0 ||
            std::memcmp(g_cinematicAspectStore + kCinematicImmediateOffset,
                kCinematicOriginalImmediate, sizeof(kCinematicOriginalImmediate)) != 0) return false;
        std::memcpy(g_cinematicOriginalImmediate, g_cinematicAspectStore + kCinematicImmediateOffset,
            sizeof(g_cinematicOriginalImmediate));
        auto hook = safetyhook::MidHook::create(
            g_cinematicAspectStore,
            SafeMidHookEntry<&ApplyCinematicAspectStore, &PassThroughCinematicAspectStore>,
            safetyhook::MidHook::StartDisabled);
        if (!hook) return false;
        g_cinematicAspectStoreHook = std::move(*hook);
        g_cinematicAspectPatched = true;
        return true;
    }

    void RestoreCinematicAspect() noexcept
    {
        static_cast<void>(g_cinematicAspectStoreHook.disable());
        g_cinematicAspectStoreHook.reset();
        g_cinematicAspectPatched = false;
    }

    void TraceCinematicEnter(SafetyHookContext& context)
    {
        if (!g_cinematicHookGate.load(std::memory_order_acquire) ||
            g_stopping.load(std::memory_order_acquire)) return;
        auto correctionLock = g_cameraCorrectionGate.Acquire();
        if (!g_cinematicHookGate.load(std::memory_order_acquire) ||
            g_stopping.load(std::memory_order_acquire)) return;
        const auto previousCoordinator = g_coordinatorState.load(std::memory_order_acquire);
        std::optional<camera::CameraFovObservation> gameplayBoundaryObservation;
        if (previousCoordinator == CoordinatorState::Gameplay)
            gameplayBoundaryObservation = g_fovObservationStore.ReadLatest(
                camera::FovObservationBoundary::CameraWriter);
        if (gameplayBoundaryObservation &&
            !gameplayBoundaryObservation->observedDuringGameplay)
            gameplayBoundaryObservation.reset();
        const bool awaitingValidatedRecovery = g_cinematicContinuationPending.load(
            std::memory_order_acquire) && g_postCinematicDialogueExclusion.IsActive();
        const auto previousContinuationSource = g_cinematicContinuationSource.load(
            std::memory_order_acquire);
        const float previousExitNativeTarget =
            g_cinematicContinuationExitNativeTarget.load(std::memory_order_acquire);
        const float previousTransformedTarget =
            g_cinematicContinuationPreviousTarget.load(std::memory_order_acquire);
        const bool cinematicEnabled = g_runtimeCinematicsEnabled.load(
            std::memory_order_acquire);
        // Do not inspect correction modes while the module is OFF. ENTER
        // lifecycle observation still advances with neutral, non-correcting
        // selections so the coordinator remains accurate.
        const auto requestedAspectPolicy = cinematicEnabled
            ? g_runtimeCinematicPolicy.load(std::memory_order_acquire)
            : CinematicAspectPolicy::Native;
        const auto requestedFovMode = cinematicEnabled
            ? g_runtimeCinematicFovMode.load(std::memory_order_acquire)
            : config::CinematicFovMode::NativeHorPlus;
        const auto lifecycleState = GetCinematicLifecycleStateView();
        const auto selection = plugin::PrepareCinematicEnter(
            lifecycleState, requestedAspectPolicy, requestedFovMode);
        if (!selection.valid)
            return;
        FlushCinematicContinuationTrace("superseded_by_accepted_generation");
        if (diagnostics::Enabled())
            BeginEnterWriterCapture(selection.generation,
                static_cast<std::uintptr_t>(context.rcx),
                static_cast<std::uintptr_t>(context.rsi), context.xmm0.f32[0],
                selection.fovMode,
                g_runtimeGameplayEnabled.load(std::memory_order_acquire),
                g_runtimeGameplayMode.load(std::memory_order_acquire));
#ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST
        CancelDeferredGameplayReplay("new-cinematic-enter");
#endif
        const auto cinematicSettingsEpoch = cinematicEnabled
            ? g_cinematicsSettingsEpoch.load(std::memory_order_acquire) : 0;
        const float before = context.xmm0.f32[0];
        const float aspect = cinematicEnabled
            ? ResolveCinematicAspect(selection.aspectPolicy) : kNativeAspect;
        if (diagnostics::Enabled())
            g_cinematicEnterAspect.store(aspect, std::memory_order_release);
        const auto gameplayBaseline = cinematicEnabled || diagnostics::Enabled()
            ? g_gameplayBaselineStore.Read() : camera::GameplayBaseline{};
        const bool gameplayBaselineUsable = cinematicEnabled &&
            camera::IsUsableGameplayBaseline(gameplayBaseline);
        if (diagnostics::ShouldEmitCinematicBaselineProvenance(diagnostics::Enabled())) {
            const auto publication = ReadCinematicBaselineWriterPublication();
            const auto latestWriter = g_cinematicWriterAssociation.Read().latestWriter;
            const auto enterBaselineRecord = diagnostics::CaptureCinematicEnterBaselineProvenance(
                selection.generation, static_cast<std::uintptr_t>(context.rcx),
                static_cast<std::uintptr_t>(context.rsi), before, requestedFovMode,
                selection.fovMode, gameplayBaseline, publication, latestWriter);
            diagnostics::EmitIfEnabled([&] {
                LogCinematicBaselineProvenance(enterBaselineRecord);
            });
        }
        const plugin::AcceptedCinematicEnterInput enterInput{
            selection, cinematicEnabled, cinematicSettingsEpoch, before,
            gameplayBaseline, aspect, kNativeAspect,
            previousContinuationSource, previousExitNativeTarget,
            previousTransformedTarget, awaitingValidatedRecovery};
        const auto enterTransaction = plugin::ProcessAcceptedCinematicEnter(
            GetCinematicLifecycleStateView(), enterInput);
        if (!enterTransaction.accepted) return;
        g_runtime.cameraTransitionBoundary = camera::BeginCameraTransitionBoundary(
            g_runtime.cameraTransitionBoundary, selection.generation,
            gameplayBoundaryObservation, enterTransaction.continuedOwnership);
        MarkCameraPresentationTransition(true);
        const float after = enterTransaction.outputFov;
        const bool transformed = enterTransaction.transformed;
        const auto& baselineSelection = enterTransaction.baselineSelection;
        if (diagnostics::ShouldEmitCinematicBaselineProvenance(diagnostics::Enabled())) {
            diagnostics::EmitIfEnabled([&] {
                LogCinematicBaselineSelection(selection.generation,
                    baselineSelection, after, transformed);
            });
        }
        context.xmm0.f32[0] = enterTransaction.outputFov;
        if (enterTransaction.handoffCancelled)
            Log("AtomicExitHandoffCancelled: reason=new-cinematic-enter.");
        if (enterTransaction.exclusionCleared)
            Log("Post-cinematic Dialogue exclusion cleared: reason=new-cinematic-enter.");
        if (diagnostics::Enabled())
            CompleteEnterWriterCapture(selection.generation, context.xmm0.f32[0]);
        diagnostics::EmitIfEnabled([&] {
            LogCinematicDiagnosticEnter(selection.generation, before,
                transformed ? after : before,
                { cinematicEnabled, selection.aspectPolicy, selection.fovMode });
        });
        const bool continuedOwnership = enterTransaction.continuedOwnership;
        if (diagnostics::Enabled()) {
            if (auto* traceState = GetCinematicContinuationTrace()) {
            if (continuedOwnership) {
                traceState->Begin(true, selection.generation,
                    cinematicEnabled && camera::IsUsableGameplayBaseline(gameplayBaseline) &&
                        gameplayBaseline.source.valid
                        ? gameplayBaseline.source.value : 0,
                    previousExitNativeTarget, after);
            } else {
                traceState->Discard();
            }
            }
        }
        if (IsValidFovValue(before) && IsValidFovValue(context.xmm0.f32[0]) &&
            IsValidAspect(aspect)) {
            camera::CameraFovObservation observation{};
            observation.boundary = camera::FovObservationBoundary::CinematicEnter;
            observation.inputFov = {
                before, camera::FovSpace::Native,
                camera::FovProvenance::NativeRegisterInput, true };
            observation.resultFov = {
                context.xmm0.f32[0],
                transformed ? camera::FovSpace::Transformed : camera::FovSpace::Native,
                transformed ? camera::FovProvenance::ModTransformResult :
                    camera::FovProvenance::PassThroughResult,
                true };
            observation.aspect = { aspect, camera::FovProvenance::ResolvedAspect, true };
            const auto committed = g_fovObservationStore.Publish(observation);
            g_runtime.cameraTransitionBoundary.cinematic = committed;
            g_runtime.cameraTransitionBoundary.cinematicValid = true;
            if (diagnostics::Enabled()) {
                Log("FOV_OBSERVATION boundary=", camera::FovObservationBoundaryName(committed.boundary),
                    " input=", committed.inputFov.value,
                    " inputSpace=", camera::FovSpaceName(committed.inputFov.space),
                    " inputProvenance=", camera::FovProvenanceName(committed.inputFov.provenance),
                    " result=", committed.resultFov.value,
                    " resultSpace=", camera::FovSpaceName(committed.resultFov.space),
                    " resultProvenance=", camera::FovProvenanceName(committed.resultFov.provenance),
                    " aspect=", committed.aspect.value,
                    " sequence=", committed.publicationSequence, ".");
            }
        }
        if (diagnostics::Enabled()) {
            const diagnostics::CinematicLifecycleGprSnapshot gprs{
                static_cast<std::uintptr_t>(context.rax),
                static_cast<std::uintptr_t>(context.rbx),
                static_cast<std::uintptr_t>(context.rcx),
                static_cast<std::uintptr_t>(context.rdx),
                static_cast<std::uintptr_t>(context.rsi),
                static_cast<std::uintptr_t>(context.rdi),
                static_cast<std::uintptr_t>(context.r8),
                static_cast<std::uintptr_t>(context.r9),
                static_cast<std::uintptr_t>(context.r10),
                static_cast<std::uintptr_t>(context.r11),
                static_cast<std::uintptr_t>(context.r12),
                static_cast<std::uintptr_t>(context.r13),
                static_cast<std::uintptr_t>(context.r14),
                static_cast<std::uintptr_t>(context.r15),
            };
            diagnostics::CinematicLifecycleEnterContext enterContext{};
            if (diagnostics::CaptureCinematicLifecycleEnter(true,
                    selection.generation, before, gprs,
                    static_cast<std::uint32_t>(CoordinatorState::CinematicActive),
                    g_cinematicSelectionGeneration.load(std::memory_order_acquire),
                    g_cinematicSelectionValid.load(std::memory_order_acquire),
                    enterContext)) {
                Log("CINEMATIC_LIFECYCLE_ENTER_CONTEXT generation=",
                    enterContext.generation, " xmm0=", enterContext.xmm0,
                    " rax=0x", std::hex, enterContext.gprs.rax,
                    " rbx=0x", enterContext.gprs.rbx,
                    " rcx=0x", enterContext.gprs.rcx,
                    " rdx=0x", enterContext.gprs.rdx,
                    " rsi=0x", enterContext.gprs.rsi,
                    " rdi=0x", enterContext.gprs.rdi,
                    " r8=0x", enterContext.gprs.r8,
                    " r9=0x", enterContext.gprs.r9,
                    " r10=0x", enterContext.gprs.r10,
                    " r11=0x", enterContext.gprs.r11,
                    " r12=0x", enterContext.gprs.r12,
                    " r13=0x", enterContext.gprs.r13,
                    " r14=0x", enterContext.gprs.r14,
                    " r15=0x", enterContext.gprs.r15, std::dec,
                    " coordinator=", CoordinatorStateName(
                        static_cast<CoordinatorState>(enterContext.coordinator)),
                    " selection_generation=", enterContext.selectionGeneration,
                    " selection_valid=", enterContext.selectionValid ? 1 : 0, ".");
            }
        }
        Log("Global cinematic ENTER: aspect=", aspect, " authoredFov=", before,
            " transformedFov=", context.xmm0.f32[0],
            " aspectPolicy=", CinematicAspectPolicyName(selection.aspectPolicy), ".",
            " Gameplay replay suppressed.");
        if (diagnostics::Enabled()) {
            Log("CINEMATIC_BASELINE_CONTEXT source=",
                baselineSelection.valueOrigin ==
                    cinematics::CinematicBaselineValueOrigin::GameplayNativeNumericBaseline
                    ? "GameplayNativeNumericValue" : "AuthoredEnterFallback",
                " mode=", config::CinematicFovModeName(
                    selection.fovMode),
                " baselineValid=", gameplayBaselineUsable ? "true" : "false",
                " native=", gameplayBaseline.nativeFov.value,
                " nativeValid=", gameplayBaseline.nativeFov.valid ? "true" : "false",
                " horPlus=", gameplayBaseline.horPlusFov.value,
                " horPlusValid=", gameplayBaseline.horPlusFov.valid ? "true" : "false",
                " gameplayAspect=", gameplayBaseline.aspect,
                " sequence=", gameplayBaseline.observationSequence,
                " authoredEnter=", before,
                " targetBaseline=", baselineSelection.targetBaselineFov,
                " effectiveCinematicAspect=", aspect,
                " transformed=", transformed ? "true" : "false", ".");
        }
    }

    void TraceCinematicExit(SafetyHookContext& context)
    {
        if (!g_cinematicHookGate.load(std::memory_order_acquire) ||
            g_stopping.load(std::memory_order_acquire)) return;
        auto correctionLock = g_cameraCorrectionGate.Acquire();
        if (!g_cinematicHookGate.load(std::memory_order_acquire) ||
            g_stopping.load(std::memory_order_acquire)) return;
        const auto previousCoordinator = g_coordinator.load(std::memory_order_acquire);
        const bool selectionValid = g_cinematicSelectionValid.load(
            std::memory_order_acquire);
        const auto exitingGeneration = g_cinematicSelectionGeneration.load(
            std::memory_order_acquire);
        if (diagnostics::Enabled()) {
            const diagnostics::CinematicLifecycleGprSnapshot gprs{
                static_cast<std::uintptr_t>(context.rax),
                static_cast<std::uintptr_t>(context.rbx),
                static_cast<std::uintptr_t>(context.rcx),
                static_cast<std::uintptr_t>(context.rdx),
                static_cast<std::uintptr_t>(context.rsi),
                static_cast<std::uintptr_t>(context.rdi),
                static_cast<std::uintptr_t>(context.r8),
                static_cast<std::uintptr_t>(context.r9),
                static_cast<std::uintptr_t>(context.r10),
                static_cast<std::uintptr_t>(context.r11),
                static_cast<std::uintptr_t>(context.r12),
                static_cast<std::uintptr_t>(context.r13),
                static_cast<std::uintptr_t>(context.r14),
                static_cast<std::uintptr_t>(context.r15),
            };
            const auto association = g_cinematicWriterAssociation.Read();
            diagnostics::CinematicLifecycleExitContext exitContext{};
            const auto form = g_cinematicExitIndexed.load(std::memory_order_acquire)
                ? diagnostics::CinematicExitCandidateForm::Indexed
                : diagnostics::CinematicExitCandidateForm::Legacy;
            const bool acceptedByF7A = cinematics::IsAcceptedCinematicExit(
                previousCoordinator, selectionValid);
            if (diagnostics::CaptureCinematicLifecycleExit(true, form,
                    context.xmm0.f32[0], gprs,
                    static_cast<std::uint32_t>(previousCoordinator),
                    exitingGeneration, selectionValid, acceptedByF7A,
                    association.generation, association.aspectStoreTargetObject,
                    association.associatedWriterRsi, association.associationValid,
                    association.associationConflict, exitContext)) {
                Log("CINEMATIC_LIFECYCLE_EXIT_CONTEXT candidate_form=",
                    diagnostics::CinematicExitCandidateFormName(exitContext.form),
                    " xmm0=", exitContext.xmm0,
                    " rax=0x", std::hex, exitContext.gprs.rax,
                    " rbx=0x", exitContext.gprs.rbx,
                    " rcx=0x", exitContext.gprs.rcx,
                    " rdx=0x", exitContext.gprs.rdx,
                    " rsi=0x", exitContext.gprs.rsi,
                    " rdi=0x", exitContext.gprs.rdi,
                    " r8=0x", exitContext.gprs.r8,
                    " r9=0x", exitContext.gprs.r9,
                    " r10=0x", exitContext.gprs.r10,
                    " r11=0x", exitContext.gprs.r11,
                    " r12=0x", exitContext.gprs.r12,
                    " r13=0x", exitContext.gprs.r13,
                    " r14=0x", exitContext.gprs.r14,
                    " r15=0x", exitContext.gprs.r15, std::dec,
                    " native_operand_rbx=0x", std::hex,
                    exitContext.form == diagnostics::CinematicExitCandidateForm::Indexed
                        ? exitContext.gprs.rbx : 0,
                    " native_operand_rax=0x",
                    exitContext.form == diagnostics::CinematicExitCandidateForm::Indexed
                        ? exitContext.gprs.rax : 0, std::dec,
                    " coordinator=", CoordinatorStateName(
                        static_cast<CoordinatorState>(exitContext.coordinator)),
                    " selection_generation=", exitContext.selectionGeneration,
                    " selection_valid=", exitContext.selectionValid ? 1 : 0,
                    " accepted_f7a=", exitContext.acceptedByF7A ? 1 : 0,
                    " association_generation=", exitContext.associationGeneration,
                    " aspect_store_target=0x", std::hex,
                    exitContext.aspectStoreTargetObject,
                    " associated_writer_rsi=0x", exitContext.associatedWriterRsi,
                    std::dec, " association_valid=", exitContext.associationValid ? 1 : 0,
                    " association_conflict=", exitContext.associationConflict ? 1 : 0, ".");
                if (exitContext.form == diagnostics::CinematicExitCandidateForm::Indexed)
                    Log("CINEMATIC_LIFECYCLE_EXIT_NATIVE_OPERAND rbx=0x", std::hex,
                        exitContext.gprs.rbx, " rax=0x", exitContext.gprs.rax,
                        std::dec, " action=registers-only-no-dereference.");
            }
        }
        plugin::AcceptedCinematicExitResult exitTransaction{};
        const bool dispatched = cinematics::DispatchCinematicExit(
            previousCoordinator, selectionValid,
            [&] {
                if (!diagnostics::Enabled()) return;
                static_cast<void>(g_cameraCorrectionGate.DeferUntilReleased(
                    [previousCoordinator, selectionValid, exitingGeneration,
                     nativeExitFov = context.xmm0.f32[0]] {
                        Log("CINEMATIC_EXIT_REJECTED generation=", exitingGeneration,
                            " coordinator=", CoordinatorStateName(previousCoordinator),
                            " selection_valid=", selectionValid ? 1 : 0,
                            " native_fov=", nativeExitFov,
                            " mutations=0.");
                    }));
            }, [&] {
        const auto previousContinuationSource = g_gameplayBaselineContinuationSource.load(
            std::memory_order_acquire);
        const float previousTransformedTarget = g_cinematicTransformedFov.load(
            std::memory_order_acquire);
        const auto gameplayMode = g_runtimeGameplayMode.load(std::memory_order_acquire);
        const auto exitTransition = gameplay::ResolveCinematicExitTransition(
            g_gameplayRecoveryObservationAvailable.load(std::memory_order_acquire),
            g_runtimeGameplayEnabled.load(std::memory_order_acquire), gameplayMode);
        const auto associationAtExit = g_cinematicWriterAssociation.Read();
        exitTransaction = plugin::ProcessAcceptedCinematicExit(
            GetCinematicLifecycleStateView(),
            {previousCoordinator, exitTransition.nextState, exitingGeneration,
                context.xmm0.f32[0],
                g_runtimeGameplayEnabled.load(std::memory_order_acquire), gameplayMode,
                previousContinuationSource, previousTransformedTarget});
        if (!exitTransaction.accepted) return;
        MarkCameraPresentationTransition(true);
        const auto supersededLogProbe = diagnostics::ArmCinematicExitWriterProbe(
            g_cinematicExitLogProbeGeneration, diagnostics::Enabled(),
            exitingGeneration);
        if (diagnostics::Enabled()) {
            if (supersededLogProbe != 0)
                Log("CAMERA_LOG_PROBE stage=exit-probe-superseded previous_generation=",
                    supersededLogProbe, " generation=", exitingGeneration, ".");
            OutputCameraLogProbeDebug("exit-callback", exitingGeneration,
                associationAtExit.latestWriter.sequence,
                associationAtExit.latestWriter.source,
                associationAtExit.latestWriter.output,
                exitTransition.nextState);
        }
        const bool recoveryOwnerCaptured = exitTransaction.recoveryOwnerCaptured;
        if (recoveryOwnerCaptured) {
            const auto owner = exitTransaction.recoveryOwner;
            if (diagnostics::Enabled())
                Log("CINEMATIC_RECOVERY_OWNER_SNAPSHOT generation=", owner.generation,
                    " writer_rsi=0x", std::hex, owner.writerRsi,
                    " observed_rbx=0x", owner.observedWriterRbx, std::dec,
                    " source=cinematic_association.");
        }
#ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST
        CancelDeferredGameplayReplay("new-cinematic-exit");
#endif
        if (exitTransaction.handoffCancelled)
            Log("AtomicExitHandoffCancelled: reason=new-cinematic-exit.");
        if (exitTransaction.exclusionCleared)
            Log("Post-cinematic Dialogue exclusion cleared: reason=new-cinematic-exit.");
        if (recoveryOwnerCaptured)
            Log("Post-cinematic Dialogue exclusion armed: release=validated-native-recovery.");
#ifdef POST_EXIT_TRACE_STATE
        g_postExitTraceWriterCount.store(0, std::memory_order_release);
        g_postExitTraceSequence.fetch_add(1, std::memory_order_acq_rel);
        g_postExitTraceStartNs.store(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count(), std::memory_order_release);
        g_postExitTraceArmed.store(true, std::memory_order_release);
#endif
#ifdef POST_EXIT_DELAYED_ASPECT_DIAGNOSTIC
        g_delayedAspectDiagnosticApplied.store(false, std::memory_order_release);
#endif
        const bool associationInvalidated = exitTransaction.associationInvalidated;
        diagnostics::EmitIfEnabled([&] {
            LogCinematicDiagnosticExit(exitingGeneration, associationInvalidated,
                g_cinematicWriterAssociation.Read());
        });
        if (diagnostics::Enabled())
            g_cinematicEnterAspect.store(std::numeric_limits<float>::quiet_NaN(),
                std::memory_order_release);
        FlushCinematicContinuationTrace("cinematic_exit");
        if (!recoveryOwnerCaptured ||
            exitTransition.nextState != CoordinatorState::CinematicExiting) {
            const char* reason = exitTransition.nextState !=
                    CoordinatorState::CinematicExiting
                ? "recovery-observer-unavailable" :
                  associationAtExit.associationConflict
                    ? "association-conflicted" : "association-unavailable";
            if (diagnostics::Enabled())
                Log("CINEMATIC_RECOVERY_NO_OWNER reason=", reason,
                    " coordinator=",
                    CoordinatorStateName(g_coordinator.load(std::memory_order_acquire)),
                    " recovery_armed=0.");
            diagnostics::ScopedRuntimeMutex dialogueLock(
                g_dialogueMutex, diagnostics::RuntimeLockPath::Dialogue);
            ResetDialogueRuntimeState();
        } else if (exitTransaction.armGameplayHandoff) {
            Log("AtomicExitHandoffArmed: trigger=first-downward-fov-sample.");
        } else {
        diagnostics::ScopedRuntimeMutex dialogueLock(g_dialogueMutex, diagnostics::RuntimeLockPath::Dialogue);
            ResetDialogueRuntimeState();
        }
        Log("Global cinematic EXIT: nativeTargetFov=", context.xmm0.f32[0],
#ifdef POST_EXIT_TRACE_STATE
            " postExitTrace=armed sequence=", g_postExitTraceSequence.load(std::memory_order_acquire),
#else
            " postExitTrace=disabled",
#endif
            " coordinator_after=", CoordinatorStateName(exitTransition.nextState),
            " recovery_owner_captured=", recoveryOwnerCaptured ? 1 : 0,
            " gameplay_mode=", config::GameplayModeName(gameplayMode), ".");
        if (diagnostics::Enabled())
            Log("CINEMATIC_EXIT_ROUTING generation=", exitingGeneration,
                " recovery_owner_captured=", recoveryOwnerCaptured ? 1 : 0,
                " coordinator_after=", CoordinatorStateName(exitTransition.nextState),
                " gameplay_mode=", config::GameplayModeName(gameplayMode), ".");
        DeferCameraLogProbeFlush("after-cinematic-exit", exitingGeneration,
            associationAtExit.latestWriter.sequence,
            associationAtExit.latestWriter.source,
            associationAtExit.latestWriter.output, exitTransition.nextState);
        });
        if (!dispatched) return;
    }

    bool InstallCinematicAspectComponent()
    {
        std::uint8_t* cinematicAspectStore = nullptr;
        if (!ResolveCinematicAspectStore(cinematicAspectStore) ||
            !InstallCinematicAspect(cinematicAspectStore)) return false;
        Log("Global cinematic aspect store hooked: policy=",
            CinematicAspectPolicyName(g_config.cinematicAspectPolicy), ".");
        return true;
    }

    bool InstallCinematicFovComponent()
    {
        std::uint8_t* cinematicEnter = nullptr;
        std::uint8_t* cinematicExit = nullptr;
        if (!ResolveCinematicFovCallsites(cinematicEnter, cinematicExit)) return false;
        auto enterHook = safetyhook::MidHook::create(
            cinematicEnter, SafeMidHookEntry<&TraceCinematicEnter>, safetyhook::MidHook::StartDisabled);
        auto exitHook = safetyhook::MidHook::create(
            cinematicExit, SafeMidHookEntry<&TraceCinematicExit>, safetyhook::MidHook::StartDisabled);
        if (!enterHook || !exitHook) return false;
        g_cinematicEnterHook = std::move(*enterHook);
        g_cinematicExitHook = std::move(*exitHook);
        if (!g_cinematicEnterHook || !g_cinematicExitHook) return false;
        return true;
    }

    bool CommitCinematicComponents()
    {
        if (!g_cinematicAspectStoreHook.enable() ||
            !g_cinematicEnterHook.enable() || !g_cinematicExitHook.enable()) {
            static_cast<void>(g_cinematicAspectStoreHook.disable());
            static_cast<void>(g_cinematicEnterHook.disable());
            static_cast<void>(g_cinematicExitHook.disable());
            return false;
        }
        g_cinematicHookGate.store(true, std::memory_order_release);
        return true;
    }

    void RollbackCinematicAspectComponent() noexcept
    {
        RestoreCinematicAspect();
    }

    void RollbackCinematicFovComponent() noexcept
    {
        g_cinematicHookGate.store(false, std::memory_order_release);
        static_cast<void>(g_cinematicExitHook.disable());
        static_cast<void>(g_cinematicEnterHook.disable());
        g_cinematicExitHook.reset();
        g_cinematicEnterHook.reset();
    }

    std::atomic<std::uint64_t> g_dialogueDiagnosticCallbacks{};
    std::atomic<std::uint64_t> g_dialogueDiagnosticReturn1{};
    std::atomic<std::uint64_t> g_dialogueDiagnosticReturn2{};
    std::atomic<std::uint64_t> g_dialogueDiagnosticReturn3{};
    std::atomic<std::uint64_t> g_dialogueDiagnosticReturn4{};
    std::atomic<std::uint64_t> g_dialogueDiagnosticReturn5{};

    void LogDialogueDiagnosticReturn(const char* reason, SafetyHookContext& context)
    {
        if (!diagnostics::Enabled()) return;
        static_cast<void>(context);
        if (std::strcmp(reason, "invalid-input") == 0) {
            g_dialogueDiagnosticReturn1.fetch_add(1, std::memory_order_relaxed);
        } else if (std::strcmp(reason, "non-gameplay") == 0) {
            g_dialogueDiagnosticReturn2.fetch_add(1, std::memory_order_relaxed);
        } else if (std::strcmp(reason, "native-policy") == 0) {
            g_dialogueDiagnosticReturn3.fetch_add(1, std::memory_order_relaxed);
        } else if (std::strcmp(reason, "processed") == 0) {
            g_dialogueDiagnosticReturn4.fetch_add(1, std::memory_order_relaxed);
        } else if (std::strcmp(reason, "post-cinematic-recovery") == 0) {
            g_dialogueDiagnosticReturn5.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void LogDialogueDiagnosticSummary()
    {
        if (!diagnostics::Enabled()) return;
        if (g_dialogueDiagnosticCallbacks.load(std::memory_order_relaxed) == 0) return;
        Log("DIALOGUE_CALLBACK_SUMMARY total=", g_dialogueDiagnosticCallbacks.load(std::memory_order_relaxed),
            " return1=", g_dialogueDiagnosticReturn1.load(std::memory_order_relaxed),
            " return2=", g_dialogueDiagnosticReturn2.load(std::memory_order_relaxed),
            " return3=", g_dialogueDiagnosticReturn3.load(std::memory_order_relaxed),
            " return4=", g_dialogueDiagnosticReturn4.load(std::memory_order_relaxed),
            " return5=", g_dialogueDiagnosticReturn5.load(std::memory_order_relaxed), ".");
    }

#ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC
    void TraceDialogueDiscovery(SafetyHookContext& context);
#endif

    constexpr std::uint32_t kDialogueRecoveryTraceMaxSamples = 256;

    struct DialogueRecoveryEndpointTrace
    {
        bool active{};
        std::uintptr_t source{};
        std::uint32_t samples{};
        bool previousValid{};
        bool readable{};
        DialoguePhase phase{DialoguePhase::Inactive};
        float currentFov{std::numeric_limits<float>::quiet_NaN()};
        float state28{std::numeric_limits<float>::quiet_NaN()};
        float targetFov{std::numeric_limits<float>::quiet_NaN()};
        float pairedFov{std::numeric_limits<float>::quiet_NaN()};
        bool wouldRearm{};
    };

    thread_local DialogueRecoveryEndpointTrace g_dialogueRecoveryEndpointTrace{};

    void ArmDialogueRecoveryEndpointTrace(std::uintptr_t source)
    {
        if (!diagnostics::Enabled()) return;
        g_dialogueRecoveryEndpointTrace = {};
        g_dialogueRecoveryEndpointTrace.active = source != 0;
        g_dialogueRecoveryEndpointTrace.source = source;
        if (g_dialogueRecoveryEndpointTrace.active)
            Log("DIALOGUE_RECOVERY_TRACE start source=0x", std::hex, source, std::dec, ".");
    }

    void ObserveDialogueRecoveryEndpointTrace(SafetyHookContext& context,
        DialoguePhase phase, float currentFov, float baseline, float previous,
        std::uint32_t stableSamples, bool wouldRearm)
    {
        if (!diagnostics::Enabled()) return;
        auto& trace = g_dialogueRecoveryEndpointTrace;
        if (!trace.active) return;

        const auto source = static_cast<std::uintptr_t>(context.rsi);
        if (!source || source != trace.source) {
            Log("DIALOGUE_RECOVERY_TRACE stop reason=source-change samples=", trace.samples, ".");
            trace = {};
            return;
        }
        if (trace.samples++ >= kDialogueRecoveryTraceMaxSamples) {
            Log("DIALOGUE_RECOVERY_TRACE stop reason=sample-limit samples=", trace.samples, ".");
            trace = {};
            return;
        }

        float state28 = std::numeric_limits<float>::quiet_NaN();
        float targetFov = std::numeric_limits<float>::quiet_NaN();
        float pairedFov = std::numeric_limits<float>::quiet_NaN();
        const bool readable = SafeRead(source + 0x28, state28) &&
            SafeRead(source + 0x2C, targetFov) && SafeRead(source + 0x30, pairedFov);
        const float comparisonPrevious = std::isfinite(previous) ? previous : trace.currentFov;
        const auto direction = !std::isfinite(comparisonPrevious) ||
            !std::isfinite(currentFov) ? "unknown" :
            currentFov > comparisonPrevious + kDialogueTransformEpsilon ? "up" :
            currentFov < comparisonPrevious - kDialogueTransformEpsilon ? "down" : "stable";
        const auto floatChanged = [](float a, float b) {
            return std::isfinite(a) != std::isfinite(b) ||
                (std::isfinite(a) && std::isfinite(b) && std::fabs(a - b) > 0.001f);
        };
        const bool changed = !trace.previousValid || trace.phase != phase ||
            trace.readable != readable || floatChanged(trace.currentFov, currentFov) ||
            floatChanged(trace.state28, state28) || floatChanged(trace.targetFov, targetFov) ||
            floatChanged(trace.pairedFov, pairedFov) || trace.wouldRearm != wouldRearm;
        if (changed) {
            const float deltaTarget = readable && std::isfinite(targetFov)
                ? currentFov - targetFov : std::numeric_limits<float>::quiet_NaN();
            const float deltaBaseline = std::isfinite(baseline)
                ? currentFov - baseline : std::numeric_limits<float>::quiet_NaN();
            Log("DIALOGUE_RECOVERY_TRACE phase=", PhaseName(phase),
                " current=", currentFov, " state28=", state28,
                " target=", targetFov, " paired=", pairedFov,
                " baseline=", baseline, " previous=", comparisonPrevious,
                " deltaTarget=", deltaTarget, " deltaBaseline=", deltaBaseline,
                " direction=", direction, " stableCount=", stableSamples,
                " wouldRearmCurrentPredicate=", wouldRearm ? 1 : 0,
                " readable=", readable ? 1 : 0, " samples=", trace.samples, ".");
        }
        trace.previousValid = true;
        trace.readable = readable;
        trace.phase = phase;
        trace.currentFov = currentFov;
        trace.state28 = state28;
        trace.targetFov = targetFov;
        trace.pairedFov = pairedFov;
        trace.wouldRearm = wouldRearm;
    }

    void PublishCameraFovTransaction(float nativeInput, float output,
        std::uintptr_t source, CoordinatorState coordinator,
        bool targetValid, float target, camera::EvidenceProvenance targetProvenance,
        camera::EvidenceProvenance outputProvenance);

    void TraceDialogueBoundary(SafetyHookContext& context)
    {
        diagnostics::RecordRuntimeEvent(diagnostics::RuntimeEvent::DialogueCameraSample);
        auto correctionLock = g_cameraCorrectionGate.Acquire();
        if (g_stopping.load(std::memory_order_acquire)) return;
#ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC
        TraceDialogueDiscovery(context);
#endif
        if (diagnostics::Enabled())
            g_dialogueDiagnosticCallbacks.fetch_add(1, std::memory_order_relaxed);
        const float incoming = context.xmm6.f32[0];
        const auto source = static_cast<std::uintptr_t>(context.rsi);
        const bool inputValid = std::isfinite(incoming) && incoming > 1.0f && incoming < 179.0f;
        DialoguePhase phaseBefore{DialoguePhase::Inactive};
        float nativeTarget = std::numeric_limits<float>::quiet_NaN();
        bool nativeTargetReadable{};
        const auto result = [&] {
            diagnostics::ScopedRuntimeMutex lock(g_dialogueMutex,
                diagnostics::RuntimeLockPath::Dialogue);
            nativeTargetReadable = inputValid && source != 0 &&
                SafeRead(source + 0x2C, nativeTarget) && IsValidFovValue(nativeTarget);
            if (diagnostics::Enabled()) {
                const auto recoveryPhase = g_dialoguePhase;
                const auto stableSamples = g_dialogueRecoveryRearm.StableSamples();
                const bool recoverySampleStable = recoveryPhase == DialoguePhase::RearmPending &&
                    nativeTargetReadable && std::isfinite(g_dialoguePrevious) &&
                    std::fabs(incoming - nativeTarget) <= kRecoveryEndpointToleranceDegrees &&
                    std::fabs(incoming - g_dialoguePrevious) <= kDialogueTransformEpsilon;
                const bool wouldRearmCurrentPredicate = recoverySampleStable &&
                    stableSamples + 1 >= kDialogueRecoveryStableSamples;
                ObserveDialogueRecoveryEndpointTrace(context, recoveryPhase, incoming,
                    g_dialogueBaseline, g_dialoguePrevious, stableSamples,
                    wouldRearmCurrentPredicate);
            }
            phaseBefore = g_dialoguePhase;
            if (inputValid)
                RecordDialogueSourceCallback(phaseBefore, source);
            const plugin::DialogueBoundaryInput input{incoming, source, nativeTarget,
                nativeTargetReadable, kDialogueTransformEpsilon,
                kRecoveryEndpointToleranceDegrees, kDialogueCandidateStableSamples};
            return plugin::ProcessDialogueBoundary(GetDialogueBoundaryStateView(), input);
        }();
        if (result.stateReset || phaseBefore != result.phaseAfter)
            MarkCameraStateFreshness({
                camera::CameraStateFreshnessGroup::Dialogue,
                camera::CameraStateFreshnessGroup::TransformedResult,
                camera::CameraStateFreshnessGroup::GameplayFov});

        if (result.stateReset) {
            RecordDialogueSourceEnd("runtime-reset");
            if (result.activePolicyReleased)
                Log("Dialogue active policy released: policy=",
                    DialogueZoomPolicyName(result.releasedPolicy), ".");
        }

        if (result.action == plugin::DialogueBoundaryAction::InvalidInput) {
            if (result.stateReset) Log("Dialogue candidate cancelled: reason=invalid-sample.");
            LogDialogueDiagnosticReturn("invalid-input", context);
            return;
        }
        if (result.action == plugin::DialogueBoundaryAction::ExclusionReset) {
            LogDialogueDiagnosticReturn("post-cinematic-recovery", context);
            return;
        }
        if (result.action == plugin::DialogueBoundaryAction::NonGameplayReset) {
            LogDialogueDiagnosticReturn("non-gameplay", context);
            return;
        }
        if (result.action == plugin::DialogueBoundaryAction::CandidateCaptured)
            Log("Dialogue candidate captured: provisionalBaselineG=", result.baseline,
                " source=0x", std::hex, source, std::dec, " nativeTarget=", nativeTarget,
                " selectedPolicy=", result.selectedPolicy == DialogueZoomPolicy::Native
                    ? "OFF" : DialogueZoomPolicyName(result.selectedPolicy),
                ". No transform applied.");
        else if (result.action == plugin::DialogueBoundaryAction::CandidateRejected)
            Log("Dialogue candidate ignored: reason=invalid-context policy=",
                result.selectedPolicy == DialogueZoomPolicy::Native
                    ? "OFF" : DialogueZoomPolicyName(result.selectedPolicy),
                " source=0x", std::hex, source, std::dec, " target=", nativeTarget,
                " targetReadable=", result.candidateContextValid ? 1 : 0, ".");
        else if (result.action == plugin::DialogueBoundaryAction::CandidateCancelled)
            Log("Dialogue candidate cancelled: reason=trajectory-not-confirmed.");
        else if (result.action == plugin::DialogueBoundaryAction::AcceptedStart) {
            Log("Dialogue lifecycle started after confirmed native descent: baselineG=",
                result.baseline, " activePolicy=",
                g_activeDialoguePolicy.IsValid()
                    ? DialogueZoomPolicyName(g_activeDialoguePolicy.Value()) : "UNSELECTED_OFF",
                " selectedPolicy=", result.selectedPolicy == DialogueZoomPolicy::Native
                    ? "OFF" : DialogueZoomPolicyName(result.selectedPolicy),
                " correctionEnabledAtStart=",
                g_dialogueCorrectionActivation.CanCorrect(true) ? "true" : "false", ".");
            if (diagnostics::Enabled()) {
                if (auto* trace = GetDialogueSourceProvenanceTrace()) {
                    const auto owner = CoordinatorStateName(
                        g_coordinator.load(std::memory_order_acquire));
                    EmitDialogueSourceEvent(trace->Accepted(true, result.acceptedSource,
                        owner, ReadDialogueWriterSnapshot()));
                }
            }
        }
        if (result.action == plugin::DialogueBoundaryAction::LifecycleExiting) {
            ArmDialogueRecoveryEndpointTrace(source);
            Log("Dialogue lifecycle exiting: incoming=", incoming,
                " baselineG=", result.baseline, ".");
        }
        if (!result.earlyReturn)
            RecordDialogueSourceCallback(result.phaseAtCallback, source);
        if (result.action == plugin::DialogueBoundaryAction::RecoveryStarted) {
            RecordDialogueRecoverySource(result.recoverySource);
            Log("Dialogue recovery entered re-arm pending: baselineG=", result.baseline,
                " incoming=", incoming, " nativeTarget=", nativeTarget,
                " targetReadable=", nativeTargetReadable ? 1 : 0,
                ". Awaiting native FOV convergence.");
        } else if (result.action == plugin::DialogueBoundaryAction::RecoveryCompleted) {
            Log("Dialogue recovery re-armed after native FOV convergence: targetFov=",
                nativeTarget, " baselineG=", result.baseline, ".");
        } else if (result.action == plugin::DialogueBoundaryAction::RecoveryCancelled) {
            Log("Dialogue recovery cancelled: reason=source-changed.");
        }
        if (result.mutateXmm1) context.xmm1.f32[0] = result.outputFov;
        if (phaseBefore == DialoguePhase::Active ||
            phaseBefore == DialoguePhase::Exiting ||
            phaseBefore == DialoguePhase::RearmPending ||
            result.phaseAfter == DialoguePhase::Active ||
            result.phaseAfter == DialoguePhase::Exiting ||
            result.phaseAfter == DialoguePhase::RearmPending) {
            const auto coordinator = g_coordinator.load(std::memory_order_acquire);
            PublishCameraFovTransaction(incoming,
                result.mutateXmm1 ? result.outputFov : incoming, source,
                coordinator, false, std::numeric_limits<float>::quiet_NaN(),
                camera::EvidenceProvenance::Unavailable,
                result.mutateXmm1 ? camera::EvidenceProvenance::ModDerivedState
                    : camera::EvidenceProvenance::PassThroughResult);
        }
        LogDialogueDiagnosticReturn("processed", context);
    }

    bool InstallDialogueBoundary()
    {
        const auto matches = hooks::FindAll(g_executable, DialogueBoundary);
        if (matches.size() != 1) {
            Log("Dialogue boundary resolver rejected: matches=", matches.size(), ". Native pass-through retained.");
            return false;
        }
        auto* hook = matches.front() + kDialogueBoundaryHookOffset;
        constexpr std::uint8_t expected[] = { 0xFF, 0x90, 0x08, 0x06, 0x00, 0x00 };
        if (std::memcmp(hook, expected, sizeof(expected)) != 0) {
            Log("Dialogue boundary instruction contract rejected. Native pass-through retained.");
            return false;
        }
        g_dialogueBoundaryHook = safetyhook::create_mid(
            hook, SafeMidHookEntry<&TraceDialogueBoundary>);
        if (!g_dialogueBoundaryHook || !g_dialogueBoundaryHook.enable()) {
            g_dialogueBoundaryHook.reset();
            Log("Dialogue boundary hook setup failed. Native pass-through retained.");
            return false;
        }
        g_dialogueBoundaryHookAvailable.store(true, std::memory_order_release);
        Log("Dialogue boundary hook installed: RVA=0x", std::hex,
            reinterpret_cast<std::uintptr_t>(hook) - reinterpret_cast<std::uintptr_t>(g_executable),
            std::dec, " policy=", DialogueZoomPolicyName(g_runtimeDialoguePolicy.load(std::memory_order_acquire)), ".");
        return true;
    }

    struct HorPlusCinematicWriterTraceSnapshot
    {
        bool valid{};
        std::uintptr_t source{};
        std::uintptr_t output{};
        float writerFov{};
        float cameraWorldFov{};
        float cameraFirstPersonFov{};
        float aspect{};
        std::uint8_t flags{};
        float outputFov{};
        float outputAspect{};
        bool eligible{};
        float cachedEnterFov{std::numeric_limits<float>::quiet_NaN()};
        float enterAspect{std::numeric_limits<float>::quiet_NaN()};
        bool cacheValid{};
        bool numericGuardMatched{};
    };

    thread_local HorPlusCinematicWriterTraceSnapshot g_previousHorPlusCinematicWriter{};
    thread_local bool g_logHorPlusCinematicWriterInvocation{};

    bool ReadHorPlusCinematicWriterState(SafetyHookContext& context, float writerFov,
        HorPlusCinematicWriterTraceSnapshot& snapshot)
    {
        snapshot = {};
        snapshot.source = static_cast<std::uintptr_t>(context.rsi);
        snapshot.output = static_cast<std::uintptr_t>(context.rbx);
        snapshot.writerFov = writerFov;
        snapshot.cachedEnterFov = g_cinematicTransformedFov.load(std::memory_order_acquire);
        snapshot.enterAspect = g_cinematicEnterAspect.load(std::memory_order_acquire);
        snapshot.cacheValid = std::isfinite(snapshot.cachedEnterFov);
        snapshot.numericGuardMatched = snapshot.cacheValid &&
            camera::AreFovValuesSemanticallyEquivalent(writerFov, snapshot.cachedEnterFov);
        if (!snapshot.source || !snapshot.output ||
            !SafeRead(snapshot.source + kAspectOffset, snapshot.aspect) ||
            !SafeRead(snapshot.source + kFlagsOffset, snapshot.flags) ||
            !SafeRead(snapshot.source + 0x230, snapshot.cameraWorldFov) ||
            !SafeRead(snapshot.source + 0x234, snapshot.cameraFirstPersonFov) ||
            !SafeRead(snapshot.output + 0x30, snapshot.outputFov) ||
            !SafeRead(snapshot.output + 0x5C, snapshot.outputAspect)) return false;
        float candidate{};
        snapshot.eligible =
            g_coordinator.load(std::memory_order_acquire) == CoordinatorState::CinematicActive &&
            gameplay::TryTransformHorPlus(writerFov, snapshot.aspect, snapshot.flags,
                kNativeAspect, candidate);
        snapshot.valid = true;
        return true;
    }

    bool HorPlusCinematicWriterSnapshotChanged(
        const HorPlusCinematicWriterTraceSnapshot& current)
    {
        const auto floatChanged = [](float left, float right) {
            return std::isfinite(left) != std::isfinite(right) ||
                (std::isfinite(left) && std::isfinite(right) && std::fabs(left - right) > 0.01f);
        };
        const auto& previous = g_previousHorPlusCinematicWriter;
        return !previous.valid || current.source != previous.source || current.output != previous.output ||
            floatChanged(current.writerFov, previous.writerFov) ||
            floatChanged(current.cameraWorldFov, previous.cameraWorldFov) ||
            floatChanged(current.cameraFirstPersonFov, previous.cameraFirstPersonFov) ||
            floatChanged(current.aspect, previous.aspect) || current.flags != previous.flags ||
            floatChanged(current.outputFov, previous.outputFov) ||
            floatChanged(current.outputAspect, previous.outputAspect) ||
            current.eligible != previous.eligible ||
            floatChanged(current.cachedEnterFov, previous.cachedEnterFov) ||
            floatChanged(current.enterAspect, previous.enterAspect) ||
            current.cacheValid != previous.cacheValid ||
            current.numericGuardMatched != previous.numericGuardMatched;
    }

    void LogHorPlusCinematicWriterTrace(const char* phase,
        const HorPlusCinematicWriterTraceSnapshot& snapshot, float beforeFov, float afterFov,
        float candidateNativeFov)
    {
        const float gameplayNative = g_matchGameplayPreEnterNativeFov.load(std::memory_order_acquire);
        const float gameplayHorPlus = g_matchGameplayPreEnterHorPlusFov.load(std::memory_order_acquire);
        const float gameplayAspect = g_matchGameplayPreEnterAspect.load(std::memory_order_acquire);
        const float enterObservation = g_matchGameplayEnterNativeFov.load(std::memory_order_acquire);
        const bool pairValid = g_matchGameplayPreEnterPairValid.load(std::memory_order_acquire);
        const diagnostics::matchgameplay::PredictionInput input{
            snapshot.numericGuardMatched, pairValid, gameplayNative, gameplayHorPlus,
            gameplayAspect, enterObservation, candidateNativeFov, snapshot.aspect};
        const auto prediction = diagnostics::matchgameplay::Evaluate(input, kNativeAspect);
        const char* sampleSpace = prediction.sampleSpace ==
                diagnostics::matchgameplay::SampleSpace::CachedTransformedEnter
            ? "CACHED_TRANSFORMED_ENTER"
            : prediction.sampleSpace == diagnostics::matchgameplay::SampleSpace::Native
                ? "NATIVE"
                : "UNAVAILABLE";
        Log("HORPLUS_CINEMATIC_WRITER ", phase,
            " coordinator=", CoordinatorStateName(g_coordinator.load(std::memory_order_acquire)),
            " source=0x", std::hex, snapshot.source, " output=0x", snapshot.output, std::dec,
            " xmm0Before=", beforeFov, " xmm0After=", afterFov,
            " eligible=", snapshot.eligible ? "YES" : "NO",
            " applied=", std::fabs(afterFov - beforeFov) > 0.001f ? "YES" : "NO",
            " cameraWorldFov=", snapshot.cameraWorldFov,
            " cameraFirstPersonFov=", snapshot.cameraFirstPersonFov,
            " aspect=", snapshot.aspect, " flags=0x", std::hex,
            static_cast<unsigned>(snapshot.flags), std::dec,
            " outputFOV=", snapshot.outputFov, " outputAspect=", snapshot.outputAspect,
            " cachedEnterFov=", snapshot.cachedEnterFov,
            " enterAspect=", snapshot.enterAspect,
            " cacheValid=", snapshot.cacheValid ? "true" : "false",
            " numericGuardMatched=", snapshot.numericGuardMatched ? "true" : "false"
            , " matchGameplayCandidateAvailable=", prediction.candidateAvailable ? "true" : "false",
            " matchGameplaySampleSpace=", sampleSpace,
            " matchGameplayReferenceSource=ENTER_OBSERVATION",
            " matchGameplayEnterObservation=", enterObservation,
            " matchGameplayNativeBaseline=", gameplayNative,
            " matchGameplayHorPlusBaseline=", gameplayHorPlus,
            " matchGameplayBaselineAspect=", gameplayAspect,
            " matchGameplaySampleNative=", candidateNativeFov,
            " matchGameplayNativeMatched=", prediction.nativeMatched,
            " matchGameplayHorPlus=", prediction.horPlusMatched
            , ".");
    }

    void TraceHorPlusCinematicWriterBefore(SafetyHookContext& context)
    {
        if (!diagnostics::Enabled()) return;
        g_logHorPlusCinematicWriterInvocation = false;
        if (g_coordinator.load(std::memory_order_acquire) != CoordinatorState::CinematicActive) return;
        HorPlusCinematicWriterTraceSnapshot snapshot{};
        if (!ReadHorPlusCinematicWriterState(context, context.xmm0.f32[0], snapshot)) return;
        if (HorPlusCinematicWriterSnapshotChanged(snapshot)) {
            LogHorPlusCinematicWriterTrace("BEFORE", snapshot, context.xmm0.f32[0], context.xmm0.f32[0],
                context.xmm0.f32[0]);
            g_logHorPlusCinematicWriterInvocation = true;
        }
    }

    void TraceHorPlusCinematicWriterAfter(SafetyHookContext& context, float beforeFov)
    {
        if (!diagnostics::Enabled()) return;
        if (g_coordinator.load(std::memory_order_acquire) != CoordinatorState::CinematicActive &&
            !g_logHorPlusCinematicWriterInvocation) return;
        HorPlusCinematicWriterTraceSnapshot snapshot{};
        if (!ReadHorPlusCinematicWriterState(context, context.xmm0.f32[0], snapshot)) return;
        if (g_logHorPlusCinematicWriterInvocation || HorPlusCinematicWriterSnapshotChanged(snapshot))
            LogHorPlusCinematicWriterTrace("AFTER", snapshot, beforeFov, context.xmm0.f32[0], beforeFov);
        g_previousHorPlusCinematicWriter = snapshot;
        g_logHorPlusCinematicWriterInvocation = false;
    }

    using HorPlusGameplayApplyResult = plugin::CameraWriterGameplayResult;

    struct GameplayFovSourceTraceSnapshot
    {
        bool initialized{};
        std::uintptr_t source{};
        bool selectedFieldReadable{};
        float selectedField{};
        bool primaryFieldReadable{};
        float primaryField{};
        float writerInput{};
        float horPlusResult{};
        float aspect{};
        float policyAspect{};
        bool policyAspectValid{};
        float currentViewportAspect{};
        bool currentViewportAspectValid{};
        std::uint8_t flags{};
        CoordinatorState coordinator{CoordinatorState::Gameplay};
        bool gameplayEnabled{};
        config::GameplayMode gameplayMode{config::GameplayMode::RuntimeCombine};
    };

    bool FovTraceValueChanged(float current, float previous, float threshold) noexcept
    {
        const bool currentFinite = std::isfinite(current);
        const bool previousFinite = std::isfinite(previous);
        return currentFinite != previousFinite ||
            (currentFinite && previousFinite && std::fabs(current - previous) >= threshold);
    }

    void TraceGameplayFovSourceSample(const HorPlusGameplayApplyResult& result,
        bool selectedFieldReadable, float selectedField,
        bool primaryFieldReadable, float primaryField,
        CoordinatorState coordinator, bool gameplayEnabled,
        config::GameplayMode gameplayMode)
    {
        if (!diagnostics::Enabled()) return;
        thread_local GameplayFovSourceTraceSnapshot previous{};
        static std::atomic<std::uint64_t> nextTraceSequence{};

        const bool selectedFieldValid = selectedFieldReadable &&
            std::isfinite(selectedField) && selectedField > 1.0f && selectedField < 179.0f;
        const bool primaryFieldValid = primaryFieldReadable &&
            std::isfinite(primaryField) && primaryField > 1.0f && primaryField < 179.0f;
        constexpr float candidateThreshold = 0.01f;
        constexpr float cameraFovEventThreshold = 1.0f;
        constexpr float aspectEventThreshold = 0.001f;

        const bool event = !previous.initialized ||
            previous.source != result.source ||
            previous.selectedFieldReadable != selectedFieldReadable ||
            FovTraceValueChanged(selectedField, previous.selectedField, candidateThreshold) ||
            previous.primaryFieldReadable != primaryFieldReadable ||
            FovTraceValueChanged(primaryField, previous.primaryField, cameraFovEventThreshold) ||
            FovTraceValueChanged(result.inputFov, previous.writerInput, cameraFovEventThreshold) ||
            FovTraceValueChanged(result.outputFov, previous.horPlusResult, cameraFovEventThreshold) ||
            FovTraceValueChanged(result.aspect, previous.aspect, aspectEventThreshold) ||
            FovTraceValueChanged(result.policyAspect, previous.policyAspect,
                aspectEventThreshold) ||
            result.policyAspectValid != previous.policyAspectValid ||
            FovTraceValueChanged(result.currentViewportAspect,
                previous.currentViewportAspect, aspectEventThreshold) ||
            result.currentViewportAspectValid != previous.currentViewportAspectValid ||
            result.flags != previous.flags || coordinator != previous.coordinator ||
            gameplayEnabled != previous.gameplayEnabled || gameplayMode != previous.gameplayMode;
        if (!event) return;

        const auto sequence = nextTraceSequence.fetch_add(1, std::memory_order_relaxed) + 1;
        const float unavailable = std::numeric_limits<float>::quiet_NaN();
        Log("GAMEPLAY_FOV_SOURCE_TRACE seq=", sequence,
            " writerSeq=", result.observationPublished
                ? result.observation.publicationSequence : 0,
            " threadId=", GetCurrentThreadId(),
            " source=0x", std::hex, result.source, std::dec,
            " coordinator=", CoordinatorStateName(coordinator),
            " gameplayEnabled=", gameplayEnabled ? "true" : "false",
            " gameplayMode=", config::GameplayModeName(gameplayMode),
            " flags=0x", std::hex, static_cast<unsigned>(result.flags), std::dec,
            " nativeWriterAspect=", result.aspect,
            " horPlusPolicyAspect=", result.policyAspect,
            " horPlusPolicyAspectValid=", result.policyAspectValid ? "true" : "false",
            " currentViewportAspect=", result.currentViewportAspect,
            " currentViewportAspectValid=", result.currentViewportAspectValid ? "true" : "false",
            " selectedCandidate(+0x234)=", selectedFieldReadable ? selectedField : unavailable,
            " selectedFieldReadable=", selectedFieldReadable ? "true" : "false",
            " selectedFieldValid=", selectedFieldValid ? "true" : "false",
            " primaryFov(+0x230)=", primaryFieldReadable ? primaryField : unavailable,
            " primaryFieldValid=", primaryFieldValid ? "true" : "false",
            " writerInput=", result.inputFov,
            " horPlusResult=", result.outputFov,
            " eligible=", result.eligible ? "true" : "false",
            " applied=", result.applied ? "true" : "false",
            " reason=", result.reason, ".");

        previous = GameplayFovSourceTraceSnapshot{
            true, result.source, selectedFieldReadable, selectedField,
            primaryFieldReadable, primaryField, result.inputFov, result.outputFov,
            result.aspect, result.policyAspect, result.policyAspectValid,
            result.currentViewportAspect, result.currentViewportAspectValid,
            result.flags, coordinator, gameplayEnabled, gameplayMode };
    }

    void TraceHorPlusRecoverySample(SafetyHookContext& context,
        float inputFov, float outputFov, bool mapped, const char* reason)
    {
        if (!diagnostics::Enabled()) return;
        HorPlusGameplayApplyResult result{};
        result.source = static_cast<std::uintptr_t>(context.rsi);
        result.inputFov = inputFov;
        result.outputFov = outputFov;
        result.eligible = mapped;
        result.applied = mapped;
        result.reason = reason;
        float selectedFovCandidate = std::numeric_limits<float>::quiet_NaN();
        bool selectedFovCandidateReadable = false;
        float primaryCameraFov = std::numeric_limits<float>::quiet_NaN();
        bool primaryCameraFovReadable = false;
        if (result.source) {
            result.sourceReadable = SafeRead(result.source + kAspectOffset, result.aspect) &&
                SafeRead(result.source + kFlagsOffset, result.flags);
            selectedFovCandidateReadable = SafeRead(
                result.source + 0x234, selectedFovCandidate);
            primaryCameraFovReadable = SafeRead(
                result.source + 0x230, primaryCameraFov);
        }
        TraceGameplayFovSourceSample(result,
            selectedFovCandidateReadable, selectedFovCandidate,
            primaryCameraFovReadable, primaryCameraFov,
            CoordinatorState::CinematicExiting,
            g_runtimeGameplayEnabled.load(std::memory_order_acquire),
            config::GameplayMode::HorPlus);
    }

    void MarkCameraStateFreshness(
        std::initializer_list<camera::CameraStateFreshnessGroup> groups)
    {
        camera::CameraStateSnapshot previous{};
        camera::CameraStateSnapshot current{};
        bool changed = false;
        {
            diagnostics::ScopedRuntimeMutex stateLock(g_cameraStateMutex,
                diagnostics::RuntimeLockPath::CameraState);
            previous = g_cameraStateSnapshot;
            for (const auto group : groups)
                camera::MarkCameraStateGroupHistorical(g_cameraStateSnapshot,
                    g_cameraStateSnapshotValid, group);
            current = g_cameraStateSnapshot;
            changed = camera::SnapshotSemanticsChanged(current, previous);
        }
        if (changed) NotifyOverlayCameraStatePublished();
    }

    void MarkCameraPresentationTransition(bool includeDialogue)
    {
        camera::CameraStateSnapshot previous{};
        camera::CameraStateSnapshot current{};
        bool changed = false;
        {
            diagnostics::ScopedRuntimeMutex stateLock(g_cameraStateMutex,
                diagnostics::RuntimeLockPath::CameraState);
            previous = g_cameraStateSnapshot;
            camera::MarkCameraPresentationTransition(g_cameraStateSnapshot,
                g_cameraStateSnapshotValid);
            if (includeDialogue)
                camera::MarkCameraDialogueTransition(g_cameraStateSnapshot,
                    g_cameraStateSnapshotValid);
            current = g_cameraStateSnapshot;
            changed = camera::SnapshotSemanticsChanged(current, previous);
        }
        if (changed) NotifyOverlayCameraStatePublished();
    }

    void MarkCameraStateForSetting(void*,
        plugin::CameraSettingFreshnessChange change)
    {
        camera::CameraStateSnapshot previous{};
        camera::CameraStateSnapshot current{};
        bool changed = false;
        {
            diagnostics::ScopedRuntimeMutex stateLock(g_cameraStateMutex,
                diagnostics::RuntimeLockPath::CameraState);
            previous = g_cameraStateSnapshot;
            plugin::MarkCameraSettingFactsHistorical(g_cameraStateSnapshot,
                g_cameraStateSnapshotValid, change);
            current = g_cameraStateSnapshot;
            changed = camera::SnapshotSemanticsChanged(current, previous);
        }
        if (changed) NotifyOverlayCameraStatePublished();
    }

    void PublishCameraStateSnapshot(const HorPlusGameplayApplyResult& result)
    {
        if (!result.observationPublished) return;
        diagnostics::RecordRuntimeEvent(diagnostics::RuntimeEvent::CameraStatePublish);

        const auto coordinator = g_coordinator.load(std::memory_order_acquire);
        const auto mode = g_runtimeGameplayMode.load(std::memory_order_acquire);
        const bool zoomActive = g_snapshotZoomActive.load(std::memory_order_acquire);
        const bool recoveryExcluded = g_postCinematicDialogueExclusion.IsActive();
        DialoguePhase dialoguePhase = DialoguePhase::Inactive;
        {
            diagnostics::ScopedRuntimeMutex dialogueLock(g_dialogueMutex, diagnostics::RuntimeLockPath::Dialogue);
            dialoguePhase = g_dialoguePhase;
        }

        camera::CameraStatePublicationInput input{};
        input.observationPublished = result.observationPublished;
        input.observation = result.observation;
        input.source = result.source;
        input.applied = result.applied;
        input.presentation = coordinator == CoordinatorState::CinematicActive
            ? camera::PresentationState::CinematicActive
            : coordinator == CoordinatorState::CinematicExiting
                ? camera::PresentationState::CinematicExiting
                : coordinator == CoordinatorState::Gameplay
                    ? camera::PresentationState::Gameplay
                    : camera::PresentationState::Unknown;
        switch (dialoguePhase) {
        case DialoguePhase::Inactive: input.dialogue = camera::DialogueState::Inactive; break;
        case DialoguePhase::Candidate: input.dialogue = camera::DialogueState::Candidate; break;
        case DialoguePhase::Active: input.dialogue = camera::DialogueState::Active; break;
        case DialoguePhase::Exiting: input.dialogue = camera::DialogueState::Exiting; break;
        case DialoguePhase::RearmPending: input.dialogue = camera::DialogueState::Recovery; break;
        }
        input.recoveryExcluded = recoveryExcluded;
        input.modeTransitionPending =
            g_gameplayModeTransitionPending.load(std::memory_order_acquire);
        input.zoomActive = zoomActive;
        input.zoomObservationValid = g_runtime.snapshotZoomObservationValid.load(
            std::memory_order_acquire);
        input.zoomSource = g_runtime.snapshotZoomSource.load(std::memory_order_acquire);
        input.zoomPrimary = g_runtime.snapshotZoomPrimary.load(std::memory_order_acquire);
        input.zoomSecondary = g_runtime.snapshotZoomSecondary.load(std::memory_order_acquire);
        input.gameplayMode = static_cast<std::uint8_t>(mode);
        input.nativeAspect = kNativeAspect;
        input.runtimeCombineValid = result.runtimeCombineTelemetryValid;
        input.runtimeCombineAspectBefore = result.runtimeCombineAspectBefore;
        input.runtimeCombineTargetAspect = result.runtimeCombineTargetAspect;
        input.runtimeCombineAspectAfter = result.runtimeCombineAspectAfter;
        input.runtimeCombineFovInput = result.runtimeCombineFovInput;
        input.runtimeCombineFovOutput = result.runtimeCombineFovOutput;
        input.runtimeCombineFlagsBefore = result.runtimeCombineFlagsBefore;
        input.runtimeCombineFlagsAfter = result.runtimeCombineFlagsAfter;
        input.runtimeCombineMismatch = result.runtimeCombineMismatch;
        input.runtimeCombineWriteAttempted = result.runtimeCombineWriteAttempted;
        input.runtimeCombineWriteSucceeded = result.runtimeCombineWriteSucceeded;
        input.runtimeCombineReadbackValid = result.runtimeCombineReadbackValid;
        input.runtimeCombineConfirmed = result.runtimeCombineConfirmed;
        input.runtimeCombineFovApplied = result.runtimeCombineFovApplied;

        bool changed = false;
        {
            diagnostics::ScopedRuntimeMutex stateLock(g_cameraStateMutex, diagnostics::RuntimeLockPath::CameraState);
            if (dialoguePhase == DialoguePhase::Active ||
                dialoguePhase == DialoguePhase::Exiting ||
                dialoguePhase == DialoguePhase::RearmPending)
                input.activeDialoguePolicyValid = g_activeDialoguePolicy.IsValid();
            input.zoomDirection = g_runtime.snapshotZoomDirection.load(std::memory_order_acquire) == 1
                ? camera::ZoomDirection::In
                : g_runtime.snapshotZoomDirection.load(std::memory_order_acquire) == 2
                    ? camera::ZoomDirection::Out : camera::ZoomDirection::None;
            input.zoomSequence = g_runtime.snapshotZoomSequence.load(std::memory_order_acquire);
            const auto publication = camera::PublishCameraStateSnapshotLocked({
                g_cameraStateSnapshot, g_cameraStateSnapshotValid,
                g_neutralGameplayFovCandidate, g_neutralGameplayFovSamples,
                g_neutralGameplayFovCandidateSource, g_snapshotZoomHold,
                g_runtime.snapshotEventSequence}, input);
            changed = publication.published && camera::SnapshotSemanticsChanged(
                publication.current, publication.previous);
            if (diagnostics::Enabled() && camera::SnapshotSemanticsChanged(
                    publication.current, publication.previous))
                Log("CAMERA_STATE_SNAPSHOT updated eventSequence=",
                    publication.current.generation.eventSequence,
                    " gameplayFov=", publication.current.evidence.gameplayFov.value,
                    " nativeFov=", publication.current.evidence.nativeFov.value,
                    " horPlusTarget=", publication.current.evidence.horPlusTargetFov.value,
                    " currentOutput=", publication.current.evidence.currentOutputFov.value,
                    " zoomActive=", publication.current.zoom.active ? "true" : "false", ".");
        }
        if (changed) NotifyOverlayCameraStatePublished();
    }

    void PublishCameraFovTransaction(float nativeInput, float output,
        std::uintptr_t source, CoordinatorState coordinator,
        bool targetValid, float target, camera::EvidenceProvenance targetProvenance,
        camera::EvidenceProvenance outputProvenance)
    {
        camera::CameraFovTransactionPublication publication{};
        publication.nativeInput = nativeInput;
        publication.nativeInputValid = IsValidFovValue(nativeInput);
        publication.nativeProvenance = camera::EvidenceProvenance::NativeNumericObservation;
        publication.output = output;
        publication.outputValid = IsValidFovValue(output);
        publication.outputProvenance = outputProvenance;
        publication.horPlusTarget = target;
        publication.horPlusTargetValid = targetValid && IsValidFovValue(target);
        publication.targetProvenance = targetProvenance;
        publication.transactionOwner = coordinator == CoordinatorState::Gameplay
            ? camera::PresentationState::Gameplay
            : coordinator == CoordinatorState::CinematicActive
                ? camera::PresentationState::CinematicActive
                : coordinator == CoordinatorState::CinematicExiting
                    ? camera::PresentationState::CinematicExiting
                    : camera::PresentationState::Unknown;
        publication.source = source;

        bool changed = false;
        camera::CameraStateSnapshot current{};
        {
            diagnostics::ScopedRuntimeMutex stateLock(g_cameraStateMutex,
                diagnostics::RuntimeLockPath::CameraState);
            publication.sequence = g_runtime.snapshotEventSequence.fetch_add(
                1, std::memory_order_acq_rel) + 1;
            const auto previous = g_cameraStateSnapshot;
            camera::PublishCameraFovTransactionLocked(g_cameraStateSnapshot,
                g_cameraStateSnapshotValid, publication);
            g_cameraStateSnapshot.generation.eventSequence = publication.sequence;
            g_cameraStateSnapshot.generation.eventSequenceValid = true;
            changed = camera::SnapshotSemanticsChanged(g_cameraStateSnapshot, previous);
            current = g_cameraStateSnapshot;
        }
        if (changed) {
            if (diagnostics::Enabled())
                Log("CAMERA_FOV_STATE transaction_owner=",
                    camera::PresentationStateName(publication.transactionOwner),
                    " coordinator=", CoordinatorStateName(coordinator),
                    " last_full_publication_state=",
                    camera::PresentationStateName(current.presentation.state),
                    " sequence=", current.generation.eventSequence,
                    " gameplay=", current.evidence.gameplayFov.value,
                    " gameplay_freshness=", camera::SnapshotFreshnessName(
                        current.evidence.gameplayFov.freshness),
                    " native=", current.evidence.nativeFov.value,
                    " target=", current.evidence.horPlusTargetFov.value,
                    " target_freshness=", camera::SnapshotFreshnessName(
                        current.evidence.horPlusTargetFov.freshness),
                    " output=", current.evidence.currentOutputFov.value,
                    " source=0x", std::hex, source, std::dec, ".");
            NotifyOverlayCameraStatePublished();
        }
    }

    enum class HorPlusTelemetryOwner : std::uint8_t
    {
        Gameplay,
        Dialogue,
        Cinematic,
        Recovery,
        Unknown,
    };

    const char* HorPlusTelemetryOwnerName(HorPlusTelemetryOwner owner)
    {
        switch (owner) {
        case HorPlusTelemetryOwner::Gameplay: return "Gameplay";
        case HorPlusTelemetryOwner::Dialogue: return "Dialogue";
        case HorPlusTelemetryOwner::Cinematic: return "Cinematic";
        case HorPlusTelemetryOwner::Recovery: return "Recovery";
        case HorPlusTelemetryOwner::Unknown: return "Unknown";
        }
        return "Unknown";
    }

    HorPlusTelemetryOwner ResolveHorPlusTelemetryOwner()
    {
        if (g_coordinator.load(std::memory_order_acquire) == CoordinatorState::CinematicActive)
            return HorPlusTelemetryOwner::Cinematic;
        if (g_postCinematicDialogueExclusion.IsActive())
            return HorPlusTelemetryOwner::Recovery;
        {
            diagnostics::ScopedRuntimeMutex dialogueLock(g_dialogueMutex, diagnostics::RuntimeLockPath::Dialogue);
            if (g_dialoguePhase == DialoguePhase::Active ||
                g_dialoguePhase == DialoguePhase::Exiting ||
                g_dialoguePhase == DialoguePhase::RearmPending)
                return HorPlusTelemetryOwner::Dialogue;
        }
        if (g_coordinator.load(std::memory_order_acquire) == CoordinatorState::Gameplay)
            return HorPlusTelemetryOwner::Gameplay;
        return HorPlusTelemetryOwner::Unknown;
    }

    void TraceHorPlusFovState(const HorPlusGameplayApplyResult& applyResult)
    {
        if (!diagnostics::Enabled()) return;
        const auto owner = ResolveHorPlusTelemetryOwner();
        const auto mode = g_runtimeGameplayMode.load(std::memory_order_acquire);
        const bool observationAvailable = applyResult.observationPublished;
        const auto& observation = applyResult.observation;
        const float nativeFov = applyResult.inputFov;
        const float aspect = applyResult.aspect;
        const auto flags = applyResult.flags;
        const float transformedFov = applyResult.applied ? applyResult.outputFov :
            std::numeric_limits<float>::quiet_NaN();
        const bool transformed = applyResult.applied;
        const auto ownerValue = static_cast<std::uint8_t>(owner);

        const auto currentCoordinator = g_coordinator.load(std::memory_order_acquire);
        DialoguePhase dialoguePhase = DialoguePhase::Inactive;
        {
            diagnostics::ScopedRuntimeMutex dialogueLock(g_dialogueMutex, diagnostics::RuntimeLockPath::Dialogue);
            dialoguePhase = g_dialoguePhase;
        }
        const bool stableContext = owner == HorPlusTelemetryOwner::Gameplay &&
            currentCoordinator == CoordinatorState::Gameplay &&
            dialoguePhase == DialoguePhase::Inactive &&
            !g_postCinematicDialogueExclusion.IsActive() &&
            applyResult.sourceReadable && std::isfinite(nativeFov);

        std::lock_guard telemetryLock(g_horPlusFovTelemetryMutex);
        const bool ownerChanged = !g_runtime.horPlusFovTelemetryValid ||
            g_runtime.horPlusTelemetryOwner != ownerValue;
        if (ownerChanged) {
            const auto previous = g_runtime.horPlusFovTelemetryValid
                ? HorPlusTelemetryOwnerName(static_cast<HorPlusTelemetryOwner>(g_runtime.horPlusTelemetryOwner))
                : "None";
            Log("CAMERA_OWNER from=", previous, " to=", HorPlusTelemetryOwnerName(owner), ".");
        }

        const auto floatChanged = [](float a, float b) {
            return std::isfinite(a) != std::isfinite(b) ||
                (std::isfinite(a) && std::isfinite(b) && std::fabs(a - b) > 0.01f);
        };
        const bool nativeChanged = !g_runtime.horPlusFovTelemetryValid ||
            floatChanged(nativeFov, g_runtime.horPlusTelemetryNativeFov);
        const bool aspectChanged = !g_runtime.horPlusFovTelemetryValid ||
            floatChanged(aspect, g_runtime.horPlusTelemetryAspect);
        const bool modeChanged = !g_runtime.horPlusFovTelemetryValid ||
            mode != g_runtime.horPlusTelemetryMode;

        if (aspectChanged || modeChanged) {
            g_runtime.horPlusGameplayCacheValid = false;
            Log("HORPLUS_STATE valid=false reason=", modeChanged ? "MODE_CHANGED" : "ASPECT_CHANGED",
                " configured=UNKNOWN nativeGameplay=", nativeFov, " aspect=", aspect,
                " mode=", config::GameplayModeName(mode), " owner=", HorPlusTelemetryOwnerName(owner), ".");
        }
        if (applyResult.gameplayStateEstablished && applyResult.applied) {
            if (!g_runtime.horPlusGameplayCacheValid) {
                g_runtime.horPlusGameplayCacheValid = true;
                Log("HORPLUS_STATE valid=true reason=GAMEPLAY_STATE_ESTABLISHED configured=UNKNOWN nativeGameplay=",
                    nativeFov, " horPlusGameplay=", transformedFov, " aspect=", aspect,
                    " mode=", config::GameplayModeName(mode), " owner=", HorPlusTelemetryOwnerName(owner), ".");
            }
        }
        if (nativeChanged) {
            const char* event = owner == HorPlusTelemetryOwner::Cinematic ||
                owner == HorPlusTelemetryOwner::Recovery ? "CINEMATIC_FOV_CHANGE" :
                owner == HorPlusTelemetryOwner::Dialogue ? "DIALOGUE_FOV_CHANGE" :
                owner == HorPlusTelemetryOwner::Unknown ? "UNKNOWN_FOV_CHANGE" : "GAMEPLAY_FOV_CHANGE";
            Log(event, " configured=UNKNOWN nativeOld=", g_runtime.horPlusTelemetryNativeFov,
                " nativeNew=", nativeFov, " modifier=UNKNOWN horPlusOld=",
                g_runtime.horPlusTelemetryResult, " horPlusNew=", transformedFov,
                " nativeBeforeHorPlus=", nativeFov,
                " horPlusResult=", transformedFov,
                " aspect=", aspect, " owner=", HorPlusTelemetryOwnerName(owner),
                " cacheValid=", g_runtime.horPlusGameplayCacheValid ? "true" : "false",
                " reason=", applyResult.reason, " handled=", transformed ? "true" : "false", ".");
            if (observationAvailable) {
                Log("FOV_OBSERVATION boundary=", camera::FovObservationBoundaryName(observation.boundary),
                    " input=", observation.inputFov.value,
                    " inputSpace=", camera::FovSpaceName(observation.inputFov.space),
                    " inputProvenance=", camera::FovProvenanceName(observation.inputFov.provenance),
                    " result=", observation.resultFov.value,
                    " resultSpace=", camera::FovSpaceName(observation.resultFov.space),
                    " resultProvenance=", camera::FovProvenanceName(observation.resultFov.provenance),
                    " aspect=", observation.aspect.value,
                    " horPlusPolicyAspect=", observation.policyAspect.value,
                    " currentViewportAspect=", observation.currentViewportAspect.value,
                    " writerSource=0x", std::hex, observation.writerSource.value, std::dec,
                    " writerSourceValid=", observation.writerSource.valid ? "true" : "false",
                    " sequence=", observation.publicationSequence, ".");
                Log("GAMEPLAY_BASELINE_PROJECTION disposition=",
                    camera::GameplayBaselineProjectionDispositionName(
                        applyResult.baselineProjection.disposition),
                    " eligible=", applyResult.baselineProjection.eligible ? "true" : "false",
                    " valid=", applyResult.baselineProjection.baseline.valid ? "true" : "false",
                    " native=", applyResult.baselineProjection.baseline.nativeFov.value,
                    " nativeValid=", applyResult.baselineProjection.baseline.nativeFov.valid ? "true" : "false",
                    " horPlus=", applyResult.baselineProjection.baseline.horPlusFov.value,
                    " horPlusValid=", applyResult.baselineProjection.baseline.horPlusFov.valid ? "true" : "false",
                    " aspect=", applyResult.baselineProjection.baseline.aspect,
                    " source=0x", std::hex,
                    applyResult.baselineProjection.baseline.source.value, std::dec,
                    " sequence=", applyResult.baselineProjection.baseline.observationSequence, ".");
            }
        }

        if (!stableContext) {
            g_runtime.stableObservationSource = 0;
            g_runtime.stableObservationFov = std::numeric_limits<float>::quiet_NaN();
            g_runtime.stableObservationSamples = 0;
            g_runtime.stableObservationReported = false;
        } else if (g_runtime.stableObservationSource == applyResult.source &&
            std::isfinite(g_runtime.stableObservationFov) &&
            std::fabs(g_runtime.stableObservationFov - nativeFov) <= 0.01f) {
            ++g_runtime.stableObservationSamples;
        } else {
            g_runtime.stableObservationSource = applyResult.source;
            g_runtime.stableObservationFov = nativeFov;
            g_runtime.stableObservationSamples = 1;
            g_runtime.stableObservationReported = false;
        }
        if (stableContext && g_runtime.stableObservationSamples >= 3 &&
            !g_runtime.stableObservationReported) {
            g_runtime.stableObservationReported = true;
            Log("STABLE_GAMEPLAY_ENDPOINT_OBSERVATION fov=", nativeFov,
                " source=0x", std::hex, applyResult.source, std::dec,
                " stableSamples=", g_runtime.stableObservationSamples,
                " coordinator=", CoordinatorStateName(currentCoordinator),
                " dialoguePhase=", DialoguePhaseName(dialoguePhase),
                " recoveryExclusion=false",
                " lastZoomDirection=observation-only",
                " lastZoomSequence=", g_runtime.snapshotZoomSequence.load(std::memory_order_acquire),
                " hypothesis=stable-endpoint-only.");
        }
        g_runtime.horPlusFovTelemetryValid = true;
        g_runtime.horPlusTelemetryNativeFov = nativeFov;
        g_runtime.horPlusTelemetryAspect = aspect;
        g_runtime.horPlusTelemetryResult = transformedFov;
        g_runtime.horPlusTelemetryFlags = flags;
        g_runtime.horPlusTelemetryMode = mode;
        g_runtime.horPlusTelemetryOwner = ownerValue;

    }

    void ReplayManualTransition(SafetyHookContext& context)
    {
        diagnostics::RecordRuntimeEvent(diagnostics::RuntimeEvent::GameplayCameraSample);
        if (!g_cameraWriterHookGate.load(std::memory_order_acquire) ||
            g_stopping.load(std::memory_order_acquire)) return;
        auto correctionLock = g_cameraCorrectionGate.Acquire();
        if (!g_cameraWriterHookGate.load(std::memory_order_acquire) ||
            g_stopping.load(std::memory_order_acquire)) return;
        auto coordinator = g_coordinator.load(std::memory_order_acquire);
        const auto observedSource = static_cast<std::uintptr_t>(context.rsi);
        const auto observedOutput = static_cast<std::uintptr_t>(context.rbx);
        const float writerInputXmm0 = context.xmm0.f32[0];
        const bool associationDiagnosticsActive = diagnostics::Enabled();
        float observedAspect = 0.0f;
        std::uint8_t observedFlags = 0;
        const bool observedAspectValid = observedSource != 0 &&
            SafeRead(observedSource + kAspectOffset, observedAspect) &&
            IsValidAspect(observedAspect);
        const bool observedFlagsValid = observedSource != 0 &&
            SafeRead(observedSource + kFlagsOffset, observedFlags);
        const bool modeChanged = observedAspectValid && observedFlagsValid &&
            LogCameraModeChange(observedSource, observedOutput, writerInputXmm0,
                observedAspect, observedFlags);
        const auto transitionTraceSequence = modeChanged
            ? g_transitionTraceSequence.fetch_add(1, std::memory_order_relaxed) + 1
            : 0;
        if (modeChanged)
            LogTransitionSnapshot("PRE", transitionTraceSequence, observedSource);
        const auto logTransitionPost = [&] {
            if (modeChanged)
                LogTransitionSnapshot("POST", transitionTraceSequence, observedSource);
#ifdef COMBINED_GAMEPLAY_DIAGNOSTIC
            if (modeChanged && g_logCombinedOutputForInvocation)
                LogCombinedOutputPost(transitionTraceSequence, observedSource, observedOutput);
#endif
        };
        const auto gameplayWriterState = GetCameraWriterGameplayStateView();
        const bool gameplayEnabled = g_runtimeGameplayEnabled.load(
            std::memory_order_acquire);
        const auto gameplayMode = g_runtimeGameplayMode.load(
            std::memory_order_acquire);
        float currentViewportAspect = std::numeric_limits<float>::quiet_NaN();
        if (gameplayEnabled &&
            (gameplayMode == config::GameplayMode::HorPlus ||
                gameplayMode == config::GameplayMode::RuntimeCombine) &&
            coordinator != CoordinatorState::CinematicActive) {
            currentViewportAspect =
                platform::win32::ReadCurrentProcessClientViewportAspect();
        }
        plugin::CameraWriterInput writerInput{
            observedSource, observedOutput, writerInputXmm0, observedAspect,
            observedAspectValid, observedFlags, observedFlagsValid,
            kRecoveryEndpointToleranceDegrees, gameplayEnabled, gameplayMode,
            kNativeAspect, kDialogueContextFovJump};
        writerInput.currentViewportAspect = currentViewportAspect;
        writerInput.currentViewportAspectValid = std::isfinite(currentViewportAspect) &&
            currentViewportAspect > 0.0f;
        const auto writerTransaction = plugin::ProcessCameraWriter(
            g_cameraCorrectionGate,
            CameraWriterRecoveryStateView(),
            writerInput, GetCameraWriterCinematicStateView(),
            GetCameraWriterEngineOperations(), {nullptr, +[](void*) {
                diagnostics::ScopedRuntimeMutex dialogueLock(
                    g_dialogueMutex, diagnostics::RuntimeLockPath::Dialogue);
                ResetDialogueRuntimeState();
            }, +[](void*) {
                diagnostics::ScopedRuntimeMutex dialogueLock(
                    g_dialogueMutex, diagnostics::RuntimeLockPath::Dialogue);
                return plugin::DialogueBoundaryIsActive(
                    GetDialogueBoundaryStateView());
            }}, &gameplayWriterState,
            {nullptr,
                +[](void*, std::uintptr_t previousSource, float previousFov,
                    std::uintptr_t source, float fov) {
                    diagnostics::ScopedRuntimeMutex dialogueLock(
                        g_dialogueMutex, diagnostics::RuntimeLockPath::Dialogue);
                    if (!plugin::DialogueBoundaryIsActive(
                        GetDialogueBoundaryStateView())) return false;
                    Log("Dialogue runtime invalidated by gameplay camera context change: sourceChanged=",
                        previousSource != source, " previousSource=0x", std::hex,
                        previousSource, " source=0x", source, std::dec,
                        " previousFOV=", previousFov, " currentFOV=", fov,
                        ". Native pass-through until a new descent.");
                    ResetDialogueRuntimeState();
                    return true;
                }});
        const bool writerFovTransactionObserved = writerTransaction.processed &&
            !writerTransaction.gameplay.handled &&
            (writerTransaction.cinematicHandled ||
                writerTransaction.horPlusRecoveryProcessed);
        if (writerFovTransactionObserved) {
            bool targetValid = false;
            float horPlusTarget = std::numeric_limits<float>::quiet_NaN();
            auto targetProvenance = camera::EvidenceProvenance::ModDerivedState;
            if (writerTransaction.gameplay.handled && writerTransaction.gameplay.applied) {
                horPlusTarget = writerTransaction.gameplay.outputFov;
                targetValid = IsValidFovValue(horPlusTarget);
            } else if (writerTransaction.coordinatorAfter ==
                    CoordinatorState::CinematicActive &&
                g_cinematicFovApplied.load(std::memory_order_acquire)) {
                horPlusTarget = g_cinematicTransformedFov.load(std::memory_order_acquire);
                targetValid = IsValidFovValue(horPlusTarget);
                targetProvenance = camera::EvidenceProvenance::ConfirmedModLifecycle;
            } else if (writerTransaction.coordinatorAfter ==
                    CoordinatorState::CinematicExiting &&
                g_cinematicFovApplied.load(std::memory_order_acquire)) {
                horPlusTarget = g_cinematicTransformedFov.load(std::memory_order_acquire);
                targetValid = IsValidFovValue(horPlusTarget);
                targetProvenance = camera::EvidenceProvenance::ConfirmedModLifecycle;
            }
            const auto outputProvenance = writerTransaction.mutateFov
                ? camera::EvidenceProvenance::ModDerivedState
                : camera::EvidenceProvenance::PassThroughResult;
            PublishCameraFovTransaction(writerTransaction.inputFov,
                writerTransaction.outputFov, observedSource,
                writerTransaction.coordinatorAfter, targetValid, horPlusTarget,
                targetProvenance, outputProvenance);
        }
        if (diagnostics::Enabled() &&
            writerTransaction.gameplay.runtimeCombineTelemetryValid) {
            struct RuntimeCombineDiagnosticSnapshot
            {
                bool valid{};
                std::uintptr_t source{};
                float before{};
                float target{};
                float after{};
                float inputFov{};
                float outputFov{};
                std::uint8_t flagsBefore{};
                std::uint8_t flagsAfter{};
                bool mismatch{};
                bool attempted{};
                bool writeSucceeded{};
                bool readbackValid{};
                bool confirmed{};
                bool fovApplied{};
            };
            static thread_local RuntimeCombineDiagnosticSnapshot previous{};
            const auto& current = writerTransaction.gameplay;
            const auto changedFloat = [](float a, float b) {
                return std::isfinite(a) != std::isfinite(b) ||
                    (std::isfinite(a) && std::isfinite(b) &&
                        std::fabs(a - b) > 0.001f);
            };
            const bool changed = !previous.valid || previous.source != observedSource ||
                changedFloat(previous.before, current.runtimeCombineAspectBefore) ||
                changedFloat(previous.target, current.runtimeCombineTargetAspect) ||
                changedFloat(previous.after, current.runtimeCombineAspectAfter) ||
                changedFloat(previous.inputFov, current.runtimeCombineFovInput) ||
                changedFloat(previous.outputFov, current.runtimeCombineFovOutput) ||
                previous.flagsBefore != current.runtimeCombineFlagsBefore ||
                previous.flagsAfter != current.runtimeCombineFlagsAfter ||
                previous.mismatch != current.runtimeCombineMismatch ||
                previous.attempted != current.runtimeCombineWriteAttempted ||
                previous.writeSucceeded != current.runtimeCombineWriteSucceeded ||
                previous.readbackValid != current.runtimeCombineReadbackValid ||
                previous.confirmed != current.runtimeCombineConfirmed ||
                previous.fovApplied != current.runtimeCombineFovApplied;
            if (changed) {
                Log("RUNTIME_COMBINE source=0x", std::hex, observedSource,
                    " flags_before=0x", static_cast<unsigned>(current.runtimeCombineFlagsBefore),
                    " flags_after=0x", static_cast<unsigned>(current.runtimeCombineFlagsAfter),
                    std::dec, " aspect_before=", current.runtimeCombineAspectBefore,
                    " target=", current.runtimeCombineTargetAspect,
                    " mismatch=", current.runtimeCombineMismatch ? 1 : 0,
                    " write_attempted=", current.runtimeCombineWriteAttempted ? 1 : 0,
                    " write_succeeded=", current.runtimeCombineWriteSucceeded ? 1 : 0,
                    " aspect_after=", current.runtimeCombineAspectAfter,
                    " readback_valid=", current.runtimeCombineReadbackValid ? 1 : 0,
                    " confirmed=", current.runtimeCombineConfirmed ? 1 : 0,
                    " fov_input=", current.runtimeCombineFovInput,
                    " fov_output=", current.runtimeCombineFovOutput,
                    " fov_applied=", current.runtimeCombineFovApplied ? 1 : 0,
                    " reason=", current.reason, ".");
                previous = {true, observedSource,
                    current.runtimeCombineAspectBefore,
                    current.runtimeCombineTargetAspect,
                    current.runtimeCombineAspectAfter,
                    current.runtimeCombineFovInput,
                    current.runtimeCombineFovOutput,
                    current.runtimeCombineFlagsBefore,
                    current.runtimeCombineFlagsAfter,
                    current.runtimeCombineMismatch,
                    current.runtimeCombineWriteAttempted,
                    current.runtimeCombineWriteSucceeded,
                    current.runtimeCombineReadbackValid,
                    current.runtimeCombineConfirmed,
                    current.runtimeCombineFovApplied};
            }
        }
#ifdef POST_EXIT_TRACE_STATE
        if (writerTransaction.recoveryRetired) {
            g_postExitTraceArmed.store(false, std::memory_order_release);
            g_postExitTraceWriterCount.store(0, std::memory_order_release);
            g_postExitTraceStartNs.store(0, std::memory_order_release);
        }
#endif
        const auto& associationBefore = writerTransaction.previous;
        const auto& observedAssociation = writerTransaction.current;
        const auto& writerPairTransition = writerTransaction.pairTransition;
        if (writerPairTransition.changed &&
            writerPairTransition.previous.source !=
                writerPairTransition.current.source)
            MarkCameraStateFreshness({
                camera::CameraStateFreshnessGroup::WriterObservation,
                camera::CameraStateFreshnessGroup::TransformedResult,
                camera::CameraStateFreshnessGroup::GameplayFov});
        if (coordinator != writerTransaction.coordinatorAfter)
            MarkCameraPresentationTransition(false);
        if (writerTransaction.gameplay.dialogueContextInvalidated)
            MarkCameraStateFreshness({
                camera::CameraStateFreshnessGroup::Dialogue,
                camera::CameraStateFreshnessGroup::TransformedResult,
                camera::CameraStateFreshnessGroup::GameplayFov});
        const auto postExitProbeGeneration =
            diagnostics::TakeCinematicExitWriterProbe(
                g_cinematicExitLogProbeGeneration, associationDiagnosticsActive);
        if (postExitProbeGeneration != 0) {
            OutputCameraLogProbeDebug("first-post-exit-writer", postExitProbeGeneration,
                observedAssociation.latestWriter.sequence, observedSource,
                observedOutput, coordinator);
            Log("CAMERA_LOG_PROBE stage=first-post-exit-writer generation=",
                postExitProbeGeneration, " sequence=",
                observedAssociation.latestWriter.sequence, " rsi=0x", std::hex,
                observedSource, " rbx=0x", observedOutput, std::dec,
                " coordinator=", CoordinatorStateName(coordinator), ".");
            DeferCameraLogProbeFlush("after-first-post-exit-writer",
                postExitProbeGeneration, observedAssociation.latestWriter.sequence,
                observedSource, observedOutput, coordinator);
        }
        if (associationDiagnosticsActive) {
            if (associationBefore.latestWriter.sequence == 0 ||
                associationBefore.latestWriter.source != observedAssociation.latestWriter.source ||
                associationBefore.latestWriter.output != observedAssociation.latestWriter.output) {
                const auto identityChange = diagnostics::MakeCameraWriterIdentityChange(
                    true, associationBefore, observedAssociation, coordinator,
                    writerTransaction.recoveryOwnerBefore);
                if (identityChange.valid) {
                    const char* marker = identityChange.initialization
                        ? "CAMERA_WRITER_IDENTITY_INITIALIZED"
                        : "CAMERA_WRITER_IDENTITY_CHANGED";
                    Log(marker, " old_rsi=0x", std::hex,
                        identityChange.oldRsi, " old_rbx=0x", identityChange.oldRbx,
                        " new_rsi=0x", identityChange.newRsi, " new_rbx=0x",
                        identityChange.newRbx, std::dec,
                        " change_kind=",
                        identityChange.initialization ? "Initialization" :
                            diagnostics::CameraWriterIdentityChangeKindName(
                                identityChange.changeKind),
                        " old_identity_epoch=", identityChange.oldIdentityEpoch,
                        " new_identity_epoch=", identityChange.newIdentityEpoch,
                        " old_sequence=", identityChange.oldSequence,
                        " new_sequence=", identityChange.newSequence,
                        " coordinator=", CoordinatorStateName(identityChange.coordinator),
                        " cinematic_generation_active=",
                        identityChange.cinematicGenerationActive ? 1 : 0,
                        " cinematic_generation=", identityChange.cinematicGeneration,
                        " recovery_owner_valid=", identityChange.recoveryOwner.valid ? 1 : 0,
                        " recovery_owner_generation=",
                        identityChange.recoveryOwner.generation,
                        " recovery_owner_rsi=0x", std::hex,
                        identityChange.recoveryOwner.writerRsi,
                        " recovery_owner_rbx=0x",
                        identityChange.recoveryOwner.observedWriterRbx, std::dec,
                        " recovery_complete=",
                        identityChange.recoveryOwner.recoveryComplete ? 1 : 0, ".");
                }
            }
            diagnostics::EmitIfEnabled([&] {
                LogCinematicDiagnosticAssociationTransition(associationBefore,
                    observedAssociation);
            });
        }
        auto recoveryOwner = writerTransaction.recoveryOwnerBefore;
        const auto recoveryGenerationBeforeWriterChange =
            writerTransaction.recoveryOwnerBefore.generation;
        const bool recoveryRetired = writerTransaction.recoveryRetired;
        if (recoveryRetired) {
                    if (writerTransaction.handoffCancelled)
                        Log("AtomicExitHandoffCancelled: reason=writer-rsi-changed.");
                    if (writerTransaction.exclusionCleared)
                        Log("Post-cinematic Dialogue exclusion cleared: reason=writer-rsi-changed.");
                    if (associationDiagnosticsActive) {
                        OutputCameraLogProbeDebug("retirement-begin",
                            recoveryOwner.generation,
                            writerPairTransition.current.sequence,
                            writerPairTransition.current.source,
                            writerPairTransition.current.output, coordinator);
                        Log("CAMERA_LOG_PROBE stage=retirement-begin generation=",
                            recoveryOwner.generation, " sequence=",
                            writerPairTransition.current.sequence, " owner_rsi=0x",
                            std::hex, recoveryOwner.writerRsi, " new_rsi=0x",
                            writerPairTransition.current.source, " new_rbx=0x",
                            writerPairTransition.current.output, std::dec,
                            " coordinator=", CoordinatorStateName(coordinator), ".");
                    }
                    if (associationDiagnosticsActive) {
                        Log("CINEMATIC_RECOVERY_RETIRED_WRITER_CHANGE generation=",
                            recoveryOwner.generation,
                            " owner_rsi=0x", std::hex,
                            recoveryOwner.writerRsi,
                            " owner_rbx=0x", recoveryOwner.observedWriterRbx,
                            " previous_rsi=0x", writerPairTransition.previous.source,
                            " previous_rbx=0x", writerPairTransition.previous.output,
                            " new_rsi=0x", writerPairTransition.current.source,
                            " new_rbx=0x", writerPairTransition.current.output,
                            std::dec,
                            " previous_sequence=", writerPairTransition.previous.sequence,
                            " writer_sequence=", writerPairTransition.current.sequence,
                            " previous_identity_epoch=",
                            writerPairTransition.previous.identityEpoch,
                            " writer_identity_epoch=",
                            writerPairTransition.current.identityEpoch,
                            " resulting_coordinator=Gameplay recovery_owner_valid=0.");
                        OutputCameraLogProbeDebug("retirement-complete",
                            recoveryOwner.generation,
                            writerPairTransition.current.sequence,
                            writerPairTransition.current.source,
                            writerPairTransition.current.output,
                            CoordinatorState::Gameplay);
                        Log("CAMERA_LOG_PROBE stage=retirement-complete generation=",
                            recoveryOwner.generation, " sequence=",
                            writerPairTransition.current.sequence,
                            " new_rsi=0x", std::hex,
                            writerPairTransition.current.source,
                            " new_rbx=0x", writerPairTransition.current.output,
                            std::dec, " coordinator=Gameplay recovery_owner_valid=0.");
                    }
        }
        if (recoveryRetired) {
            coordinator = CoordinatorState::Gameplay;
            recoveryOwner = {};
            DeferCameraLogProbeFlush("after-writer-change-retirement",
                recoveryGenerationBeforeWriterChange,
                writerPairTransition.current.sequence,
                writerPairTransition.current.source,
                writerPairTransition.current.output, coordinator);
        } else {
            recoveryOwner = writerTransaction.recoveryOwnerAfter;
        }
        if (diagnostics::Enabled())
            RecordEnterWriterObservation(context);
        const float horPlusTraceBeforeFov = context.xmm0.f32[0];
        TraceHorPlusCinematicWriterBefore(context);
#if defined(POST_EXIT_RAW_TRACE_DIAGNOSTIC) || defined(POST_EXIT_INTERPOLATED_FOV_PRODUCER_TRACE)
        ObservePostExitRawWriterEntry(context);
#endif
        if (writerTransaction.continuation ==
                plugin::CameraWriterContinuation::RecoveryWithoutOwner) {
            if (writerTransaction.handoffCancelled)
                Log("AtomicExitHandoffCancelled: reason=exiting-without-owner.");
            if (writerTransaction.exclusionCleared)
                Log("Post-cinematic Dialogue exclusion cleared: reason=exiting-without-owner.");
            if (writerTransaction.noOwnerRecoveryCleanup && diagnostics::Enabled())
                Log("CINEMATIC_RECOVERY_NO_OWNER reason=exiting-without-owner coordinator=",
                    CoordinatorStateName(writerTransaction.coordinatorAfter),
                    " recovery_armed=0.");
            return;
        }
        if (writerTransaction.aspectRecoveryEndpoint) {
            FlushCinematicContinuationTrace("validated_shared_native_endpoint");
            {
                diagnostics::ScopedRuntimeMutex dialogueLock(
                    g_dialogueMutex, diagnostics::RuntimeLockPath::Dialogue);
                ResetDialogueRuntimeState();
            }
            Log("Shared cinematic recovery complete with Gameplay correction disabled: native endpoint validated.");
            return;
        }
        if (writerTransaction.atomicExitHandoff.handled) {
            switch (writerTransaction.atomicExitHandoff.event) {
            case plugin::AtomicExitHandoffEvent::ForeignSource:
                LogForeignRecoverySource(observedSource, recoveryOwner,
                    CoordinatorState::CinematicExiting);
                return;
            case plugin::AtomicExitHandoffEvent::Applied:
                Log("AtomicExitHandoffApplied: source=0x", std::hex,
                    observedSource, std::dec, " fov=", writerInputXmm0,
                    " aspect=", kNativeAspect, " flags=0x4 previousAspect=",
                    writerTransaction.atomicExitHandoff.previousAspect,
                    " previousFlags=0x", std::hex,
                    static_cast<unsigned>(writerTransaction.atomicExitHandoff.previousFlags),
                    std::dec, ".");
                return;
            case plugin::AtomicExitHandoffEvent::WriteFailed:
                Log("AtomicExitHandoffRefused: final aspect/flags fields were not writable.");
                return;
            default:
                return;
            }
        }
        if (writerTransaction.continuation ==
                plugin::CameraWriterContinuation::RecoveryForeignSource ||
            writerTransaction.continuation ==
                plugin::CameraWriterContinuation::PostRecoveryForeignSource) {
            LogForeignRecoverySource(observedSource, recoveryOwner, coordinator);
            return;
        }
        auto recoveryObservation = writerTransaction.exclusionObservation;
        if (recoveryObservation ==
                dialogue::PostCinematicRecoveryExclusion::Observation::Cancelled)
            Log("Post-cinematic Dialogue exclusion cleared: reason=source-or-sample-mismatch.");
        else if (recoveryObservation ==
                dialogue::PostCinematicRecoveryExclusion::Observation::Recovered)
            Log("Post-cinematic Dialogue exclusion cleared: reason=native-recovery-converged.");
        if (writerTransaction.horPlusRecoveryProcessed) {
            const auto& recoveryTransaction = writerTransaction.horPlusRecovery;
            if (recoveryTransaction.action ==
                    gameplay::HorPlusRecoveryAction::TransformRecoveryInterpolation) {
                const float nativeInterpolationFov = context.xmm0.f32[0];
                context.xmm0.f32[0] = writerTransaction.outputFov;
                TraceHorPlusRecoverySample(context, nativeInterpolationFov,
                    writerTransaction.outputFov, true,
                    "POST_CINEMATIC_RECOVERY_INTERPOLATION");
                return;
            }
            if (recoveryTransaction.action ==
                    gameplay::HorPlusRecoveryAction::HoldNativePassThrough) {
                // Unowned, ambiguous, out-of-segment, or invalid samples stay
                // native pass-through. Re-arm mutation was completed inside
                // the shared production transaction.
                TraceHorPlusRecoverySample(context, context.xmm0.f32[0],
                    writerTransaction.outputFov, false,
                    writerTransaction.recoveryExclusionRearmed
                        ? "POST_CINEMATIC_RECOVERY_REARM"
                        : "POST_CINEMATIC_RECOVERY_HOLD");
                return;
            }
            if (recoveryTransaction.action ==
                    gameplay::HorPlusRecoveryAction::ResumeGameplay) {
                FlushCinematicContinuationTrace("validated_horplus_recovery");
                if (recoveryTransaction.completed && diagnostics::Enabled())
                    Log("CINEMATIC_RECOVERY_COMPLETED generation=",
                        recoveryTransaction.completion.owner.generation,
                        " source=0x", std::hex, observedSource, std::dec,
                        " path=horplus-native-endpoint exclusion_active=",
                        recoveryTransaction.completion.exclusionActive ? 1 : 0,
                        " recovery_owner_authority=none.");
                Log("HorPlus gameplay resumed: source-owned native gameplay sample matched EXIT target.");
            }
        }
        // The Cinematic writer decision is part of ProcessCameraWriter; this
        // block retains observation-only diagnostics at the hook boundary.
        if (coordinator == CoordinatorState::CinematicActive) {
            const auto continuationStartFov = writerTransaction.continuationStartFov;
            const bool continuationInterpolationActive =
                writerTransaction.continuationInterpolationActive;
            RecordCinematicContinuationTrace(associationDiagnosticsActive,
                continuationInterpolationActive, observedSource,
                writerTransaction.continuationSourceMatch, writerInputXmm0,
                writerTransaction.outputFov, continuationStartFov,
                static_cast<std::uint8_t>(writerTransaction.cinematicAction));
            if (associationDiagnosticsActive) {
                float rawWriterFov = std::numeric_limits<float>::quiet_NaN();
                const bool rawWriterFovValid = observedSource != 0 &&
                    SafeRead(observedSource + kCameraWorldFovOffset, rawWriterFov) &&
                    IsValidFovValue(rawWriterFov);
                diagnostics::EmitIfEnabled([&] {
                    LogCinematicDiagnosticWriterState(writerTransaction.associationAfterDecision,
                        CoordinatorState::CinematicActive, observedSource, observedOutput,
                        rawWriterFov, rawWriterFovValid, writerInputXmm0,
                        writerTransaction.outputFov,
                        writerTransaction.cinematicAction,
                        writerTransaction.cinematicHandled,
                        writerTransaction.initialEstablishmentAuthorized,
                        writerTransaction.continuationSourceMatch);
                });
            }
            if (writerTransaction.disposition ==
                    plugin::CameraWriterDisposition::FinalOutput &&
                writerTransaction.cinematicHandled) {
                if (writerTransaction.mutateFov)
                    context.xmm0.f32[0] = writerTransaction.outputFov;
                TraceHorPlusCinematicWriterAfter(context, horPlusTraceBeforeFov);
                return;
            }
        }
        if (writerTransaction.gameplay.handled) {
            const auto& applyResult = writerTransaction.gameplay;
            const auto& modeTransition = applyResult.modeTransition;
            if (modeTransition.shouldLog) {
                switch (modeTransition.event) {
                case plugin::GameplayModeTransitionEvent::DeferredUnreadable:
                    Log("Gameplay mode transition deferred: reason=unreadable_camera_state",
                        " retryBoundary=GameplayCameraWriter.");
                    if (diagnostics::Enabled())
                        Log("RESTORATION_CONSUMER decision=DEFER reason=unreadable_camera_state.");
                    break;
                case plugin::GameplayModeTransitionEvent::ConsumedNoWrite:
                    Log("Gameplay mode transition applied: AspectRecalculation -> HorPlus; "
                        "actual runtime aspect already available aspect=", modeTransition.currentAspect,
                        " retryBoundary=GameplayCameraWriter flags=0x", std::hex,
                        static_cast<unsigned>(modeTransition.flags), std::dec, ".");
                    break;
                case plugin::GameplayModeTransitionEvent::DeferredNativeNotEstablished:
                    Log("Gameplay mode transition deferred: reason=actual_runtime_aspect_not_established",
                        " currentAspect=", modeTransition.currentAspect,
                        " restorationAspect=", modeTransition.restorationAspect,
                        " restorationValid=", IsValidAspect(modeTransition.restorationAspect),
                        " currentSource=0x", std::hex, observedSource, std::dec,
                        " retryBoundary=GameplayCameraWriter.");
                    break;
                case plugin::GameplayModeTransitionEvent::RestoreWriteFailed:
                    Log("Gameplay mode transition deferred: reason=actual_aspect_restore_not_writable",
                        " retryBoundary=GameplayCameraWriter.");
                    break;
                case plugin::GameplayModeTransitionEvent::RestoreApplied:
                    Log("Gameplay mode transition applied: AspectRecalculation -> HorPlus; restored aspect=",
                        modeTransition.restorationAspect, " flags=0x", std::hex,
                        static_cast<unsigned>(modeTransition.flags), std::dec,
                        " retryBoundary=GameplayCameraWriter.");
                    break;
                default:
                    break;
                }
            }
            const auto& enabledTransition = applyResult.enabledTransition;
            if (enabledTransition.shouldLog) {
                switch (enabledTransition.event) {
                case plugin::GameplayEnabledTransitionEvent::Deferred: {
                    const char* reason = writerTransaction.coordinatorAfter !=
                            CoordinatorState::Gameplay
                        ? "coordinator_not_gameplay"
                        : (!enabledTransition.currentAspectValid
                            ? "camera_aspect_unavailable"
                            : (enabledTransition.selectedMode ==
                                config::GameplayMode::AspectRecalculation
                                ? "native_transition_not_ready"
                                : "restoration_target_unavailable"));
                    Log("Gameplay.Enabled transition deferred: enabled=",
                        enabledTransition.enabled ? "true" : "false",
                        " reason=", reason, " mode=",
                        enabledTransition.enabled
                            ? config::GameplayModeName(enabledTransition.selectedMode)
                            : "OFF-cleanup",
                        " coordinator=", CoordinatorStateName(writerTransaction.coordinatorAfter),
                        " replayState=", ReplayStateName(enabledTransition.replayState),
                        " cameraReadable=", enabledTransition.cameraReadable ? "true" : "false",
                        " aspect=", enabledTransition.currentAspect,
                        " restorationAspect=", enabledTransition.restoration.aspect,
                        " restorationValid=", enabledTransition.restoration.valid &&
                            IsValidAspect(enabledTransition.restoration.aspect)
                            ? "true" : "false",
                        " restorationSourceMatches=",
                        enabledTransition.restorationSourceMatches ? "true" : "false",
                        " retryBoundary=GameplayCameraWriter.");
                    break;
                }
                case plugin::GameplayEnabledTransitionEvent::RestoreWriteFailed:
                    Log("Gameplay.Enabled transition failed: reason=native_aspect_restore_not_writable",
                        " enabled=", enabledTransition.enabled ? "true" : "false",
                        " pending=true retryBoundary=GameplayCameraWriter.");
                    break;
                case plugin::GameplayEnabledTransitionEvent::RestoreApplied:
                    if (enabledTransition.enabled)
                        Log("Gameplay.Enabled re-enable restored prior AspectRecalculation state before applying mode=",
                            config::GameplayModeName(enabledTransition.selectedMode),
                            " aspect=", enabledTransition.restoration.aspect,
                            " boundary=GameplayCameraWriter.");
                    else
                        Log("Gameplay.Enabled disable restoration applied: mode=OFF-cleanup",
                            " restoredAspect=", enabledTransition.restoration.aspect,
                            " flags=0x", std::hex,
                            static_cast<unsigned>(enabledTransition.flags), std::dec,
                            " boundary=GameplayCameraWriter.");
                    break;
                case plugin::GameplayEnabledTransitionEvent::AlreadyNative:
                    Log("Gameplay.Enabled disable restoration complete: result=already_native",
                        " mode=OFF-cleanup aspect=", enabledTransition.currentAspect,
                        " boundary=GameplayCameraWriter.");
                    break;
                case plugin::GameplayEnabledTransitionEvent::AlreadyApplied:
                    Log("Gameplay.Enabled re-enable applied: mode=AspectRecalculation",
                        " result=already_active boundary=GameplayCameraWriter.");
                    break;
                default:
                    break;
                }
            }
            if (applyResult.enabledTransitionHandled) {
                logTransitionPost();
                return;
            }
            if (applyResult.aspectRestorationObserved && applyResult.aspectRestoration.changed)
                TraceGameplayAspectRestorationState(
                    applyResult.aspectRestoration.disposition,
                    applyResult.aspectRestoration.state);

            float selectedFovCandidate = std::numeric_limits<float>::quiet_NaN();
            float primaryCameraFov = std::numeric_limits<float>::quiet_NaN();
            bool selectedFovCandidateReadable = false;
            bool primaryCameraFovReadable = false;
            if (associationDiagnosticsActive && applyResult.sourceReadable) {
                selectedFovCandidateReadable = SafeRead(
                    applyResult.source + 0x234, selectedFovCandidate);
                primaryCameraFovReadable = SafeRead(
                    applyResult.source + 0x230, primaryCameraFov);
            }
            if (applyResult.dialogueContextInvalidated) {
                // The transaction callback emitted the existing event and
                // performed the same reset while holding Dialogue's lock.
            }
            if (applyResult.enableTransitionApplied)
                Log("Gameplay.Enabled re-enable applied: mode=HorPlus",
                    " result=writer_transform_ready aspect=", applyResult.aspect,
                    " outputFov=", applyResult.outputFov,
                    " boundary=GameplayCameraWriter.");
            if (applyResult.automaticAspectRearmed)
                Log("Gameplay camera was rebuilt; re-arming aspect transition.");
            if (applyResult.constrainedAspectPass) {
                if (applyResult.nativeAspectWriteSucceeded)
                    Log("Normalized 21:9 camera aspect while preserving constrained aspect ratio.");
                else
                    Log("Constrained 21:9 aspect normalization refused: native aspect/flags fields were not writable.");
            }
            if (applyResult.nativeAspectWriteAttempted && !applyResult.constrainedAspectPass) {
                if (applyResult.nativeAspectWriteSucceeded)
                    Log("AtomicReplayApplied: phase=GameplayDetected source=0x", std::hex,
                        applyResult.source, std::dec, " fov=", applyResult.inputFov,
                        " aspect=", kNativeAspect, " flags=0x4 previousAspect=",
                        applyResult.aspect, " previousFlags=0x", std::hex,
                        static_cast<unsigned>(applyResult.flags), std::dec, ".");
                else
                    Log("AtomicReplayRefused: phase=GameplayDetected final aspect/flags fields were not writable.");
            }
            if (applyResult.observationPublished) {
                CaptureCinematicBaselineWriterPublication(
                    applyResult.baselineProjection);
                TraceGameplayFovSourceSample(applyResult,
                    selectedFovCandidateReadable, selectedFovCandidate,
                    primaryCameraFovReadable, primaryCameraFov,
                    writerTransaction.coordinatorAfter,
                    g_runtimeGameplayEnabled.load(std::memory_order_acquire),
                    g_runtimeGameplayMode.load(std::memory_order_acquire));
                PublishCameraStateSnapshot(applyResult);
                TraceHorPlusFovState(applyResult);
            }
            if (writerTransaction.mutateFov)
                context.xmm0.f32[0] = writerTransaction.outputFov;
            logTransitionPost();
            TraceHorPlusCinematicWriterAfter(context, horPlusTraceBeforeFov);
            return;
        }
        if (writerTransaction.coordinatorAfter == CoordinatorState::CinematicActive ||
            writerTransaction.coordinatorAfter == CoordinatorState::CinematicExiting)
            return;
#ifdef POST_CINEMATIC_GAMEPLAY_REPLAY_DEFER_TEST
        if (TryDeferredGameplayReplay(context)) return;
#endif
        TraceHorPlusCinematicWriterAfter(context, horPlusTraceBeforeFov);
    }

    constexpr char kZoomTransitionGameSha256[] =
        "61BC1E030740CEBC30CF1DAD0C86CF65E39E12FF0500225821D684181E08D56B";
    constexpr std::size_t kZoomTransitionPostLoadOffset = 13;

    struct ZoomTransitionSnapshot
    {
        bool valid{};
        std::uintptr_t rcx{};
        std::uintptr_t rsi{};
        std::uintptr_t rax{};
        float rax4c{};
        float rax50{};
        float rsi138{};
        float rsi13c{};
        float xmm0{};
        float xmm1{};
        float xmm6{};
    };

    thread_local ZoomTransitionSnapshot g_previousZoomIn{};
    thread_local ZoomTransitionSnapshot g_previousZoomOut{};

    bool ZoomTransitionFloatChanged(float current, float previous)
    {
        return std::isfinite(current) != std::isfinite(previous) ||
            (std::isfinite(current) && std::isfinite(previous) &&
                std::fabs(current - previous) > 0.01f);
    }

    void TraceZoomTransition(SafetyHookContext& context, bool entering)
    {
        MarkCameraStateFreshness({
            camera::CameraStateFreshnessGroup::Zoom,
            camera::CameraStateFreshnessGroup::GameplayFov});
        g_runtime.snapshotZoomObservationValid.store(false, std::memory_order_release);
        if (diagnostics::Enabled()) {
            auto& hitCounter = entering ? g_zoomInHits : g_zoomOutHits;
            hitCounter.fetch_add(1, std::memory_order_relaxed);
        }
        g_snapshotZoomActive.store(entering, std::memory_order_release);
        if (entering) {
            g_snapshotZoomHold.store(true, std::memory_order_release);
            g_neutralGameplayFovSamples = 0;
        }

        const auto rax = static_cast<std::uintptr_t>(context.rax);
        const auto rsi = static_cast<std::uintptr_t>(context.rsi);
        ZoomTransitionSnapshot current{
            true,
            static_cast<std::uintptr_t>(context.rcx),
            rsi,
            rax,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            context.xmm0.f32[0],
            context.xmm1.f32[0],
            context.xmm6.f32[0] };
        if (!rax || !rsi ||
            !RUNTIME_ADAPTER_ZOOM_SAFE_READ(rax + 0x4C, current.rax4c) ||
            !RUNTIME_ADAPTER_ZOOM_SAFE_READ(rax + 0x50, current.rax50) ||
            !RUNTIME_ADAPTER_ZOOM_SAFE_READ(rsi + 0x138, current.rsi138) ||
            !RUNTIME_ADAPTER_ZOOM_SAFE_READ(rsi + 0x13C, current.rsi13c))
            return;
        g_runtime.snapshotZoomObservationValid.store(true, std::memory_order_release);

        auto& previous = entering ? g_previousZoomIn : g_previousZoomOut;
        const bool changed = !previous.valid || previous.rcx != current.rcx ||
            previous.rsi != current.rsi || previous.rax != current.rax ||
            ZoomTransitionFloatChanged(current.rax4c, previous.rax4c) ||
            ZoomTransitionFloatChanged(current.rax50, previous.rax50) ||
            ZoomTransitionFloatChanged(current.rsi138, previous.rsi138) ||
            ZoomTransitionFloatChanged(current.rsi13c, previous.rsi13c) ||
            ZoomTransitionFloatChanged(current.xmm0, previous.xmm0) ||
            ZoomTransitionFloatChanged(current.xmm1, previous.xmm1) ||
            ZoomTransitionFloatChanged(current.xmm6, previous.xmm6);
        if (changed) {
            const auto sequence = g_zoomLoggedEdges.fetch_add(1, std::memory_order_relaxed) + 1;
            g_runtime.snapshotZoomDirection.store(entering ? 1 : 2, std::memory_order_release);
            g_runtime.snapshotZoomSequence.store(sequence, std::memory_order_release);
            g_runtime.snapshotZoomSource.store(rsi, std::memory_order_release);
            g_runtime.snapshotZoomPrimary.store(current.rax4c, std::memory_order_release);
            g_runtime.snapshotZoomSecondary.store(current.rax50, std::memory_order_release);
            if (diagnostics::Enabled()) Log(entering ? "ZOOM_IN" : "ZOOM_OUT",
                " seq=", sequence,
                " thread=", GetCurrentThreadId(),
                " rcx=0x", std::hex, current.rcx,
                " rsi=0x", current.rsi, " rax=0x", current.rax, std::dec,
                " rax4c=", current.rax4c, " rax50=", current.rax50,
                " rsi138=", current.rsi138, " rsi13c=", current.rsi13c,
                " xmm0=", current.xmm0, " xmm1=", current.xmm1,
                " xmm6=", current.xmm6,
                " coordinator=", CoordinatorStateName(g_coordinator.load(std::memory_order_acquire)),
                " cinematicRecovery=", ReplayStateName(g_state.load(std::memory_order_acquire)), ".");
        }
        previous = current;
    }

    void TraceZoomIn(SafetyHookContext& context) { TraceZoomTransition(context, true); }
    void TraceZoomOut(SafetyHookContext& context) { TraceZoomTransition(context, false); }

#ifdef RUNTIME_ADAPTER_ZOOM_SAFE_READ_DEFAULT
#undef RUNTIME_ADAPTER_ZOOM_SAFE_READ_DEFAULT
#undef RUNTIME_ADAPTER_ZOOM_SAFE_READ
#endif

    bool InstallZoomTransitionObservation(const std::string& gameHash)
    {
        if (_stricmp(gameHash.c_str(), kZoomTransitionGameSha256) != 0) {
            if (diagnostics::Enabled()) Log("Zoom transition observation rejected: executable hash mismatch.");
            return false;
        }
        const auto inMatches = hooks::FindAll(g_executable, ZoomIn);
        const auto outMatches = hooks::FindAll(g_executable, ZoomOut);
        if (inMatches.size() != 1 || outMatches.size() != 1) {
            if (diagnostics::Enabled()) Log("Zoom transition observation rejected: inMatches=", inMatches.size(),
                " outMatches=", outMatches.size(), ".");
            return false;
        }
        const auto validatePostLoad = [](std::uint8_t* match) {
            constexpr std::uint8_t expected[] = { 0x0F, 0x2E, 0xC8 };
            return std::memcmp(match + kZoomTransitionPostLoadOffset, expected, sizeof(expected)) == 0;
        };
        if (!validatePostLoad(inMatches.front()) || !validatePostLoad(outMatches.front())) {
            if (diagnostics::Enabled()) Log("Zoom transition observation rejected: post-load instruction contract mismatch.");
            return false;
        }
        g_zoomInHook = safetyhook::create_mid(
            inMatches.front() + kZoomTransitionPostLoadOffset, SafeMidHookEntry<&TraceZoomIn>);
        if (!g_zoomInHook || !g_zoomInHook.enable()) {
            g_zoomInHook.reset();
            if (diagnostics::Enabled()) Log("ZOOM_IN observation hook setup failed.");
            return false;
        }
        g_zoomOutHook = safetyhook::create_mid(
            outMatches.front() + kZoomTransitionPostLoadOffset, SafeMidHookEntry<&TraceZoomOut>);
        if (!g_zoomOutHook || !g_zoomOutHook.enable()) {
            g_zoomOutHook.reset();
            g_zoomInHook.reset();
            if (diagnostics::Enabled()) Log("ZOOM_OUT observation hook setup failed; ZOOM_IN hook rolled back.");
            return false;
        }
        if (diagnostics::Enabled()) Log("Zoom transition observation hooks installed: ZOOM_IN RVA=0x", std::hex,
            reinterpret_cast<std::uintptr_t>(inMatches.front()) + kZoomTransitionPostLoadOffset -
                reinterpret_cast<std::uintptr_t>(g_executable),
            " ZOOM_OUT RVA=0x", reinterpret_cast<std::uintptr_t>(outMatches.front()) +
                kZoomTransitionPostLoadOffset - reinterpret_cast<std::uintptr_t>(g_executable),
            std::dec, ". Read-only edge telemetry enabled.");
        return true;
    }

    bool VerifyExecutableAndInstruction()
    {
        const auto resolution = gameplay::ResolveCameraWriter(g_executable);
        if (!resolution.imageValid) return false;
        if (resolution.matches != 1) {
            Log("Camera-writer signature rejected: matches=", resolution.matches, ".");
            return false;
        }
        if (!resolution.writeAddress) return false;
        g_fovWriteAddress = resolution.writeAddress;
        Log("Camera-writer signature validated at RVA=0x", std::hex,
            reinterpret_cast<std::uintptr_t>(g_fovWriteAddress) -
                reinterpret_cast<std::uintptr_t>(g_executable), std::dec, ".");
        return true;
    }

    HorPlusGameplayApplyResult ApplyHorPlusGameplay(SafetyHookContext& context)
    {
        HorPlusGameplayApplyResult result{};
        result.inputFov = context.xmm0.f32[0];
        result.outputFov = result.inputFov;
        const auto coordinator = g_coordinator.load(std::memory_order_acquire);
        if (coordinator != CoordinatorState::Gameplay) {
            result.reason = "COORDINATOR_GUARD";
            return result;
        }

        const auto source = static_cast<std::uintptr_t>(context.rsi);
        result.source = source;
        float aspect = 0.0f;
        std::uint8_t flags = 0;
        if (!source || !SafeRead(source + kAspectOffset, aspect) ||
            !SafeRead(source + kFlagsOffset, flags)) {
            result.reason = "SOURCE_READ_FAILED";
            return result;
        }
        result.sourceReadable = true;
        result.aspect = aspect;
        result.flags = flags;
        float selectedFovCandidate = std::numeric_limits<float>::quiet_NaN();
        bool selectedFovCandidateReadable = false;
        float primaryCameraFov = std::numeric_limits<float>::quiet_NaN();
        bool primaryCameraFovReadable = false;
        if (diagnostics::Enabled()) {
            selectedFovCandidateReadable = SafeRead(source + 0x234, selectedFovCandidate);
            primaryCameraFovReadable = SafeRead(source + 0x230, primaryCameraFov);
        }

        const auto previousSource = g_lastGameplayCameraSource.exchange(
            source, std::memory_order_acq_rel);
        const auto previousFov = g_lastGameplayCameraFov.exchange(
            result.inputFov, std::memory_order_acq_rel);
        const auto contextChange = gameplay::EvaluateGameplayContextChange(
            previousSource, previousFov, source, result.inputFov, kDialogueContextFovJump);
        if (contextChange.InvalidatesDialogue()) {
            diagnostics::ScopedRuntimeMutex dialogueLock(g_dialogueMutex, diagnostics::RuntimeLockPath::Dialogue);
            if (g_dialoguePhase != DialoguePhase::Inactive) {
                Log("Dialogue runtime invalidated by gameplay camera context change: sourceChanged=",
                    contextChange.sourceChanged, " previousSource=0x", std::hex, previousSource,
                    " source=0x", source, std::dec, " previousFOV=", previousFov,
                    " currentFOV=", result.inputFov, ". Native pass-through until a new descent.");
                ResetDialogueRuntimeState();
            }
        }

        // Gameplay.Mode=HorPlus returns through this path without reaching
        // ReplayManualTransitionOriginal. Retain the latest valid native
        // Gameplay observation here so a later cinematic ENTER can consume
        // the established baseline instead of falling back to NativeHorPlus.
        if (coordinator == CoordinatorState::Gameplay &&
            IsValidFovValue(result.inputFov) && IsValidAspect(aspect)) {
            UpdateGameplayAspectRestorationState(aspect, source);
            // A HorPlus request can arrive while the native aspect transition
            // is still settling. The observation above is the deterministic
            // establishment boundary: retry once with the newly published
            // coherent aspect instead of waiting for a later writer sample.
            if (g_gameplayModeTransitionPending.load(std::memory_order_acquire)) {
                ApplyPendingGameplayModeTransition(source);
                SafeRead(source + kAspectOffset, aspect);
                SafeRead(source + kFlagsOffset, flags);
            }
        }

        const auto policy = plugin::ResolveGameplayWriterPolicy(
            g_runtimeGameplayEnabled.load(std::memory_order_acquire), coordinator,
            g_runtimeGameplayMode.load(std::memory_order_acquire),
            result.inputFov, aspect, flags, kNativeAspect);
        const auto& transform = policy.transform;
        result.eligible = transform.eligible;
        result.applied = transform.applied;
        result.gameplayStateEstablished = coordinator == CoordinatorState::Gameplay;
        result.outputFov = transform.outputFov;
        result.reason = result.applied ? "GAMEPLAY_FOV_CHANGED" :
            (result.eligible ? "TRANSFORM_RESULT_REJECTED" : "INPUT_BYPASS");
        if (result.applied) context.xmm0.f32[0] = result.outputFov;

        if (IsValidFovValue(result.inputFov) && IsValidFovValue(result.outputFov) &&
            IsValidAspect(result.aspect)) {
            camera::CameraFovObservation observation{};
            observation.boundary = camera::FovObservationBoundary::CameraWriter;
            observation.inputFov = {
                result.inputFov,
                camera::FovSpace::Native,
                camera::FovProvenance::NativeRegisterInput,
                true };
            observation.resultFov = {
                result.outputFov,
                result.applied ? camera::FovSpace::Transformed : camera::FovSpace::Native,
                result.applied ? camera::FovProvenance::ModTransformResult :
                    camera::FovProvenance::PassThroughResult,
                true };
            observation.aspect = { result.aspect, camera::FovProvenance::ResolvedAspect, true };
            observation.writerSource = { result.source, result.source != 0 };
            observation.writerFlags = result.flags;
            observation.writerFlagsValid = true;
            observation.observedDuringGameplay =
                coordinator == CoordinatorState::Gameplay;
            result.observation = g_fovObservationStore.Publish(observation);
            result.observationPublished = true;
            result.baselineProjection = g_gameplayBaselineStore.Project(
                result.observation, coordinator == CoordinatorState::Gameplay);
            CaptureCinematicBaselineWriterPublication(result.baselineProjection);
        }
        TraceGameplayFovSourceSample(result,
            selectedFovCandidateReadable, selectedFovCandidate,
            primaryCameraFovReadable, primaryCameraFov, coordinator,
            g_runtimeGameplayEnabled.load(std::memory_order_acquire),
            g_runtimeGameplayMode.load(std::memory_order_acquire));
        return result;
    }

#ifdef POST_EXIT_GAMEPLAY_OBSERVER_DIAGNOSTIC
#ifdef HORPLUS_GAMEPLAY_DIAGNOSTIC
    struct HorPlusDiagnosticSnapshot
    {
        bool valid{};
        std::uintptr_t source{};
        float inputFov{};
        float transformedFov{};
        float aspect{};
        std::uint8_t flags{};
        bool eligible{};
        bool applied{};
    };

    thread_local HorPlusDiagnosticSnapshot g_previousHorPlusDiagnostic{};

    void ApplyHorPlusGameplayDiagnostic(SafetyHookContext& context)
    {
        g_horPlusDiagnosticWriterCallbacks.fetch_add(1, std::memory_order_relaxed);
        const auto source = static_cast<std::uintptr_t>(context.rsi);
        float aspect = 0.0f;
        std::uint8_t flags = 0;
        if (!source || !SafeRead(source + kAspectOffset, aspect) ||
            !SafeRead(source + kFlagsOffset, flags) ||
            !std::isfinite(aspect))
            return;

        const float inputFov = context.xmm0.f32[0];
        const auto floatChanged = [](float current, float previous) {
            return std::isfinite(current) != std::isfinite(previous) ||
                (std::isfinite(current) && std::isfinite(previous) &&
                    std::fabs(current - previous) > 0.01f);
        };
        const bool inputFovValid = std::isfinite(inputFov) && inputFov > 1.0f && inputFov < 179.0f;
        const bool isUltrawide = gameplay::IsUltrawideAspect(aspect, kNativeAspect);
        const bool validHorPlusFlags = flags == 0x4 || flags == 0x5;
        const bool eligible =
            g_coordinator.load(std::memory_order_acquire) == CoordinatorState::Gameplay &&
            isUltrawide && validHorPlusFlags && inputFovValid;
        float transformedFov = std::numeric_limits<float>::quiet_NaN();
        bool applied = false;
        if (eligible) {
            transformedFov = camera::HorPlus(inputFov, aspect, kNativeAspect);
            if (std::isfinite(transformedFov) && transformedFov > 1.0f && transformedFov < 179.0f) {
                context.xmm0.f32[0] = transformedFov;
                g_horPlusDiagnosticApplications.fetch_add(1, std::memory_order_relaxed);
                applied = true;
            }
        }

        const bool changed = !g_previousHorPlusDiagnostic.valid ||
            g_previousHorPlusDiagnostic.source != source ||
            floatChanged(inputFov, g_previousHorPlusDiagnostic.inputFov) ||
            floatChanged(transformedFov, g_previousHorPlusDiagnostic.transformedFov) ||
            floatChanged(aspect, g_previousHorPlusDiagnostic.aspect) ||
            flags != g_previousHorPlusDiagnostic.flags ||
            eligible != g_previousHorPlusDiagnostic.eligible ||
            applied != g_previousHorPlusDiagnostic.applied;
        if (changed) {
            const auto changes = g_horPlusDiagnosticChanges.fetch_add(
                1, std::memory_order_relaxed) + 1;
            Log("HORPLUS_DIAG change: source=0x", std::hex, source, std::dec,
                " inputFov=", inputFov, " transformedFov=", transformedFov,
                " aspect=", aspect, " flags=0x", std::hex, static_cast<int>(flags),
                std::dec, " eligible=", eligible ? "YES" : "NO",
                " applied=", applied ? "YES" : "NO",
                std::dec, " callbacks=", g_horPlusDiagnosticWriterCallbacks.load(std::memory_order_relaxed),
                " applications=", g_horPlusDiagnosticApplications.load(std::memory_order_relaxed),
                " changes=", changes, ".");
        }
        g_previousHorPlusDiagnostic = HorPlusDiagnosticSnapshot{
            true, source, inputFov, transformedFov, aspect, flags, eligible, applied };
    }
#endif

    void ObservePostExitDiagnosticWriter(SafetyHookContext& context)
    {
        // Diagnostic-only observer. It never calls the production
        // replay/correction path or writes aspect/flags. The optional HorPlus
        // POC changes only the diagnostic hook's XMM0 input.
#if defined(POST_EXIT_RAW_TRACE_DIAGNOSTIC) || defined(POST_EXIT_INTERPOLATED_FOV_PRODUCER_TRACE)
        ObservePostExitRawWriterEntry(context);
#endif
#ifdef HORPLUS_GAMEPLAY_DIAGNOSTIC
        ApplyHorPlusGameplayDiagnostic(context);
#endif
    }

    bool InstallPostExitGameplayObserverDiagnostic()
    {
        if (!VerifyExecutableAndInstruction()) {
            Log("Post-EXIT diagnostic gameplay observer refused: validated writer resolution failed.");
            return false;
        }
        g_postExitGameplayObserverHook = safetyhook::create_mid(
            g_fovWriteAddress, ObservePostExitDiagnosticWriter);
        if (!g_postExitGameplayObserverHook) {
            Log("Post-EXIT diagnostic gameplay observer refused: hook creation failed.");
            g_fovWriteAddress = nullptr;
            return false;
        }
        Log("Post-EXIT diagnostic gameplay observer installed: read-only; production Gameplay remains disabled.");
        return true;
    }
#endif

#ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC
    struct DialogueDiscoverySnapshot
    {
        bool valid{};
        std::uintptr_t context{};
        std::uintptr_t receiver{};
        std::uintptr_t receiverVtable{};
        std::uintptr_t rdx{};
        float xmm6{};
        float xmm1{};
        float fov28{};
        float fov2c{};
        float fov30{};
        std::array<std::uint32_t, 33> words{};
        std::array<bool, 33> readable{};
        CoordinatorState coordinator{CoordinatorState::Gameplay};
        ReplayState replayState{ReplayState::WaitingForAutomaticUpdate};
        DialoguePhase dialoguePhase{DialoguePhase::Inactive};
        DialogueZoomPolicy policy{DialogueZoomPolicy::Native};
        DialogueZoomPolicy activePolicy{DialogueZoomPolicy::Native};
        bool activePolicyValid{};
    };

    thread_local DialogueDiscoverySnapshot g_previousDialogueDiscovery{};
    thread_local std::uint64_t g_dialogueDiscoverySeenResetGeneration{};

    std::int64_t DialogueDiscoveryNowNs()
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    void LogDialogueDiscoveryEnd(const char* reason)
    {
        const bool wasActive = g_dialogueDiscoveryActive.exchange(false, std::memory_order_acq_rel);
        if (!wasActive) return;
        const auto startNs = g_dialogueDiscoveryStartNs.load(std::memory_order_acquire);
        const auto elapsedMs = startNs > 0 ? (DialogueDiscoveryNowNs() - startNs) / 1'000'000 : -1;
        Log("DIALOG_DISCOVERY END reason=", reason, " durationMs=", elapsedMs,
            " samples=", g_dialogueDiscoverySamples.load(std::memory_order_relaxed),
            " changes=", g_dialogueDiscoveryChanges.load(std::memory_order_relaxed), ".");
        g_runtime.dialogueDiscoveryResetGeneration.fetch_add(1, std::memory_order_acq_rel);
    }

    void LogDialogueDiscoveryChanges(const DialogueDiscoverySnapshot& current,
        const DialogueDiscoverySnapshot& previous, std::int64_t elapsedUs)
    {
        std::ostringstream changes;
        bool hasChange = false;
        const auto add = [&changes, &hasChange](const char* name) {
            if (hasChange) changes << '|';
            changes << name;
            hasChange = true;
        };
        const auto floatChanged = [](float a, float b) {
            return std::isfinite(a) != std::isfinite(b) ||
                (std::isfinite(a) && std::isfinite(b) && std::fabs(a - b) > 0.01f);
        };
        if (current.context != previous.context) add("context");
        if (current.receiver != previous.receiver) add("receiver");
        if (current.receiverVtable != previous.receiverVtable) add("vtable");
        if (current.rdx != previous.rdx) add("rdx");
        if (floatChanged(current.xmm6, previous.xmm6)) add("xmm6");
        if (floatChanged(current.xmm1, previous.xmm1)) add("xmm1");
        if (floatChanged(current.fov28, previous.fov28)) add("fov28");
        if (floatChanged(current.fov2c, previous.fov2c)) add("fov2c");
        if (floatChanged(current.fov30, previous.fov30)) add("fov30");
        if (current.coordinator != previous.coordinator) add("coordinator");
        if (current.replayState != previous.replayState) add("replay");
        if (current.dialoguePhase != previous.dialoguePhase) add("dialoguePhase");
        if (current.policy != previous.policy) add("policy");
        if (current.activePolicy != previous.activePolicy ||
            current.activePolicyValid != previous.activePolicyValid) add("activePolicy");
        for (std::size_t i = 0; i < current.words.size(); ++i) {
            if (current.readable[i] && previous.readable[i] && current.words[i] != previous.words[i]) {
                add("memory");
                break;
            }
        }
        if (!hasChange) return;

        Log("DIALOG_DISCOVERY SAMPLE elapsedUs=", elapsedUs,
            " changed=", changes.str(), " context=0x", std::hex, current.context,
            " receiver=0x", current.receiver, " vtable=0x", current.receiverVtable,
            " rdx=0x", current.rdx, std::dec, " xmm6=", current.xmm6,
            " xmm1=", current.xmm1, " fov28=", current.fov28, " fov2c=", current.fov2c,
            " fov30=", current.fov30, " coordinator=", CoordinatorStateName(current.coordinator),
            " replay=", ReplayStateName(current.replayState),
            " dialoguePhase=", DialoguePhaseName(current.dialoguePhase),
            " selectedPolicy=", DialogueZoomPolicyName(current.policy),
            " activePolicy=", DialogueZoomPolicyName(current.activePolicy),
            " activePolicyValid=", current.activePolicyValid, ".");
        for (std::size_t i = 0; i < current.words.size(); ++i) {
            if (current.readable[i] && previous.readable[i] && current.words[i] != previous.words[i])
                Log("DIALOG_DISCOVERY MEM elapsedUs=", elapsedUs, " context=0x", std::hex,
                    current.context, " offset=0x", i * sizeof(std::uint32_t), " old=0x",
                    previous.words[i], " new=0x", current.words[i], std::dec, ".");
        }
    }

    void TraceDialogueDiscovery(SafetyHookContext& context)
    {
        if (!g_dialogueDiscoveryActive.load(std::memory_order_acquire)) return;
        const auto resetGeneration = g_runtime.dialogueDiscoveryResetGeneration.load(
            std::memory_order_acquire);
        if (resetGeneration != g_dialogueDiscoverySeenResetGeneration) {
            g_previousDialogueDiscovery = {};
            g_dialogueDiscoverySeenResetGeneration = resetGeneration;
        }
        const auto startNs = g_dialogueDiscoveryStartNs.load(std::memory_order_acquire);
        const auto nowNs = DialogueDiscoveryNowNs();
        if (startNs <= 0 || nowNs - startNs >= kDialogueDiscoveryDurationNs) return;

        const auto source = static_cast<std::uintptr_t>(context.rsi);
        if (!source || source > (std::numeric_limits<std::uintptr_t>::max)() - 0x80) return;
        DialogueDiscoverySnapshot current{};
        current.valid = true;
        current.context = source;
        current.receiver = static_cast<std::uintptr_t>(context.rcx);
        current.rdx = static_cast<std::uintptr_t>(context.rdx);
        SafeRead(current.receiver, current.receiverVtable);
        current.xmm6 = context.xmm6.f32[0];
        current.xmm1 = context.xmm1.f32[0];
        SafeRead(source + 0x28, current.fov28);
        SafeRead(source + 0x2C, current.fov2c);
        SafeRead(source + 0x30, current.fov30);
        for (std::size_t i = 0; i < current.words.size(); ++i)
            current.readable[i] = SafeRead(source + i * sizeof(std::uint32_t), current.words[i]);
        current.coordinator = g_coordinator.load(std::memory_order_acquire);
        current.replayState = g_state.load(std::memory_order_acquire);
        current.policy = g_runtimeDialoguePolicy.load(std::memory_order_acquire);
        {
            diagnostics::ScopedRuntimeMutex dialogueLock(g_dialogueMutex, diagnostics::RuntimeLockPath::Dialogue);
            current.dialoguePhase = g_dialoguePhase;
            current.activePolicy = g_activeDialoguePolicy.Value();
            current.activePolicyValid = g_activeDialoguePolicy.IsValid();
        }
        g_dialogueDiscoverySamples.fetch_add(1, std::memory_order_relaxed);
        if (!g_previousDialogueDiscovery.valid || g_previousDialogueDiscovery.context != current.context) {
            Log("DIALOG_DISCOVERY CONTEXT elapsedUs=", (nowNs - startNs) / 1000,
                " old=0x", std::hex, g_previousDialogueDiscovery.context,
                " new=0x", current.context, std::dec, ". Baseline reset.");
            g_previousDialogueDiscovery = current;
            return;
        }
        const auto previousChanges = g_dialogueDiscoveryChanges.load(std::memory_order_relaxed);
        LogDialogueDiscoveryChanges(current, g_previousDialogueDiscovery, (nowNs - startNs) / 1000);
        if (g_dialogueDiscoveryChanges.load(std::memory_order_relaxed) == previousChanges) {
            bool changed = false;
            for (std::size_t i = 0; i < current.words.size(); ++i)
                changed = changed || (current.readable[i] && g_previousDialogueDiscovery.readable[i] &&
                    current.words[i] != g_previousDialogueDiscovery.words[i]);
            changed = changed || current.context != g_previousDialogueDiscovery.context ||
                current.receiver != g_previousDialogueDiscovery.receiver ||
                current.receiverVtable != g_previousDialogueDiscovery.receiverVtable ||
                current.rdx != g_previousDialogueDiscovery.rdx ||
                current.coordinator != g_previousDialogueDiscovery.coordinator ||
                current.replayState != g_previousDialogueDiscovery.replayState ||
                current.dialoguePhase != g_previousDialogueDiscovery.dialoguePhase ||
                current.policy != g_previousDialogueDiscovery.policy ||
                std::fabs(current.xmm6 - g_previousDialogueDiscovery.xmm6) > 0.01f ||
                std::fabs(current.xmm1 - g_previousDialogueDiscovery.xmm1) > 0.01f ||
                std::fabs(current.fov28 - g_previousDialogueDiscovery.fov28) > 0.01f ||
                std::fabs(current.fov2c - g_previousDialogueDiscovery.fov2c) > 0.01f ||
                std::fabs(current.fov30 - g_previousDialogueDiscovery.fov30) > 0.01f;
            if (changed) g_dialogueDiscoveryChanges.fetch_add(1, std::memory_order_relaxed);
        }
        g_previousDialogueDiscovery = current;
    }

    DWORD WINAPI DialogueDiscoveryLoop(void*)
    {
        bool previousF12 = false;
        while (!WaitForWorkerStop(25)) {
            const bool f12 = (GetAsyncKeyState(VK_F12) & 0x8000) != 0;
            if (f12 && !previousF12 && !g_dialogueDiscoveryActive.exchange(true, std::memory_order_acq_rel)) {
                g_dialogueDiscoverySamples.store(0, std::memory_order_release);
                g_dialogueDiscoveryChanges.store(0, std::memory_order_release);
                g_dialogueDiscoveryStartNs.store(DialogueDiscoveryNowNs(), std::memory_order_release);
                g_previousDialogueDiscovery = {};
                Log("DIALOG_DISCOVERY START durationMs=10000 trigger=F12 region=0x00..0x80 granularity=4.");
            }
            previousF12 = f12;
            if (g_dialogueDiscoveryActive.load(std::memory_order_acquire)) {
                const auto startNs = g_dialogueDiscoveryStartNs.load(std::memory_order_acquire);
                if (startNs > 0 && DialogueDiscoveryNowNs() - startNs >= kDialogueDiscoveryDurationNs)
                    LogDialogueDiscoveryEnd("timeout");
            }
        }
        LogDialogueDiscoveryEnd("worker-stop");
        return 0;
    }
#endif

    void SelectGameplayMode(config::GameplayMode newMode)
    {
        const auto oldMode = g_runtimeGameplayMode.load(std::memory_order_acquire);
        const auto coordinator = g_coordinator.load(std::memory_order_acquire);
        const auto plan = gameplay::ResolveGameplayModeTransition(oldMode, newMode, coordinator);
        if (!plan.changed) {
            Log("Gameplay mode selection ignored: mode=", config::GameplayModeName(newMode),
                " reason=no-op.");
            return;
        }

        if (!g_runtimeGameplayEnabled.load(std::memory_order_acquire)) {
            g_gameplayModeTransitionPending.store(false, std::memory_order_release);
            g_gameplayModeTransitionDeferralLogged.store(false, std::memory_order_release);
            g_runtimeGameplayMode.store(newMode, std::memory_order_release);
            Log("Gameplay mode selected while disabled: ", config::GameplayModeName(oldMode),
                " -> ", config::GameplayModeName(newMode),
                "; physical application deferred until Gameplay.Enabled=true.");
            return;
        }

        if (oldMode == config::GameplayMode::AspectRecalculation &&
            newMode == config::GameplayMode::HorPlus) {
            g_state.store(ReplayState::WaitingForAutomaticUpdate, std::memory_order_release);
            g_lastAutoRestoreSource.store(0, std::memory_order_release);
            g_gameplayModeTransitionTarget.store(newMode, std::memory_order_release);
            g_gameplayModeTransitionDeferralLogged.store(false, std::memory_order_release);
            g_gameplayModeTransitionPending.store(true, std::memory_order_release);
        } else if (newMode == config::GameplayMode::AspectRecalculation) {
            g_gameplayModeTransitionPending.store(false, std::memory_order_release);
            g_gameplayModeTransitionDeferralLogged.store(false, std::memory_order_release);
            g_state.store(ReplayState::WaitingForAutomaticUpdate, std::memory_order_release);
            g_lastAutoRestoreSource.store(0, std::memory_order_release);
            InvalidateGameplayAspectRestorationState();
            g_lastGameplayCameraSource.store(0, std::memory_order_release);
            g_lastGameplayCameraFov.store(std::numeric_limits<float>::quiet_NaN(),
                std::memory_order_release);
            g_gameplayBaselineStore.Invalidate();
            {
                if (diagnostics::Enabled()) {
                    std::lock_guard telemetryLock(g_horPlusFovTelemetryMutex);
                    g_runtime.horPlusFovTelemetryValid = false;
                    g_runtime.horPlusGameplayCacheValid = false;
                }
            }
        } else if (newMode == config::GameplayMode::RuntimeCombine) {
            // RuntimeCombine owns a separate, confirm-before-HorPlus transaction.
            // Do not leave an earlier HorPlus physical-transition retry armed.
            g_gameplayModeTransitionPending.store(false, std::memory_order_release);
            g_gameplayModeTransitionDeferralLogged.store(false,
                std::memory_order_release);
        }

        g_runtimeGameplayMode.store(newMode, std::memory_order_release);
        Log("Gameplay mode transition ", config::GameplayModeName(oldMode), " -> ",
            config::GameplayModeName(newMode), " coordinator=", CoordinatorStateName(coordinator),
            " physicalBoundary=", plan.deferPhysicalTransition ? "deferred" : "Gameplay", ".");
    }

    void ApplyGameplayModeSetting(void*, config::GameplayMode mode)
    {
        SelectGameplayMode(mode);
    }

    plugin::RuntimeMutationResult ApplyRuntimeSetting(
        const plugin::RuntimeSettingMutation& mutation, void*)
    {
        auto admission = g_nativeCallbackAdmission.TryEnter();
        if (!admission || g_stopping.load(std::memory_order_acquire)) return {};
        const auto enabledTransaction = plugin::ApplyCameraSetting(
            g_cameraCorrectionGate,
            plugin::CameraSettingStateView{
                g_runtimeGameplayMode, g_runtimeGameplayEnabled, g_gameplaySettingsEpoch,
                g_gameplayEnableApplyPending, g_gameplayDisableRestorePending,
                g_gameplayModeTransitionPending, g_gameplayEnabledTransitionDeferralLogged,
                g_runtimeCinematicsEnabled, g_cinematicsSettingsEpoch,
                g_activeCinematicsEnabled, g_activeCinematicSettingsEpoch,
                g_cinematicFovApplied, g_cinematicTransformedFov,
                g_exitCinematicTransformedFov, g_cinematicContinuationPending,
                g_cinematicContinuationCarryPending,
                g_cinematicContinuationInterpolationActive,
                g_cinematicContinuationSource, g_runtimeDialogueEnabled,
                g_runtimeDialoguePolicy,
                g_dialogueSettingsEpoch, g_activeDialogueSettingsEpoch,
                g_dialogueNonNativeCapabilityAvailable.load(std::memory_order_acquire),
                g_runtimeCinematicPolicy, g_runtimeCinematicFovMode,
                g_dialogueCorrectionActivation, nullptr, &ApplyGameplayModeSetting,
                nullptr, &MarkCameraStateForSetting
            }, mutation,
            g_gameplayRecoveryObservationAvailable.load(std::memory_order_acquire),
            [&] { FlushCinematicContinuationTrace("cinematics_disabled"); });
        if (enabledTransaction.handled) {
            if (!enabledTransaction.mutation.accepted &&
                mutation.kind == plugin::RuntimeSettingKind::GameplayEnabled) {
                Log("Gameplay.Enabled transition rejected: reason=gameplay_hook_unavailable.");
            } else if (enabledTransaction.mutation.changed) {
                if (mutation.kind == plugin::RuntimeSettingKind::GameplayEnabled) {
                    const auto* value = std::get_if<bool>(&mutation.value);
                    Log("Gameplay.Enabled transition requested: ", value && *value
                        ? "false -> true" : "true -> false", " mode=",
                        config::GameplayModeName(g_runtimeGameplayMode.load(
                            std::memory_order_acquire)), value && *value
                        ? " applyBoundary=GameplayCameraWriter."
                        : " restoreBoundary=GameplayCameraWriter.");
                } else if (mutation.kind == plugin::RuntimeSettingKind::CinematicsEnabled) {
                    const auto* value = std::get_if<bool>(&mutation.value);
                    Log(value && *value
                        ? "Cinematics.Enabled transition: false -> true; correction will activate at the next real cinematic ENTER."
                        : "Cinematics.Enabled transition: true -> false; lifecycle observation retained, camera corrections disabled immediately.");
                }
            }
            return enabledTransaction.mutation;
        }
        auto correctionLock = g_cameraCorrectionGate.Acquire();
        if (g_stopping.load(std::memory_order_acquire)) return {};
        switch (mutation.kind) {
        case plugin::RuntimeSettingKind::OverlayLocaleCode: {
            const auto* value = std::get_if<std::string>(&mutation.value);
            const auto* descriptor = value
                ? localization::FindLocaleDescriptor(*value) : nullptr;
            if (!descriptor) return {};
            const auto* old = g_runtimeOverlayLocale.exchange(descriptor,
                std::memory_order_acq_rel);
            g_runtimeOverlayLocaleAuto.store(false, std::memory_order_release);
            g_runtimeOverlayAutoLocaleSynchronized.store(false, std::memory_order_release);
            const int oldFontSize = g_runtimeOverlayFontSize.exchange(
                descriptor->initialFontSize, std::memory_order_acq_rel);
            return {true, old != descriptor ||
                oldFontSize != descriptor->initialFontSize};
        }
        case plugin::RuntimeSettingKind::OverlayAutoLocale: {
            const auto* value = std::get_if<bool>(&mutation.value);
            if (!value) return {};
            const bool old = g_runtimeOverlayLocaleAuto.exchange(*value,
                std::memory_order_acq_rel);
            if (*value)
                g_runtimeOverlayAutoLocaleSynchronized.store(false, std::memory_order_release);
            return {true, old != *value};
        }
        case plugin::RuntimeSettingKind::OverlayFontSize: {
            const auto* value = std::get_if<int>(&mutation.value);
            if (!value || *value < config::OverlayFontSizeMin ||
                *value > config::OverlayFontSizeMax) return {};
            const int old = g_runtimeOverlayFontSize.exchange(*value,
                std::memory_order_acq_rel);
            return {true, old != *value};
        }
        case plugin::RuntimeSettingKind::HotkeysEnabled: {
            const auto* value = std::get_if<bool>(&mutation.value);
            if (!value) return {};
            std::lock_guard hotkeyLock(g_hotkeyConfigMutex);
            const bool changed = g_config.hotkeysEnabled != *value;
            g_config.hotkeysEnabled = *value;
            return {true, changed};
        }
        case plugin::RuntimeSettingKind::GameplayCycleKey:
        case plugin::RuntimeSettingKind::CinematicCycleKey:
        case plugin::RuntimeSettingKind::CinematicFovCycleKey:
        case plugin::RuntimeSettingKind::DialogueCycleKey:
        case plugin::RuntimeSettingKind::OverlayToggleKey: {
            const auto* value = std::get_if<int>(&mutation.value);
            if (!value || !config::IsSupportedHotkey(*value))
                return {};
            std::lock_guard hotkeyLock(g_hotkeyConfigMutex);
            config::HotkeyBindingId binding{};
            if (!plugin::detail::RuntimeHotkeyBinding(mutation.kind, binding) ||
                config::HasHotkeyConflict(g_config, *value, binding)) return {};
            const bool changed = config::HotkeyBindingValue(g_config, binding) != *value;
            config::SetHotkeyBindingValue(g_config, binding, *value);
            return {true, changed};
        }
        }
        return {};
    }

    bool ReadRuntimeSettings(plugin::RuntimeSettingsSnapshot& snapshot, void*)
    {
        auto admission = g_nativeCallbackAdmission.TryEnter();
        if (!admission || g_stopping.load(std::memory_order_acquire)) return false;
        auto correctionLock = g_cameraCorrectionGate.Acquire();
        if (g_stopping.load(std::memory_order_acquire)) return false;
        snapshot.gameplayEnabled = g_runtimeGameplayEnabled.load(std::memory_order_acquire);
        snapshot.gameplayMode = g_runtimeGameplayMode.load(std::memory_order_acquire);
        snapshot.cinematicsEnabled = g_runtimeCinematicsEnabled.load(std::memory_order_acquire);
        snapshot.cinematicAspectPolicy = g_runtimeCinematicPolicy.load(std::memory_order_acquire);
        snapshot.cinematicFovMode = g_runtimeCinematicFovMode.load(std::memory_order_acquire);
        snapshot.dialogueEnabled = g_runtimeDialogueEnabled.load(std::memory_order_acquire);
        snapshot.dialogueZoomPolicy = g_runtimeDialoguePolicy.load(std::memory_order_acquire);
        const auto* locale = g_runtimeOverlayLocale.load(std::memory_order_acquire);
        snapshot.overlayLocaleCode = locale
            ? std::string(locale->code) : std::string(localization::CanonicalLocaleCode);
        snapshot.overlayLocaleAuto = g_runtimeOverlayLocaleAuto.load(std::memory_order_acquire);
        snapshot.overlayLocaleAutoSynchronized =
            g_runtimeOverlayAutoLocaleSynchronized.load(std::memory_order_acquire);
        snapshot.overlayFontSize = g_runtimeOverlayFontSize.load(std::memory_order_acquire);
        {
            std::lock_guard hotkeyLock(g_hotkeyConfigMutex);
            snapshot.hotkeysEnabled = g_config.hotkeysEnabled;
            snapshot.overlayToggleKey = g_config.overlayToggleKey;
            snapshot.gameplayCycleKey = g_config.gameplayCycleKey;
            snapshot.cinematicCycleKey = g_config.cinematicCycleKey;
            snapshot.cinematicFovCycleKey = g_config.cinematicFovCycleKey;
            snapshot.dialogueCycleKey = g_config.dialogueCycleKey;
        }
        return true;
    }

    bool ReadOverlaySemanticSnapshot(plugin::OverlaySemanticSnapshot& snapshot, void*)
    {
        diagnostics::RecordRuntimeEvent(diagnostics::RuntimeEvent::SemanticSnapshotRead);
        auto admission = g_nativeCallbackAdmission.TryEnter();
        if (!admission) return false;
        if (!g_runtimeInitializationComplete.load(std::memory_order_acquire) ||
            g_stopping.load(std::memory_order_acquire))
            return false;

        auto correctionLock = g_cameraCorrectionGate.Acquire();
        if (!g_runtimeInitializationComplete.load(std::memory_order_acquire) ||
            g_stopping.load(std::memory_order_acquire)) return false;

        snapshot = {};
        const auto setCurrentFact = [](auto& fact, auto value,
            plugin::SemanticProvenance provenance) {
            fact.value = value;
            fact.valid = true;
            fact.provenance = provenance;
            fact.freshness = plugin::SemanticFreshness::AtRead;
        };

        setCurrentFact(snapshot.gameplayEnabled,
            g_runtimeGameplayEnabled.load(std::memory_order_acquire),
            plugin::SemanticProvenance::RuntimeSettings);
        setCurrentFact(snapshot.gameplayMode,
            g_runtimeGameplayMode.load(std::memory_order_acquire),
            plugin::SemanticProvenance::RuntimeSettings);
        setCurrentFact(snapshot.cinematicsEnabled,
            g_runtimeCinematicsEnabled.load(std::memory_order_acquire),
            plugin::SemanticProvenance::RuntimeSettings);
        setCurrentFact(snapshot.cinematicAspectPolicy,
            g_runtimeCinematicPolicy.load(std::memory_order_acquire),
            plugin::SemanticProvenance::RuntimeSettings);
        setCurrentFact(snapshot.cinematicFovMode,
            g_runtimeCinematicFovMode.load(std::memory_order_acquire),
            plugin::SemanticProvenance::RuntimeSettings);
        setCurrentFact(snapshot.dialogueZoomPolicy,
            g_runtimeDialoguePolicy.load(std::memory_order_acquire),
            plugin::SemanticProvenance::RuntimeSettings);
        setCurrentFact(snapshot.dialogueEnabled,
            g_runtimeDialogueEnabled.load(std::memory_order_acquire),
            plugin::SemanticProvenance::RuntimeSettings);

        setCurrentFact(snapshot.gameplayHookAvailable,
            g_cameraWriterHookGate.load(std::memory_order_acquire),
            plugin::SemanticProvenance::GameplayHookGate);
        setCurrentFact(snapshot.gameplayEnableApplyPending,
            g_gameplayEnableApplyPending.load(std::memory_order_acquire),
            plugin::SemanticProvenance::GameplayTransition);
        setCurrentFact(snapshot.gameplayDisableRestorePending,
            g_gameplayDisableRestorePending.load(std::memory_order_acquire),
            plugin::SemanticProvenance::GameplayTransition);
        setCurrentFact(snapshot.gameplayModeTransitionPending,
            g_gameplayModeTransitionPending.load(std::memory_order_acquire),
            plugin::SemanticProvenance::GameplayTransition);
        setCurrentFact(snapshot.cinematicAspectComponentAvailable,
            g_cinematicAspectComponentAvailable.load(std::memory_order_acquire),
            plugin::SemanticProvenance::CinematicHookState);
        setCurrentFact(snapshot.cinematicFovLifecycleAvailable,
            g_cinematicLifecycleObservationAvailable.load(std::memory_order_acquire),
            plugin::SemanticProvenance::CinematicHookState);

        const bool cinematicSelectionActive =
            g_cinematicSelectionValid.load(std::memory_order_acquire);
        setCurrentFact(snapshot.cinematicSelectionActive, cinematicSelectionActive,
            plugin::SemanticProvenance::CinematicSelection);
        if (cinematicSelectionActive) {
            auto& activeEnabled = snapshot.activeCinematicsEnabled;
            activeEnabled.value = g_activeCinematicsEnabled.load(std::memory_order_acquire);
            activeEnabled.valid = true;
            activeEnabled.provenance = plugin::SemanticProvenance::CinematicSelection;
            activeEnabled.freshness = plugin::SemanticFreshness::LifecycleScoped;

            auto& activeAspect = snapshot.activeCinematicAspectPolicy;
            activeAspect.value = g_activeCinematicAspectPolicy.load(std::memory_order_acquire);
            activeAspect.valid = true;
            activeAspect.provenance = plugin::SemanticProvenance::CinematicSelection;
            activeAspect.freshness = plugin::SemanticFreshness::LifecycleScoped;

            auto& activeFov = snapshot.activeCinematicFovMode;
            activeFov.value = g_activeCinematicFovMode.load(std::memory_order_acquire);
            activeFov.valid = true;
            activeFov.provenance = plugin::SemanticProvenance::CinematicSelection;
            activeFov.freshness = plugin::SemanticFreshness::LifecycleScoped;
        }

        setCurrentFact(snapshot.dialogueBoundaryHookAvailable,
            g_dialogueBoundaryHookAvailable.load(std::memory_order_acquire),
            plugin::SemanticProvenance::DialogueHookState);
        setCurrentFact(snapshot.dialogueNonNativeCapabilityAvailable,
            g_dialogueNonNativeCapabilityAvailable.load(std::memory_order_acquire),
            plugin::SemanticProvenance::DialogueHookState);
        {
            diagnostics::ScopedRuntimeMutex dialogueLock(g_dialogueMutex, diagnostics::RuntimeLockPath::Dialogue);
            auto& phase = snapshot.dialoguePhase;
            switch (g_dialoguePhase) {
            case DialoguePhase::Inactive:
                phase.value = plugin::OverlayDialoguePhase::Inactive;
                break;
            case DialoguePhase::Candidate:
                phase.value = plugin::OverlayDialoguePhase::Candidate;
                break;
            case DialoguePhase::Active:
                phase.value = plugin::OverlayDialoguePhase::Active;
                break;
            case DialoguePhase::Exiting:
                phase.value = plugin::OverlayDialoguePhase::Exiting;
                break;
            case DialoguePhase::RearmPending:
                phase.value = plugin::OverlayDialoguePhase::RearmPending;
                break;
            }
            phase.valid = true;
            phase.provenance = plugin::SemanticProvenance::DialogueLifecycle;
            phase.freshness = plugin::SemanticFreshness::LifecycleScoped;

            auto& activeCorrection = snapshot.activeDialogueCorrection;
            activeCorrection.value = g_dialogueCorrectionActivation.CanCorrect(
                g_runtimeDialogueEnabled.load(std::memory_order_acquire)) &&
                (g_dialoguePhase == DialoguePhase::Active ||
                    g_dialoguePhase == DialoguePhase::Exiting);
            activeCorrection.valid = true;
            activeCorrection.provenance = plugin::SemanticProvenance::DialogueLifecycle;
            activeCorrection.freshness = plugin::SemanticFreshness::LifecycleScoped;

            if (g_activeDialoguePolicy.IsValid()) {
                auto& activePolicy = snapshot.activeDialogueZoomPolicy;
                activePolicy.value = g_activeDialoguePolicy.Value();
                activePolicy.valid = true;
                activePolicy.provenance = plugin::SemanticProvenance::DialogueLifecycle;
                activePolicy.freshness = plugin::SemanticFreshness::LifecycleScoped;
            }
        }

        const auto baseline = g_gameplayBaselineStore.Read();
        auto& baselineFact = snapshot.gameplayBaselineUsable;
        baselineFact.value = camera::IsUsableGameplayBaseline(baseline);
        baselineFact.valid = true;
        baselineFact.provenance = plugin::SemanticProvenance::GameplayBaselineStore;
        baselineFact.freshness = plugin::SemanticFreshness::RetainedUntilInvalidated;

        camera::CameraStateSnapshot cameraState{};
        bool cameraStateValid = false;
        {
            diagnostics::ScopedRuntimeMutex stateLock(g_cameraStateMutex, diagnostics::RuntimeLockPath::CameraState);
            const auto cameraRead = camera::ReadCameraStateSnapshotLocked(
                g_cameraStateSnapshot, g_cameraStateSnapshotValid);
            cameraState = cameraRead.snapshot;
            cameraStateValid = cameraRead.valid;
        }
        snapshot.cameraState = cameraState;
        snapshot.cameraStateValid = cameraStateValid;

        auto& transitionBoundary = snapshot.cameraTransitionBoundary;
        transitionBoundary.value = g_runtime.cameraTransitionBoundary;
        transitionBoundary.valid = transitionBoundary.value.accepted;
        transitionBoundary.provenance =
            plugin::SemanticProvenance::CameraBoundaryObservation;
        transitionBoundary.freshness = plugin::SemanticFreshness::LifecycleScoped;

        const auto coordinator = g_coordinatorState.load(std::memory_order_acquire);
        auto& coordinatorFact = snapshot.coordinator;
        switch (coordinator) {
        case CoordinatorState::Gameplay:
            coordinatorFact.value = plugin::OverlayCoordinatorState::Gameplay;
            break;
        case CoordinatorState::CinematicActive:
            coordinatorFact.value = plugin::OverlayCoordinatorState::CinematicActive;
            break;
        case CoordinatorState::CinematicExiting:
            coordinatorFact.value = plugin::OverlayCoordinatorState::CinematicExiting;
            break;
        }
        coordinatorFact.valid = true;
        coordinatorFact.provenance = plugin::SemanticProvenance::PresentationCoordinator;
        coordinatorFact.freshness = plugin::SemanticFreshness::AtRead;

        auto& viewport = snapshot.runtimeViewportAspect;
        viewport.value = platform::win32::ReadCurrentProcessClientViewportAspect();
        viewport.valid = std::isfinite(viewport.value) && viewport.value > 0.0f;
        viewport.provenance = plugin::SemanticProvenance::RuntimeViewportResolver;
        viewport.freshness = plugin::SemanticFreshness::AtRead;
        return true;
    }

    bool PersistRuntimeSetting(const plugin::RuntimeSettingMutation& mutation, void*)
    {
        std::string_view localeCode;
        if (mutation.kind == plugin::RuntimeSettingKind::OverlayAutoLocale) {
            const auto* value = std::get_if<bool>(&mutation.value);
            if (value && !*value)
                localeCode = g_runtimeOverlayLocale.load(std::memory_order_acquire)->code;
        }
        return plugin::detail::PersistRuntimeSetting(mutation, g_configPath,
            localeCode, [](std::string message) { Log(message); });
    }

    void HotkeyLoop()
    {
        bool previousDialogueKey = false;
        bool previousCinematicKey = false;
        bool previousCinematicFovKey = false;
        bool previousGameplayKey = false;
        while (!WaitForWorkerStop(50)) {
            std::string performanceSummary;
            if (diagnostics::TakeRuntimePerformanceSummary(performanceSummary))
                Log(performanceSummary);
            const auto hotkeySettings = plugin::GetRuntimeSettingsSnapshot();
#if defined(OVERLAY_STARTUP_TIMELINE)
            // Diagnostic-only, one-shot drain even when no Overlay WndProc was
            // activated. It neither dispatches nor consumes the toggle key.
            if (!diagnostics::startup::keyProbeTaken.load(std::memory_order_relaxed) &&
                (GetAsyncKeyState(hotkeySettings.overlayToggleKey) & 0x8000) != 0)
                diagnostics::startup::ToggleProbe();
#endif
            DWORD foregroundProcess{};
            const HWND foreground = GetForegroundWindow();
            const bool foregroundGame = foreground &&
                GetWindowThreadProcessId(foreground, &foregroundProcess) &&
                foregroundProcess == GetCurrentProcessId();
            const auto keys = plugin::SampleRuntimeHotkeys(hotkeySettings,
                foregroundGame, &GetAsyncKeyState);
            if (!keys.observed) {
                // Do not treat a key already held while returning from another
                // application or re-enabling hotkeys as a fresh press.
                previousDialogueKey = previousCinematicKey =
                    previousCinematicFovKey = previousGameplayKey = true;
                continue;
            }
            const bool dialogueKey = keys.dialogue;
            const bool cinematicKey = keys.cinematic;
            const bool cinematicFovKey = keys.cinematicFov;
            const bool gameplayKey = keys.gameplay;
            const bool captureActive = g_hotkeyRebindCaptureActive.load(std::memory_order_acquire);
            const int consumedKey = g_hotkeyRebindConsumedKey.load(std::memory_order_acquire);
            if (consumedKey != 0 && (GetAsyncKeyState(consumedKey) & 0x8000) == 0) {
                int expectedConsumedKey = consumedKey;
                g_hotkeyRebindConsumedKey.compare_exchange_strong(
                    expectedConsumedKey, 0, std::memory_order_acq_rel);
            }
            if (plugin::ShouldDispatchHotkeyAction(hotkeySettings.hotkeysEnabled,
                captureActive, gameplayKey, previousGameplayKey,
                consumedKey == hotkeySettings.gameplayCycleKey)) {
                    const auto previousMode = g_runtimeGameplayMode.load(std::memory_order_acquire);
                    const auto mode = config::NextGameplayMode(
                        previousMode);
                    const auto commit = g_runtimeSettings.ApplyAndPersist(
                        plugin::RuntimeSettingMutation::Gameplay(mode));
                    const auto result = commit.mutation;
                    if (result.accepted && !commit.persisted)
                        plugin::PublishOverlayNotification(
                            plugin::OverlayNotificationKind::PersistenceFailed,
                            plugin::OverlayNotificationAction::GameplayMode,
                            {}, {}, plugin::OverlayNotificationStatus::Error);
                    else
                        plugin::PublishOverlayNotification(plugin::OverlayNotificationKind::SettingChanged,
                            plugin::OverlayNotificationAction::GameplayMode,
                            config::GameplayModeName(previousMode), config::GameplayModeName(mode),
                            result.accepted
                                ? (g_gameplayModeTransitionPending.load(std::memory_order_acquire)
                                    ? plugin::OverlayNotificationStatus::Pending
                                    : plugin::OverlayNotificationStatus::Applied)
                                : plugin::OverlayNotificationStatus::Error);
                    if (result.accepted && commit.persisted)
                        Log("Hotkey ", HotkeyName(hotkeySettings.gameplayCycleKey), ": Gameplay.Mode=",
                            config::GameplayModeName(mode), " persisted.");
                    else if (result.accepted)
                        Log("Hotkey ", HotkeyName(hotkeySettings.gameplayCycleKey), ": Gameplay.Mode=",
                            config::GameplayModeName(mode), " active; persistence failed.");
                    else
                        Log("Hotkey ", HotkeyName(hotkeySettings.gameplayCycleKey), ": Gameplay.Mode rejected.");
                }
                if (plugin::ShouldDispatchHotkeyAction(hotkeySettings.hotkeysEnabled,
                    captureActive, dialogueKey, previousDialogueKey,
                    consumedKey == hotkeySettings.dialogueCycleKey)) {
                    const auto previousPolicy = g_runtimeDialoguePolicy.load(std::memory_order_acquire);
                    const auto policy = NextDialogueZoomPolicy(
                        previousPolicy);
                    const auto commit = g_runtimeSettings.ApplyAndPersist(
                        plugin::RuntimeSettingMutation::Dialogue(policy));
                    const auto result = commit.mutation;
                    if (result.accepted && !commit.persisted)
                        plugin::PublishOverlayNotification(
                            plugin::OverlayNotificationKind::PersistenceFailed,
                            plugin::OverlayNotificationAction::DialogueZoom,
                            {}, {}, plugin::OverlayNotificationStatus::Error);
                    else
                        plugin::PublishOverlayNotification(plugin::OverlayNotificationKind::SettingChanged,
                            plugin::OverlayNotificationAction::DialogueZoom,
                            DialogueZoomPolicyName(previousPolicy), DialogueZoomPolicyName(policy),
                            result.accepted ? plugin::OverlayNotificationStatus::Ready
                                : plugin::OverlayNotificationStatus::Error);
                    if (!result.accepted) {
                        Log("Hotkey ", HotkeyName(hotkeySettings.dialogueCycleKey),
                            ": Dialogue.Mode=", DialogueZoomPolicyName(policy),
                            " rejected.");
                        previousDialogueKey = dialogueKey;
                        previousCinematicKey = cinematicKey;
                        previousCinematicFovKey = cinematicFovKey;
                        previousGameplayKey = gameplayKey;
                        continue;
                    }
                    DialogueZoomPolicy activePolicy = DialogueZoomPolicy::Native;
                    bool activePolicyValid = false;
                    {
            diagnostics::ScopedRuntimeMutex dialogueLock(g_dialogueMutex, diagnostics::RuntimeLockPath::Dialogue);
                        activePolicy = g_activeDialoguePolicy.Value();
                        activePolicyValid = g_activeDialoguePolicy.IsValid();
                    }
                    if (commit.persisted)
                        Log("Hotkey ", HotkeyName(hotkeySettings.dialogueCycleKey), ": Dialogue.Mode selected=",
                            DialogueZoomPolicyName(policy), " active=", DialogueZoomPolicyName(activePolicy),
                            " activeValid=", activePolicyValid, " persisted.");
                    else
                        Log("Hotkey ", HotkeyName(hotkeySettings.dialogueCycleKey), ": Dialogue.Mode selected=",
                            DialogueZoomPolicyName(policy), " active=", DialogueZoomPolicyName(activePolicy),
                            " activeValid=", activePolicyValid, " active; persistence failed.");
                }
                if (plugin::ShouldDispatchHotkeyAction(hotkeySettings.hotkeysEnabled,
                    captureActive, cinematicKey, previousCinematicKey,
                    consumedKey == hotkeySettings.cinematicCycleKey)) {
                    const auto previousPolicy = g_runtimeCinematicPolicy.load(std::memory_order_acquire);
                    const auto policy = NextCinematicAspectPolicy(
                        previousPolicy);
                    const auto commit = g_runtimeSettings.ApplyAndPersist(
                        plugin::RuntimeSettingMutation::CinematicAspect(policy));
                    const auto result = commit.mutation;
                    if (result.accepted && !commit.persisted)
                        plugin::PublishOverlayNotification(
                            plugin::OverlayNotificationKind::PersistenceFailed,
                            plugin::OverlayNotificationAction::CinematicAspect,
                            {}, {}, plugin::OverlayNotificationStatus::Error);
                    else
                        plugin::PublishOverlayNotification(plugin::OverlayNotificationKind::SettingChanged,
                            plugin::OverlayNotificationAction::CinematicAspect,
                            CinematicAspectPolicyName(previousPolicy), CinematicAspectPolicyName(policy),
                            result.accepted ? plugin::OverlayNotificationStatus::Ready
                                : plugin::OverlayNotificationStatus::Error);
                    if (result.accepted && commit.persisted)
                        Log("Hotkey ", HotkeyName(hotkeySettings.cinematicCycleKey), ": Cinematics.Mode=", CinematicAspectPolicyName(policy),
                            " persisted for next cinematic.");
                    else if (result.accepted)
                        Log("Hotkey ", HotkeyName(hotkeySettings.cinematicCycleKey), ": Cinematics.Mode=", CinematicAspectPolicyName(policy),
                            " active for next cinematic; persistence failed.");
                    else
                        Log("Hotkey ", HotkeyName(hotkeySettings.cinematicCycleKey), ": Cinematics.Mode rejected.");
                }
                if (plugin::ShouldDispatchHotkeyAction(hotkeySettings.hotkeysEnabled,
                    captureActive, cinematicFovKey, previousCinematicFovKey,
                    consumedKey == hotkeySettings.cinematicFovCycleKey)) {
                    const auto previousMode = g_runtimeCinematicFovMode.load(std::memory_order_acquire);
                    const auto mode = config::NextCinematicFovMode(
                        previousMode);
                    const auto commit = g_runtimeSettings.ApplyAndPersist(
                        plugin::RuntimeSettingMutation::CinematicFov(mode));
                    const auto result = commit.mutation;
                    if (result.accepted && !commit.persisted)
                        plugin::PublishOverlayNotification(
                            plugin::OverlayNotificationKind::PersistenceFailed,
                            plugin::OverlayNotificationAction::CinematicFov,
                            {}, {}, plugin::OverlayNotificationStatus::Error);
                    else
                        plugin::PublishOverlayNotification(plugin::OverlayNotificationKind::SettingChanged,
                            plugin::OverlayNotificationAction::CinematicFov,
                            config::CinematicFovModeName(previousMode), config::CinematicFovModeName(mode),
                            result.accepted ? plugin::OverlayNotificationStatus::Ready
                                : plugin::OverlayNotificationStatus::Error);
                    if (result.accepted && commit.persisted)
                        Log("Hotkey ", HotkeyName(hotkeySettings.cinematicFovCycleKey),
                            ": Cinematics.FovMode=", config::CinematicFovModeName(mode),
                            " persisted for next cinematic.");
                    else if (result.accepted)
                        Log("Hotkey ", HotkeyName(hotkeySettings.cinematicFovCycleKey),
                            ": Cinematics.FovMode=", config::CinematicFovModeName(mode),
                            " active for next cinematic; persistence failed.");
                    else
                        Log("Hotkey ", HotkeyName(hotkeySettings.cinematicFovCycleKey), ": Cinematics.FovMode rejected.");
                }
            previousDialogueKey = dialogueKey;
            previousCinematicKey = cinematicKey;
            previousCinematicFovKey = cinematicFovKey;
            previousGameplayKey = gameplayKey;
        }
    }

    void CloseInitializationReadiness() noexcept
    {
        g_stopping.store(true, std::memory_order_release);
        g_nativeCallbackAdmission.Close();
        g_runtimeInitializationComplete.store(false, std::memory_order_release);
        g_hotkeyRebindCaptureActive.store(false, std::memory_order_release);
        g_hotkeyRebindConsumedKey.store(0, std::memory_order_release);
        g_cameraWriterHookGate.store(false, std::memory_order_release);
        g_cinematicHookGate.store(false, std::memory_order_release);
        g_runtime.overlayStartupSucceeded.store(false, std::memory_order_release);
        g_workers.SignalStop();
    }

    bool TryInitializationFailureCleanup() noexcept
    {
        if (g_initializationFailureCleanupAttempted) return false;
        g_initializationFailureCleanupAttempted = true;
        try {
#ifdef RUNTIME_ADAPTER_INITIALIZATION_FAILURE_CLEANUP
            RUNTIME_ADAPTER_INITIALIZATION_FAILURE_CLEANUP();
#else
            ResetAllRuntimeResources();
#endif
            return g_workers.WorkerCount() == 0;
        } catch (...) {
            return false;
        }
    }

    DWORD WINAPI Initialize(void*)
    {
        g_runtimeInitializationComplete.store(false, std::memory_order_release);
        if (!platform::win32::PinModuleForProcessLifetime(g_module)) {
            CloseInitializationReadiness();
            OutputDebugStringA("STALKER2CameraTweaks: process residency unavailable; initialization aborted.\n");
            return ERROR_FUNCTION_FAILED;
        }
        WCHAR modulePath[MAX_PATH]{};
        const DWORD modulePathLength = GetModuleFileNameW(g_module,
            modulePath, MAX_PATH);
        if (!modulePathLength || modulePathLength >= MAX_PATH) {
            CloseInitializationReadiness();
            OutputDebugStringA("STALKER2CameraTweaks: module path unavailable; initialization aborted.\n");
            return ERROR_FUNCTION_FAILED;
        }
        const auto moduleDirectory = std::filesystem::path(modulePath).remove_filename();
        const auto logPath = moduleDirectory / "STALKER2CameraTweaks.log";
        const auto configPath = config::DefaultConfigPath(moduleDirectory);
        g_configPath = configPath;
        try {
            g_logger = std::make_shared<spdlog::logger>("STALKER2CameraTweaks",
                std::make_shared<platform::win32::NativeLogSink>(logPath.c_str()));
            g_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
            g_logger->flush_on(spdlog::level::err);
            g_logger->set_error_handler([](const std::string& message) {
                if (!diagnostics::Enabled()) return;
                OutputDebugStringA("CAMERA_LOGGER_ERROR ");
                OutputDebugStringA(message.c_str());
                OutputDebugStringA("\n");
            });
        } catch (...) {
            g_logger.reset();
            CloseInitializationReadiness();
            OutputDebugStringA("STALKER2CameraTweaks: log initialization failed; initialization aborted.\n");
            return ERROR_OPEN_FAILED;
        }

        try {
            if (!CreateWorkerStopEvent())
                throw std::runtime_error("worker stop event could not be created");
            if (!g_nativeCallbackAdmission.Open())
                throw std::runtime_error("native callback admission was not drained before initialization");
            g_stopping.store(false, std::memory_order_release);
            std::string modHash;
            std::string gameHash;
            WCHAR executablePath[MAX_PATH]{};
            GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
            const bool modHashAvailable = ComputeSha256(modulePath, modHash);
            const bool gameHashAvailable = ComputeSha256(executablePath, gameHash);
            Log("Runtime identity: modSha256=", modHashAvailable ? modHash : "unavailable",
                " gameSha256=", gameHashAvailable ? gameHash : "unavailable", ".");
#ifdef POST_EXIT_CAMERA_FOV_WRITE_OWNER_TRACE
            if (!gameHashAvailable || !InstallPostExitFovWriteOwnerTrace(gameHash))
                Log("Post-EXIT FOV physical-write trace: NOT_INSTALLED; production behavior unchanged.");
#endif
#ifdef POST_EXIT_FOV_PRODUCER_STORE_TRACE
            if (!gameHashAvailable || !InstallPostExitProducerStoreTrace(gameHash))
                Log("Post-EXIT FOV producer store trace: NOT_INSTALLED; production behavior unchanged.");
#endif
#ifdef POST_EXIT_FOV_STATE_CONSUMER_TRACE
            if (!gameHashAvailable || !InstallPostExitFovConsumerTrace(gameHash))
                Log("Post-EXIT FOV consumer trace: NOT_INSTALLED; production behavior unchanged.");
#endif
            if (!config::LoadFeatureConfig(configPath, g_config,
                [](const std::filesystem::path& path) {
                    return config::SynchronizeManagedConfigTemplate(path,
                        [](std::string message) { Log(message); });
                },
                [](std::string message) { Log(message); }))
                Log("Configuration unavailable; using defaults with all fixes enabled.");
#ifdef OVERLAY_COMBINED
            g_runtime.overlayStartupSucceeded.store(
                plugin::TryStartConfiguredOverlay(g_config.overlayEnabled,
                    []() { return StartOverlayDiscovery(g_module); }),
                std::memory_order_release);
#else
            g_runtime.overlayStartupSucceeded.store(false, std::memory_order_release);
#endif
            if (!g_config.overlayEnabled)
                Log("Overlay disabled by configuration (Overlay.Enabled=false).");
            diagnostics::SetEnabled(g_config.diagnosticsEnabled);
            InitializeCinematicContinuationDiagnostics(g_config.diagnosticsEnabled);
            g_runtimeCinematicPolicy.store(g_config.cinematicAspectPolicy, std::memory_order_release);
            g_runtimeCinematicFovMode.store(g_config.cinematicFovMode, std::memory_order_release);
            g_runtimeCinematicsEnabled.store(g_config.cinematicsEnabled, std::memory_order_release);
            g_runtimeDialogueEnabled.store(g_config.dialogueEnabled, std::memory_order_release);
            g_runtimeDialoguePolicy.store(g_config.dialogueZoomPolicy, std::memory_order_release);
            const auto* locale = localization::FindLocaleDescriptor(
                g_config.overlayLocaleCode);
            g_runtimeOverlayLocale.store(locale ? locale :
                localization::CanonicalLocaleDescriptor(), std::memory_order_release);
            g_runtimeOverlayLocaleAuto.store(g_config.overlayLocaleAuto,
                std::memory_order_release);
            g_runtimeOverlayFontSize.store(g_config.overlayFontSize, std::memory_order_release);
            g_runtimeGameplayMode.store(g_config.gameplayMode, std::memory_order_release);
            g_runtimeGameplayEnabled.store(g_config.gameplayEnabled, std::memory_order_release);
            if (!g_runtimeSettings.Publish({
                &ApplyRuntimeSetting, nullptr,
                &ReadRuntimeSettings, nullptr,
                &ReadOverlaySemanticSnapshot, nullptr,
                &PersistRuntimeSetting, nullptr})) {
                Log("Runtime settings API publication failed; overlay settings remain unavailable.");
            }
            const bool gameplayRequested = g_config.gameplayEnabled;
#ifdef NATIVE_GAMEPLAY_BASELINE_DIAGNOSTIC
            if (!gameplayRequested) {
                g_postExitTraceWriterCount.store(0, std::memory_order_release);
                g_postExitTraceSequence.fetch_add(1, std::memory_order_acq_rel);
                g_postExitTraceStartNs.store(std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count(),
                    std::memory_order_release);
                g_postExitTraceArmed.store(true, std::memory_order_release);
                Log("Native gameplay baseline diagnostic armed: read-only steady-state trace.");
            }
#endif
            // Runtime controls can change after startup, so install their
            // pass-through-capable observation/components independently of
            // whether the associated feature or hotkey action is enabled now.
            constexpr bool cinematicRequested = true;
            constexpr bool dialogueRequested = true;
            const bool nonNativeDialogueRequested = true;
            auto gameplayStatus = gameplayRequested ? FeatureStatus::Failed : FeatureStatus::Disabled;
            auto cinematicStatus = cinematicRequested ? FeatureStatus::Failed : FeatureStatus::Disabled;
            auto dialogueStatus = dialogueRequested ? FeatureStatus::Failed : FeatureStatus::Disabled;
            auto hotkeyStatus = FeatureStatus::Failed;
            auto diagnosticStatus = FeatureStatus::Disabled;
            const bool zoomTransitionInstalled = gameHashAvailable &&
                InstallZoomTransitionObservation(gameHash);
            if (g_config.diagnosticsEnabled && !zoomTransitionInstalled)
                Log("Zoom transition observation: NOT_INSTALLED; production behavior unchanged.");
            if (g_config.diagnosticsEnabled && zoomTransitionInstalled)
                diagnosticStatus = FeatureStatus::Available;
            Log("Configuration: Gameplay.Enabled=", g_config.gameplayEnabled,
                " Gameplay.Mode=", config::GameplayModeName(g_config.gameplayMode),
                " Cinematics.Enabled=", g_config.cinematicsEnabled ? "true" : "false",
                " Cinematics.Mode=", CinematicAspectPolicyName(g_config.cinematicAspectPolicy),
                " Cinematic.FovMode=", config::CinematicFovModeName(g_config.cinematicFovMode),
                " Dialogue.Enabled=", g_config.dialogueEnabled ? "true" : "false",
                " Dialogue.Mode=", DialogueZoomPolicyName(g_config.dialogueZoomPolicy),
                " Diagnostics.Enabled=", g_config.diagnosticsEnabled ? "true" : "false", ".");
            Log("Gameplay aspect fix loaded. FOV is preserved from the game's settings.");
            cinematicStatus = cinematics::InitializeTransactional(cinematicRequested,
                InstallCinematicAspectComponent, InstallCinematicFovComponent,
                RollbackCinematicAspectComponent, RollbackCinematicFovComponent,
                CommitCinematicComponents);
            g_cinematicAspectComponentAvailable.store(
                cinematicStatus == FeatureStatus::Available, std::memory_order_release);
            const bool lifecycleObservationAvailable =
                cinematicStatus == FeatureStatus::Available;
            g_cinematicLifecycleObservationAvailable.store(lifecycleObservationAvailable,
                std::memory_order_release);
            if (cinematicStatus == FeatureStatus::Available)
                Log("Cinematics feature initialized transactionally: policy=",
                    CinematicAspectPolicyName(g_config.cinematicAspectPolicy), ".");
            else if (cinematicStatus == FeatureStatus::Failed)
                Log("Cinematics feature failed transactionally; native cinematic behavior retained.");
            else
                Log("Cinematics lifecycle hooks unavailable; native cinematic output retained.");
            try {
                if (!VerifyExecutableAndInstruction())
                    throw std::runtime_error("camera-writer signature or validated FOV instruction did not match");
                Log("Installing validated gameplay camera-writer observer.");
                auto gameplayHook = safetyhook::MidHook::create(
                    g_fovWriteAddress, SafeMidHookEntry<&ReplayManualTransition>,
                    safetyhook::MidHook::StartDisabled);
                if (!gameplayHook) throw std::runtime_error("validated gameplay hook creation failed");
                g_hook = std::move(*gameplayHook);
                if (!g_hook.enable()) throw std::runtime_error("validated gameplay hook activation failed");
                g_cameraWriterHookGate.store(true, std::memory_order_release);
                g_gameplayRecoveryObservationAvailable.store(true, std::memory_order_release);
                gameplayStatus = gameplayRequested ? FeatureStatus::Available : FeatureStatus::Disabled;
                Log("Validated gameplay camera-writer observer installed: true correctionEnabled=",
                    gameplayRequested ? "true" : "false", ".");
                if (!gameplayRequested)
                    Log("Gameplay correction disabled by configuration; observer remains active for runtime enabling and recovery.");
            } catch (...) {
                g_hook.reset();
                g_fovWriteAddress = nullptr;
                gameplayStatus = FeatureStatus::Failed;
                g_gameplayRecoveryObservationAvailable.store(false, std::memory_order_release);
                Log("Gameplay camera-writer observer failed locally; runtime Gameplay activation is unavailable.");
            }
#ifdef POST_EXIT_GAMEPLAY_OBSERVER_DIAGNOSTIC
            if (!gameplayRequested) {
                if (!InstallPostExitGameplayObserverDiagnostic())
                    Log("Post-EXIT diagnostic gameplay observer: NOT_INSTALLED; production behavior unchanged.");
            }
#endif
            const bool gameplayRecoveryObservationAvailable =
                g_gameplayRecoveryObservationAvailable.load(std::memory_order_acquire);
            const bool nonNativeDialogueCapability =
                plugin::DialogueLifecycleCapabilityAvailable(
                    true, lifecycleObservationAvailable,
                    gameplayRecoveryObservationAvailable);
            g_dialogueNonNativeCapabilityAvailable.store(nonNativeDialogueCapability,
                std::memory_order_release);
            if (dialogueRequested) {
                const bool currentDialogueCapability =
                    plugin::DialogueLifecycleCapabilityAvailable(
                        nonNativeDialogueRequested, lifecycleObservationAvailable,
                        gameplayRecoveryObservationAvailable);
                if (!currentDialogueCapability) {
                    dialogueStatus = FeatureStatus::Failed;
                    Log("Dialogue feature unavailable: non-Native Dialogue requires "
                        "cinematic lifecycle observation and Gameplay recovery observation.");
                } else {
                    try {
                        if (!InstallDialogueBoundary())
                            throw std::runtime_error("dialogue boundary setup failed");
                        dialogueStatus = FeatureStatus::Available;
                    } catch (...) {
                        g_dialogueBoundaryHook.reset();
                        dialogueStatus = FeatureStatus::Failed;
                        Log("Dialogue feature failed locally; native dialogue behavior retained.");
                    }
                }
            } else {
                dialogueStatus = FeatureStatus::Disabled;
                Log("Dialogue policy is Native; dialogue boundary hook bypassed.");
            }
            Log("Initialization summary: Gameplay=", FeatureStatusName(gameplayStatus),
                " Cinematics=", FeatureStatusName(cinematicStatus),
                " Dialogue=", FeatureStatusName(dialogueStatus), ".");
            if (!StartWorker(
                [](void*) -> DWORD { HotkeyLoop(); return 0; }, "hotkey"))
                Log("Hotkey worker failed locally; production hooks remain active.");
            else {
                hotkeyStatus = FeatureStatus::Available;
                const auto bindings = plugin::GetRuntimeSettingsSnapshot();
                Log("Hotkey worker active: OverlayToggle=", HotkeyName(bindings.overlayToggleKey),
                    " Enabled=", bindings.hotkeysEnabled ? "true" : "false",
                    " Gameplay=", HotkeyName(bindings.gameplayCycleKey),
                    " CinematicAspect=", HotkeyName(bindings.cinematicCycleKey),
                    " CinematicFov=", HotkeyName(bindings.cinematicFovCycleKey),
                    " Dialogue=", HotkeyName(bindings.dialogueCycleKey), ".");
            }
#ifdef GAMEPLAY_ONE_SHOT_CINEMATIC_TRIGGER
            diagnosticStatus = FeatureStatus::Failed;
            if (!StartWorker(OneShotTriggerLoop, "one-shot trigger"))
                Log("Diagnostic worker failed locally; production behavior continues.");
            else {
                diagnosticStatus = FeatureStatus::Available;
                Log("Diagnostic one-shot trigger enabled: press F7 to arm; no timer or repeated flag write is used.");
            }
#ifdef COMBINED_GAMEPLAY_DIAGNOSTIC
            if (!StartWorker(ResolutionMonitorLoop, "resolution monitor")) {
                diagnosticStatus = FeatureStatus::Failed;
                Log("Diagnostic resolution worker failed locally; production behavior continues.");
            } else
                Log("Combined diagnostic resolution monitor enabled: display/window/client changes are logged automatically.");
#endif
#endif
#ifdef DIALOGUE_DISCOVERY_DIAGNOSTIC
            diagnosticStatus = FeatureStatus::Failed;
            if (!StartWorker(DialogueDiscoveryLoop, "dialogue discovery"))
                Log("Dialogue discovery worker failed locally; production behavior continues.");
            else {
                diagnosticStatus = FeatureStatus::Available;
                Log("Dialogue discovery enabled: press F12 for one bounded 10-second read-only trace.");
            }
#endif
            Log("Initialization status: Gameplay=", FeatureStatusName(gameplayStatus),
                " Cinematics=", FeatureStatusName(cinematicStatus),
                " Dialogue=", FeatureStatusName(dialogueStatus),
                " Hotkeys=", FeatureStatusName(hotkeyStatus),
                " Diagnostics=", FeatureStatusName(diagnosticStatus), ".");
            g_logger->flush();
        } catch (const std::exception& exception) {
            if (!TryInitializationFailureCleanup()) {
                CloseInitializationReadiness();
                OutputDebugStringA("STALKER2CameraTweaks: initialization cleanup incomplete.\n");
                return ERROR_UNHANDLED_EXCEPTION;
            }
            CloseInitializationReadiness();
            Log("Gameplay hook setup failed safely: ", exception.what());
            g_logger->flush();
            return ERROR_UNHANDLED_EXCEPTION;
        } catch (...) {
            if (!TryInitializationFailureCleanup()) {
                CloseInitializationReadiness();
                OutputDebugStringA("STALKER2CameraTweaks: initialization cleanup incomplete.\n");
                return ERROR_UNHANDLED_EXCEPTION;
            }
            CloseInitializationReadiness();
            Log("Gameplay hook setup failed safely with an unknown exception.");
            g_logger->flush();
            return ERROR_UNHANDLED_EXCEPTION;
        }
        g_runtimeInitializationComplete.store(true, std::memory_order_release);
        return 0;
    }
}

namespace plugin::detail
{
    RuntimeState& GetRuntimeState() noexcept
    {
        return g_runtime;
    }
}

namespace plugin
{
    void PublishOverlayNotification(OverlayNotificationKind kind,
        OverlayNotificationAction action, std::string primaryValue,
        std::string secondaryValue,
        OverlayNotificationStatus status, std::uint32_t durationMs) noexcept
    {
        if (!g_config.overlayEnabled) return;
        try {
            std::lock_guard lock(g_notificationMutex);
            if (g_pendingNotifications.size() >= 8)
                g_pendingNotifications.pop_front();
            g_pendingNotifications.push_back({++g_nextNotificationId,
                kind, action, std::move(primaryValue), std::move(secondaryValue), status,
                static_cast<std::uint64_t>(GetTickCount64()), durationMs});
        } catch (...) {
            // Notifications are optional UX; never affect the setting mutation path.
        }
        RequestOverlayRedraw();
    }

    void DrainOverlayNotifications(std::vector<OverlayNotification>& notifications) noexcept
    {
        try {
            std::lock_guard lock(g_notificationMutex);
            while (!g_pendingNotifications.empty()) {
                notifications.push_back(std::move(g_pendingNotifications.front()));
                g_pendingNotifications.pop_front();
            }
        } catch (...) {
            // Keep rendering and settings behavior alive if allocation fails.
        }
    }

    void SetHotkeyRebindCaptureActive(bool active) noexcept
    {
        g_hotkeyRebindCaptureActive.store(active, std::memory_order_release);
    }

    void MarkHotkeyRebindKeyConsumed(int key) noexcept
    {
        g_hotkeyRebindConsumedKey.store(key, std::memory_order_release);
    }

    RuntimeSettingsApi& GetRuntimeSettingsApi() noexcept
    {
        return g_runtimeSettings;
    }

    bool SetDetectedOverlayLocale(std::string_view localeCode) noexcept
    {
        if (!g_runtimeOverlayLocaleAuto.load(std::memory_order_acquire)) return false;
        if (localeCode == "sr") localeCode = "sr-Cyrl";
        else if (localeCode == "es") localeCode = "es-ES";
        const auto* descriptor = localization::FindLocaleDescriptor(localeCode);
        if (!descriptor) descriptor = localization::FindLocaleDescriptor("en");
        if (!descriptor) return false;
        const auto* old = g_runtimeOverlayLocale.exchange(descriptor,
            std::memory_order_acq_rel);
        const bool firstSync = !g_runtimeOverlayAutoLocaleSynchronized.exchange(true,
            std::memory_order_acq_rel);
        if (firstSync || old != descriptor) {
            g_runtimeOverlayFontSize.store(descriptor->initialFontSize,
                std::memory_order_release);
            Log("Overlay Auto locale synchronized: game=", descriptor->code,
                " font_size=", descriptor->initialFontSize);
            RequestOverlayRedraw();
        }
        return true;
    }

    RuntimeSettingsSnapshot GetRuntimeSettingsSnapshot() noexcept
    {
        RuntimeSettingsSnapshot snapshot{};
        try { ReadRuntimeSettings(snapshot, nullptr); }
        catch (...) { return {}; } // Do not expose partially filled readback on failure.
        return snapshot;
    }

    bool GetOverlaySemanticSnapshot(OverlaySemanticSnapshot& snapshot) noexcept
    {
        return g_runtimeSettings.SemanticSnapshot(snapshot);
    }

    float GetRuntimeViewportAspect() noexcept
    {
        return platform::win32::ReadClientViewportAspect();
    }

    void SetModuleHandle(HMODULE module)
    {
        g_module = module;
    }

    DWORD WINAPI InitializeThread(void* parameter)
    {
        g_initializationFailureCleanupAttempted = false;
        try {
#if defined(OVERLAY_STARTUP_JOURNAL)
        diagnostics::startup_journal::Mark("INITIALIZATION_WORKER_START");
        diagnostics::startup_journal::Flush();
#endif
#if defined(OVERLAY_STARTUP_TIMELINE)
        diagnostics::startup::Mark("initialization_worker_enter");
        diagnostics::startup::BeginNotifications();
        diagnostics::startup::SnapshotModules("worker_enter");
#endif
#ifdef OVERLAY_COMBINED
        const DWORD result =
#ifdef RUNTIME_ADAPTER_INITIALIZE_THREAD_CALL
            RUNTIME_ADAPTER_INITIALIZE_THREAD_CALL(parameter);
#else
            Initialize(parameter);
#endif
        const bool initializationSucceeded = result == ERROR_SUCCESS;
        const bool mayPublishCoreReady = plugin::ShouldPublishCameraCoreReady(
            initializationSucceeded,
            g_runtimeInitializationComplete.load(std::memory_order_acquire),
            g_stopping.load(std::memory_order_acquire));
        if (mayPublishCoreReady &&
            g_runtime.overlayStartupSucceeded.load(std::memory_order_acquire))
            NotifyOverlayCameraCoreReady();
        else if (!mayPublishCoreReady) {
            CloseInitializationReadiness();
            Log("Core initialization failed or stopped; camera-core readiness remains closed.");
        } else if (g_config.overlayEnabled && g_logger)
            Log("Overlay discovery startup failed; core runtime remains initialized.");
        return result;
#else
 #ifdef RUNTIME_ADAPTER_INITIALIZE_THREAD_CALL
        return RUNTIME_ADAPTER_INITIALIZE_THREAD_CALL(parameter);
 #else
        return Initialize(parameter);
 #endif
#endif
        } catch (...) {
            CloseInitializationReadiness();
            TryInitializationFailureCleanup();
            // Reset may return early with workers/resources still resident.
            // Keep readiness closed and never describe that as successful teardown.
            CloseInitializationReadiness();
#if defined(OVERLAY_STARTUP_JOURNAL)
            try { diagnostics::startup_journal::Mark("INITIALIZATION_FATAL_EXCEPTION"); }
            catch (...) {}
#endif
#if defined(OVERLAY_STARTUP_TIMELINE)
            try { diagnostics::startup::Mark("initialization_fatal_exception"); }
            catch (...) {}
#endif
            OutputDebugStringA("STALKER2CameraTweaks: fatal initialization exception contained.\n");
            return ERROR_UNHANDLED_EXCEPTION;
        }
    }

    void Shutdown()
    {
        ResetAllRuntimeResources();
        if (g_logger) g_logger->flush();
    }

    void NotifyProcessDetach(bool processTerminating)
    {
        if (processTerminating) return;
        g_stopping.store(true, std::memory_order_release);
        g_nativeCallbackAdmission.Close();
        g_cameraWriterHookGate.store(false, std::memory_order_release);
        g_cinematicHookGate.store(false, std::memory_order_release);
        g_workers.SignalStop();
    }
}
