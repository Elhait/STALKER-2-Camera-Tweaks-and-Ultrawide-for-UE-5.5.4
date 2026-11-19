#include "../../src/plugin/lifecycle_authority_core.hpp"
#include "../../src/plugin/lifecycle_authority_policy.hpp"
#include "../../src/plugin/dialogue_boundary_transaction.hpp"
#include "../../src/cinematics/cinematic_fov.hpp"
#include "../../src/camera/horplus.hpp"

#include <cmath>
#include <iostream>
#include <string_view>

namespace
{
    using namespace plugin::lifecycle_authority;
    int assertions{};

    bool Check(bool condition, const char* label)
    {
        ++assertions;
        if (!condition) std::cerr << "FAIL: " << label << '\n';
        return condition;
    }

    EventFacts Basic(EventKind kind, ScopeId scope = 1)
    {
        EventFacts facts{};
        facts.kind = kind;
        facts.scope = scope;
        facts.receiver = 101;
        facts.evidence.callbackBoundaryValidated = true;
        return facts;
    }

    EventFacts OriginalGameplay(double value, ScopeId scope = 1, std::uint64_t receiver = 101)
    {
        auto facts = Basic(EventKind::GameplaySample, scope);
        facts.receiver = receiver;
        facts.fov = value;
        facts.fovProvenance = FovProvenance::OriginalNative;
        facts.fovBoundary = FovBoundary::GameplayWriterInput;
        facts.evidence.gameplayRoleProven = true;
        return facts;
    }

    bool Initialize(Authority& authority)
    {
        auto facts = Basic(EventKind::Initialize);
        facts.initialGlobalEnabled = true;
        facts.initialGameplayEnabled = true;
        facts.initialCinematicEnabled = true;
        facts.initialDialogueEnabled = true;
        facts.initialGameplayPolicy = 1;
        facts.initialCinematicPolicy = 2;
        facts.initialDialoguePolicy = 3;
        return authority.Accept(facts).accepted;
    }

    bool ExerciseOwnershipAndR1Projection()
    {
        bool pass = true;
        Authority authority;
        pass &= Check(Initialize(authority), "r1_t01_initialize");
        pass &= Check(!Initialize(authority), "initialize_is_single_commit");

        auto gameplay = OriginalGameplay(130.311);
        pass &= Check(authority.Accept(gameplay).baseline == BaselineAction::Update,
            "r1_t28_proven_gameplay_sample_updates_baseline");
        const auto originalBefore = authority.Read().scopes.at(1).originalFov;

        auto enter = Basic(EventKind::CinematicEnter);
        enter.generation = 1;
        enter.aspectContextCompatible = true;
        pass &= Check(authority.Accept(enter).correction == CorrectionOwner::Cinematic,
            "r1_t02_accept_cinematic_enter");

        auto exit = Basic(EventKind::CinematicExit);
        exit.generation = 1;
        exit.evidence.sourceRelationValidated = true;
        pass &= Check(authority.Accept(exit).accepted, "r1_t08_exit_transfers_to_recovery");

        auto arm = Basic(EventKind::ProviderArmed);
        arm.component = 202;
        arm.thread = 303;
        arm.generation = 1;
        arm.providerGeneration = 7;
        arm.evidence.callerValidated = true;
        const auto firstArm = authority.Accept(arm);
        pass &= Check(firstArm.accepted && firstArm.authorityEventId != 0,
            "r1_t18_armer_has_authority_event_identity");

        auto setter = Basic(EventKind::ProviderSetterObserved);
        setter.component = arm.component;
        setter.thread = arm.thread;
        setter.generation = arm.generation;
        setter.providerGeneration = arm.providerGeneration;
        setter.armingEventId = firstArm.authorityEventId;
        setter.fov = 80.0;
        setter.fovProvenance = FovProvenance::PreparedDialogue;
        setter.fovBoundary = FovBoundary::DialogueSetterInput;
        setter.evidence.causalArmingJoinProven = true;
        pass &= Check(authority.Accept(setter).accepted, "r1_t19_setter_bound_to_arm");

        auto writer = Basic(EventKind::ProviderWriterObserved);
        writer.component = arm.component;
        writer.thread = arm.thread;
        writer.generation = arm.generation;
        writer.providerGeneration = arm.providerGeneration;
        writer.armingEventId = firstArm.authorityEventId;
        writer.transactionId = 9001;
        writer.fov = 80.0;
        writer.fovProvenance = FovProvenance::PreparedDialogue;
        writer.fovBoundary = FovBoundary::PrimaryWriterInput;
        writer.evidence.causalArmingJoinProven = true;
        pass &= Check(authority.Accept(writer).accepted, "r1_t20_writer_confirmed_for_same_arm");
        const auto afterPreparedWriter = authority.Read().scopes.at(1);
        pass &= Check(originalBefore && afterPreparedWriter.originalFov &&
            afterPreparedWriter.originalFov->value == 130.311 &&
            afterPreparedWriter.writerInputFov && afterPreparedWriter.writerInputFov->value == 80.0,
            "prepared_writer_input_never_overwrites_original_capture");

        auto recovery = Basic(EventKind::RecoveryComplete);
        recovery.generation = 1;
        recovery.evidence.sourceRelationValidated = true;
        recovery.evidence.recoveryEndpointProven = true;
        const auto transferred = authority.Accept(recovery);
        pass &= Check(transferred.accepted && transferred.baseline == BaselineAction::Exclude &&
            transferred.correction == CorrectionOwner::Dialogue,
            "r1_t13_provider_transfer_has_no_intermediate_gameplay");
        pass &= Check(authority.Read().baseline &&
            authority.Read().baseline->trust == BaselineTrust::Retained,
            "r1_t21_dialogue_retains_compatible_gameplay_reference");

        auto policyChanged = Basic(EventKind::SettingsChanged);
        policyChanged.settingsDomain = SettingsDomain::Dialogue;
        policyChanged.settingsMutation = SettingsMutation::PolicySelection;
        policyChanged.policySelection = 9;
        pass &= Check(authority.Accept(policyChanged).accepted &&
            authority.Read().baseline->trust == BaselineTrust::Retained,
            "dialogue_policy_change_does_not_invalidate_gameplay_reference");
        pass &= Check(authority.Read().scopes.at(1).dialoguePolicySnapshot == 3,
            "provider_activation_keeps_latched_policy_after_setting_change");

        auto terminated = Basic(EventKind::DialogueTerminated);
        terminated.evidence.dialogueTerminalProven = true;
        terminated.evidence.sourceRelationValidated = true;
        pass &= Check(authority.Accept(terminated).accepted,
            "dialogue_terminal_requires_explicit_native_proof");
        auto retainedUse = Basic(EventKind::PrepareEffect);
        retainedUse.transactionId = 9002;
        retainedUse.fov = 153.931;
        retainedUse.fovProvenance = FovProvenance::ModCorrectedOutput;
        retainedUse.fovBoundary = FovBoundary::WriterDecision;
        pass &= Check(!authority.Accept(retainedUse).accepted,
            "retained_baseline_alone_does_not_reauthorize_gameplay_effect");
        return pass;
    }

    bool ExerciseArmingIdentity()
    {
        bool pass = true;
        Authority authority;
        pass &= Check(Initialize(authority), "rearm_initialize");
        auto enter = Basic(EventKind::CinematicEnter);
        enter.generation = 4;
        enter.aspectContextCompatible = true;
        pass &= Check(authority.Accept(enter).accepted, "rearm_enter");
        auto exit = Basic(EventKind::CinematicExit);
        exit.generation = 4;
        exit.evidence.sourceRelationValidated = true;
        pass &= Check(authority.Accept(exit).accepted, "rearm_exit");
        auto arm = Basic(EventKind::ProviderArmed);
        arm.component = 12;
        arm.thread = 17;
        arm.generation = 4;
        arm.providerGeneration = 8;
        arm.evidence.callerValidated = true;
        const auto oldArm = authority.Accept(arm);
        const auto newArm = authority.Accept(arm);
        pass &= Check(oldArm.authorityEventId != newArm.authorityEventId,
            "same_native_tuple_gets_new_authority_event_id");

        auto oldSetter = Basic(EventKind::ProviderSetterObserved);
        oldSetter.component = arm.component;
        oldSetter.thread = arm.thread;
        oldSetter.generation = arm.generation;
        oldSetter.providerGeneration = arm.providerGeneration;
        oldSetter.armingEventId = oldArm.authorityEventId;
        oldSetter.fov = 80.0;
        oldSetter.fovProvenance = FovProvenance::PreparedDialogue;
        oldSetter.fovBoundary = FovBoundary::DialogueSetterInput;
        oldSetter.evidence.causalArmingJoinProven = true;
        const auto before = authority.Read().revision;
        const auto oldDecision = authority.Accept(oldSetter);
        pass &= Check(!oldDecision.accepted && authority.Read().revision == before &&
            authority.Read().scopes.at(1).activeArmingId == newArm.authorityEventId,
            "late_old_setter_cannot_change_new_arm_or_commit_partial_state");

        auto tupleOnly = oldSetter;
        tupleOnly.armingEventId = newArm.authorityEventId;
        tupleOnly.evidence.causalArmingJoinProven = false;
        pass &= Check(!authority.Accept(tupleOnly).accepted,
            "component_thread_generation_tuple_alone_is_not_causal_join");
        return pass;
    }

