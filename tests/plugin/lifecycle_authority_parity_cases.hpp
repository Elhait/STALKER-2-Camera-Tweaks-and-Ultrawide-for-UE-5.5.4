// Test-only paired drivers. Native fixture packets feed both sides independently.
namespace authority_parity
{
    using namespace camera;
    using namespace cinematics;
    namespace la = plugin::lifecycle_authority;

    struct Production
    {
        SettingsFixture settings;
        std::atomic<CoordinatorState> coordinator{CoordinatorState::Gameplay};
        CinematicWriterAssociation association;
        CinematicRecoveryOwner owner;
        dialogue::PostCinematicRecoveryExclusion exclusion;
        std::atomic<bool> handoffPending{};
        std::atomic<std::uintptr_t> handoffSource{0xA};
        std::atomic<float> handoffFov{92.0f};
        std::atomic<bool> continuationPending{};
        std::atomic<bool> continuationCarryPending{};
        std::atomic<std::uintptr_t> continuationSource{0xA};
        std::atomic<float> continuationExitTarget{127.0f};
        std::atomic<float> continuationPreviousTarget{126.0f};
        std::atomic<bool> continuationInterpolation{true};
        std::atomic<float> continuationStart{90.0f};
        std::atomic<std::uintptr_t> gameplayBaselineSource{0xA};
        std::atomic<float> exitTarget{90.0f};
        std::atomic<float> exitCinematicTarget{127.0f};
        GameplayWriterFixture gameplayState;
        FreshFieldsFixture freshFields{0xA};
        const plugin::CameraWriterEngineOperations freshReader{
            &freshFields, &ReadFreshFields, nullptr, &WriteFreshAspectAndFlags};
        std::atomic<bool> cinematicEnabled{};
        std::atomic<std::uint64_t> cinematicSettingsEpoch{};
        cinematics::CinematicEnterActivation enterActivation;
        std::atomic<config::CinematicAspectPolicy> cinematicAspectPolicy{
            config::CinematicAspectPolicy::Auto};
        std::atomic<config::CinematicFovMode> cinematicFovMode{
            config::CinematicFovMode::GameplayHorPlus};
        std::atomic<bool> cinematicSelectionValid{};
        std::atomic<std::uint64_t> cinematicGeneration{};
        std::atomic<bool> cinematicTargetEstablished{};
        std::atomic<float> cinematicAuthoredFov{NAN};
        std::atomic<float> preEnterNativeFov{NAN};
        std::atomic<float> preEnterHorPlusFov{NAN};
        std::atomic<float> preEnterAspect{NAN};
        std::atomic<bool> preEnterPairValid{};
        std::atomic<float> cinematicTargetFov{NAN};
        std::atomic<std::uintptr_t> cinematicContinuationSource{};
        std::atomic<float> cinematicContinuationStart{NAN};
        std::atomic<bool> cinematicContinuationActive{};
        plugin::CameraCorrectionGate gate;
        plugin::CameraWriterRecoveryStateView state{coordinator, association, owner, exclusion,
            handoffPending, handoffSource, handoffFov, continuationPending,
            continuationCarryPending,
            continuationSource, continuationExitTarget, continuationPreviousTarget,
            continuationInterpolation, continuationStart, gameplayBaselineSource,
            exitTarget, exitCinematicTarget};
        plugin::CameraWriterCinematicStateView cinematicState{
            cinematicEnabled, cinematicSelectionValid, cinematicGeneration,
            cinematicSettingsEpoch, cinematicSettingsEpoch,
            cinematicTargetEstablished, cinematicAuthoredFov, cinematicTargetFov,
            cinematicContinuationSource, cinematicContinuationStart,
            cinematicContinuationActive, association, enterActivation};
        plugin::CinematicLifecycleStateView cinematicLifecycleState{
            coordinator, cinematicSelectionValid, cinematicGeneration,
            cinematicEnabled, cinematicSettingsEpoch, cinematicAspectPolicy,
            cinematicFovMode, cinematicTargetEstablished, cinematicTargetFov,
            exitTarget, exitCinematicTarget, cinematicAuthoredFov,
            preEnterNativeFov, preEnterHorPlusFov, preEnterAspect,
            preEnterPairValid, gameplayBaselineSource, continuationPending,
            continuationCarryPending,
            continuationSource, continuationExitTarget, continuationPreviousTarget,
            continuationInterpolation, cinematicContinuationStart, association,
            owner, exclusion, handoffPending, handoffSource, handoffFov};

