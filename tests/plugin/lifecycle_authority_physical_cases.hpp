// Included in the existing production transaction harness namespace.
namespace authority_physical
{
    namespace la = plugin::lifecycle_authority;
    struct Pair
    {
        la::Authority authority;
        plugin::CameraCorrectionGate gate;
        SettingsFixture settings;
        GameplayWriterFixture production;
        RuntimeCombineMemory expected, actual;
        bool dialogueContext{};
        bool pass{true};

        explicit Pair(config::GameplayMode mode)
        {
            settings.gameplayMode = mode;
            auto init = authority_parity::Facts(la::EventKind::Initialize);
            init.initialGlobalEnabled = init.initialGameplayEnabled = true;
            init.initialDialogueEnabled = true;
            init.initialDialoguePolicy = static_cast<unsigned>(config::DialogueZoomPolicy::Reduced);
            init.initialGameplayPolicy = static_cast<unsigned>(mode);
            authority.Accept(init);
            expected.aspect = actual.aspect = mode == config::GameplayMode::RuntimeCombine
                ? 16.0f / 9.0f : 32.0f / 9.0f;
        }

        plugin::CameraWriterGameplayStateView View()
        {
            return {settings.gameplayEnablePending, settings.gameplayDisablePending,
                settings.gameplayTransitionPending, production.enableTransitionDeferralLogged,
                settings.gameplayTransitionDeferralLogged, production.modeTransitionTarget,
                production.replayState, production.lastAutoRestoreSource, production.lastSource,
                production.lastFov, production.aspectRestoration, production.fovObservation, production.baseline};
        }

        la::WriterPortResult Writer(const char* name)
        {
            auto lock = gate.Acquire();
            const plugin::CameraWriterEngineOperations before{&expected, ReadRuntimeCombineMemory,
                WriteRuntimeCombineAspect, WriteRuntimeCombineAspectAndFlags};
            const plugin::CameraWriterEngineOperations after{&actual, ReadRuntimeCombineMemory,
                WriteRuntimeCombineAspect, WriteRuntimeCombineAspectAndFlags};
            plugin::CameraWriterInput input;
            input.sourceIdentity = expected.source;
            input.incomingFov = 130.311f;
            input.gameplayMode = settings.gameplayMode;
            input.gameplayEnabled = settings.gameplayEnabled;
            input.currentViewportAspect = 32.0f / 9.0f;
            input.currentViewportAspectValid = true;
            if (dialogueContext)
                input.writerContext = camera::WriterContext::DialogueLifecycleObserved;
            const auto legacy = input.gameplayMode == config::GameplayMode::AspectRecalculation
                ? plugin::ProcessGameplayAspectRecalculationWriter(View(), input,
                    camera::CoordinatorState::Gameplay, before, {})
                : plugin::ProcessGameplayHorPlusWriter(View(), input,
                    camera::CoordinatorState::Gameplay, before, {});
            la::WriterEvent event;
            event.input = authority_parity::Facts(la::EventKind::GameplaySample, actual.source);
            event.input.fov = input.incomingFov;
            event.input.outputObject = 0x701;
            event.input.thread = 9;
            event.input.aspect = actual.aspect;
            event.input.aspectValid = event.input.flagsValid = true;
            event.input.flags = actual.flags;
            event.input.viewportAspect = input.currentViewportAspect;
            event.input.viewportAspectValid = true;
            event.input.fovProvenance = la::FovProvenance::OriginalNative;
            event.input.evidence.sourceRelationValidated = true;
            event.input.requestPolicyAdmission = true;
            const auto revision = authority.Read().revision;
            float argument = input.incomingFov;
            const auto result = authority.WriterBoundary(gate).Process(event, argument, after);
            const auto state = authority.Read();
            const auto restoration = production.aspectRestoration.Read();
            pass &= Check(result.decision.accepted && state.revision == revision + 1,
                "physical_writer_has_one_final_commit");
            const bool legacyMutates = legacy.applied && !legacy.modeTransitionPending;
            const float legacyArgument = legacyMutates ? legacy.outputFov : input.incomingFov;
            const bool equal = result.physical && result.argumentPrepared == legacyMutates &&
                std::fabs(argument - legacyArgument) < 0.002f &&
                expected.aspect == actual.aspect && expected.flags == actual.flags &&
                expected.reads == actual.reads && expected.aspectWrites == actual.aspectWrites &&
                expected.aspectFlagsWrites == actual.aspectFlagsWrites &&
                state.gameplayPhysical.replay == production.replayState.load() &&
                state.gameplayPhysical.restoration.observationSequence == restoration.observationSequence &&
                state.gameplayPhysical.restoration.valid == restoration.valid &&
                state.gameplayPhysical.modePending == settings.gameplayTransitionPending.load() &&
                state.gameplayPhysical.enablePending == settings.gameplayEnablePending.load() &&
                state.gameplayPhysical.disablePending == settings.gameplayDisablePending.load();
            if (!equal) std::cerr << "PHYSICAL divergence step=" << name << " legacy=" << legacy.outputFov
                << " authority=" << argument << " has_effects=" << bool(result.physical) << '\n';
            pass &= Check(equal, name);
            return result;
        }

