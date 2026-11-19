#pragma once

namespace authority_transition_model
{
    namespace la = plugin::lifecycle_authority;
    enum class Episode { Gameplay, Cinematic, Dialogue };
    enum class Milestone { Boot, Gameplay, Entered, ArmerBound, Cinematic, Recovery, Armed, Joined, Endpoint,
        Candidate, Dialogue, Exiting, Rearm, Returned, Stopped };
    constexpr Episode episodes[]{Episode::Gameplay, Episode::Cinematic, Episode::Dialogue};

    // The rows describe observable boundaries, not reducer states or guards.
    constexpr Milestone edges[][2]{
        {Milestone::Boot, Milestone::Gameplay}, {Milestone::Gameplay, Milestone::Gameplay},
        {Milestone::Gameplay, Milestone::Entered}, {Milestone::Entered, Milestone::Cinematic},
        {Milestone::Entered, Milestone::ArmerBound}, {Milestone::ArmerBound, Milestone::Cinematic},
        {Milestone::Cinematic, Milestone::Cinematic}, {Milestone::Cinematic, Milestone::Recovery},
        {Milestone::Recovery, Milestone::Recovery}, {Milestone::Recovery, Milestone::Gameplay},
        {Milestone::Recovery, Milestone::Armed}, {Milestone::Armed, Milestone::Entered},
        {Milestone::Armed, Milestone::Joined},
        {Milestone::Joined, Milestone::Endpoint}, {Milestone::Endpoint, Milestone::Dialogue},
        {Milestone::Gameplay, Milestone::Candidate}, {Milestone::Candidate, Milestone::Candidate},
        {Milestone::Candidate, Milestone::Dialogue}, {Milestone::Dialogue, Milestone::Dialogue},
        {Milestone::Dialogue, Milestone::Entered}, {Milestone::Dialogue, Milestone::Exiting},
        {Milestone::Exiting, Milestone::Exiting}, {Milestone::Exiting, Milestone::Rearm},
        {Milestone::Rearm, Milestone::Returned}, {Milestone::Returned, Milestone::Gameplay},
    };

    struct Counts
    {
        unsigned sequences{}, events{}, frames{}, seeds{}, pressureFrames{};
        std::uint64_t states{}, transitions{}, episodeEdges{};
    };

    double Projection(double degrees) { return std::tan(degrees * 3.14159265358979323846 / 360.0); }
    float Degrees(double projection)
    {
        return static_cast<float>(std::atan(projection) * 360.0 / 3.14159265358979323846);
    }

    enum class Revocation { Gameplay, Cinematic, Dialogue };
    struct Interrupted { bool passed; };

    struct Options
    {
        bool diagnostics{}, endpointFirst{}, omitReadback{};
        unsigned armerOrder{};
        std::optional<Milestone> stopAt;
        Revocation revoke{Revocation::Dialogue};
        float native{130.311f}, aspect{32.0f / 9.0f};
        std::uintptr_t source{0xB};
        config::GameplayMode gameplayMode{config::GameplayMode::HorPlus};
    };

    class Sequence
    {
    public:
        Sequence(Options options, Counts& counts, std::string label)
            : options_(options), counts_(counts), label_(std::move(label)), lock_(gate_.Acquire()),
              engine_{&memory_, ReadRuntimeCombineMemory, WriteRuntimeCombineAspect,
                  WriteRuntimeCombineAspectAndFlags}
        {
            ++counts_.sequences;
            diagnostics::SetEnabled(options_.diagnostics);
            config::FeatureConfig config;
            config.gameplayEnabled = config.cinematicsEnabled = config.dialogueEnabled = true;
            config.gameplayMode = options_.gameplayMode;
            config.cinematicFovMode = config::CinematicFovMode::GameplayHorPlus;
            config.dialogueZoomPolicy = config::DialogueZoomPolicy::Reduced;
            memory_.source = options_.source;
            memory_.aspect = options_.aspect;
            capture_.sourceIdentity = options_.source;
            capture_.outputObjectIdentity = 0x701;
            capture_.threadId = 7;
            capture_.aspect = capture_.currentViewportAspect = options_.aspect;
            capture_.aspectValid = capture_.currentViewportAspectValid = capture_.flagsValid = true;
            capture_.flags = 4;
            setter_ = {0x119, 0xDA, options_.source, 7, options_.native, options_.native, 70, true, true};
            Expect(authority_.Initialize(gate_, config), "initialize");
            Observe("initialize");
            Expect(!plugin::CanEnterCameraCallback(gate_), "same-thread reentrant native entry is blocked");
            Gameplay();
        }