    bool ExerciseSourceAndProvenance()
    {
        bool pass = true;
        Authority identity;
        Initialize(identity);
        auto unprovenRole = OriginalGameplay(90.0);
        unprovenRole.evidence.gameplayRoleProven = false;
        pass &= Check(!identity.Accept(unprovenRole).accepted && !identity.Read().baseline,
            "no_gameplay_role_proof_means_no_bootstrap_baseline");
        auto unknownFov = OriginalGameplay(90.0);
        unknownFov.fovProvenance = FovProvenance::Unknown;
        pass &= Check(!identity.Accept(unknownFov).accepted,
            "unknown_native_provenance_cannot_admit_baseline");
        pass &= Check(identity.Accept(OriginalGameplay(90.0)).accepted,
            "positive_gameplay_proof_admits_baseline");
        auto replaced = Basic(EventKind::SourceReplaced, 2);
        replaced.previousScope = 1;
        replaced.evidence.sourceRelationValidated = true;
        pass &= Check(identity.Accept(replaced).baseline == BaselineAction::Invalidate &&
            identity.Read().baseline->trust == BaselineTrust::Invalid,
            "source_replacement_invalidates_old_baseline");

        Authority receiverReplacement;
        Initialize(receiverReplacement);
        receiverReplacement.Accept(OriginalGameplay(130.0, 1, 101));
        auto newReceiverEnter = Basic(EventKind::CinematicEnter, 1);
        newReceiverEnter.receiver = 102;
        newReceiverEnter.generation = 5;
        pass &= Check(receiverReplacement.Accept(newReceiverEnter).accepted &&
            receiverReplacement.Read().baseline->trust == BaselineTrust::Invalid,
            "same_source_new_receiver_invalidates_baseline_identity");

        Authority scopes;
        Initialize(scopes);
        auto candidateB = Basic(EventKind::DialogueCandidate, 2);
        pass &= Check(scopes.Accept(candidateB).accepted, "foreign_scope_candidate_is_scoped");
        pass &= Check(scopes.Accept(OriginalGameplay(120.0)).baseline == BaselineAction::Update &&
            scopes.Read().scopes.at(2).dialogue == DialoguePhase::Candidate,
            "foreign_candidate_does_not_globally_veto_proven_gameplay");

        Authority quarantine;
        Initialize(quarantine);
        auto qEnter = Basic(EventKind::CinematicEnter);
        qEnter.generation = 3;
        quarantine.Accept(qEnter);
        auto qExit = Basic(EventKind::CinematicExit);
        qExit.generation = 3;
        qExit.evidence.sourceRelationValidated = true;
        quarantine.Accept(qExit);
        auto qArm = Basic(EventKind::ProviderArmed);
        qArm.component = 51;
        qArm.thread = 61;
        qArm.generation = 3;
        qArm.evidence.callerValidated = true;
        quarantine.Accept(qArm);
        auto incompleteTransfer = Basic(EventKind::RecoveryComplete);
        incompleteTransfer.generation = 3;
        incompleteTransfer.evidence.recoveryEndpointProven = true;
        incompleteTransfer.evidence.sourceRelationValidated = true;
        incompleteTransfer.evidence.gameplayRoleProven = true;
        incompleteTransfer.fov = 70.0;
        incompleteTransfer.fovProvenance = FovProvenance::OriginalNative;
        incompleteTransfer.fovBoundary = FovBoundary::RecoveryEndpoint;
        const auto incompleteDecision = quarantine.Accept(incompleteTransfer);
        pass &= Check(incompleteDecision.accepted &&
            incompleteDecision.baseline == BaselineAction::Exclude && !quarantine.Read().baseline &&
            quarantine.Read().scopes.at(1).dialogue != DialoguePhase::Active,
            "provider_quarantine_without_correlated_writer_cannot_fall_through_to_gameplay");
        return pass;
    }

    bool ExerciseEffectReadbackCorrelation()
    {
        bool pass = true;
        Authority authority;
        Initialize(authority);
        authority.Accept(OriginalGameplay(100.0));
        auto prepare = Basic(EventKind::PrepareEffect);
        prepare.transactionId = 44;
        prepare.fov = 147.796;
        prepare.fovProvenance = FovProvenance::ModCorrectedOutput;
        prepare.fovBoundary = FovBoundary::WriterDecision;
        const auto intent = authority.Accept(prepare);
        pass &= Check(intent.accepted && intent.effectId != 0,
            "prepare_commits_single_call_effect_intent");
        const auto planned = authority.Read().scopes.at(1).plannedOutputFov;
        pass &= Check(planned && planned->transactionId == 44 && planned->value == 147.796,
            "planned_output_keeps_named_writer_transaction");

        auto readback = Basic(EventKind::EffectObserved);
        readback.effectId = intent.effectId;
        readback.transactionId = 44;
        readback.dependencyRevision = intent.revision;
        readback.fov = 147.796;
        readback.fovProvenance = FovProvenance::EffectiveReadback;
        readback.fovBoundary = FovBoundary::PostStore;
        readback.effectResult = EffectResult::Applied;
        readback.evidence.effectBoundaryCorrelated = true;
        pass &= Check(authority.Accept(readback).accepted, "same_transaction_post_store_is_recorded");
        auto observed = authority.Read().effects.at(intent.effectId);
        pass &= Check(observed.postStoreResult == EffectResult::Applied && observed.postStoreFov == 147.796 &&
            !observed.postHelperResult,
            "post_store_does_not_fabricate_post_helper_observation");

        auto postHelper = readback;
        postHelper.fovBoundary = FovBoundary::PostHelper;
        postHelper.fov = 146.5;
        pass &= Check(authority.Accept(postHelper).accepted, "same_transaction_post_helper_is_separate");
        observed = authority.Read().effects.at(intent.effectId);
        pass &= Check(observed.postStoreFov == 147.796 && observed.postHelperFov == 146.5 &&
            authority.Read().scopes.at(1).postStoreFov->boundary == FovBoundary::PostStore &&
            authority.Read().scopes.at(1).postHelperFov->boundary == FovBoundary::PostHelper,
            "post_store_and_post_helper_are_independent_witnesses");

        auto duplicate = postHelper;
        const auto beforeDuplicate = authority.Read().revision;
        pass &= Check(!authority.Accept(duplicate).accepted && authority.Read().revision == beforeDuplicate,
            "duplicate_named_readback_is_rejected");
        auto nMinusOne = readback;
        nMinusOne.transactionId = 43;
        pass &= Check(!authority.Accept(nMinusOne).accepted, "n_minus_one_readback_cannot_match_intent");
        auto wrongReceiver = readback;
        wrongReceiver.receiver = 102;
        pass &= Check(!authority.Accept(wrongReceiver).accepted, "receiver_mismatch_readback_is_rejected");
        auto wrongGeneration = readback;
        wrongGeneration.generation = 1;
        pass &= Check(!authority.Accept(wrongGeneration).accepted,
            "generation_mismatch_readback_is_rejected");
        auto missingBoundary = readback;
        missingBoundary.fovBoundary = FovBoundary::None;
        pass &= Check(!authority.Accept(missingBoundary).accepted, "unnamed_readback_boundary_is_rejected");
        auto missingCorrelation = readback;
        missingCorrelation.evidence.effectBoundaryCorrelated = false;
        pass &= Check(!authority.Accept(missingCorrelation).accepted,
            "missing_readback_correlation_stays_unavailable");

        auto disable = Basic(EventKind::SettingsChanged);
        disable.settingsDomain = SettingsDomain::Global;
        disable.enabled = false;
        authority.Accept(disable);
        auto anotherPrepare = prepare;
        anotherPrepare.transactionId = 45;
        pass &= Check(!authority.Accept(anotherPrepare).accepted,
            "settings_off_revokes_new_effect_permission");

        Authority revoked;
        Initialize(revoked);
        revoked.Accept(OriginalGameplay(100.0));
        auto pendingPrepare = prepare;
        pendingPrepare.transactionId = 46;
        const auto pending = revoked.Accept(pendingPrepare);
        auto revoke = Basic(EventKind::SettingsChanged);
        revoke.settingsDomain = SettingsDomain::Global;
        revoke.enabled = false;
        revoked.Accept(revoke);
        auto lateReadback = Basic(EventKind::EffectObserved);
        lateReadback.effectId = pending.effectId;
        lateReadback.transactionId = 46;
        lateReadback.dependencyRevision = pending.revision;
        lateReadback.fov = pendingPrepare.fov;
        lateReadback.fovProvenance = FovProvenance::EffectiveReadback;
        lateReadback.fovBoundary = FovBoundary::PostStore;
        lateReadback.effectResult = EffectResult::Applied;
        lateReadback.evidence.effectBoundaryCorrelated = true;
        pass &= Check(pending.accepted && !revoked.Accept(lateReadback).accepted &&
            revoked.Read().effects.at(pending.effectId).revoked,
            "settings_revoke_prevents_fabricated_applied_result");
        return pass;
    }

