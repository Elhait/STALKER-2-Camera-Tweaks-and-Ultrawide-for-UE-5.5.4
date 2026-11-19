#pragma once

#include "camera_writer_transaction.hpp"
#include <optional>

namespace plugin::lifecycle_authority
{
    // Value state owned by Authority. No native pointer or mutable RuntimeState
    // reference is retained; source tokens are identities for later validation.
    struct GameplayPhysicalState
    {
        bool enablePending{}, disablePending{}, modePending{};
        bool enableDeferralLogged{}, modeDeferralLogged{};
        config::GameplayMode modeTarget{config::GameplayMode::HorPlus};
        gameplay::ReplayState replay{gameplay::ReplayState::WaitingForAutomaticUpdate};
        std::uintptr_t autoRestoreSource{}, lastSource{};
        float lastFov{NAN};
        camera::GameplayAspectRestorationState restoration{};
        std::uint64_t restorationSequence{};
    };

    namespace detail
    {
        // The enclosing recovery route already validated generation and source.
        // Consume the one-shot write on attempt, including a refused native write.
        inline plugin::AtomicExitHandoffResult ProcessRecoveryAspectHandoff(bool& pending, float& previousFov, bool dialogueActive,
            GameplayPhysicalState& state, const plugin::CameraWriterInput& input, float exitTarget,
            const plugin::CameraWriterEngineOperations& engine)
        {
            plugin::AtomicExitHandoffResult result;
            if (!pending) return result;
            if (dialogueActive) {
                pending = false;
                previousFov = NAN;
                result.event = plugin::AtomicExitHandoffEvent::CancelledDialogue;
                return result;
            }
            const float previous = std::exchange(previousFov, input.incomingFov);
            if (!std::isfinite(previous) || input.incomingFov >= previous - input.atomicHandoffFovEpsilon ||
                input.incomingFov <= exitTarget + input.recoveryInterpolationRangeEpsilon) return result;
            const auto fields = plugin::ReadCameraWriterAspectFlags(engine, input.sourceIdentity, true);
            pending = false;
            if (!fields.aspectReadable || !fields.flagsReadable) return result;
            result.handled = true;
            result.writeSucceeded = plugin::RequestCameraWriterAspectAndFlags(engine, input.sourceIdentity, input.nativeAspect, 4);
            result.event = result.writeSucceeded ? plugin::AtomicExitHandoffEvent::Applied :
                plugin::AtomicExitHandoffEvent::WriteFailed;
            if (result.writeSucceeded) {
                state.replay = gameplay::ReplayState::Complete;
                state.autoRestoreSource = input.sourceIdentity;
            }
            return result;
        }

        struct RecoveryWriterEffect
        {
            bool complete{};
            std::optional<float> argument;
        };

        inline RecoveryWriterEffect EvaluateRecoveryWriter(
            const plugin::CameraWriterInput& input, float exitTarget, float cinematicTarget,
            const plugin::CameraWriterEngineOperations& engine)
        {
            auto fields = plugin::ReadCameraWriterAspectFlags(engine, input.sourceIdentity, true);
            const bool runtimeCombine = input.gameplayEnabled &&
                input.gameplayMode == config::GameplayMode::RuntimeCombine;
            const bool horPlus = input.gameplayEnabled && input.gameplayMode == config::GameplayMode::HorPlus;
            bool confirmed = true;
            if (runtimeCombine) {
                const auto sync = plugin::SynchronizeRuntimeCombineAspect(engine, input.sourceIdentity,
                    input.currentViewportAspectValid ? input.currentViewportAspect : NAN, fields);
                fields = sync.fields;
                confirmed = sync.confirmed;
            }
            gameplay::HorPlusRecoverySample sample;
            sample.source = sample.validatedSource = input.sourceIdentity;
            sample.inputFov = input.incomingFov;
            sample.exitNativeTarget = exitTarget;
            sample.cachedCinematicFov = cinematicTarget;
            sample.aspect = fields.aspect;
            sample.policyAspect = runtimeCombine ? input.currentViewportAspect : fields.aspect;
            sample.policyAspectValid = confirmed && plugin::IsValidCameraWriterAspect(sample.policyAspect);
            sample.flags = fields.flags;
            sample.cameraReadable = fields.aspectReadable && fields.flagsReadable;
            sample.gameplayEnabled = input.gameplayEnabled;
            const bool endpoint = gameplay::IsNativeHorPlusRecoverySample(sample, input.recoveryEndpointTolerance);
            if (!horPlus && !runtimeCombine) return {endpoint, {}};
            const auto action = gameplay::ResolveHorPlusRecoveryAction(
                camera::CoordinatorState::CinematicExiting, sample, endpoint, input.recoveryEndpointTolerance);
            const bool atCinematicEndpoint = runtimeCombine && sample.cameraReadable &&
                plugin::IsValidCameraWriterAspect(sample.aspect) && sample.flags == 4 &&
                plugin::IsValidCameraWriterFov(cinematicTarget) && plugin::IsValidCameraWriterFov(exitTarget) &&
                std::fabs(cinematicTarget - exitTarget) > input.recoveryInterpolationRangeEpsilon &&
                std::fabs(sample.inputFov - cinematicTarget) <= input.recoveryInterpolationRangeEpsilon;
            if (atCinematicEndpoint && action == gameplay::HorPlusRecoveryAction::HoldNativePassThrough)
                return {false, cinematicTarget};
            if (!confirmed) {
                if (action != gameplay::HorPlusRecoveryAction::HoldNativePassThrough &&
                    plugin::IsValidCameraWriterFov(cinematicTarget)) return {false, cinematicTarget};
                return {};
            }
            if (action == gameplay::HorPlusRecoveryAction::ResumeGameplay) return {true, {}};
            if (action == gameplay::HorPlusRecoveryAction::TransformRecoveryInterpolation) {
                const auto corrected = gameplay::ResolveHorPlusRecoveryInterpolationFov(sample, input.nativeAspect);
                if (plugin::IsValidCameraWriterFov(corrected)) return {false, corrected};
            }
            return {};
        }