        std::mutex dialogueMutex;
        dialogue::Phase dialoguePhase{dialogue::Phase::Inactive};
        bool dialoguePostExitProviderOwned{};
        std::uintptr_t dialoguePostExitProviderCallbackSource{};
        std::uintptr_t dialoguePostExitProviderReceiver{};
        std::uint64_t dialoguePostExitProviderGeneration{};
        std::uint64_t dialoguePostExitProviderSettingsEpoch{};
        std::uint32_t dialoguePostExitProviderThreadId{};
        dialogue::PolicySnapshot dialoguePolicy;
        dialogue::CorrectionActivation dialogueActivation;
        float dialogueBaseline{NAN}, dialoguePrevious{NAN}, dialogueExitStart{NAN};
        dialogue::CandidateTracker dialogueCandidate;
        dialogue::RecoveryRearm dialogueRecovery;
        std::atomic<bool> dialogueEnabled{true};
        std::atomic<config::DialogueZoomPolicy> selectedDialogue{config::DialogueZoomPolicy::Reduced};
        std::atomic<std::uint64_t> dialogueEpoch{1}, activeDialogueEpoch{};
        std::atomic<std::uintptr_t> dialogueSource{};
        std::atomic<float> dialogueTarget{NAN};
        std::atomic<bool> dialogueTargetValid{};
        plugin::DialogueBoundaryStateView dialogueView{dialogueMutex, coordinator, exclusion,
            dialoguePhase, dialoguePostExitProviderOwned,
            dialoguePostExitProviderCallbackSource, dialoguePostExitProviderReceiver,
            dialoguePostExitProviderGeneration, dialoguePostExitProviderSettingsEpoch,
            dialoguePostExitProviderThreadId,
            dialoguePolicy,
            dialogueActivation, dialogueBaseline, dialoguePrevious,
            dialogueExitStart, dialogueCandidate, dialogueRecovery, dialogueEnabled,
            selectedDialogue, dialogueEpoch, activeDialogueEpoch, dialogueSource,
            dialogueTarget, dialogueTargetValid};

        Production() {
            freshFields.aspect = 32.0f / 9.0f;
            settings.gameplayMode.store(config::GameplayMode::RuntimeCombine);
        }
        plugin::CameraWriterResult Writer(float fov, bool eligible = true,
            const dialogue::PostExitProviderWriterEvidence& provider = {},
            dialogue::PostExitProviderHandoff* tracker = nullptr)
        {
            auto lock = gate.Acquire();
            plugin::CameraWriterInput input{freshFields.expectedSource, 0x20, fov,
                freshFields.aspect, true, 4, true, 0.02f, settings.gameplayEnabled.load(),
                config::GameplayMode::RuntimeCombine, 16.0f / 9.0f};
            input.currentViewportAspect = freshFields.aspect;
            input.currentViewportAspectValid = true;
            input.dialogueCorrectionEnabled = true;
            input.dialoguePolicy = config::DialogueZoomPolicy::Reduced;
            input.dialogueSettingsEpoch = dialogueEpoch.load();
            input.threadId = 77;
            if (dialoguePhase == dialogue::Phase::Candidate)
                plugin::RetireStationaryDialogueCandidateOnGameplayWriter(dialogueView,
                    gameplayState.baseline.Read(), input.sourceIdentity, fov,
                    freshFields.aspect, freshFields.readable, cinematicGeneration.load(),
                    false, 0.02f);
            input.writerContext = plugin::ClassifyDialogueWriterContext(dialogueView,
                input.sourceIdentity, dialogueEpoch.load(), cinematicGeneration.load());
            if (tracker && tracker->RecoveryCompleted() && tracker->DialogueAdmissionConsumed() &&
                plugin::IsConfirmedProviderDialogueLifecycle(dialogueView, tracker->Activation(),
                    input.sourceIdentity, input.threadId, cinematicGeneration.load())) {
                input.writerContext = WriterContext::ConfirmedProviderDialogueLifecycleObserved;
                input.providerDialogueContinuation = true;
                input.providerDialogueBaseline = dialogueBaseline;
                input.providerDialogueNativeTarget = dialogueTarget.load();
                input.providerDialogueTarget = dialogue::ResolveDialogueDestinationTarget(
                    config::DialogueZoomPolicy::Reduced, dialogueBaseline, dialogueTarget.load());
                input.providerDialogueInputAlreadyCorrected = tracker->MatchesCorrectedDialogueSetterSample(
                    input.sourceIdentity, input.threadId, cinematicGeneration.load(), dialogueEpoch.load(), fov);
            }
            if (!eligible) input.writerContext = WriterContext::TransitionAmbiguous;
            if (coordinator.load() == CoordinatorState::CinematicActive)
                input.writerContext = WriterContext::TransitionAmbiguous;
            if (coordinator.load() == CoordinatorState::CinematicExiting)
                input.writerContext = owner.Read().HasRecoveryAuthority() &&
                    owner.Read().writerRsi == input.sourceIdentity
                    ? WriterContext::RecoveryOwned : WriterContext::TransitionAmbiguous;
            input.postExitProviderEvidence = provider;
            // The actual runtime adapter applies this classification after a
            // validated helper match, independently of Authority's lane.
            if (provider.valid) input.writerContext = WriterContext::ProviderTransitionObserved;
            auto view = gameplayState.View();
            return plugin::ProcessCameraWriter(gate, state, input, cinematicState,
                freshReader, {}, &view);
        }
    };

