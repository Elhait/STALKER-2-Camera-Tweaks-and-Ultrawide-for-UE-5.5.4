// Included in the existing isolated harness namespace.
void InitializeCorrective(Authority& authority)
{
    auto event = Basic(EventKind::Initialize);
    event.initialGlobalEnabled = event.initialGameplayEnabled =
        event.initialCinematicEnabled = event.initialDialogueEnabled = true;
    event.initialGameplayPolicy = static_cast<unsigned>(config::GameplayMode::RuntimeCombine);
    event.initialCinematicPolicy = static_cast<unsigned>(config::CinematicFovMode::GameplayHorPlus);
    event.initialDialoguePolicy = static_cast<unsigned>(config::DialogueZoomPolicy::Reduced);
    authority.Accept(event);
}

WriterEvent CorrectiveWriter(float fov = 130.311f)
{
    auto event = CompositeInput(fov);
    event.input.outputObject = 202;
    event.input.aspect = 32.0f / 9.0f;
    event.input.aspectValid = event.input.flagsValid = true;
    event.input.flags = 4;
    event.input.thread = 7;
    event.evaluate = +[](void*, const WriterPolicyContext& context) {
        return EvaluateProductionPolicy({32.0f / 9.0f, 16.0f / 9.0f, 4}, context);
    };
    return event;
}

