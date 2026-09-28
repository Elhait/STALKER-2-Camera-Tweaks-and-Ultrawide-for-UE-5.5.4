#pragma once

#include "../camera/camera_state_snapshot.hpp"
#include "../config/feature_config.hpp"

#include <atomic>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace plugin
{
    struct RuntimeSettingsSnapshot
    {
        bool gameplayEnabled{};
        config::GameplayMode gameplayMode{config::GameplayMode::HorPlus};
        config::CinematicAspectPolicy cinematicAspectPolicy{
            config::CinematicAspectPolicy::Auto};
        config::CinematicFovMode cinematicFovMode{
            config::CinematicFovMode::GameplayHorPlus};
        config::DialogueZoomPolicy dialogueZoomPolicy{
            config::DialogueZoomPolicy::Adaptive};
        std::string overlayLocaleCode{localization::CanonicalLocaleCode};
        bool overlayLocaleAuto{};
        bool overlayLocaleAutoSynchronized{};
        int overlayFontSize{config::OverlayFontSizeDefault};
        bool hotkeysEnabled{};
        int overlayToggleKey{config::DefaultHotkeyKey(config::HotkeyBindingId::OverlayToggle)};
        int gameplayCycleKey{config::DefaultHotkeyKey(config::HotkeyBindingId::GameplayMode)};
        int cinematicCycleKey{config::DefaultHotkeyKey(config::HotkeyBindingId::CinematicAspect)};
        int cinematicFovCycleKey{config::DefaultHotkeyKey(config::HotkeyBindingId::CinematicFov)};
        int dialogueCycleKey{config::DefaultHotkeyKey(config::HotkeyBindingId::DialogueZoom)};
    };

    inline bool ShouldDispatchHotkeyAction(bool enabled, bool captureActive,
        bool keyDown, bool previouslyDown, bool captureConsumedKey) noexcept
    {
        return enabled && !captureActive && keyDown && !previouslyDown &&
            !captureConsumedKey;
    }

    enum class SemanticProvenance : std::uint8_t
    {
        None,
        RuntimeSettings,
        GameplayHookGate,
        GameplayTransition,
        CinematicHookState,
        CinematicSelection,
        DialogueHookState,
        DialogueLifecycle,
        GameplayBaselineStore,
        CameraStateSnapshot,
        PresentationCoordinator,
        RuntimeViewportResolver,
    };

    enum class SemanticFreshness : std::uint8_t
    {
        Unknown,
        AtRead,
        LifecycleScoped,
        RetainedUntilInvalidated,
    };

    template<typename T>
    struct SemanticFact
    {
        T value{};
        bool valid{};
        SemanticProvenance provenance{SemanticProvenance::None};
        SemanticFreshness freshness{SemanticFreshness::Unknown};
    };

    enum class OverlayCoordinatorState : std::uint8_t
    {
        Gameplay,
        CinematicActive,
        CinematicExiting,
    };

    enum class OverlayDialoguePhase : std::uint8_t
    {
        Inactive,
        Candidate,
        Active,
        Exiting,
        RearmPending,
    };

    // These facts are read independently from their production owners. The
    // cameraState member is a read-only copy of the shared production snapshot,
    // not a second state store.
    struct OverlaySemanticSnapshot
    {
        SemanticFact<bool> gameplayEnabled{};
        SemanticFact<config::GameplayMode> gameplayMode{};
        SemanticFact<config::CinematicAspectPolicy> cinematicAspectPolicy{};
        SemanticFact<config::CinematicFovMode> cinematicFovMode{};
        SemanticFact<config::DialogueZoomPolicy> dialogueZoomPolicy{};
        SemanticFact<bool> gameplayHookAvailable{};
        SemanticFact<bool> gameplayEnableApplyPending{};
        SemanticFact<bool> gameplayDisableRestorePending{};
        SemanticFact<bool> gameplayModeTransitionPending{};
        SemanticFact<bool> cinematicAspectComponentAvailable{};
        SemanticFact<bool> cinematicFovLifecycleAvailable{};
        SemanticFact<bool> cinematicSelectionActive{};
        SemanticFact<config::CinematicAspectPolicy> activeCinematicAspectPolicy{};
        SemanticFact<config::CinematicFovMode> activeCinematicFovMode{};
        SemanticFact<bool> dialogueBoundaryHookAvailable{};
        SemanticFact<bool> dialogueNonNativeCapabilityAvailable{};
        SemanticFact<OverlayDialoguePhase> dialoguePhase{};
        SemanticFact<config::DialogueZoomPolicy> activeDialogueZoomPolicy{};
        SemanticFact<bool> gameplayBaselineUsable{};
        camera::CameraStateSnapshot cameraState{};
        bool cameraStateValid{};
        SemanticFact<OverlayCoordinatorState> coordinator{};
        SemanticFact<float> runtimeViewportAspect{};
    };

    enum class RuntimeSettingKind
    {
        GameplayEnabled,
        GameplayMode,
        CinematicAspectPolicy,
        CinematicFovMode,
        DialogueZoomPolicy,
        OverlayLocaleCode,
        OverlayAutoLocale,
        OverlayFontSize,
        HotkeysEnabled,
        GameplayCycleKey,
        CinematicCycleKey,
        CinematicFovCycleKey,
        DialogueCycleKey,
        OverlayToggleKey,
    };

    using RuntimeSettingValue = std::variant<
        bool,
        int,
        config::GameplayMode,
        config::CinematicAspectPolicy,
        config::CinematicFovMode,
        config::DialogueZoomPolicy,
        std::string>;

    struct RuntimeSettingMutation
    {
        RuntimeSettingKind kind{};
        RuntimeSettingValue value{false};

        static RuntimeSettingMutation GameplayEnabled(bool value)
        { return {RuntimeSettingKind::GameplayEnabled, value}; }

        static RuntimeSettingMutation Gameplay(config::GameplayMode value)
        { return {RuntimeSettingKind::GameplayMode, value}; }
        static RuntimeSettingMutation CinematicAspect(config::CinematicAspectPolicy value)
        { return {RuntimeSettingKind::CinematicAspectPolicy, value}; }
        static RuntimeSettingMutation CinematicFov(config::CinematicFovMode value)
        { return {RuntimeSettingKind::CinematicFovMode, value}; }
        static RuntimeSettingMutation Dialogue(config::DialogueZoomPolicy value)
        { return {RuntimeSettingKind::DialogueZoomPolicy, value}; }
        static RuntimeSettingMutation LocaleCode(std::string value)
        { return {RuntimeSettingKind::OverlayLocaleCode, value}; }
        static RuntimeSettingMutation AutoLocale(bool value)
        { return {RuntimeSettingKind::OverlayAutoLocale, value}; }
        static RuntimeSettingMutation FontSize(int value)
        { return {RuntimeSettingKind::OverlayFontSize, value}; }
        static RuntimeSettingMutation HotkeysEnabled(bool value)
        { return {RuntimeSettingKind::HotkeysEnabled, value}; }
        static RuntimeSettingMutation GameplayHotkey(int value)
        { return {RuntimeSettingKind::GameplayCycleKey, value}; }
        static RuntimeSettingMutation CinematicAspectHotkey(int value)
        { return {RuntimeSettingKind::CinematicCycleKey, value}; }
        static RuntimeSettingMutation CinematicFovHotkey(int value)
        { return {RuntimeSettingKind::CinematicFovCycleKey, value}; }
        static RuntimeSettingMutation DialogueHotkey(int value)
        { return {RuntimeSettingKind::DialogueCycleKey, value}; }
        static RuntimeSettingMutation OverlayToggleHotkey(int value)
        { return {RuntimeSettingKind::OverlayToggleKey, value}; }
    };

    struct RuntimeMutationResult
    {
        bool accepted{};
        bool changed{};
    };

    enum class OverlayNotificationStatus : std::uint8_t
    {
        Applied,
        Pending,
        Ready,
        Error,
    };

    enum class OverlayNotificationKind : std::uint8_t
    {
        SettingChanged,
        BindingChanged,
        HotkeyConflict,
        StartupOverlayHint,
    };

    enum class OverlayNotificationAction : std::uint8_t
    {
        GameplayMode,
        CinematicAspect,
        CinematicFov,
        DialogueZoom,
        OverlayToggle,
        AnotherSetting,
    };

    struct OverlayNotification
    {
        std::uint64_t id{};
        OverlayNotificationKind kind{OverlayNotificationKind::SettingChanged};
        OverlayNotificationAction action{OverlayNotificationAction::GameplayMode};
        std::string primaryValue;
        std::string secondaryValue;
        OverlayNotificationStatus status{OverlayNotificationStatus::Applied};
        std::uint64_t createdAtMs{};
        std::uint32_t durationMs{3200};
    };

    void PublishOverlayNotification(OverlayNotificationKind kind,
        OverlayNotificationAction action, std::string primaryValue,
        std::string secondaryValue,
        OverlayNotificationStatus status,
        std::uint32_t durationMs = 3200) noexcept;
    void DrainOverlayNotifications(std::vector<OverlayNotification>& notifications) noexcept;
    void SetHotkeyRebindCaptureActive(bool active) noexcept;
    void MarkHotkeyRebindKeyConsumed(int key) noexcept;

    class RuntimeSettingsApi
    {
    public:
        using Handler = RuntimeMutationResult(*)(const RuntimeSettingMutation&, void*);
        using SnapshotHandler = bool(*)(RuntimeSettingsSnapshot&, void*);
        using SemanticSnapshotHandler = bool(*)(OverlaySemanticSnapshot&, void*);
        using PersistHandler = bool(*)(const RuntimeSettingMutation&, void*);

        struct CallbackBundle
        {
            Handler handler{};
            void* userData{};
            SnapshotHandler snapshotHandler{};
            void* snapshotUserData{};
            SemanticSnapshotHandler semanticSnapshotHandler{};
            void* semanticSnapshotUserData{};
            PersistHandler persistHandler{};
            void* persistUserData{};
        };

        // Production owns this API for the process lifetime and publishes it
        // exactly once. The winning publisher initializes the whole bundle
        // before release; readers acquire Published before touching this storage.
        bool Publish(CallbackBundle callbacks) noexcept
        {
            auto expected = PublicationState::Unpublished;
            if (!publicationState_.compare_exchange_strong(expected,
                PublicationState::Publishing, std::memory_order_acq_rel,
                std::memory_order_acquire))
                return false;

            callbacks_ = callbacks;
            publicationState_.store(PublicationState::Published,
                std::memory_order_release);
            return true;
        }

        RuntimeMutationResult Apply(const RuntimeSettingMutation& mutation) const noexcept
        {
            const bool compatible =
                (mutation.kind == RuntimeSettingKind::GameplayEnabled &&
                    std::holds_alternative<bool>(mutation.value)) ||
                (mutation.kind == RuntimeSettingKind::GameplayMode &&
                    std::holds_alternative<config::GameplayMode>(mutation.value)) ||
                (mutation.kind == RuntimeSettingKind::CinematicAspectPolicy &&
                    std::holds_alternative<config::CinematicAspectPolicy>(mutation.value)) ||
                (mutation.kind == RuntimeSettingKind::CinematicFovMode &&
                    std::holds_alternative<config::CinematicFovMode>(mutation.value)) ||
                (mutation.kind == RuntimeSettingKind::DialogueZoomPolicy &&
                    std::holds_alternative<config::DialogueZoomPolicy>(mutation.value)) ||
                (mutation.kind == RuntimeSettingKind::OverlayLocaleCode &&
                    std::holds_alternative<std::string>(mutation.value)) ||
                (mutation.kind == RuntimeSettingKind::OverlayAutoLocale &&
                    std::holds_alternative<bool>(mutation.value)) ||
                (mutation.kind == RuntimeSettingKind::OverlayFontSize &&
                    std::holds_alternative<int>(mutation.value)) ||
                (mutation.kind == RuntimeSettingKind::HotkeysEnabled &&
                    std::holds_alternative<bool>(mutation.value)) ||
                ((mutation.kind == RuntimeSettingKind::GameplayCycleKey ||
                    mutation.kind == RuntimeSettingKind::CinematicCycleKey ||
                    mutation.kind == RuntimeSettingKind::CinematicFovCycleKey ||
                    mutation.kind == RuntimeSettingKind::DialogueCycleKey ||
                    mutation.kind == RuntimeSettingKind::OverlayToggleKey) &&
                    std::holds_alternative<int>(mutation.value));
            const auto* callbacks = PublishedCallbacks();
            if (!compatible || !callbacks || !callbacks->handler) return {};
            // Callbacks are not noexcept (locking/persistence may throw). Keep
            // the API's native-safe contract without bypassing caller boundaries.
            try { return callbacks->handler(mutation, callbacks->userData); }
            catch (...) { return {}; }
        }
        bool Snapshot(RuntimeSettingsSnapshot& snapshot) const noexcept
        {
            const auto* callbacks = PublishedCallbacks();
            try {
                return callbacks && callbacks->snapshotHandler &&
                    callbacks->snapshotHandler(snapshot, callbacks->snapshotUserData);
            } catch (...) { return false; }
        }
        bool SemanticSnapshot(OverlaySemanticSnapshot& snapshot) const noexcept
        {
            const auto* callbacks = PublishedCallbacks();
            try {
                return callbacks && callbacks->semanticSnapshotHandler &&
                    callbacks->semanticSnapshotHandler(snapshot,
                        callbacks->semanticSnapshotUserData);
            } catch (...) { return false; }
        }
        bool Persist(const RuntimeSettingMutation& mutation) const noexcept
        {
            const auto* callbacks = PublishedCallbacks();
            try {
                return callbacks && callbacks->persistHandler &&
                    callbacks->persistHandler(mutation, callbacks->persistUserData);
            } catch (...) { return false; }
        }

    private:
        enum class PublicationState : std::uint8_t
        {
            Unpublished,
            Publishing,
            Published,
        };

        const CallbackBundle* PublishedCallbacks() const noexcept
        {
            return publicationState_.load(std::memory_order_acquire) ==
                PublicationState::Published ? &callbacks_ : nullptr;
        }

        CallbackBundle callbacks_{};
        std::atomic<PublicationState> publicationState_{PublicationState::Unpublished};
    };
}