    la::EventFacts Facts(la::EventKind kind, std::uintptr_t source = 0xA, std::uint64_t generation = 0)
    {
        la::EventFacts event;
        event.kind = kind;
        event.scope = source;
        event.receiver = source;
        event.generation = generation;
        event.evidence.callbackBoundaryValidated = true;
        return event;
    }

    std::optional<la::WriterProposal> NumericProposal(void*, const la::WriterPolicyContext& context)
    {
        return la::EvaluateProductionPolicy({32.0f / 9.0f, 16.0f / 9.0f, 4}, context);
    }

    using WriterPort = la::WriterAdapterPort;
    template<class T> concept AcceptsLegacyWriter = requires(T& port, const la::EventFacts& event) {
        port.Accept(event);
    };
    static_assert(!AcceptsLegacyWriter<WriterPort>);
    template<class T> concept ExposesOwnerState = requires(T& port) { port.Read(); };
    template<class T> concept ExposesPreparation = requires(T& port, la::WriterInvocation& permission, double& value) {
        port.PrepareWriterArgument(permission, value);
    };
    static_assert(!ExposesOwnerState<WriterPort> && !ExposesPreparation<WriterPort>);
    static_assert(!std::is_copy_constructible_v<la::WriterInvocation>);

    struct Pair
    {
        Production production;
        la::Authority authority;
        bool diagnosticsOn{};
        unsigned comparisons{};
        unsigned divergences{};
        std::uint64_t generation{};
        dialogue::PostExitProviderHandoff handoff;
        la::EventId arming{};

        explicit Pair(bool diagnostics) : diagnosticsOn(diagnostics) {
            diagnostics::SetEnabled(diagnostics);
            auto init = Facts(la::EventKind::Initialize);
            init.initialGlobalEnabled = init.initialGameplayEnabled =
                init.initialCinematicEnabled = init.initialDialogueEnabled = true;
            init.initialGameplayPolicy = static_cast<unsigned>(config::GameplayMode::RuntimeCombine);
            init.initialCinematicPolicy = static_cast<unsigned>(config::CinematicFovMode::GameplayHorPlus);
            init.initialDialoguePolicy = static_cast<unsigned>(config::DialogueZoomPolicy::Reduced);
            authority.Accept(init);
        }

        void Compare(const char* name, bool same, const char* field) {
            ++comparisons;
            if (!same) ++divergences;
            std::cout << "PARITY diagnostics=" << diagnosticsOn << " step=" << name
                << " field=" << field << " result=" << (same ? "MATCH" : "DIVERGENCE") << '\n';
        }

        void Reference(const char* name) {
            const auto p = production.gameplayState.baseline.Read();
            const auto a = authority.Read().baseline;
            const bool usable = a && (a->trust == la::BaselineTrust::Current || a->trust == la::BaselineTrust::Retained);
            Compare(name, camera::IsUsableGameplayBaseline(p) == usable && (!p.valid ||
                (a && p.source.value == a->receiver && std::fabs(p.nativeFov.value - a->fov) < 0.002)),
                "baseline_identity_value_usable");
            std::cout << "TRUST production_valid=" << p.valid << " production_current=" << p.current
                << " authority=" << (a ? static_cast<int>(a->trust) : -1) << '\n';
        }