    bool ExerciseFailureAndTerminalPaths()
    {
        bool pass = true;
        Authority partial;
        Initialize(partial);
        partial.Accept(OriginalGameplay(100.0));
        auto prepare = Basic(EventKind::PrepareEffect);
        prepare.transactionId = 55;
        prepare.fov = 120.0;
        prepare.fovProvenance = FovProvenance::ModCorrectedOutput;
        prepare.fovBoundary = FovBoundary::WriterDecision;
        const auto intent = partial.Accept(prepare);
        const auto baselineSource = partial.Read().baseline->source;
        auto partialResult = Basic(EventKind::EffectObserved);
        partialResult.effectId = intent.effectId;
        partialResult.transactionId = 55;
        partialResult.dependencyRevision = intent.revision;
        partialResult.fov = 119.5;
        partialResult.fovProvenance = FovProvenance::EffectiveReadback;
        partialResult.fovBoundary = FovBoundary::PostStore;
        partialResult.effectResult = EffectResult::Partial;
        partialResult.evidence.effectBoundaryCorrelated = true;
        pass &= Check(partial.Accept(partialResult).accepted &&
            partial.Read().baseline->source == baselineSource &&
            partial.Read().effects.at(intent.effectId).postStoreResult == EffectResult::Partial,
            "partial_effect_is_observation_without_lifecycle_rollback");

        Authority terminal;
        Initialize(terminal);
        terminal.Accept(Basic(EventKind::DialogueCandidate));
        auto activate = Basic(EventKind::DialogueActivated);
        activate.evidence.dialogueDescentProven = true;
        terminal.Accept(activate);
        auto noTerminalProof = Basic(EventKind::DialogueTerminated);
        pass &= Check(!terminal.Accept(noTerminalProof).accepted &&
            terminal.Read().scopes.at(1).dialogue == DialoguePhase::Active,
            "unproven_end_does_not_synthesize_dialogue_termination");
        return pass;
    }

    bool ExerciseCapturedProviderShadowReplay()
    {
        bool pass = true;
        constexpr std::uint64_t generation = 5;
        constexpr std::uint64_t settingsEpoch = 1;
        constexpr std::uint64_t receiver = 0x22c91546700;
        constexpr std::uint64_t component = 0x22e0e5c5320;
        constexpr std::uint32_t thread = 21200;
        constexpr float nativeStart = 130.311f;
        constexpr float nativeTarget = 70.0f;
        constexpr float joinedSample = 130.264f;

        // Recorded in PID 21200 at 01:30:18.822/.842/.861. Use the same
        // production helper for the bounded provider relation, then feed those
        // observed facts into the isolated Authority.
        dialogue::PostExitProviderHandoff currentPath;
        const dialogue::PostExitProviderActivation activation{generation,
            settingsEpoch, component, receiver, thread, nativeStart, nativeTarget,
            nativeStart, config::DialogueZoomPolicy::Reduced};
        pass &= Check(currentPath.ObserveArmer(true, activation),
            "shadow_current_path_accepts_recorded_armer");
        pass &= Check(!currentPath.TakeWriterSample(true, receiver, thread,
            generation, settingsEpoch, nativeStart).valid,
            "shadow_current_path_waits_for_setter_before_writer_confirmation");
        pass &= Check(currentPath.ObserveSetterSample(true, component, receiver,
            thread, generation, settingsEpoch, nativeStart, nativeTarget,
            joinedSample, joinedSample),
            "shadow_current_path_matches_recorded_setter");
        const auto currentWriter = currentPath.TakeWriterSample(true, receiver,
            thread, generation, settingsEpoch, joinedSample);
        pass &= Check(currentWriter.valid && currentWriter.sample == joinedSample,
            "shadow_current_path_confirms_recorded_writer");

        Authority authority;
        pass &= Check(Initialize(authority), "shadow_authority_initialize");
        auto reducedPolicy = Basic(EventKind::SettingsChanged);
        reducedPolicy.settingsDomain = SettingsDomain::Dialogue;
        reducedPolicy.settingsMutation = SettingsMutation::PolicySelection;
        reducedPolicy.policySelection = static_cast<std::uint32_t>(
            config::DialogueZoomPolicy::Reduced);
        pass &= Check(authority.Accept(reducedPolicy).accepted,
            "shadow_authority_selects_captured_reduced_policy");
        auto gameplay = OriginalGameplay(nativeStart, 1, receiver);
        pass &= Check(authority.Accept(gameplay).baseline == BaselineAction::Update,
            "shadow_authority_admits_existing_gameplay_reference");
        auto enter = Basic(EventKind::CinematicEnter);
        enter.receiver = receiver;
        enter.generation = generation;
        enter.aspectContextCompatible = true;
        pass &= Check(authority.Accept(enter).correction == CorrectionOwner::Cinematic,
            "shadow_authority_accepts_cinematic_enter");
        auto exit = Basic(EventKind::CinematicExit);
        exit.receiver = receiver;
        exit.generation = generation;
        exit.evidence.sourceRelationValidated = true;
        pass &= Check(authority.Accept(exit).accepted, "shadow_authority_accepts_cinematic_exit");

        // In this capture recovery completes for the same source before the
        // provider-return armer appears. Keep those two facts distinct.
        auto recovery = Basic(EventKind::RecoveryComplete);
        recovery.receiver = receiver;
        recovery.generation = generation;
        recovery.fov = nativeStart;
        recovery.fovProvenance = FovProvenance::OriginalNative;
        recovery.fovBoundary = FovBoundary::RecoveryEndpoint;
        recovery.evidence.sourceRelationValidated = true;
        recovery.evidence.recoveryEndpointProven = true;
        recovery.evidence.gameplayRoleProven = true;
        pass &= Check(authority.Accept(recovery).baseline == BaselineAction::Update,
            "shadow_authority_commits_same_source_recovery_before_provider");

        auto arm = Basic(EventKind::ProviderArmed);
        arm.receiver = receiver;
        arm.component = component;
        arm.thread = thread;
        arm.generation = generation;
        arm.providerGeneration = generation;
        arm.evidence.callerValidated = true;
        const auto armResult = authority.Accept(arm);
        pass &= Check(armResult.accepted && armResult.authorityEventId != 0 &&
            authority.Read().scopes.at(1).dialogue == DialoguePhase::Candidate,
            "provider_arm_creates_only_a_scoped_dialogue_candidate");

        auto setter = Basic(EventKind::ProviderSetterObserved);
        setter.receiver = receiver;
        setter.component = component;
        setter.thread = thread;
        setter.generation = generation;
        setter.providerGeneration = generation;
        setter.armingEventId = armResult.authorityEventId;
        setter.fov = joinedSample;
        setter.fovProvenance = FovProvenance::PreparedDialogue;
        setter.fovBoundary = FovBoundary::DialogueSetterInput;
        setter.evidence.causalArmingJoinProven = true;
        pass &= Check(authority.Accept(setter).accepted,
            "shadow_authority_binds_setter_to_current_arm");

        auto writer = Basic(EventKind::ProviderWriterObserved);
        writer.receiver = receiver;
        writer.component = component;
        writer.thread = thread;
        writer.generation = generation;
        writer.providerGeneration = generation;
        writer.armingEventId = armResult.authorityEventId;
        writer.transactionId = 8839;
        writer.fov = joinedSample;
        writer.fovProvenance = FovProvenance::PreparedDialogue;
        writer.fovBoundary = FovBoundary::PrimaryWriterInput;
        writer.evidence.causalArmingJoinProven = true;
        const auto shadowWriter = authority.Accept(writer);
        pass &= Check(shadowWriter.accepted && currentWriter.valid &&
            shadowWriter.correction == CorrectionOwner::Dialogue &&
            shadowWriter.selectedPolicy == static_cast<std::uint32_t>(
                config::DialogueZoomPolicy::Reduced),
            "shadow_and_current_select_reduced_for_recorded_writer");
        auto dialogueStart = Basic(EventKind::DialogueActivated);
        dialogueStart.receiver = receiver;
        dialogueStart.evidence.dialogueDescentProven = true;
        pass &= Check(authority.Accept(dialogueStart).accepted &&
            authority.Read().scopes.at(1).dialogue == DialoguePhase::Active &&
            authority.Read().baseline &&
            authority.Read().baseline->trust == BaselineTrust::Retained,
            "shadow_authority_activates_dialogue_after_observed_descent");
        const float target = camera::HorPlus(dialogue::ResolveDialogueDestinationTarget(
            config::DialogueZoomPolicy::Reduced, nativeStart, nativeTarget),
            32.0f / 9.0f, 16.0f / 9.0f);
        const float startOutput = camera::HorPlus(nativeStart, 32.0f / 9.0f, 16.0f / 9.0f);
        const float mappedOutput = dialogue::MapProjectionTransitionSample(currentWriter.sample,
            nativeStart, nativeTarget, startOutput, target);
        pass &= Check(std::fabs(mappedOutput - 153.925f) < 0.003f &&
            std::fabs(target - 149.071f) < 0.003f,
            "shadow_production_mapping_computes_captured_output_and_target");
        std::cout << "shadow_match generation=5 provider=armer-setter-writer "
            "policy=Reduced output=153.925 target=149.071 (captured)\n";
        return pass;
    }

