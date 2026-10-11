// Characterize the actual private adapters, not copies of their algorithms.
// Viewport input and external Overlay wake delivery are substituted. Memory
// leaves call real operations and record order; one case refuses post-write readback.
#include "../../src/platform/win32/viewport.hpp"
#include "../../src/platform/win32/memory.hpp"
#include "../../src/gameplay/gameplay_camera.hpp"
#include "../../src/plugin/camera_writer_transaction.hpp"
#include <limits>
namespace {
    template <typename T>
    bool RuntimeAdapterFixtureZoomSafeRead(std::uintptr_t address, T& value);
    DWORD RuntimeAdapterFixtureInitializeThreadCall(void* parameter);
    void RuntimeAdapterFixtureInitializationFailureCleanup();
}
namespace platform::win32 {
    float RuntimeAdapterFixtureViewport();
    float RuntimeAdapterFixtureAutoViewport();
    bool RuntimeAdapterFixtureReadMemory(std::uintptr_t, void*, std::size_t);
}
namespace gameplay {
    bool RuntimeAdapterFixtureWriteAspectOnly(std::uintptr_t, std::uintptr_t,
        float, WritablePredicate);
}
#define ReadCurrentProcessClientViewportAspect RuntimeAdapterFixtureViewport
#define ReadClientViewportAspect RuntimeAdapterFixtureAutoViewport
#define NotifyOverlayCameraStatePublished RuntimeAdapterFixtureWake
#define ReadMemory RuntimeAdapterFixtureReadMemory
#define RUNTIME_ADAPTER_ZOOM_SAFE_READ(address, value) RuntimeAdapterFixtureZoomSafeRead((address), (value))
#define RUNTIME_ADAPTER_INITIALIZE_THREAD_CALL(parameter) RuntimeAdapterFixtureInitializeThreadCall((parameter))
#define RUNTIME_ADAPTER_INITIALIZATION_FAILURE_CLEANUP() RuntimeAdapterFixtureInitializationFailureCleanup()
#define WriteAspectOnly RuntimeAdapterFixtureWriteAspectOnly
#include "../../src/plugin/runtime.cpp"
#undef WriteAspectOnly
#undef RUNTIME_ADAPTER_ZOOM_SAFE_READ
#undef RUNTIME_ADAPTER_INITIALIZE_THREAD_CALL
#undef RUNTIME_ADAPTER_INITIALIZATION_FAILURE_CLEANUP
#undef ReadMemory
#undef NotifyOverlayCameraStatePublished
#undef ReadClientViewportAspect
#undef ReadCurrentProcessClientViewportAspect

#include <iostream>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <future>
#include <spdlog/sinks/base_sink.h>

plugin::detail::RuntimeState* RuntimeOwnerFromSeparateTranslationUnit() noexcept;