        void Writer(const char* name, float fov, bool policyAdmission = true,
            bool endpoint = false, bool provider = false,
            la::FovProvenance provenance = la::FovProvenance::OriginalNative,
            bool replacement = false) {
            const auto source = production.freshFields.expectedSource;
            const auto evidence = provider ? handoff.TakeWriterSample(true, source, 77,
                generation, 1, fov) : dialogue::PostExitProviderWriterEvidence{};
            const auto p = production.Writer(fov, policyAdmission, evidence, &handoff);
            if (p.postExitProviderRecoveryCompleted) handoff.CompleteRecovery();
            else if (p.horPlusRecovery.completed || p.recoveryRetired || p.noOwnerRecoveryCleanup)
                handoff.Reset();
            la::WriterEvent event;
            event.input = Facts(la::EventKind::GameplaySample, source, generation);
            event.input.previousScope = replacement ? 0xA : 0;
            event.input.fov = fov;
            event.input.outputObject = 0x20;
            event.input.thread = 77;
            event.input.aspect = production.freshFields.aspect;
            event.input.aspectValid = production.freshFields.readable;
            event.input.flags = production.freshFields.flags;
            event.input.flagsValid = production.freshFields.readable;
            event.input.fovProvenance = provenance;
            event.input.evidence.sourceRelationValidated = true;
            event.input.requestPolicyAdmission = policyAdmission;
            event.input.evidence.recoveryEndpointProven = endpoint;
            event.input.evidence.gameplayRoleProven = endpoint;
            if (provider) {
                event.input.component = 0xC05;
                event.input.thread = 77;
                event.input.providerGeneration = generation;
                event.input.armingEventId = arming;
                // Explicit supported fixture premise: validated ordered native
                // armer/setter/writer. Never taken from evidence.valid or p.
                event.input.evidence.causalArmingJoinProven = true;
            }
            event.observeOutput = diagnosticsOn;
            event.diagnosticFrameId = diagnosticsOn ? comparisons + 1 : 0;
            event.evaluate = NumericProposal;
            auto guard = production.gate.Acquire();
            auto port = authority.WriterBoundary(production.gate);
            float actual = fov;
            const auto a = port.Process(event, actual);
            const bool prepared = a.argumentPrepared;
            const auto pOwner = !production.settings.gameplayEnabled.load() &&
                production.coordinator.load() == CoordinatorState::Gameplay ? la::CorrectionOwner::PassThrough :
                p.postExitProviderHandoffApplied ? la::CorrectionOwner::Dialogue :
                production.coordinator.load() == CoordinatorState::CinematicActive ? la::CorrectionOwner::Cinematic :
                production.coordinator.load() == CoordinatorState::CinematicExiting ? la::CorrectionOwner::Recovery :
                production.dialoguePhase == dialogue::Phase::Active ? la::CorrectionOwner::Dialogue : la::CorrectionOwner::Gameplay;
            Compare(name, p.processed && a.decision.accepted &&
                std::fabs(p.outputFov - actual) < 0.03, "writer_output");
            Compare(name, pOwner == a.decision.correction, "correction_owner");
            std::cout << "FOV production=" << p.outputFov << " authority=" << actual
                << " permission=" << prepared << " raw=" << fov << '\n';
            // Read() returns a value. Hold that value while comparing its fields.
            const auto snapshot = authority.Read();
            const auto found = snapshot.scopes.find(source);
            Compare(name, found == snapshot.scopes.end() || (found->second.writerInputFov &&
                found->second.writerInputFov->provenance == provenance &&
                found->second.writerInputFov->transactionId == a.writerInvocationId), "captured_input_provenance");
            const bool providerHypothesis = (handoff.Active() ||
                (handoff.RecoveryCompleted() && !handoff.DialogueAdmissionConsumed())) &&
                production.dialoguePhase == dialogue::Phase::Inactive &&
                found != snapshot.scopes.end() && found->second.dialogue == la::DialoguePhase::Candidate;
            Compare(name, found == snapshot.scopes.end() || providerHypothesis ||
                static_cast<int>(production.dialoguePhase) == static_cast<int>(found->second.dialogue),
                "dialogue_lifecycle_after_writer");
            Reference(name);
            if (diagnosticsOn && a.observationId) {
                const la::WriterReadback observed{a.observationId, a.writerInvocationId, source,
                    source, event.input.outputObject, static_cast<std::uint32_t>(event.input.thread),
                    generation, a.decision.revision, la::FovBoundary::PostStore, p.outputFov};
                const auto writerBoundary = authority.WriterBoundary(production.gate);
                auto foreign = observed;
                ++foreign.output;
                Compare(name, !writerBoundary.Observe(foreign), "foreign_output_readback_rejected");
                foreign = observed;
                ++foreign.thread;
                Compare(name, !writerBoundary.Observe(foreign), "foreign_thread_readback_rejected");
                Compare(name, writerBoundary.Observe(observed), "modeled_named_readback");
                Compare(name, writerBoundary.Close(observed), "observation_closure");
            }
        }