        // Existing physical helpers use views over atomics/stores. These views
        // are confined to one private reduction. The temporary FOV/reference
        // projections do not become owners: Authority admits its own baseline.
        class GameplayPhysicalWorkingState final
        {
        public:
            explicit GameplayPhysicalWorkingState(const GameplayPhysicalState& state)
                : enable(state.enablePending), disable(state.disablePending), mode(state.modePending),
                  enableLog(state.enableDeferralLogged), modeLog(state.modeDeferralLogged),
                  target(state.modeTarget), replay(state.replay), autoSource(state.autoRestoreSource),
                  lastSource(state.lastSource), lastFov(state.lastFov)
            {
                if (state.restoration.valid)
                    restoration.Update(state.restoration.aspect, state.restoration.source);
            }

            plugin::CameraWriterGameplayStateView View()
            {
                return {enable, disable, mode, enableLog, modeLog, target, replay,
                    autoSource, lastSource, lastFov, restoration, observation, baseline};
            }

            GameplayPhysicalState Read() const
            {
                return {enable.load(), disable.load(), mode.load(), enableLog.load(), modeLog.load(),
                    target.load(), replay.load(), autoSource.load(), lastSource.load(), lastFov.load(),
                    restoration.Read()};
            }

        private:
            std::atomic<bool> enable, disable, mode, enableLog, modeLog;
            std::atomic<config::GameplayMode> target;
            std::atomic<gameplay::ReplayState> replay;
            std::atomic<std::uintptr_t> autoSource, lastSource;
            std::atomic<float> lastFov;
            camera::GameplayAspectRestorationStore restoration;
            camera::CameraFovObservationStore observation;
            camera::GameplayBaselineStore baseline;
        };

        inline plugin::CameraWriterGameplayResult ProcessGameplayPhysicalWriter(
            GameplayPhysicalState& state, const plugin::CameraWriterInput& input,
            const plugin::CameraWriterEngineOperations& engine)
        {
            GameplayPhysicalWorkingState working(state);
            const auto sequence = state.restorationSequence;
            const auto seed = state.restoration.valid ? 1u : 0u;
            auto result = input.gameplayMode == config::GameplayMode::AspectRecalculation
                ? plugin::ProcessGameplayAspectRecalculationWriter(working.View(), input,
                    camera::CoordinatorState::Gameplay, engine, {})
                : plugin::ProcessGameplayHorPlusWriter(working.View(), input,
                    camera::CoordinatorState::Gameplay, engine, {});
            state = working.Read();
            const auto translate = [sequence, seed](camera::GameplayAspectRestorationState& value) {
                if (value.valid) value.observationSequence = sequence + value.observationSequence - seed;
            };
            translate(state.restoration);
            state.restorationSequence = state.restoration.valid ? state.restoration.observationSequence : sequence;
            translate(result.aspectRestoration.state);
            translate(result.enabledTransition.restoration);
            return result;
        }
    }
}