        void Enabled(bool value)
        {
            plugin::ApplyCameraSetting(gate, settings.View(),
                plugin::RuntimeSettingMutation::GameplayEnabled(value), true, [] {});
            auto event = authority_parity::Facts(la::EventKind::SettingsChanged);
            event.settingsDomain = la::SettingsDomain::Gameplay;
            event.enabled = value;
            authority.Accept(event);
        }

        bool BeginDialogue()
        {
            auto lock = gate.Acquire();
            auto event = authority_parity::Facts(la::EventKind::DialogueCandidate, actual.source);
            event.evidence.sourceRelationValidated = true;
            if (!authority.Accept(event).accepted) return false;
            event.kind = la::EventKind::DialogueActivated;
            event.evidence.dialogueDescentProven = true;
            dialogueContext = authority.Accept(event).accepted;
            return dialogueContext;
        }
    };

    bool Run()
    {
        bool pass = true;
        for (const bool diagnosticsOn : {false, true}) {
            diagnostics::SetEnabled(diagnosticsOn);
            for (const auto mode : {config::GameplayMode::HorPlus, config::GameplayMode::AspectRecalculation,
                    config::GameplayMode::RuntimeCombine}) {
                Pair sequence(mode);
                sequence.Writer("ordinary-physical-writer");
                sequence.Writer("repeated-physical-writer");
                sequence.Enabled(false);
                sequence.Writer("disabled-restoration");
                sequence.Enabled(true);
                sequence.Writer("enabled-physical-application");
                pass &= sequence.pass;
            }
            for (const unsigned failure : {0u, 1u, 2u}) {
                Pair sequence(config::GameplayMode::RuntimeCombine);
                sequence.expected.writeSucceeds = sequence.actual.writeSucceeds = failure != 0;
                sequence.expected.writeActuallyApplies = sequence.actual.writeActuallyApplies = failure != 1;
                sequence.expected.readable = sequence.actual.readable = failure != 2;
                const auto denied = sequence.Writer("runtimecombine-failed-or-unconfirmed-write");
                pass &= Check(!denied.argumentPrepared, "runtimecombine_requires_fresh_aspect_confirmation");
                sequence.expected.writeSucceeds = sequence.actual.writeSucceeds = true;
                sequence.expected.writeActuallyApplies = sequence.actual.writeActuallyApplies = true;
                sequence.expected.readable = sequence.actual.readable = true;
                pass &= Check(sequence.Writer("runtimecombine-retry").argumentPrepared,
                    "runtimecombine_retry_recovers_without_extra_lifecycle_event");
                pass &= sequence.pass;
            }
            Pair pending(config::GameplayMode::AspectRecalculation);
            pending.Writer("aspect-before-mode-change");
            auto mode = authority_parity::Facts(la::EventKind::SettingsChanged);
            mode.settingsDomain = la::SettingsDomain::Gameplay;
            mode.settingsMutation = la::SettingsMutation::PolicySelection;
            mode.policySelection = static_cast<unsigned>(config::GameplayMode::HorPlus);
            pending.authority.Accept(mode);
            pending.settings.gameplayMode = config::GameplayMode::HorPlus;
            pending.settings.gameplayTransitionPending = true;
            pending.production.replayState = gameplay::ReplayState::WaitingForAutomaticUpdate;
            pending.production.lastAutoRestoreSource = 0;
            pending.expected.writeSucceeds = pending.actual.writeSucceeds = false;
            pending.Writer("pending-horplus-write-failure");
            pass &= Check(pending.authority.Read().gameplayPhysical.modePending,
                "failed_mode_restore_retains_pending_intent");
            pending.expected.writeSucceeds = pending.actual.writeSucceeds = true;
            pending.Writer("pending-horplus-restoration-retry");
            pass &= pending.pass;
            for (const auto mode : {config::GameplayMode::HorPlus, config::GameplayMode::RuntimeCombine,
                    config::GameplayMode::AspectRecalculation}) {
                Pair dialogue(mode);
                dialogue.Writer("gameplay-before-ordinary-dialogue");
                pass &= Check(dialogue.BeginDialogue(), "ordinary_dialogue_fixture_has_active_scope");
                dialogue.Writer("ordinary-dialogue-respects-gameplay-mode");
                auto setting = authority_parity::Facts(la::EventKind::SettingsChanged);
                setting.settingsDomain = la::SettingsDomain::Dialogue;
                setting.enabled = false;
                dialogue.authority.Accept(setting);
                dialogue.Writer("dialogue-disabled-preserves-gameplay-mode");
                dialogue.Enabled(false);
                dialogue.Writer("gameplay-disabled-during-dialogue-uses-physical-cleanup");
                pass &= dialogue.pass;
            }
        }
        diagnostics::SetEnabled(false);
        std::cout << (pass ? "PASS" : "FAIL") << ": Authority physical Gameplay OFF/ON parity\n";
        return pass;
    }
}