        void Enter(const char* name) {
            auto guard = production.gate.Acquire();
            const auto precedingWriter = production.gameplayState.fovObservation.ReadLatest(
                camera::FovObservationBoundary::CameraWriter);
            const auto dialogueStart = precedingWriter
                ? cinematics::MakeDialogueWriterResultCandidate(*precedingWriter,
                    production.freshFields.expectedSource, production.dialogueEpoch.load(), generation,
                    production.dialoguePhase == dialogue::Phase::Active)
                : cinematics::EnterActivationCorrectedStartCandidate{};
            const auto selection = plugin::PrepareCinematicEnter(production.cinematicLifecycleState,
                config::CinematicAspectPolicy::Auto, config::CinematicFovMode::GameplayHorPlus);
            ++generation;
            plugin::AcceptedCinematicEnterInput input;
            input.selection = selection;
            input.correctionEnabled = true;
            input.authoredFov = 90.0f;
            input.gameplayBaseline = production.gameplayState.baseline.Read();
            input.effectiveAspect = 32.0f / 9.0f;
            input.nativeAspect = 16.0f / 9.0f;
            const auto p = plugin::ProcessAcceptedCinematicEnter(production.cinematicLifecycleState, input);
            production.enterActivation.Arm(generation, input.settingsEpoch,
                input.gameplayBaseline.source.value, input.gameplayBaseline.nativeFov.value,
                input.gameplayBaseline.horPlusFov.value,
                p.transformed && camera::IsUsableGameplayBaseline(input.gameplayBaseline) &&
                    input.gameplayBaseline.horPlusFov.valid,
                p.transformed ? p.outputFov : NAN, dialogueStart, false,
                input.effectiveAspect, production.settings.gameplayEpoch.load());
            if (p.accepted && plugin::DialogueBoundaryIsActive(production.dialogueView))
                plugin::ResetDialogueBoundaryState(production.dialogueView);
            auto event = Facts(la::EventKind::CinematicEnter, production.freshFields.expectedSource, generation);
            event.aspectContextCompatible = true;
            event.fov = 90;
            event.fovBoundary = la::FovBoundary::CinematicEnter;
            event.fovProvenance = la::FovProvenance::OriginalNative;
            const auto a = authority.Accept(event);
            plugin::CinematicAspectStoreInput aspect;
            aspect.targetObject = production.freshFields.expectedSource;
            plugin::ObserveCinematicAspectStore(production.cinematicLifecycleState, aspect);
            auto sourceStore = Facts(la::EventKind::CinematicAspectStore,
                production.freshFields.expectedSource, generation);
            authority.Accept(sourceStore);
            Compare(name, p.accepted == a.accepted && generation == selection.generation,
                "enter_activation");
            const auto numeric = NumericProposal(nullptr, {la::CorrectionOwner::Cinematic,
                static_cast<unsigned>(config::CinematicFovMode::GameplayHorPlus),
                authority.Read().baseline, 90.0, la::FovProvenance::OriginalNative});
            Compare(name, numeric && std::fabs(numeric->outputFov - p.outputFov) < 0.03,
                "enter_numeric_target");
            Reference(name);
        }

        void CaptureCinematic(float start, float target) {
            auto guard = production.gate.Acquire();
            const auto source = production.freshFields.expectedSource;
            const bool p = production.enterActivation.Capture(true, generation,
                production.cinematicSettingsEpoch.load(), 0xCAE, source, 77, start, target,
                production.dialogueEpoch.load());
            auto event = Facts(la::EventKind::CinematicActivationCaptured, source, generation);
            event.evidence.callerValidated = true;
            event.component = 0xCAE;
            event.thread = 77;
            event.nativeStart = start;
            event.nativeTarget = target;
            event.nativeTargetReadable = true;
            event.aspect = production.freshFields.aspect;
            event.aspectValid = event.flagsValid = true;
            event.flags = 4;
            const auto a = authority.Accept(event);
            Compare("activation/capture", p && a.accepted, "native_activation_capture");
        }