    struct CurrentWriterProjection
    {
        std::mutex mutex;
        std::atomic<camera::CoordinatorState> coordinator{camera::CoordinatorState::Gameplay};
        dialogue::PostCinematicRecoveryExclusion exclusion;
        dialogue::Phase phase{dialogue::Phase::Inactive};
        bool providerOwned{};
        std::uintptr_t providerCallbackSource{};
        std::uintptr_t providerReceiver{};
        std::uint64_t providerGeneration{};
        std::uint64_t providerEpoch{};
        std::uint32_t providerThread{};
        dialogue::PolicySnapshot activePolicy;
        dialogue::CorrectionActivation activation;
        float baseline{NAN};
        float previous{NAN};
        float exitStart{NAN};
        dialogue::CandidateTracker candidate;
        dialogue::RecoveryRearm recovery;
        std::atomic<bool> enabled{true};
        std::atomic<config::DialogueZoomPolicy> selectedPolicy{
            config::DialogueZoomPolicy::Reduced};
        std::atomic<std::uint64_t> settingsEpoch{1};
        std::atomic<std::uint64_t> activeEpoch{};
        std::atomic<std::uintptr_t> snapshotSource{};
        std::atomic<float> snapshotTarget{NAN};
        std::atomic<bool> snapshotTargetValid{};

        plugin::DialogueBoundaryStateView View()
        {
            return {mutex, coordinator, exclusion, phase, providerOwned,
                providerCallbackSource, providerReceiver, providerGeneration,
                providerEpoch, providerThread, activePolicy, activation,
                baseline, previous, exitStart, candidate, recovery, enabled,
                selectedPolicy, settingsEpoch, activeEpoch, snapshotSource,
                snapshotTarget, snapshotTargetValid};
        }
    };

    bool ExerciseNewSourceAndDialogueScopeLimits()
    {
        bool pass = true;
        constexpr std::uint64_t receiverA = 0x22707d56700;
        constexpr std::uint64_t receiverB = 0x2279302d9c0;
        Authority authority;
        pass &= Check(Initialize(authority), "shadow_source_initialize");
        auto seed = OriginalGameplay(90.3115, 1, receiverA);
        pass &= Check(authority.Accept(seed).accepted, "shadow_source_seed_A");
        auto replace = Basic(EventKind::SourceReplaced, 2);
        replace.receiver = receiverB;
        replace.previousScope = 1;
        replace.evidence.sourceRelationValidated = true;
        pass &= Check(authority.Accept(replace).baseline == BaselineAction::Invalidate,
            "shadow_source_replacement_invalidates_A");
        auto newSource = OriginalGameplay(130.3115, 2, receiverB);
        newSource.evidence.gameplayRoleProven = false;
        const auto authorityNewSource = authority.Accept(newSource);

        CurrentWriterProjection current;
        const auto currentClassification = plugin::ClassifyDialogueWriterContext(
            current.View(), static_cast<std::uintptr_t>(receiverB), 1, 5);
        pass &= Check(!authorityNewSource.accepted && authority.Read().baseline &&
            authority.Read().baseline->trust == BaselineTrust::Invalid &&
            currentClassification == camera::WriterContext::GameplayEligible,
            "shadow_exposes_new_receiver_gameplay_admission_divergence");
        std::cout << "shadow_divergence new_receiver current=GameplayEligible "
            "authority=Unknown baseline=Invalid\n";

        // A still-active Dialogue scope does not veto a different source when
        // that source supplies its own explicit Gameplay-role evidence.
        auto dialogueCandidate = Basic(EventKind::DialogueCandidate, 1);
        dialogueCandidate.receiver = receiverA;
        pass &= Check(authority.Accept(dialogueCandidate).accepted,
            "shadow_old_source_dialogue_candidate");
        auto dialogueActive = Basic(EventKind::DialogueActivated, 1);
        dialogueActive.receiver = receiverA;
        dialogueActive.evidence.dialogueDescentProven = true;
        pass &= Check(authority.Accept(dialogueActive).accepted,
            "shadow_old_source_dialogue_active");
        auto independentlyProvenB = OriginalGameplay(130.3115, 2, receiverB);
        pass &= Check(authority.Accept(independentlyProvenB).accepted &&
            authority.Read().scopes.at(1).dialogue == DialoguePhase::Active &&
            authority.Read().baseline->source == 2,
            "dialogue_owner_is_scoped_away_from_independent_proven_source");
        auto unprovenEnd = Basic(EventKind::DialogueTerminated, 1);
        unprovenEnd.receiver = receiverA;
        const auto beforeEnd = authority.Read().revision;
        pass &= Check(!authority.Accept(unprovenEnd).accepted &&
            authority.Read().revision == beforeEnd &&
            authority.Read().scopes.at(1).dialogue == DialoguePhase::Active,
            "dialogue_silence_does_not_synthesize_terminal_event");
        return pass;
    }

