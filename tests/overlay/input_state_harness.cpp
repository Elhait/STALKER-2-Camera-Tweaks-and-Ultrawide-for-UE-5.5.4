#include "../../src/overlay/input/input_state.hpp"
#include "../../src/overlay/input/game_thread_subclass.hpp"
#include "../../src/overlay/composition/stable_record_pool.hpp"
#include "../../src/overlay/composition/owner_finalizer.hpp"
#include "../../src/overlay/presentation/hotkey_binding_presentation.hpp"

#include <iostream>
#include <thread>
#include <atomic>
#include <array>
#include <cstdint>

namespace
{
    constexpr wchar_t FixtureClassName[] =
        L"CameraTweaks.InputSubclassLifecycleFixture";
    constexpr wchar_t FinalizerFixtureClassName[] =
        L"CameraTweaks.OwnerFinalizerFixture";
    constexpr wchar_t FixtureBindingProperty[] =
        L"CameraTweaks.InputSubclassLifecycleFixture.Binding";
    constexpr UINT FixtureProbeMessage = WM_APP + 0x621;
    constexpr UINT FixtureBlockingMessage = WM_APP + 0x622;
    bool Check(bool condition, const char* name);

    struct FixtureBinding
    {
        HWND window{};
        WNDPROC continuation{};
        std::uint64_t generation{};
    };

    FixtureBinding g_fixtureBinding{};
    WNDPROC g_foreignContinuation{};
    std::uint64_t g_fixtureCurrentGeneration{1};
    unsigned g_fixtureSubclassCalls{};
    unsigned g_fixtureForeignCalls{};
    unsigned g_fixtureBaseCalls{};
    unsigned g_fixtureOwnedMutations{};
    bool g_fixtureNcDestroySawBinding{};
    overlay::InputCallbackAdmissionGate g_fixtureAdmission;
    HANDLE g_fixtureBlockEntered{};
    HANDLE g_fixtureBlockRelease{};
    HANDLE g_fixtureCallbackDrained{};
    HANDLE g_fixtureWindowReady{};
    std::atomic<HWND> g_fixtureWorkerWindow{};

    LRESULT CALLBACK FixtureBaseWindowProc(HWND window, UINT message,
        WPARAM wParam, LPARAM lParam)
    {
        if (message == FixtureProbeMessage) {
            ++g_fixtureBaseCalls;
            return 0x621;
        }
        return DefWindowProcW(window, message, wParam, lParam);
    }

    LRESULT CALLBACK FixtureSubclassProc(HWND window, UINT message,
        WPARAM wParam, LPARAM lParam)
    {
        auto* binding = static_cast<FixtureBinding*>(
            GetPropW(window, FixtureBindingProperty));
        if (!binding || binding->window != window || !binding->continuation)
            return DefWindowProcW(window, message, wParam, lParam);
        const bool admitted = g_fixtureAdmission.TryEnter();
        ++g_fixtureSubclassCalls;
        if (admitted && binding->generation == g_fixtureCurrentGeneration)
            ++g_fixtureOwnedMutations;
        if (admitted && message == FixtureBlockingMessage) {
            SetEvent(g_fixtureBlockEntered);
            WaitForSingleObject(g_fixtureBlockRelease, 5000);
        }
        const auto result = CallWindowProcW(binding->continuation,
            window, message, wParam, lParam);
        if (message == WM_NCDESTROY) {
            g_fixtureNcDestroySawBinding = GetPropW(window,
                FixtureBindingProperty) == binding;
            RemovePropW(window, FixtureBindingProperty);
        }
        if (admitted && g_fixtureAdmission.Leave() &&
            g_fixtureCallbackDrained)
            SetEvent(g_fixtureCallbackDrained);
        return result;
    }

    LRESULT CALLBACK FixtureForeignSubclassProc(HWND window, UINT message,
        WPARAM wParam, LPARAM lParam)
    {
        ++g_fixtureForeignCalls;
        return CallWindowProcW(g_foreignContinuation, window, message, wParam,
            lParam);
    }