        void Exit(const char* name, float nativeTarget = 130.311f) {
            auto guard = production.gate.Acquire();
            plugin::AcceptedCinematicExitInput input;
            input.previousCoordinator = production.coordinator.load();
            input.nextCoordinator = CoordinatorState::CinematicExiting;
            input.generation = generation;
            input.nativeFov = nativeTarget;
            input.gameplayEnabled = true;
            input.gameplayMode = config::GameplayMode::RuntimeCombine;
            const auto p = plugin::ProcessAcceptedCinematicExit(production.cinematicLifecycleState, input);
            auto event = Facts(la::EventKind::CinematicExit, production.freshFields.expectedSource, generation);
            event.evidence.sourceRelationValidated = true;
            event.nativeTarget = nativeTarget;
            event.nativeTargetReadable = true;
            const auto a = authority.Accept(event);
            Compare(name, p.accepted == a.accepted && p.recoveryOwnerCaptured,
                "exit_and_source_recovery_capture");
            Reference(name);
        }

        void Setter(const char* name, float fov, float target, bool provider = false) {
            auto guard = production.gate.Acquire();
            plugin::DialogueBoundaryInput input;
            input.incomingFov = fov;
            input.callbackSource = 0xC05 + 0x28;
            input.nativeTarget = target;
            input.nativeTargetReadable = true;
            input.observedReceiver = production.freshFields.expectedSource;
            input.validatedReceiver = input.observedReceiver;
            input.generation = generation;
            input.threadId = 77;
            if (provider) handoff.ObserveSetterSample(true, 0xC05, input.observedReceiver,
                77, generation, 1, 130.311f, 70.0f, fov, fov);
            if (provider && handoff.RecoveryCompleted()) {
                input.handledProviderActivation = handoff.ConsumeDialogueAdmission(0xC05,
                    input.observedReceiver, 77, generation, 1, 130.311f, target);
                const auto reference = production.gameplayState.baseline.Read();
                input.providerGameplayBaseline = reference.nativeFov.value;
                input.providerGameplayBaselineValid = camera::IsUsableGameplayBaseline(reference) &&
                    reference.source.value == input.observedReceiver;
                input.providerDialoguePolicy = config::DialogueZoomPolicy::Reduced;
                input.providerDialoguePolicyValid = input.handledProviderActivation;
            }
            const auto p = plugin::ProcessDialogueBoundary(production.dialogueView, input);
            if (provider && input.handledProviderActivation)
                handoff.RecordCorrectedDialogueSetterSample(p.outputFov);
            auto event = Facts(provider ? la::EventKind::ProviderSetterObserved : la::EventKind::DialogueCandidate,
                input.observedReceiver, generation);
            event.component = 0xC05;
            event.thread = 77;
            event.providerGeneration = generation;
            event.armingEventId = arming;
            event.fov = fov;
            event.fovBoundary = la::FovBoundary::DialogueSetterInput;
            event.fovProvenance = la::FovProvenance::OriginalNative;
            event.evidence.causalArmingJoinProven = provider;
            event.evidence.sourceRelationValidated = true;
            event.nativeStart = 130.311f;
            event.nativeTarget = target;
            event.nativeTargetReadable = true;
            auto boundary = authority.DialogueBoundary(production.gate);
            // The native setter samples XMM6 but changes XMM1. Ordinary
            // pass-through must preserve XMM1 even when those values differ.
            const float capturedArgument = provider ? fov : fov + 0.125f;
            const la::SetterEvent setter{event, input.callbackSource, capturedArgument};
            const auto revision = authority.Read().revision;
            float otherArgument = capturedArgument + 1.0f;
            const auto refused = boundary.Process(setter, otherArgument);
            Compare(name, !refused.decision.accepted && authority.Read().revision == revision &&
                otherArgument == capturedArgument + 1.0f, "mismatched_setter_argument_preserves_state");
            float argument = capturedArgument;
            const auto a = boundary.Process(setter, argument);
            const auto scope = authority.Read().scopes.at(input.observedReceiver);
            const auto aPhase = scope.dialogue;
            const auto pPhase = production.dialoguePhase;
            // Provider arming hypothesis is stored in Handoff in production
            // and in Candidate in Authority; compare the combined claim.
            const bool providerHypothesis = provider && handoff.Active() &&
                pPhase == dialogue::Phase::Inactive && aPhase == la::DialoguePhase::Candidate;
            Compare(name, providerHypothesis || static_cast<int>(pPhase) == static_cast<int>(aPhase),
                "setter_phase_and_provider_hypothesis");
            std::cout << "SETTER production_phase=" << static_cast<int>(pPhase)
                << " authority_phase=" << static_cast<int>(aPhase)
                << " exclusion=" << production.exclusion.IsActive()
                << " production_action=" << static_cast<int>(p.action) << '\n';
            const float expectedArgument = p.mutateXmm1 ? p.outputFov : capturedArgument;
            Compare(name, std::fabs(expectedArgument - argument) < 0.002 && a.decision.accepted &&
                p.mutateXmm1 == a.prepareArgument,
                "setter_argument_and_acceptance");
            Reference(name);
        }