    bool ExerciseSameSourceRecoveryCompletionBeforeNextCinematic()
    {
        bool pass = true;
        constexpr std::uint64_t generation = 4;
        constexpr std::uintptr_t receiver = 0x22c91546700;
        Authority authority;
        pass &= Check(Initialize(authority), "same_source_recovery_shadow_initialize");
        pass &= Check(authority.Accept(OriginalGameplay(130.311, 1, receiver)).accepted,
            "same_source_recovery_shadow_seed_current_gameplay_reference");

        auto enter = Basic(EventKind::CinematicEnter);
        enter.receiver = receiver;
        enter.generation = generation;
        enter.aspectContextCompatible = true;
        pass &= Check(authority.Accept(enter).correction == CorrectionOwner::Cinematic,
            "same_source_recovery_shadow_cinematic_enter");
        auto exit = Basic(EventKind::CinematicExit);
        exit.receiver = receiver;
        exit.generation = generation;
        exit.evidence.sourceRelationValidated = true;
        pass &= Check(authority.Accept(exit).accepted, "same_source_recovery_shadow_cinematic_exit");

        // Source-bound recovery admits the exact native endpoint
        // in the completion transaction. There is no extra Gameplay writer
        // between completion and the neutral setter Candidate.
        auto recovery = Basic(EventKind::RecoveryComplete);
        recovery.receiver = receiver;
        recovery.generation = generation;
        recovery.fov = 130.311;
        recovery.fovProvenance = FovProvenance::OriginalNative;
        recovery.fovBoundary = FovBoundary::RecoveryEndpoint;
        recovery.evidence.sourceRelationValidated = true;
        recovery.evidence.recoveryEndpointProven = true;
        recovery.evidence.gameplayRoleProven = true;
        const auto completed = authority.Accept(recovery);
        pass &= Check(completed.accepted && completed.baseline == BaselineAction::Update &&
            authority.Read().baseline &&
            authority.Read().baseline->trust == BaselineTrust::Current &&
            authority.Read().baseline->fov == 130.311,
            "same_source_recovery_shadow_completion_commits_same-transaction_gameplay-baseline");

        auto neutralSetter = Basic(EventKind::DialogueCandidate);
        neutralSetter.receiver = receiver;
        pass &= Check(authority.Accept(neutralSetter).accepted &&
            authority.Read().baseline->trust == BaselineTrust::Current,
            "same_source_recovery_shadow_neutral_setter_does_not_revoke_current-baseline");

        auto nextEnter = Basic(EventKind::CinematicEnter);
        nextEnter.receiver = receiver;
        nextEnter.generation = generation + 1;
        nextEnter.aspectContextCompatible = true;
        const auto nextScene = authority.Accept(nextEnter);
        pass &= Check(nextScene.accepted &&
            nextScene.correction == CorrectionOwner::Cinematic &&
            authority.Read().baseline &&
            authority.Read().baseline->trust != BaselineTrust::Invalid &&
            authority.Read().baseline->fov == 130.311,
            "same_source_recovery_shadow_second_cinematic_keeps_same-source-gameplay-reference");
        camera::GameplayBaseline actualReference{};
        const auto retained = authority.Read().baseline;
        actualReference.valid = retained.has_value();
        actualReference.current = retained && (retained->trust == BaselineTrust::Current ||
            retained->trust == BaselineTrust::Retained);
        actualReference.aspect = 32.0f / 9.0f;
        actualReference.nativeFov = {retained ? static_cast<float>(retained->fov) : NAN,
            camera::FovSpace::Native, camera::FovProvenance::NativeRegisterInput, true};
        const auto selected = cinematics::SelectCinematicBaseline(true, 90.0f, actualReference);
        float selectedOutput{};
        pass &= Check(selected.valueOrigin ==
                cinematics::CinematicBaselineValueOrigin::GameplayNativeNumericBaseline &&
            cinematics::TryTransformSelectedCinematicFov(true,
                config::CinematicFovMode::GameplayHorPlus, 90.0f,
                selected.authoredReferenceFov, selected.targetBaselineFov,
                actualReference.aspect, 16.0f / 9.0f, selectedOutput) &&
            std::fabs(selectedOutput - 153.931f) < 0.002f,
            "same_source_recovery_authority_reference_drives_real_production_selector_and_formula");
        std::cout << "shadow_match SameSourceRecoveryCompletionBeforeNextCinematic completion=GameplayNativeNumericBaseline "
            "native=130.311 target=153.931 (captured contract)\n";
        return pass;
    }


