// Included once inside the existing actual-adapter harness namespace.
// All decisions run through production adapters; only fixture inputs and
// native context/memory observations live here. No alternate runtime owner.

    void EvidenceResetDialogue() {
        auto gate = g_cameraCorrectionGate.Acquire();
        std::lock_guard dialogueLock(g_dialogueMutex);
        ResetDialogueRuntimeState();
    }

    SafetyHookContext EvidenceDialogueSample(Memory& source, Memory& output,
        float incoming, std::uintptr_t rsi, std::uintptr_t rbx) {
        auto context = Context(source, output);
        context.rsi = rsi;
        context.rbx = rbx;
        context.xmm6.f32[0] = incoming;
        const auto before = context;
        std::array<std::uint8_t, 4096> outputBefore{};
        std::memcpy(outputBefore.data(), output.bytes, outputBefore.size());
        currentContext = &context;
        SafeMidHookEntry<&TraceDialogueBoundary>(context);
        currentContext = nullptr;
        UnchangedContext(before, context, false, true);
        Check(std::memcmp(outputBefore.data(), output.bytes, outputBefore.size()) == 0,
            "Dialogue adapter wrote native output memory");
        return context;
    }

    void EvidencePrimeDialogue(Memory& source, Memory& output, Phase phase) {
        EvidenceResetDialogue();
        source.Put(0x2C, 70.0f);
        if (phase == Phase::Inactive) return;
        EvidenceDialogueSample(source, output, 100.0f, source.Address(), output.Address());
        if (phase == Phase::Candidate) {
            Check(g_dialoguePhase == phase, "Fixture did not establish Candidate");
            return;
        }
        for (float fov : {98.0f, 95.0f, 90.0f, 80.0f, 70.0f})
            EvidenceDialogueSample(source, output, fov, source.Address(), output.Address());
        if (phase != Phase::Active)
            EvidenceDialogueSample(source, output, 75.0f, source.Address(), output.Address());
        if (phase == Phase::RearmPending) {
            EvidenceDialogueSample(source, output, 80.0f, source.Address(), output.Address());
            EvidenceDialogueSample(source, output, 100.0f, source.Address(), output.Address());
        }
        Check(g_dialoguePhase == phase, "Fixture did not establish requested Dialogue phase");
    }

    void EvidenceDialogueIdentities() {
        Memory source, output, inaccessible;
        Setup(source);
        inaccessible.Protect(PAGE_NOACCESS);
        // RBX is deliberately independent: production Dialogue consumes RSI,
        // not RBX. This characterizes that boundary, not game pointer relations.
        for (auto rbx : {output.Address(), source.Address(), std::uintptr_t{0}, inaccessible.Address()}) {
            EvidenceResetDialogue();
            for (float fov : {100.0f, 98.0f, 95.0f, 90.0f, 80.0f, 70.0f}) {
                const auto context = EvidenceDialogueSample(source, output, fov, source.Address(), rbx);
                if (fov == 70.0f) {
                    Check(g_dialoguePhase == Phase::Active, "RBX altered Dialogue lifecycle");
                    Near(context.xmm1.f32[0], dialogue::AdaptiveTarget(100.0f), "RSI input / XMM1 output contract");
                    Near(xmm1AtWake, context.xmm1.f32[0], "Final Dialogue wake preceded XMM1 write");
                    Near(ReadFacts().cameraState.evidence.currentOutputFov.value,
                        context.xmm1.f32[0], "Published Dialogue output differs from XMM1");
                }
            }
        }
    }

    void EvidenceDialogueInvalidFov() {
        Memory source, output; Setup(source);
        auto writer = Context(source, output); Writer(writer);
        for (auto phase : {Phase::Inactive, Phase::Candidate, Phase::Active, Phase::Exiting, Phase::RearmPending}) {
            for (float invalid : {std::numeric_limits<float>::quiet_NaN(),
                std::numeric_limits<float>::infinity(), -1.0f, 0.0f, 1.0f, 179.0f}) {
                EvidencePrimeDialogue(source, output, phase);
                const auto priorOutput = ReadFacts().cameraState.evidence.currentOutputFov;
                const auto previous = g_dialoguePrevious;
                const auto sentinel = Context(source, output).xmm1.f32[0];
                const auto sample = EvidenceDialogueSample(source, output, invalid, source.Address(), output.Address());
                Check(sample.xmm1.f32[0] == sentinel, "Invalid Dialogue input modified XMM1");
                Check(g_dialoguePhase == (phase == Phase::Candidate ? Phase::Inactive : phase),
                    "Invalid Dialogue input violated phase-specific reset contract");
                Near(ReadFacts().cameraState.evidence.currentOutputFov.value, priorOutput.value,
                    "Invalid input published a new output value");
                if (phase == Phase::Candidate) {
                    Check(!g_dialogueCandidate.IsPending() && std::isnan(g_dialogueBaseline) &&
                        !g_runtime.snapshotDialogueTargetValid, "Invalid Candidate did not clear context");
                } else if (std::isfinite(previous)) {
                    Near(g_dialoguePrevious, previous, "Invalid input advanced previous sample");
                }
            }
        }
    }

    void EvidenceDialogueInvalidTarget() {
        Memory source, output, inaccessible; Setup(source);
        inaccessible.Protect(PAGE_NOACCESS);
        for (auto phase : {Phase::Inactive, Phase::Candidate, Phase::Active, Phase::Exiting, Phase::RearmPending}) {
            for (float target : {std::numeric_limits<float>::quiet_NaN(),
                std::numeric_limits<float>::infinity(), 1.0f, 179.0f}) {
                EvidencePrimeDialogue(source, output, phase);
                source.Put(0x2C, target);
                const auto incoming = phase == Phase::Active ? 70.0f : phase == Phase::Exiting ? 75.0f : 100.0f;
                const auto sample = EvidenceDialogueSample(source, output, incoming, source.Address(), output.Address());
                Check(!g_runtime.snapshotDialogueTargetValid, "Invalid native target advertised as valid");
                if (phase == Phase::Inactive || phase == Phase::Candidate) {
                    Check(g_dialoguePhase == Phase::Inactive, "Invalid target promoted Candidate");
                    Check(sample.xmm1.f32[0] == Context(source, output).xmm1.f32[0], "Invalid Candidate transformed FOV");
                } else {
                    Check(g_dialoguePhase == phase, "Invalid native target fabricated lifecycle completion");
                    if (phase == Phase::RearmPending)
                        Check(g_dialogueRecoveryRearm.IsPending(), "Invalid target completed re-arm");
                }
            }
            EvidencePrimeDialogue(source, output, phase);
            const float incoming = phase == Phase::Active ? 70.0f : phase == Phase::Exiting ? 75.0f : 100.0f;
            EvidenceDialogueSample(source, output, incoming, inaccessible.Address(), 0);
            Check(!g_runtime.snapshotDialogueTargetValid, "Unreadable target advertised as valid");
            Check(g_dialoguePhase == ((phase == Phase::Inactive || phase == Phase::Candidate || phase == Phase::RearmPending)
                ? Phase::Inactive : phase), "Unreadable foreign source violated phase-specific lifecycle contract");
        }
    }

    void EvidenceDialogueSourceChange() {
        Memory source, output, foreign; Setup(source);
        foreign.Put(0x2C, 70.0f);
        for (auto phase : {Phase::Candidate, Phase::Active, Phase::Exiting, Phase::RearmPending}) {
            for (auto identity : {foreign.Address(), std::uintptr_t{0}}) {
                EvidencePrimeDialogue(source, output, phase);
                const auto incoming = phase == Phase::Candidate ? 98.0f :
                    phase == Phase::Active ? 70.0f : phase == Phase::Exiting ? 75.0f : 100.0f;
                const auto sample = EvidenceDialogueSample(source, output, incoming, identity, source.Address());
                if (phase == Phase::Candidate || phase == Phase::RearmPending) {
                    Check(g_dialoguePhase == Phase::Inactive && !g_dialogueRecoveryRearm.IsPending(),
                        "Foreign/null RSI did not cancel Candidate/RearmPending");
                } else {
                    // Current policy intentionally has no Active/Exiting RSI
                    // cancellation branch. Record that limit, do not label this
                    // assertion proof of foreign-source ownership safety.
                    Check(g_dialoguePhase == phase, "Active/Exiting source-change characterization changed");
                    Check(sample.xmm1.f32[0] != Context(source, output).xmm1.f32[0],
                        "Active/Exiting policy unexpectedly declined valid incoming FOV");
                }
            }
        }
    }

    void EvidenceDialogueReset() {
        Memory source, output; Setup(source);
        for (auto phase : {Phase::Inactive, Phase::Candidate, Phase::Active, Phase::Exiting, Phase::RearmPending}) {
            EvidencePrimeDialogue(source, output, phase);
            const auto owner = &g_runtime;
            EvidenceResetDialogue();
            Check(&g_runtime == owner && g_dialoguePhase == Phase::Inactive &&
                !g_activeDialoguePolicy.IsValid() && !g_dialogueCorrectionActivation.CanCorrect(true) &&
                std::isnan(g_dialogueBaseline) && std::isnan(g_dialoguePrevious) &&
                std::isnan(g_dialogueExitIncomingStart) && !g_dialogueCandidate.IsPending() &&
                !g_dialogueRecoveryRearm.IsPending() && g_runtime.snapshotDialogueSource == 0 &&
                std::isnan(g_runtime.snapshotDialogueTarget.load()) && !g_runtime.snapshotDialogueTargetValid,
                "Production Dialogue reset left lifecycle/context/correction armed");
            EvidenceResetDialogue();
            Check(g_dialoguePhase == Phase::Inactive && !g_cameraCorrectionGate.IsHeldByCurrentThread(),
                "Repeated reset changed terminal state or retained lock");
        }
    }

    void EvidenceDialoguePolicySnapshot(bool reenable) {
        Memory source, output; Setup(source);
        EvidencePrimeDialogue(source, output, Phase::Active);
        if (reenable) {
            Check(ApplyRuntimeSetting(plugin::RuntimeSettingMutation::DialogueEnabled(false), nullptr).accepted,
                "Dialogue disable rejected");
            Check(ApplyRuntimeSetting(plugin::RuntimeSettingMutation::DialogueEnabled(true), nullptr).accepted,
                "Dialogue reenable rejected");
            const auto sample = EvidenceDialogueSample(source, output, 70.0f, source.Address(), output.Address());
            Check(sample.xmm1.f32[0] == Context(source, output).xmm1.f32[0] &&
                !g_dialogueCorrectionActivation.CanCorrect(true), "Reenable resurrected in-flight correction");
        } else {
            const auto epoch = g_activeDialogueSettingsEpoch.load();
            Check(ApplyRuntimeSetting(plugin::RuntimeSettingMutation::Dialogue(config::DialogueZoomPolicy::Disabled), nullptr).accepted,
                "Dialogue policy mutation rejected");
            const auto sample = EvidenceDialogueSample(source, output, 70.0f, source.Address(), output.Address());
            Near(sample.xmm1.f32[0], dialogue::AdaptiveTarget(100.0f), "Selected policy replaced active policy mid-flight");
            // These epochs track Enabled transitions, not policy selection.
            // Policy changes invalidate presentation freshness independently.
            Check(g_activeDialoguePolicy.Value() == config::DialogueZoomPolicy::Adaptive &&
                g_runtimeDialoguePolicy == config::DialogueZoomPolicy::Disabled &&
                g_activeDialogueSettingsEpoch == epoch && g_dialogueSettingsEpoch == epoch,
                "Active/requested policy epoch contract changed");
        }
        EvidenceResetDialogue();
        EvidencePrimeDialogue(source, output, Phase::Active);
        const auto sample = EvidenceDialogueSample(source, output, 70.0f, source.Address(), output.Address());
        Near(sample.xmm1.f32[0], reenable ? dialogue::AdaptiveTarget(100.0f) : 100.0f,
            "New lifecycle did not capture newly selected/enabled correction");
    }

    void EvidenceDialogueExclusion() {
        Memory source, output; Setup(source);
        EvidencePrimeDialogue(source, output, Phase::Active);
        g_exitTargetFov = 100.0f;
        { auto gate = g_cameraCorrectionGate.Acquire(); ArmPostCinematicDialogueExclusion(); }
        const auto sample = EvidenceDialogueSample(source, output, 70.0f, source.Address(), output.Address());
        Check(g_dialoguePhase == Phase::Inactive && g_postCinematicDialogueExclusion.IsActive() &&
            sample.xmm1.f32[0] == Context(source, output).xmm1.f32[0], "Exclusion did not reset/pass through Dialogue");
        { auto gate = g_cameraCorrectionGate.Acquire(); ResetPostCinematicDialogueExclusion("evidence-fixture"); }
        EvidencePrimeDialogue(source, output, Phase::Active);
        g_coordinator = CoordinatorState::CinematicActive;
        const auto cinematic = EvidenceDialogueSample(source, output, 70.0f, source.Address(), output.Address());
        Check(g_dialoguePhase == Phase::Inactive && g_coordinator == CoordinatorState::CinematicActive &&
            cinematic.xmm1.f32[0] == Context(source, output).xmm1.f32[0], "Cinematic ownership did not reset/pass through Dialogue");
    }

    void EvidenceWriterIdentities() {
        Memory source, output, inaccessible; Setup(source);
        inaccessible.Protect(PAGE_NOACCESS);
        std::array<std::uint8_t, 4096> sourceBefore{};
        std::memcpy(sourceBefore.data(), source.bytes, sourceBefore.size());
        for (auto rbx : {output.Address(), source.Address(), std::uintptr_t{0}, inaccessible.Address()}) {
            auto context = Context(source, output); context.rbx = rbx;
            const auto before = context; Writer(context); UnchangedContext(before, context, true);
            Near(context.xmm0.f32[0], camera::HorPlus(90.0f, fixtureViewport, kNativeAspect), "Gameplay transform unexpectedly depends on readable RBX");
            Check(std::memcmp(sourceBefore.data(), source.bytes, sourceBefore.size()) == 0,
                "HorPlus adapter wrote RSI native storage");
        }
        for (auto rsi : {std::uintptr_t{0}, inaccessible.Address()}) {
            auto context = Context(source, output); context.rsi = rsi;
            const auto before = context; Writer(context); UnchangedContext(before, context);
            Near(context.xmm0.f32[0], 90.0f, "Unreadable/null Gameplay RSI transformed");
        }
    }

    void EvidenceWriterInvalidFields() {
        Memory source, output; Setup(source);
        for (float aspect : {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), 0.0f, -1.0f}) {
            source.Put(kAspectOffset, aspect);
            auto context = Context(source, output); const auto before = context;
            Writer(context); UnchangedContext(before, context);
            Near(context.xmm0.f32[0], 90.0f, "Invalid aspect transformed Gameplay input");
        }
        source.Put(kAspectOffset, fixtureViewport);
        for (float incoming : {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), 0.0f, 1.0f, 179.0f}) {
            auto context = Context(source, output, incoming); const auto before = context;
            Writer(context); UnchangedContext(before, context);
        }
    }

    void EvidenceWriterAds() {
        Memory source, output, replacement; Setup(source);
        for (int n = 0; n < 4; ++n) { auto context = Context(source, output); Writer(context); }
        const auto baseline = g_gameplayBaselineStore.Read();
        Check(camera::IsUsableGameplayBaseline(baseline), "No Gameplay baseline before ADS");
        for (float incoming : {60.0f, 70.0f, 90.0f}) {
            auto context = Context(source, output, incoming); const auto before = context;
            Writer(context); UnchangedContext(before, context, true);
            Near(context.xmm0.f32[0], camera::HorPlus(incoming, fixtureViewport, kNativeAspect), "Gameplay FOV change ignored");
            Near(ReadFacts().cameraState.evidence.nativeFov.value, incoming, "Writer native role did not track incoming FOV");
        }
        replacement.Put(kAspectOffset, fixtureViewport); replacement.Put(kFlagsOffset, std::uint8_t{4});
        replacement.Put(kCameraWorldFovOffset, 100.0f);
        auto context = Context(replacement, output, 100.0f); const auto before = context;
        Writer(context); UnchangedContext(before, context, true);
        Check(g_cinematicWriterAssociation.Read().latestWriter.source == replacement.Address(), "Writer identity change not observed");
        Check(g_coordinator == CoordinatorState::Gameplay, "New Gameplay source changed coordinator");
    }

    void EvidenceRuntimeResize() {
        Memory source, output; Setup(source);
        Check(ApplyRuntimeSetting(plugin::RuntimeSettingMutation::Gameplay(config::GameplayMode::RuntimeCombine), nullptr).accepted,
            "RuntimeCombine setting rejected");
        for (float aspect : {16.0f/9.0f, 32.0f/9.0f, 3.0f, 21.0f/9.0f}) {
            fixtureViewport = aspect;
            auto context = Context(source, output); const auto before = context;
            Writer(context); UnchangedContext(before, context, true);
            Near(source.Get<float>(kAspectOffset), aspect, "RuntimeCombine failed immediate viewport synchronization");
            Near(context.xmm0.f32[0], camera::HorPlus(90.0f, aspect, kNativeAspect), "RuntimeCombine resize output");
            Check(source.Get<std::uint8_t>(kFlagsOffset) == 4, "Viewport change altered flags");
        }
    }

    void EvidenceCinematicInvalidEnter() {
        Memory source, output; Setup(source);
        auto context = Context(source, output, std::numeric_limits<float>::quiet_NaN());
        const auto before = context;
        SafeMidHookEntry<&TraceCinematicEnter>(context); UnchangedContext(before, context);
        Check(g_coordinator == CoordinatorState::CinematicActive && g_cinematicSelectionGeneration == 1 &&
            g_cinematicFovApplied && !std::isfinite(g_cinematicTransformedFov.load()) &&
            !g_runtime.cameraTransitionBoundary.cinematicValid,
            "Invalid ENTER fabricated target/evidence or changed observed lifecycle");
    }

    void EvidenceCinematicRejectedExit() {
        Memory source, output; Setup(source);
        auto writer = Context(source, output); Writer(writer);
        const auto beforeFacts = ReadFacts();
        auto context = Context(source, output); const auto before = context;
        SafeMidHookEntry<&TraceCinematicExit>(context); UnchangedContext(before, context);
        Check(g_coordinator == CoordinatorState::Gameplay && !g_cinematicSelectionValid &&
            g_cinematicSelectionGeneration == 0 && !g_postCinematicDialogueExclusion.IsActive(),
            "Rejected EXIT changed coordinator/selection/exclusion");
        Near(ReadFacts().cameraState.evidence.currentOutputFov.value,
            beforeFacts.cameraState.evidence.currentOutputFov.value, "Rejected EXIT altered FOV publication");
    }

    void EvidenceCinematicIndexedExit() {
        Memory source, output; Setup(source);
        auto context = Context(source, output);
        SafeMidHookEntry<&TraceCinematicEnter>(context);
        g_cinematicExitIndexed = true;
        context = Context(source, output);
        context.rbx = 0; context.rax = (std::numeric_limits<std::uintptr_t>::max)();
        const auto before = context;
        SafeMidHookEntry<&TraceCinematicExit>(context); UnchangedContext(before, context);
        Check(g_coordinator != CoordinatorState::CinematicActive && !g_cinematicSelectionValid,
            "Indexed EXIT did not release observed lifecycle");
        // Adapter receives already-loaded XMM0; it must not dereference indexed
        // operand registers. Original operand load is a separate native gate.
    }

    void EvidenceCinematicCancel() {
        Memory source, output; Setup(source);
        g_gameplayRecoveryObservationAvailable = true;
        for (int n = 0; n < 4; ++n) { auto writer = Context(source, output); Writer(writer); }
        auto context = Context(source, output); SafeMidHookEntry<&TraceCinematicEnter>(context);
        SafeMidHookEntry<&ApplyCinematicAspectStore>(context);
        context = Context(source, output); Writer(context);
        const auto target = context.xmm0.f32[0];
        Check(ApplyRuntimeSetting(plugin::RuntimeSettingMutation::CinematicsEnabled(false), nullptr).accepted,
            "Cinematic disable rejected");
        context = Context(source, output); const auto before = context; Writer(context);
        UnchangedContext(before, context, true);
        Near(context.xmm0.f32[0], 90.0f, "Disabled Cinematic writer retained correction");
        Check(g_coordinator == CoordinatorState::CinematicActive && !g_activeCinematicsEnabled,
            "Disabling Cinematics lost observation ownership");
        Check(ApplyRuntimeSetting(plugin::RuntimeSettingMutation::CinematicsEnabled(true), nullptr).accepted,
            "Cinematic reenable rejected");
        context = Context(source, output); Writer(context);
        Near(context.xmm0.f32[0], 90.0f, "Reenable resurrected current ENTER correction");
        context = Context(source, output); SafeMidHookEntry<&TraceCinematicExit>(context);
        context = Context(source, output); SafeMidHookEntry<&TraceCinematicEnter>(context);
        Check(g_activeCinematicsEnabled && g_cinematicSelectionGeneration == 2,
            "Next ENTER did not activate newly enabled module");
        Near(context.xmm0.f32[0], target, "Next ENTER target differs after reenable");
    }

    void EvidenceCinematicAspectTargets() {
        Memory source, output, inaccessible; Setup(source);
        inaccessible.Protect(PAGE_NOACCESS);
        auto enter = Context(source, output); SafeMidHookEntry<&TraceCinematicEnter>(enter);
        for (auto target : {std::uintptr_t{0}, inaccessible.Address()}) {
            auto context = Context(source, output); context.rax = target;
            const auto before = context;
            SafeMidHookEntry<&ApplyCinematicAspectStore>(context);
            UnchangedContext(before, context, false, false, true);
            Check(context.rip == before.rip + kCinematicStoreInstructionLength,
                "Refused native aspect store did not resume after replaced instruction");
        }
        g_stopping = true;
        auto context = Context(source, output); const auto before = context;
        SafeMidHookEntry<&TraceCinematicExit>(context); UnchangedContext(before, context);
        Check(g_coordinator == CoordinatorState::CinematicActive, "Stopping EXIT mutated state");
    }

    void EvidenceSettingsKinds() {
        Memory source, output; Setup(source);
        using M = plugin::RuntimeSettingMutation;
        const M valid[] = {M::GameplayEnabled(false), M::Gameplay(config::GameplayMode::RuntimeCombine),
            M::CinematicsEnabled(false), M::CinematicAspect(config::CinematicAspectPolicy::Forced16x9),
            M::CinematicFov(config::CinematicFovMode::NativeHorPlus), M::DialogueEnabled(false),
            M::Dialogue(config::DialogueZoomPolicy::Reduced), M::LocaleCode("uk"), M::AutoLocale(true),
            M::FontSize(18), M::HotkeysEnabled(true), M::GameplayHotkey(VK_F1),
            M::CinematicAspectHotkey(VK_F2), M::CinematicFovHotkey(VK_F3),
            M::DialogueHotkey(VK_F4), M::OverlayToggleHotkey(VK_F5)};
        for (const auto& mutation : valid) {
            Check(ApplyRuntimeSetting(mutation, nullptr).accepted, "Valid setting kind rejected");
            const auto repeated = ApplyRuntimeSetting(mutation, nullptr);
            Check(repeated.accepted && !repeated.changed, "Identical mutation not accepted as no-op");
            Check(!ApplyRuntimeSetting({mutation.kind, std::string("wrong-type")}, nullptr).accepted,
                "Wrong-typed setting accepted");
        }
        plugin::RuntimeSettingsSnapshot settings{};
        Check(ReadRuntimeSettings(settings, nullptr) && !settings.gameplayEnabled &&
            settings.gameplayMode == config::GameplayMode::RuntimeCombine && !settings.cinematicsEnabled &&
            settings.cinematicAspectPolicy == config::CinematicAspectPolicy::Forced16x9 &&
            settings.cinematicFovMode == config::CinematicFovMode::NativeHorPlus && !settings.dialogueEnabled &&
            settings.dialogueZoomPolicy == config::DialogueZoomPolicy::Reduced && settings.overlayLocaleCode == "uk" &&
            settings.overlayLocaleAuto && settings.overlayFontSize == 18 && settings.hotkeysEnabled &&
            settings.gameplayCycleKey == VK_F1 && settings.cinematicCycleKey == VK_F2 &&
            settings.cinematicFovCycleKey == VK_F3 && settings.dialogueCycleKey == VK_F4 && settings.overlayToggleKey == VK_F5,
            "Production readback did not reflect all accepted setting kinds");
        Check(!ApplyRuntimeSetting(M::GameplayHotkey(VK_F5), nullptr).accepted,
            "Conflicting Overlay/camera hotkey accepted");
        // The internal typed compatibility value is still accepted by Apply;
        // visible selectors exclude it and persistence rejects it separately.
        Check(ApplyRuntimeSetting(M::Dialogue(config::DialogueZoomPolicy::Native), nullptr).accepted,
            "Internal Native compatibility value changed acceptance contract");
    }

    void EvidenceSettingsLockOrder() {
        Memory source, output; Setup(source);
        Check(ApplyRuntimeSetting(plugin::RuntimeSettingMutation::Gameplay(config::GameplayMode::RuntimeCombine), nullptr).accepted,
            "Settings mutation rejected");
        std::promise<void> started;
        std::future<plugin::RuntimeSettingsSnapshot> reading;
        {
            auto gate = g_cameraCorrectionGate.Acquire();
            reading = std::async(std::launch::async, [&] {
                started.set_value();
                plugin::RuntimeSettingsSnapshot snapshot{};
                Check(ReadRuntimeSettings(snapshot, nullptr), "Concurrent settings reader rejected");
                return snapshot;
            });
            started.get_future().wait();
            Check(reading.wait_for(std::chrono::milliseconds(30)) == std::future_status::timeout,
                "Settings readback bypassed correction gate");
            Check(g_hotkeyConfigMutex.try_lock(), "Blocked reader acquired hotkey mutex before correction gate");
            g_config.hotkeysEnabled = true;
            g_hotkeyConfigMutex.unlock();
        }
        Check(reading.wait_for(std::chrono::seconds(2)) == std::future_status::ready, "Settings lock-order fixture timed out");
        const auto settings = reading.get();
        Check(settings.hotkeysEnabled && settings.gameplayMode == config::GameplayMode::RuntimeCombine,
            "Reader returned state before serialized mutation commit");
    }

    void EvidencePersistenceKinds() {
        Memory source, output; Setup(source);
        WCHAR path[MAX_PATH]{}; Check(GetTempPathW(MAX_PATH, path) != 0, "GetTempPathW");
        g_configPath = std::filesystem::path(path) /
            (L"native-evidence-config-" + std::to_wstring(GetCurrentProcessId()) + L".ini");
        { std::ofstream file(g_configPath); file << "[Gameplay]\nEnabled = true\nMode = HorPlus\n[Cinematics]\nEnabled = true\nMode = Auto\nFovMode = GameplayHorPlus\n[Dialogue]\nEnabled = true\nMode = Adaptive\n[Overlay]\nLanguage = Auto\nFontSize = 13\nToggleKey = VK_2E\n[Hotkeys]\nEnabled = false\nGameplayCycle = F9\nCinematicCycle = F10\nCinematicFovCycle = F11\nDialogueCycle = F12\n"; Check(bool(file), "Fixture INI write failed"); }
        using M = plugin::RuntimeSettingMutation;
        const M mutations[] = {M::GameplayEnabled(false), M::Gameplay(config::GameplayMode::RuntimeCombine),
            M::CinematicsEnabled(false), M::CinematicAspect(config::CinematicAspectPolicy::Forced16x9),
            M::CinematicFov(config::CinematicFovMode::NativeHorPlus), M::DialogueEnabled(false),
            M::Dialogue(config::DialogueZoomPolicy::Reduced), M::LocaleCode("uk"), M::AutoLocale(true),
            M::FontSize(18), M::HotkeysEnabled(true), M::GameplayHotkey(VK_F1), M::CinematicAspectHotkey(VK_F2),
            M::CinematicFovHotkey(VK_F3), M::DialogueHotkey(VK_F4), M::OverlayToggleHotkey(VK_F5)};
        for (const auto& mutation : mutations)
            Check(PersistRuntimeSetting(mutation, nullptr), "Valid production persistence kind failed");
        config::FeatureConfig persisted{};
        Check(config::LoadFeatureConfig(g_configPath, persisted,
            [](const std::filesystem::path&) { return true; }, [](std::string) {}),
            "Persisted fixture INI could not be parsed");
        Check(!persisted.gameplayEnabled && persisted.gameplayMode == config::GameplayMode::RuntimeCombine &&
            !persisted.cinematicsEnabled && persisted.cinematicAspectPolicy == config::CinematicAspectPolicy::Forced16x9 &&
            persisted.cinematicFovMode == config::CinematicFovMode::NativeHorPlus && !persisted.dialogueEnabled &&
            persisted.dialogueZoomPolicy == config::DialogueZoomPolicy::Reduced && persisted.overlayLocaleAuto &&
            persisted.overlayFontSize == 18 && persisted.hotkeysEnabled && persisted.gameplayCycleKey == VK_F1 &&
            persisted.cinematicCycleKey == VK_F2 && persisted.cinematicFovCycleKey == VK_F3 &&
            persisted.dialogueCycleKey == VK_F4 && persisted.overlayToggleKey == VK_F5,
            "Production persistence wrote wrong sections/keys/values");
        Check(!PersistRuntimeSetting(M::Dialogue(config::DialogueZoomPolicy::Native), nullptr),
            "Native compatibility policy persisted as user-selectable value");
        Check(!g_cameraCorrectionGate.IsHeldByCurrentThread(), "Persistence retained correction gate");
    }

    void EvidenceResetEmpty() {
        Memory source, output; Setup(source);
        g_gameplayRecoveryObservationAvailable = true;
        g_cinematicLifecycleObservationAvailable = true;
        g_cinematicAspectComponentAvailable = true;
        g_cinematicSelectionValid = true;
        g_hotkeyRebindCaptureActive = true;

        const auto logsBeforeDetach = logCalls;
        {
            auto admitted = g_nativeCallbackAdmission.TryEnter();
            Check(static_cast<bool>(admitted), "Detach fixture admission failed");
            // Exercise the actual DllMain dispatcher. This direct invocation
            // tests effects; it does not emulate Windows holding loader lock.
            Check(DllMain(nullptr, DLL_PROCESS_DETACH,
                    reinterpret_cast<void*>(1)) == TRUE &&
                !g_stopping && !g_nativeCallbackAdmission.IsClosed() &&
                g_cameraWriterHookGate && g_cinematicHookGate &&
                g_runtimeInitializationComplete && g_cinematicSelectionValid,
                "Process-termination detach performed runtime teardown");
            Check(DllMain(nullptr, DLL_PROCESS_DETACH, nullptr) == TRUE &&
                DllMain(nullptr, DLL_PROCESS_DETACH, nullptr) == TRUE &&
                g_stopping && g_nativeCallbackAdmission.IsClosed() &&
                !g_cameraWriterHookGate && !g_cinematicHookGate &&
                g_cinematicSelectionValid && g_hotkeyRebindCaptureActive &&
                logCalls == logsBeforeDetach,
                "Detach notification drained/reset resources or logged under loader lock");
        }

        for (bool menuVisible : {false, true})
            for (bool partialImGui : {false, true})
                for (bool queuedToast : {false, true})
                    Check(overlay::RendererOwnershipTestAccess::VerifyEmptyShutdown(
                            menuVisible, partialImGui, queuedToast),
                        "Repeated CPU-only renderer shutdown retained input/ImGui ownership");

        Check(&g_pendingNotifications == &g_runtime.pendingNotifications &&
            &g_notificationMutex == &g_runtime.notificationMutex &&
            &g_nextNotificationId == &g_runtime.nextNotificationId,
            "Notification queue storage escaped the resident Runtime owner");
        const bool overlayEnabled = g_config.overlayEnabled;
        g_config.overlayEnabled = true;
        const auto firstId = g_nextNotificationId;
        std::atomic<unsigned> publishersDone{};
        std::vector<std::thread> publishers;
        for (unsigned producer = 0; producer != 4; ++producer)
            publishers.emplace_back([&] {
                for (unsigned item = 0; item != 128; ++item)
                    plugin::PublishOverlayNotification(
                        plugin::OverlayNotificationKind::SettingChanged,
                        plugin::OverlayNotificationAction::GameplayMode,
                        "test", "", plugin::OverlayNotificationStatus::Applied);
                publishersDone.fetch_add(1, std::memory_order_release);
            });
        std::vector<plugin::OverlayNotification> observed;
        while (publishersDone.load(std::memory_order_acquire) != 4) {
            plugin::DrainOverlayNotifications(observed);
            std::this_thread::yield();
        }
        for (auto& publisher : publishers) publisher.join();
        plugin::DrainOverlayNotifications(observed);
        auto previousId = firstId;
        for (const auto& notification : observed) {
            Check(notification.id > previousId,
                "Concurrent notification delivery duplicated or reordered IDs");
            previousId = notification.id;
        }
        Check(!observed.empty() && g_pendingNotifications.empty() &&
            g_nextNotificationId == firstId + 512,
            "Resident notification queue lost publication identity or failed to drain");
        g_config.overlayEnabled = overlayEnabled;

        // No hooks or workers were installed. This calls real orchestration
        // only for its empty-resource terminal branch, not partial installation.
        ResetAllRuntimeResources();
        Check(g_stopping && !g_runtimeInitializationComplete && !g_cameraWriterHookGate &&
            !g_cinematicHookGate && !g_hotkeyRebindCaptureActive && !g_gameplayRecoveryObservationAvailable &&
            !g_cinematicLifecycleObservationAvailable && !g_cinematicAspectComponentAvailable &&
            !g_dialogueBoundaryHookAvailable && !g_cinematicSelectionValid,
            "Empty-resource reset did not close readiness/gates/capabilities");
        ResetAllRuntimeResources();
        Check(g_stopping && !g_cameraWriterHookGate && !g_cinematicHookGate,
            "Repeated empty reset resurrected readiness");
    }

    void EvidenceThrowingCallback(SafetyHookContext&) { throw std::runtime_error("fixture exception"); }

    void EvidenceExceptionBoundary() {
        Memory source, output; Setup(source);
        auto context = Context(source, output); const auto before = context;
        SafeMidHookEntry<&EvidenceThrowingCallback>(context);
        UnchangedContext(before, context);
        // Exercises the production exception wrapper, not an exception injected
        // into a real camera callback or generated native trampoline.
    }

    void EvidenceNestedDeferral() {
        Memory source, output; Setup(source);
        const auto previous = logCalls;
        {
            auto outer = g_cameraCorrectionGate.Acquire();
            Log("evidence nested outer");
            // The production gate is deliberately non-recursive. A nested
            // logging scope inherits the held gate; it must not acquire it.
            { Log("evidence nested inner"); }
            Check(logCalls == previous, "Nested logging scope drained deferred logs prematurely");
        }
        Check(logCalls == previous + 2 && !logHeldGate, "Outermost gate did not drain logs after release");
        Check(observedLogs[previous] == "evidence nested outer" && observedLogs[previous + 1] == "evidence nested inner",
            "Deferred logging changed FIFO order");
    }

    // Compiler-defined Win64 float(float) ABI. The volatile call target keeps
    // execution at the patched entry; this leaf owns no game memory or state.
    __declspec(noinline) float EvidenceNativeLeaf(float value) noexcept {
        return value * 2.0f + 3.0f;
    }
    unsigned nativeLeafCalls{};
    bool nativeLeafContract{true};
    bool nativeLeafThrows{};
    void EvidenceNativeObserver(SafetyHookContext& context) {
        ++nativeLeafCalls;
        nativeLeafContract &= (context.rsp & 15) == 8 &&
            (reinterpret_cast<std::uintptr_t>(&context) & 15) == 0 &&
            context.trampoline_rsp + 8 == context.rsp && context.xmm0.f32[0] == 4.0f;
        if (nativeLeafThrows) throw std::runtime_error("native leaf observer exception");
        context.xmm0.f32[0] = 5.0f;
    }
    void EvidenceRequireHookOperation(bool success) {
        if (!success) {
            std::cerr << "Local native hook operation failed; terminating owned fixture without unsafe reset\n";
            // Avoid vendor reset/free after a failed protection/disable path.
            ExitProcess(1);
        }
    }
    void EvidenceNativeTrampoline() {
        safetyHookFlushCalls = 0;
        safetyHookFailFlushAt = 0;
        static_assert(sizeof(SafetyHookContext) == 0x198);
        static_assert(offsetof(SafetyHookContext, rsp) == 0x180);
        static_assert(offsetof(SafetyHookContext, trampoline_rsp) == 0x188);
        static_assert(offsetof(SafetyHookContext, rip) == 0x190);
        Check(g_nativeCallbackAdmission.Open(),
            "Native callback admission did not begin in its closed state");
        float (* volatile entry)(float) = &EvidenceNativeLeaf;
        Near(entry(4.0f), 11.0f, "Unhooked compiled leaf result");
        auto result = safetyhook::MidHook::create(&EvidenceNativeLeaf,
            SafeMidHookEntry<&EvidenceNativeObserver>, safetyhook::MidHook::StartDisabled);
        Check(result.has_value(), "Local native hook creation failed");
        auto hook = std::move(*result);
        const auto original = hook.original_bytes();
        Check(!original.empty() && !hook.enabled() && safetyHookFlushCalls == 3,
            "Native stub/trampoline were not synchronized before publication");
        bool passed = true;
        passed &= entry(4.0f) == 11.0f && nativeLeafCalls == 0;
        EvidenceRequireHookOperation(hook.enable().has_value());
        Check(safetyHookFlushCalls == 5, "Enable did not synchronize both executable ranges");
        const float enabledResult = entry(4.0f);
        passed &= hook.enabled() && enabledResult == 13.0f && nativeLeafCalls == 1;
        EvidenceRequireHookOperation(hook.disable().has_value());
        Check(safetyHookFlushCalls == 7, "Disable did not synchronize restored executable bytes");
        const float disabledResult = entry(4.0f);
        const bool restoredBytes = std::memcmp(hook.target(), original.data(),
            original.size()) == 0;
        passed &= !hook.enabled() && disabledResult == 11.0f &&
            nativeLeafCalls == 1 && restoredBytes;
        EvidenceRequireHookOperation(hook.enable().has_value());
        nativeLeafThrows = true;
        const float exceptionResult = entry(4.0f);
        passed &= exceptionResult == 11.0f && nativeLeafCalls == 2;
        EvidenceRequireHookOperation(hook.disable().has_value());
        const bool exceptionRestoredBytes = std::memcmp(hook.target(),
            original.data(), original.size()) == 0;
        passed &= exceptionRestoredBytes;
        hook.reset();
        const float resetResult = entry(4.0f);
        passed &= resetResult == 11.0f && nativeLeafCalls == 2 && nativeLeafContract;
        Check(passed, "Local trampoline context/continuation/enable/disable/exception contract failed");

        safetyHookFailFlushAt = safetyHookFlushCalls + 1;
        auto rejected = safetyhook::MidHook::create(&EvidenceNativeLeaf,
            SafeMidHookEntry<&EvidenceNativeObserver>, safetyhook::MidHook::StartDisabled);
        Check(!rejected && entry(4.0f) == 11.0f,
            "Failed stub synchronization did not refuse publication");
        safetyHookFailFlushAt = 0;
        auto retry = safetyhook::MidHook::create(&EvidenceNativeLeaf,
            SafeMidHookEntry<&EvidenceNativeObserver>, safetyhook::MidHook::StartDisabled);
        Check(retry.has_value(), "Cache-failure retry setup failed");
        safetyHookFailFlushAt = safetyHookFlushCalls + 1;
        Check(!retry->enable() && retry->enabled(),
            "Failed patch synchronization reported success or lost uncertain ownership");
        safetyHookFailFlushAt = 0;
        EvidenceRequireHookOperation(retry->disable().has_value());
        EvidenceRequireHookOperation(retry->enable().has_value());
        safetyHookFailFlushAt = safetyHookFlushCalls + 1;
        Check(!retry->disable() && retry->enabled(),
            "Failed restore synchronization discarded uncertain ownership");
        safetyHookFailFlushAt = 0;
        EvidenceRequireHookOperation(retry->disable().has_value());
        Check(!retry->enabled() && entry(4.0f) == 11.0f,
            "Retry after restore synchronization failure did not recover native execution");
    }

    void EvidenceZoomNativeContinuation() {
        Memory source, output; Setup(source);
        source.Put(0x4C, 2.0f); source.Put(0x50, 1.0f);
        source.Put(0x138, 1.0f); source.Put(0x13C, 2.0f);
        auto* code = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, 4096,
            MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));
        Check(code != nullptr, "Zoom native code allocation failed");
        constexpr std::uint8_t instructions[] = {
            0x56,                         // push rsi
            0x48, 0x89, 0xCE,             // mov rsi, rcx
            0x48, 0x89, 0xC8,             // mov rax, rcx
            0xF3, 0x0F, 0x10, 0x41, 0x4C, // movss xmm0, [rcx+4Ch]
            0xF3, 0x0F, 0x10, 0x49, 0x50, // movss xmm1, [rcx+50h]
            0x0F, 0x2E, 0xC8,             // ucomiss xmm1, xmm0 (Zoom hook site)
            0x0F, 0x97, 0xC0,             // seta al: observe original compare flags
            0x0F, 0xB6, 0xC0,             // movzx eax, al
            0x5E, 0xC3                    // pop rsi; ret
        };
        std::memcpy(code, instructions, sizeof(instructions));
        auto entry = reinterpret_cast<unsigned (*)(void*)>(code);
        constexpr std::size_t compareOffset = 17;
        auto created = safetyhook::MidHook::create(code + compareOffset,
            SafeMidHookEntry<&TraceZoomIn>, safetyhook::MidHook::StartDisabled);
        Check(created.has_value(), "Actual Zoom callback native hook creation failed");
        auto hook = std::move(*created);
        Check(hook.enable().has_value(), "Actual Zoom callback native hook enable failed");
        const auto lower = entry(reinterpret_cast<void*>(source.Address()));
        source.Put(0x4C, 1.0f); source.Put(0x50, 2.0f);
        const auto higher = entry(reinterpret_cast<void*>(source.Address()));
        Check(lower == 0 && higher == 1,
            "Original UCOMISS flags/continuation did not reach the leaf result");
        Check(g_zoomInHits == (diagnostics::Enabled() ? 2u : 0u) &&
            g_runtime.snapshotZoomObservationValid.load(),
            "Native Zoom callback did not execute its actual production observation");
        Check(hook.disable().has_value(), "Actual Zoom callback native hook disable failed");
        hook.reset();
        Check(entry(reinterpret_cast<void*>(source.Address())) == 1,
            "Native leaf result changed after Zoom hook removal");
        VirtualFree(code, 0, MEM_RELEASE);
    }

    void EvidenceSafetyHookLifetimeFaults() {
        Check(safetyhook::SafetyHookTestShutdownTrapBypass(),
            "Shutdown VEH attempted to acquire the mutex or inspect interrupted trap state");
        Memory source, output; Setup(source);
        source.Put(0x4C, 2.0f); source.Put(0x50, 1.0f);
        source.Put(0x138, 1.0f); source.Put(0x13C, 2.0f);
        auto* code = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, 4096,
            MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE));
        Check(code != nullptr, "SafetyHook fault fixture allocation failed");
        constexpr std::uint8_t instructions[] = {
            0x56, 0x48, 0x89, 0xCE, 0x48, 0x89, 0xC8,
            0xF3, 0x0F, 0x10, 0x41, 0x4C, 0xF3, 0x0F, 0x10, 0x49, 0x50,
            0x0F, 0x2E, 0xC8, 0x0F, 0x97, 0xC0, 0x0F, 0xB6, 0xC0, 0x5E, 0xC3
        };
        std::memcpy(code, instructions, sizeof(instructions));
        auto entry = reinterpret_cast<unsigned (*)(void*)>(code);
        auto created = safetyhook::MidHook::create(code + 17,
            SafeMidHookEntry<&TraceZoomIn>, safetyhook::MidHook::StartDisabled);
        Check(created.has_value(), "SafetyHook fault fixture create failed");
        auto hook = std::move(*created);
        safetyHookAllocationReleases.clear();

        safetyHookVirtualProtectCalls = 0;
        safetyHookFailVirtualProtectAt = 3;
        safetyHookFailAfterVirtualProtect = true;
        const auto enableResult = hook.enable();
        const auto postWriteEnableWasSafe = !enableResult && hook.enabled() &&
            safetyHookVirtualProtectCalls >= 3 && entry(reinterpret_cast<void*>(source.Address())) == 0;
        if (!postWriteEnableWasSafe) {
            std::cerr << "N01 failed-enable post-write state was not retained\n";
            ExitProcess(1);
        }

        safetyHookVirtualProtectCalls = 0;
        safetyHookFailVirtualProtectAt = 2;
        safetyHookFailAfterVirtualProtect = false;
        const auto disableResult = hook.disable();
        const auto preWriteDisableWasSafe = !disableResult && hook.enabled() &&
            safetyHookVirtualProtectCalls == 3 && entry(reinterpret_cast<void*>(source.Address())) == 0;
        if (!preWriteDisableWasSafe) {
            std::cerr << "N01 failed-disable pre-write state was not retained\n";
            ExitProcess(1);
        }

        auto moved = std::move(hook);
        const auto movePreservedState = moved.enabled() && !hook &&
            entry(reinterpret_cast<void*>(source.Address())) == 0;
        if (!movePreservedState) {
            std::cerr << "N01 move lost patched executable ownership\n";
            ExitProcess(1);
        }

        safetyHookVirtualProtectCalls = 0;
        safetyHookFailVirtualProtectAt = 2;
        safetyHookFailAfterVirtualProtect = false;
        moved.reset();
        const auto afterOwnerDestruction = entry(reinterpret_cast<void*>(source.Address()));
        const auto retainedBlocks = std::count_if(safetyHookAllocationReleases.begin(),
            safetyHookAllocationReleases.end(), [](const auto& event) { return !event.returned; });
        if (afterOwnerDestruction != 0 || retainedBlocks < 2) {
            std::cerr << "N01 reset/destruction released the live stub or trampoline\n";
            ExitProcess(1);
        }
        std::cout << "safetyhook_faults=postwrite-enable,prewrite-disable,move,reset,destructor; "
            << "retained_executable_allocations=" << retainedBlocks << '\n';
        ExitProcess(0); // The target remains patched; child termination is the safe terminal state.
    }

    void EvidenceSafetyHookProtectionRecovery() {
        auto* code = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, 4096,
            MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
        Check(code != nullptr, "Protection recovery fixture allocation failed");
        std::memset(code, 0x90, 32);
        constexpr std::uint8_t leaf[] = {0xB8, 0x07, 0, 0, 0, 0xC3};
        std::memcpy(code, leaf, sizeof(leaf));
        DWORD previous{};
        Check(VirtualProtect(code, 4096, PAGE_EXECUTE_READ, &previous) != FALSE,
            "Protection recovery fixture could not publish RX code");
        const auto entry = reinterpret_cast<int(*)()>(code);
        Check(entry() == 7, "Protection recovery fixture did not execute");
        const auto protection = [&] {
            MEMORY_BASIC_INFORMATION mbi{};
            Check(VirtualQuery(code, &mbi, sizeof(mbi)) != 0,
                "Protection recovery target query failed");
            return mbi.Protect;
        };
        auto created = safetyhook::MidHook::create(code,
            +[](safetyhook::Context&) {}, safetyhook::MidHook::StartDisabled);
        Check(created.has_value(), "Protection recovery hook creation failed");
        auto hook = std::move(*created);

        safetyHookVirtualProtectCalls = 0;
        safetyHookFailVirtualProtectAt = 2;
        safetyHookFailVirtualProtectAtSecond = 3;
        safetyHookFailAfterVirtualProtect = false;
        const auto failedEnable = hook.enable();
        safetyHookFailVirtualProtectAt = 0;
        safetyHookFailVirtualProtectAtSecond = 0;
        Check(!failedEnable && safetyHookVirtualProtectCalls == 4 &&
            protection() == PAGE_EXECUTE_READ && entry() == 7,
            "Real pre-write failures did not restore RX target after partial rollback");

        safetyHookVirtualProtectCalls = 0;
        safetyHookFailVirtualProtectAt = 3;
        const auto failedRestore = hook.enable();
        safetyHookFailVirtualProtectAt = 0;
        Check(!failedRestore && safetyHookVirtualProtectCalls == 5 &&
            protection() == PAGE_EXECUTE_READ && entry() == 7 && hook.enabled(),
            "Real restore failure did not recover page permissions and uncertain hook state");
        Check(hook.disable().has_value() && protection() == PAGE_EXECUTE_READ && entry() == 7,
            "Hook did not recover after a protection restore retry");
        hook.reset();
        VirtualFree(code, 0, MEM_RELEASE);
    }
    void EvidenceSafetyHookVehAdmission() {
        Check(safetyhook::SafetyHookTestVehAdmission(),
            "VEH admitted unrelated read/write AV or rejected an owned execute trap");
        Check(safetyhook::SafetyHookTestVehTransactionIsolation(),
            "VEH reused a completed route or retry ticket across a new transaction/address generation");
    }
    void EvidenceSafetyHookAllocationOwnership() {
        auto allocator = safetyhook::Allocator::create();
        safetyHookInjectedAllocationAddress = nullptr;
        safetyHookInjectedAllocationSize = 0;
        safetyHookInjectOwnedAllocationBadAlloc = true;
        bool threw = false;
        try {
            (void)allocator->allocate(0x180);
        } catch (const std::bad_alloc&) {
            threw = true;
        }
        safetyHookInjectOwnedAllocationBadAlloc = false;
        Check(threw && safetyHookInjectedAllocationAddress &&
            safetyHookInjectedAllocationSize >= 0x180,
            "Allocation fault injection did not occur after the region had an RAII owner");
        MEMORY_BASIC_INFORMATION released{};
        Check(VirtualQuery(safetyHookInjectedAllocationAddress, &released, sizeof(released)) != 0 &&
            released.State == MEM_FREE,
            "bad_alloc after VirtualAlloc left its reservation committed/reserved");

        auto successful = allocator->allocate(0x180);
        Check(successful.has_value(), "Allocator failed after recovering from injected bad_alloc");
        MEMORY_BASIC_INFORMATION owned{};
        Check(VirtualQuery(successful->data(), &owned, sizeof(owned)) != 0 &&
            owned.State == MEM_COMMIT && owned.Protect == PAGE_READWRITE,
            "Successful reservation was not retained by allocator metadata");
    }
    void EvidenceSafetyHookDelayedExecute() {
        auto* code = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, 4096,
            MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
        Check(code != nullptr, "Delayed execute fixture allocation failed");
        constexpr std::uint8_t leaf[] = {0xB8, 0x07, 0, 0, 0, 0xC3};
        std::memcpy(code, leaf, sizeof(leaf));
        DWORD previous{};
        Check(VirtualProtect(code, 4096, PAGE_EXECUTE_READ, &previous) != FALSE,
            "Delayed execute fixture could not publish its leaf");
        auto entry = reinterpret_cast<unsigned(*)()>(code);
        auto created = safetyhook::MidHook::create(code,
            +[](SafetyHookContext&) {}, safetyhook::MidHook::StartDisabled);
        Check(created.has_value(), "Delayed execute hook creation failed");
        auto hook = std::move(*created);

        safetyHookNxReadyEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        safetyHookFaultsEnteredEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        safetyHookReleaseFaultsEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        safetyHookRxRestoredEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!safetyHookNxReadyEvent || !safetyHookFaultsEnteredEvent ||
            !safetyHookReleaseFaultsEvent || !safetyHookRxRestoredEvent) {
            hook.reset();
            VirtualFree(code, 0, MEM_RELEASE);
            throw std::runtime_error("Delayed execute barrier creation failed");
        }
        safetyHookDelayedFaultAddress.store(reinterpret_cast<std::uintptr_t>(code),
            std::memory_order_release);
        safetyHookDelayedFaultCount.store(0, std::memory_order_release);
        safetyHookDelayedFaultTimedOut.store(false, std::memory_order_release);
        safetyHookExpectedDelayedFaults = 2;
        safetyHookDelayExecuteFault.store(true, std::memory_order_release);

        SafetyHookDelayedExecuteCall calls[2]{{entry, safetyHookNxReadyEvent},
            {entry, safetyHookNxReadyEvent}};
        HANDLE workers[2]{
            CreateThread(nullptr, 0, SafetyHookDelayedExecuteWorker, &calls[0], 0, nullptr),
            CreateThread(nullptr, 0, SafetyHookDelayedExecuteWorker, &calls[1], 0, nullptr)};
        bool workersCreated = workers[0] && workers[1];
        if (!workersCreated) {
            SetEvent(safetyHookNxReadyEvent);
            SetEvent(safetyHookReleaseFaultsEvent);
            for (auto worker : workers) if (worker) {
                WaitForSingleObject(worker, 3000);
                CloseHandle(worker);
            }
            safetyHookDelayExecuteFault.store(false, std::memory_order_release);
            hook.reset();
            VirtualFree(code, 0, MEM_RELEASE);
            CloseHandle(safetyHookNxReadyEvent);
            CloseHandle(safetyHookFaultsEnteredEvent);
            CloseHandle(safetyHookReleaseFaultsEvent);
            CloseHandle(safetyHookRxRestoredEvent);
            safetyHookNxReadyEvent = safetyHookFaultsEnteredEvent =
                safetyHookReleaseFaultsEvent = safetyHookRxRestoredEvent = nullptr;
            throw std::runtime_error("Delayed execute worker creation failed");
        }

        const auto enabled = hook.enable();
        const bool restored = WaitForSingleObject(safetyHookRxRestoredEvent, 1000) == WAIT_OBJECT_0;
        SetEvent(safetyHookReleaseFaultsEvent);
        const bool workersExited = WaitForMultipleObjects(2, workers, TRUE, 5000) == WAIT_OBJECT_0;
        if (!workersExited)
            ExitProcess(2); // The process-tree supervisor owns this isolated fixture process.
        DWORD workerCodes[2]{1, 1};
        if (workersExited) {
            GetExitCodeThread(workers[0], &workerCodes[0]);
            GetExitCodeThread(workers[1], &workerCodes[1]);
        }
        for (auto worker : workers) CloseHandle(worker);
        const auto faultCount = safetyHookDelayedFaultCount.load(std::memory_order_acquire);
        const bool timedOut = safetyHookDelayedFaultTimedOut.load(std::memory_order_acquire);
        safetyHookDelayExecuteFault.store(false, std::memory_order_release);
        hook.reset();
        const auto originalResult = entry();
        VirtualFree(code, 0, MEM_RELEASE);
        CloseHandle(safetyHookNxReadyEvent);
        CloseHandle(safetyHookFaultsEnteredEvent);
        CloseHandle(safetyHookReleaseFaultsEvent);
        CloseHandle(safetyHookRxRestoredEvent);
        safetyHookNxReadyEvent = safetyHookFaultsEnteredEvent =
            safetyHookReleaseFaultsEvent = safetyHookRxRestoredEvent = nullptr;
        Check(enabled.has_value() && restored && workersExited && !timedOut &&
            faultCount == 2 && workerCodes[0] == 0 && workerCodes[1] == 0 &&
            calls[0].result == 7 && calls[1].result == 7 &&
            originalResult == 7,
            "Two execute faults held before VEH admission were not safely retried after RX restoration/retirement");
    }

    void EvidenceSafetyHookRetryGenerations() {
        auto* code = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, 4096,
            MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
        Check(code != nullptr, "Retry generation fixture allocation failed");
        constexpr std::uint8_t leaf[] = {0xB8, 0x07, 0, 0, 0, 0xC3};
        std::memcpy(code, leaf, sizeof(leaf));
        DWORD previous{};
        Check(VirtualProtect(code, 4096, PAGE_EXECUTE_READ, &previous) != FALSE,
            "Retry generation fixture could not publish RX code");
        auto entry = reinterpret_cast<unsigned(*)()>(code);
        auto created = safetyhook::MidHook::create(code,
            +[](SafetyHookContext&) {}, safetyhook::MidHook::StartDisabled);
        Check(created.has_value(), "Retry generation hook creation failed");
        auto hook = std::move(*created);

        safetyHookNxReadyEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        safetyHookFaultsEnteredEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        safetyHookReleaseFaultsEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        safetyHookRxRestoredEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        HANDLE firstCallDone = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        HANDLE secondCallStart = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!safetyHookNxReadyEvent || !safetyHookFaultsEnteredEvent ||
            !safetyHookReleaseFaultsEvent || !safetyHookRxRestoredEvent ||
            !firstCallDone || !secondCallStart)
            throw std::runtime_error("Retry generation barrier creation failed");

        SafetyHookRetryGenerationCall call{entry, safetyHookNxReadyEvent,
            firstCallDone, secondCallStart};
        HANDLE worker = CreateThread(nullptr, 0, SafetyHookRetryGenerationWorker,
            &call, 0, nullptr);
        if (!worker) throw std::runtime_error("Retry generation worker creation failed");

        safetyHookDelayedFaultAddress.store(reinterpret_cast<std::uintptr_t>(code),
            std::memory_order_release);
        safetyHookDelayedFaultCount.store(0, std::memory_order_release);
        safetyHookExpectedDelayedFaults = 1;
        safetyHookDelayedFaultTimedOut.store(false, std::memory_order_release);
        safetyHookVirtualProtectCalls = 0;
        safetyHookFailVirtualProtectAt = 0;
        safetyHookDelayExecuteFault.store(true, std::memory_order_release);

        const auto firstEnable = hook.enable();
        const bool firstRestored = WaitForSingleObject(safetyHookRxRestoredEvent, 1000) == WAIT_OBJECT_0;
        SetEvent(safetyHookReleaseFaultsEvent);
        const bool firstCompleted = WaitForSingleObject(firstCallDone, 5000) == WAIT_OBJECT_0;
        const auto firstFaults = safetyHookDelayedFaultCount.load(std::memory_order_acquire);
        safetyHookDelayExecuteFault.store(false, std::memory_order_release);
        const auto firstDisable = hook.disable();
        if (!firstCompleted) ExitProcess(3);

        ResetEvent(safetyHookNxReadyEvent);
        ResetEvent(safetyHookFaultsEnteredEvent);
        ResetEvent(safetyHookReleaseFaultsEvent);
        ResetEvent(safetyHookRxRestoredEvent);
        safetyHookDelayedFaultCount.store(0, std::memory_order_release);
        safetyHookExpectedDelayedFaults = 1;
        safetyHookDelayExecuteFault.store(true, std::memory_order_release);
        SetEvent(secondCallStart);

        const auto secondEnable = hook.enable();
        const bool secondRestored = WaitForSingleObject(safetyHookRxRestoredEvent, 1000) == WAIT_OBJECT_0;
        SetEvent(safetyHookReleaseFaultsEvent);
        const bool workerExited = WaitForSingleObject(worker, 5000) == WAIT_OBJECT_0;
        if (!workerExited) ExitProcess(4);
        DWORD workerCode{1};
        GetExitCodeThread(worker, &workerCode);
        const auto secondFaults = safetyHookDelayedFaultCount.load(std::memory_order_acquire);
        const bool timedOut = safetyHookDelayedFaultTimedOut.load(std::memory_order_acquire);

        safetyHookDelayExecuteFault.store(false, std::memory_order_release);
        const auto secondDisable = hook.disable();
        hook.reset();
        CloseHandle(worker);
        for (auto event : {safetyHookNxReadyEvent, safetyHookFaultsEnteredEvent,
                 safetyHookReleaseFaultsEvent, safetyHookRxRestoredEvent,
                 firstCallDone, secondCallStart}) CloseHandle(event);
        safetyHookNxReadyEvent = safetyHookFaultsEnteredEvent =
            safetyHookReleaseFaultsEvent = safetyHookRxRestoredEvent = nullptr;
        VirtualFree(code, 0, MEM_RELEASE);

        Check(firstEnable && firstDisable && secondEnable && secondDisable &&
            firstRestored && secondRestored && firstCompleted && workerCode == 0 &&
            call.results[0] == 7 && call.results[1] == 7 &&
            firstFaults == 1 && secondFaults == 1 && !timedOut,
            "A persistent worker's retry ticket suppressed the same-address execute fault in a later transaction");
    }

    void EvidenceSafetyHookPrewriteRouteLifetime() {
        SYSTEM_INFO allocationInfo{};
        GetSystemInfo(&allocationInfo);
        constexpr std::uintptr_t maxRelativeDistance = 0x7fff0000;
        const auto granularity = static_cast<std::uintptr_t>(allocationInfo.dwAllocationGranularity);
        const auto anchor = reinterpret_cast<std::uintptr_t>(&SafetyHookNativeRouteBarrier);
        const auto alignedAnchor = anchor & ~(granularity - 1);
        std::uint8_t* code{};
        for (std::uintptr_t distance = granularity; distance < maxRelativeDistance && !code;
            distance += granularity) {
            const std::uintptr_t candidates[] = {alignedAnchor - distance, alignedAnchor + distance};
            for (const auto candidate : candidates) {
                const auto delta = static_cast<std::int64_t>(anchor) -
                    static_cast<std::int64_t>(candidate + 9);
                if (delta < INT32_MIN || delta > INT32_MAX) continue;
                code = static_cast<std::uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(candidate),
                    4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
                if (code) break;
            }
        }
        Check(code != nullptr, "Pre-write lifetime fixture allocation failed");
        std::uint8_t leaf[] = {0x48, 0x83, 0xEC, 0x28, 0xE8, 0, 0, 0, 0,
            0x48, 0x83, 0xC4, 0x28, 0xB8, 0x07, 0, 0, 0, 0xC3};
        std::memcpy(code, leaf, sizeof(leaf));
        const auto callNext = reinterpret_cast<std::uintptr_t>(code + 9);
        const auto callDelta = static_cast<std::int32_t>(
            static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(&SafetyHookNativeRouteBarrier)) -
            static_cast<std::int64_t>(callNext));
        std::memcpy(code + 5, &callDelta, sizeof(callDelta));
        std::memcpy(leaf + 5, &callDelta, sizeof(callDelta));
        DWORD previous{};
        Check(VirtualProtect(code, 4096, PAGE_EXECUTE_READ, &previous) != FALSE,
            "Pre-write lifetime fixture could not publish RX code");
        auto entry = reinterpret_cast<unsigned(*)()>(code);
        auto created = safetyhook::MidHook::create(code,
            SafeMidHookEntry<&SafetyHookNoopMidCallback>,
            safetyhook::MidHook::StartDisabled);
        Check(created.has_value(), "Pre-write lifetime hook creation failed");
        auto hook = std::move(*created);
        auto* const trampoline = hook.test_trampoline();
        auto* const stub = hook.test_stub();

        safetyHookNxReadyEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        safetyHookPrewriteProtectionChangedEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        safetyHookContinuePrewriteProtectionEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        safetyHookRouteObservedEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        safetyHookReleaseRouteEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        HANDLE enableDone = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!safetyHookNxReadyEvent || !safetyHookPrewriteProtectionChangedEvent ||
            !safetyHookContinuePrewriteProtectionEvent || !safetyHookRouteObservedEvent ||
            !safetyHookReleaseRouteEvent || !enableDone)
            throw std::runtime_error("Pre-write lifetime barrier creation failed");

        SafetyHookDelayedExecuteCall workerCall{entry, safetyHookNxReadyEvent};
        safetyHookAllocationReleases.clear();
        safetyHookVirtualProtectCalls = 0;
        safetyHookFailVirtualProtectAt = 2; // The trampoline page fails before its first change/write.
        safetyHookFailVirtualProtectAtSecond = 0;
        safetyHookFailAfterVirtualProtect = false;
        safetyHookPrewriteTarget = reinterpret_cast<std::uintptr_t>(code);
        safetyHookPrewriteProtectionTimedOut.store(false, std::memory_order_release);
        safetyHookRouteCallbackTimedOut.store(false, std::memory_order_release);
        safetyHookHoldPrewriteProtection.store(true, std::memory_order_release);
        HANDLE worker = CreateThread(nullptr, 0, SafetyHookDelayedExecuteWorker,
            &workerCall, 0, nullptr);
        if (!worker) throw std::runtime_error("Pre-write lifetime worker creation failed");
        SafetyHookEnableThreadCall enableCall{&hook, enableDone};
        HANDLE transaction = CreateThread(nullptr, 0, SafetyHookEnableThread,
            &enableCall, 0, nullptr);
        if (!transaction) ExitProcess(5);

        const bool sourceBecameNx = WaitForSingleObject(
            safetyHookPrewriteProtectionChangedEvent, 5000) == WAIT_OBJECT_0;
        const bool workerWasRedirected = sourceBecameNx &&
            WaitForSingleObject(safetyHookRouteObservedEvent, 5000) == WAIT_OBJECT_0;
        if (!workerWasRedirected) ExitProcess(6);
        SetEvent(safetyHookContinuePrewriteProtectionEvent);
        if (WaitForSingleObject(transaction, 5000) != WAIT_OBJECT_0) ExitProcess(7);
        CloseHandle(transaction);
        safetyHookHoldPrewriteProtection.store(false, std::memory_order_release);
        safetyHookFailVirtualProtectAt = 0;

        MEMORY_BASIC_INFORMATION sourceProtection{}, trampolineBefore{}, stubBefore{};
        const bool sourceQueryable = VirtualQuery(code, &sourceProtection, sizeof(sourceProtection)) != 0;
        const bool sourceProtectionRestored = sourceQueryable && sourceProtection.State == MEM_COMMIT &&
            sourceProtection.Protect == PAGE_EXECUTE_READ;
        const bool sourceBytesRestored = std::memcmp(code, leaf, sizeof(leaf)) == 0;
        const bool enableRejected = !enableCall.succeeded;
        const bool rolledBack = sourceProtectionRestored && sourceBytesRestored && enableRejected;
        const bool retainedBeforeReset = VirtualQuery(trampoline, &trampolineBefore,
            sizeof(trampolineBefore)) != 0 && VirtualQuery(stub, &stubBefore, sizeof(stubBefore)) != 0 &&
            trampolineBefore.State == MEM_COMMIT && trampolineBefore.Protect == PAGE_EXECUTE_READ &&
            stubBefore.State == MEM_COMMIT && stubBefore.Protect == PAGE_EXECUTE_READ;
        hook.reset();
        MEMORY_BASIC_INFORMATION trampolineAfter{}, stubAfter{};
        const bool retainedAfterDestroy = VirtualQuery(trampoline, &trampolineAfter,
            sizeof(trampolineAfter)) != 0 && VirtualQuery(stub, &stubAfter, sizeof(stubAfter)) != 0 &&
            trampolineAfter.State == MEM_COMMIT && trampolineAfter.Protect == PAGE_EXECUTE_READ &&
            stubAfter.State == MEM_COMMIT && stubAfter.Protect == PAGE_EXECUTE_READ;
        const auto retainedAllocations = std::count_if(safetyHookAllocationReleases.begin(),
            safetyHookAllocationReleases.end(), [](const auto& release) { return !release.returned; });

        SetEvent(safetyHookReleaseRouteEvent);
        const bool workerExited = WaitForSingleObject(worker, 5000) == WAIT_OBJECT_0;
        if (!workerExited) ExitProcess(8);
        DWORD workerCode{1};
        GetExitCodeThread(worker, &workerCode);
        CloseHandle(worker);
        safetyHookRouteObservedEvent = safetyHookReleaseRouteEvent = nullptr;
        safetyHookPrewriteProtectionChangedEvent =
            safetyHookContinuePrewriteProtectionEvent = nullptr;
        CloseHandle(enableDone);
        CloseHandle(safetyHookNxReadyEvent);
        safetyHookNxReadyEvent = nullptr;
        safetyHookPrewriteTarget = 0;
        VirtualFree(code, 0, MEM_RELEASE);

        Check(sourceBecameNx && workerWasRedirected && rolledBack && retainedBeforeReset &&
            retainedAfterDestroy && retainedAllocations >= 2 && workerCode == 0 &&
            workerCall.result == 7 && !safetyHookPrewriteProtectionTimedOut.load(
                std::memory_order_acquire) && !safetyHookRouteCallbackTimedOut.load(
                std::memory_order_acquire),
            "Pre-write rollback/destruction released a continuation while a redirected worker was using it");
    }
    void EvidenceSafetyHookRxPublication() {
        auto makeLeaf = [](unsigned result) {
            auto* code = static_cast<std::uint8_t*>(VirtualAlloc(nullptr, 4096,
                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
            Check(code != nullptr, "RX publication fixture allocation failed");
            std::memset(code, 0x90, 32);
            code[0] = 0xB8;
            std::memcpy(code + 1, &result, sizeof(result));
            code[5] = 0xC3;
            Check(FlushInstructionCache(GetCurrentProcess(), code, 32) != FALSE,
                "RX publication fixture code sync failed");
            DWORD previous{};
            Check(VirtualProtect(code, 4096, PAGE_EXECUTE_READ, &previous) != FALSE,
                "RX publication fixture code protection failed");
            return code;
        };
        auto* first = makeLeaf(7);
        auto* second = makeLeaf(9);
        auto permission = [](const void* address) {
            MEMORY_BASIC_INFORMATION mbi{};
            Check(VirtualQuery(address, &mbi, sizeof(mbi)) != 0 && mbi.State == MEM_COMMIT,
                "Published allocation is no longer committed");
            return mbi.Protect;
        };
        auto firstHook = safetyhook::MidHook::create(first,
            +[](safetyhook::Context&) {}, safetyhook::MidHook::StartDisabled);
        auto secondHook = safetyhook::MidHook::create(second,
            +[](safetyhook::Context&) {}, safetyhook::MidHook::StartDisabled);
        Check(firstHook && secondHook, "Two independent hooks could not be created");
        const auto firstStub = firstHook->test_stub();
        const auto firstTrampoline = firstHook->test_trampoline();
        const auto secondStub = secondHook->test_stub();
        const auto secondTrampoline = secondHook->test_trampoline();
        SYSTEM_INFO info{};
        GetSystemInfo(&info);
        MEMORY_BASIC_INFORMATION stubRegion{}, trampolineRegion{};
        Check(VirtualQuery(firstStub, &stubRegion, sizeof(stubRegion)) != 0 &&
            VirtualQuery(firstTrampoline, &trampolineRegion, sizeof(trampolineRegion)) != 0 &&
            stubRegion.RegionSize == info.dwPageSize &&
            trampolineRegion.RegionSize == info.dwPageSize,
            "Private executable allocations committed more than one page per small hook");
        Check(firstStub != secondStub && firstTrampoline != secondTrampoline &&
            permission(firstStub) == PAGE_EXECUTE_READ &&
            permission(firstTrampoline) == PAGE_EXECUTE_READ &&
            permission(secondStub) == PAGE_EXECUTE_READ &&
            permission(secondTrampoline) == PAGE_EXECUTE_READ,
            "Stub/trampoline allocations were shared or remained writable after publication");
        Check(firstHook->enable() && secondHook->enable(),
            "Two RX hooks could not be enabled together");
        Check(reinterpret_cast<unsigned(*)()>(first)() == 7 &&
            reinterpret_cast<unsigned(*)()>(second)() == 9 &&
            permission(firstStub) == PAGE_EXECUTE_READ &&
            permission(firstTrampoline) == PAGE_EXECUTE_READ &&
            permission(secondStub) == PAGE_EXECUTE_READ &&
            permission(secondTrampoline) == PAGE_EXECUTE_READ,
            "Enabled hook execution or RX permissions changed");
        Check(firstHook->disable() && secondHook->disable() &&
            permission(firstStub) == PAGE_EXECUTE_READ &&
            permission(firstTrampoline) == PAGE_EXECUTE_READ,
            "Disabling the hooks left a trampoline writable");
        firstHook->reset();
        secondHook->reset();
        Check(permission(firstStub) == PAGE_EXECUTE_READ &&
            permission(firstTrampoline) == PAGE_EXECUTE_READ,
            "Retained continuation lost RX protection after reset");
        auto recreated = safetyhook::MidHook::create(first,
            +[](safetyhook::Context&) {}, safetyhook::MidHook::StartDisabled);
        Check(recreated && recreated->test_stub() != firstStub &&
            permission(recreated->test_stub()) == PAGE_EXECUTE_READ &&
            permission(recreated->test_trampoline()) == PAGE_EXECUTE_READ,
            "Recreated hook reused published RX storage");
        Check(recreated->enable() && reinterpret_cast<unsigned(*)()>(first)() == 7 &&
            recreated->disable(), "Recreated RX hook failed execution or teardown");
        recreated->reset();
        VirtualFree(first, 0, MEM_RELEASE);
        VirtualFree(second, 0, MEM_RELEASE);
    }

    void EvidenceSafetyHookFailedCreate() {
        Memory source, output; Setup(source);
        safetyHookRefuseVeh = true;
        safetyHookVirtualProtectCalls = 0;
        const auto missingHandler = safetyhook::MidHook::create(&EvidenceNativeLeaf,
            SafeMidHookEntry<&EvidenceNativeObserver>);
        Check(!missingHandler && safetyHookVirtualProtectCalls == 0,
            "Missing VEH did not refuse patching before protection changes");
        safetyHookRefuseVeh = false;
        safetyHookAllocationReleases.clear();
        safetyHookVirtualProtectCalls = 0;
        safetyHookFailVirtualProtectAt = 3;
        safetyHookFailAfterVirtualProtect = true;
        const auto result = safetyhook::MidHook::create(&EvidenceNativeLeaf,
            SafeMidHookEntry<&EvidenceNativeObserver>);
        safetyHookFailVirtualProtectAt = 0;
        const auto retained = std::count_if(safetyHookAllocationReleases.begin(),
            safetyHookAllocationReleases.end(), [](const auto& event) { return !event.returned; });
        const auto postCreateValue = EvidenceNativeLeaf(4.0f);
        if (result || postCreateValue != 11.0f || retained < 2)
            std::cerr << "failed-create state: result=" << static_cast<bool>(result)
                << " value=" << postCreateValue << " retained=" << retained
                << " protects=" << safetyHookVirtualProtectCalls << '\n';
        Check(!result && postCreateValue == 11.0f && retained >= 2,
            "Failed default-enabled create lost executable ownership or failed to restore target");

        safetyHookAllocationReleases.clear();
        safetyHookVirtualProtectCalls = 0;
        safetyHookFailVirtualProtectAt = 1;
        safetyHookFailAfterVirtualProtect = false;
        const auto prewrite = safetyhook::MidHook::create(&EvidenceNativeLeaf,
            SafeMidHookEntry<&EvidenceNativeObserver>);
        safetyHookFailVirtualProtectAt = 0;
        const auto reusable = std::count_if(safetyHookAllocationReleases.begin(),
            safetyHookAllocationReleases.end(), [](const auto& event) { return event.returned; });
        Check(!prewrite && EvidenceNativeLeaf(4.0f) == 11.0f && reusable >= 2,
            "Pre-write failed create did not safely release unreferenced storage");
    }

    void EvidenceSafetyHookDisablePostWriteFailure() {
        Memory source, output; Setup(source);
        auto result = safetyhook::MidHook::create(&EvidenceNativeLeaf,
            SafeMidHookEntry<&EvidenceNativeObserver>, safetyhook::MidHook::StartDisabled);
        Check(result.has_value(), "Disable post-write fixture create failed");
        auto hook = std::move(*result);
        safetyHookAllocationReleases.clear();
        Check(hook.enable().has_value() && EvidenceNativeLeaf(4.0f) == 13.0f,
            "Disable post-write fixture did not activate");
        safetyHookVirtualProtectCalls = 0;
        safetyHookFailVirtualProtectAt = 3;
        safetyHookFailAfterVirtualProtect = true;
        const auto disable = hook.disable();
        safetyHookFailVirtualProtectAt = 0;
        const auto retained = std::count_if(safetyHookAllocationReleases.begin(),
            safetyHookAllocationReleases.end(), [](const auto& event) { return !event.returned; });
        Check(!disable && !hook.enabled() && EvidenceNativeLeaf(4.0f) == 11.0f && retained == 0,
            "Post-write disable did not distinguish restored bytes from protection failure");
        hook.reset();
        const auto retainedAfterReset = std::count_if(safetyHookAllocationReleases.begin(),
            safetyHookAllocationReleases.end(), [](const auto& event) { return !event.returned; });
        Check(retainedAfterReset >= 2,
            "Previously executable trampoline/stub was returned while a native frame could remain in flight");
    }

    struct EvidenceSelfResetSignals { HANDLE start{}, done{}; bool retained{}; };
    DWORD WINAPI EvidenceSelfResetWorker(void* value) {
        auto& signals = *static_cast<EvidenceSelfResetSignals*>(value);
        if (WaitForSingleObject(signals.start, 2000) != WAIT_OBJECT_0) return 1;
        ResetAllRuntimeResources();
        signals.retained = g_workers.WorkerCount() == 1 && g_cinematicSelectionValid &&
            g_gameplayRecoveryObservationAvailable && g_dialogueBoundaryHookAvailable &&
            g_stopping && !g_cameraWriterHookGate && !g_cinematicHookGate;
        SetEvent(signals.done);
        return 0;
    }
    void EvidenceResetFailedSelfJoin() {
        Memory source, output; Setup(source);
        g_gameplayRecoveryObservationAvailable = true;
        g_cinematicSelectionValid = true;
        EvidenceSelfResetSignals signals{CreateEventW(nullptr, TRUE, FALSE, nullptr),
            CreateEventW(nullptr, TRUE, FALSE, nullptr)};
        Check(signals.start && signals.done && CreateWorkerStopEvent(), "Self-join fixture event setup failed");
        Check(g_workers.Start(&EvidenceSelfResetWorker, &signals), "Self-join fixture worker start failed");
        // Transfer the externally serialized lifecycle control only after Start
        // returns. The main thread waits and does not overlap reset operations.
        SetEvent(signals.start);
        Check(WaitForSingleObject(signals.done, 3000) == WAIT_OBJECT_0, "Self-reset fixture timed out");
        Check(signals.retained, "Failed production self-join proceeded into resource/capability reset");
        Check(StopWorkers(true) && g_workers.WorkerCount() == 0, "Retained worker could not be joined by control thread");
        ResetAllRuntimeResources();
        Check(!g_cinematicSelectionValid && !g_gameplayRecoveryObservationAvailable && !g_dialogueBoundaryHookAvailable,
            "Reset after successful join did not finish resource branch");
        CloseHandle(signals.start); CloseHandle(signals.done);
    }

    DWORD WINAPI EvidenceStopObserver(void*) {
        return WaitForWorkerStop(3000) ? 0 : 1;
    }
    void EvidenceResetNativeHook() {
        Memory source, output; Setup(source);
        auto result = safetyhook::MidHook::create(&EvidenceNativeLeaf,
            SafeMidHookEntry<&EvidenceNativeObserver>, safetyhook::MidHook::StartDisabled);
        Check(result.has_value(), "Reset fixture local hook creation failed");
        g_hook = std::move(*result);
        const auto original = g_hook.original_bytes();
        EvidenceRequireHookOperation(g_hook.enable().has_value());
        Check(CreateWorkerStopEvent() && g_workers.Start(&EvidenceStopObserver), "Reset fixture worker unavailable");
        float (* volatile entry)(float) = &EvidenceNativeLeaf;
        const auto activeResult = entry(4.0f);
        // Join and reset run on the externally serialized control thread; no
        // concurrent leaf call or game callback occurs during patch removal.
        ResetAllRuntimeResources();
        Check(activeResult == 13.0f && !g_hook && g_workers.WorkerCount() == 0 &&
            entry(4.0f) == 11.0f && std::memcmp(reinterpret_cast<const void*>(&EvidenceNativeLeaf),
                original.data(), original.size()) == 0,
            "Production reset failed to join fixture worker/remove local native hook/restore bytes");
    }

    std::vector<int> installationEvents;
    int installationFailure{};
    bool installationThrows{};
    bool EvidenceInstallStep(int step) {
        installationEvents.push_back(step);
        if (installationFailure != step) return true;
        if (installationThrows) throw std::runtime_error("fixture installer failure");
        return false;
    }
    void EvidenceInstallTransactionOrder() {
        installationEvents.reserve(8);
        for (bool throws : {false, true}) {
            for (int failure : {0, 1, 2, 3}) {
                installationEvents.clear(); installationFailure = failure; installationThrows = throws;
                const auto status = cinematics::InitializeTransactional(true,
                    [] { return EvidenceInstallStep(1); }, [] { return EvidenceInstallStep(2); },
                    []() noexcept { installationEvents.push_back(5); },
                    []() noexcept { installationEvents.push_back(4); },
                    [] { return EvidenceInstallStep(3); });
                const std::vector<int> expected = failure == 0 ? std::vector<int>{1, 2, 3} :
                    failure == 1 ? std::vector<int>{1, 4, 5} :
                    failure == 2 ? std::vector<int>{1, 2, 4, 5} : std::vector<int>{1, 2, 3, 4, 5};
                Check(installationEvents == expected && status == (failure == 0
                    ? plugin::FeatureStatus::Available : plugin::FeatureStatus::Failed),
                    "Production installation transaction violated commit/reverse-rollback order");
            }
        }
        // Actual helper with controlled installers, not production Initialize
        // wiring or game-image installation/rollback proof.
    }

    void NativeEvidence(const std::string& scenario) {
        if (scenario == "evidence-dialogue-identities") EvidenceDialogueIdentities();
        else if (scenario == "evidence-dialogue-invalid-fov") EvidenceDialogueInvalidFov();
        else if (scenario == "evidence-dialogue-invalid-target") EvidenceDialogueInvalidTarget();
        else if (scenario == "evidence-dialogue-source-change") EvidenceDialogueSourceChange();
        else if (scenario == "evidence-dialogue-reset") EvidenceDialogueReset();
        else if (scenario == "evidence-dialogue-policy-snapshot") EvidenceDialoguePolicySnapshot(false);
        else if (scenario == "evidence-dialogue-reenable") EvidenceDialoguePolicySnapshot(true);
        else if (scenario == "evidence-dialogue-exclusion") EvidenceDialogueExclusion();
        else if (scenario == "evidence-writer-identities") EvidenceWriterIdentities();
        else if (scenario == "evidence-writer-invalid-fields") EvidenceWriterInvalidFields();
        else if (scenario == "evidence-writer-ads") EvidenceWriterAds();
        else if (scenario == "evidence-runtime-resize") EvidenceRuntimeResize();
        else if (scenario == "evidence-cinematic-invalid-enter") EvidenceCinematicInvalidEnter();
        else if (scenario == "evidence-cinematic-rejected-exit") EvidenceCinematicRejectedExit();
        else if (scenario == "evidence-cinematic-indexed-exit") EvidenceCinematicIndexedExit();
        else if (scenario == "evidence-cinematic-cancel") EvidenceCinematicCancel();
        else if (scenario == "evidence-cinematic-aspect-targets") EvidenceCinematicAspectTargets();
        else if (scenario == "evidence-settings-kinds") EvidenceSettingsKinds();
        else if (scenario == "evidence-settings-lock-order") EvidenceSettingsLockOrder();
        else if (scenario == "evidence-persistence-kinds") EvidencePersistenceKinds();
        else if (scenario == "evidence-reset-empty") EvidenceResetEmpty();
        else if (scenario == "evidence-exception-boundary") EvidenceExceptionBoundary();
        else if (scenario == "evidence-nested-deferral") EvidenceNestedDeferral();
        else if (scenario == "evidence-native-trampoline") EvidenceNativeTrampoline();
        else if (scenario == "evidence-zoom-native-continuation") EvidenceZoomNativeContinuation();
        else if (scenario == "evidence-safetyhook-lifetime-faults") EvidenceSafetyHookLifetimeFaults();
        else if (scenario == "evidence-safetyhook-protection-recovery") EvidenceSafetyHookProtectionRecovery();
        else if (scenario == "evidence-safetyhook-veh-admission") EvidenceSafetyHookVehAdmission();
        else if (scenario == "evidence-safetyhook-allocation-ownership") EvidenceSafetyHookAllocationOwnership();
        else if (scenario == "evidence-safetyhook-delayed-execute") EvidenceSafetyHookDelayedExecute();
        else if (scenario == "evidence-safetyhook-retry-generations") EvidenceSafetyHookRetryGenerations();
        else if (scenario == "evidence-safetyhook-prewrite-route-lifetime") EvidenceSafetyHookPrewriteRouteLifetime();
        else if (scenario == "evidence-safetyhook-rx-publication") EvidenceSafetyHookRxPublication();
        else if (scenario == "evidence-safetyhook-failed-create") EvidenceSafetyHookFailedCreate();
        else if (scenario == "evidence-safetyhook-disable-postwrite") EvidenceSafetyHookDisablePostWriteFailure();
        else if (scenario == "evidence-reset-self-join") EvidenceResetFailedSelfJoin();
        else if (scenario == "evidence-reset-native-hook") EvidenceResetNativeHook();
        else if (scenario == "evidence-install-transaction-order") EvidenceInstallTransactionOrder();
        else throw std::runtime_error("Unknown native evidence case");
    }