namespace {
    float fixtureViewport = 21.5f / 9.0f;
    unsigned wakes{};
    bool wakeHeldGate{};
    bool wakeSawPublication{};
    SafetyHookContext* currentContext{};
    float xmmAtWake{};
    float xmm1AtWake{};
    bool diagnosticMode{};
    std::vector<std::string> observedLogs;
    unsigned logCalls{};
    unsigned restorationLogCalls{};
    bool logHeldGate{};
    enum class NativeEffect { Read, WriteAspect, Wake };
    struct NativeEvent { NativeEffect effect; std::uintptr_t address; bool succeeded; };
    std::vector<NativeEvent> nativeEvents;
    bool aspectWriteSucceeded{};
    bool refuseReadback{};
    bool throwOnZoomSafeRead{};
    unsigned safetyHookVirtualProtectCalls{};
    unsigned safetyHookFailVirtualProtectAt{};
    bool safetyHookFailAfterVirtualProtect{};
    enum class StartupFault { Early, Partial };
    StartupFault startupFault{StartupFault::Early};
    bool startupCleanupThrows{};
    unsigned startupCleanupCalls{};
    HANDLE startupWorkerDone{};
    std::atomic<bool> startupWorkerEntryReturned{};
    struct SafetyHookAllocationRelease { void* address; std::size_t size; bool returned; };
    std::vector<SafetyHookAllocationRelease> safetyHookAllocationReleases;
    std::uintptr_t fixtureSource{};
    class ObservingSink : public spdlog::sinks::base_sink<std::mutex> {
        void sink_it_(const spdlog::details::log_msg& message) override {
            observedLogs.emplace_back(message.payload.data(), message.payload.size());
            ++logCalls;
            if (std::string_view(message.payload.data(), message.payload.size()).starts_with("RESTORATION_STATE "))
                ++restorationLogCalls;
            logHeldGate |= g_cameraCorrectionGate.IsHeldByCurrentThread();
        }
        void flush_() override {}
    };
    void Check(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }
    void Near(float a, float b, const char* message) {
        Check(std::isfinite(a) && std::fabs(a - b) < 0.0002f, message);
    }
    struct Memory {
        std::uint8_t* bytes = static_cast<std::uint8_t*>(VirtualAlloc(
            nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        Memory() { Check(bytes != nullptr, "VirtualAlloc"); std::memset(bytes, 0xA5, 4096); }
        ~Memory() { VirtualFree(bytes, 0, MEM_RELEASE); }
        std::uintptr_t Address() const { return reinterpret_cast<std::uintptr_t>(bytes); }
        template<typename T> void Put(std::size_t offset, T value) { std::memcpy(bytes + offset, &value, sizeof(value)); }
        template<typename T> T Get(std::size_t offset) const { T value{}; std::memcpy(&value, bytes + offset, sizeof(value)); return value; }
        void Protect(DWORD protection) { DWORD previous{}; Check(VirtualProtect(bytes, 4096, protection, &previous) != FALSE, "VirtualProtect"); }
    };
    SafetyHookContext Context(Memory& source, Memory& output, float fov = 90.0f) {
        SafetyHookContext context;
        std::memset(&context, 0x5A, sizeof(context));
        context.rsi = source.Address(); context.rbx = output.Address();
        context.rax = source.Address(); context.rcx = source.Address();
        context.rip = 0x123400; context.rflags = 0x202;
        context.xmm0.f32[0] = fov; context.xmm6.f32[0] = fov;
        return context;
    }
    void UnchangedContext(const SafetyHookContext& before, const SafetyHookContext& after,
        bool xmm0 = false, bool xmm1 = false, bool rip = false) {
        auto expected = before;
        if (xmm0) expected.xmm0.f32[0] = after.xmm0.f32[0];
        if (xmm1) expected.xmm1.f32[0] = after.xmm1.f32[0];
        if (rip) expected.rip = after.rip;
        Check(std::memcmp(&expected, &after, sizeof(expected)) == 0, "Unexpected register/lane modification");
        Check(!g_cameraCorrectionGate.IsHeldByCurrentThread(), "Callback retained correction gate");
        Check(&g_cameraCorrectionGate == &g_runtime.cameraCorrectionGate, "Gate owner differs");
        Check(&plugin::detail::GetRuntimeState() == &g_runtime,
            "Private runtime binding resolved a different RuntimeState owner");
        Check(RuntimeOwnerFromSeparateTranslationUnit() == &g_runtime,
            "Separate translation unit resolved a different RuntimeState owner");
        Check(&plugin::detail::GetRuntimeState().cameraCorrectionGate ==
            &g_cameraCorrectionGate, "Correction gate is not on the bound owner");
        Check(&plugin::detail::GetRuntimeState().hooks == &g_hooks,
            "HookSet is not on the bound owner");
        Check(&g_coordinator == &g_runtime.coordinator, "Coordinator owner differs");
    }
    void Setup(Memory& source) {
        diagnostics::SetEnabled(diagnosticMode);
        InitializeCinematicContinuationDiagnostics(diagnosticMode);
        Check((GetCinematicContinuationTrace() != nullptr) == diagnosticMode,
            "Cinematic continuation trace allocation did not follow startup Diagnostics gate");
        nativeEvents.reserve(1024);
        fixtureSource = source.Address();
        g_logger = std::make_shared<spdlog::logger>("runtime-adapter-fixture", std::make_shared<ObservingSink>());
        g_runtimeInitializationComplete = true;
        g_cameraWriterHookGate = true; g_cinematicHookGate = true;
        g_dialogueNonNativeCapabilityAvailable = true;
        g_dialogueBoundaryHookAvailable = true;
        g_runtimeGameplayMode = config::GameplayMode::HorPlus;
        g_runtimeCinematicPolicy = config::CinematicAspectPolicy::Forced21x9;
        source.Put(kAspectOffset, fixtureViewport);
        source.Put(kFlagsOffset, std::uint8_t{4});
        source.Put(kCameraWorldFovOffset, 90.0f);
        source.Put(0x234, 90.0f); source.Put(0x2C, 70.0f);
    }
    plugin::OverlaySemanticSnapshot ReadFacts() {
        plugin::OverlaySemanticSnapshot snapshot{};
        Check(ReadOverlaySemanticSnapshot(snapshot, nullptr), "Actual semantic reader refused");
        Check(snapshot.cameraStateValid, "No actual shared publication");
        return snapshot;
    }
    void Writer(SafetyHookContext& context) {
        currentContext = &context;
        SafeMidHookEntry<&ReplayManualTransition>(context);
        currentContext = nullptr;
    }
    void DialogueCallback(SafetyHookContext& context) {
        currentContext = &context;
        TraceDialogueBoundary(context);
        currentContext = nullptr;
    }
    template <typename T>
    bool RuntimeAdapterFixtureZoomSafeRead(std::uintptr_t address, T& value) {
        if (throwOnZoomSafeRead) {
            throwOnZoomSafeRead = false;
            throw std::runtime_error("injected Zoom memory-read exception");
        }
        return platform::win32::ReadMemory(address, &value, sizeof(value));
    }
    DWORD RuntimeAdapterFixtureInitializeThreadCall(void*) {
        if (startupFault == StartupFault::Partial) {
            g_runtimeInitializationComplete.store(true, std::memory_order_release);
            g_cameraWriterHookGate.store(true, std::memory_order_release);
            g_cinematicHookGate.store(true, std::memory_order_release);
            g_runtime.overlayStartupSucceeded.store(true, std::memory_order_release);
        }
        throw std::runtime_error(startupFault == StartupFault::Partial
            ? "injected partial startup failure" : "injected early startup failure");
    }
    void RuntimeAdapterFixtureInitializationFailureCleanup() {
        ++startupCleanupCalls;
        if (startupCleanupThrows)
            throw std::runtime_error("injected secondary cleanup failure");
        ResetAllRuntimeResources();
    }
    DWORD WINAPI RuntimeAdapterStartupFailureWorker(void*) {
        plugin::InitializeThread(nullptr);
        startupWorkerEntryReturned.store(true, std::memory_order_release);
        if (startupWorkerDone) SetEvent(startupWorkerDone);
        return 0;
    }
    void StartupFailure(const std::string& scenario) {
        startupFault = scenario == "startup-fatal-early"
            ? StartupFault::Early : StartupFault::Partial;
        startupCleanupThrows = scenario == "startup-fatal-cleanup-throws";
        startupCleanupCalls = 0;
        startupWorkerEntryReturned.store(false, std::memory_order_release);
        g_runtimeInitializationComplete.store(false, std::memory_order_release);
        g_stopping.store(false, std::memory_order_release);
        g_cameraWriterHookGate.store(false, std::memory_order_release);
        g_cinematicHookGate.store(false, std::memory_order_release);
        g_runtime.overlayStartupSucceeded.store(false, std::memory_order_release);

        if (scenario == "startup-fatal-retained-worker") {
            startupFault = StartupFault::Partial;
            startupWorkerDone = CreateEventW(nullptr, TRUE, FALSE, nullptr);
            Check(startupWorkerDone && CreateWorkerStopEvent() &&
                g_workers.Start(&RuntimeAdapterStartupFailureWorker),
                "Retained startup-worker fixture setup failed");
            Check(WaitForSingleObject(startupWorkerDone, 3000) == WAIT_OBJECT_0,
                "Fatal initialization worker timed out");
            Check(startupWorkerEntryReturned.load(std::memory_order_acquire) &&
                g_workers.WorkerCount() == 1 && g_stopping &&
                !g_runtimeInitializationComplete && !g_cameraWriterHookGate &&
                !g_cinematicHookGate && !g_runtime.overlayStartupSucceeded &&
                startupCleanupCalls == 1,
                "Failed self-join lost worker ownership or repeated cleanup");
            ResetAllRuntimeResources();
            Check(g_workers.WorkerCount() == 0, "Retained startup worker was not joined");
            CloseHandle(startupWorkerDone);
            startupWorkerDone = nullptr;
            return;
        }

        if (scenario == "startup-fatal-cleanup-throws") startupCleanupThrows = true;
        const DWORD result = plugin::InitializeThread(nullptr);
        Check(result == ERROR_UNHANDLED_EXCEPTION && startupCleanupCalls == 1 &&
            g_stopping && !g_runtimeInitializationComplete && !g_cameraWriterHookGate &&
            !g_cinematicHookGate && !g_runtime.overlayStartupSucceeded,
            "Fatal thread exception escaped containment or left readiness open");
        if (scenario == "startup-fatal-cleanup-throws")
            Check(g_runtimeInitializationComplete == false,
                "Secondary cleanup exception reopened initialization readiness");
    }
    void ZoomInCallback(SafetyHookContext& context) {
        SafeMidHookEntry<&TraceZoomIn>(context);
    }
    void ZoomOutCallback(SafetyHookContext& context) {
        SafeMidHookEntry<&TraceZoomOut>(context);
    }
    void Gameplay(const std::string& scenario) {
        Memory source, output; Setup(source);
        if (scenario == "writer-native") { fixtureViewport = kNativeAspect; source.Put(kAspectOffset, kNativeAspect); }
        if (scenario == "writer-combine" || scenario == "writer-combine-readonly" || scenario == "writer-combine-invalid-viewport" || scenario == "writer-combine-readback-failure") {
            g_runtimeGameplayMode = config::GameplayMode::RuntimeCombine;
            source.Put(kAspectOffset, kNativeAspect);
        }
        if (scenario == "writer-aspect") g_runtimeGameplayMode = config::GameplayMode::AspectRecalculation;
        if (scenario == "writer-off") g_runtimeGameplayEnabled = false;
        if (scenario == "writer-gate-off") g_cameraWriterHookGate = false;
        if (scenario == "writer-stopping") g_stopping = true;
        if (scenario == "writer-invalid-fov") source.Put(kCameraWorldFovOffset, std::numeric_limits<float>::quiet_NaN());
        if (scenario == "writer-combine-invalid-viewport") fixtureViewport = std::numeric_limits<float>::quiet_NaN();
        if (scenario == "writer-combine-readback-failure") refuseReadback = true;
        if (scenario == "writer-combine-readonly") source.Protect(PAGE_READONLY);
        if (scenario == "writer-unreadable") source.Protect(PAGE_NOACCESS);
        auto context = Context(source, output);
        if (scenario == "writer-invalid-fov") context.xmm0.f32[0] = std::numeric_limits<float>::quiet_NaN();
        const auto before = context;
        std::array<std::uint8_t, 4096> outputBefore{}; std::memcpy(outputBefore.data(), output.bytes, 4096);
        std::array<std::uint8_t, 4096> sourceBefore{};
        if (scenario == "writer-unreadable") source.Protect(PAGE_READWRITE);
        std::memcpy(sourceBefore.data(), source.bytes, 4096);
        if (scenario == "writer-unreadable") source.Protect(PAGE_NOACCESS);
        Writer(context); UnchangedContext(before, context, true);
        if (scenario == "writer-unreadable") source.Protect(PAGE_READWRITE);
        for (std::size_t offset=0; offset<4096; ++offset) {
            if ((offset >= kAspectOffset && offset < kAspectOffset + sizeof(float)) || offset == kFlagsOffset) continue;
            Check(source.bytes[offset] == sourceBefore[offset], "Unexpected source-memory write outside aspect/flags");
        }
        Check(std::memcmp(outputBefore.data(), output.bytes, 4096) == 0, "Adapter wrote native RBX output memory");
        Check(g_coordinator == CoordinatorState::Gameplay, "Gameplay ownership changed");
        if (scenario == "writer-wide" || scenario == "writer-combine") {
            const auto expected = camera::HorPlus(90.0f, fixtureViewport, kNativeAspect);
            Near(context.xmm0.f32[0], expected, "Actual writer transform");
            const auto facts = ReadFacts();
            Near(facts.cameraState.evidence.currentOutputFov.value, expected, "Actual output publication");
            Check(wakes > 0 && wakeHeldGate && wakeSawPublication, "Publication/wake/gate order");
            Near(xmmAtWake, 90.0f, "Writer publication must precede XMM0 update");
            Check(facts.cameraState.evidence.nativeFov.valid, "Native role missing");
        } else if (scenario == "writer-invalid-fov") Check(std::isnan(context.xmm0.f32[0]), "Invalid FOV transformed");
        else Near(context.xmm0.f32[0], 90.0f, "Expected native pass-through");
        if (scenario == "writer-combine") {
            Near(source.Get<float>(kAspectOffset), fixtureViewport, "RuntimeCombine aspect sync");
            Check(source.Get<std::uint8_t>(kFlagsOffset) == 4, "RuntimeCombine changed flags");
            const auto telemetry = ReadFacts().cameraState.evidence;
            Check(telemetry.runtimeCombineWriteAttempted && telemetry.runtimeCombineWriteSucceeded &&
                telemetry.runtimeCombineReadbackValid && telemetry.runtimeCombineConfirmed && telemetry.runtimeCombineFovApplied,
                "Actual confirmed RuntimeCombine telemetry");
            const auto write = std::find_if(nativeEvents.begin(), nativeEvents.end(), [](auto event) {
                return event.effect == NativeEffect::WriteAspect && event.succeeded;
            });
            Check(write != nativeEvents.end(), "No actual native aspect write");
            const auto readback = std::find_if(write+1, nativeEvents.end(), [&](auto event) {
                return event.effect == NativeEffect::Read && event.address == source.Address() + kAspectOffset && event.succeeded;
            });
            Check(readback != nativeEvents.end(), "Missing readback after actual write");
            const auto wake = std::find_if(readback+1, nativeEvents.end(), [](auto event) { return event.effect == NativeEffect::Wake; });
            Check(wake != nativeEvents.end(), "Publication wake preceded write/readback");
        }
        if (scenario == "writer-combine-readback-failure") {
            Check(aspectWriteSucceeded, "Readback failure case never wrote actual aspect");
            Near(source.Get<float>(kAspectOffset), fixtureViewport, "Successful write not retained after failed readback");
            Check(!ReadFacts().cameraState.evidence.runtimeCombineConfirmed, "Failed readback treated as confirmed");
        }
        if (scenario == "writer-combine-readonly") Near(source.Get<float>(kAspectOffset), kNativeAspect, "Refused write changed aspect");
        if (scenario == "writer-gate-off" || scenario == "writer-stopping" || scenario == "writer-unreadable") Check(!g_cameraStateSnapshotValid, "Rejected callback published");
    }
    void Aspect(const std::string& scenario) {
        Memory source, output; Setup(source);
        if (scenario == "aspect-off") g_runtimeCinematicsEnabled = false;
        auto enter = Context(source, output); TraceCinematicEnter(enter);
        if (scenario == "aspect-readonly") source.Protect(PAGE_READONLY);
        if (scenario == "aspect-unreadable") source.Protect(PAGE_NOACCESS);
        auto context = Context(source, output); const auto before = context;
        ApplyCinematicAspectStore(context); UnchangedContext(before, context, false, false, true);
        Check(context.rip == before.rip + 10, "Cinematic replaced-store RIP adjustment");
        if (scenario == "aspect-off") Near(source.Get<float>(kAspectOffset), kNativeAspect, "OFF native aspect store");
        if (scenario == "aspect-wide") Near(source.Get<float>(kAspectOffset), cinematics::Forced21x9Aspect, "Configured aspect store");
        if (scenario == "aspect-readonly") Near(source.Get<float>(kAspectOffset), fixtureViewport, "Read-only aspect changed");
    }
    void Lifecycle(const std::string& scenario) {
        Memory source, output; Setup(source);
        g_gameplayRecoveryObservationAvailable = true;
        auto writer = Context(source, output); Writer(writer);
        if (scenario == "lifecycle-off") g_runtimeCinematicsEnabled = false;
        if (scenario == "lifecycle-gate-off") g_cinematicHookGate = false;
        auto context = Context(source, output); auto before = context;
        TraceCinematicEnter(context); UnchangedContext(before, context, true);
        if (scenario == "lifecycle-gate-off") {
            Near(context.xmm0.f32[0], 90.0f, "Disabled ENTER mutated");
            Check(g_coordinator == CoordinatorState::Gameplay && g_cinematicSelectionGeneration == 0, "Disabled ENTER accepted"); return;
        }
        Check(g_coordinator == CoordinatorState::CinematicActive && g_cinematicSelectionGeneration == 1, "ENTER owner/generation");
        if (scenario != "lifecycle-off") Near(context.xmm0.f32[0], camera::HorPlus(90.0f, cinematics::Forced21x9Aspect, kNativeAspect), "Actual ENTER XMM0 target");
        const auto first = ReadFacts();
        Check(first.cameraTransitionBoundary.value.cinematicValid, "Actual ENTER evidence missing");
        const auto generation = g_cinematicSelectionGeneration.load();
        before = context; TraceCinematicEnter(context); UnchangedContext(before, context);
        Check(g_cinematicSelectionGeneration == generation, "Duplicate ENTER accepted");
        if (scenario == "lifecycle-foreign-association") {
            auto firstStore = Context(source, output);
            firstStore.rax = source.Address();
            ApplyCinematicAspectStore(firstStore);
            auto conflictingStore = Context(source, output);
            conflictingStore.rax = output.Address();
            ApplyCinematicAspectStore(conflictingStore);
            const auto association = g_cinematicWriterAssociation.Read();
            Check(association.associationConflict && !association.associationValid &&
                association.associatedWriterRsi == 0,
                "Actual aspect-store target conflict did not retire initial-establishment association");
            auto foreignWriter = Context(source, output);
            foreignWriter.rsi = output.Address();
            const auto foreignBefore = foreignWriter;
            Writer(foreignWriter);
            UnchangedContext(foreignBefore, foreignWriter, true);
            Check(g_cinematicWriterAssociation.Read().associationConflict &&
                !g_cinematicWriterAssociation.Read().associationValid,
                "Foreign writer revived conflicted association");
            return;
        }
        if (scenario == "lifecycle-off") Near(context.xmm0.f32[0], 90.0f, "OFF ENTER modified FOV");
        if (scenario == "lifecycle-chain") {
            ApplyCinematicAspectStore(context);
            auto sample = Context(source, output); Writer(sample);
        }
        context.xmm0.f32[0] = 90.0f; // Native EXIT operand, not transformed ENTER output.
        before = context; TraceCinematicExit(context); UnchangedContext(before, context);
        Check(g_coordinator != CoordinatorState::CinematicActive, "EXIT did not hand off");
        Check(ReadFacts().cameraTransitionBoundary.value.cinematicValid, "EXIT lost validated evidence");
        if (scenario == "lifecycle-chain") {
            for (unsigned expected = 2; expected <= 4; ++expected) {
                Check(g_coordinator == CoordinatorState::CinematicExiting &&
                    (g_cinematicContinuationPending || g_cinematicContinuationCarryPending),
                    "Fixture did not establish validated continuation");
                // Match the supplied native EXIT operand. The old Gameplay
                // output is a different FOV intent under Forced21x9 and would
                // correctly fail the production continuation predicate.
                context = Context(source, output, 90.0f);
                before = context; TraceCinematicEnter(context); UnchangedContext(before, context, true);
                Check(g_cinematicSelectionGeneration == expected && g_coordinator == CoordinatorState::CinematicActive, "Continuation generation/owner");
                Check(ReadFacts().cameraTransitionBoundary.value.cinematicValid, "Continuation evidence lost");
                Check(ReadFacts().cameraTransitionBoundary.value.lifecycleGeneration == expected, "Evidence generation differs");
                Check(ReadFacts().cameraTransitionBoundary.value.gameplayValid, "Continuation lost original Gameplay operand");
                if (expected != 3) {
                    ApplyCinematicAspectStore(context);
                    auto sample = Context(source, output, g_cinematicTransformedFov.load());
                    Writer(sample);
                }
                context.xmm0.f32[0] = 90.0f;
                before = context; TraceCinematicExit(context); UnchangedContext(before, context);
            }
        }
    }
    void RestorationLogging() {
        Memory source, output; Setup(source);
        g_runtimeGameplayMode = config::GameplayMode::RuntimeCombine;
        auto first = Context(source, output);
        Writer(first);
        const auto initial = g_gameplayAspectRestorationStore.Read();
        Check(initial.valid, "First restoration observation missing");
        Check(restorationLogCalls == (diagnosticMode ? 1u : 0u),
            "First restoration change did not follow diagnostics policy");
        for (unsigned i = 0; i < 64; ++i) {
            auto sample = Context(source, output);
            const auto before = sample;
            Writer(sample);
            UnchangedContext(before, sample, true);
        }
        const auto repeated = g_gameplayAspectRestorationStore.Read();
        Check(repeated.valid && repeated.observationSequence == initial.observationSequence + 64,
            "Deduplication skipped restoration observations");
        Near(repeated.aspect, initial.aspect, "Repeated restoration aspect changed");
        Check(restorationLogCalls == (diagnosticMode ? 1u : 0u),
            "Unchanged restoration samples emitted duplicate logs");
        fixtureViewport = 3.0f;
        auto changed = Context(source, output);
        Writer(changed);
        Near(source.Get<float>(kAspectOffset), fixtureViewport,
            "Deduplication suppressed RuntimeCombine aspect synchronization");
        Check(restorationLogCalls == (diagnosticMode ? 2u : 0u),
            "Real restoration change was not logged");
        auto unchanged = Context(source, output);
        Writer(unchanged);
        Check(restorationLogCalls == (diagnosticMode ? 2u : 0u),
            "Changed restoration state continued flooding logs");
    }
    void CinematicGameplayBaselineContinuation(const std::string& scenario) {
        Memory source, output, foreignSource, foreignOutput;
        Setup(source);
        g_gameplayRecoveryObservationAvailable = true;
        fixtureViewport = 3.0f;
        g_runtimeGameplayMode = scenario == "cinematic-continuation-combine"
            ? config::GameplayMode::RuntimeCombine : config::GameplayMode::HorPlus;
        g_runtimeCinematicPolicy = config::CinematicAspectPolicy::Auto;
        g_runtimeCinematicFovMode = config::CinematicFovMode::GameplayHorPlus;
        g_runtimeDialoguePolicy = config::DialogueZoomPolicy::Disabled;
        source.Put(kAspectOffset, fixtureViewport);
        source.Put(kCameraWorldFovOffset, 95.1311f);
        for (unsigned i = 0; i < 4; ++i) {
            auto sample = Context(source, output, 95.1311f);
            Writer(sample);
        }
        auto enter = Context(source, output, 90.0f);
        TraceCinematicEnter(enter);
        ApplyCinematicAspectStore(enter);
        const float target = g_cinematicTransformedFov.load();
        Near(target, camera::HorPlus(95.1311f, fixtureViewport, kNativeAspect),
            "Continuation fixture target");
        auto initial = Context(source, output, 95.1311f);
        Writer(initial);
        Near(initial.xmm0.f32[0], target, "Generation 1 establishment regressed");
        const bool invalidExit = scenario == "cinematic-continuation-no-validated-exit";
        if (invalidExit) g_cinematicWriterAssociation.InvalidateGeneration(1);
        auto exit = Context(source, output,
            scenario == "cinematic-continuation-exit-departure" ? 95.2186f : 95.1311f);
        TraceCinematicExit(exit);
        Check(g_cinematicContinuationPending == !invalidExit &&
            g_coordinator == CoordinatorState::CinematicExiting,
            "Generation 1 EXIT did not capture validated continuation");
        if (scenario == "cinematic-continuation-invalid-baseline")
            g_gameplayBaselineStore.Invalidate();
        if (scenario == "cinematic-continuation-native-mode")
            g_runtimeCinematicFovMode = config::CinematicFovMode::NativeHorPlus;
        if (scenario == "cinematic-continuation-target-change")
            fixtureViewport = kNativeAspect;
        if (scenario == "cinematic-continuation-policy-change")
            Check(ApplyRuntimeSetting(plugin::RuntimeSettingMutation::CinematicAspect(
                config::CinematicAspectPolicy::Forced16x9), nullptr).accepted,
                "Production Cinematic policy handler rejected change");
        enter = Context(source, output, 90.0f);
        TraceCinematicEnter(enter);
        ApplyCinematicAspectStore(enter);
        Check(g_cinematicSelectionGeneration == 2 && g_coordinator == CoordinatorState::CinematicActive,
            "Generation 2 ENTER owner/generation");
        if (scenario == "cinematic-continuation-policy-change") {
            const float expected = camera::HorPlus(95.1311f, kNativeAspect, kNativeAspect);
            Near(g_cinematicTransformedFov.load(), expected,
                "ENTER used the previous active aspect policy instead of the selected policy");
            auto selectedPolicyWriter = Context(source, output, 95.1311f);
            Writer(selectedPolicyWriter);
            Near(selectedPolicyWriter.xmm0.f32[0], expected,
                "Production writer output did not use the newly selected aspect policy");
            Near(ReadFacts().cameraState.evidence.currentOutputFov.value, expected,
                "Published writer output differs from selected-policy output");
        }
        if (scenario == "cinematic-continuation-invalid-baseline" ||
            scenario == "cinematic-continuation-native-mode" ||
            scenario == "cinematic-continuation-target-change" ||
            scenario == "cinematic-continuation-policy-change" || invalidExit ||
            scenario == "cinematic-continuation-exit-departure") {
            Check(!g_cinematicContinuationInterpolationActive,
                "Unconfirmed baseline or changed intent armed continuation");
            auto rejected = Context(source, output, 95.2186f);
            Writer(rejected);
            Near(rejected.xmm0.f32[0], 95.2186f,
                "Unconfirmed baseline or changed intent retained old target");
            return;
        }
        auto first = Context(source, output, 95.1311f);
        Writer(first);
        Near(first.xmm0.f32[0], target, "Generation 2 initial writer regressed");
        float incoming = 95.2186f;
        Memory* writerSource = &source;
        Memory* writerOutput = &output;
        if (scenario == "cinematic-continuation-foreign-source") {
            foreignSource.Put(kAspectOffset, fixtureViewport);
            foreignSource.Put(kFlagsOffset, std::uint8_t{5});
            foreignSource.Put(kCameraWorldFovOffset, incoming);
            writerSource = &foreignSource;
        }
        if (scenario == "cinematic-continuation-foreign-writer") writerOutput = &foreignOutput;
        if (scenario == "cinematic-continuation-stale-generation") g_cinematicSelectionGeneration = 3;
        if (scenario == "cinematic-continuation-no-association") {
            // Reproduce a writer arriving before the generation's aspect-store association.
            g_cinematicWriterAssociation.BeginGeneration(2, 90.0f, target,
                {true, config::CinematicAspectPolicy::Auto, config::CinematicFovMode::GameplayHorPlus},
                95.1311f, source.Address());
        }
        if (scenario == "cinematic-continuation-outside-low") incoming = 95.0f;
        if (scenario == "cinematic-continuation-outside-high") incoming = target + 0.1f;
        if (scenario == "cinematic-continuation-correction-off") g_activeCinematicsEnabled = false;
        const bool accepted = scenario == "cinematic-continuation-baseline" ||
            scenario == "cinematic-continuation-combine" || scenario == "cinematic-continuation-chain";
        auto sample = Context(*writerSource, *writerOutput, incoming);
        auto before = sample;
        Writer(sample);
        UnchangedContext(before, sample, true);
        Near(sample.xmm0.f32[0], accepted ? target : incoming,
            "Gameplay-baseline continuation output/negative guard");
        Near(ReadFacts().cameraState.evidence.currentOutputFov.value,
            accepted ? target : incoming, "Continuation output publication differs from register");
        Check(g_coordinator == CoordinatorState::CinematicActive,
            "Continuation decision changed camera ownership");
        if (!accepted) return;
        for (float fov : {95.1534f, 95.2186f, 100.0f, 120.0f, target}) {
            sample = Context(source, output, fov);
            Writer(sample);
            Near(sample.xmm0.f32[0], target, "Validated continuation ramp lost target");
        }
        if (scenario == "cinematic-continuation-chain") {
            exit = Context(source, output, 95.1311f);
            TraceCinematicExit(exit);
            enter = Context(source, output, 90.0f);
            TraceCinematicEnter(enter); // Generation 3 is superseded without a writer.
            exit = Context(source, output, 95.1311f);
            TraceCinematicExit(exit);
            enter = Context(source, output, 90.0f);
            TraceCinematicEnter(enter);
            ApplyCinematicAspectStore(enter);
            Check(g_cinematicSelectionGeneration == 4, "Empty-generation chain lost generation");
            sample = Context(source, output, 95.2186f);
            Writer(sample);
            Near(sample.xmm0.f32[0], target, "Empty-generation continuation lost target");
        }
        auto dialogue = Context(source, output, 70.0f);
        before = dialogue;
        DialogueCallback(dialogue);
        UnchangedContext(before, dialogue);
        Check(g_coordinator == CoordinatorState::CinematicActive,
            "Disabled Dialogue took Cinematic ownership");
        sample = Context(source, output, 100.0f);
        Writer(sample);
        Near(sample.xmm0.f32[0], target, "Disabled Dialogue broke Cinematic continuation");
        exit = Context(source, output, 95.1311f);
        before = exit;
        TraceCinematicExit(exit);
        UnchangedContext(before, exit);
        Check(g_coordinator == CoordinatorState::CinematicExiting,
            "Final EXIT lost recovery ownership");
        sample = Context(source, output, 95.1311f);
        Writer(sample);
        Near(sample.xmm0.f32[0], camera::HorPlus(95.1311f, fixtureViewport, kNativeAspect),
            "Recovery did not resume Gameplay HorPlus");
        Check(g_coordinator == CoordinatorState::Gameplay,
            "Validated native recovery did not complete");
        Near(ReadFacts().cameraState.evidence.nativeFov.value, 95.1311f,
            "Recovery contaminated the native Gameplay FOV");
    }
    void CinematicInitialGameplayBaseline(const std::string& scenario) {
        Memory source, output, foreignSource;
        Setup(source);
        fixtureViewport = 3.0f;
        g_runtimeGameplayMode = config::GameplayMode::HorPlus;
        g_runtimeCinematicPolicy = config::CinematicAspectPolicy::Auto;
        g_runtimeCinematicFovMode = config::CinematicFovMode::GameplayHorPlus;
        source.Put(kAspectOffset, fixtureViewport);
        source.Put(kCameraWorldFovOffset, 95.1311f);

        const bool noBaseline = scenario == "cinematic-initial-missing-baseline";
        const bool invalidBaseline = scenario == "cinematic-initial-invalid-baseline";
        if (!noBaseline) {
            if (invalidBaseline)
                source.Put(kCameraWorldFovOffset,
                    std::numeric_limits<float>::quiet_NaN());
            for (unsigned i = 0; i < 4; ++i) {
                auto sample = Context(source, output,
                    invalidBaseline ? std::numeric_limits<float>::quiet_NaN() : 95.1311f);
                Writer(sample);
            }
            if (!invalidBaseline) {
                const auto baseline = g_gameplayBaselineStore.Read();
                Check(camera::IsUsableGameplayBaseline(baseline) &&
                    baseline.source.valid && baseline.source.value == source.Address(),
                    "Production Gameplay baseline fixture was not qualified for this source");
                Near(baseline.nativeFov.value, 95.1311f,
                    "Production baseline fixture native FOV");
            }
        }
        source.Put(kCameraWorldFovOffset, 95.1311f);

        auto enter = Context(source, output, 90.0f);
        auto before = enter;
        TraceCinematicEnter(enter);
        UnchangedContext(before, enter, true);
        Check(g_coordinator == CoordinatorState::CinematicActive &&
            g_cinematicSelectionGeneration == 1,
            "Initial Gameplay-baseline ENTER did not accept generation 1");
        const float target = g_cinematicTransformedFov.load();
        const float expectedTarget = noBaseline || invalidBaseline
            ? camera::HorPlus(90.0f, fixtureViewport, kNativeAspect)
            : camera::HorPlus(95.1311f, fixtureViewport, kNativeAspect);
        Near(target, expectedTarget, "Initial Gameplay-baseline cinematic target");
        ApplyCinematicAspectStore(enter);

        if (scenario == "cinematic-initial-foreign-source") {
            foreignSource.Put(kAspectOffset, fixtureViewport);
            foreignSource.Put(kFlagsOffset, std::uint8_t{5});
            foreignSource.Put(kCameraWorldFovOffset, 95.1311f);
            auto foreign = Context(foreignSource, output, 95.1311f);
            before = foreign;
            Writer(foreign);
            UnchangedContext(before, foreign, true);
            Near(foreign.xmm0.f32[0], 95.1311f,
                "Foreign writer received initial target");
            Check(!g_cinematicWriterAssociation.Read().associationValid,
                "Foreign writer established the accepted ENTER association");
            return;
        }

        if (scenario == "cinematic-initial-stale-generation")
            g_cinematicSelectionGeneration.store(2, std::memory_order_release);

        auto firstWriter = Context(source, output, 95.1311f);
        before = firstWriter;
        Writer(firstWriter);
        UnchangedContext(before, firstWriter, true);
        const bool accepted = scenario == "cinematic-initial-gameplay-baseline";
        if (accepted) {
            Near(firstWriter.xmm0.f32[0], target,
                "Confirmed Gameplay baseline was not replaced by cinematic target");
            Near(ReadFacts().cameraState.evidence.currentOutputFov.value, target,
                "Production output publication missed initial cinematic target");
            const auto association = g_cinematicWriterAssociation.Read();
            Check(association.initialEstablishmentActive &&
                association.associationValid && association.generation == 1,
                "Accepted Gameplay baseline damaged generation-scoped establishment");
        } else {
            Near(firstWriter.xmm0.f32[0], 95.1311f,
                "Unqualified Gameplay baseline was mutated");
            const auto association = g_cinematicWriterAssociation.Read();
            if (scenario == "cinematic-initial-stale-generation")
                Check(association.initialEstablishmentActive &&
                    association.generation == 1,
                    "Stale generation altered the active generation's establishment");
            else
                Check(!association.initialEstablishmentActive &&
                    association.initialEstablishmentEndReason ==
                        cinematics::InitialEstablishmentEndReason::AuthoredBaselineDeparture,
                    "Missing/invalid Gameplay baseline did not fail closed");
        }
    }
    void Dialogue(const std::string& scenario) {
        Memory source, output; Setup(source);
        if (scenario == "dialogue-off") g_runtimeDialogueEnabled = false;
        if (scenario == "dialogue-cinematic") g_coordinator = CoordinatorState::CinematicActive;
        if (scenario == "dialogue-unreadable") source.Protect(PAGE_NOACCESS);
        float finalOutput{};
        for (const float incoming : {90.0f, 88.0f, 85.0f, 80.0f, 75.0f, 70.0f}) {
            auto context = Context(source, output); context.xmm6.f32[0] = incoming;
            const auto before = context; DialogueCallback(context);
            UnchangedContext(before, context, false, true);
            finalOutput = context.xmm1.f32[0];
            if (scenario != "dialogue-active") Near(context.xmm1.f32[0], before.xmm1.f32[0], "Dialogue refusal mutated XMM1");
        }
        if (scenario == "dialogue-active") {
            Check(g_dialoguePhase == Phase::Active, "Actual Dialogue descent not accepted");
            Near(finalOutput, 70.0f, "Adaptive endpoint from 90 baseline must be 70");
            Near(xmm1AtWake, finalOutput,
                "Dialogue publication wake did not observe post-transform XMM1");
            const auto facts = ReadFacts();
            Near(facts.cameraState.evidence.currentOutputFov.value, finalOutput, "Dialogue output publication");
        }
        Check(g_coordinator == (scenario == "dialogue-cinematic" ? CoordinatorState::CinematicActive : CoordinatorState::Gameplay), "Dialogue changed coordinator");
    }
    void Zoom(const std::string& scenario) {
        Memory source, output; Setup(source);
        source.Put(0x4C, 70.0f); source.Put(0x50, 90.0f); source.Put(0x138, 1.0f); source.Put(0x13C, 2.0f);
        if (scenario == "zoom-unreadable") source.Protect(PAGE_NOACCESS);
        const bool throwingIn = scenario == "zoom-in-throw";
        const bool throwingOut = scenario == "zoom-out-throw";
        throwOnZoomSafeRead = throwingIn;
        auto context = Context(source, output); auto before = context;
        ZoomInCallback(context); UnchangedContext(before, context);
        Check(g_snapshotZoomActive && g_snapshotZoomHold &&
            g_zoomInHits == (diagnosticMode ? 1u : 0u), "Zoom IN observation/diagnostic counter gate");
        Check(g_runtime.snapshotZoomObservationValid ==
            (scenario != "zoom-unreadable" && !throwingIn), "Zoom read validity");
        if (throwingIn) Check(g_runtime.snapshotZoomSequence.load() == 0,
            "Zoom exception unexpectedly published post-read sequence");
        throwOnZoomSafeRead = throwingOut;
        ZoomOutCallback(context); UnchangedContext(before, context);
        Check(!g_snapshotZoomActive &&
            g_zoomOutHits == (diagnosticMode ? 1u : 0u), "Zoom OUT observation/diagnostic counter gate");
        if (throwingOut) Check(!g_runtime.snapshotZoomObservationValid,
            "Zoom OUT exception unexpectedly validated its observation");
    }
    void SettingsAndWake() {
        Memory source, output; Setup(source);
        auto context = Context(source, output);
        for (int sample=0; sample<4; ++sample) { context.xmm0.f32[0] = 90.0f; Writer(context); }
        const auto before = ReadFacts();
        Check(before.cameraState.evidence.gameplayFov.valid, "Qualified Gameplay role invalid");
        const auto stableWakes = wakes;
        context.xmm0.f32[0] = 90.0f; Writer(context);
        Check(wakes == stableWakes, "Unchanged writer woke Overlay");
        MarkCameraStateFreshness({camera::CameraStateFreshnessGroup::GameplayFov});
        Check(wakes > stableWakes, "Freshness-only mutation did not wake");
        const auto historical = ReadFacts();
        Near(historical.cameraState.evidence.gameplayFov.value, before.cameraState.evidence.gameplayFov.value, "Freshness changed value");
        // Gameplay baseline is retained, not a historical writer fact.
        Check(historical.cameraState.evidence.gameplayFov.freshness == camera::SnapshotFreshness::Retained, "Retained baseline metadata not read back");
        const auto priorEpoch = g_dialogueSettingsEpoch.load();
        const auto disabled = ApplyRuntimeSetting({plugin::RuntimeSettingKind::DialogueEnabled, false}, nullptr);
        Check(disabled.accepted && disabled.changed && !g_runtimeDialogueEnabled && g_dialogueSettingsEpoch == priorEpoch + 1, "Actual setting handler/epoch");
        plugin::RuntimeSettingsSnapshot settings{};
        Check(ReadRuntimeSettings(settings, nullptr) && !settings.dialogueEnabled, "Actual settings reader differs");
        Check(!ApplyRuntimeSetting({plugin::RuntimeSettingKind::OverlayFontSize, -1}, nullptr).accepted, "Invalid setting accepted");
        Check(!g_cameraCorrectionGate.IsHeldByCurrentThread(), "Settings retained gate");
    }
    void Serialization(const std::string& scenario) {
        Memory source, output; Setup(source);
        auto context = Context(source, output);
        Writer(context);
        context.xmm0.f32[0] = 90.0f;
        const auto previousLogs = logCalls;
        std::promise<void> started;
        std::future<void> completion;
        bool waited{};
        {
            auto lock = g_cameraCorrectionGate.Acquire();
            Log("V0: production logging helper under the runtime gate");
            Check(logCalls == previousLogs, "Log helper did not defer while gate held");
            completion = std::async(std::launch::async, [&] {
                started.set_value();
                if (scenario == "serialization-dialogue") DialogueCallback(context);
                else if (scenario == "serialization-enter") TraceCinematicEnter(context);
                else if (scenario == "serialization-exit") TraceCinematicExit(context);
                else if (scenario == "serialization-aspect") ApplyCinematicAspectStore(context);
                else if (scenario == "serialization-settings") ApplyRuntimeSetting({plugin::RuntimeSettingKind::DialogueEnabled, false}, nullptr);
                else if (scenario == "serialization-reader") { plugin::OverlaySemanticSnapshot facts{}; ReadOverlaySemanticSnapshot(facts, nullptr); }
                else Writer(context);
            });
            started.get_future().wait();
            waited = completion.wait_for(std::chrono::milliseconds(30)) == std::future_status::timeout;
        }
        Check(completion.wait_for(std::chrono::seconds(2)) == std::future_status::ready, "Callback did not resume after gate release");
        completion.get();
        Check(waited && g_cameraStateSnapshotValid, "Real writer bypassed runtime-owned gate");
        Check(logCalls > previousLogs && !logHeldGate, "Production logging helper did not emit after gate release");
    }
    void HistoricalTarget() {
        Memory source, output; Setup(source);
        auto context = Context(source, output); Writer(context);
        const auto target = ReadFacts().cameraState.evidence.horPlusTargetFov;
        Check(target.valid && target.freshness == camera::SnapshotFreshness::Current, "No initial Current target");
        source.Put(kAspectOffset, kNativeAspect);
        context.xmm0.f32[0] = 90.0f; const auto before = context; Writer(context);
        UnchangedContext(before, context);
        const auto fact = ReadFacts().cameraState.evidence.horPlusTargetFov;
        // Full Gameplay pass-through publication demotes the old target to
        // Historical; ENTER/EXIT transaction retention is a separate contract.
        Check(fact.valid && fact.freshness == camera::SnapshotFreshness::Historical, "Pass-through target not Historical");
        Near(fact.value, target.value, "Historical target changed");
    }
    void Recovery(const std::string& scenario) {
        Memory source, output; Setup(source);
        g_gameplayRecoveryObservationAvailable = true;
        g_runtimeGameplayMode = config::GameplayMode::RuntimeCombine;
        auto context = Context(source, output);
        for (int sample=0; sample<3; ++sample) { context.xmm0.f32[0] = 90.0f; Writer(context); }
        const auto baseline = ReadFacts().cameraState.evidence.gameplayFov;
        Check(baseline.valid, "Recovery fixture baseline not qualified");
        context = Context(source, output); auto before = context;
        TraceCinematicEnter(context); UnchangedContext(before, context, true);
        ApplyCinematicAspectStore(context);
        const auto cinematicTarget = g_cinematicTransformedFov.load();
        auto cinematicWriter = Context(source, output); before = cinematicWriter;
        Writer(cinematicWriter); UnchangedContext(before, cinematicWriter, true);
        Near(cinematicWriter.xmm0.f32[0], cinematicTarget, "Validated Cinematic writer target");
        Check(g_coordinator == CoordinatorState::CinematicActive, "Cinematic writer lost ownership");
        const auto cinematic = ReadFacts();
        Check(cinematic.cameraState.evidence.gameplayFov.freshness == camera::SnapshotFreshness::Retained, "Cinematic baseline not Retained");
        Near(cinematic.cameraState.evidence.gameplayFov.value, baseline.value, "Retained baseline value changed");
        Near(cinematic.cameraState.evidence.nativeFov.value, 90.0f, "Cinematic native observation");
        Near(cinematic.cameraState.evidence.horPlusTargetFov.value, cinematicTarget, "Cinematic target publication");
        Check(cinematic.cameraState.evidence.nativeFov.freshness == camera::SnapshotFreshness::Current &&
            cinematic.cameraState.evidence.horPlusTargetFov.freshness == camera::SnapshotFreshness::Current &&
            cinematic.cameraState.evidence.currentOutputFov.freshness == camera::SnapshotFreshness::Current,
            "Cinematic live roles not Current");
        Near(cinematic.cameraState.evidence.currentOutputFov.value, cinematicTarget, "Cinematic transaction publication");
        context = Context(source, output); before = context;
        TraceCinematicExit(context); UnchangedContext(before, context);
        Check(g_coordinator == CoordinatorState::CinematicExiting, "Owned EXIT did not enter recovery");
        const auto recoveryOwner = g_cinematicRecoveryOwner.Read();
        Check(recoveryOwner.valid && recoveryOwner.writerRsi == source.Address(),
            "EXIT did not retain the associated source as recovery owner");
        if (scenario == "recovery-foreign-source") {
            Memory foreignSource;
            foreignSource.Put(kAspectOffset, fixtureViewport);
            foreignSource.Put(kFlagsOffset, std::uint8_t{4});
            foreignSource.Put(kCameraWorldFovOffset, 95.0f);
            auto foreignSample = Context(foreignSource, output, 95.0f);
            before = foreignSample; Writer(foreignSample);
            UnchangedContext(before, foreignSample, true);
            Check(g_coordinator == CoordinatorState::Gameplay &&
                !g_cinematicRecoveryOwner.Read().valid,
                "Foreign RSI did not retire the prior-source Cinematic recovery handoff");
            return;
        }
        source.Put(kAspectOffset, fixtureViewport);
        auto interpolation = Context(source, output, 95.0f); before = interpolation;
        Writer(interpolation); UnchangedContext(before, interpolation, true);
        Check(g_coordinator == CoordinatorState::CinematicExiting, "Interpolation completed recovery early");
        Near(ReadFacts().cameraState.evidence.gameplayFov.value, baseline.value, "Recovery contaminated shared Gameplay baseline");
        auto endpoint = Context(source, output); before = endpoint;
        Writer(endpoint); UnchangedContext(before, endpoint, true);
        Check(g_coordinator == CoordinatorState::Gameplay, "Native endpoint did not complete recovery");
        Near(endpoint.xmm0.f32[0], camera::HorPlus(90.0f, fixtureViewport, kNativeAspect), "Gameplay after handoff output");
        for (int sample=0; sample<3; ++sample) { endpoint.xmm0.f32[0] = 90.0f; Writer(endpoint); }
        const auto gameplay = ReadFacts().cameraState.evidence;
        Check(gameplay.gameplayFov.valid && gameplay.gameplayFov.freshness == camera::SnapshotFreshness::Current,
            "Gameplay after handoff never re-qualified");
        Near(gameplay.nativeFov.value, 90.0f, "Gameplay native after handoff");
        Near(gameplay.horPlusTargetFov.value, endpoint.xmm0.f32[0], "Gameplay target after handoff");
        Near(gameplay.currentOutputFov.value, endpoint.xmm0.f32[0], "Gameplay output after handoff");
        Check(ReadFacts().cameraTransitionBoundary.value.cinematicValid, "Recovery lost independent Evidence");
    }
    void SettingsApi() {
        Memory source, output; Setup(source);
        for (int sample = 0; sample < 4; ++sample) {
            auto context = Context(source, output);
            Writer(context);
        }
        Check(g_runtimeSettings.Publish({&ApplyRuntimeSetting, nullptr,
            &ReadRuntimeSettings, nullptr, &ReadOverlaySemanticSnapshot, nullptr,
            &PersistRuntimeSetting, nullptr}), "Production settings API publication rejected");
        Check(!g_runtimeSettings.Publish({&ApplyRuntimeSetting, nullptr,
            &ReadRuntimeSettings, nullptr, &ReadOverlaySemanticSnapshot, nullptr,
            &PersistRuntimeSetting, nullptr}), "Production settings API accepted duplicate publication");

        plugin::RuntimeSettingsSnapshot settings{};
        Check(g_runtimeSettings.Snapshot(settings), "Published production settings reader failed");
        plugin::OverlaySemanticSnapshot semantic{};
        Check(g_runtimeSettings.SemanticSnapshot(semantic) && semantic.cameraStateValid,
            "Published production semantic reader failed");
        const auto applied = g_runtimeSettings.Apply(
            {plugin::RuntimeSettingKind::DialogueEnabled, false});
        Check(applied.accepted && !g_runtimeDialogueEnabled,
            "Published production mutation handler was bypassed");
        Check(g_runtimeSettings.Snapshot(settings) && !settings.dialogueEnabled,
            "Published readback did not reflect accepted mutation");

        WCHAR tempPath[MAX_PATH]{};
        Check(GetTempPathW(MAX_PATH, tempPath) != 0, "GetTempPathW");
        g_configPath = std::filesystem::path(tempPath) /
            (L"runtime-settings-v0-" + std::to_wstring(GetCurrentProcessId()) + L".ini");
        {
            std::ofstream config(g_configPath, std::ios::binary | std::ios::trunc);
            Check(static_cast<bool>(config), "Create fixture INI");
            config << "[Overlay]\nFontSize = 13\n";
        }
        const plugin::RuntimeSettingMutation fontSize{
            plugin::RuntimeSettingKind::OverlayFontSize, 17};
        Check(g_runtimeSettings.Persist(fontSize), "Production settings persistence handler failed on valid file");
        auto readFile = [] {
            std::ifstream input(g_configPath, std::ios::binary);
            return std::string(std::istreambuf_iterator<char>(input), {});
        };
        Check(readFile().find("FontSize = 17") != std::string::npos,
            "Production persistence did not update fixture value");

        const auto lastKnownGood = readFile();
        HANDLE lock = CreateFileW(g_configPath.c_str(), GENERIC_READ, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        Check(lock != INVALID_HANDLE_VALUE, "Lock fixture INI for atomic-replace failure");
        const plugin::RuntimeSettingMutation nextFontSize{
            plugin::RuntimeSettingKind::OverlayFontSize, 18};
        const bool persistedWhileLocked = g_runtimeSettings.Persist(nextFontSize);
        CloseHandle(lock);
        Check(!persistedWhileLocked, "Locked-file atomic replacement unexpectedly succeeded");
        Check(readFile() == lastKnownGood, "Failed atomic replacement damaged last-known-good INI");
        const auto missingPath = g_configPath;
        g_configPath += L".missing";
        Check(!g_runtimeSettings.Persist(fontSize), "Persistence accepted a missing configuration file");
        g_configPath = missingPath;
        Check(!g_cameraCorrectionGate.IsHeldByCurrentThread(),
            "Published persistence path retained correction gate");
    }
    void DialogueLifecycle(const std::string& scenario) {
        Memory source, output, foreignSource;
        Setup(source);
        auto sample = [&](std::uintptr_t callbackSource, float fov) {
            auto context = Context(source, output);
            context.rsi = callbackSource;
            context.xmm6.f32[0] = fov;
            const auto before = context;
            DialogueCallback(context);
            UnchangedContext(before, context, false, true);
            return context;
        };
        for (const float fov : {90.0f, 88.0f, 85.0f, 80.0f, 75.0f, 70.0f})
            sample(source.Address(), fov);
        Check(g_dialoguePhase == Phase::Active, "Dialogue lifecycle did not reach Active");
        if (scenario == "dialogue-disabled-in-flight") {
            const auto disabled = ApplyRuntimeSetting(
                {plugin::RuntimeSettingKind::DialogueEnabled, false}, nullptr);
            Check(disabled.accepted && !g_runtimeDialogueEnabled,
                "Actual production setting handler did not disable Dialogue");
            const auto callback = sample(source.Address(), 68.0f);
            const auto untouchedContext = Context(source, output);
            Check(callback.xmm1.f32[0] == untouchedContext.xmm1.f32[0],
                "Disabled in-flight Dialogue changed XMM1");
            Check(g_dialoguePhase == Phase::Active,
                "Disabling correction discarded the observed in-flight lifecycle");
            return;
        }
        sample(source.Address(), 75.0f);
        Check(g_dialoguePhase == Phase::Exiting, "Ascending Dialogue trajectory did not enter Exiting");
        sample(source.Address(), 80.0f);
        sample(source.Address(), 90.0f);
        Check(g_dialoguePhase == Phase::RearmPending,
            "Returning to baseline did not enter Dialogue RearmPending");
        if (scenario == "dialogue-rearm-source-change") {
            foreignSource.Put(0x2C, 70.0f);
            sample(foreignSource.Address(), 89.0f);
            Check(g_dialoguePhase == Phase::Inactive && !g_dialogueRecoveryRearm.IsPending(),
                "Foreign source did not cancel Dialogue re-arm");
        } else {
            source.Put(0x2C, 90.0f);
            sample(source.Address(), 90.0f);
            Check(g_dialoguePhase == Phase::Inactive && !g_dialogueRecoveryRearm.IsPending(),
                "Native endpoint did not complete Dialogue re-arm");
        }
        Check(g_coordinator == CoordinatorState::Gameplay,
            "Dialogue lifecycle altered camera coordinator");
    }
    void DiagnosticWork() {
        Memory source, output;
        Setup(source);
        if (!diagnosticMode) {
            std::unique_lock telemetryLock(g_horPlusFovTelemetryMutex);
            plugin::CameraWriterGameplayResult diagnosticInput{};
            TraceHorPlusFovState(diagnosticInput);
        }
        const auto readCount = [&] {
            return static_cast<std::size_t>(std::count_if(nativeEvents.begin(),
                nativeEvents.end(), [](const NativeEvent& event) {
                    return event.effect == NativeEffect::Read;
                }));
        };

        g_coordinator = CoordinatorState::CinematicActive;
        auto writer = Context(source, output);
        const auto beforeWriterReads = readCount();
        TraceHorPlusCinematicWriterBefore(writer);
        const auto writerReads = readCount() - beforeWriterReads;
        Check(writerReads == (diagnosticMode ? 6u : 0u),
            "Cinematic writer diagnostic memory reads escaped the OFF gate");
        TraceHorPlusCinematicWriterAfter(writer, writer.xmm0.f32[0]);
        if (diagnosticMode) {
            Check(std::any_of(observedLogs.begin(), observedLogs.end(), [](const auto& line) {
                return line.find("matchGameplayCandidateAvailable=") != std::string::npos;
            }), "MatchGameplay prediction was absent from the ordinary adapter build");
        }

        g_coordinator = CoordinatorState::Gameplay;
        if (diagnosticMode) {
            plugin::CameraWriterGameplayResult diagnosticInput{};
            diagnosticInput.inputFov = 90.0f;
            diagnosticInput.aspect = fixtureViewport;
            diagnosticInput.reason = "diagnostic-fixture";
            TraceHorPlusFovState(diagnosticInput);
            Check(std::any_of(observedLogs.begin(), observedLogs.end(), [](const auto& line) {
                return line.find("HORPLUS_STATE") != std::string::npos;
            }), "HorPlus FOV state trace was absent from the ordinary adapter build");
        }
        auto dialogue = Context(source, output);
        dialogue.xmm6.f32[0] = 90.0f;
        const auto beforeCallbacks = g_dialogueDiagnosticCallbacks.load(
            std::memory_order_relaxed);
        DialogueCallback(dialogue);
        Check(g_dialogueDiagnosticCallbacks.load(std::memory_order_relaxed) ==
                beforeCallbacks + (diagnosticMode ? 1u : 0u),
            "Dialogue diagnostic callback counter did not follow runtime gate");

        ArmDialogueRecoveryEndpointTrace(source.Address());
        dialogue = Context(source, output);
        dialogue.rsi = source.Address();
        const auto beforeRecoveryReads = readCount();
        ObserveDialogueRecoveryEndpointTrace(dialogue, DialoguePhase::RearmPending,
            89.0f, 90.0f, 88.0f, 1, false);
        const auto recoveryReads = readCount() - beforeRecoveryReads;
        Check(recoveryReads == (diagnosticMode ? 3u : 0u),
            "Dialogue recovery diagnostic reads escaped the OFF gate");
        if (diagnosticMode) {
            auto* trace = GetCinematicContinuationTrace();
            trace->Begin(true, 1, source.Address(), 90.0f, 100.0f);
            RecordCinematicContinuationTrace(true, true, source.Address(), true,
                95.0f, 100.0f, 90.0f, 1);
            const auto snapshot = trace->Finish(true, "adapter-gate-test");
            Check(snapshot.storedSamples == 1 && snapshot.totalSamples == 1,
                "Enabled continuation trace did not record the actual adapter sample");
        }
        std::cout << "diagnostic_work diagnostic=" << (diagnosticMode ? "on" : "off")
            << " cinematic_writer_reads=" << writerReads
            << " dialogue_callbacks=" << (diagnosticMode ? 1 : 0)
            << " recovery_reads=" << recoveryReads << "\n";
    }
    void DiagnosticPerformance() {
        Memory source, output;
        diagnosticMode = true;
        Setup(source);
        constexpr std::size_t iterations = 4096;
        constexpr std::size_t repetitions = 7;
        const diagnostics::ContinuationTraceInput sample{
            source.Address(), true, 95.0f, 100.0f, 90.0f, 100.0f,
            static_cast<std::uint8_t>(cinematics::CinematicWriterAction::RetainContinuationTarget),
            true, false};
        diagnostics::CinematicContinuationTrace preservedBaseline;
        std::vector<double> baselineNs, disabledNs, enabledNs;
        baselineNs.reserve(repetitions);
        disabledNs.reserve(repetitions);
        enabledNs.reserve(repetitions);
        for (std::size_t repetition = 0; repetition < repetitions; ++repetition) {
            auto start = std::chrono::steady_clock::now();
            for (std::size_t i = 0; i < iterations; ++i) {
                const auto lower = (std::min)(sample.continuationStartFov,
                    sample.cinematicTargetFov);
                const auto upper = (std::max)(sample.continuationStartFov,
                    sample.cinematicTargetFov);
                const bool finite = std::isfinite(sample.rawInputFov) &&
                    std::isfinite(sample.continuationStartFov) &&
                    std::isfinite(sample.cinematicTargetFov);
                diagnostics::ContinuationTraceInput prepared = sample;
                prepared.inProductionRange = finite && sample.rawInputFov >= lower &&
                    sample.rawInputFov <= upper;
                prepared.inSemanticTargetRegion = std::isfinite(sample.rawInputFov) &&
                    std::isfinite(sample.cinematicTargetFov) &&
                    camera::AreFovValuesSemanticallyEquivalent(sample.rawInputFov,
                        sample.cinematicTargetFov);
                static_cast<void>(preservedBaseline.Record(false, prepared));
            }
            baselineNs.push_back(std::chrono::duration<double, std::nano>(
                std::chrono::steady_clock::now() - start).count() / iterations);

            diagnostics::SetEnabled(false);
            start = std::chrono::steady_clock::now();
            for (std::size_t i = 0; i < iterations; ++i)
                RecordCinematicContinuationTrace(false, true, source.Address(), true,
                    95.0f, 100.0f, 90.0f, 1);
            disabledNs.push_back(std::chrono::duration<double, std::nano>(
                std::chrono::steady_clock::now() - start).count() / iterations);

            diagnostics::SetEnabled(true);
            auto* trace = GetCinematicContinuationTrace();
            Check(trace != nullptr, "Enabled performance fixture lacks trace storage");
            trace->Begin(true, repetition + 1, source.Address(), 90.0f, 100.0f);
            start = std::chrono::steady_clock::now();
            for (std::size_t i = 0; i < iterations; ++i)
                RecordCinematicContinuationTrace(true, true, source.Address(), true,
                    95.0f, 100.0f, 90.0f, 1);
            enabledNs.push_back(std::chrono::duration<double, std::nano>(
                std::chrono::steady_clock::now() - start).count() / iterations);
            const auto snapshot = trace->Finish(true, "performance-fixture");
            Check(snapshot.storedSamples == diagnostics::kContinuationTraceCapacity &&
                snapshot.totalSamples == iterations && snapshot.overflow,
                "Enabled continuation workload was not bounded at the sample capacity");
        }
        const auto median = [](std::vector<double>& values) {
            std::sort(values.begin(), values.end());
            return values[values.size() / 2];
        };
        std::cout << "continuation_trace_ns_per_sample baseline_off=" << median(baselineNs)
            << " current_off=" << median(disabledNs)
            << " current_on=" << median(enabledNs)
            << " iterations_per_repetition=" << iterations
            << " repetitions=" << repetitions
            << " enabled_capacity=" << diagnostics::kContinuationTraceCapacity
            << " off_trace_owner=" << (GetCinematicContinuationTrace() ? "allocated_for_on_fixture" : "null")
            << "\n";
    }
#include "runtime_native_evidence_cases.hpp"
}

extern "C" BOOL WINAPI SafetyHookTestVirtualProtect(LPVOID address, SIZE_T size,
    DWORD protection, PDWORD previous) {
    const auto call = ++safetyHookVirtualProtectCalls;
    if (call == safetyHookFailVirtualProtectAt && !safetyHookFailAfterVirtualProtect)
        return FALSE;
    const auto result = ::VirtualProtect(address, size, protection, previous);
    if (call == safetyHookFailVirtualProtectAt && safetyHookFailAfterVirtualProtect && result)
        return FALSE;
    return result;
}

extern "C" void SafetyHookTestAllocationRelease(void* address, std::size_t size,
    int returnedToAllocator) {
    safetyHookAllocationReleases.push_back({address, size, returnedToAllocator != 0});
}
namespace platform::win32 {
    float RuntimeAdapterFixtureViewport() { return fixtureViewport; }
    float RuntimeAdapterFixtureAutoViewport() { return fixtureViewport; }
    bool RuntimeAdapterFixtureReadMemory(std::uintptr_t address, void* destination, std::size_t size) {
        // The only simulated memory failure is an explicit post-write readback case.
        const bool result = !(refuseReadback && aspectWriteSucceeded && address == fixtureSource + kAspectOffset) &&
            ReadMemory(address, destination, size);
        nativeEvents.push_back({NativeEffect::Read, address, result});
        return result;
    }
}
namespace gameplay {
    bool RuntimeAdapterFixtureWriteAspectOnly(std::uintptr_t source, std::uintptr_t offset,
        float aspect, WritablePredicate writable) {
        const bool result = WriteAspectOnly(source, offset, aspect, writable);
        aspectWriteSucceeded |= result;
        nativeEvents.push_back({NativeEffect::WriteAspect, source+offset, result});
        return result;
    }
}
extern "C" void RuntimeAdapterFixtureWake() noexcept {
    ++wakes;
    wakeHeldGate = g_cameraCorrectionGate.IsHeldByCurrentThread();
    wakeSawPublication = g_cameraStateSnapshotValid;
    if (currentContext) xmmAtWake = currentContext->xmm0.f32[0];
    if (currentContext) xmm1AtWake = currentContext->xmm1.f32[0];
    nativeEvents.push_back({NativeEffect::Wake, 0, true});
}
int main(int argc, char** argv) {
    if (argc == 1) {
        const char* cases[] = {"writer-wide", "writer-native", "writer-combine", "writer-combine-readonly",
            "writer-combine-invalid-viewport", "writer-aspect", "writer-off", "writer-gate-off", "writer-stopping",
            "writer-unreadable", "aspect-wide", "aspect-off", "aspect-readonly", "aspect-unreadable",
            "lifecycle", "lifecycle-off", "lifecycle-gate-off", "lifecycle-chain", "dialogue-active", "dialogue-off",
            "dialogue-cinematic", "dialogue-unreadable", "zoom", "zoom-unreadable", "zoom-in-throw", "zoom-out-throw", "settings-wake", "serialization", "writer-invalid-fov",
            "writer-combine-readback-failure", "historical-target", "serialization-dialogue", "serialization-enter", "serialization-exit",
            "serialization-aspect", "serialization-settings", "serialization-reader", "recovery",
            "settings-api", "dialogue-exiting", "dialogue-rearm-source-change", "dialogue-disabled-in-flight",
            "lifecycle-foreign-association", "recovery-foreign-source",
            "cinematic-initial-gameplay-baseline", "cinematic-initial-foreign-source",
            "cinematic-initial-stale-generation", "cinematic-initial-missing-baseline",
            "cinematic-initial-invalid-baseline", "restoration-logging",
            "cinematic-continuation-baseline", "cinematic-continuation-combine",
            "cinematic-continuation-chain", "cinematic-continuation-foreign-source",
            "cinematic-continuation-foreign-writer", "cinematic-continuation-stale-generation",
            "cinematic-continuation-no-association", "cinematic-continuation-invalid-baseline",
            "cinematic-continuation-native-mode", "cinematic-continuation-target-change",
            "cinematic-continuation-outside-low", "cinematic-continuation-outside-high",
            "cinematic-continuation-correction-off", "cinematic-continuation-exit-departure",
            "cinematic-continuation-no-validated-exit", "cinematic-continuation-policy-change",
            "evidence-dialogue-identities", "evidence-dialogue-invalid-fov",
            "evidence-dialogue-invalid-target", "evidence-dialogue-source-change",
            "evidence-dialogue-reset", "evidence-dialogue-policy-snapshot",
            "evidence-dialogue-reenable", "evidence-dialogue-exclusion",
            "evidence-writer-identities", "evidence-writer-invalid-fields",
            "evidence-writer-ads", "evidence-runtime-resize",
            "evidence-cinematic-invalid-enter", "evidence-cinematic-rejected-exit",
            "evidence-cinematic-indexed-exit", "evidence-cinematic-cancel",
            "evidence-cinematic-aspect-targets", "evidence-settings-kinds",
            "evidence-settings-lock-order", "evidence-persistence-kinds",
            "evidence-reset-empty", "evidence-exception-boundary",
            "evidence-nested-deferral", "evidence-native-trampoline", "evidence-zoom-native-continuation",
            "evidence-safetyhook-lifetime-faults", "evidence-safetyhook-failed-create",
            "evidence-safetyhook-disable-postwrite", "evidence-reset-self-join",
            "evidence-reset-native-hook", "evidence-install-transaction-order",
            "diagnostic-work",
            "startup-fatal-early", "startup-fatal-partial",
            "startup-fatal-cleanup-throws", "startup-fatal-retained-worker"};
        wchar_t executable[MAX_PATH]{};
        const auto length = GetModuleFileNameW(nullptr, executable, MAX_PATH);
        if (!length || length >= MAX_PATH) return 1;
        for (auto scenario : cases) for (auto mode : {"off", "on"}) {
            const std::string arguments = std::string(scenario) + " " + mode;
            std::wstring command = std::wstring(L"\"") + executable + L"\" " +
                std::wstring(arguments.begin(), arguments.end());
            STARTUPINFOW startup{}; startup.cb = sizeof(startup);
            startup.dwFlags = STARTF_USESTDHANDLES;
            startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
            startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
            startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
            PROCESS_INFORMATION child{};
            if (!CreateProcessW(executable, command.data(), nullptr, nullptr, TRUE,
                    CREATE_NO_WINDOW, nullptr, nullptr, &startup, &child)) return 1;
            CloseHandle(child.hThread);
            const auto wait = WaitForSingleObject(child.hProcess, 10000);
            DWORD code = 1;
            if (wait == WAIT_OBJECT_0) GetExitCodeProcess(child.hProcess, &code);
            else {
                // Only this fixture-owned child may be terminated; a gate
                // regression must fail the harness rather than hang test.cmd.
                TerminateProcess(child.hProcess, 1);
                WaitForSingleObject(child.hProcess, 2000);
                std::cerr << scenario << ": FAIL: fixture timed out\n";
            }
            CloseHandle(child.hProcess);
            if (code != 0) return 1;
        }
        std::cout << "runtime_native_adapter_harness: PASS (" << std::size(cases)
            << " cases x Diagnostic Mode OFF/ON; " << std::size(cases) * 2
            << " fresh processes)\n"; return 0;
    }
    if (argc == 3 && std::string(argv[1]) == "diagnostics-performance") {
        try {
            diagnosticMode = std::string(argv[2]) == "on";
            DiagnosticPerformance();
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "diagnostics-performance: FAIL: " << error.what() << '\n';
            return 1;
        }
    }
    try {
        const std::string scenario = argv[1];
        diagnosticMode = argc > 2 && std::string(argv[2]) == "on";
        if (scenario == "diagnostic-work") DiagnosticWork();
        else if (scenario.starts_with("evidence-")) NativeEvidence(scenario);
        else if (scenario.starts_with("startup-fatal-")) StartupFailure(scenario);
        else if (scenario == "restoration-logging") RestorationLogging();
        else if (scenario.starts_with("writer-")) Gameplay(scenario);
        else if (scenario.starts_with("aspect-")) Aspect(scenario);
        else if (scenario.starts_with("lifecycle")) Lifecycle(scenario);
        else if (scenario.starts_with("cinematic-initial-")) CinematicInitialGameplayBaseline(scenario);
        else if (scenario.starts_with("cinematic-continuation-")) CinematicGameplayBaselineContinuation(scenario);
        else if (scenario == "dialogue-exiting" || scenario == "dialogue-rearm-source-change" ||
            scenario == "dialogue-disabled-in-flight") DialogueLifecycle(scenario);
        else if (scenario.starts_with("dialogue-")) Dialogue(scenario);
        else if (scenario.starts_with("zoom")) Zoom(scenario);
        else if (scenario == "settings-wake") SettingsAndWake();
        else if (scenario == "settings-api") SettingsApi();
        else if (scenario.starts_with("serialization")) Serialization(scenario);
        else if (scenario == "historical-target") HistoricalTarget();
        else if (scenario == "recovery" || scenario == "recovery-foreign-source") Recovery(scenario);
        else throw std::runtime_error("Unknown case");
        Check(!logHeldGate, "Production Log emitted while gate held");
        std::cout << scenario << " diagnostic=" << (diagnosticMode ? "on" : "off") << ": PASS\n"; return 0;
    } catch (const std::exception& error) { std::cerr << argv[1] << ": FAIL: " << error.what() << '\n'; return 1; }
}