    bool ExerciseHardening()
    {
        bool pass = true;
        Authority candidate;
        Initialize(candidate);
        candidate.Accept(Basic(EventKind::DialogueCandidate));
        auto cancel = Basic(EventKind::DialogueTerminated);
        cancel.evidence.dialogueTerminalProven = true;
        cancel.evidence.sourceRelationValidated = true;
        pass &= Check(candidate.Accept(cancel).accepted &&
            candidate.Read().scopes.at(1).dialogue == DialoguePhase::None,
            "candidate_has_source_bound_terminal_without_setter");

        Authority independent;
        Initialize(independent);
        independent.Accept(OriginalGameplay(130.311));
        auto setting = Basic(EventKind::SettingsChanged);
        setting.settingsDomain = SettingsDomain::Gameplay;
        setting.settingsMutation = SettingsMutation::PolicySelection;
        setting.policySelection = 77;
        independent.Accept(setting);
        auto enter = Basic(EventKind::CinematicEnter);
        enter.generation = 1;
        enter.aspectContextCompatible = true;
        independent.Accept(enter);
        auto prepare = Basic(EventKind::PrepareEffect);
        prepare.generation = 1;
        prepare.transactionId = 700;
        prepare.fov = 126.87;
        prepare.fovBoundary = FovBoundary::WriterDecision;
        prepare.fovProvenance = FovProvenance::ModCorrectedOutput;
        pass &= Check(independent.Accept(prepare).accepted,
            "independent_cinematic_effect_survives_invalid_gameplay_baseline");
        auto stale = prepare;
        stale.generation = 9;
        pass &= Check(!independent.Accept(stale).accepted,
            "wrong_generation_cannot_prepare_cinematic_effect");

        Authority supersession;
        Initialize(supersession);
        supersession.Accept(enter);
        const auto intent = supersession.Accept(prepare);
        enter.generation = 2;
        supersession.Accept(enter);
        pass &= Check(intent.accepted && supersession.Read().effects.at(intent.effectId).revoked,
            "new_cinematic_generation_revokes_old_effect");

        auto oldResult = Basic(EventKind::EffectObserved);
        oldResult.effectId = intent.effectId;
        oldResult.transactionId = 700;
        oldResult.generation = 1;
        oldResult.dependencyRevision = intent.revision;
        oldResult.fov = 126.87;
        oldResult.fovBoundary = FovBoundary::PostStore;
        oldResult.fovProvenance = FovProvenance::EffectiveReadback;
        oldResult.effectResult = EffectResult::Applied;
        oldResult.evidence.effectBoundaryCorrelated = true;
        pass &= Check(!supersession.Accept(oldResult).accepted,
            "late_old_generation_readback_cannot_refresh_current_projection");
        Authority independentDialogue;
        Initialize(independentDialogue);
        independentDialogue.Accept(OriginalGameplay(130.311));
        independentDialogue.Accept(setting);
        independentDialogue.Accept(Basic(EventKind::DialogueCandidate));
        auto activateDialogue = Basic(EventKind::DialogueActivated);
        activateDialogue.evidence.dialogueDescentProven = true;
        independentDialogue.Accept(activateDialogue);
        auto dialogueEffect = prepare;
        dialogueEffect.generation = 0;
        pass &= Check(independentDialogue.Accept(dialogueEffect).accepted,
            "independent_dialogue_effect_survives_invalid_gameplay_baseline");

        prepare.gameplayBaselineDependent = true;
        prepare.generation = 1;
        pass &= Check(!independent.Accept(prepare).accepted,
            "invalid_reference_still_blocks_baseline_dependent_effect");
        prepare.gameplayBaselineDependent = false;

        Authority admission;
        Initialize(admission);
        auto policy = OriginalGameplay(130.311, 2, 202);
        policy.evidence.gameplayRoleProven = false;
        policy.requestPolicyAdmission = true;
        pass &= Check(!admission.Accept(policy).accepted,
            "policy_request_requires_source_bound_writer_relation");
        policy.evidence.sourceRelationValidated = true;
        const auto admitted = admission.Accept(policy);
        pass &= Check(admitted.accepted && admitted.admission == GameplayAdmission::PolicyAdmitted &&
            admission.Read().baseline->admission == GameplayAdmission::PolicyAdmitted,
            "policy_admission_is_not_native_proof");
        auto conflicting = Basic(EventKind::DialogueCandidate, 2);
        conflicting.receiver = 202;
        admission.Accept(conflicting);
        pass &= Check(!admission.Accept(policy).accepted,
            "candidate_cannot_be_silently_overwritten_by_policy_admission");
        auto foreignCancel = Basic(EventKind::DialogueTerminated, 2);
        foreignCancel.evidence.dialogueTerminalProven = true;
        foreignCancel.evidence.sourceRelationValidated = true;
        pass &= Check(!admission.Accept(foreignCancel).accepted,
            "foreign_receiver_cannot_cancel_candidate");
        foreignCancel.receiver = 202;
        pass &= Check(admission.Accept(foreignCancel).accepted && admission.Accept(policy).accepted,
            "explicit_candidate_terminal_restores_policy_admission");

        Authority bounded;
        Initialize(bounded);
        bounded.Accept(OriginalGameplay(130.311));
        auto effect = prepare;
        effect.generation = 0;
        for (unsigned i = 0; i < 400; ++i) {
            effect.transactionId = 1000 + i;
            const auto planned = bounded.Accept(effect);
            auto close = Basic(EventKind::EffectClosed);
            close.effectId = planned.effectId;
            close.transactionId = effect.transactionId;
            close.dependencyRevision = planned.revision;
            if (!planned.accepted || !bounded.Accept(close).accepted) { pass = false; break; }
        }
        pass &= Check(bounded.Read().effects.empty(),
            "explicit_close_bounds_diagnostics_off_effect_history");
        EventId firstId{};
        std::uint64_t firstRevision{};
        for (unsigned i = 0; i < Authority::MaxEffects; ++i) {
            effect.transactionId = 2000 + i;
            const auto planned = bounded.Accept(effect);
            if (i == 0) { firstId = planned.effectId; firstRevision = planned.revision; }
            if (!planned.accepted) pass = false;
        }
        effect.transactionId = 9999;
        pass &= Check(!bounded.Accept(effect).accepted &&
            bounded.Read().effects.size() == Authority::MaxEffects,
            "full_pending_capacity_applies_backpressure_without_eviction");
        auto delayed = Basic(EventKind::EffectObserved);
        delayed.effectId = firstId;
        delayed.transactionId = 2000;
        delayed.dependencyRevision = firstRevision;
        delayed.fov = 126.87;
        delayed.fovBoundary = FovBoundary::PostStore;
        delayed.fovProvenance = FovProvenance::EffectiveReadback;
        delayed.effectResult = EffectResult::Applied;
        delayed.evidence.effectBoundaryCorrelated = true;
        pass &= Check(bounded.Accept(delayed).accepted,
            "oldest_pending_observation_survives_capacity_pressure");

        Authority rearming;
        Initialize(rearming);
        rearming.Accept(enter);
        auto exit = enter;
        exit.kind = EventKind::CinematicExit;
        exit.evidence.sourceRelationValidated = true;
        rearming.Accept(exit);
        auto arm = Basic(EventKind::ProviderArmed);
        arm.generation = enter.generation;
        arm.component = 10;
        arm.thread = 20;
        arm.evidence.callerValidated = true;
        for (unsigned i = 0; i < 400; ++i)
            if (!rearming.Accept(arm).accepted) pass = false;
        pass &= Check(rearming.Read().armings.size() == 1,
            "superseded_arming_history_is_bounded");
        auto cancelProvider = Basic(EventKind::DialogueTerminated);
        cancelProvider.evidence.sourceRelationValidated = true;
        cancelProvider.evidence.dialogueTerminalProven = true;
        pass &= Check(rearming.Accept(cancelProvider).accepted &&
            rearming.Read().scopes.at(1).activeArmingId == 0,
            "provider_candidate_cancellation_retires_quarantine");
        auto missingReadbackClose = Basic(EventKind::EffectClosed);
        missingReadbackClose.effectId = firstId;
        missingReadbackClose.transactionId = 2000;
        missingReadbackClose.dependencyRevision = firstRevision;
        pass &= Check(bounded.Accept(missingReadbackClose).accepted && bounded.Accept(effect).accepted,
            "explicit_observation_close_releases_capacity_without_fabricated_readback");


        // Exercise the real selector and formula code, with independent numeric
        // expectations from the preserved 130-degree runtime scenario.
        camera::GameplayBaseline baseline{};
        baseline.valid = true;
        baseline.current = true;
        baseline.aspect = 32.0f / 9.0f;
        baseline.nativeFov = {130.311f, camera::FovSpace::Native,
            camera::FovProvenance::NativeRegisterInput, true};
        const auto selection = cinematics::SelectCinematicBaseline(true, 90.0f, baseline);
        float cineOutput{};
        const bool mapped = cinematics::TryTransformSelectedCinematicFov(true,
            config::CinematicFovMode::GameplayHorPlus, 90.0f,
            selection.authoredReferenceFov, selection.targetBaselineFov,
            baseline.aspect, 16.0f / 9.0f, cineOutput);
        pass &= Check(selection.usedGameplayBaselineValue && mapped &&
            std::fabs(cineOutput - 153.931f) < 0.002f,
            "production_cinematic_selector_and_mapping_match_runtime_number");
        const float reduced = dialogue::ResolveDialogueDestinationTarget(
            config::DialogueZoomPolicy::Reduced, 130.311f, 70.0f);
        const float dialogueOutput = camera::HorPlus(reduced, baseline.aspect, 16.0f / 9.0f);
        const float transitionOutput = dialogue::MapProjectionTransitionSample(
            130.264f, 130.311f, 70.0f, cineOutput, dialogueOutput);
        pass &= Check(std::fabs(dialogueOutput - 149.071f) < 0.003f &&
            std::fabs(transitionOutput - 153.925f) < 0.003f,
            "production_reduced_and_provider_mapping_match_runtime_numbers");
        baseline.current = false;
        const auto fallback = cinematics::SelectCinematicBaseline(true, 90.0f, baseline);
        pass &= Check(!fallback.usedGameplayBaselineValue && fallback.targetBaselineFov == 90.0f,
            "production_selector_rejects_stale_baseline_after_replacement");
        std::cout << (pass ? "PASS" : "FAIL") << ": hardening assertions=" << assertions << '\n';
        return pass;
    }



    std::optional<WriterProposal> CompositePolicy(void*, const WriterPolicyContext& context)
    {
        constexpr float aspect = 32.0f / 9.0f;
        constexpr float native = 16.0f / 9.0f;
        if (context.owner == CorrectionOwner::Gameplay)
            return WriterProposal{camera::HorPlus(static_cast<float>(context.incomingFov), aspect, native), false};
        if (context.owner == CorrectionOwner::Cinematic) {
            camera::GameplayBaseline reference{};
            if (context.baseline) {
                reference.valid = true;
                reference.current = context.baseline->trust == BaselineTrust::Current ||
                    context.baseline->trust == BaselineTrust::Retained;
                reference.aspect = aspect;
                reference.nativeFov = {static_cast<float>(context.baseline->fov),
                    camera::FovSpace::Native, camera::FovProvenance::NativeRegisterInput, true};
            }
            const auto selection = cinematics::SelectCinematicBaseline(true, 90.0f, reference);
            float output{};
            if (!cinematics::TryTransformSelectedCinematicFov(true,
                config::CinematicFovMode::GameplayHorPlus, static_cast<float>(context.incomingFov),
                selection.authoredReferenceFov, selection.targetBaselineFov, aspect, native, output))
                return std::nullopt;
            return WriterProposal{output, selection.usedGameplayBaselineValue};
        }
        if (context.owner == CorrectionOwner::Dialogue && context.baseline) {
            const auto baseline = static_cast<float>(context.baseline->fov);
            const float reduced = dialogue::ResolveDialogueDestinationTarget(
                config::DialogueZoomPolicy::Reduced, baseline, 70.0f);
            const float target = camera::HorPlus(reduced, aspect, native);
            const float output = context.inputProvenance == FovProvenance::PreparedDialogue
                ? camera::HorPlus(static_cast<float>(context.incomingFov), aspect, native)
                : dialogue::MapProjectionTransitionSample(static_cast<float>(context.incomingFov),
                    baseline, 70.0f, camera::HorPlus(baseline, aspect, native), target);
            return WriterProposal{output, true};
        }
        return std::nullopt;
    }

    WriterEvent CompositeInput(double fov = 130.311)
    {
        WriterEvent event;
        event.input = OriginalGameplay(fov);
        event.input.evidence.gameplayRoleProven = false;
        event.input.evidence.sourceRelationValidated = true;
        event.input.requestPolicyAdmission = true;
        event.evaluate = CompositePolicy;
        return event;
    }