bool ExerciseCorrectiveBindings()
{
    bool pass = true;
    double argument{};
    Authority replacement;
    InitializeCorrective(replacement);
    auto seed = CorrectiveWriter(90.3115f);
    auto initial = replacement.Accept(seed);
    replacement.PrepareWriterArgument(initial, argument);
    auto ambiguous = CorrectiveWriter();
    ambiguous.input.scope = 2;
    ambiguous.input.receiver = 303;
    ambiguous.input.previousScope = 1;
    ambiguous.input.requestPolicyAdmission = false;
    auto allowed = replacement.Accept(ambiguous);
    pass &= Check(allowed.decision.correctionOnlyAdmitted &&
        allowed.decision.admission == GameplayAdmission::Unknown &&
        replacement.Read().baseline->receiver == 101 &&
        replacement.Read().baseline->trust == BaselineTrust::Invalid &&
        replacement.PrepareWriterArgument(allowed, argument) && std::fabs(argument - 153.931) < 0.002,
        "ambiguous_b_gets_correction_without_admitting_or_transferring_a_reference");
    ambiguous.input.fovProvenance = FovProvenance::PreparedDialogue;
    auto preparedUnknown = replacement.Accept(ambiguous);
    pass &= Check(!replacement.PrepareWriterArgument(preparedUnknown, argument),
        "unknown_prepared_b_cannot_gain_original_input_correction_permission");
    ambiguous.input.fovProvenance = FovProvenance::OriginalNative;
    ambiguous.input.flagsValid = false;
    auto unreadable = replacement.Accept(ambiguous);
    pass &= Check(!replacement.PrepareWriterArgument(unreadable, argument),
        "correction_only_policy_requires_readable_native_flags");

    Authority cinematic;
    InitializeCorrective(cinematic);
    auto game = cinematic.Accept(CorrectiveWriter());
    cinematic.PrepareWriterArgument(game, argument);
    auto enter = Basic(EventKind::CinematicEnter);
    enter.generation = 1;
    enter.fov = 90;
    enter.fovBoundary = FovBoundary::CinematicEnter;
    enter.fovProvenance = FovProvenance::OriginalNative;
    enter.aspectContextCompatible = true;
    cinematic.Accept(enter);
    auto store = Basic(EventKind::CinematicAspectStore);
    store.generation = 1;
    cinematic.Accept(store);
    auto sample = CorrectiveWriter();
    sample.input.generation = 1;
    auto retained = cinematic.Accept(sample);
    pass &= Check(cinematic.PrepareWriterArgument(retained, argument) &&
        std::fabs(argument - 153.931) < 0.002,
        "cinematic_native_gameplay_input_retains_target_without_second_projection_mapping");
    sample.input.fov = 153.931;
    sample.input.fovProvenance = FovProvenance::PreparedCinematic;
    auto prepared = cinematic.Accept(sample);
    argument = sample.input.fov;
    pass &= Check(!cinematic.PrepareWriterArgument(prepared, argument) &&
        prepared.decision.correction == CorrectionOwner::Cinematic &&
        cinematic.Read().scopes.at(1).plannedOutputFov->provenance == FovProvenance::PreparedCinematic &&
        std::fabs(argument - 153.931) < 0.002,
        "cinematic_prepared_target_passes_through_without_a_second_mutation_permission");
    store.receiver = 999;
    cinematic.Accept(store);
    sample.input.fov = 130.311f;
    sample.input.fovProvenance = FovProvenance::OriginalNative;
    auto conflict = cinematic.Accept(sample);
    argument = sample.input.fov;
    cinematic.PrepareWriterArgument(conflict, argument);
    pass &= Check(std::fabs(argument - sample.input.fov) < 0.002,
        "conflicting_aspect_source_cannot_authorize_initial_target_retention");

    Authority stationary;
    InitializeCorrective(stationary);
    auto native = CorrectiveWriter();
    auto admitted = stationary.Accept(native);
    stationary.PrepareWriterArgument(admitted, argument);
    SetterEvent setter;
    setter.input = native.input;
    setter.input.nativeTarget = setter.input.fov;
    setter.input.nativeTargetReadable = true;
    setter.callbackSource = 500;
    const auto captured = stationary.Accept(setter);
    pass &= Check(captured.decision.accepted && stationary.Read().scopes.at(1).dialogue == DialoguePhase::Candidate,
        "raw_setter_captures_bound_stationary_candidate");
    const auto retired = stationary.Accept(native);
    pass &= Check(retired.decision.accepted && !native.input.evidence.dialogueTerminalProven &&
        stationary.Read().scopes.at(1).dialogue == DialoguePhase::None,
        "writer_retires_stationary_candidate_from_production_predicate_without_adapter_terminal_verdict");

    stationary.Accept(setter);
    auto candidateMode = Basic(EventKind::SettingsChanged);
    candidateMode.settingsDomain = SettingsDomain::Gameplay;
    candidateMode.settingsMutation = SettingsMutation::PolicySelection;
    candidateMode.policySelection = static_cast<unsigned>(config::GameplayMode::AspectRecalculation);
    stationary.Accept(candidateMode);
    candidateMode.policySelection = static_cast<unsigned>(config::GameplayMode::RuntimeCombine);
    stationary.Accept(candidateMode);
    auto changed = CorrectiveWriter(90.0f);
    changed.input.evidence.gameplayRoleProven = false;
    for (unsigned i = 0; i < 4; ++i) {
        auto waiting = stationary.Accept(changed);
        pass &= Check(!stationary.PrepareWriterArgument(waiting, argument) &&
            stationary.Read().scopes.at(1).dialogue == DialoguePhase::Candidate &&
            stationary.Read().baseline && stationary.Read().baseline->fov != 90.0,
            "changed_scalar_alone_does_not_terminate_candidate");
    }
    changed.input.evidence.gameplayRoleProven = true;
    auto confirmedChange = stationary.Accept(changed);
    pass &= Check(stationary.PrepareWriterArgument(confirmedChange, argument) &&
        stationary.Read().scopes.at(1).dialogue == DialoguePhase::None &&
        stationary.Read().baseline && stationary.Read().baseline->fov == 90.0,
        "proven_same_source_gameplay_retires_candidate_and_refreshes_changed_baseline");

    Authority provider;
    InitializeCorrective(provider);
    auto baseline = provider.Accept(CorrectiveWriter());
    provider.PrepareWriterArgument(baseline, argument);
    provider.Accept(enter);
    store.receiver = 101;
    provider.Accept(store);
    sample.input.fovProvenance = FovProvenance::OriginalNative;
    auto cineWriter = provider.Accept(sample);
    provider.PrepareWriterArgument(cineWriter, argument);
    auto exit = Basic(EventKind::CinematicExit);
    exit.generation = 1;
    exit.evidence.sourceRelationValidated = true;
    exit.nativeTarget = 130.311f;
    exit.nativeTargetReadable = true;
    provider.Accept(exit);
    auto endpoint = CorrectiveWriter();
    endpoint.input.generation = 1;
    endpoint.input.evidence.recoveryEndpointProven = true;
    endpoint.input.evidence.gameplayRoleProven = true;
    auto completed = provider.Accept(endpoint);
    provider.PrepareWriterArgument(completed, argument);
    setter.input.generation = 1;
    const auto excluded = provider.Accept(setter);
    pass &= Check(excluded.decision.accepted && provider.Read().scopes.at(1).dialogue == DialoguePhase::None,
        "ordinary_setter_during_post_cinematic_exclusion_does_not_create_dialogue_candidate");
    auto arm = Basic(EventKind::ProviderArmed);
    arm.generation = arm.providerGeneration = 1;
    arm.component = 600;
    arm.thread = 7;
    arm.nativeStart = 130.311f;
    arm.nativeTarget = 70;
    arm.nativeTargetReadable = true;
    arm.evidence.callerValidated = true;
    const auto arming = provider.Accept(arm);
    setter.input = arm;
    setter.input.fov = 130.264f;
    setter.input.fovProvenance = FovProvenance::OriginalNative;
    setter.input.fovBoundary = FovBoundary::DialogueSetterInput;
    setter.input.armingEventId = arming.authorityEventId;
    setter.input.evidence.causalArmingJoinProven = true;
    setter.input.evidence.sourceRelationValidated = true;
    setter.callbackSource = 640;
    provider.Accept(setter);
    auto trajectory = CorrectiveWriter(70);
    trajectory.input.generation = trajectory.input.providerGeneration = 1;
    trajectory.input.component = 600;
    trajectory.input.armingEventId = arming.authorityEventId;
    trajectory.input.evidence.causalArmingJoinProven = true;
    trajectory.input.requestPolicyAdmission = false;
    auto target = provider.Accept(trajectory);
    pass &= Check(provider.PrepareWriterArgument(target, argument) && std::fabs(argument - 149.071) < 0.003,
        "already_completed_recovery_preserves_provider_target_mapping");
    setter.input.fov = 70;
    const auto activated = provider.Accept(setter);
    pass &= Check(activated.prepareArgument && std::fabs(activated.outputFov - 122.087) < 0.003 &&
        provider.Read().scopes.at(1).dialogue == DialoguePhase::Active &&
        provider.Read().baseline->trust == BaselineTrust::Retained,
        "provider_endpoint_setter_activates_reduced_and_retains_gameplay_reference");
    auto repeated = CorrectiveWriter(static_cast<float>(activated.outputFov));
    repeated.input.generation = 1;
    repeated.input.fovProvenance = FovProvenance::PreparedDialogue;
    auto dialogue = provider.Accept(repeated);
    pass &= Check(dialogue.decision.correction == CorrectionOwner::Dialogue &&
        provider.PrepareWriterArgument(dialogue, argument) && std::fabs(argument - 149.071) < 0.003,
        "prepared_dialogue_uses_dialogue_owner_and_exact_owned_setter_witness");
    Authority aspectMode;
    InitializeCorrective(aspectMode);
    auto mode = Basic(EventKind::SettingsChanged);
    mode.settingsDomain = SettingsDomain::Gameplay;
    mode.settingsMutation = SettingsMutation::PolicySelection;
    mode.policySelection = static_cast<unsigned>(config::GameplayMode::AspectRecalculation);
    aspectMode.Accept(mode);
    auto aspectWriter = aspectMode.Accept(CorrectiveWriter());
    pass &= Check(!aspectMode.PrepareWriterArgument(aspectWriter, argument),
        "aspect_recalculation_mode_never_receives_direct_horplus_writer_effect");
    std::cout << (pass ? "PASS" : "FAIL") << ": corrective bindings assertions=" << assertions << '\n';
    return pass;
}