        bool Follow(Episode next)
        {
            if (!pass_) return false;
            counts_.episodeEdges |= std::uint64_t{1} <<
                (3 * static_cast<unsigned>(episode_) + static_cast<unsigned>(next));
            episode_ = next;
            switch (next) {
            case Episode::Gameplay:
                if (phase_ == Milestone::Cinematic) { ExitCinematic(); SettleRecovery(); }
                else if (phase_ == Milestone::Dialogue) ExitDialogue();
                else Gameplay();
                break;
            case Episode::Cinematic:
                if (phase_ == Milestone::Cinematic) { ExitCinematic(); SettleRecovery(); }
                EnterCinematic();
                break;
            case Episode::Dialogue:
                if (phase_ == Milestone::Cinematic) { ExitCinematic(); Provider(); }
                else if (phase_ == Milestone::Dialogue) DialogueFrame();
                else EnterDialogue();
                break;
            }
            return pass_;
        }

        bool Interrupt(bool revokeDialogue)
        {
            if (!pass_) return false;
            if (revokeDialogue) {
                const auto epoch = authority_.Read().settings.dialogueEpoch;
                const plugin::RuntimeSettingMutation disable{plugin::RuntimeSettingKind::DialogueEnabled, false};
                const auto settings = authority_.ApplySetting(gate_, disable, true, true);
                Observe("disable-dialogue");
                Expect(settings.mutation.changed && authority_.Read().settings.dialogueEpoch == epoch + 1,
                    "Dialogue enable change revokes its old epoch");
                const auto& state = authority_.Read();
                const auto scope = state.scopes.find(options_.source);
                Expect(scope == state.scopes.end() ||
                    (!scope->second.dialogueCorrectionArmed && !scope->second.preparedDialogueFov),
                    "disable revokes Dialogue execution and prepared witness");
                if (lastReadback_) {
                    const auto before = authority_.Read();
                    auto port = authority_.WriterBoundary(gate_);
                    Expect(port.Observe(*lastReadback_), "correlated late readback remains historical");
                    const auto after = authority_.Read();
                    const auto observed = after.scopes.find(lastReadback_->scope);
                    Expect((observed == after.scopes.end() || !observed->second.postStoreFov) &&
                        before.cinematicGeneration == after.cinematicGeneration &&
                        before.settings.globalEpoch == after.settings.globalEpoch &&
                        before.selectedCinematicScope == after.selectedCinematicScope &&
                        before.armings.size() == after.armings.size(), "late readback cannot publish current FOV or change ownership");
                    Observe("historical-readback");
                    Expect(port.Close(*lastReadback_), "historical observation can be closed");
                    Observe("close-historical-readback");
                }
            }
            return Finish();
        }

        bool ReplayProviderDeparture()
        {
            if (!Follow(Episode::Cinematic) || !Follow(Episode::Dialogue)) return false;
            // A completed, same-source provider may still expose its native
            // endpoint. That frame still needs Dialogue mapping.
            Writer(70.0f, HorPlus(Reduced()), la::CorrectionOwner::Dialogue, "provider-steady-raw-endpoint");
            DialogueFrame();

            // The writer starts climbing out of the settled, prepared FOV
            // without another correlated Dialogue setter. Ownership alone
            // does not authorize forward Dialogue mapping of these inputs.
            for (const float raw : {122.090f, 122.117f, 123.71f, 129.0f})
                Writer(raw, HorPlus(raw), la::CorrectionOwner::Dialogue, "writer-only-provider-departure");
            const auto baseline = authority_.Read().baseline;
            Expect(baseline && baseline->trust == la::BaselineTrust::Retained &&
                std::fabs(baseline->fov - options_.native) < 0.005,
                "unclassified departure cannot replace the Gameplay reference");
            return Finish();
        }

        bool EndDuringRecovery()
        {
            if (!Follow(Episode::Cinematic)) return false;
            ExitCinematic();
            return Interrupt(true);
        }

        bool InterruptProviderWithCinematic()
        {
            if (!Follow(Episode::Cinematic)) return false;
            ExitCinematic();
            auto dialogue = authority_.DialogueBoundary(gate_);
            const la::ProviderArmEvent arm{0xDA, options_.source, 7, options_.native, 70, options_.native, true};
            Expect(dialogue.Arm(arm).accepted, "arm during source-bound recovery");
            Advance(Milestone::Armed);
            Observe("provider-before-reentry");
            EnterCinematic();
            const auto revision = authority_.Read().revision;
            Expect(!dialogue.Arm(arm).accepted && authority_.Read().revision == revision &&
                authority_.Read().armings.empty(), "new cinematic revokes preceding provider claim");
            Observe("late-provider-after-reentry");
            return Interrupt(true);
        }

