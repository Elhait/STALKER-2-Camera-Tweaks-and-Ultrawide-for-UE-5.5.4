#include "../../src/overlay/composition_presenter_state.hpp"
#include "../../src/overlay/renderer_state.hpp"
#include "../../src/overlay/presenter_runtime_policy.hpp"
#include "../../src/overlay/input_state.hpp"
#include "../../src/overlay/presentation_contracts.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>

namespace
{
    void Require(bool condition, const char* message)
    {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(1);
        }
    }

    void TestCoreAndWindowReadinessAreIndependent()
    {
        overlay::CompositionPresenterState state;
        Require(!state.cameraCoreReady(), "core starts unavailable in fixture");
        Require(!state.MarkWindowAvailable({2560, 1440}),
            "window cannot activate presenter before camera core");
        state.MarkCameraCoreReady();
        Require(state.cameraCoreReady(), "camera core readiness is recorded");
        Require(state.phase() == overlay::PresenterPhase::WaitingForWindow,
            "core readiness only advances Overlay to window discovery");
        Require(state.MarkWindowAvailable({3440, 1440}),
            "valid game window can be accepted after core readiness");
    }

    void TestStartupHintPublicationRequiresPresenterAndSettings()
    {
        overlay::StartupNotificationState notification;
        Require(notification.pending() &&
                notification.ShouldWakePresenter(true) &&
                !notification.ShouldWakePresenter(false),
            "startup notification itself wakes a ready presenter, hidden or not");
        Require(!notification.MarkCreated(false, true) &&
                !notification.MarkCreated(true, false) && notification.pending(),
            "startup event remains pending until presenter and settings are ready");
        Require(notification.MarkCreated(true, true) && !notification.pending() &&
                !notification.ShouldWakePresenter(true),
            "startup state records creation once after its readiness prerequisites");
    }

    void TestStartupHintWaitsForAcceptedAutoLocaleSync()
    {
        using overlay::StartupHintMustWaitForAutoLocale;
        Require(StartupHintMustWaitForAutoLocale(true, false, true),
            "accepted Auto-language sync defers toast creation");
        Require(!StartupHintMustWaitForAutoLocale(true, true, true),
            "synchronized Auto locale permits toast creation");
        Require(!StartupHintMustWaitForAutoLocale(false, false, true),
            "explicit locale does not wait for Auto synchronization");
        Require(!StartupHintMustWaitForAutoLocale(true, false, false),
            "failed async request permits confirmed English fallback");
    }

    void TestGenerationPublicationAndResizeRetirement()
    {
        overlay::CompositionPresenterState state;
        state.MarkCameraCoreReady();
        Require(state.MarkWindowAvailable({2560, 1440}), "initial HWND accepted");

        overlay::SurfaceUpdateToken initial{};
        Require(state.BeginSurfaceUpdate({2560, 1440}, initial),
            "initial surface transaction begins");
        Require(!state.PublishSurfaceUpdate(initial, false, true),
            "partial draw cannot publish a DComp surface");
        Require(!state.PublishSurfaceUpdate(initial, true, false),
            "uncommitted composition cannot publish a DComp surface");
        Require(state.generation() == 0 &&
            state.phase() == overlay::PresenterPhase::WaitingForWindow,
            "failed initial update leaves no generation published");
        Require(state.PublishSurfaceUpdate(initial, true, true),
            "fully drawn committed surface is published");
        Require(state.generation() == 1 && state.extent() ==
            overlay::SurfaceExtent{2560, 1440}, "initial generation owns its extent");

        overlay::SurfaceUpdateToken replacement{};
        Require(state.BeginSurfaceUpdate({5120, 1440}, replacement),
            "resize replacement transaction begins");
        Require(!state.PublishSurfaceUpdate(replacement, true, false),
            "failed Commit does not retire old surface generation");
        Require(state.generation() == 1 && state.extent() ==
            overlay::SurfaceExtent{2560, 1440}, "old generation remains authoritative");
        Require(state.PublishSurfaceUpdate(replacement, true, true),
            "successful resize publishes new generation");
        Require(state.generation() == 2 && state.extent() ==
            overlay::SurfaceExtent{5120, 1440}, "replacement generation is authoritative");

        overlay::SurfaceUpdateToken stale{};
        Require(state.BeginSurfaceUpdate({3440, 1440}, stale),
            "next resize transaction begins");
        auto staleToken = stale;
        staleToken.expectedGeneration = 1;
        staleToken.nextGeneration = 2;
        Require(!state.PublishSurfaceUpdate(staleToken, true, true),
            "stale generation cannot replace the current surface");
    }

    struct RetiredResource
    {
        explicit RetiredResource(int& retiredCount) : retired{retiredCount} {}
        ~RetiredResource() { ++retired; }
        int& retired;
    };

    void TestCommittedSurfaceRetiresOldResourceOnlyAfterCommit()
    {
        overlay::CompositionPresenterState state;
        state.MarkCameraCoreReady();
        Require(state.MarkWindowAvailable({2560, 1440}), "window accepted");
        overlay::SurfaceUpdateToken initial{};
        Require(state.BeginSurfaceUpdate({2560, 1440}, initial),
            "initial generation begins");
        Require(state.PublishSurfaceUpdate(initial, true, true),
            "initial generation publishes");

        int retiredCount = 0;
        auto active = std::make_unique<RetiredResource>(retiredCount);
        overlay::SurfaceUpdateToken resize{};
        Require(state.BeginSurfaceUpdate({3440, 1440}, resize),
            "replacement generation begins");
        {
            auto staged = std::make_unique<RetiredResource>(retiredCount);
            Require(!overlay::PublishCommittedSurfaceResource(state, resize,
                    active, std::move(staged), true, false),
                "uncommitted replacement cannot publish");
            Require(state.generation() == 1 && active && retiredCount == 0,
                "failed candidate cannot replace or prematurely retire active resource");
        }
        Require(active && retiredCount == 1,
            "failed staged resource retires at scope exit while active resource remains");
        {
            auto staged = std::make_unique<RetiredResource>(retiredCount);
            Require(overlay::PublishCommittedSurfaceResource(state, resize,
                    active, std::move(staged), true, true),
                "drawn and committed replacement publishes");
            Require(state.generation() == 2 && active && retiredCount == 2,
                "old resource retires exactly when the new generation is published");
        }
        active.reset();
        Require(retiredCount == 3, "current resource retires on presenter teardown");
    }

    void TestMinimizeRestoreKeepsGeneration()
    {
        overlay::CompositionPresenterState state;
        state.MarkCameraCoreReady();
        Require(state.MarkWindowAvailable({2560, 1440}), "window accepted");
        overlay::SurfaceUpdateToken token{};
        Require(state.BeginSurfaceUpdate({2560, 1440}, token), "surface update begins");
        Require(state.PublishSurfaceUpdate(token, true, true), "surface published");
        state.SetMinimized(true);
        Require(state.phase() == overlay::PresenterPhase::Minimized,
            "minimize suppresses presenter work");
        Require(!state.BeginSurfaceUpdate({2560, 1440}, token),
            "minimized presenter does not draw");
        Require(state.generation() == 1,
            "minimize retains the committed presentation generation");
        state.SetMinimized(false);
        Require(state.phase() == overlay::PresenterPhase::Ready &&
            state.generation() == 1, "restore resumes current generation");
        overlay::SurfaceUpdateToken restored{};
        Require(state.BeginSurfaceUpdate({3440, 1440}, restored) &&
                state.PublishSurfaceUpdate(restored, true, true) &&
                state.generation() == 2 &&
                state.extent() == overlay::SurfaceExtent{3440, 1440},
            "restore can stage, commit and publish a new valid surface generation");
    }

    void TestDeviceLossRecoveryIsBounded()
    {
        overlay::CompositionPresenterState state;
        state.MarkCameraCoreReady();
        Require(state.MarkWindowAvailable({2560, 1440}), "window accepted");
        overlay::SurfaceUpdateToken token{};
        Require(state.BeginSurfaceUpdate({2560, 1440}, token), "surface update begins");
        Require(state.PublishSurfaceUpdate(token, true, true), "surface published");
        Require(state.BeginDeviceRecovery(), "first device recovery is allowed");
        Require(state.CompleteDeviceRecovery(true, {3440, 1440}),
            "recovered surface is committed");
        Require(state.generation() == 2 && state.phase() == overlay::PresenterPhase::Ready,
            "device recovery publishes a new generation");
        Require(!state.BeginDeviceRecovery(),
            "second device recovery is terminal rather than an unbounded retry loop");
        Require(state.phase() == overlay::PresenterPhase::Disabled,
            "terminal graphics failure disables only the Overlay state");
        Require(state.cameraCoreReady(), "Overlay failure does not revoke camera core");
    }

    void TestInputLifecycleAndNotificationRedraw()
    {
        overlay::CompositionPresenterState state;
        state.MarkCameraCoreReady();
        Require(state.MarkWindowAvailable({2560, 1440}), "window accepted");
        overlay::SurfaceUpdateToken token{};
        Require(state.BeginSurfaceUpdate({2560, 1440}, token), "surface update begins");
        Require(state.PublishSurfaceUpdate(token, true, true), "surface published");
        Require(overlay::SelectRedrawReason(true, false, false, false) ==
            overlay::RedrawReason::None, "idle hidden overlay does not redraw");
        Require(overlay::SelectRedrawReason(true, false, false, true) ==
            overlay::RedrawReason::Notification,
            "hidden startup toast/notification continues to redraw");
        Require(overlay::SelectRedrawReason(true, false, true, false) ==
            overlay::RedrawReason::Visible, "visible settings UI redraws");
        Require(overlay::SelectRedrawReason(true, true, false, false) ==
            overlay::RedrawReason::Dirty, "hidden dirty state redraws once");
        Require(overlay::SelectRedrawReason(false, true, true, true) ==
            overlay::RedrawReason::None, "no draw occurs without a committed presenter");

        constexpr std::uint64_t createdAt = 1000;
        constexpr std::uint32_t duration = 10000;
        Require(!overlay::NotificationExpired(createdAt, createdAt, duration) &&
                !overlay::NotificationExpired(createdAt, createdAt + duration - 1,
                    duration), "startup toast remains alive before its deadline");
        Require(overlay::NotificationExpired(createdAt, createdAt + duration,
                    duration), "notification expires at its configured deadline");
        Require(!overlay::NotificationExpired(createdAt, createdAt - 1, duration),
            "clock rollback cannot expire a notification early");

        overlay::NotificationLifetime lifetime;
        Require(lifetime.awaitingFirstCommit() && lifetime.IsDrawable(50000, duration) &&
                !lifetime.IsExpired(50000, duration),
            "pending toast remains drawable and cannot expire before its first commit");
        Require(lifetime.StartAfterCommit(50000) && !lifetime.awaitingFirstCommit() &&
                lifetime.AgeMs(50000) == 0 && lifetime.IsDrawable(50000, duration),
            "visible lifetime starts after the attached surface commit");
        Require(!lifetime.IsExpired(59999, duration) &&
                lifetime.IsExpired(60000, duration) &&
                !lifetime.IsDrawable(60000, duration),
            "toast redraw/expiry deadline is measured from its first committed frame");
    }

    void TestProductionPresenterRoutingAcrossRestore()
    {
        using overlay::PresenterRoute;
        Require(overlay::SelectPresenterRoute(false, false, false, false) ==
                PresenterRoute::Initialize,
            "production route initializes only before any surface generation");
        Require(overlay::SelectPresenterRoute(false, false, true, true) ==
                PresenterRoute::WaitForWindow,
            "minimized window before first generation waits instead of initializing");
        Require(overlay::SelectPresenterRoute(true, true, true, true) ==
                PresenterRoute::UpdateGeometry,
            "minimize routes a live generation through geometry state");
        Require(overlay::SelectPresenterRoute(true, true, false, true) ==
                PresenterRoute::UpdateGeometry,
            "restore routes through geometry and surface validation even when not ready");
        Require(overlay::SelectPresenterRoute(true, false, false, true) ==
                PresenterRoute::RebindWindow,
            "replacement host window rebinds existing presenter ownership");
        Require(overlay::SelectPresenterRoute(true, true, false, false) ==
                PresenterRoute::Idle,
            "stable window does not restart initialization on unrelated redraw");
    }

    void TestTimerIdentityDrivesAutonomousProgression()
    {
        overlay::PresenterTimerIdentity timer;
        constexpr std::uint32_t interval = 16;
        constexpr std::uintptr_t returnedWindowsId = 0xA713;
        Require(timer.NeedsArm(interval), "inactive presenter timer must be armed");
        timer.Arm(returnedWindowsId, interval);
        Require(timer.active() && timer.id() == returnedWindowsId &&
                !timer.NeedsArm(interval),
            "the returned Windows timer identifier is the active timer identity");
        Require(!timer.Matches(0x5A3) && timer.Matches(returnedWindowsId),
            "only WM_TIMER carrying the returned identifier dispatches a frame");

        std::uint64_t nextFrameAt = interval;
        unsigned frameCount = 0;
        for (std::uint64_t now = 0; now <= interval * 3; ++now) {
            if (now < nextFrameAt || !timer.Matches(returnedWindowsId)) continue;
            ++frameCount;
            nextFrameAt = now + timer.intervalMs();
        }
        Require(frameCount == 3,
            "fallback animation timer advances only on its returned identifier");

        Require(timer.NeedsArm(24), "fallback animation cadence change rearms timer");
        const auto oldId = timer.TakeForCancellation();
        Require(oldId == returnedWindowsId && !timer.active(),
            "replacement cancels the actual previous Windows timer ID");
        timer.Arm(0xB024, 24);
        Require(!timer.Matches(returnedWindowsId) && timer.Matches(0xB024),
            "replaced timer rejects stale ID and accepts the new returned ID");
        Require(timer.TakeForCancellation() == 0xB024 && !timer.active(),
            "idle presenter cancels its active timer and clears ownership");
    }

    void TestSemanticPresenterPacingStates()
    {
        using overlay::PresenterFrameState;
        Require(overlay::SelectPresenterFrameState(false, false, false,
                    false, false) == PresenterFrameState::Idle,
            "unready presenter is quiescent");
        Require(overlay::SelectPresenterFrameState(true, true, true,
                    true, true) == PresenterFrameState::Idle,
            "minimize suspends even otherwise active pacing demand");
        Require(overlay::SelectPresenterFrameState(true, false, true,
                    false, false) == PresenterFrameState::NotificationAnimation &&
                overlay::NeedsCompositorClock(
                    PresenterFrameState::NotificationAnimation) &&
                overlay::NeedsFallbackAnimationTimer(
                    PresenterFrameState::NotificationAnimation),
            "notifications use compositor cadence and bounded fallback animation timer");
        Require(overlay::SelectPresenterFrameState(true, false, false,
                    false, true) == PresenterFrameState::InteractiveUi &&
                !overlay::NeedsCompositorClock(PresenterFrameState::InteractiveUi) &&
                !overlay::NeedsFallbackAnimationTimer(
                    PresenterFrameState::InteractiveUi),
            "clean open UI waits for input instead of rendering continuously");
        Require(overlay::SelectPresenterFrameState(true, false, false,
                    true, true) == PresenterFrameState::EventDrivenRedraw &&
                overlay::NeedsCompositorClock(
                    PresenterFrameState::EventDrivenRedraw),
            "interactive changes are paced by the compositor rather than a 30Hz cap");
        Require(overlay::SelectPresenterFrameState(true, false, false,
                    true, false) == PresenterFrameState::EventDrivenRedraw,
            "hidden dirty state still receives one event-driven frame");

        overlay::InputEventBridge bridge;
        bridge.Activate(20, 30);
        Require(bridge.PushRelativeMouseMotion(8, -2) &&
                overlay::SelectPresenterFrameState(true, false, false,
                    true, true) == PresenterFrameState::EventDrivenRedraw,
            "coalesced high-rate input keeps redraw demand at frame-clock cadence");
        overlay::PresenterWakeGate frameTick;
        Require(frameTick.TrySchedule() && !frameTick.TrySchedule(),
            "slow owner thread receives at most one queued compositor tick");
        frameTick.BeginHandling();
        Require(frameTick.TrySchedule(),
            "next compositor tick can queue after the current tick is consumed");
    }

    void TestInputEventsScheduleOneOwnerWake()
    {
        overlay::InputEventBridge bridge;
        bridge.Activate(10, 20);
        overlay::PresenterWakeGate wake;
        Require(bridge.PushRelativeMouseMotion(3, -2) && wake.TrySchedule(),
            "queued pointer movement schedules an owner-thread redraw");
        Require(!wake.TrySchedule() && wake.pending(),
            "bursts of input coalesce into one pending owner wake");
        wake.BeginHandling();
        Require(!wake.pending() && wake.TrySchedule(),
            "a later input event can schedule the next redraw after dispatch");
        wake.CancelFailedPost();
        Require(!wake.pending() && wake.TrySchedule(),
            "failed message posting releases the wake gate for a future redraw");
    }

    void TestToastLifetimeRequiresSubmittedVisibleGeneration()
    {
        constexpr std::uintptr_t toastDrawList = 0x1001;
        constexpr std::uint64_t toastId = 77;
        overlay::NotificationLifetime lifetime;
        Require(!overlay::NotificationDrawListWasSubmitted(toastDrawList,
                    std::span<const overlay::NotificationDrawListEvidence>{}),
            "ImGui autosize frame without a draw list is not a visible toast frame");
        Require(!overlay::StartNotificationLifetimeAfterCommit(lifetime,
                    toastId, std::span<const std::uint64_t>{}, true, 1000) &&
                lifetime.awaitingFirstCommit(),
            "a successful blank surface commit cannot start toast lifetime");

        const overlay::NotificationDrawListEvidence hiddenSizingFrame[]{
            {0x2002, 40}};
        Require(!overlay::NotificationDrawListWasSubmitted(toastDrawList,
                    hiddenSizingFrame),
            "a different visible window does not count as toast submission");
        const overlay::NotificationDrawListEvidence visibleFrame[]{
            {toastDrawList, 24}};
        Require(overlay::NotificationDrawListWasSubmitted(toastDrawList,
                    visibleFrame),
            "toast draw list with vertices is included in rendered ImGui data");
        const std::uint64_t submittedToast[]{toastId};
        Require(!overlay::StartNotificationLifetimeAfterCommit(lifetime,
                    toastId, submittedToast, false, 2000) &&
                lifetime.awaitingFirstCommit(),
            "replacement surface draw cannot start lifetime before attachment commit");
        Require(overlay::StartNotificationLifetimeAfterCommit(lifetime,
                    toastId, submittedToast, true, 3000) &&
                !lifetime.awaitingFirstCommit() && lifetime.AgeMs(3000) == 0,
            "attached committed visible frame starts the notification lifetime");
        Require(!lifetime.IsExpired(12999, 10000) &&
                lifetime.IsExpired(13000, 10000),
            "autonomous redraw advances the visible lifetime to expiry");
    }

    void TestDeviceRecoveryPreservesVisibleAndHiddenInputOwnership()
    {
        for (const bool panelVisible : {false, true}) {
            overlay::CompositionPresenterState presenter;
            presenter.MarkCameraCoreReady();
            Require(presenter.MarkWindowAvailable({2560, 1440}),
                "recovery fixture creates a live presenter target");
            overlay::SurfaceUpdateToken initial{};
            Require(presenter.BeginSurfaceUpdate({2560, 1440}, initial) &&
                    presenter.PublishSurfaceUpdate(initial, true, true),
                "recovery fixture publishes its initial generation");
            overlay::InputState input;
            overlay::InputEventBridge bridge;
            if (panelVisible) {
                input.Toggle();
                input.SetFocused(true);
                bridge.Activate(300, 200);
            }
            const auto before = overlay::CaptureInputOwnership(input, bridge);
            Require(before.Coherent(), "input ownership begins in a coherent state");
            Require(presenter.BeginDeviceRecovery(),
                "graphics recovery begins without taking input ownership");
            Require(presenter.CompleteDeviceRecovery(true, {3440, 1440}),
                "graphics recovery publishes a replacement generation");
            Require(overlay::InputOwnershipUnchanged(before, input, bridge),
                panelVisible
                    ? "visible panel remains visible and bridge-active after recovery"
                    : "hidden panel remains hidden and game-owned after recovery");
        }
    }

    void TestDamageTrackingPreservesRetainedSurfacePixels()
    {
        using overlay::presentation::DamageTracker;
        using overlay::presentation::Rect;
        constexpr Rect extent{0, 0, 5120, 1440};
        DamageTracker damage;
        const Rect initialContent{12, 20, 900, 1200};
        Require(damage.Plan(initialContent, extent, false) == extent,
            "unknown/new surface generation requires a full initialization draw");
        damage.Publish(initialContent, extent);

        const Rect cursorBefore{1250, 700, 1280, 742};
        const Rect cursorAfter{1400, 780, 1430, 822};
        const Rect oldFrame = overlay::presentation::Union(initialContent,
            cursorBefore);
        const Rect newFrame = overlay::presentation::Union(initialContent,
            cursorAfter);
        damage.Publish(oldFrame, extent);
        const Rect cursorDamage = damage.Plan(newFrame, extent, false);
        Require(cursorDamage.left == 12 && cursorDamage.top == 20 &&
                cursorDamage.right == cursorAfter.right &&
                cursorDamage.bottom == initialContent.bottom,
            "damage includes prior and current panel/cursor pixels to erase old cursor content");
        damage.Publish(newFrame, extent);

        const Rect unchanged = damage.Plan(newFrame, extent, false);
        Require(unchanged == newFrame,
            "unchanged visible draw data damages only its conservative content bounds");

        const Rect hiddenDamage = damage.Plan({}, extent, false);
        Require(hiddenDamage == newFrame,
            "hiding panel clears previously retained pixels exactly within old bounds");
        damage.Publish({}, extent);
        Require(damage.Plan({}, extent, false).empty(),
            "after successful clear, hidden idle surface has no redraw damage");

        const Rect notification{4000, 40, 4500, 140};
        damage.Publish(notification, extent);
        const Rect notificationFade = damage.Plan(
            {4000, 40, 4500, 140}, extent, false);
        Require(notificationFade == notification,
            "notification fade redraw retains its visible content bounds");
        Require(damage.Plan(notification, extent, true) == extent,
            "font/locale/DPI changes can explicitly require a full redraw");

        const Rect prior = damage.previous();
        const Rect failedFrame{100, 200, 600, 500};
        (void)damage.Plan(failedFrame, extent, false);
        Require(damage.previous() == prior,
            "uncommitted damage is not published and prior retained state stays authoritative");
        damage.Invalidate();
        Require(damage.Plan(notification, extent, false) == extent,
            "resize/replacement generation invalidates region history and falls back full");
    }

    void TestExplicitDpiCoordinateAndFontContract()
    {
        using overlay::presentation::DpiCoordinateAwareness;
        using overlay::presentation::HostCoordinateToPhysicalScale;
        using overlay::presentation::EffectiveFontPixels;
        Require(HostCoordinateToPhysicalScale(DpiCoordinateAwareness::Unaware,
                    144, 96) == 1.5f,
            "DPI-unaware host client coordinates scale from 96-DPI units to physical pixels");
        Require(HostCoordinateToPhysicalScale(DpiCoordinateAwareness::SystemAware,
                    144, 120) == 1.2f,
            "system-aware host coordinates scale from system DPI to target-window DPI");
        Require(HostCoordinateToPhysicalScale(DpiCoordinateAwareness::PerMonitorAware,
                    144, 96) == 1.0f,
            "per-monitor host client coordinates already use physical pixels");
        Require(EffectiveFontPixels(15, 96) == 15 &&
                EffectiveFontPixels(15, 144) == 23 &&
                EffectiveFontPixels(15, 192) == 30,
            "configured font size is a 96-DPI baseline rasterized at effective physical size");
    }
}

int main()
{
    TestCoreAndWindowReadinessAreIndependent();
    TestStartupHintPublicationRequiresPresenterAndSettings();
    TestStartupHintWaitsForAcceptedAutoLocaleSync();
    TestGenerationPublicationAndResizeRetirement();
    TestCommittedSurfaceRetiresOldResourceOnlyAfterCommit();
    TestMinimizeRestoreKeepsGeneration();
    TestDeviceLossRecoveryIsBounded();
    TestInputLifecycleAndNotificationRedraw();
    TestProductionPresenterRoutingAcrossRestore();
    TestTimerIdentityDrivesAutonomousProgression();
    TestSemanticPresenterPacingStates();
    TestInputEventsScheduleOneOwnerWake();
    TestToastLifetimeRequiresSubmittedVisibleGeneration();
    TestDeviceRecoveryPreservesVisibleAndHiddenInputOwnership();
    TestDamageTrackingPreservesRetainedSurfacePixels();
    TestExplicitDpiCoordinateAndFontContract();
    std::cout << "composition presenter lifecycle: PASS\n";
    return 0;
}