        void DisableGameplay(const char* name) {
            const auto p = plugin::ApplyCameraSetting(production.gate, production.settings.View(),
                plugin::RuntimeSettingMutation::GameplayEnabled(false), true, [] {});
            auto guard = production.gate.Acquire();
            const auto a = authority.ApplySetting(production.gate,
                plugin::RuntimeSettingMutation::GameplayEnabled(false), true, true);
            Compare(name, p.mutation.accepted && a.handled && a.mutation.accepted && a.mutation.changed &&
                !production.settings.gameplayEnabled.load() && !authority.Read().settings.gameplayEnabled,
                "effective_settings_revocation");
            Reference(name);
        }

        void Arm(const char* name) {
            const auto source = production.freshFields.expectedSource;
            const dialogue::PostExitProviderActivation native{generation, 1, 0xC05,
                source, 77, 130.311f, 70.0f, 130.311f, config::DialogueZoomPolicy::Reduced};
            const bool p = handoff.ObserveArmer(true, native);
            auto event = Facts(la::EventKind::ProviderArmed, source, generation);
            event.component = native.component;
            event.thread = native.threadId;
            event.providerGeneration = generation;
            event.evidence.callerValidated = true;
            event.nativeStart = native.nativeStart;
            event.nativeTarget = native.nativeTarget;
            event.nativeTargetReadable = true;
            const auto a = authority.Accept(event);
            arming = a.authorityEventId;
            Compare(name, p == a.accepted, "provider_arm");
        }
    };

    bool GateAndAccessChecks()
    {
        la::Authority owner;
        auto init = Facts(la::EventKind::Initialize);
        init.initialGlobalEnabled = init.initialGameplayEnabled = true;
        init.initialGameplayPolicy = static_cast<unsigned>(config::GameplayMode::RuntimeCombine);
        owner.Accept(init);
        plugin::CameraCorrectionGate gate;
        std::promise<void> attempted;
        std::future<void> worker;
        std::atomic<bool> settingsCommitted{};
        bool prepared{};
        bool nestedRejected{};
        auto port = owner.WriterBoundary(gate);
        auto event = la::WriterEvent{};
        event.input = Facts(la::EventKind::GameplaySample);
        event.input.fov = 130.311;
        event.input.fovProvenance = la::FovProvenance::OriginalNative;
        event.input.requestPolicyAdmission = true;
        event.input.evidence.sourceRelationValidated = true;
        event.input.outputObject = 0x20;
        event.input.aspect = 32.0f / 9.0f;
        event.input.aspectValid = event.input.flagsValid = true;
        event.input.flags = 4;
        // Adapter-supplied proposals cannot bypass the canonical policy.
        event.evaluate = +[](void*, const la::WriterPolicyContext&) -> std::optional<la::WriterProposal> {
            return la::WriterProposal{12.0, false};
        };
        float value = static_cast<float>(event.input.fov);
        const auto initialRevision = owner.Read().revision;
        const bool outsideGateRejected = !port.Process(event, value).decision.accepted &&
            owner.Read().revision == initialRevision;
        bool mismatchedArgumentRejected{};
        bool unreadableFieldsRejected{};
        {
            auto lock = gate.Acquire();
            worker = std::async(std::launch::async, [&] {
                attempted.set_value();
                auto settingsLock = gate.Acquire();
                auto disabled = Facts(la::EventKind::SettingsChanged);
                disabled.enabled = false;
                owner.Accept(disabled);
                settingsCommitted = true;
            });
            attempted.get_future().wait();
            float mismatched = 90.0f;
            mismatchedArgumentRejected = !port.Process(event, mismatched).decision.accepted &&
                owner.Read().revision == initialRevision;
            auto unreadable = event;
            unreadable.input.flagsValid = false;
            unreadableFieldsRejected = !port.Process(unreadable, value).decision.accepted &&
                owner.Read().revision == initialRevision;
            const auto invocation = port.Process(event, value);
            prepared = invocation.argumentPrepared && !settingsCommitted &&
                std::fabs(value - 153.931f) < 0.003f;
            nestedRejected = !plugin::CanEnterCameraCallback(gate);
            // Receipt is destroyed before lock leaves this PRE-shaped scope.
        }
        worker.get();
        std::cout << "GATE prepared_before_settings=" << prepared << " nested_rejected=" << nestedRejected
            << " settings_after_release=" << settingsCommitted << " legacy_port_access=compile_time_rejected\n";
        return prepared && nestedRejected && settingsCommitted && outsideGateRejected &&
            mismatchedArgumentRejected && unreadableFieldsRejected;
    }