        bool ReplaceReceiverAtNativeAspect(bool confirmsAspect)
        {
            const auto old = options_.source;
            options_.source = capture_.sourceIdentity = memory_.source = 0xC;
            memory_.aspect = capture_.aspect = 16.0f / 9.0f;
            memory_.writeActuallyApplies = confirmsAspect;
            capture_.incomingFov = 120.0f;
            float argument = capture_.incomingFov;
            const auto writes = memory_.aspectWrites;
            const auto result = authority_.WriterBoundary(gate_).ProcessNative(
                capture_, argument, engine_, options_.diagnostics);
            trace_.emplace_back("native-aspect-replacement-writer");
            ++counts_.events;
            ++counts_.frames;
            const auto baseline = authority_.Read().baseline;
            const bool combines = options_.gameplayMode == config::GameplayMode::RuntimeCombine;
            Expect(result.decision.accepted && result.argumentPrepared == (combines && confirmsAspect) &&
                std::fabs(argument - (combines && confirmsAspect ? HorPlus(120.0f) : 120.0f)) < 0.012f,
                "replacement writer applies RuntimeCombine only after confirmed aspect");
            Expect(memory_.aspectWrites == writes + (combines ? 1 : 0) &&
                baseline && baseline->receiver == old && baseline->trust == la::BaselineTrust::Invalid,
                "replacement correction does not admit Gameplay baseline or change HorPlus aspect");
            if (!pass_ || !combines || !confirmsAspect) return Finish();

            options_.native = 120.0f;
            capture_.aspect = memory_.aspect;
            Gameplay();
            Expect(memory_.aspectWrites == writes + 1, "next writer keeps the already synchronized aspect");
            return Finish();
        }

        bool ReplaceReceiver(std::uintptr_t next)
        {
            const auto old = options_.source;
            options_.source = capture_.sourceIdentity = setter_.receiver = memory_.source = next;
            capture_.incomingFov = options_.native;
            float argument = options_.native;
            const auto result = authority_.WriterBoundary(gate_).ProcessNative(
                capture_, argument, engine_, false);
            const auto state = authority_.Read();
            ++counts_.events;
            ++counts_.frames;
            trace_.emplace_back("new-receiver-first-writer");
            Expect(result.decision.accepted && result.argumentPrepared &&
                std::fabs(argument - HorPlus(options_.native)) < 0.012f &&
                (!state.baseline || (state.baseline->receiver == old &&
                    state.baseline->trust == la::BaselineTrust::Invalid)),
                "new receiver may correct without transferring the old baseline");
            if (!pass_) return false;
            Gameplay();
            return pass_;
        }
        bool Finish()
        {
            if (!pass_) return false;
            Expect(authority_.Shutdown(gate_), "shutdown accepted");
            phase_ = Milestone::Stopped;
            Observe("shutdown");
            const auto revision = authority_.Read().revision;
            float raw = capture_.incomingFov = 90;
            const auto rejected = authority_.WriterBoundary(gate_).ProcessNative(capture_, raw, engine_, options_.diagnostics);
            Expect(!rejected.decision.accepted && raw == 90 && authority_.Read().revision == revision,
                "writer after shutdown leaves argument and revision unchanged");
            Observe("late writer after shutdown");
            return pass_;
        }

    private:
        friend class CallbackFixture;
        void Expect(bool condition, const char* rule)
        {
            if (condition || !pass_) return;
            pass_ = false;
            std::cerr << "M3 counterexample " << label_ << " event=" << counts_.events
                << " rule=" << rule << "\n  prefix:";
            for (const auto& step : trace_) std::cerr << ' ' << step;
            std::cerr << '\n';
        }

        void Advance(Milestone next)
        {
            const auto edge = std::find_if(std::begin(edges), std::end(edges),
                [=, this](const auto& pair) { return pair[0] == phase_ && pair[1] == next; });
            Expect(edge != std::end(edges), "reference transition declared");
            if (edge != std::end(edges)) counts_.transitions |= std::uint64_t{1} << (edge - std::begin(edges));
            phase_ = next;
        }