    bool RunWin32SubclassLifecycleFixture()
    {
        WNDCLASSW windowClass{};
        windowClass.lpfnWndProc = &FixtureBaseWindowProc;
        windowClass.hInstance = GetModuleHandleW(nullptr);
        windowClass.lpszClassName = FixtureClassName;
        if (!RegisterClassW(&windowClass) &&
            GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return false;
        HWND window = CreateWindowExW(0, FixtureClassName, L"", WS_OVERLAPPED,
            0, 0, 16, 16, nullptr, nullptr, windowClass.hInstance, nullptr);
        if (!window) return false;
        g_fixtureBinding = {window, nullptr, 1};
        if (!SetPropW(window, FixtureBindingProperty, &g_fixtureBinding)) {
            DestroyWindow(window);
            return false;
        }
        SetLastError(0);
        g_fixtureBinding.continuation = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrW(window, GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(&FixtureSubclassProc)));
        if (!g_fixtureBinding.continuation) {
            RemovePropW(window, FixtureBindingProperty);
            DestroyWindow(window);
            return false;
        }

        bool pass = Check(SendMessageW(window, FixtureProbeMessage, 0, 0) == 0x621 &&
            g_fixtureSubclassCalls == 1 && g_fixtureBaseCalls == 1 &&
            g_fixtureOwnedMutations == 1,
            "real_win32_subclass_forwards_native_continuation_once");

        g_fixtureCurrentGeneration = 2;
        const auto mutationsBeforeStale = g_fixtureOwnedMutations;
        pass &= Check(SendMessageW(window, FixtureProbeMessage, 0, 0) == 0x621 &&
            g_fixtureOwnedMutations == mutationsBeforeStale &&
            g_fixtureBaseCalls == 2,
            "stale_win32_subclass_generation_forwards_without_owner_mutation");
        g_fixtureCurrentGeneration = 1;

        SetLastError(0);
        g_foreignContinuation = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
            window, GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(&FixtureForeignSubclassProc)));
        pass &= Check(g_foreignContinuation == &FixtureSubclassProc &&
            SendMessageW(window, FixtureProbeMessage, 0, 0) == 0x621 &&
            g_fixtureForeignCalls == 1 && g_fixtureSubclassCalls == 3 &&
            g_fixtureBaseCalls == 3,
            "foreign_subclass_replacement_preserves_original_chain");

        DestroyWindow(window);
        pass &= Check(g_fixtureNcDestroySawBinding &&
            GetPropW(window, FixtureBindingProperty) == nullptr,
            "wm_ncdestroy_continuation_observes_then_removes_binding");
        UnregisterClassW(FixtureClassName, windowClass.hInstance);
        return pass;
    }

    std::atomic<WNDPROC> g_installedPredecessor{};
    LRESULT CALLBACK InstalledFixtureProc(HWND window, UINT message,
        WPARAM wParam, LPARAM lParam)
    {
        return CallWindowProcW(g_installedPredecessor.load(std::memory_order_acquire),
            window, message, wParam, lParam);
    }

    bool RunPredecessorPublicationFixture()
    {
        WNDCLASSW wc{};
        wc.lpfnWndProc = &FixtureBaseWindowProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = FixtureClassName;
        if (!RegisterClassW(&wc)) return false;
        const HWND window = CreateWindowExW(0, FixtureClassName, L"", WS_POPUP,
            0, 0, 16, 16, nullptr, nullptr, wc.hInstance, nullptr);
        if (!window) return false;
        const auto observed = reinterpret_cast<WNDPROC>(
            GetWindowLongPtrW(window, GWLP_WNDPROC));
        g_foreignContinuation = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
            window, GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(&FixtureForeignSubclassProc)));
        g_fixtureBinding = {window, observed, 1};
        SetPropW(window, FixtureBindingProperty, &g_fixtureBinding);
        const auto actualPredecessor = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
            window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&FixtureSubclassProc)));
        const auto foreignBefore = g_fixtureForeignCalls;
        SendMessageW(window, FixtureProbeMessage, 0, 0);
        bool pass = Check(actualPredecessor == &FixtureForeignSubclassProc &&
                g_fixtureForeignCalls == foreignBefore,
            "predecessor_publication_gap_reproduced_with_real_win32_chain");
        g_fixtureBinding.continuation = actualPredecessor;
        SendMessageW(window, FixtureProbeMessage, 0, 0);
        pass &= Check(g_fixtureForeignCalls == foreignBefore + 1,
            "published_actual_predecessor_preserves_foreign_chain");
        // Restore the foreign top, then use the production installation helper.
        SetWindowLongPtrW(window, GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(actualPredecessor));
        RemovePropW(window, FixtureBindingProperty);
        bool wrongThreadAccepted{};
        std::thread wrongThread([&] {
            wrongThreadAccepted = overlay::InstallGameThreadSubclass(window,
                FixtureBindingProperty, &g_fixtureBinding, &InstalledFixtureProc,
                g_installedPredecessor);
        });
        wrongThread.join();
        pass &= Check(!wrongThreadAccepted && !GetPropW(window, FixtureBindingProperty),
            "off_thread_subclass_install_refused_without_mutation");
        const bool installed = overlay::InstallGameThreadSubclass(window,
            FixtureBindingProperty, &g_fixtureBinding, &InstalledFixtureProc,
            g_installedPredecessor);
        const auto beforeProductionProbe = g_fixtureForeignCalls;
        pass &= Check(installed && g_installedPredecessor.load() == actualPredecessor &&
            SendMessageW(window, FixtureProbeMessage, 0, 0) == 0x621 &&
            g_fixtureForeignCalls == beforeProductionProbe + 1,
            "game_thread_install_publishes_actual_chain_before_next_dispatch");
        DestroyWindow(window);
        UnregisterClassW(FixtureClassName, wc.hInstance);
        g_fixtureSubclassCalls = g_fixtureForeignCalls = g_fixtureBaseCalls = 0;
        g_fixtureOwnedMutations = 0;
        g_fixtureNcDestroySawBinding = false;
        return pass;
    }

    DWORD WINAPI FixtureWindowThread(void*)
    {
        WNDCLASSW windowClass{};
        windowClass.lpfnWndProc = &FixtureBaseWindowProc;
        windowClass.hInstance = GetModuleHandleW(nullptr);
        windowClass.lpszClassName = FixtureClassName;
        if (!RegisterClassW(&windowClass) &&
            GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            SetEvent(g_fixtureWindowReady);
            return 4;
        }
        HWND window = CreateWindowExW(0, FixtureClassName, L"", WS_OVERLAPPED,
            0, 0, 16, 16, nullptr, nullptr, windowClass.hInstance, nullptr);
        if (!window) {
            SetEvent(g_fixtureWindowReady);
            return 1;
        }
        g_fixtureBinding = {window, nullptr, 77};
        if (!SetPropW(window, FixtureBindingProperty, &g_fixtureBinding)) {
            DestroyWindow(window);
            SetEvent(g_fixtureWindowReady);
            return 2;
        }
        SetLastError(0);
        g_fixtureBinding.continuation = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrW(window, GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(&FixtureSubclassProc)));
        if (!g_fixtureBinding.continuation) {
            RemovePropW(window, FixtureBindingProperty);
            DestroyWindow(window);
            SetEvent(g_fixtureWindowReady);
            return 3;
        }
        g_fixtureWorkerWindow.store(window, std::memory_order_release);
        SetEvent(g_fixtureWindowReady);
        MSG message{};
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            if (message.message == WM_CLOSE) {
                DestroyWindow(window);
                break;
            }
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        return 0;
    }

    bool RunWin32CallbackDrainFixture()
    {
        g_fixtureBlockEntered = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        g_fixtureBlockRelease = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        g_fixtureCallbackDrained = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        g_fixtureWindowReady = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!g_fixtureBlockEntered || !g_fixtureBlockRelease ||
            !g_fixtureCallbackDrained || !g_fixtureWindowReady)
            return false;
        HANDLE thread = CreateThread(nullptr, 0, &FixtureWindowThread,
            nullptr, 0, nullptr);
        if (!thread) return false;
        bool pass = WaitForSingleObject(g_fixtureWindowReady, 3000) == WAIT_OBJECT_0;
        const HWND window = g_fixtureWorkerWindow.load(std::memory_order_acquire);
        pass &= window != nullptr;
        std::thread sender;
        if (window) {
            sender = std::thread([window] {
                SendMessageW(window, FixtureBlockingMessage, 0, 0);
            });
            pass &= WaitForSingleObject(g_fixtureBlockEntered, 3000) == WAIT_OBJECT_0;
            g_fixtureAdmission.Close();
            pass &= Check(g_fixtureAdmission.inFlight() == 1 &&
                WaitForSingleObject(g_fixtureCallbackDrained, 0) == WAIT_TIMEOUT,
                "win32_callback_remains_admitted_until_native_continuation_returns");
            SetEvent(g_fixtureBlockRelease);
            sender.join();
            pass &= WaitForSingleObject(g_fixtureCallbackDrained, 3000) == WAIT_OBJECT_0 &&
                g_fixtureAdmission.inFlight() == 0;
        }
        if (window) PostMessageW(window, WM_CLOSE, 0, 0);
        pass &= WaitForSingleObject(thread, 3000) == WAIT_OBJECT_0;
        CloseHandle(thread);
        CloseHandle(g_fixtureWindowReady);
        CloseHandle(g_fixtureCallbackDrained);
        CloseHandle(g_fixtureBlockRelease);
        CloseHandle(g_fixtureBlockEntered);
        g_fixtureWindowReady = nullptr;
        g_fixtureCallbackDrained = nullptr;
        g_fixtureBlockRelease = nullptr;
        g_fixtureBlockEntered = nullptr;
        return pass;
    }

    LRESULT CALLBACK FinalizerFixtureWindowProc(HWND window, UINT message,
        WPARAM wParam, LPARAM lParam)
    {
        return DefWindowProcW(window, message, wParam, lParam);
    }

    bool Check(bool condition, const char* name)
    {
        if (!condition) std::cerr << "FAIL: " << name << "\n";
        return condition;
    }
}

