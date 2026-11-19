void PostExitBlendActivationCases();

void PostExitBlendObservationCases()
{
    PostExitBlendActivationCases();
    Memory blend, camera, output, stack;
    const bool dialogueEnabledBefore = g_runtimeDialogueEnabled.load();
    const auto coordinatorBefore = g_coordinator.load();
    Memory nativeCode;
    auto* hook = nativeCode.bytes + 512;
    const std::uint8_t endpoints[]{0xF3,0x0F,0x10,0x76,0x2C,0xF3,0x0F,0x10,0x46,0x30};
    const std::uint8_t countdown[]{0xF3,0x0F,0x10,0x4E,0x28,0xF3,0x0F,0x5C,0xCF,0xF3,0x0F,0x11,0x4E,0x28};
    const std::uint8_t call[]{0xFF,0x90,0x08,0x06,0x00,0x00};
    std::memcpy(hook - 128, endpoints, sizeof(endpoints));
    std::memcpy(hook - 300, countdown, sizeof(countdown));
    std::memcpy(hook, call, sizeof(call));
    const hooks::ExecutableSpan span{nativeCode.bytes, 4096};
    Check(ValidatePostExitBlendLayout(hook, span), "Validated local blend layout was rejected");
    hook[-300] = 0;
    Check(!ValidatePostExitBlendLayout(hook, span), "Missing countdown admitted a guessed layout");
    Check(!ValidatePostExitBlendLayout(nativeCode.bytes + 100, span), "Out-of-span layout read");
    g_runtime.postExitBlendLayoutAvailable = true;
    g_runtime.postExitBlendObservationOnlyBoundary = true;
    g_runtime.postExitBlendObservation.Begin(12, camera.Address(), 90.3115f);
    g_runtime.postExitBlendActivationObservation.Begin(12);
    g_runtime.postExitBlendActivationAvailable = true;
    g_runtime.postExitBlendSetterReturnAvailable.store(true,
        std::memory_order_release);
    g_runtime.postExitBlendSetterTraceActive.store(diagnosticMode,
        std::memory_order_release);
    constexpr std::uintptr_t enterReturn = 0x140012345;
    constexpr std::uintptr_t exitReturn = 0x140067890;
    g_runtime.postExitBlendEnterReturn = enterReturn;
    g_runtime.postExitBlendExitReturn = exitReturn;
    g_runtimeDialogueEnabled = false;
    g_coordinator = CoordinatorState::CinematicExiting;
    blend.Put(0x50, 1.0f); blend.Put(0x54, 90.3115f); blend.Put(0x58, 70.0f);
    camera.Put(0x230, 70.0f);
    stack.Put(0x100 + diagnostics::kBlendArmerReturnSlotOffset, exitReturn);
    auto activation = Context(blend, output, 70.0f);
    activation.rdi = camera.Address();
    activation.rsp = stack.Address() + 0x100;
    const auto activationBefore = activation;
    TracePostExitBlendStartCapture(activation);
    UnchangedContext(activationBefore, activation);
    Check(g_runtime.postExitBlendActivationObservation.Captures() ==
            (diagnosticMode ? 1u : 0u),
        "Start-capture observer did not obey the Diagnostics switch");
    auto context = Context(blend, output, 70.0f);
    context.rsi = blend.Address() + 0x28;
    context.rcx = camera.Address();
    context.rsp = stack.Address() + 0x200;
    context.xmm1.f32[0] = 70.0f;
    const auto before = context;
    const auto snapshotBefore = GetDialogueBoundaryStateView().snapshotSource.load();
    TraceDialogueBoundary(context);
    UnchangedContext(before, context);
    Check(GetDialogueBoundaryStateView().snapshotSource.load() == snapshotBefore,
        "Observation-only hook published production Dialogue state");
    Check(g_runtime.postExitBlendObservation.Callbacks() == (diagnosticMode ? 1u : 0u),
        "Blend observation depended on disabled Dialogue policy or ignored Diagnostics OFF");
    Check(g_runtime.postExitBlendObservation.Active(),
        "Diagnostics OFF changed the observation sequence state");
    Near(camera.Get<float>(0x230), 70.0f, "Pre-setter observation wrote camera state");
    if (diagnosticMode) {
        Check(g_postExitBlendSetterPairing.Pending() == 1,
            "Enabled post-setter observer did not retain the in-flight invocation");
        camera.Put(0x230, context.xmm1.f32[0]); // Simulated native setter effect.
        auto afterSetter = context;
        afterSetter.xmm6.f32[0] = context.xmm6.f32[0];
        TracePostExitBlendSetterReturn(afterSetter);
        Check(g_postExitBlendSetterPairing.Pending() == 0,
            "Post-setter continuation did not consume its matching invocation");
        Check(std::any_of(observedLogs.begin(), observedLogs.end(), [](const auto& line) {
                return line.find("POST_EXIT_BLEND_POST_SETTER") != std::string::npos &&
                    line.find("epoch=1") != std::string::npos &&
                    line.find("camera_field_after_setter=70") != std::string::npos &&
                    line.find("setter_equals_field=1") != std::string::npos &&
                    line.find("easing_branch=unknown") != std::string::npos &&
                    line.find("writer_same_invocation=not_established") != std::string::npos;
            }), "Post-setter readback was not logged with its evidence limits");

        diagnostics::BlendSetterInvocation retired{};
        retired.generation = g_runtime.postExitBlendObservation.Generation();
        retired.epoch = 1;
        retired.component = blend.Address();
        retired.receiver = camera.Address();
        retired.stackPointer = context.rsp + 0x10;
        retired.thread = GetCurrentThreadId();
        retired.recordSelected = true;
        Check(g_postExitBlendSetterPairing.Push(retired),
            "Could not set up an in-flight pair for generation-retirement coverage");
        g_runtime.postExitBlendSetterTraceActive.store(false,
            std::memory_order_release);
        const auto readCountBeforeRetiredReturn = std::count_if(nativeEvents.begin(),
            nativeEvents.end(), [](const NativeEvent& event) {
                return event.effect == NativeEffect::Read;
            });
        const auto logCountBeforeRetiredReturn = observedLogs.size();
        auto retiredReturn = context;
        retiredReturn.rsp = retired.stackPointer;
        TracePostExitBlendSetterReturn(retiredReturn);
        Check(g_postExitBlendSetterPairing.Pending() == 0 &&
            std::count_if(nativeEvents.begin(), nativeEvents.end(),
                [](const NativeEvent& event) { return event.effect == NativeEffect::Read; }) ==
                    readCountBeforeRetiredReturn &&
            observedLogs.size() == logCountBeforeRetiredReturn,
            "Retired in-flight setter pair was not drained without reads or logging");
    } else {
        const auto countReads = [] {
            return std::count_if(nativeEvents.begin(), nativeEvents.end(),
                [](const NativeEvent& event) { return event.effect == NativeEffect::Read; });
        };
        const auto readsBefore = countReads();
        TracePostExitBlendSetterReturn(context);
        Check(countReads() == readsBefore && g_postExitBlendSetterPairing.Pending() == 0,
            "Diagnostics OFF performed post-setter reads or pairing work");
    }
    auto writer = Context(camera, output, 70.0f);
    const auto writerBefore = writer;
    {
        PostExitBlendWriterCapture capture(writer);
    }
    UnchangedContext(writerBefore, writer);
    if (diagnosticMode) {
        std::cout << "post_exit_record_count=" << g_runtime.postExitBlendObservation.Records() << '\n';
        Check(g_runtime.postExitBlendObservation.Records() <= diagnostics::PostExitBlendObservation::RecordLimit,
            "Shared capture record budget exceeded its limit");
        Check(std::any_of(observedLogs.begin(), observedLogs.end(), [](const auto& line) {
                return line.find("POST_EXIT_BLEND_START_CAPTURE disposition=candidate") != std::string::npos &&
                    line.find("epoch=1") != std::string::npos;
            }), "Candidate activation epoch was not logged at start capture");
        Check(std::any_of(observedLogs.begin(), observedLogs.end(), [](const auto& line) {
                return line.find("POST_EXIT_BLEND_SAMPLE") != std::string::npos &&
                    line.find("activation_epoch=1") != std::string::npos &&
                    line.find("activation_match=matched_tick") != std::string::npos &&
                    line.find("caller_kind=cinematic_exit_return") != std::string::npos;
            }), "Tick adapter did not attach the matching candidate epoch and caller evidence");
        Check(std::any_of(observedLogs.begin(), observedLogs.end(), [](const auto& line) {
                return line.find("POST_EXIT_BLEND_WRITER") != std::string::npos &&
                    line.find("activation_epoch=1") != std::string::npos;
            }), "Existing writer observation did not retain the matched candidate epoch");
        const auto tickMatch = g_runtime.postExitBlendActivationObservation.MatchTick(
            context.rsi - 0x28, camera.Address(), 70.0f, 90.3115f);
        Check(tickMatch.status == diagnostics::BlendActivationMatchStatus::Matched &&
            tickMatch.candidate.epoch == 1 &&
            tickMatch.candidate.capture.caller.kind == diagnostics::BlendCallerKind::CinematicExit,
            "Actual callback adapter did not carry caller/epoch evidence to the tick");
        auto invalid = context;
        invalid.rsi = (std::numeric_limits<std::uintptr_t>::max)() - 8;
        {
            PostExitBlendCallbackCapture capture(invalid);
            Check(capture.sample.readable == 24,
                "Overflow/unreadable blend fields were presented as readable");
        }
    }
    FinishPostExitBlendObservation("adapter_test");
    const auto path = std::filesystem::current_path() / "build-artifacts" / "tests" /
        ("blend-observation-" + std::to_string(GetCurrentProcessId()) + ".log");
    auto sink = std::make_shared<platform::win32::NativeLogSink>(path.c_str());
    g_logger->sinks().push_back(sink);
    g_runtime.postExitBlendObservation.Begin(13, camera.Address(), 90.3115f);
    for (unsigned i = 0; i < 10000; ++i) {
        context.xmm6.f32[0] = context.xmm1.f32[0] = 70.0f + (i % 80) * 0.2f;
        const auto registers = context;
        TraceDialogueBoundary(context);
        UnchangedContext(registers, context);
    }
    FinishPostExitBlendObservation("stress_end");
    g_logger->sinks().pop_back(); sink.reset();
    std::ifstream recorded(path, std::ios::binary);
    std::size_t lines{}; std::string line;
    while (std::getline(recorded, line)) ++lines;
    recorded.close();
    Check(lines <= 20 && (diagnosticMode || !lines),
        "10000 blend callbacks generated unbounded real file records");
    std::cout << "blend_observation callbacks=10000 file_lines=" << lines
        << " bytes=" << std::filesystem::file_size(path) << '\n';
    std::filesystem::remove(path);
    g_runtime.postExitBlendLayoutAvailable = false;
    g_runtime.postExitBlendObservationOnlyBoundary = false;
    g_runtime.postExitBlendActivationAvailable = false;
    g_runtime.postExitBlendSetterReturnAvailable.store(false,
        std::memory_order_release);
    g_runtime.postExitBlendSetterTraceActive.store(false,
        std::memory_order_release);
    g_runtime.postExitBlendActivationObservation.Retire();
    g_runtime.postExitBlendEnterReturn = 0;
    g_runtime.postExitBlendExitReturn = 0;
    g_runtimeDialogueEnabled = dialogueEnabledBefore;
    g_coordinator = coordinatorBefore;
}