        void Observe(const char* event)
        {
            counts_.states |= std::uint64_t{1} << static_cast<unsigned>(phase_);
            trace_.emplace_back(event);
            ++counts_.events;
            const auto state = authority_.Read();
            const auto scope = state.scopes.find(options_.source);
            Expect(state.scopes.size() <= la::Authority::MaxScopes &&
                state.armings.size() <= la::Authority::MaxArmings &&
                state.effects.size() <= la::Authority::MaxEffects &&
                state.observations.size() <= la::Authority::MaxEffects, "bounded storage");
            if (phase_ == Milestone::Stopped) {
                Expect(state.stopping && state.scopes.empty() && state.armings.empty() &&
                    state.effects.empty() && state.observations.empty() && !state.baseline,
                    "terminal claims removed");
                return;
            }
            const bool cinematic = phase_ == Milestone::Entered || phase_ == Milestone::ArmerBound ||
                phase_ == Milestone::Cinematic;
            Expect(state.cinematicSelectionActive == cinematic, "selection agrees with episode");
            if (phase_ == Milestone::Entered)
                Expect(state.pendingCinematic && !state.pendingCinematic->receiver,
                    "unbound activation has no receiver authority");
            if (phase_ == Milestone::Cinematic || phase_ == Milestone::ArmerBound)
                Expect(scope != state.scopes.end() && scope->second.cinematicActive &&
                    state.selectedCinematicScope == options_.source, "aspect-bound cinematic source");
            if (phase_ == Milestone::Candidate)
                Expect(scope != state.scopes.end() && scope->second.dialogue == la::DialoguePhase::Candidate,
                    "candidate holds no active Dialogue claim");
            if (phase_ == Milestone::Armed || phase_ == Milestone::Joined)
                Expect(scope != state.scopes.end() && scope->second.nativeProvider &&
                    scope->second.recovery == la::RecoveryPhase::Pending, "bounded provider claim during recovery");
            if (phase_ == Milestone::Dialogue)
                Expect(scope != state.scopes.end() && scope->second.dialogue == la::DialoguePhase::Active,
                    "proved dialogue activation");
            if (phase_ == Milestone::Exiting || phase_ == Milestone::Rearm)
                Expect(scope != state.scopes.end() && scope->second.dialogue ==
                    (phase_ == Milestone::Exiting ? la::DialoguePhase::Exiting : la::DialoguePhase::RearmPending),
                    "dialogue exit stage");
            if (phase_ == Milestone::Returned)
                Expect(scope != state.scopes.end() && scope->second.dialogue == la::DialoguePhase::None &&
                    !scope->second.nativeProvider, "terminal setter retires dialogue claims");
            if (phase_ == Milestone::Gameplay)
                Expect(state.baseline && state.baseline->source == options_.source &&
                    state.baseline->trust == la::BaselineTrust::Current &&
                    std::fabs(state.baseline->fov - options_.native) < 0.005,
                    "current baseline belongs to the gameplay receiver");
            const auto lifecycle = authority_.ReadLifecycle(gate_);
            Expect(lifecycle.generation == state.cinematicGeneration &&
                lifecycle.gameplayEpoch == state.settings.gameplayEpoch &&
                lifecycle.cinematicEpoch == state.settings.cinematicEpoch &&
                lifecycle.dialogueEpoch == state.settings.dialogueEpoch, "publication epochs match committed state");
            plugin::OverlaySemanticSnapshot overlay;
            Expect(authority_.ReadOverlayLifecycle(gate_, overlay), "committed overlay readable");
            Expect((overlay.coordinator.value == plugin::OverlayCoordinatorState::CinematicActive) == cinematic,
                "overlay and committed cinematic owner agree");
            if (pass_ && options_.stopAt == phase_) InterruptAtCheckpoint(state);
        }

        void InterruptAtCheckpoint(const la::Projection& state)
        {
            using Kind = plugin::RuntimeSettingKind;
            const auto domain = options_.revoke;
            options_.stopAt.reset();
            if (domain == Revocation::Dialogue)
                throw Interrupted{Interrupt(true)};

            const auto kind = domain == Revocation::Gameplay ? Kind::GameplayEnabled : Kind::CinematicsEnabled;
            const auto mutation = authority_.ApplySetting(gate_, {kind, false}, true, true);
            const auto disabled = authority_.Read();
            Expect(mutation.mutation.changed &&
                disabled.settings.globalEpoch == state.settings.globalEpoch + 1 &&
                disabled.cinematicGeneration == state.cinematicGeneration &&
                disabled.cinematicSelectionActive == state.cinematicSelectionActive,
                "settings revoke execution without inventing a generation or endpoint");
            if (domain == Revocation::Gameplay)
                Expect(!disabled.settings.gameplayEnabled &&
                    disabled.settings.gameplayEpoch == state.settings.gameplayEpoch + 1,
                    "Gameplay disable revokes its epoch");
            else {
                Expect(!disabled.settings.cinematicEnabled &&
                    disabled.settings.cinematicEpoch == state.settings.cinematicEpoch + 1,
                    "Cinematic disable revokes its epoch");
                const auto selected = disabled.scopes.find(disabled.selectedCinematicScope);
                Expect((!disabled.pendingCinematic || !disabled.pendingCinematic->cinematicCorrectionArmed) &&
                    (selected == disabled.scopes.end() || !selected->second.cinematicCorrectionArmed),
                    "Cinematic disable disarms pending and associated corrections");
            }
            Observe("settings-revoke");
            throw Interrupted{Finish()};
        }

        float HorPlus(float raw) const
        {
            return Degrees(Projection(raw) * options_.aspect / (16.0 / 9.0));
        }

        float Reduced() const
        {
            return Degrees(Projection(options_.native) * std::sqrt(Projection(70)));
        }

        float EnterSample(float raw) const
        {
            const auto start = Projection(options_.native);
            const auto progress = (Projection(raw) - start) / (Projection(70) - start);
            return Degrees(std::lerp(start, Projection(Reduced()), progress));
        }

