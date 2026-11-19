// Actual production adapter and native file sink, with synthetic camera inputs.
void DiagnosticAggregation()
{
    Memory source, a, b, foreign;
    Setup(source);
    PostExitBlendObservationCases();
    g_runtimeCinematicPolicy = config::CinematicAspectPolicy::Auto;
    g_runtimeCinematicFovMode = config::CinematicFovMode::GameplayHorPlus;
    constexpr float baseline = 95.1311f;
    source.Put(kCameraWorldFovOffset, baseline);
    for (unsigned i = 0; i < 3; ++i) {
        auto writer = Context(source, a, baseline);
        Writer(writer);
    }
    auto enter = Context(source, a, 90.0f);
    TraceCinematicEnter(enter);
    ApplyCinematicAspectStore(enter);
    const auto target = g_cinematicTransformedFov.load();

    const auto path = std::filesystem::current_path() / "build-artifacts" / "tests" /
        ("diagnostic-aggregation-" + std::to_string(GetCurrentProcessId()) + ".log");
    auto fileSink = std::make_shared<platform::win32::NativeLogSink>(path.c_str());
    g_logger->sinks().push_back(fileSink);
    const auto logStart = observedLogs.size();
    for (unsigned i = 0; i < 10000; ++i) {
        auto writer = Context(source, i % 2 ? b : a,
            baseline + static_cast<float>(i % 7) * 0.00001f);
        const auto before = writer;
        Writer(writer);
        Near(writer.xmm0.f32[0], target, "RBX aggregation altered actual correction");
        UnchangedContext(before, writer, true);
    }
    const auto count = [&](std::string_view marker) {
        return std::count_if(observedLogs.begin() + logStart, observedLogs.end(),
            [&](const auto& line) { return line.find(marker) != std::string::npos; });
    };
    Check(count("change_kind=RbxOnlyChanged") == 0,
        "Repeated output transitions still generated individual identity records");
    Check(count("HORPLUS_CINEMATIC_WRITER") <= 8 &&
        count("CINEMATIC_ASSOCIATION_WRITER_STATE") <= 4,
        "Output cycling/numeric samples flooded full diagnostic records");
    if (!diagnosticMode) Check(count("HORPLUS_CINEMATIC_WRITER") == 0 &&
        count("CINEMATIC_ASSOCIATION_WRITER_STATE") == 0,
        "Disabled diagnostics emitted cinematic samples");

    std::cout << "diagnostic_aggregation steady_callbacks=10000 writer_state_records="
        << count("CINEMATIC_ASSOCIATION_WRITER_STATE") << " before_after_records="
        << count("HORPLUS_CINEMATIC_WRITER") << '\n';

    // Source change must publish the old source's aggregate and remain visible.
    foreign.Put(kAspectOffset, fixtureViewport);
    foreign.Put(kFlagsOffset, std::uint8_t{5});
    auto unrelated = Context(foreign, a, baseline);
    Writer(unrelated);
    Near(unrelated.xmm0.f32[0], baseline, "Foreign writer gained correction");
    if (diagnosticMode) {
        Check(count("CAMERA_WRITER_IDENTITY_CHANGED") >= 1,
            "Source identity change was lost");
        Check(std::any_of(observedLogs.begin()+logStart, observedLogs.end(), [](const auto& line) {
            return line.find("output_transitions=9999 samples=10000") != std::string::npos;
        }), "Source boundary lost exact aggregated counts");
    }

    auto conflict = Context(foreign, a, baseline);
    ApplyCinematicAspectStore(conflict);
    auto rejected = Context(source, a, baseline);
    Writer(rejected);
    Near(rejected.xmm0.f32[0], baseline, "Conflict failed to preserve native output");
    if (diagnosticMode) Check(count("association_conflict=1") >= 1,
        "Ownership conflict was suppressed");

    // Exercise the actual diagnostic emission boundary with an exact Apply/Skip
    // change much smaller than either historical numeric logging threshold.
    if (diagnosticMode) {
        auto guard = g_cameraCorrectionGate.Acquire();
        const auto association = g_cinematicWriterAssociation.Read();
        const auto emit = [&](bool applied) {
            return LogCinematicDiagnosticWriterState(association,
                CoordinatorState::CinematicActive, source.Address(), a.Address(),
                90.0f, true, 90.0f, applied ? std::nextafter(90.0f, 100.0f) : 90.0f,
                cinematics::CinematicWriterAction::RetainTransformedTarget,
                true, false, false, applied);
        };
        Check(emit(true) && emit(false), "Tiny Apply-to-Skip decision was suppressed");
        Check(!emit(false), "Repeated identical decision emitted a full record");
    }

    auto exit = Context(source, a, baseline);
    TraceCinematicExit(exit);
    if (diagnosticMode) Check(count("CAMERA_WRITER_SUMMARY reason=cinematic_exit") >= 1,
        "EXIT failed to publish diagnostic tail");
    const auto afterExitSummaries = count("CAMERA_WRITER_SUMMARY");
    { auto guard = g_cameraCorrectionGate.Acquire(); FlushWriterDiagnosticSummary("empty"); }
    Check(count("CAMERA_WRITER_SUMMARY") == afterExitSummaries, "Empty flush duplicated summary");

    enter = Context(source, a, 90.0f);
    TraceCinematicEnter(enter); ApplyCinematicAspectStore(enter);
    auto writer = Context(source, a, baseline); Writer(writer);
    // A duplicate ENTER while already Active is rejected by the production
    // lifecycle. Exercise a real new generation after validated EXIT recovery,
    // leaving an output-only Gameplay aggregate for ENTER to publish.
    exit = Context(source, a, baseline); TraceCinematicExit(exit);
    writer = Context(source, a, baseline); Writer(writer);
    Check(g_coordinator == CoordinatorState::Gameplay, "Fixture did not recover before next ENTER");
    writer = Context(source, b, baseline); Writer(writer);
    enter = Context(source, a, 90.0f);
    TraceCinematicEnter(enter); ApplyCinematicAspectStore(enter);
    if (diagnosticMode) Check(count("CAMERA_WRITER_SUMMARY reason=superseded_by_accepted_generation") >= 1,
        "New generation failed to publish prior aggregate");
    writer = Context(source, a, baseline); Writer(writer);
    g_runtime.postExitBlendEnterReturn = 0x1005;
    g_runtime.postExitBlendExitReturn = 0x2005;
    g_runtime.postExitBlendEnterRelocatedReturn = 0x3005;
    g_runtime.postExitBlendExitRelocatedReturn = 0x4005;
    ResetAllRuntimeResources();
    Check(!g_runtime.postExitBlendEnterReturn && !g_runtime.postExitBlendExitReturn &&
        !g_runtime.postExitBlendEnterRelocatedReturn && !g_runtime.postExitBlendExitRelocatedReturn,
        "Actual runtime cleanup retained stale native/relocated CALL mappings");
    if (diagnosticMode) Check(count("CAMERA_WRITER_SUMMARY reason=runtime_reset") >= 1,
        "Controlled runtime cleanup failed to publish tail");
    else Check(count("CAMERA_WRITER_SUMMARY") == 0, "OFF aggregation emitted summaries");

    g_logger->sinks().pop_back(); fileSink.reset();
    std::ifstream recorded(path, std::ios::binary);
    std::size_t lines = 0; std::string line;
    while (std::getline(recorded, line)) ++lines;
    recorded.close();
    Check(lines < 200, "10000 alternating outputs generated unbounded real file records");
    std::cout << "diagnostic_aggregation callbacks=10000 file_lines=" << lines
        << " bytes=" << std::filesystem::file_size(path) << '\n';
    std::filesystem::remove(path);
}
