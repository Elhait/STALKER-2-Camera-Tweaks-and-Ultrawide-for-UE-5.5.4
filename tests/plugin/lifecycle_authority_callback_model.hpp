#pragma once

namespace authority_transition_model
{
    // Vertices describe the facts available to the oracle. They deliberately
    // do not read Authority's state when offering the next callback.
    enum class FactStage {
        Gameplay, Entered, AspectBound, Cinematic, CinematicPrepared, Exit, EarlyEndpoint, Departed,
        ProviderArmed, ProviderPaired, ProviderBlend, ProviderEndpoint, ProviderActive,
        ProviderSteady, Candidate, CandidateStable, CandidateNearlyExpired, OrdinaryActive, OrdinaryPrepared, OrdinarySteady, BlendEntered,
        BlendArmerBound, BlendAspectBound, BlendBound, Exiting, Rearm, Returned, Disabled, Count
    };
    enum class Callback {
        Enter, Aspect, CinematicWriter, PreparedCinematicWriter, Exit, EarlyEndpoint, RecoveryWriter, GameplayWriter,
        ProviderArmer, ProviderSetter, ForeignWriter, ProviderFirstWriter, ProviderEndpoint,
        AdmitProvider, PreparedWriter, NativeEndpointWriter, DepartureWriter, ForeignSetter,
        CandidateSetter, DescentSetter, SteadySetter, BlendArmer, BlendStartWriter, BlendEndpoint,
        ExitSetter, ExitMiddleSetter, ExitWriter, RearmSetter, TerminalSetter, DisableDialogue, ForeignReceiver,
        NativeGameplayFovChange, NativeRecoveryFovChange, ChangedExitEndpoint
    };

    using F = FactStage;
    using C = Callback;
    struct CallbackEdge { F before; C callback; F after; };
    constexpr CallbackEdge callbackEdges[]{
        {F::Gameplay, C::GameplayWriter, F::Gameplay},
        {F::Gameplay, C::NativeGameplayFovChange, F::Gameplay},
        {F::Gameplay, C::Enter, F::Entered},
        {F::Entered, C::Aspect, F::AspectBound},
        {F::AspectBound, C::CinematicWriter, F::Cinematic},
        {F::AspectBound, C::ForeignReceiver, F::AspectBound},
        {F::Cinematic, C::CinematicWriter, F::Cinematic},
        {F::Cinematic, C::Exit, F::Exit},
        {F::CinematicPrepared, C::PreparedCinematicWriter, F::CinematicPrepared},
        {F::CinematicPrepared, C::Exit, F::Exit},
        {F::Exit, C::EarlyEndpoint, F::EarlyEndpoint},
        {F::Exit, C::RecoveryWriter, F::Departed},
        {F::EarlyEndpoint, C::RecoveryWriter, F::Departed},
        {F::EarlyEndpoint, C::EarlyEndpoint, F::EarlyEndpoint},
        {F::Departed, C::GameplayWriter, F::Gameplay},
        {F::Exit, C::ProviderArmer, F::ProviderArmed},
        {F::ProviderArmed, C::ProviderSetter, F::ProviderPaired},
        {F::ProviderPaired, C::ForeignWriter, F::ProviderPaired},
        {F::ProviderPaired, C::ProviderFirstWriter, F::ProviderBlend},
        {F::ProviderBlend, C::ProviderEndpoint, F::ProviderEndpoint},
        {F::ProviderEndpoint, C::AdmitProvider, F::ProviderActive},
        {F::ProviderEndpoint, C::ProviderEndpoint, F::ProviderEndpoint},
        {F::ProviderActive, C::PreparedWriter, F::ProviderSteady},
        {F::ProviderActive, C::ForeignSetter, F::ProviderActive},
        {F::ProviderSteady, C::ForeignSetter, F::ProviderSteady},
        {F::ProviderSteady, C::PreparedWriter, F::ProviderSteady},
        {F::ProviderSteady, C::NativeEndpointWriter, F::ProviderSteady},
        {F::ProviderSteady, C::DepartureWriter, F::ProviderSteady},
        {F::ProviderSteady, C::SteadySetter, F::ProviderActive},
        {F::ProviderSteady, C::ExitSetter, F::Exiting},
        {F::ProviderSteady, C::DisableDialogue, F::Disabled},
        {F::Gameplay, C::CandidateSetter, F::Candidate},
        {F::Candidate, C::CandidateSetter, F::CandidateStable},
        {F::CandidateStable, C::CandidateSetter, F::CandidateNearlyExpired},
        {F::CandidateNearlyExpired, C::CandidateSetter, F::Gameplay},
        {F::Candidate, C::DescentSetter, F::OrdinaryActive},
        {F::CandidateStable, C::DescentSetter, F::OrdinaryActive},
        {F::CandidateNearlyExpired, C::DescentSetter, F::OrdinaryActive},
        {F::OrdinaryActive, C::SteadySetter, F::OrdinaryPrepared},
        {F::OrdinaryPrepared, C::PreparedWriter, F::OrdinarySteady},
        {F::OrdinarySteady, C::PreparedWriter, F::OrdinarySteady},
        {F::OrdinarySteady, C::SteadySetter, F::OrdinaryPrepared},
        {F::OrdinarySteady, C::ExitSetter, F::Exiting},
        {F::OrdinarySteady, C::Enter, F::BlendEntered},
        {F::BlendEntered, C::Aspect, F::BlendAspectBound},
        {F::BlendEntered, C::BlendArmer, F::BlendArmerBound},
        {F::BlendArmerBound, C::Aspect, F::BlendBound},
        {F::BlendAspectBound, C::BlendArmer, F::BlendBound},
        {F::BlendBound, C::BlendStartWriter, F::BlendBound},
        {F::BlendBound, C::BlendEndpoint, F::CinematicPrepared},
        {F::Exiting, C::ExitWriter, F::Exiting},
        {F::Exiting, C::ExitMiddleSetter, F::Exiting},
        {F::Exiting, C::RearmSetter, F::Rearm},
        {F::Exiting, C::ChangedExitEndpoint, F::Returned},
        {F::Rearm, C::TerminalSetter, F::Returned},
        {F::Rearm, C::NativeRecoveryFovChange, F::Rearm},
        {F::Returned, C::GameplayWriter, F::Gameplay},
    };