        void Writer(float raw, float expected, la::CorrectionOwner owner, const char* event)
        {
            capture_.incomingFov = raw;
            float argument = raw;
            auto port = authority_.WriterBoundary(gate_);
            const auto result = port.ProcessNative(capture_, argument, engine_, options_.diagnostics);
            Observe(event);
            ++counts_.frames;
            if (pass_ && (std::fabs(argument - expected) >= 0.012f || result.decision.correction != owner))
                std::cerr << "M3 writer " << event << " raw=" << raw << " expected=" << expected <<
                    " actual=" << argument << " expected owner=" << static_cast<int>(owner) <<
                    " actual owner=" << static_cast<int>(result.decision.correction) << '\n';
            Expect(result.decision.accepted && result.decision.correction == owner &&
                std::fabs(argument - expected) < 0.012f, "writer frame output and authority");
            const auto publication = authority_.ReadCameraEvidence(gate_);
            Expect(publication && publication->writerTransaction.id == result.writerInvocationId,
                "publication correlated to the current writer");
            if (result.observationUnavailable) ++counts_.pressureFrames;
            if (result.observationId && publication) {
                const auto& identity = publication->writerTransaction;
                const la::WriterReadback readback{result.observationId, result.writerInvocationId,
                    options_.source, identity.receiver, identity.output, identity.thread, identity.lifecycle,
                    result.decision.revision, la::FovBoundary::PostStore, argument};
                if (options_.omitReadback) {
                    lastReadback_ = readback;
                    return;
                }
                const auto revision = authority_.Read().revision;
                auto foreign = readback;
                ++foreign.receiver;
                Expect(!port.Observe(foreign) && authority_.Read().revision == revision,
                    "foreign readback cannot mutate committed state");
                Observe("foreign-readback");
                Expect(port.Observe(readback) && !port.Observe(readback), "post-store readback is unique");
                Observe("post-store");
                auto helper = readback;
                helper.boundary = la::FovBoundary::PostHelper;
                Expect(port.Observe(helper) && !port.Observe(helper), "post-helper readback is independently unique");
                Observe("post-helper");
                Expect(port.Close(readback) && !port.Observe(readback), "closed window cannot be reopened");
                Observe("readback-close");
            }
        }

        float Setter(float raw, float expected, const char* event)
        {
            setter_.incomingFov = raw;
            float argument = raw;
            const auto result = authority_.DialogueBoundary(gate_).ProcessNative(setter_, argument);
            Observe(event);
            ++counts_.frames;
            if (pass_ && std::fabs(argument - expected) >= 0.012f)
                std::cerr << "M3 setter " << event << " raw=" << raw << " expected=" << expected
                    << " actual=" << argument << '\n';
            Expect(result.decision.accepted && std::fabs(argument - expected) < 0.012f,
                "setter frame output");
            return argument;
        }

        void Gameplay()
        {
            Advance(Milestone::Gameplay);
            Writer(options_.native, HorPlus(options_.native), la::CorrectionOwner::Gameplay, "gameplay-writer");
        }

        void EnterCinematic()
        {
            auto port = authority_.CinematicBoundary(gate_);
            const bool animateFromDialogue = phase_ == Milestone::Dialogue && options_.armerOrder != 0;
            const float authored = 90;
            float argument = authored;
            const auto enter = port.Enter({authored, options_.aspect, true, 7}, argument);
            Advance(Milestone::Entered);
            Observe("cinematic-enter");
            Expect(enter.decision.accepted, "cinematic enter accepted");
            const auto revision = authority_.Read().revision;
            float duplicateArgument = authored;
            Expect(!port.Enter({authored, options_.aspect, true, 7}, duplicateArgument).decision.accepted &&
                duplicateArgument == authored && authority_.Read().revision == revision,
                "duplicate enter does not replace active generation");
            Observe("duplicate-enter");
            const float target = HorPlus(options_.native);
            const auto arm = [&] {
                const la::NativeCinematicArmer armer{0xCA, options_.source, 7, lastDialogueInput_,
                    argument, lastDialogueInput_, options_.aspect, options_.aspect, 4, true, true};
                auto wrongThread = armer;
                ++wrongThread.thread;
                const auto revision = authority_.Read().revision;
                Expect(!port.Armer(wrongThread).decision.accepted &&
                    authority_.Read().revision == revision, "armer must match the ENTER thread");
                Observe("foreign-cinematic-armer");
                Expect(port.Armer(armer).decision.accepted, "armer binds the native blend");
                Advance(phase_ == Milestone::Entered ? Milestone::ArmerBound : Milestone::Cinematic);
                Observe("cinematic-armer");
            };
            if (animateFromDialogue && options_.armerOrder == 1) arm();
            const auto beforeStore = memory_.aspectWrites + memory_.aspectFlagsWrites;
            const auto store = port.Aspect({options_.source, options_.aspect, true}, engine_);
            Advance(Milestone::Cinematic);
            Observe("cinematic-aspect");
            Expect(store.effectSucceeded && memory_.aspectWrites + memory_.aspectFlagsWrites == beforeStore + 1,
                "cinematic aspect store executes once");
            if (animateFromDialogue) {
                if (options_.armerOrder == 2) arm();
                for (const float weight : {0.0f, 0.25f, 0.5f, 0.75f, 1.0f})
                    Writer(std::lerp(lastDialogueInput_, target, weight),
                        std::lerp(HorPlus(lastDialogueInput_), target, weight),
                        la::CorrectionOwner::Cinematic, "cinematic-blend-frame");
            } else {
                Writer(authored, target, la::CorrectionOwner::Cinematic, "cinematic-writer");
            }
        }