    bool Run()
    {
        unsigned divergences{};
        bool gate = GateAndAccessChecks();
        for (bool diagnostics : {false, true}) {
            Pair sameSource(diagnostics);
            sameSource.Writer("same-source/gameplay", 130.311f);
            sameSource.Enter("same-source/cinematic-enter");
            sameSource.Writer("same-source/cinematic-writer", 130.311f);
            sameSource.Exit("same-source/cinematic-exit");
            sameSource.Writer("same-source/recovery-completion", 130.311f, true, true);
            sameSource.Setter("same-source/neutral-setter", 130.311f, 130.311f);
            sameSource.Enter("same-source/next-cinematic");
            sameSource.Writer("same-source/next-writer", 130.311f);
            divergences += sameSource.divergences;

            Pair provider(diagnostics);
            provider.Writer("provider/gameplay", 130.311f);
            provider.Enter("provider/enter");
            provider.Writer("provider/cinematic-writer", 130.311f);
            provider.Exit("provider/exit");
            provider.Writer("provider/recovery-endpoint", 130.311f, true, true);
            provider.Arm("provider/arm");
            provider.Setter("provider/setter", 130.264f, 70.0f, true);
            provider.Writer("provider/first-writer", 130.264f, false, false, true);
            provider.Writer("provider/endpoint-writer", 70.0f, false, false, true);
            provider.Setter("provider/endpoint-setter", 70.0f, 70.0f, true);
            provider.Writer("provider/prepared-dialogue-writer", 122.087f, true, false, false,
                la::FovProvenance::PreparedDialogue);
            provider.Enter("provider/next-cinematic-enter");
            provider.Writer("provider/next-cinematic-writer", 153.931f, false, false, false,
                la::FovProvenance::PreparedCinematic);
            divergences += provider.divergences;

            Pair replacement(diagnostics);
            replacement.Writer("replacement/A", 90.3115f);
            replacement.production.freshFields.expectedSource = 0xB;
            replacement.Writer("replacement/B-ambiguous", 130.311f, false, false, false,
                la::FovProvenance::OriginalNative, true);
            replacement.Writer("replacement/B-admitted", 130.311f);
            replacement.Enter("replacement/enter");
            replacement.Writer("replacement/cinematic", 130.311f);
            divergences += replacement.divergences;

            Pair settings(diagnostics);
            settings.Writer("settings/gameplay", 130.311f);
            settings.DisableGameplay("settings/disable");
            settings.Writer("settings/disabled-writer", 130.311f);
            divergences += settings.divergences;

            Pair terminal(diagnostics);
            terminal.Writer("candidate/gameplay", 130.311f);
            terminal.Setter("candidate/stationary-setter", 130.311f, 130.311f);
            terminal.Writer("candidate/terminal-writer", 130.311f);
            divergences += terminal.divergences;

            Pair between(diagnostics);
            between.Writer("disable-before-enter/gameplay", 130.311f);
            between.DisableGameplay("disable-before-enter/disable");
            between.Enter("disable-before-enter/enter");
            divergences += between.divergences;
        }
        diagnostics::SetEnabled(false);
        std::cout << "DIFFERENTIAL divergences=" << divergences << " gate=" << gate << '\n';
        return gate && divergences == 0;
    }
}