    class CallbackFixture
    {
    public:
        CallbackFixture(Options options, Counts& counts, std::string label)
            : fixture_(options, counts, std::move(label)) {}

        bool Apply(const CallbackEdge& edge, unsigned variant)
        {
            auto& f = fixture_;
            const float base = f.options_.native;
            const float target = f.HorPlus(base);
            const float reduced = f.Reduced();
            constexpr float dialogueTarget = 70;
            const float exitStart = std::lerp(dialogueTarget, base, 0.16f);
            const auto writer = [&](float input, float expected, la::CorrectionOwner owner, const char* name) {
                f.Writer(input, expected, owner, name);
            };
            using Owner = la::CorrectionOwner;
            switch (edge.callback) {
            case C::Enter: {
                enteredTarget_ = 90;
                const auto result = f.authority_.CinematicBoundary(f.gate_).Enter(
                    {90, f.options_.aspect, true, 7}, enteredTarget_);
                f.phase_ = Milestone::Entered;
                f.Observe("event/enter");
                f.Expect(result.decision.accepted && std::fabs(enteredTarget_ - target) < 0.012f,
                    "ENTER maps the admitted Gameplay reference");
                break;
            }
            case C::Aspect: {
                const auto result = f.authority_.CinematicBoundary(f.gate_).Aspect(
                    {f.options_.source, f.options_.aspect, true}, f.engine_);
                f.phase_ = Milestone::Cinematic;
                f.Observe("event/aspect");
                f.Expect(result.effectSucceeded, "aspect association");
                break;
            }
            case C::CinematicWriter:
                writer(90, target, Owner::Cinematic, "event/cinematic-writer");
                break;
            case C::PreparedCinematicWriter:
                writer(target, target, Owner::Cinematic, "event/prepared-cinematic-writer");
                break;
            case C::Exit: {
                const auto result = f.authority_.CinematicBoundary(f.gate_).Exit({base, true, true});
                f.phase_ = Milestone::Recovery;
                f.Observe("event/exit");
                f.Expect(result.decision.accepted, "EXIT opens recovery");
                break;
            }
            case C::EarlyEndpoint:
                writer(base, target, Owner::Gameplay, "event/early-endpoint");
                break;
            case C::RecoveryWriter:
                writer(std::lerp(base, target, 0.25f + 0.1f * (variant % 5)), target,
                    Owner::Recovery, "event/recovery-writer");
                break;
            case C::GameplayWriter:
                f.phase_ = Milestone::Gameplay;
                writer(base, target, Owner::Gameplay, "event/gameplay-writer");
                break;
            case C::NativeGameplayFovChange: {
                const float changed = base >= 110.0f ? 90.0f : 130.0f;
                const auto receiver = f.capture_.sourceIdentity;
                f.options_.native = changed;
                f.phase_ = Milestone::Gameplay;
                writer(changed, f.HorPlus(changed), Owner::Gameplay, "event/native-gameplay-fov-change");
                const auto reference = f.authority_.Read().baseline;
                f.Expect(reference && reference->receiver == receiver &&
                    reference->trust == la::BaselineTrust::Current &&
                    std::fabs(reference->fov - changed) < 0.001f,
                    "same-receiver FOV change refreshes the reference for subsequent events");
                break;
            }
            case C::ProviderArmer: {
                f.setter_.nativeStart = base;
                const auto result = f.authority_.DialogueBoundary(f.gate_).Arm(
                    {0xDA, f.options_.source, 7, base, dialogueTarget, base, true});
                f.phase_ = Milestone::Armed;
                f.Observe("event/provider-armer");
                f.Expect(result.accepted, "provider accepted after EXIT");
                break;
            }
            case C::ProviderSetter:
                f.phase_ = Milestone::Joined;
                f.Setter(base - 0.047f, base - 0.047f, "event/provider-setter");
                break;
            case C::ForeignWriter: {
                auto foreign = f.capture_;
                ++foreign.threadId;
                foreign.incomingFov = base - 0.047f;
                float argument = foreign.incomingFov;
                const auto revision = f.authority_.Read().revision;
                const auto ignored = f.authority_.WriterBoundary(f.gate_).ProcessNative(
                    foreign, argument, f.engine_, f.options_.diagnostics);
                f.Observe("event/foreign-writer");
                f.Expect(!ignored.decision.accepted && argument == foreign.incomingFov &&
                    f.authority_.Read().revision == revision, "foreign thread cannot consume the paired setter");
                break;
            }
            case C::ProviderFirstWriter:
                writer(base - 0.047f, f.HorPlus(f.EnterSample(base - 0.047f)), Owner::Dialogue,
                    "event/provider-first-writer");
                break;
            case C::ProviderEndpoint:
                f.phase_ = Milestone::Endpoint;
                writer(dialogueTarget, f.HorPlus(reduced), Owner::Dialogue, "event/provider-endpoint");
                break;
            case C::AdmitProvider:
                f.phase_ = Milestone::Dialogue;
                f.Setter(dialogueTarget, reduced, "event/provider-admission");
                break;
            case C::PreparedWriter:
                writer(reduced, f.HorPlus(reduced),
                    edge.after == F::ProviderSteady ? Owner::Dialogue : Owner::Gameplay,
                    "event/prepared-writer");
                break;
            case C::NativeEndpointWriter:
                writer(dialogueTarget, f.HorPlus(reduced), Owner::Dialogue, "event/native-dialogue-endpoint");
                break;
            case C::DepartureWriter: {
                // Deterministic samples span the return direction. These are
                // Writer captures with no new paired setter, not a provenance tag
                // injected into Authority or a replay of a scene's FOV records.
                const float raw = std::lerp(reduced, base, 0.08f + 0.15f * (variant % 5));
                writer(raw, f.HorPlus(raw), Owner::Dialogue, "event/departing-writer");
                break;
            }
            case C::ForeignSetter:
                f.ForeignProviderSetter();
                break;
            case C::CandidateSetter:
                f.phase_ = edge.after == F::Gameplay ? Milestone::Gameplay : Milestone::Candidate;
                f.Setter(base, base, "event/candidate-setter");
                break;
            case C::DescentSetter: {
                f.phase_ = Milestone::Dialogue;
                const float raw = std::lerp(base, dialogueTarget, 0.55f);
                f.Setter(raw, f.EnterSample(raw), "event/descent-setter");
                break;
            }
            case C::SteadySetter:
                f.Setter(dialogueTarget, reduced, "event/steady-setter");
                break;
            case C::BlendArmer: {
                const auto result = f.authority_.CinematicBoundary(f.gate_).Armer(
                    {0xCA, f.options_.source, 7, reduced, enteredTarget_, reduced,
                     f.options_.aspect, f.options_.aspect, 4, true, true});
                f.phase_ = edge.after == F::BlendArmerBound ? Milestone::ArmerBound : Milestone::Cinematic;
                f.Observe("event/blend-armer");
                f.Expect(result.decision.accepted, "blend activates on confirmed source");
                break;
            }
            case C::BlendStartWriter: {
                const float progress = 0.125f * (variant % 5);
                writer(std::lerp(reduced, target, progress), std::lerp(f.HorPlus(reduced), target, progress),
                    Owner::Cinematic, "event/blend-start-writer");
                break;
            }
            case C::BlendEndpoint:
                writer(target, target, Owner::Cinematic, "event/blend-endpoint");
                break;
            case C::ExitSetter:
                f.phase_ = Milestone::Exiting;
                exitArgument_ = reduced;
                f.Setter(exitStart, exitArgument_, "event/dialogue-exit-setter");
                break;
            case C::ExitMiddleSetter: {
                const float sample = std::lerp(exitStart, base, 0.4f);
                const double progress = (Projection(sample) - Projection(exitStart)) /
                    (Projection(base) - Projection(exitStart));
                exitArgument_ = Degrees(std::lerp(Projection(reduced), Projection(base), progress));
                f.Setter(sample, exitArgument_, "event/dialogue-exit-middle-setter");
                break;
            }
            case C::ExitWriter:
                writer(exitArgument_, f.HorPlus(exitArgument_), Owner::Gameplay, "event/dialogue-exit-writer");
                break;
            case C::RearmSetter:
                f.phase_ = Milestone::Rearm;
                f.Setter(base, base, "event/rearm-setter");
                break;
            case C::NativeRecoveryFovChange:
                // The next setter reports the new endpoint. Changing the
                // setting supplies no callback or lifecycle proof by itself.
                f.options_.native = base >= 110.0f ? 90.0f : 130.0f;
                f.Observe("event/native-recovery-fov-change");
                break;
            case C::ChangedExitEndpoint: {
                // The modeled terminal sample is on the return trajectory.
                // After an intermediate setter, a fixed lower value could
                // instead describe a new descent and would not prove a return.
                const float changed = base >= 110.0f
                    ? std::lerp(f.setter_.incomingFov, base, 0.5f) : 145.0f;
                f.options_.native = changed;
                f.setter_.nativeTarget = changed;
                f.phase_ = Milestone::Returned;
                f.Setter(changed, changed, "event/changed-exit-endpoint");
                f.setter_.nativeTarget = dialogueTarget;
                f.Expect(f.authority_.Read().scopes.at(f.options_.source).dialogue ==
                    la::DialoguePhase::None, "verified changed endpoint completes Dialogue on the same receiver");
                break;
            }
            case C::TerminalSetter:
                f.setter_.nativeTarget = base;
                f.phase_ = Milestone::Returned;
                f.Setter(base, base, "event/terminal-setter");
                f.setter_.nativeTarget = dialogueTarget;
                break;
            case C::ForeignReceiver: {
                auto foreign = f.capture_;
                ++foreign.sourceIdentity;
                foreign.incomingFov = base;
                float argument = base;
                const auto before = f.authority_.Read().baseline;
                const auto result = f.authority_.WriterBoundary(f.gate_).ProcessNative(
                    foreign, argument, f.engine_, f.options_.diagnostics);
                f.Observe("event/foreign-receiver");
                const auto after = f.authority_.Read().baseline;
                f.Expect(result.decision.correction == Owner::PassThrough && !result.argumentPrepared &&
                    argument == base && before && after && before->source == after->source &&
                    before->observationSequence == after->observationSequence,
                    "foreign receiver cannot use the Cinematic effect or replace the current baseline");
                break;
            }
            case C::DisableDialogue: {
                const auto before = f.authority_.Read().settings.dialogueEpoch;
                const auto result = f.authority_.ApplySetting(f.gate_,
                    {plugin::RuntimeSettingKind::DialogueEnabled, false}, true, true);
                f.Observe("event/disable-dialogue");
                const auto state = f.authority_.Read();
                f.Expect(result.mutation.changed && state.settings.dialogueEpoch == before + 1 &&
                    !state.scopes.at(f.options_.source).dialogueCorrectionArmed,
                    "settings revoke the current provider correction");
                break;
            }
            }
            return f.pass_;
        }