        void ExitCinematic()
        {
            auto port = authority_.CinematicBoundary(gate_);
            Expect(port.Exit({options_.native, true, true}).decision.accepted, "cinematic exit accepted");
            Advance(Milestone::Recovery);
            Observe("cinematic-exit");
            const auto revision = authority_.Read().revision;
            Expect(!port.Exit({options_.native, true, true}).decision.accepted &&
                authority_.Read().revision == revision, "late exit cannot restart recovery");
            Observe("duplicate-exit");
        }

        void SettleRecovery();
        void ExitDialogue();
        void Provider();
        void EnterDialogue();
        void DialogueFrame();
        void ForeignProviderSetter();

        Options options_;
        Counts& counts_;
        std::string label_;
        bool pass_{true}, providerDialogue_{};
        Milestone phase_{Milestone::Boot};
        Episode episode_{Episode::Gameplay};
        std::vector<std::string> trace_;
        la::Authority authority_;
        plugin::CameraCorrectionGate gate_;
        decltype(gate_.Acquire()) lock_;
        RuntimeCombineMemory memory_;
        plugin::CameraWriterEngineOperations engine_;
        plugin::CameraWriterInput capture_;
        la::NativeDialogueSetter setter_;
        float lastDialogueInput_{};
        std::optional<la::WriterReadback> lastReadback_;
    };

    void Sequence::SettleRecovery()
    {
        // A departure sample followed by the bound native endpoint proves return.
        if (options_.endpointFirst)
            Writer(options_.native, HorPlus(options_.native), la::CorrectionOwner::Gameplay, "recovery-early-endpoint");
        Advance(Milestone::Recovery);
        const float between = (options_.native + HorPlus(options_.native)) / 2;
        Writer(between, HorPlus(options_.native),
            la::CorrectionOwner::Recovery, "recovery-departure");
        Gameplay();
    }

    void Sequence::Provider()
    {
        auto port = authority_.DialogueBoundary(gate_);
        Expect(port.Arm({0xDA, options_.source, 7, options_.native, 70, options_.native, true}).accepted,
            "provider armed only for exit source");
        Advance(Milestone::Armed);
        Observe("provider-arm");
        Advance(Milestone::Joined);
        Setter(options_.native - 0.047f, options_.native - 0.047f, "provider-setter");
        auto foreignWriter = capture_;
        ++foreignWriter.threadId;
        foreignWriter.incomingFov = options_.native - 0.047f;
        float foreignArgument = foreignWriter.incomingFov;
        const auto writerRevision = authority_.Read().revision;
        const auto ignored = authority_.WriterBoundary(gate_).ProcessNative(
            foreignWriter, foreignArgument, engine_, options_.diagnostics);
        Observe("foreign-thread-provider-writer");
        Expect(!ignored.decision.accepted && foreignArgument == foreignWriter.incomingFov &&
            authority_.Read().revision == writerRevision, "foreign writer cannot consume joined provider evidence");
        Writer(options_.native - 0.047f, HorPlus(EnterSample(options_.native - 0.047f)),
            la::CorrectionOwner::Dialogue, "provider-first-writer");
        providerDialogue_ = true;
        Advance(Milestone::Endpoint);
        Writer(70, HorPlus(Reduced()), la::CorrectionOwner::Dialogue, "provider-native-endpoint");
        Advance(Milestone::Dialogue);
        DialogueFrame();
        ForeignProviderSetter();
    }

    void Sequence::ForeignProviderSetter()
    {
        // A callback from a different caller sharing the camera address is
        // neither a continuation nor evidence of terminal ownership.
        auto foreign = setter_;
        ++foreign.callbackSource;
        foreign.incomingFov = 85;
        float argument = foreign.incomingFov;
        const auto revision = authority_.Read().revision;
        const auto rejected = authority_.DialogueBoundary(gate_).ProcessNative(foreign, argument);
        Observe("unrelated-provider-setter");
        Expect(!rejected.decision.accepted && !rejected.prepareArgument &&
            argument == foreign.incomingFov && authority_.Read().revision == revision,
            "unrelated provider setter preserves active lifecycle and raw argument");
    }

    void Sequence::EnterDialogue()
    {
        providerDialogue_ = false;
        Advance(Milestone::Candidate);
        Setter(options_.native, options_.native, "dialogue-start");
        Advance(Milestone::Candidate);
        Setter(options_.native, options_.native, "dialogue-stationary-candidate");
        Advance(Milestone::Dialogue);
        for (const float progress : {0.55f, 0.75f, 0.90f}) {
            const float raw = std::lerp(options_.native, 70.0f, progress);
            Setter(raw, EnterSample(raw), "dialogue-descent-frame");
        }
        DialogueFrame();
    }

