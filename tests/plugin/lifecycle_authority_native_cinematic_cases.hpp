// Receiver is unavailable at ENTER. It is discovered from the actual native
// armer/aspect-store boundaries, rather than borrowed from a Gameplay address.
namespace authority_native_cinematic
{
    bool Run()
    {
        namespace la = plugin::lifecycle_authority;
        static_assert(!authority_parity::AcceptsLegacyWriter<la::CinematicAdapterPort>);
        static_assert(!authority_parity::ExposesOwnerState<la::CinematicAdapterPort>);
        static_assert(!std::is_constructible_v<la::CinematicAdapterPort, la::Authority&, plugin::CameraCorrectionGate&>);
        bool pass = true;
        for (const bool diagnosticsOn : {false, true}) {
            diagnostics::SetEnabled(diagnosticsOn);
            la::Authority authority;
            plugin::CameraCorrectionGate gate;
            auto lock = gate.Acquire();
            auto init = authority_parity::Facts(la::EventKind::Initialize);
            init.initialGlobalEnabled = init.initialGameplayEnabled = init.initialCinematicEnabled = true;
            init.initialGameplayPolicy = static_cast<unsigned>(config::GameplayMode::RuntimeCombine);
            init.initialCinematicPolicy = static_cast<unsigned>(config::CinematicFovMode::GameplayHorPlus);
            authority.Accept(init);
            auto writer = la::WriterEvent{};
            writer.input = authority_parity::Facts(la::EventKind::GameplaySample);
            writer.input.fov = 130.311f;
            writer.input.aspect = 32.0f / 9.0f;
            writer.input.aspectValid = writer.input.flagsValid = true;
            writer.input.flags = 4;
            writer.input.outputObject = 0x701;
            writer.input.thread = 7;
            writer.input.requestPolicyAdmission = true;
            writer.input.evidence.sourceRelationValidated = true;
            writer.input.fovProvenance = la::FovProvenance::OriginalNative;
            float argument = 130.311f;
            authority.WriterBoundary(gate).Process(writer, argument);
            const float target = argument;
            auto cinematic = authority.CinematicBoundary(gate);
            float authored = 90.0f;
            const auto enter = cinematic.Enter({authored, 32.0f / 9.0f, true}, authored);
            pass &= Check(enter.decision.accepted && enter.prepareArgument && std::fabs(authored - 153.931f) < 0.003f &&
                authority.Read().pendingCinematic && authority.Read().pendingCinematic->receiver == 0,
                "native_enter_uses_trusted_numeric_reference_without_inventing_receiver");
            constexpr std::uintptr_t receiver = 0xB;
            writer.input.scope = writer.input.receiver = receiver;
            writer.input.generation = enter.generation;
            argument = 130.311f;
            const auto unknown = authority.WriterBoundary(gate).Process(writer, argument);
            pass &= Check(!unknown.argumentPrepared && authority.Read().baseline->receiver == 0xA,
                "unbound_cinematic_writer_cannot_refresh_gameplay_reference");
            la::NativeCinematicArmer armer{0xCA, receiver, 7, 130.311f, target, 130.311f,
                32.0f / 9.0f, 32.0f / 9.0f, 4, true, true};
            pass &= Check(cinematic.Armer(armer).decision.accepted, "native_armer_binds_real_receiver");
            RuntimeCombineMemory memory;
            memory.source = receiver;
            const plugin::CameraWriterEngineOperations engine{&memory, ReadRuntimeCombineMemory,
                WriteRuntimeCombineAspect, WriteRuntimeCombineAspectAndFlags};
            const auto aspect = cinematic.Aspect({receiver, 32.0f / 9.0f, true}, engine);
            pass &= Check(aspect.effectSucceeded && memory.aspect == 32.0f / 9.0f,
                "native_aspect_store_is_executed_on_bound_target");
            la::SetterEvent interactive;
            interactive.input = writer.input;
            interactive.input.fov = 70;
            interactive.input.nativeTarget = 70;
            interactive.input.nativeTargetReadable = true;
            interactive.callbackSource = 0x11D;
            const auto choice = authority.Accept(interactive);
            pass &= Check(choice.decision.accepted && !choice.prepareArgument &&
                authority.Read().scopes.at(receiver).dialogue == la::DialoguePhase::None,
                "interactive_choice_during_native_cinematic_does_not_create_dialogue_ownership");
            writer.input.fov = (130.311f + target) * 0.5f;
            writer.input.fovProvenance = la::FovProvenance::PreparedCinematic;
            argument = static_cast<float>(writer.input.fov);
            const auto mapped = authority.WriterBoundary(gate).Process(writer, argument);
            pass &= Check(mapped.argumentPrepared && mapped.decision.correction == la::CorrectionOwner::Cinematic &&
                std::fabs(argument - target) < 0.003f && authority.Read().baseline->receiver == 0xA &&
                authority.Read().baseline->trust == la::BaselineTrust::Retained,
                "native_bound_cinematic_maps_progress_without_transferring_reference_to_receiver_b");
            const auto revision = authority.Read().revision;
            armer.receiver = 0xC;
            armer.validated = false;
            pass &= Check(!cinematic.Armer(armer).decision.accepted && authority.Read().revision == revision,
                "foreign_native_caller_does_not_destroy_current_activation");
            auto disable = authority_parity::Facts(la::EventKind::SettingsChanged);
            disable.settingsDomain = la::SettingsDomain::Cinematic;
            disable.enabled = false;
            authority.Accept(disable);
            disable.enabled = true;
            authority.Accept(disable);
            argument = static_cast<float>(writer.input.fov);
            const auto revoked = authority.WriterBoundary(gate).Process(writer, argument);
            pass &= Check(!revoked.argumentPrepared && argument == static_cast<float>(writer.input.fov) &&
                authority.Read().scopes.at(receiver).cinematicActive,
                "cinematic_enable_toggle_does_not_rearm_an_already_revoked_activation");
            pass &= Check(cinematic.Exit({130.311f, true, true}).decision.accepted &&
                !cinematic.Exit({130.311f, true, true}).decision.accepted,
                "native_exit_retires_selection_exactly_once");

            authored = 90.0f;
            pass &= Check(cinematic.Enter({authored, 32.0f / 9.0f, true}, authored).decision.accepted,
                "next_native_enter_is_not_blocked_by_previous_activation");
            cinematic.Exit({130.311f, true, true});
            pass &= Check(!authority.Read().pendingCinematic && !authority.Read().cinematicSelectionActive &&
                authority.Read().baseline->receiver == 0xA,
                "writer_empty_native_generation_has_terminal_path_without_false_baseline_admission");

            auto aspectPolicy = authority_parity::Facts(la::EventKind::SettingsChanged);
            aspectPolicy.settingsDomain = la::SettingsDomain::Cinematic;
            aspectPolicy.settingsMutation = la::SettingsMutation::AspectPolicySelection;
            aspectPolicy.policySelection = static_cast<unsigned>(config::CinematicAspectPolicy::Native);
            const auto epoch = authority.Read().settings.cinematicEpoch;
            authority.Accept(aspectPolicy);
            authored = 90.0f;
            const auto nativeEnter = cinematic.Enter({authored, 32.0f / 9.0f, true}, authored);
            cinematic.Aspect({receiver, 32.0f / 9.0f, true}, engine);
            writer.input.generation = nativeEnter.generation;
            writer.input.fov = 90.0f;
            argument = 90.0f;
            const auto nativeWriter = authority.WriterBoundary(gate).Process(writer, argument);
            pass &= Check(authority.Read().settings.cinematicEpoch == epoch &&
                !nativeEnter.prepareArgument && !nativeWriter.argumentPrepared && argument == 90.0f &&
                memory.aspect == 16.0f / 9.0f && authority.Read().scopes.at(receiver).writerArgumentFov,
                "native_cinematic_aspect_has_no_hidden_horplus_and_observes_unchanged_writer_argument");

            // The camera used by the native blend and the aspect-store policy
            // source are not interchangeable identities.
            la::Authority distinct;
            auto separateInit = init;
            separateInit.initialDialogueEnabled = true;
            separateInit.initialDialoguePolicy = static_cast<unsigned>(config::DialogueZoomPolicy::Reduced);
            distinct.Accept(separateInit);
            auto preceding = authority_parity::Facts(la::EventKind::CinematicEnter, 0xC, 1);
            preceding.fov = 90;
            preceding.aspectContextCompatible = true;
            distinct.Accept(preceding);
            preceding.kind = la::EventKind::CinematicExit;
            preceding.nativeTarget = 130.311f;
            preceding.nativeTargetReadable = true;
            preceding.evidence.sourceRelationValidated = true;
            distinct.Accept(preceding);
            preceding.kind = la::EventKind::ProviderArmed;
            preceding.component = 0xDA;
            preceding.thread = 7;
            preceding.providerGeneration = 1;
            preceding.nativeStart = 130.311f;
            preceding.nativeTarget = 70;
            preceding.evidence.callerValidated = true;
            const auto oldClaim = distinct.Accept(preceding);
            pass &= Check(oldClaim.accepted && oldClaim.authorityEventId != 0,
                "transfer_control_has_a_live_provider_claim_on_the_future_policy_source");
            writer.input = authority_parity::Facts(la::EventKind::GameplaySample);
            writer.input.fov = 130.311f;
            writer.input.aspect = 32.0f / 9.0f;
            writer.input.aspectValid = writer.input.flagsValid = true;
            writer.input.flags = 4;
            writer.input.outputObject = 0x701;
            writer.input.thread = 7;
            writer.input.requestPolicyAdmission = true;
            writer.input.evidence.sourceRelationValidated = true;
            writer.input.fovProvenance = la::FovProvenance::OriginalNative;
            argument = 130.311f;
            distinct.WriterBoundary(gate).Process(writer, argument);
            auto boundary = distinct.CinematicBoundary(gate);
            authored = 90.0f;
            const auto separateEnter = boundary.Enter({authored, 32.0f / 9.0f, true, 7}, authored);
            armer.receiver = 0xB;
            armer.validated = true;
            pass &= Check(boundary.Armer(armer).decision.accepted, "armer_before_aspect_can_bind_provisional_source");
            memory.source = 0xC;
            boundary.Aspect({0xC, 32.0f / 9.0f, true}, engine);
            pass &= Check(!distinct.Read().armings.contains(oldClaim.authorityEventId),
                "cinematic_policy_source_transfer_retires_the_replaced_provider_claim");
            pass &= Check(boundary.Armer(armer).decision.accepted &&
                distinct.Read().selectedCinematicScope == 0xC &&
                distinct.Read().scopes.at(0xC).cinematicActivation.Read().receiver == 0xB,
                "validated_blend_rearming_does_not_replace_the_independent_aspect_policy_source");
            auto otherThread = armer;
            otherThread.thread = 8;
            pass &= Check(!boundary.Armer(otherThread).decision.accepted,
                "native_enter_and_armer_must_share_the_observed_call_thread");
            writer.input.scope = writer.input.receiver = 0xC;
            writer.input.generation = separateEnter.generation;
            argument = 130.311f;
            const auto retained = distinct.WriterBoundary(gate).Process(writer, argument);
            pass &= Check(retained.argumentPrepared && std::fabs(argument - target) < 0.003f &&
                distinct.Read().scopes.at(0xC).cinematicReference->receiver == 0xA,
                "distinct_native_sources_preserve_target_retention_and_original_reference_identity");
            memory.source = 0xD;
            boundary.Aspect({0xD, 32.0f / 9.0f, true}, engine);
            argument = 130.311f;
            pass &= Check(!distinct.WriterBoundary(gate).Process(writer, argument).argumentPrepared &&
                distinct.Read().scopes.at(0xC).cinematicAssociationConflict,
                "a_second_conflicting_aspect_source_cannot_authorize_target_retention");
            la::Authority malformed;
            malformed.Accept(init);
            auto malformedBoundary = malformed.CinematicBoundary(gate);
            authored = 90.0f;
            const auto validEnter = malformedBoundary.Enter({authored, 32.0f / 9.0f, true}, authored);
            memory.source = receiver;
            malformedBoundary.Aspect({receiver, 32.0f / 9.0f, true}, engine);
            writer.input.scope = writer.input.receiver = receiver;
            writer.input.generation = validEnter.generation;
            writer.input.fov = 90;
            argument = 90;
            malformed.WriterBoundary(gate).Process(writer, argument);
            const auto invalidExit = malformedBoundary.Exit({NAN, true, true});
            pass &= Check(invalidExit.decision.accepted && !malformed.Read().cinematicSelectionActive &&
                malformed.Read().scopes.at(receiver).recovery == la::RecoveryPhase::None,
                "invalid_exit_target_does_not_create_an_unfinishable_recovery_owner");
            authored = NAN;
            const auto invalidEnter = malformedBoundary.Enter({authored, 32.0f / 9.0f, true}, authored);
            const auto independentAspect = malformedBoundary.Aspect({receiver, 32.0f / 9.0f, true}, engine);
            pass &= Check(invalidEnter.decision.accepted && !invalidEnter.prepareArgument && std::isnan(authored) &&
                !malformed.Read().scopes.at(receiver).originalFov && independentAspect.effectSucceeded &&
                malformed.Read().cinematicSelectionActive,
                "invalid_authored_scalar_does_not_erase_native_lifecycle_or_block_independent_aspect_effects");

            la::Authority recovery;
            recovery.Accept(init);
            auto recoveryBoundary = recovery.CinematicBoundary(gate);
            authored = 90.0f;
            const auto recoveryEnter = recoveryBoundary.Enter({authored, 32.0f / 9.0f, true}, authored);
            recoveryBoundary.Aspect({receiver, 32.0f / 9.0f, true}, engine);
            writer.input.generation = recoveryEnter.generation;
            writer.input.fov = 90;
            argument = 90;
            recovery.WriterBoundary(gate).Process(writer, argument);
            recoveryBoundary.Exit({100, true, true});
            writer.input.viewportAspect = 32.0f / 9.0f;
            writer.input.viewportAspectValid = true;
            writer.input.fov = 100;
            writer.input.evidence.recoveryEndpointProven = false;
            writer.input.fovProvenance = la::FovProvenance::OriginalNative;
            memory.aspect = 16.0f / 9.0f;
            memory.writeSucceeds = false;
            argument = 100;
            const auto held = recovery.WriterBoundary(gate).Process(writer, argument, engine);
            pass &= Check(held.argumentPrepared && std::fabs(argument - authored) < 0.003f &&
                recovery.Read().scopes.at(receiver).recovery == la::RecoveryPhase::Pending,
                "unconfirmed_recovery_aspect_holds_cinematic_argument_without_completing");
            writer.input.fov = 110;
            argument = 110;
            recovery.WriterBoundary(gate).Process(writer, argument, engine);
            memory.writeSucceeds = true;
            writer.input.fov = 100;
            argument = 100;
            const auto resumed = recovery.WriterBoundary(gate).Process(writer, argument, engine);
            pass &= Check(resumed.argumentPrepared && resumed.physical && resumed.physical->runtimeCombineConfirmed &&
                recovery.Read().scopes.at(receiver).recovery == la::RecoveryPhase::Complete &&
                recovery.Read().baseline && recovery.Read().baseline->receiver == receiver,
                "runtimecombine_recovery_completes_from_fresh_fields_and_resumes_gameplay_in_same_writer");

            for (const bool previousWriterWasOwner : {false, true}) {
                la::Authority replacement;
                replacement.Accept(init);
                auto boundary = replacement.CinematicBoundary(gate);
                auto sample = writer;
                constexpr std::uintptr_t nextReceiver = 0xF;
                sample.input.scope = sample.input.receiver = receiver;
                sample.input.generation = 0;
                sample.input.fov = 100;
                sample.input.requestPolicyAdmission = true;
                argument = 100;
                replacement.WriterBoundary(gate).Process(sample, argument, engine);
                authored = 90;
                const auto enter = boundary.Enter({authored, 32.0f / 9.0f, true}, authored);
                boundary.Aspect({receiver, 32.0f / 9.0f, true}, engine);
                sample.input.generation = enter.generation;
                sample.input.fov = 90;
                argument = 90;
                replacement.WriterBoundary(gate).Process(sample, argument, engine);
                if (!previousWriterWasOwner) {
                    sample.input.scope = sample.input.receiver = 0xD;
                    argument = 90;
                    replacement.WriterBoundary(gate).Process(sample, argument, engine);
                }
                boundary.Exit({100, true, true});
                memory.source = nextReceiver;
                sample.input.scope = sample.input.receiver = nextReceiver;
                sample.input.fov = 130.311f;
                sample.input.requestPolicyAdmission = false;
                argument = static_cast<float>(sample.input.fov);
                const auto outcome = replacement.WriterBoundary(gate).Process(sample, argument, engine);
                const auto state = replacement.Read();
                pass &= Check(outcome.decision.accepted && state.baseline &&
                    state.baseline->receiver == receiver, "recovery_receiver_change_does_not_admit_ambiguous_baseline");
                if (previousWriterWasOwner) {
                    pass &= Check(outcome.argumentPrepared && std::fabs(argument - 153.931f) < 0.003f &&
                        !state.scopes.contains(receiver) && state.baseline->trust == la::BaselineTrust::Invalid,
                        "owner_to_new_receiver_retires_recovery_before_same_writer_gameplay");
                } else {
                    pass &= Check(!outcome.argumentPrepared && !outcome.physical &&
                        state.scopes.at(receiver).recovery == la::RecoveryPhase::Pending &&
                        state.baseline->trust == la::BaselineTrust::Retained,
                        "foreign_writer_without_owner_departure_preserves_pending_recovery");
                }
                memory.source = receiver;
            }

            {
                la::Authority provider;
                config::FeatureConfig providerConfig;
                providerConfig.gameplayEnabled = providerConfig.cinematicsEnabled = providerConfig.dialogueEnabled = true;
                providerConfig.gameplayMode = config::GameplayMode::RuntimeCombine;
                providerConfig.cinematicFovMode = config::CinematicFovMode::GameplayHorPlus;
                providerConfig.dialogueZoomPolicy = config::DialogueZoomPolicy::Reduced;
                pass &= Check(provider.Initialize(gate, providerConfig), "camera_settings_initialize_from_feature_config");
                auto sample = writer;
                sample.input.scope = sample.input.receiver = receiver;
                sample.input.generation = 0;
                sample.input.fov = 130.311f;
                plugin::CameraWriterInput capture;
                capture.sourceIdentity = receiver;
                capture.outputObjectIdentity = sample.input.outputObject;
                capture.threadId = 7;
                capture.aspect = capture.currentViewportAspect = 32.0f / 9.0f;
                capture.aspectValid = capture.currentViewportAspectValid = capture.flagsValid = true;
                capture.flags = 4;
                // These preclassified fields must have no influence on the
                // fact-only path, including on the initial Gameplay call.
                capture.writerContext = camera::WriterContext::RecoveryOwned;
                capture.gameplayEnabled = false;
                const auto writerPort = provider.WriterBoundary(gate);
                const auto write = [&](float fov) {
                    capture.incomingFov = argument = fov;
                    return writerPort.ProcessNative(capture, argument, engine, diagnosticsOn);
                };
                argument = 130.311f;
                pass &= Check(write(argument).argumentPrepared && provider.Read().baseline,
                    "native_writer_admits_gameplay_from_committed_state_without_adapter_verdict");
                auto cinematicPort = provider.CinematicBoundary(gate);
                authored = 90;
                const auto enter = cinematicPort.Enter({authored, 32.0f / 9.0f, true}, authored);
                const auto atEnter = provider.ReadCameraEvidence(gate);
                pass &= Check(atEnter && !atEnter->writerTransaction.Valid() &&
                    atEnter->gameplayFov.freshness == camera::SnapshotFreshness::Retained,
                    "enter_retires_last_writer_publication_and_preserves_gameplay_reference");
                plugin::OverlaySemanticSnapshot overlay;
                pass &= Check(provider.ReadOverlayLifecycle(gate, overlay) &&
                    overlay.coordinator.value == plugin::OverlayCoordinatorState::CinematicActive &&
                    overlay.activeCinematicFovMode.value == config::CinematicFovMode::GameplayHorPlus &&
                    !overlay.cameraState.evidence.writerTransaction.Valid(),
                    "overlay_reads_the_same_committed_cinematic_selection_and_fov_evidence");
                cinematicPort.Aspect({receiver, 32.0f / 9.0f, true}, engine);
                sample.input.generation = enter.generation;
                sample.input.fov = 90;
                argument = 90;
                write(argument);
                cinematicPort.Exit({130.311f, true, true});
                auto setter = provider.DialogueBoundary(gate);
                const auto armed = setter.Arm({0xDA, receiver, 7, 130.311f, 70, 130.311f, true});
                pass &= Check(armed.accepted, "native_provider_arming_uses_current_recovery_owner");
                la::NativeDialogueSetter setterCapture{
                    0x119, 0xDA, receiver, 7, 130.264f, 130.311f, 70, true, true};
                argument = 130.264f;
                setter.ProcessNative(setterCapture, argument);
                sample.input.fov = 130.264f;
                argument = 130.264f;
                const auto paired = write(argument);
                pass &= Check(paired.argumentPrepared && paired.decision.correction == la::CorrectionOwner::Dialogue,
                    "writer_joins_native_setter_sample_without_replay_causal_flags");
                sample.input.fov = 70;
                argument = 70;
                const auto endpoint = write(argument);
                pass &= Check(endpoint.argumentPrepared && std::fabs(argument - 149.071f) < 0.003f &&
                    provider.Read().scopes.at(receiver).recovery == la::RecoveryPhase::Complete,
                    "native_provider_endpoint_maps_and_completes_recovery");
                const auto published = provider.ReadCameraEvidence(gate);
                pass &= Check(published && published->writerTransaction.id == endpoint.writerInvocationId &&
                    published->currentOutputFov.provenance == camera::EvidenceProvenance::ModDerivedState &&
                    !published->postStoreFov.valid && !published->postHelperFov.valid,
                    "publication_reports_prepared_argument_without_inventing_readback");
                if (diagnosticsOn && published) {
                    const auto& identity = published->writerTransaction;
                    const la::WriterReadback readback{endpoint.observationId, endpoint.writerInvocationId,
                        receiver, identity.receiver, identity.output, identity.thread, identity.lifecycle,
                        endpoint.decision.revision, la::FovBoundary::PostStore, argument};
                    const auto aspectWrites = memory.aspectWrites + memory.aspectFlagsWrites;
                    pass &= Check(writerPort.Observe(readback) && !writerPort.Observe(readback) &&
                        provider.ReadCameraEvidence(gate)->postStoreFov.valid &&
                        writerPort.Close(readback) && !writerPort.Observe(readback) &&
                        memory.aspectWrites + memory.aspectFlagsWrites == aspectWrites,
                        "correlated_readback_is_observation_only_and_cannot_reopen_or_repeat_effects");
                }
                setterCapture.incomingFov = 70;
                argument = 70;
                const auto admitted = setter.ProcessNative(setterCapture, argument);
                pass &= Check(admitted.prepareArgument && std::fabs(argument - 122.087f) < 0.003f &&
                    provider.Read().scopes.at(receiver).providerDialogueOwned,
                    "native_provider_endpoint_setter_consumes_admission_once");
                sample.input.fov = argument;
                sample.input.requestPolicyAdmission = false;
                sample.input.fovProvenance = la::FovProvenance::PreparedDialogue;
                const auto continuation = write(argument);
                pass &= Check(continuation.argumentPrepared && std::fabs(argument - 149.071f) < 0.004f,
                    "native_provider_processed_setter_argument_is_not_mapped_twice");
                // A foreign writer can be the most recent caller while the
                // preceding Cinematic scope still owns the provider handoff.
                capture.sourceIdentity = 0xE;
                memory.source = capture.sourceIdentity;
                write(130.311f);
                authored = 90;
                const auto next = cinematicPort.Enter({authored, 32.0f / 9.0f, true}, authored);
                const auto transferred = provider.Read();
                pass &= Check(next.decision.accepted && transferred.scopes.contains(receiver) &&
                    transferred.scopes.at(receiver).dialogue == la::DialoguePhase::None &&
                    !transferred.scopes.at(receiver).nativeProvider &&
                    !transferred.scopes.at(receiver).activeArmingId &&
                    !transferred.scopes.at(receiver).preparedDialogueFov,
                    "native_enter_revokes_provider_on_previous_cinematic_even_after_foreign_writer");
                memory.source = receiver;
                pass &= Check(provider.Shutdown(gate) && provider.Read().scopes.empty() &&
                    provider.Read().armings.empty() && !provider.Read().baseline &&
                    !provider.Initialize(gate, providerConfig),
                    "camera_shutdown_retires_provider_claims_and_disallows_reentry");
            }

            for (const bool validatedExit : {true, false}) {
                la::Authority continuation;
                continuation.Accept(init);
                auto port = continuation.CinematicBoundary(gate);
                auto writerPort = continuation.WriterBoundary(gate);
                plugin::CameraWriterInput capture;
                capture.sourceIdentity = receiver;
                capture.outputObjectIdentity = 0x701;
                capture.threadId = 7;
                capture.aspect = capture.currentViewportAspect = 3.0f;
                capture.aspectValid = capture.currentViewportAspectValid = capture.flagsValid = true;
                capture.flags = 4;
                memory.aspect = capture.aspect;
                const auto write = [&](float fov) {
                    capture.incomingFov = argument = fov;
                    return writerPort.ProcessNative(capture, argument, engine, diagnosticsOn);
                };
                constexpr float native = 95.1311f;
                // Beyond the authored-input tolerance, so only validated
                // continuation can authorize retaining the corrected target.
                constexpr float midpoint = 97.0f;
                write(native);
                const float previousTarget = argument;
                authored = native;
                port.Enter({authored, capture.aspect, true}, authored);
                port.Aspect({receiver, capture.aspect, true}, engine);
                if (validatedExit) write(native);
                port.Exit({native, true, true});
                authored = native;
                port.Enter({authored, capture.aspect, true}, authored);
                port.Aspect({receiver, capture.aspect, true}, engine);
                write(native);
                const auto retained = write(midpoint);
                pass &= Check(retained.argumentPrepared == validatedExit &&
                    std::fabs(argument - (validatedExit ? previousTarget : midpoint)) < 0.003f,
                    "native_cinematic_reentry_retains_target_without_armer_only_after_validated_exit");
            }

            for (const auto mode : {config::GameplayMode::HorPlus, config::GameplayMode::RuntimeCombine}) {
                for (const bool endpointArrivesFirst : {false, true}) {
                    la::Authority transition;
                    config::FeatureConfig config;
                    config.gameplayEnabled = config.cinematicsEnabled = true;
                    config.gameplayMode = mode;
                    config.cinematicFovMode = config::CinematicFovMode::GameplayHorPlus;
                    transition.Initialize(gate, config);
                    auto camera = transition.CinematicBoundary(gate);
                    auto writerPort = transition.WriterBoundary(gate);
                    plugin::CameraWriterInput capture;
                    capture.sourceIdentity = receiver;
                    capture.outputObjectIdentity = 0x701;
                    capture.threadId = 7;
                    capture.aspect = capture.currentViewportAspect = 32.0f / 9.0f;
                    capture.aspectValid = capture.currentViewportAspectValid = capture.flagsValid = true;
                    capture.flags = 4;
                    memory.source = receiver;
                    memory.aspect = capture.aspect;
                    const auto write = [&](float fov) {
                        capture.incomingFov = argument = fov;
                        return writerPort.ProcessNative(capture, argument, engine, false);
                    };
                    write(130.311f);
                    authored = 90;
                    camera.Enter({authored, capture.aspect, true}, authored);
                    camera.Aspect({receiver, capture.aspect, true}, engine);
                    write(90);
                    camera.Exit({130.311f, true, true});
                    if (endpointArrivesFirst) write(130.311f);
                    write(140.0f);
                    const auto returned = write(130.311f);
                    const auto final = transition.Read();
                    pass &= Check(returned.argumentPrepared && std::fabs(argument - 153.931f) < 0.005f &&
                        final.baseline && final.baseline->trust == la::BaselineTrust::Current &&
                        final.baseline->observationSequence == returned.writerInvocationId,
                        "source_bound_exclusion_release_reclassifies_same_writer_for_either_recovery_order");
                }
            }

            for (const bool foreignWriterInterleaves : {false, true}) {
                la::Authority foreground;
                auto dialogueInit = init;
                dialogueInit.initialDialogueEnabled = true;
                dialogueInit.initialDialoguePolicy = static_cast<unsigned>(config::DialogueZoomPolicy::Reduced);
                foreground.Accept(dialogueInit);
                auto dialoguePort = foreground.DialogueBoundary(gate);
                la::NativeDialogueSetter setter{0xC01, 0xC00, receiver, 7, 130.311f, 130.311f, 70, true, true};
                for (const float sampleFov : {130.311f, 120.0f, 110.0f, 100.0f, 90.0f}) {
                    setter.incomingFov = argument = sampleFov;
                    dialoguePort.ProcessNative(setter, argument);
                }
                pass &= Check(foreground.Read().scopes.at(receiver).dialogue == la::DialoguePhase::Active,
                    "independent_dialogue_precondition_for_foreground_transfer");
                plugin::CameraWriterInput capture;
                capture.outputObjectIdentity = 0x701;
                capture.threadId = 7;
                capture.aspect = capture.currentViewportAspect = 32.0f / 9.0f;
                capture.aspectValid = capture.currentViewportAspectValid = capture.flagsValid = true;
                capture.flags = 4;
                const auto write = [&](std::uintptr_t source, float fov) {
                    memory.source = capture.sourceIdentity = source;
                    memory.aspect = capture.aspect;
                    capture.incomingFov = argument = fov;
                    return foreground.WriterBoundary(gate).ProcessNative(capture, argument, engine, false);
                };
                write(receiver, 115);
                if (foreignWriterInterleaves) write(0xD, 130.311f);
                authored = 90;
                auto camera = foreground.CinematicBoundary(gate);
                camera.Enter({authored, capture.aspect, true}, authored);
                const auto beforeBind = write(receiver, 115);
                memory.source = 0xE;
                camera.Aspect({memory.source, capture.aspect, true}, engine);
                const auto afterBind = write(receiver, 115);
                pass &= Check(beforeBind.decision.correction == la::CorrectionOwner::PassThrough &&
                    afterBind.decision.correction == la::CorrectionOwner::PassThrough &&
                    foreground.Read().scopes.at(receiver).dialogue == la::DialoguePhase::None,
                    "accepted_cinematic_retires_foreground_dialogue_even_after_interleaved_foreign_writer");
                camera.Exit({130.311f, true, true});
                for (const float sampleFov : {130.311f, 120.0f, 110.0f, 100.0f, 90.0f}) {
                    setter.incomingFov = argument = sampleFov;
                    dialoguePort.ProcessNative(setter, argument);
                }
                pass &= Check(foreground.Read().scopes.at(receiver).dialogue == la::DialoguePhase::Active,
                    "new_dialogue_on_same_receiver_can_start_after_foreground_transfer");
                memory.source = receiver;
            }

            {
                la::Authority repeated;
                repeated.Accept(init);
                auto boundary = repeated.CinematicBoundary(gate);
                bool boundEveryGeneration = true;
                for (std::uintptr_t index = 0; index < la::Authority::MaxScopes + 4; ++index) {
                    memory.source = 0x900 + index;
                    authored = 90;
                    auto selection = boundary.Enter({authored, 32.0f / 9.0f, true}, authored);
                    auto store = boundary.Aspect({memory.source, 32.0f / 9.0f, true}, engine);
                    auto exit = boundary.Exit({130.311f, true, true});
                    boundEveryGeneration &= selection.decision.accepted && store.effectSucceeded &&
                        exit.decision.accepted;
                }
                pass &= Check(boundEveryGeneration && repeated.Read().scopes.size() <= la::Authority::MaxScopes,
                    "finished_cinematic_scopes_do_not_exhaust_bindings_in_one_session");
                memory.source = receiver;
            }

            for (const bool writeSucceeds : {false, true}) {
                la::Authority aspectRecovery;
                auto aspectInit = init;
                aspectInit.initialGameplayPolicy = static_cast<unsigned>(config::GameplayMode::AspectRecalculation);
                aspectRecovery.Accept(aspectInit);
                auto boundary = aspectRecovery.CinematicBoundary(gate);
                authored = 90;
                const auto enter = boundary.Enter({authored, 32.0f / 9.0f, true}, authored);
                boundary.Aspect({receiver, 32.0f / 9.0f, true}, engine);
                writer.input.generation = enter.generation;
                writer.input.fov = 90;
                argument = 90;
                aspectRecovery.WriterBoundary(gate).Process(writer, argument);
                boundary.Exit({100, true, true});
                memory.writeSucceeds = writeSucceeds;
                const auto writes = memory.aspectFlagsWrites;
                const auto sample = [&](float fov) {
                    writer.input.fov = fov;
                    argument = fov;
                    return aspectRecovery.WriterBoundary(gate).Process(writer, argument, engine);
                };
                const auto first = sample(120);
                const auto descending = sample(115);
                const auto next = sample(110);
                const auto outcome = aspectRecovery.Read();
                pass &= Check(first.decision.accepted && descending.decision.accepted && next.decision.accepted &&
                    !descending.argumentPrepared && memory.aspectFlagsWrites == writes + 1 &&
                    !outcome.scopes.at(receiver).atomicExitPending &&
                    (outcome.gameplayPhysical.replay == gameplay::ReplayState::Complete) == writeSucceeds,
                    "aspect_recovery_downward_handoff_consumes_one_write_attempt_and_records_success_only");
                const auto endpoint = sample(100);
                pass &= Check(endpoint.decision.accepted && !endpoint.argumentPrepared &&
                    aspectRecovery.Read().scopes.at(receiver).recovery == la::RecoveryPhase::Complete &&
                    !aspectRecovery.Read().baseline,
                    "aspect_recovery_endpoint_retires_without_a_gameplay_baseline_or_fov_write");
                memory.writeSucceeds = true;
            }
        }
        diagnostics::SetEnabled(false);
        std::cout << (pass ? "PASS" : "FAIL") << ": Authority native Cinematic boundaries OFF/ON\n";
        return pass;
    }
}