        bool Finish() { return fixture_.Finish(); }

    private:
        Sequence fixture_;
        float enteredTarget_{}, exitArgument_{};
    };

    using CallbackPath = std::vector<const CallbackEdge*>;

    std::vector<const CallbackEdge*> Outgoing(F stage)
    {
        std::vector<const CallbackEdge*> result;
        for (const auto& edge : callbackEdges)
            if (edge.before == stage) result.push_back(&edge);
        return result;
    }

    bool ReplayGenerated(const CallbackPath& path, Options options, Counts& counts,
        const std::string& label, unsigned variant, size_t suffixOffset = 0)
    {
        CallbackFixture fixture(options, counts, label);
        for (size_t i = 0; i < path.size(); ++i) {
            const size_t localStep = i < suffixOffset ? i : i - suffixOffset;
            if (!fixture.Apply(*path[i], variant + static_cast<unsigned>(localStep))) return false;
        }
        return fixture.Finish();
    }

    bool RunGenerated()
    {
        Counts counts;
        constexpr unsigned vertexCount = static_cast<unsigned>(F::Count);
        std::array<std::optional<CallbackPath>, vertexCount> prefixes;
        prefixes[0].emplace();
        std::vector<F> queue{F::Gameplay};
        for (size_t next = 0; next < queue.size(); ++next) {
            const auto stage = queue[next];
            for (const auto* edge : Outgoing(stage)) {
                auto& prefix = prefixes[static_cast<unsigned>(edge->after)];
                if (prefix) continue;
                prefix = prefixes[static_cast<unsigned>(stage)];
                prefix->push_back(edge);
                queue.push_back(edge->after);
            }
        }
        if (!Check(queue.size() == vertexCount, "native callback model has no unreachable states")) return false;
        std::vector<CallbackPath> shortPaths;
        for (const auto& edge : callbackEdges) {
            auto path = *prefixes[static_cast<unsigned>(edge.before)];
            path.push_back(&edge);
            shortPaths.push_back(path);
            for (const auto* next : Outgoing(edge.after)) {
                path.push_back(next);
                shortPaths.push_back(path);
                path.pop_back();
            }
        }
        CallbackPath history;
        for (const auto stage : {F::Departed, F::Returned}) {
            const auto cycle = *prefixes[static_cast<unsigned>(stage)];
            const auto returns = Outgoing(stage);
            if (!Check(returns.size() == 1 && returns.front()->after == F::Gameplay,
                "history leaves a settled Gameplay context")) return false;
            for (unsigned repetition = 0; repetition < 3; ++repetition) {
                history.insert(history.end(), cycle.begin(), cycle.end());
                history.push_back(returns.front());
            }
        }
        constexpr std::uint32_t eventSeeds[]{0xD1A10u, 0xA2C6u, 0x7372u, 0x59F2u, 0x91A7u, 0x75E9u};
        for (const bool diagnosticsOn : {false, true})
            for (const auto gameplay : {config::GameplayMode::HorPlus, config::GameplayMode::RuntimeCombine})
                for (const float aspect : {21.0f / 9.0f, 32.0f / 9.0f}) {
                    Options options{diagnosticsOn};
                    options.gameplayMode = gameplay;
                    options.aspect = aspect;
                    options.omitReadback = true;
                    const std::string profile = (diagnosticsOn ? "/ON/" : "/OFF/") +
                        std::to_string(static_cast<unsigned>(gameplay)) + "/" + std::to_string(aspect) + "/";
                    for (size_t i = 0; i < shortPaths.size(); ++i) {
                        const auto variant = static_cast<unsigned>(i);
                        const auto label = "generated" + profile + std::to_string(i);
                        if (!ReplayGenerated(shortPaths[i], options, counts, label, variant)) return false;
                        auto warmedPath = history;
                        warmedPath.insert(warmedPath.end(), shortPaths[i].begin(), shortPaths[i].end());
                        // Reset the input variant at the suffix so this runs
                        // the same numerical samples against earlier history.
                        if (!ReplayGenerated(warmedPath, options, counts, label + "/history",
                            variant, history.size())) return false;
                    }
                    for (std::uint32_t seed : eventSeeds) {
                        ++counts.seeds;
                        std::uint32_t random = seed;
                        CallbackPath path;
                        auto stage = F::Gameplay;
                        for (unsigned step = 0; step < 96; ++step) {
                            const auto choices = Outgoing(stage);
                            if (choices.empty()) break;
                            random ^= random << 13;
                            random ^= random >> 17;
                            random ^= random << 5;
                            const auto* edge = choices[random % choices.size()];
                            path.push_back(edge);
                            stage = edge->after;
                        }
                        if (!ReplayGenerated(path, options, counts, "generated-seed" + profile +
                            std::to_string(seed), seed)) return false;
                    }
                }
        diagnostics::SetEnabled(false);
        std::cout << "M3 native callbacks: " << queue.size() << " states, " << std::size(callbackEdges)
            << " edges, " << shortPaths.size() << " short paths per profile, " << counts.sequences
            << " sequences, " << counts.events << " checkpoints, " << counts.frames
            << " frames, " << counts.seeds << " seeded runs\n";
        return true;
    }
}