    void Sequence::DialogueFrame()
    {
        Advance(Milestone::Dialogue);
        const float prepared = lastDialogueInput_ = Setter(70, Reduced(), "dialogue-target-setter");
        Writer(prepared, HorPlus(Reduced()), providerDialogue_ ? la::CorrectionOwner::Dialogue : la::CorrectionOwner::Gameplay, "dialogue-target-writer");
    }

    void Sequence::ExitDialogue()
    {
        Advance(Milestone::Exiting);
        const float start = std::lerp(70.0f, options_.native, 0.16f);
        const float middle = std::lerp(start, options_.native, 0.4f);
        Setter(start, Reduced(), "dialogue-exit-start");
        Advance(Milestone::Exiting);
        const double progress = (Projection(middle) - Projection(start)) /
            (Projection(options_.native) - Projection(start));
        Setter(middle, Degrees(std::lerp(Projection(Reduced()), Projection(options_.native), progress)),
            "dialogue-exit-frame");
        Advance(Milestone::Rearm);
        Setter(options_.native, options_.native, "dialogue-exit-endpoint");
        setter_.nativeTarget = options_.native;
        Advance(Milestone::Returned);
        Setter(options_.native, options_.native, "dialogue-return-setter");
        setter_.nativeTarget = 70;
        Gameplay();
    }

    unsigned CountBits(std::uint64_t mask)
    {
        unsigned set = 0;
        for (; mask; mask &= mask - 1) ++set;
        return set;
    }

    constexpr std::uint32_t seeds[]{0x7372u, 0x5E771Eu, 0x13579BDFu, 0xA593u,
        0x1234ABCDu, 0xC001CAFEu, 0x600DCAFEu, 0x7781u};