    bool ExerciseCompositeWriter()
    {
        bool pass = true;
        for (bool observe : {false, true}) {
            Authority authority;
            Initialize(authority);
            auto event = CompositeInput();
            event.observeOutput = observe;
            event.diagnosticFrameId = observe ? 500 : 0;
            const auto before = authority.Read().revision;
            auto first = authority.Accept(event);
            pass &= Check(first.decision.accepted && first.decision.revision == before + 1 &&
                authority.Read().revision == before + 1,
                "composite_writer_has_one_visible_commit");
            double argument = 130.311;
            pass &= Check(first.writerInvocationId == 1 &&
                authority.PrepareWriterArgument(first, argument) &&
                std::fabs(argument - 153.931) < 0.002 && first.ArgumentPrepared(),
                "composite_invocation_identity_and_output_ignore_diagnostics");
            pass &= Check(!authority.PrepareWriterArgument(first, argument),
                "execution_permission_is_single_use");
            pass &= Check(!observe || !authority.Read().observations.at(first.observationId).HasObservation(),
                "preparation_does_not_claim_native_applied");
            unsigned prepared = 1;
            unsigned untracked{};
            for (unsigned i = 2; i <= 600; ++i) {
                auto next = authority.Accept(event);
                if (authority.PrepareWriterArgument(next, argument) &&
                    std::fabs(argument - 153.931) < 0.002) ++prepared;
                if (next.observationUnavailable) ++untracked;
            }
            pass &= Check(prepared == 600 && authority.Read().observations.size() ==
                    (observe ? Authority::MaxEffects : 0) &&
                (!observe || untracked == 600 - Authority::MaxEffects),
                "observation_capacity_never_refuses_correction");
            if (observe) {
                auto readback = Basic(EventKind::EffectObserved);
                readback.effectId = first.observationId;
                readback.transactionId = first.writerInvocationId;
                readback.dependencyRevision = first.decision.revision;
                readback.fov = argument;
                readback.fovBoundary = FovBoundary::PostStore;
                readback.fovProvenance = FovProvenance::EffectiveReadback;
                readback.effectResult = EffectResult::Applied;
                readback.evidence.effectBoundaryCorrelated = true;
                pass &= Check(authority.Accept(readback).accepted &&
                    !authority.Read().scopes.at(1).postStoreFov,
                    "old_pending_readback_survives_capacity_without_overwriting_current");
                readback.kind = EventKind::EffectClosed;
                const bool closed = authority.Accept(readback).accepted;
                auto replacementWindow = authority.Accept(event);
                pass &= Check(closed && replacementWindow.observationId != 0 &&
                    !replacementWindow.observationUnavailable &&
                    authority.PrepareWriterArgument(replacementWindow, argument),
                    "explicit_observation_close_releases_tracking_capacity_only");
            }
        }
        Authority aborted;
        Initialize(aborted);
        auto event = CompositeInput();
        event.observeOutput = true;
        {
            auto invocation = aborted.Accept(event);
            pass &= Check(invocation.observationId != 0 && !invocation.ArgumentPrepared(),
                "aborted_pre_had_only_an_unconsumed_local_permission");
        }
        pass &= Check(aborted.Read().observations.empty(),
            "aborted_pre_releases_optional_tracking_without_applied");
        auto stale = aborted.Accept(event);
        auto setting = Basic(EventKind::SettingsChanged);
        setting.settingsDomain = SettingsDomain::Global;
        setting.enabled = false;
        aborted.Accept(setting);
        double output = -1;
        // Mutating public receipt metadata must not revive the private capability.
        stale.decision.revision = aborted.Read().revision;
        pass &= Check(!aborted.PrepareWriterArgument(stale, output) && output == -1 &&
            aborted.Read().observations.empty(), "settings_supersession_revokes_execution");

        Authority recovery;
        Initialize(recovery);
        auto seed = recovery.Accept(CompositeInput());
        recovery.PrepareWriterArgument(seed, output);
        auto enter = Basic(EventKind::CinematicEnter);
        enter.generation = 4;
        enter.aspectContextCompatible = true;
        recovery.Accept(enter);
        auto exit = enter;
        exit.kind = EventKind::CinematicExit;
        exit.evidence.sourceRelationValidated = true;
        recovery.Accept(exit);
        auto endpoint = CompositeInput();
        endpoint.input.generation = 4;
        endpoint.input.evidence.recoveryEndpointProven = true;
        endpoint.input.evidence.gameplayRoleProven = true;
        const auto recoveryRevision = recovery.Read().revision;
        auto completed = recovery.Accept(endpoint);
        pass &= Check(completed.decision.revision == recoveryRevision + 1 &&
            completed.decision.correction == CorrectionOwner::Gameplay &&
            recovery.Read().scopes.at(1).recovery == RecoveryPhase::Complete &&
            recovery.Read().baseline->trust == BaselineTrust::Current &&
            recovery.PrepareWriterArgument(completed, output) && std::fabs(output - 153.931) < 0.002,
            "same_source_recovery_completion_and_gameplay_admission_share_one_writer_commit");

        auto receiverB = CompositeInput(90.3115);
        receiverB.input.scope = 2;
        receiverB.input.previousScope = 1;
        receiverB.input.receiver = 202;
        receiverB.input.requestPolicyAdmission = false;
        auto unknownB = recovery.Accept(receiverB);
        pass &= Check(unknownB.decision.correction == CorrectionOwner::PassThrough &&
            recovery.Read().baseline->trust == BaselineTrust::Invalid &&
            !recovery.PrepareWriterArgument(unknownB, output),
            "replacement_cannot_inherit_receiver_a_baseline_or_permission");
        receiverB.input.previousScope = 0;
        receiverB.input.requestPolicyAdmission = true;
        auto admittedB = recovery.Accept(receiverB);
        pass &= Check(admittedB.decision.admission == GameplayAdmission::PolicyAdmitted &&
            recovery.Read().baseline->receiver == 202 && recovery.Read().baseline->fov == 90.3115 &&
            recovery.PrepareWriterArgument(admittedB, output) && std::fabs(output - 127.119) < 0.002,
            "replacement_policy_uses_b_sample_instead_of_transferring_a_reference");

        Authority cycle;
        Initialize(cycle);
        auto gameplay = cycle.Accept(CompositeInput());
        cycle.PrepareWriterArgument(gameplay, output);
        enter.generation = 5;
        cycle.Accept(enter);
        auto cine = CompositeInput(90.0);
        cine.input.generation = 5;
        cine.input.fovProvenance = FovProvenance::PreparedCinematic;
        auto cine1 = cycle.Accept(cine);
        pass &= Check(cine1.decision.correction == CorrectionOwner::Cinematic &&
            cycle.PrepareWriterArgument(cine1, output) && std::fabs(output - 153.931) < 0.002,
            "composite_cinematic_uses_retained_gameplay_reference");
        exit = enter;
        exit.kind = EventKind::CinematicExit;
        exit.evidence.sourceRelationValidated = true;
        cycle.Accept(exit);
        auto arm = Basic(EventKind::ProviderArmed);
        arm.generation = 5;
        arm.providerGeneration = 5;
        arm.component = 202;
        arm.thread = 303;
        arm.evidence.callerValidated = true;
        const auto armed = cycle.Accept(arm);
        auto setter = arm;
        setter.kind = EventKind::ProviderSetterObserved;
        setter.armingEventId = armed.authorityEventId;
        setter.fov = 130.264;
        setter.fovBoundary = FovBoundary::DialogueSetterInput;
        setter.fovProvenance = FovProvenance::OriginalNative;
        setter.evidence.causalArmingJoinProven = true;
        cycle.Accept(setter);
        auto provider = CompositeInput(130.264);
        provider.input = setter;
        provider.input.evidence.sourceRelationValidated = true;
        auto handoff = cycle.Accept(provider);
        pass &= Check(handoff.decision.correction == CorrectionOwner::Dialogue &&
            cycle.PrepareWriterArgument(handoff, output) && std::fabs(output - 153.925) < 0.003,
            "composite_preserves_bounded_provider_handoff_numeric_mapping");
        auto transfer = CompositeInput(70.0);
        transfer.input.generation = 5;
        transfer.input.evidence.recoveryEndpointProven = true;
        transfer.input.fovProvenance = FovProvenance::OriginalNative;
        auto dialogue = cycle.Accept(transfer);
        pass &= Check(dialogue.decision.correction == CorrectionOwner::Dialogue &&
            cycle.Read().scopes.at(1).dialogue == DialoguePhase::Active &&
            cycle.PrepareWriterArgument(dialogue, output) && std::fabs(output - 149.071) < 0.003,
            "provider_endpoint_transfers_ownership_within_writer_commit");
        auto preparedDialogue = CompositeInput(122.087);
        preparedDialogue.input.generation = 5;
        preparedDialogue.input.fovProvenance = FovProvenance::PreparedDialogue;
        auto dialogueContinuation = cycle.Accept(preparedDialogue);
        pass &= Check(dialogueContinuation.decision.correction == CorrectionOwner::Dialogue &&
            cycle.PrepareWriterArgument(dialogueContinuation, output) &&
            std::fabs(output - 149.071) < 0.003,
            "prepared_dialogue_continuation_is_not_mapped_twice");
        enter.generation = 6;
        cycle.Accept(enter);
        cine.input.generation = 6;
        auto cine2 = cycle.Accept(cine);
        pass &= Check(cine2.decision.correction == CorrectionOwner::Cinematic &&
            cycle.Read().scopes.at(1).dialogue == DialoguePhase::None &&
            cycle.PrepareWriterArgument(cine2, output) && std::fabs(output - 153.931) < 0.002,
            "second_cinematic_releases_dialogue_and_retains_gameplayhorplus");

        Authority isolated;
        Initialize(isolated);
        auto isolatedEvent = CompositeInput();
        isolatedEvent.observeOutput = true;
        {
            auto original = isolated.Accept(isolatedEvent);
            auto moved = std::move(original);
            pass &= Check(isolated.PrepareWriterArgument(moved, output) &&
                !isolated.PrepareWriterArgument(original, output),
                "moved_capability_cannot_be_reused_through_original_receipt");
        }

        auto observedWriter = isolated.Accept(isolatedEvent);
        isolated.PrepareWriterArgument(observedWriter, output);
        auto correlated = Basic(EventKind::EffectObserved);
        correlated.effectId = observedWriter.observationId;
        correlated.transactionId = observedWriter.writerInvocationId;
        correlated.dependencyRevision = observedWriter.decision.revision;
        correlated.fov = output;
        correlated.fovBoundary = FovBoundary::PostStore;
        correlated.fovProvenance = FovProvenance::EffectiveReadback;
        correlated.effectResult = EffectResult::Applied;
        correlated.evidence.effectBoundaryCorrelated = true;
        pass &= Check(isolated.Accept(correlated).accepted &&
            isolated.Read().scopes.at(1).postStoreFov->transactionId == observedWriter.writerInvocationId,
            "current_invocation_publishes_only_correlated_named_readback");
        auto successor = isolated.Accept(isolatedEvent);
        pass &= Check(!isolated.Read().scopes.at(1).postStoreFov &&
            isolated.Read().observations.at(observedWriter.observationId).postStoreFov.has_value(),
            "successor_resets_current_readback_but_retains_pending_observation_history");
        auto invalidGeneration = cine;
        invalidGeneration.input.generation = 1000;
        const auto revisionBeforeInvalid = cycle.Read().revision;
        auto rejected = cycle.Accept(invalidGeneration);
        pass &= Check(!rejected.decision.accepted && cycle.Read().revision == revisionBeforeInvalid &&
            !cycle.PrepareWriterArgument(rejected, output),
            "invalid_writer_generation_has_no_partial_commit_or_permission");
        const auto revisionBeforeThrow = isolated.Read().revision;
        isolatedEvent.evaluate = +[](void*, const WriterPolicyContext&) -> std::optional<WriterProposal> {
            throw 123;
        };
        bool threw{};
        try { auto unused = isolated.Accept(isolatedEvent); } catch (int) { threw = true; }
        pass &= Check(threw && isolated.Read().revision == revisionBeforeThrow,
            "proposal_exception_does_not_publish_partial_working_state");

        Authority reentrant;
        Initialize(reentrant);
        auto reentrantEvent = CompositeInput();
        reentrantEvent.observeOutput = true;
        reentrantEvent.policyContext = &reentrant;
        reentrantEvent.evaluate = +[](void* context, const WriterPolicyContext&) -> std::optional<WriterProposal> {
            auto change = Basic(EventKind::SettingsChanged);
            change.settingsDomain = SettingsDomain::Global;
            change.enabled = false;
            static_cast<Authority*>(context)->Accept(change);
            return WriterProposal{153.931, false};
        };
        auto rejectedReentry = reentrant.Accept(reentrantEvent);
        pass &= Check(!rejectedReentry.decision.accepted &&
            !reentrant.Read().settings.globalEnabled && reentrant.Read().observations.empty(),
            "unexpected_policy_reentry_cannot_overwrite_newer_committed_state");

        Authority enableGate;
        Initialize(enableGate);
        auto original = enableGate.Accept(CompositeInput());
        auto disableGameplay = Basic(EventKind::SettingsChanged);
        disableGameplay.settingsDomain = SettingsDomain::Gameplay;
        disableGameplay.enabled = false;
        enableGate.Accept(disableGameplay);
        pass &= Check(enableGate.Read().baseline->trust == BaselineTrust::Current &&
            !enableGate.PrepareWriterArgument(original, output),
            "gameplay_disable_revokes_permission_without_retiring_native_reference");
        auto cinematicAfterDisable = Basic(EventKind::CinematicEnter);
        cinematicAfterDisable.generation = 100;
        cinematicAfterDisable.aspectContextCompatible = true;
        enableGate.Accept(cinematicAfterDisable);
        auto cinematicInput = CompositeInput(90.0);
        cinematicInput.input.generation = 100;
        auto cinematicPermission = enableGate.Accept(cinematicInput);
        pass &= Check(enableGate.PrepareWriterArgument(cinematicPermission, output) &&
            std::fabs(output - 153.931) < 0.002,
            "cinematic_before_next_writer_retains_reference_after_gameplay_disable");
        auto policyChange = Basic(EventKind::SettingsChanged);
        policyChange.settingsDomain = SettingsDomain::Gameplay;
        policyChange.settingsMutation = SettingsMutation::PolicySelection;
        policyChange.policySelection = 999;
        enableGate.Accept(policyChange);
        disableGameplay.enabled = true;
        enableGate.Accept(disableGameplay);
        pass &= Check(enableGate.Read().baseline->trust == BaselineTrust::Invalid,
            "enable_toggle_cannot_resurrect_already_invalid_reference");
        std::cout << (pass ? "PASS" : "FAIL") << ": M16 assertions=" << assertions << '\n';
        return pass;
    }