int main()
{
    overlay::InputState input;
    bool pass = true;
    {
        overlay::StableRecordPool<int, 2> records;
        int* first = records.TryAcquire();
        int* second = records.TryAcquire();
        const bool reservedCapacity = first && second && !records.TryAcquire();
        if (first) *first = 11;
        if (second) *second = 22;
        records.ReleaseUncommitted(first);
        int* reusedUnpublished = records.TryAcquire();
        const bool unpublishedReusable = reusedUnpublished == first;
        const bool committed = records.Commit(reusedUnpublished) && records.Commit(second);
        records.ReleaseUncommitted(reusedUnpublished);
        const bool committedNeverReused = !records.TryAcquire() &&
            records.retainedCount() == 2 && *reusedUnpublished == 11 && *second == 22;
        pass &= Check(reservedCapacity && unpublishedReusable && committed &&
            committedNeverReused,
            "continuation_records_have_bounded_stable_lifetime");
    }
    {
        overlay::StableRecordPool<std::uint64_t, 64> records;
        std::array<std::uint64_t*, 64> retained{};
        bool allCommitted = true;
        for (std::size_t index = 0; index < retained.size(); ++index) {
            retained[index] = records.TryAcquire();
            if (!retained[index]) { allCommitted = false; break; }
            *retained[index] = index + 100;
            allCommitted &= records.Commit(retained[index]);
        }
        bool allStable = allCommitted && records.retainedCount() == retained.size() &&
            records.TryAcquire() == nullptr;
        for (std::size_t index = 0; index < retained.size(); ++index)
            allStable &= retained[index] && *retained[index] == index + 100;
        pass &= Check(allStable, "sixty_four_retained_continuations_are_stable_and_bounded");
    }
    config::FeatureConfig configuredBindings{};
    plugin::RuntimeSettingsSnapshot displayedBindings{};
    for (std::size_t index = 0; index < config::HotkeyBindingRegistry.size(); ++index) {
        const auto& binding = config::HotkeyBindingRegistry[index];
        const auto* presentation = overlay::FindHotkeyPresentation(binding.id);
        const int key = VK_F1 + static_cast<int>(index);
        config::SetHotkeyBindingValue(configuredBindings, binding.id, key);
        pass &= Check(presentation && !presentation->label.path.empty(),
            "every_config_binding_has_a_ui_label");
        if (!presentation) continue;
        const plugin::RuntimeSettingsSnapshot runtimeDefaults{};
        pass &= Check(runtimeDefaults.*(presentation->settingKey) ==
                binding.defaultVirtualKey,
            "runtime_default_matches_binding_metadata");
        displayedBindings.*(presentation->settingKey) = key;
        pass &= Check(config::HotkeyBindingValue(configuredBindings, binding.id) ==
                displayedBindings.*(presentation->settingKey),
            "ui_and_config_read_the_same_binding_identity");
        input.BeginRebind(binding.id, key);
        pass &= Check(input.rebindAction() == binding.id &&
                input.IsCapturing(binding.id),
            "input_rebind_uses_config_binding_identity");
        input.CancelRebind();
        for (std::size_t other = index + 1;
            other < config::HotkeyBindingRegistry.size(); ++other)
            pass &= Check(!overlay::FindHotkeyPresentation(
                    config::HotkeyBindingRegistry[other].id) ||
                    config::HotkeyBindingRegistry[other].id != binding.id,
                "binding_identities_are_unique");
    }
    pass &= Check(overlay::HotkeyBindingPresentations.size() ==
            config::HotkeyBindingRegistry.size(),
        "ui_and_config_binding_inventory_sizes_match");
    pass &= Check(!input.visible() && !input.ShouldCapture(WM_MOUSEMOVE),
        "hidden_by_default");
    overlay::InputEventBridge bridge;
    overlay::OwnerInputCommandQueue ownerCommands;
    overlay::OwnerInputCommand queuedCommand{};
    pass &= Check(ownerCommands.empty() && !ownerCommands.TryPop(queuedCommand),
        "owner_command_queue_starts_empty");
    bool ownerCommandsAccepted = true;
    for (int index = 0; index < 256; ++index) {
        ownerCommandsAccepted &= ownerCommands.TryPush({
            reinterpret_cast<HWND>(static_cast<std::uintptr_t>(index + 1)),
            static_cast<std::uint64_t>(index + 1), WM_KEYDOWN,
            static_cast<WPARAM>('A' + index % 26), 0,
            static_cast<std::uintptr_t>(index + 100), false});
    }
    pass &= Check(ownerCommandsAccepted && ownerCommands.full() &&
        ownerCommands.size() == 256 &&
        !ownerCommands.TryPush({}),
        "owner_command_queue_rejects_overflow_without_overwriting_accepted_actions");
    bool ownerCommandsOrdered = true;
    for (int index = 0; index < 256; ++index) {
        ownerCommandsOrdered &= ownerCommands.TryPop(queuedCommand) &&
            queuedCommand.generation == static_cast<std::uint64_t>(index + 1) &&
            queuedCommand.bindingIdentity ==
                static_cast<std::uintptr_t>(index + 100);
    }
    pass &= Check(ownerCommandsOrdered && ownerCommands.empty(),
        "owner_command_queue_preserves_fifo_generation_and_binding_identity");
    overlay::OwnerInputCommandQueue concurrentOwnerCommands;
    constexpr unsigned ProducerCount = 4;
    constexpr unsigned CommandsPerProducer = 1000;
    constexpr unsigned TotalCommands = ProducerCount * CommandsPerProducer;
    std::array<bool, TotalCommands> seenOwnerCommands{};
    std::array<std::thread, ProducerCount> commandProducers;
    for (unsigned producer = 0; producer < ProducerCount; ++producer) {
        commandProducers[producer] = std::thread([&, producer] {
            for (unsigned item = 0; item < CommandsPerProducer; ++item) {
                const auto identity = producer * CommandsPerProducer + item + 1;
                const overlay::OwnerInputCommand command{
                    reinterpret_cast<HWND>(static_cast<std::uintptr_t>(identity)),
                    identity, WM_KEYDOWN, 0, 0, identity, false};
                while (!concurrentOwnerCommands.TryPush(command))
                    std::this_thread::yield();
            }
        });
    }
    unsigned consumedOwnerCommands{};
    bool concurrentCommandsValid = true;
    while (consumedOwnerCommands < TotalCommands) {
        if (!concurrentOwnerCommands.TryPop(queuedCommand)) {
            std::this_thread::yield();
            continue;
        }
        const auto identity = static_cast<unsigned>(queuedCommand.generation);
        if (!identity || identity > TotalCommands || seenOwnerCommands[identity - 1] ||
            queuedCommand.bindingIdentity != identity) {
            concurrentCommandsValid = false;
        } else {
            seenOwnerCommands[identity - 1] = true;
        }
        ++consumedOwnerCommands;
    }
    for (auto& producer : commandProducers) producer.join();
    pass &= Check(concurrentCommandsValid && concurrentOwnerCommands.empty(),
        "owner_command_queue_preserves_all_commands_from_overlapping_window_threads");

    overlay::InputCallbackAdmissionGate admission;
    pass &= Check(admission.TryEnter() && admission.inFlight() == 1,
        "admission_enters_and_counts_callback");
    pass &= Check(admission.Close() && admission.closed() &&
        !admission.TryEnter() && admission.inFlight() == 1,
        "terminal_close_rejects_new_callbacks_without_releasing_in_flight");
    pass &= Check(admission.Leave() && admission.inFlight() == 0 &&
        !admission.Close(),
        "last_in_flight_callback_observes_terminal_drain");
    pass &= RunPredecessorPublicationFixture();
    pass &= RunWin32SubclassLifecycleFixture();

    WNDCLASSW finalizerWindowClass{};
    finalizerWindowClass.lpfnWndProc = &FinalizerFixtureWindowProc;
    finalizerWindowClass.hInstance = GetModuleHandleW(nullptr);
    finalizerWindowClass.lpszClassName = FinalizerFixtureClassName;
    const bool finalizerClassRegistered = RegisterClassW(&finalizerWindowClass) != 0;
    pass &= Check(finalizerClassRegistered,
        "owner_finalizer_fixture_window_class_registered");

    overlay::InputCallbackAdmissionGate terminalAdmission;
    overlay::InputEventBridge terminalInputBridge;
    terminalInputBridge.Activate(3, 4);
    pass &= Check(terminalAdmission.TryEnter() &&
        terminalInputBridge.active(), "failure_path_fixture_admits_live_input");
    HANDLE ownerBlocked = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    HANDLE allowOwnerExit = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    HANDLE callbackDrained = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    std::atomic<DWORD> finalizerOwnerThread{};
    std::atomic<bool> ownerReleasedResourceOnOwner{};
    std::atomic<HWND> ownerDisplayWindow{};
    std::thread stalledOwner([&] {
        finalizerOwnerThread.store(GetCurrentThreadId(),
            std::memory_order_release);
        const HWND display = CreateWindowExW(WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
            FinalizerFixtureClassName, L"", WS_POPUP, 0, 0, 16, 16, nullptr, nullptr,
            GetModuleHandleW(nullptr), nullptr);
        ownerDisplayWindow.store(display, std::memory_order_release);
        SetEvent(ownerBlocked);
        WaitForSingleObject(allowOwnerExit, INFINITE);
        WaitForSingleObject(callbackDrained, INFINITE);
        HANDLE ownerResource = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        overlay::OwnerFinalizer finalizer;
        const auto finalOutcome = finalizer.Run([&] {
            const bool released = ownerResource == nullptr &&
                !IsWindow(ownerDisplayWindow.load(std::memory_order_acquire)) &&
                ownerReleasedResourceOnOwner.load(std::memory_order_acquire) &&
                terminalAdmission.closed() &&
                terminalAdmission.inFlight() == 0;
            return released ? overlay::OwnerFinalizationOutcome::Complete
                : overlay::OwnerFinalizationOutcome::Retained;
        }, [&] {
            if (GetCurrentThreadId() != finalizerOwnerThread.load(
                    std::memory_order_acquire))
                return overlay::OwnerFinalizationOutcome::Retained;
            const HWND displayWindow = ownerDisplayWindow.load(
                std::memory_order_acquire);
            if (!displayWindow || !DestroyWindow(displayWindow))
                return overlay::OwnerFinalizationOutcome::Retained;
            ownerDisplayWindow.store(nullptr, std::memory_order_release);
            return !IsWindow(displayWindow)
                ? overlay::OwnerFinalizationOutcome::Complete
                : overlay::OwnerFinalizationOutcome::Retained;
        }, [&] {
            if (GetCurrentThreadId() != finalizerOwnerThread.load(
                    std::memory_order_acquire) || !ownerResource)
                return overlay::OwnerFinalizationOutcome::Retained;
            const bool closed = CloseHandle(ownerResource) != FALSE;
            if (closed) ownerResource = nullptr;
            ownerReleasedResourceOnOwner.store(closed,
                std::memory_order_release);
            return closed ? overlay::OwnerFinalizationOutcome::Complete
                : overlay::OwnerFinalizationOutcome::Retained;
        }, [&] {
            return GetCurrentThreadId() == finalizerOwnerThread.load(
                std::memory_order_acquire)
                ? overlay::OwnerFinalizationOutcome::Complete
                : overlay::OwnerFinalizationOutcome::Retained;
        });
        if (finalOutcome != overlay::OwnerFinalizationOutcome::Complete)
            ownerReleasedResourceOnOwner.store(false,
                std::memory_order_release);
    });
    const bool ownerBlockedReady = ownerBlocked && allowOwnerExit &&
        callbackDrained &&
        WaitForSingleObject(ownerBlocked, 5000) == WAIT_OBJECT_0;
    const HWND hostWindow = CreateWindowExW(0, FinalizerFixtureClassName, L"",
        WS_OVERLAPPED, 0, 0, 16, 16, nullptr, nullptr,
        GetModuleHandleW(nullptr), nullptr);
    const HWND displayWindow = ownerDisplayWindow.load(std::memory_order_acquire);
    if (hostWindow) DestroyWindow(hostWindow);
    pass &= Check(ownerBlockedReady && displayWindow && IsWindow(displayWindow),
        "unowned_display_survives_host_destruction_while_owner_is_blocked");
    const auto failOpenStart = GetTickCount64();
    overlay::CloseInputAdmission(terminalAdmission, terminalInputBridge);
    const auto failOpenElapsed = GetTickCount64() - failOpenStart;
    pass &= Check(ownerBlockedReady && terminalAdmission.closed() &&
        !terminalAdmission.TryEnter() && !terminalInputBridge.active() &&
        failOpenElapsed < 100,
        "terminal_input_revocation_fails_open_while_owner_is_stalled");
    if (terminalAdmission.Leave() && callbackDrained)
        SetEvent(callbackDrained);
    if (allowOwnerExit) SetEvent(allowOwnerExit);
    stalledOwner.join();
    if (finalizerClassRegistered)
        UnregisterClassW(FinalizerFixtureClassName, GetModuleHandleW(nullptr));
    if (ownerBlocked) CloseHandle(ownerBlocked);
    if (allowOwnerExit) CloseHandle(allowOwnerExit);
    if (callbackDrained) CloseHandle(callbackDrained);
    pass &= Check(ownerReleasedResourceOnOwner.load(std::memory_order_acquire),
        "owner_finalizer_releases_hwnd_and_resource_on_owner_thread_after_drain");

    overlay::OwnerFinalizer retainedFinalizer;
    unsigned retainedFinalizerCalls{};
    const auto retainedOutcome = retainedFinalizer.Run([] {
            return overlay::OwnerFinalizationOutcome::Complete;
        }, [&]() -> overlay::OwnerFinalizationOutcome {
            ++retainedFinalizerCalls;
            throw 1;
        },
        [&] {
            ++retainedFinalizerCalls;
            return overlay::OwnerFinalizationOutcome::Complete;
        });
    const auto repeatedOutcome = retainedFinalizer.Run([] {
            return overlay::OwnerFinalizationOutcome::Complete;
        }, [&] {
            ++retainedFinalizerCalls;
            return overlay::OwnerFinalizationOutcome::Complete;
        });
    pass &= Check(retainedOutcome ==
            overlay::OwnerFinalizationOutcome::Unverified &&
        repeatedOutcome == overlay::OwnerFinalizationOutcome::Unverified &&
        retainedFinalizerCalls == 2,
        "cleanup_failure_is_retained_continues_steps_and_never_retries");
    overlay::InputOwnerHeartbeat ownerHeartbeat;
    pass &= Check(!ownerHeartbeat.IsFresh(1000, 750),
        "owner_heartbeat_unpublished_is_not_responsive");
    ownerHeartbeat.Publish(1000);
    pass &= Check(ownerHeartbeat.IsFresh(1750, 750) &&
        !ownerHeartbeat.IsFresh(1751, 750) &&
        !ownerHeartbeat.IsFresh(999, 750),
        "owner_heartbeat_lease_accepts_boundary_and_rejects_stale_or_future");
    ownerHeartbeat.Reset();
    pass &= Check(!ownerHeartbeat.IsFresh(1001, 750),
        "owner_heartbeat_reset_revokes_responsiveness");
    pass &= RunWin32CallbackDrainFixture();

    overlay::InputCallbackAdmissionGate racingAdmission;
    std::atomic<bool> startRace{};
    std::atomic<unsigned> admittedCount{};
    std::thread contender([&] {
        while (!startRace.load(std::memory_order_acquire))
            std::this_thread::yield();
        for (unsigned i = 0; i < 10000; ++i) {
            if (racingAdmission.TryEnter()) {
                admittedCount.fetch_add(1, std::memory_order_relaxed);
                racingAdmission.Leave();
            }
        }
    });
    startRace.store(true, std::memory_order_release);
    racingAdmission.Close();
    contender.join();
    pass &= Check(racingAdmission.closed() &&
        racingAdmission.inFlight() == 0 &&
        admittedCount.load(std::memory_order_acquire) <= 10000 &&
        !racingAdmission.TryEnter(),
        "concurrent_terminal_close_drains_and_prevents_future_admission");

    overlay::InputEventBridge terminalBridge;
    std::vector<overlay::InputEvent> terminalEvents;
    terminalBridge.Activate(1, 2);
    pass &= Check(terminalBridge.TryDeactivate() && !terminalBridge.active(),
        "nonblocking_deactivate_closes_admission");
    terminalBridge.Drain(terminalEvents);
    pass &= Check(terminalEvents.size() == 2 &&
        terminalEvents[0].kind == overlay::InputEventKind::Reset &&
        terminalEvents[1].kind == overlay::InputEventKind::Focus &&
        terminalEvents[1].wParam == FALSE,
        "try_deactivate_publishes_imgui_reset_and_focus_release");

    overlay::InputEventBridge saturatedBridge;
    std::vector<overlay::InputEvent> saturatedEvents;
    saturatedBridge.Activate(1, 2);
    bool allQueued = true;
    for (std::size_t i = 0; i < 253; ++i)
        allQueued &= saturatedBridge.PushKeyboardMessage(WM_KEYDOWN,
            static_cast<WPARAM>('A' + (i % 26)), 0);
    pass &= Check(allQueued && saturatedBridge.active(),
        "bridge_accepts_events_up_to_capacity");
    const bool overflowAccepted = saturatedBridge.PushKeyboardMessage(
        WM_KEYDOWN, VK_RETURN, 0);
    pass &= Check(!overflowAccepted && !saturatedBridge.active(),
        "bridge_saturation_fails_open_instead_of_claiming_dropped_input");
    saturatedBridge.Drain(saturatedEvents);
    pass &= Check(saturatedEvents.size() == 2 &&
        saturatedEvents[0].kind == overlay::InputEventKind::Reset &&
        saturatedEvents[1].kind == overlay::InputEventKind::Focus &&
        saturatedEvents[1].wParam == FALSE,
        "bridge_saturation_resets_stale_imgui_input_state");

    std::vector<overlay::InputEvent> inputEvents;
    pass &= Check(!bridge.PushRelativeMouseMotion(1, 1),
        "closed_input_owner_does_not_capture_events");
    overlay::InputEventBridge zeroDeltaBridge;
    zeroDeltaBridge.Activate(40, 50);
    pass &= Check(!zeroDeltaBridge.PushRelativeMouseMotion(0, 0) &&
        zeroDeltaBridge.PushAbsoluteMousePosition(60, 70),
        "zero_delta_raw_packet_does_not_claim_pointer_motion_ownership");
    pass &= Check(zeroDeltaBridge.PushRelativeMouseMotion(2, -1) &&
        !zeroDeltaBridge.PushAbsoluteMousePosition(80, 90),
        "real_relative_motion_selects_the_raw_pointer_stream");

    bridge.Activate(100, 200);
    pass &= Check(bridge.active() && bridge.PushAbsoluteMousePosition(150, 250) &&
        bridge.PushRelativeMouseMotion(4, -3) &&
        !bridge.PushAbsoluteMousePosition(300, 300),
        "overlay_input_owner_switches_to_relative_raw_mouse_source");
    pass &= Check(bridge.PushMouseButton(WM_LBUTTONDOWN, 0) &&
        bridge.PushMouseWheel(WM_MOUSEWHEEL, MAKEWPARAM(0, WHEEL_DELTA), 0) &&
        bridge.PushKeyboardMessage(WM_KEYDOWN, VK_F9, 0),
        "host-window_events_are_copied_into_the_owner-thread_bridge");
    bridge.Drain(inputEvents);
    pass &= Check(inputEvents.size() == 8 &&
        inputEvents[0].kind == overlay::InputEventKind::Reset &&
        inputEvents[1].kind == overlay::InputEventKind::AbsoluteMousePosition &&
        inputEvents[2].kind == overlay::InputEventKind::Focus &&
        inputEvents[2].wParam == TRUE &&
        inputEvents[3].kind == overlay::InputEventKind::AbsoluteMousePosition &&
        inputEvents[3].x == 150 && inputEvents[3].y == 250 &&
        inputEvents[4].kind == overlay::InputEventKind::RelativeMouseMotion &&
        inputEvents[4].x == 4 && inputEvents[4].y == -3 &&
        inputEvents[5].kind == overlay::InputEventKind::MouseButton &&
        inputEvents[6].kind == overlay::InputEventKind::MouseWheel &&
        inputEvents[7].kind == overlay::InputEventKind::NativeKeyboardMessage,
        "presenter_owner_receives_ordered_absolute_relative_and_click_events");
    bridge.Deactivate();
    pass &= Check(!bridge.active() && !bridge.PushMouseButton(WM_LBUTTONUP, 0),
        "closed_ui_returns_event_ownership_to_the_game");
    bridge.Drain(inputEvents);
    pass &= Check(inputEvents.size() == 2 &&
        inputEvents[0].kind == overlay::InputEventKind::Reset &&
        inputEvents[1].kind == overlay::InputEventKind::Focus &&
        inputEvents[1].wParam == FALSE,
        "ownership_transition_resets_pointer_and_keyboard_focus_on_presenter_thread");

    overlay::InputState focusInput;
    overlay::InputEventBridge focusBridge;
    focusInput.Toggle();
    focusInput.SetFocused(true);
    focusBridge.Activate(15, 25);
    pass &= Check(overlay::CaptureInputOwnership(focusInput, focusBridge).Coherent(),
        "focused_open_panel_owns_the_input_bridge");
    focusBridge.PushKeyboardMessage(WM_KEYDOWN, VK_SHIFT, 0);
    focusInput.SetFocused(false);
    focusBridge.Deactivate();
    focusBridge.Drain(inputEvents);
    pass &= Check(overlay::CaptureInputOwnership(focusInput, focusBridge).Coherent() &&
        inputEvents.size() == 2 &&
        inputEvents[0].kind == overlay::InputEventKind::Reset &&
        inputEvents[1].kind == overlay::InputEventKind::Focus &&
        inputEvents[1].wParam == FALSE,
        "focus_loss_clears_queued_keydown_and_releases_imgui_keyboard_state");
    focusInput.SetFocused(true);
    focusBridge.Activate(15, 25);
    focusBridge.Drain(inputEvents);
    pass &= Check(overlay::CaptureInputOwnership(focusInput, focusBridge).Coherent() &&
        inputEvents.back().kind == overlay::InputEventKind::Focus &&
        inputEvents.back().wParam == TRUE,
        "focus_regain_reestablishes_owner_thread_keyboard_input");
    overlay::VirtualCursorPosition cursor;
    cursor.SetAbsolute(150, -5, 100, 50);
    pass &= Check(cursor.known() && cursor.point().x == 99 && cursor.point().y == 0,
        "absolute_seed_is_clamped_to_the_game_client_extent");
    cursor.ApplyRelative(-8, 10, 100, 50);
    pass &= Check(cursor.point().x == 91 && cursor.point().y == 10,
        "raw_relative_delta_moves_the_presenter_owned_cursor");
    cursor.ApplyRelative(1000, -100, 100, 50);
    pass &= Check(cursor.point().x == 99 && cursor.point().y == 0,
        "raw_relative_motion_is_clamped_without_os_cursor_warping");
    cursor.Reset();
    cursor.ApplyRelative(3, -2, 100, 50);
    pass &= Check(cursor.point().x == 53 && cursor.point().y == 23,
        "relative_input_after_focus_reset_reseeds_from_client_center");
    pass &= Check(overlay::InputState::IsRebindMessage(WM_KEYDOWN) &&
        overlay::InputState::IsRebindMessage(WM_KEYUP) &&
        overlay::InputState::IsRebindMessage(WM_SYSKEYDOWN) &&
        overlay::InputState::IsRebindMessage(WM_SYSKEYUP) &&
        !overlay::InputState::IsRebindMessage(WM_MOUSEMOVE) &&
        !overlay::InputState::IsRebindMessage(WM_INPUT) &&
        !overlay::InputState::IsRebindMessage(WM_CHAR),
        "only_keyboard_message_classes_enter_rebind_path");
    pass &= Check(overlay::InputState::IsRebindKeyDownMessage(WM_KEYDOWN) &&
        overlay::InputState::IsRebindKeyDownMessage(WM_SYSKEYDOWN) &&
        !overlay::InputState::IsRebindKeyDownMessage(WM_KEYUP) &&
        !overlay::InputState::IsRebindKeyDownMessage(WM_SYSKEYUP),
        "modifier_queries_only_for_key_down_classes");
    input.BeginRebind(overlay::HotkeyAction::GameplayMode, VK_F9);
    pass &= Check(input.HandleRebindMessage(WM_MOUSEMOVE, 0) ==
        overlay::RebindMessageResult::PassThrough && input.rebindActive(),
        "irrelevant_message_passes_without_changing_rebind_state");
    input.CancelRebind();
    pass &= Check(input.HandleToggleMessage(WM_KEYUP, VK_DELETE) && input.visible(),
        "delete_opens_overlay_by_default");
    pass &= Check(input.ShouldConsumeEscapeFromGame(WM_KEYDOWN, VK_ESCAPE) &&
        input.ShouldConsumeEscapeFromGame(WM_KEYUP, VK_ESCAPE) &&
        !input.ShouldConsumeEscapeFromGame(WM_CHAR, VK_ESCAPE),
        "open_overlay_consumes_escape_keyboard_messages_from_game");
    pass &= Check(!input.ShouldDismissOnEscapeMessage(WM_KEYDOWN, VK_ESCAPE, false) &&
        input.visible() &&
        input.ShouldDismissOnEscapeMessage(WM_KEYUP, VK_ESCAPE, false) &&
        input.HandleToggleMessage(WM_KEYUP, VK_ESCAPE, VK_ESCAPE) && !input.visible(),
        "escape_dismisses_open_overlay_through_toggle_state_path");
    pass &= Check(!input.ShouldDismissOnEscapeMessage(WM_KEYDOWN, VK_ESCAPE, false) &&
        !input.visible() &&
        !input.ShouldDismissOnEscapeMessage(WM_KEYUP, VK_ESCAPE, false) &&
        !input.ShouldConsumeEscapeFromGame(WM_KEYDOWN, VK_ESCAPE) &&
        !input.ShouldConsumeEscapeFromGame(WM_KEYUP, VK_ESCAPE) &&
        !input.HandleToggleMessage(WM_KEYUP, VK_ESCAPE, 0) && !input.visible(),
        "escape_never_opens_closed_overlay");
    pass &= Check(input.HandleToggleMessage(WM_KEYUP, VK_DELETE) && input.visible() &&
        !input.ShouldDismissOnEscapeMessage(WM_KEYDOWN, VK_ESCAPE, true) &&
        input.ConsumeEscapePopupDismissRequest() &&
        !input.ConsumeEscapePopupDismissRequest() &&
        !input.ShouldDismissOnEscapeMessage(WM_KEYUP, VK_ESCAPE, false) &&
        input.visible(),
        "child_popup_requests_own_close_and_owns_escape_press_until_keyup");
    pass &= Check(!input.ShouldDismissOnEscapeMessage(WM_KEYUP, VK_ESCAPE, true) &&
        input.visible(),
        "currently_active_child_escape_consumer_has_priority");
    pass &= Check(input.HandleToggleMessage(WM_KEYUP, VK_DELETE) && !input.visible() &&
        input.HandleToggleMessage(WM_KEYUP, VK_DELETE) && input.visible(),
        "configured_toggle_still_closes_and_opens_after_escape_cases");
    input.SetFocused(true);
    pass &= Check(input.ShouldOwnInput(),
        "panel and host focus jointly establish Overlay input ownership");
    pass &= Check(input.ShouldCapture(WM_MOUSEMOVE) &&
        input.ShouldCapture(WM_LBUTTONDOWN) && input.ShouldCapture(WM_MOUSEWHEEL) &&
        !input.ShouldCapture(WM_INPUT), "visible_captures_mouse_only");
    pass &= Check(!input.ShouldCapture(WM_KEYDOWN) &&
        !input.ShouldCapture(WM_KEYUP) && !input.ShouldCapture(WM_CHAR) &&
        !input.ShouldCapture(WM_SYSKEYDOWN), "visible_forwards_keyboard");
    pass &= Check(input.HandleToggleMessage(WM_KEYUP, VK_DELETE), "toggle_message");
    pass &= Check(!input.visible() &&
        !input.ShouldCapture(WM_KEYDOWN) &&
        !input.ShouldCapture(WM_MOUSEMOVE) && !input.ShouldCapture(WM_INPUT),
        "hidden_preserves_game_input");
    pass &= Check(!input.HandleToggleMessage(WM_KEYDOWN, VK_DELETE),
        "keydown_does_not_toggle");
    pass &= Check(!input.HandleToggleMessage(WM_KEYUP, 'F'),
        "other_key_does_not_toggle");
    pass &= Check(input.HandleToggleMessage(WM_KEYUP, VK_DELETE) && input.visible() &&
        !input.ShouldCapture(WM_KEYDOWN) && input.ShouldCapture(WM_RBUTTONDOWN),
        "second_toggle_restores_mouse_only_overlay");
    input.BeginRebind(overlay::HotkeyAction::GameplayMode, VK_F9);
    pass &= Check(input.rebindActive() &&
        input.HandleRebindMessage(WM_KEYDOWN, VK_F24) ==
            overlay::RebindMessageResult::CapturedKey && input.rebindActive(),
        "extended_function_key_is_captured");
    input.CommitAcceptedRebind(VK_F24);
    pass &= Check(input.HandleRebindMessage(WM_KEYDOWN, 0xC1) ==
        overlay::RebindMessageResult::PassThrough,
        "undefined_virtual_key_is_not_captured");
    input.BeginRebind(overlay::HotkeyAction::GameplayMode, VK_F9);
    overlay::OwnerInputCommand modifierEvent{};
    modifierEvent.message = WM_KEYDOWN;
    modifierEvent.wParam = VK_F6;
    modifierEvent.rebindModifierDown = true;
    const bool modifierQueued = ownerCommands.TryPush(modifierEvent) &&
        ownerCommands.TryPop(queuedCommand);
    pass &= Check(modifierQueued && queuedCommand.rebindModifierDown &&
        input.HandleRebindMessage(queuedCommand.message, queuedCommand.wParam,
            queuedCommand.rebindModifierDown) ==
            overlay::RebindMessageResult::PassThrough && input.rebindActive(),
        "event_time_modifier_press_survives_queue_after_modifier_release");
    overlay::OwnerInputCommand plainKeyEvent{};
    plainKeyEvent.message = WM_KEYDOWN;
    plainKeyEvent.wParam = VK_F6;
    plainKeyEvent.rebindModifierDown = false;
    const bool plainQueued = ownerCommands.TryPush(plainKeyEvent) &&
        ownerCommands.TryPop(queuedCommand);
    pass &= Check(plainQueued && !queuedCommand.rebindModifierDown &&
        input.HandleRebindMessage(queuedCommand.message, queuedCommand.wParam,
            queuedCommand.rebindModifierDown) ==
        overlay::RebindMessageResult::CapturedKey && input.rebindActive(),
        "event_time_plain_key_remains_capturable_after_modifier_press");
    input.CommitAcceptedRebind(VK_F6);
    pass &= Check(!input.rebindActive() &&
        input.previousBinding() == VK_F6 &&
        input.HandleRebindMessage(WM_KEYUP, VK_F6) ==
            overlay::RebindMessageResult::Consumed &&
        input.HandleRebindMessage(WM_KEYDOWN, VK_F6) ==
            overlay::RebindMessageResult::PassThrough,
        "captured_press_consumed_next_press_passes");
    input.BeginRebind(overlay::HotkeyAction::DialogueZoom, VK_F12);
    input.BeginRebind(overlay::HotkeyAction::CinematicFov, VK_F11);
    pass &= Check(input.rebindActive() &&
        input.rebindAction() == overlay::HotkeyAction::CinematicFov &&
        input.previousBinding() == VK_F11,
        "only_latest_capture_is_active");
    pass &= Check(input.HandleRebindMessage(WM_KEYDOWN, VK_F10) ==
        overlay::RebindMessageResult::CapturedKey && input.rebindActive(),
        "conflicting_candidate_does_not_commit_binding");
    pass &= Check(input.HandleRebindMessage(WM_KEYUP, VK_F10) ==
        overlay::RebindMessageResult::Consumed,
        "conflicting_candidate_release_consumed");
    pass &= Check(input.HandleRebindMessage(WM_KEYDOWN, VK_ESCAPE) ==
        overlay::RebindMessageResult::Cancelled && !input.rebindActive() &&
        input.previousBinding() == VK_F11,
        "escape_cancels_without_deadlock_and_restores_last_accepted_binding");
    pass &= Check(input.HandleRebindMessage(WM_KEYUP, VK_ESCAPE) ==
        overlay::RebindMessageResult::Consumed && input.visible(),
        "rebind_escape_press_consumed_without_closing_overlay");
    input.BeginRebind(overlay::HotkeyAction::CinematicAspect, VK_F10);
    const auto toggleCandidate = input.HandleRebindMessage(WM_KEYDOWN, VK_DELETE);
    const auto toggleRelease = input.HandleRebindMessage(WM_KEYUP, VK_DELETE);
    pass &= Check(toggleCandidate == overlay::RebindMessageResult::CapturedKey &&
        toggleRelease == overlay::RebindMessageResult::Consumed &&
        input.rebindActive() && input.visible(),
        "toggle_key_candidate_release_is_consumed_before_toggle_dispatch");
    input.CancelRebind();
    input.BeginRebind(overlay::HotkeyAction::DialogueZoom, VK_F12);
    input.CancelRebind();
    pass &= Check(!input.rebindActive() && input.previousBinding() == VK_F12,
        "focus_loss_or_teardown_cancels_without_changing_binding");
    input.BeginRebind(overlay::HotkeyAction::DialogueZoom, VK_F12);
    input.CommitAcceptedRebind('K');
    input.BeginRebind(overlay::HotkeyAction::DialogueZoom, 'K');
    pass &= Check(input.HandleRebindMessage(WM_KEYDOWN, VK_ESCAPE) ==
        overlay::RebindMessageResult::Cancelled && input.previousBinding() == 'K',
        "later_cancel_restores_most_recent_accepted_binding");
    input.HandleToggleMessage(WM_KEYUP, VK_DELETE);
    input.BeginRebind(overlay::HotkeyAction::DialogueZoom, 'K');
    input.Close();
    pass &= Check(!input.visible() && !input.rebindActive() &&
        !input.ShouldCapture(WM_MOUSEMOVE) &&
        input.HandleRebindMessage(WM_KEYUP, 'K') ==
            overlay::RebindMessageResult::PassThrough,
        "terminal_close_releases_capture_and_rebind_state");
    std::cout << "Overlay input state harness: " << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