    bool Run()
    {
        Counts counts;
        constexpr unsigned horizon = 4;
        for (const bool diagnosticsOn : {false, true}) {
            const char* mode = diagnosticsOn ? "/ON" : "/OFF";
            for (const auto order : {1u, 2u}) {
                Options options{diagnosticsOn};
                options.armerOrder = order;
                Sequence replay(options, counts, "repro/dialogue-before-first-cinematic/" + std::to_string(order) + mode);
                if (!replay.Follow(Episode::Dialogue) || !replay.Follow(Episode::Cinematic) || !replay.Finish())
                    return false;
            }
            {
                Options options{diagnosticsOn};
                options.endpointFirst = true;
                Sequence replay(options, counts, std::string{"repro/early-recovery-endpoint"} + mode);
                if (!replay.Follow(Episode::Cinematic) || !replay.Follow(Episode::Gameplay) || !replay.Finish())
                    return false;
            }
            {
                Sequence replay({diagnosticsOn}, counts, std::string{"repro/provider-callback-identity"} + mode);
                if (!replay.Follow(Episode::Cinematic) || !replay.Follow(Episode::Dialogue) || !replay.Finish())
                    return false;
            }
            {
                Sequence interrupt({diagnosticsOn}, counts, std::string{"armed-interrupted-by-enter"} + mode);
                if (!interrupt.InterruptProviderWithCinematic()) return false;
            }
            {
                Options options{diagnosticsOn};
                options.gameplayMode = config::GameplayMode::RuntimeCombine;
                Sequence replay(options, counts, std::string{"repro/writer-only-provider-departure"} + mode);
                if (!replay.ReplayProviderDeparture()) return false;
            }
            for (const auto mode : {config::GameplayMode::RuntimeCombine, config::GameplayMode::HorPlus}) {
                for (const bool confirmsAspect : {true, false}) {
                    Options options{diagnosticsOn};
                    options.gameplayMode = mode;
                    Sequence replace(options, counts, "receiver-native-aspect/" +
                        std::to_string(static_cast<unsigned>(mode)) + "/" + std::to_string(confirmsAspect));
                    if (!replace.ReplaceReceiverAtNativeAspect(confirmsAspect)) return false;
                }
            }
            unsigned combinations = 1;
            std::vector<Episode> path;
            for (unsigned length = 1; length <= horizon; ++length) {
                combinations *= 3;
                path.resize(length);
                for (unsigned code = 0; code < combinations; ++code) {
                    unsigned digits = code;
                    std::string label(length, 'G');
                    for (unsigned index = length; index-- > 0;) {
                        path[index] = episodes[digits % 3];
                        label[index] = "GCD"[digits % 3];
                        digits /= 3;
                    }
                    Sequence sequence({diagnosticsOn}, counts, label + mode);
                    for (const auto episode : path)
                        if (!sequence.Follow(episode)) return false;
                    if (!sequence.Finish()) return false;
                }
            }

            // Distinct nonzero linear forms over three bits form a strength-2
            // covering array. No pair of options is forced to vary together.
            constexpr Episode overlaps[]{Episode::Cinematic, Episode::Dialogue, Episode::Gameplay,
                Episode::Dialogue, Episode::Cinematic, Episode::Cinematic, Episode::Gameplay};
            for (unsigned row = 0; row < 8; ++row) {
                const auto bit = [row](unsigned mask) { return CountBits(row & mask) % 2 != 0; };
                Options options{diagnosticsOn};
                options.endpointFirst = bit(1);
                options.armerOrder = bit(6) ? 1 : 2;
                options.native = bit(2) ? 90.0f : 130.311f;
                options.aspect = bit(4) ? 21.0f / 9.0f : 32.0f / 9.0f;
                options.source = bit(3) ? 0xC : 0xB;
                options.gameplayMode = bit(5) ? config::GameplayMode::RuntimeCombine : config::GameplayMode::HorPlus;
                Sequence sequence(options, counts, "pairwise/" + std::to_string(row) + mode);
                for (const auto episode : overlaps)
                    if (!sequence.Follow(episode)) return false;
                if (!sequence.Finish()) return false;
            }

            for (const auto seed : seeds) {
                ++counts.seeds;
                std::uint32_t random = seed;
                Options options{diagnosticsOn};
                options.gameplayMode = (seed & 1) ? config::GameplayMode::RuntimeCombine : config::GameplayMode::HorPlus;
                Sequence sequence(options, counts, "seed/" + std::to_string(seed) + mode);
                for (unsigned step = 0; step < 72; ++step) {
                    random ^= random << 13;
                    random ^= random >> 17;
                    random ^= random << 5;
                    const auto next = episodes[random % 3];
                    if (!sequence.Follow(next)) return false;
                }
                if (!sequence.Finish()) return false;
            }
            constexpr Milestone checkpoints[]{Milestone::Boot, Milestone::Gameplay, Milestone::Entered,
                Milestone::ArmerBound, Milestone::Cinematic, Milestone::Recovery, Milestone::Armed,
                Milestone::Joined, Milestone::Endpoint, Milestone::Candidate, Milestone::Dialogue,
                Milestone::Exiting, Milestone::Rearm, Milestone::Returned};
            constexpr Episode throughCheckpoints[]{Episode::Dialogue, Episode::Cinematic,
                Episode::Dialogue, Episode::Gameplay};
            for (const auto revoke : {Revocation::Gameplay, Revocation::Cinematic, Revocation::Dialogue})
                for (const auto checkpoint : checkpoints) {
                    Options options{diagnosticsOn};
                    options.armerOrder = 1;
                    options.stopAt = checkpoint;
                    options.revoke = revoke;
                    const auto label = "interrupt/" + std::to_string(static_cast<unsigned>(revoke)) +
                        "/" + std::to_string(static_cast<unsigned>(checkpoint)) + mode;
                    try {
                        Sequence sequence(options, counts, label);
                        for (const auto episode : throughCheckpoints)
                            if (!sequence.Follow(episode)) return false;
                    } catch (const Interrupted& interruption) {
                        if (interruption.passed) continue;
                        return false;
                    }
                    return Check(false, "configured settings checkpoint must be reachable");
                }
            for (const auto episode : episodes) {
                Options options{diagnosticsOn};
                options.omitReadback = true;
                Sequence interrupt(options, counts, std::string{"settings/"} + "GCD"[static_cast<unsigned>(episode)] + mode);
                if (!interrupt.Follow(episode) || !interrupt.Interrupt(true)) return false;
            }
            {
                Sequence interrupt({diagnosticsOn}, counts, std::string{"reset-during-recovery"} + mode);
                if (!interrupt.EndDuringRecovery()) return false;
            }
            {
                Sequence pressure({diagnosticsOn}, counts, std::string{"receiver-pressure"} + mode);
                for (std::uintptr_t receiver = 0x200; receiver < 0x200 + la::Authority::MaxScopes + 8; ++receiver)
                    if (!pressure.ReplaceReceiver(receiver)) return false;
                if (!pressure.ReplaceReceiver(0xB) || !pressure.Follow(Episode::Cinematic) || !pressure.Finish())
                    return false;
            }
            Options pressure{diagnosticsOn};
            pressure.omitReadback = true;
            Sequence sequence(pressure, counts, std::string{"readback-pressure"} + mode);
            for (unsigned frame = 0; frame < 2 * la::Authority::MaxEffects; ++frame)
                if (!sequence.Follow(Episode::Gameplay)) return false;
            if (!sequence.Finish()) return false;
        }
        diagnostics::SetEnabled(false);
        const bool coverage = CountBits(counts.episodeEdges) == 9 &&
            CountBits(counts.transitions) == std::size(edges) && counts.pressureFrames > 0;
        std::cout << "M3 matrix: " << CountBits(counts.states) << " observed milestones, " <<
            CountBits(counts.transitions) << '/' << std::size(edges) << " declared edges, " <<
            CountBits(counts.episodeEdges) << "/9 episode edges, " <<
            counts.sequences << " sequences, " << counts.events << " event checkpoints, " <<
            counts.frames << " modeled frames, " << std::size(seeds) << " seeds (" << counts.seeds <<
            " runs), " << counts.pressureFrames << " frames after observation capacity\n";
        return Check(coverage, "M3 coverage and observation pressure");
    }
}