    #include "lifecycle_authority_corrective_cases.hpp"

    bool ExerciseShadowReplay()
    {
        assertions = 0;
        bool pass = ExerciseCapturedProviderShadowReplay();
        pass &= ExerciseSameSourceRecoveryCompletionBeforeNextCinematic();
        pass &= ExerciseNewSourceAndDialogueScopeLimits();
        std::cout << (pass ? "PASS" : "FAIL")
            << ": lifecycle_authority_shadow_replay assertions=" << assertions << '\n';
        return pass;
    }
}

int main(int argc, char** argv)
{
    if (argc == 2 && std::string_view(argv[1]) == "--corrective-bindings")
        return ExerciseCorrectiveBindings() ? 0 : 1;
    if (argc == 2 && std::string_view(argv[1]) == "--composite-writer")
        return ExerciseCompositeWriter() ? 0 : 1;
    if (argc == 2 && std::string_view(argv[1]) == "--hardening")
        return ExerciseHardening() ? 0 : 1;
    if (argc == 2 && std::string_view(argv[1]) == "--shadow-replay")
        return ExerciseShadowReplay() ? 0 : 1;
    bool pass = ExerciseOwnershipAndR1Projection();
    pass &= ExerciseArmingIdentity();
    pass &= ExerciseSourceAndProvenance();
    pass &= ExerciseEffectReadbackCorrelation();
    pass &= ExerciseFailureAndTerminalPaths();
    pass &= ExerciseHardening();
    pass &= ExerciseCompositeWriter();
    pass &= ExerciseCorrectiveBindings();
    std::cout << (pass ? "PASS" : "FAIL") << ": lifecycle_authority_m1_harness assertions="
        << assertions << '\n';
    return pass ? 0 : 1;
}
