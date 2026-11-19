#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#include "../camera/fov_observation.hpp"
#include "cinematic_fov.hpp"

namespace cinematics
{
    struct EnterActivationCorrectedStartCandidate
    {
        std::uintptr_t receiver{};
        std::uint64_t dialogueSettingsEpoch{};
        float activationStartSample{std::numeric_limits<float>::quiet_NaN()};
        float correctedStart{std::numeric_limits<float>::quiet_NaN()};
        bool valid{};
        bool dialogueContextActive{};
        std::uint64_t dialogueGeneration{};
        std::uint64_t writerSequence{};
        std::uint32_t writerThreadId{};
        float writerAspect{std::numeric_limits<float>::quiet_NaN()};
        float writerPolicyAspect{std::numeric_limits<float>::quiet_NaN()};
        std::uint8_t writerFlags{};
        bool writerFlagsValid{};
        bool cinematicContextActive{};
        bool fromCinematicWriter{};
        std::uint64_t cinematicGeneration{};
        float writerInputFov{std::numeric_limits<float>::quiet_NaN()};
    };

    inline EnterActivationCorrectedStartCandidate MakeDialogueWriterResultCandidate(
        const camera::CameraFovObservation& observation, std::uintptr_t receiver,
        std::uint64_t dialogueSettingsEpoch, std::uint64_t dialogueGeneration,
        bool dialogueContextActive) noexcept
    {
        EnterActivationCorrectedStartCandidate candidate{};
        candidate.dialogueContextActive = dialogueContextActive;
        if (!dialogueContextActive || !receiver ||
            observation.boundary != camera::FovObservationBoundary::CameraWriter ||
            (observation.writerContext != camera::WriterContext::DialogueLifecycleObserved &&
                observation.writerContext !=
                    camera::WriterContext::ConfirmedProviderDialogueLifecycleObserved) ||
            !observation.writerSource.valid || observation.writerSource.value != receiver ||
            !observation.writerThreadId || !observation.publicationSequence ||
            !observation.writerFlagsValid ||
            (observation.writerFlags != 0x4 && observation.writerFlags != 0x5) ||
            !observation.aspect.valid || !std::isfinite(observation.aspect.value) ||
            observation.aspect.value <= 0.0f || !observation.policyAspect.valid ||
            !std::isfinite(observation.policyAspect.value) ||
            observation.policyAspect.value <= 0.0f ||
            !observation.inputFov.valid || observation.inputFov.space != camera::FovSpace::Native ||
            observation.inputFov.provenance != camera::FovProvenance::NativeRegisterInput ||
            !observation.resultFov.valid || !std::isfinite(observation.inputFov.value) ||
            observation.inputFov.value <= 1.0f || observation.inputFov.value >= 179.0f ||
            !std::isfinite(observation.resultFov.value) ||
            observation.resultFov.value <= 1.0f || observation.resultFov.value >= 179.0f)
            return candidate;

        const bool transformedResult = observation.resultFov.space == camera::FovSpace::Transformed &&
            observation.resultFov.provenance == camera::FovProvenance::ModTransformResult;
        const bool passthroughResult = observation.resultFov.space == camera::FovSpace::Native &&
            observation.resultFov.provenance == camera::FovProvenance::PassThroughResult;
        if (!transformedResult && !passthroughResult) return candidate;

        candidate.receiver = receiver;
        candidate.dialogueSettingsEpoch = dialogueSettingsEpoch;
        // This is the sample at the Dialogue writer boundary. It may already
        // contain a Dialogue callback result; it is not pristine authored FOV.
        candidate.activationStartSample = observation.inputFov.value;
        candidate.writerInputFov = observation.inputFov.value;
        candidate.correctedStart = observation.resultFov.value;
        candidate.dialogueGeneration = dialogueGeneration;
        candidate.writerSequence = observation.publicationSequence;
        candidate.writerThreadId = observation.writerThreadId;
        candidate.writerAspect = observation.aspect.value;
        candidate.writerPolicyAspect = observation.policyAspect.value;
        candidate.writerFlags = observation.writerFlags;
        candidate.writerFlagsValid = observation.writerFlagsValid;
        candidate.valid = true;
        return candidate;
    }

    enum class EnterActivationStatus : std::uint8_t
    {
        Unavailable,
        Captured,
        MatchedSample,
        CompletedEndpoint,
        MissingCorrectedStart,
        ForeignSource,
        WriterNotAdmitted,
        InvalidOutput,
        StaleGeneration,
        SettingsChanged,
        EndpointMismatch,
        InvalidSample,
        DegenerateEndpoints,
        OutsideNativeSegment,
    };

    struct EnterActivationSnapshot
    {
        bool armed{};
        bool captured{};
        std::uint64_t generation{};
        std::uint64_t settingsEpoch{};
        std::uintptr_t component{};
        std::uintptr_t receiver{};
        std::uint32_t threadId{};
        std::uintptr_t writerSource{};
        std::uintptr_t writerOutput{};
        float nativeStart{std::numeric_limits<float>::quiet_NaN()};
        float nativeTarget{std::numeric_limits<float>::quiet_NaN()};
        float correctedStart{std::numeric_limits<float>::quiet_NaN()};
        float correctedTarget{std::numeric_limits<float>::quiet_NaN()};
        float sampleWeight{std::numeric_limits<float>::quiet_NaN()};
        float mappedOutput{std::numeric_limits<float>::quiet_NaN()};
        std::uint32_t sampleIndex{};
        bool correctedStartValid{};
        bool correctedStartFromDialogue{};
        bool correctedStartFromCurrentSample{};
        bool correctedStartFromCinematic{};
        std::uint64_t correctedStartWriterSequence{};
        std::uint32_t correctedStartWriterThreadId{};
        float correctedStartWriterInput{std::numeric_limits<float>::quiet_NaN()};
        float correctedStartWriterResult{std::numeric_limits<float>::quiet_NaN()};
        float correctedStartWriterAspect{std::numeric_limits<float>::quiet_NaN()};
        EnterActivationStatus status{EnterActivationStatus::Unavailable};
    };

    // Runtime callers serialize this record through CameraCorrectionGate.
    // One accepted ENTER at a time; the writer transaction applies a mapped
    // value only after its per-invocation association admission succeeds.
    class CinematicEnterActivation
    {
    public:
        void Arm(std::uint64_t generation, std::uint64_t settingsEpoch,
            std::uintptr_t expectedBaselineSource, float baselineNative,
            float baselineCorrected, bool baselinePairValid, float correctedTarget,
            EnterActivationCorrectedStartCandidate dialogueStart = {},
            bool cinematicPredecessor = false,
            float aspect = std::numeric_limits<float>::quiet_NaN(),
            std::uint64_t gameplaySettingsEpoch = 0)
        {
            if (!cinematicPredecessor || writer_.settingsEpoch != settingsEpoch ||
                writer_.gameplaySettingsEpoch != gameplaySettingsEpoch ||
                !std::isfinite(aspect) || !std::isfinite(writer_.aspect) ||
                std::fabs(aspect - writer_.aspect) > 0.001f)
                writer_ = {};
            cinematicPredecessor_ = cinematicPredecessor;
            gameplaySettingsEpoch_ = gameplaySettingsEpoch;
            state_ = {};
            state_.armed = generation != 0;
            state_.generation = generation;
            state_.settingsEpoch = settingsEpoch;
            expectedBaselineSource_ = expectedBaselineSource;
            baselineNative_ = baselineNative;
            baselineCorrected_ = baselineCorrected;
            baselinePairValid_ = baselinePairValid;
            dialogueStart_ = dialogueStart;
            state_.correctedTarget = correctedTarget;
            state_.status = state_.armed ? EnterActivationStatus::Unavailable
                : EnterActivationStatus::StaleGeneration;
        }

        bool Capture(bool enterCaller, std::uint64_t generation,
            std::uint64_t settingsEpoch, std::uintptr_t component,
            std::uintptr_t receiver, std::uint32_t threadId,
            float nativeStart, float nativeTarget,
            std::uint64_t dialogueSettingsEpoch = 0,
            EnterActivationCorrectedStartCandidate currentStart = {})
        {
            if (!state_.armed || generation != state_.generation ||
                settingsEpoch != state_.settingsEpoch || !enterCaller ||
                !component || !receiver || !threadId || !ValidFov(nativeStart) ||
                !ValidFov(nativeTarget))
                return false;

            const auto expectedGeneration = state_.generation;
            const auto expectedEpoch = state_.settingsEpoch;
            const auto correctedTarget = state_.correctedTarget;
            state_ = {};
            state_.armed = true;
            state_.generation = expectedGeneration;
            state_.settingsEpoch = expectedEpoch;
            state_.correctedTarget = correctedTarget;
            state_.component = component;
            state_.receiver = receiver;
            state_.threadId = threadId;
            state_.nativeStart = nativeStart;
            state_.nativeTarget = nativeTarget;
            state_.captured = true;
            const bool dialogueStartMatches = dialogueStart_.valid &&
                dialogueStart_.receiver == receiver &&
                dialogueStart_.dialogueSettingsEpoch == dialogueSettingsEpoch &&
                dialogueStart_.dialogueGeneration < generation && dialogueStart_.writerSequence != 0 &&
                dialogueStart_.writerThreadId == threadId &&
                Close(nativeStart, dialogueStart_.activationStartSample) &&
                ValidFov(dialogueStart_.correctedStart);
            const bool dialogueContextActive = dialogueStart_.dialogueContextActive ||
                currentStart.dialogueContextActive;
            const bool currentStartMatches = !dialogueContextActive && currentStart.valid &&
                currentStart.receiver == receiver &&
                (!currentStart.fromCinematicWriter ||
                    (currentStart.cinematicGeneration != 0 &&
                        currentStart.cinematicGeneration < generation)) &&
                currentStart.dialogueSettingsEpoch == dialogueSettingsEpoch &&
                Close(nativeStart, currentStart.activationStartSample) &&
                ValidFov(currentStart.correctedStart);
            state_.correctedStartValid = dialogueStartMatches ||
                (!dialogueContextActive && currentStartMatches) ||
                (!dialogueContextActive && !currentStart.cinematicContextActive && !dialogueStart_.valid &&
                    !currentStart.valid && baselinePairValid_ &&
                expectedBaselineSource_ == receiver && ValidFov(baselineNative_) &&
                ValidFov(baselineCorrected_) && Close(nativeStart, baselineNative_));
            if (state_.correctedStartValid)
                state_.correctedStart = dialogueStartMatches
                    ? dialogueStart_.correctedStart : currentStartMatches
                        ? currentStart.correctedStart : baselineCorrected_;
            state_.correctedStartFromDialogue = dialogueStartMatches;
            state_.correctedStartFromCurrentSample = !dialogueStartMatches &&
                currentStartMatches && !currentStart.fromCinematicWriter;
            state_.correctedStartFromCinematic = !dialogueStartMatches &&
                currentStartMatches && currentStart.fromCinematicWriter;
            if (dialogueStartMatches) {
                state_.correctedStartWriterSequence = dialogueStart_.writerSequence;
                state_.correctedStartWriterThreadId = dialogueStart_.writerThreadId;
                state_.correctedStartWriterInput = dialogueStart_.activationStartSample;
                state_.correctedStartWriterResult = dialogueStart_.correctedStart;
                state_.correctedStartWriterAspect = dialogueStart_.writerAspect;
            }
            else if (currentStartMatches && currentStart.fromCinematicWriter) {
                state_.correctedStartWriterSequence = currentStart.writerSequence;
                state_.correctedStartWriterThreadId = currentStart.writerThreadId;
                state_.correctedStartWriterInput = currentStart.writerInputFov;
                state_.correctedStartWriterResult = currentStart.correctedStart;
                state_.correctedStartWriterAspect = currentStart.writerAspect;
            }
            state_.status = EnterActivationStatus::Captured;
            return true;
        }

        EnterActivationSnapshot ObserveWriter(bool admitted,
            std::uint64_t generation, std::uint64_t settingsEpoch,
            std::uintptr_t source, std::uintptr_t output, float nativeSample)
        {
            if (!state_.armed || !state_.captured) return {};
            if (generation != state_.generation) return Reject(EnterActivationStatus::StaleGeneration);
            if (settingsEpoch != state_.settingsEpoch) return Reject(EnterActivationStatus::SettingsChanged);
            if (state_.sampleIndex != (std::numeric_limits<std::uint32_t>::max)())
                ++state_.sampleIndex;
            if (!admitted) return Mark(EnterActivationStatus::WriterNotAdmitted);
            if (source != state_.receiver || !source)
                return Reject(EnterActivationStatus::ForeignSource);
            if (!output) return Reject(EnterActivationStatus::InvalidOutput);
            if (!ValidFov(nativeSample)) return Reject(EnterActivationStatus::InvalidSample);
            const float low = (std::min)(state_.nativeStart, state_.nativeTarget);
            const float high = (std::max)(state_.nativeStart, state_.nativeTarget);
            if (nativeSample < low || nativeSample > high)
                return Reject(EnterActivationStatus::OutsideNativeSegment);
            state_.writerSource = source;
            state_.writerOutput = output;
            if (!Close(state_.nativeTarget, state_.correctedTarget))
                return Reject(EnterActivationStatus::EndpointMismatch);
            state_.status = state_.correctedStartValid
                ? EnterActivationStatus::MatchedSample
                : EnterActivationStatus::MissingCorrectedStart;
            if (!state_.correctedStartValid) {
                if (Close(nativeSample, state_.nativeTarget)) state_.armed = false;
                return state_;
            }
            if (!ValidFov(state_.correctedTarget))
                return Reject(EnterActivationStatus::EndpointMismatch);
            if (std::fabs(state_.nativeTarget - state_.nativeStart) < 0.01f)
                return Reject(EnterActivationStatus::DegenerateEndpoints);
            state_.sampleWeight = (nativeSample - state_.nativeStart) /
                (state_.nativeTarget - state_.nativeStart);
            state_.mappedOutput = state_.correctedStart +
                (state_.correctedTarget - state_.correctedStart) *
                state_.sampleWeight;
            if (!ValidFov(state_.mappedOutput)) {
                return Reject(EnterActivationStatus::InvalidSample);
            } else if (Close(nativeSample, state_.nativeTarget)) {
                state_.status = EnterActivationStatus::CompletedEndpoint;
                state_.armed = false;
            }
            return state_;
        }

        EnterActivationSnapshot Read() const
        {
            return state_;
        }

        // A prior production decision is provenance, not recovery authority.
        // This record never stores a pointer to be dereferenced or a frame ID.
        void RecordWriterResult(bool admitted, std::uint64_t generation,
            std::uint64_t settingsEpoch, std::uintptr_t receiver,
            std::uintptr_t output, std::uint32_t thread, std::uint64_t sequence,
            float input, float result, float aspect, std::uint8_t flags,
            bool corrected) noexcept
        {
            const bool inheritedEndpoint = !corrected && writer_.valid &&
                state_.captured && state_.correctedStartValid &&
                writer_.receiver == receiver && writer_.thread == thread &&
                writer_.settingsEpoch == settingsEpoch &&
                writer_.gameplaySettingsEpoch == gameplaySettingsEpoch_ &&
                writer_.flags == flags && std::isfinite(aspect) &&
                std::fabs(writer_.aspect - aspect) <= 0.001f &&
                Close(input, state_.nativeTarget) &&
                Close(state_.nativeTarget, state_.correctedTarget) &&
                Close(input, writer_.result) && Close(result, writer_.result);
            if (!admitted || (!corrected && !inheritedEndpoint) || generation != state_.generation ||
                settingsEpoch != state_.settingsEpoch || !receiver || !output ||
                !thread || !sequence || !ValidFov(input) || !ValidFov(result) ||
                !std::isfinite(aspect) || aspect <= 0.0f ||
                (flags != 0x4 && flags != 0x5)) {
                writer_ = {};
                return;
            }
            writer_ = {true, generation, settingsEpoch, gameplaySettingsEpoch_,
                receiver, output, thread, sequence, input, result, aspect, flags};
            writer_.component = state_.captured ? state_.component : 0;
        }

        EnterActivationCorrectedStartCandidate InheritedWriterStart(
            std::uintptr_t receiver, float start, float aspect, std::uint8_t flags,
            std::uint32_t thread, std::uint64_t gameplayEpoch,
            std::uint64_t dialogueEpoch, std::uintptr_t component = 0) const noexcept
        {
            EnterActivationCorrectedStartCandidate candidate{};
            // Also block blind native treatment while the current generation
            // has a known corrected writer, even before an EXIT is observed.
            candidate.cinematicContextActive = cinematicPredecessor_ || writer_.valid;
            if (!candidate.cinematicContextActive) return candidate;
            if (!writer_.valid || writer_.receiver != receiver || writer_.thread != thread ||
                (writer_.component != 0 && writer_.component != component) ||
                writer_.settingsEpoch != state_.settingsEpoch ||
                writer_.gameplaySettingsEpoch != gameplayEpoch ||
                !std::isfinite(aspect) || std::fabs(writer_.aspect - aspect) > 0.001f ||
                writer_.flags != flags || !ValidFov(start) ||
                (!Close(start, writer_.input) && !Close(start, writer_.result)))
                return candidate;
            candidate.receiver = receiver;
            candidate.activationStartSample = start;
            candidate.writerInputFov = writer_.input;
            candidate.correctedStart = writer_.result;
            candidate.dialogueSettingsEpoch = dialogueEpoch;
            candidate.writerSequence = writer_.sequence;
            candidate.writerThreadId = writer_.thread;
            candidate.writerAspect = writer_.aspect;
            candidate.writerPolicyAspect = writer_.aspect;
            candidate.writerFlags = writer_.flags;
            candidate.writerFlagsValid = true;
            candidate.fromCinematicWriter = true;
            candidate.cinematicGeneration = writer_.generation;
            candidate.valid = true;
            return candidate;
        }

        void RetireForReentry(const CinematicWriterAssociationSnapshot& association) noexcept
        {
            // An empty generation has no new writer to replace this evidence.
            // Arm rechecks settings/aspect; Capture rechecks source/thread/sample.
            const bool compatible = writer_.valid && association.generationActive &&
                association.generation == state_.generation &&
                !association.associationConflict &&
                association.aspectStoreTargetObject == writer_.receiver &&
                (!association.writerObserved ||
                    (association.associationValid && association.lastAdmittedOutput.valid &&
                        association.lastAdmittedOutput.generation == association.generation &&
                        association.lastAdmittedOutput.sequence == writer_.sequence &&
                        association.lastAdmittedOutput.source == writer_.receiver &&
                        association.lastAdmittedOutput.output == writer_.output &&
                        association.latestWriter.sequence == writer_.sequence));
            const auto writer = compatible ? writer_ : WriterProvenance{};
            Retire();
            writer_ = writer;
            cinematicPredecessor_ = true;
        }

        void Retire() noexcept
        {
            state_ = {};
            expectedBaselineSource_ = 0;
            baselineNative_ = baselineCorrected_ =
                std::numeric_limits<float>::quiet_NaN();
            baselinePairValid_ = false;
            dialogueStart_ = {};
            writer_ = {};
            cinematicPredecessor_ = false;
        }

    private:
        static bool ValidFov(float value) noexcept
        {
            return std::isfinite(value) && value > 1.0f && value < 179.0f;
        }
        static bool Close(float a, float b) noexcept
        {
            return std::fabs(a - b) <= 0.05f;
        }
        EnterActivationSnapshot Reject(EnterActivationStatus status)
        {
            state_.status = status;
            state_.armed = false;
            return state_;
        }
        EnterActivationSnapshot Mark(EnterActivationStatus status)
        {
            state_.status = status;
            return state_;
        }

        EnterActivationSnapshot state_{};
        std::uintptr_t expectedBaselineSource_{};
        float baselineNative_{std::numeric_limits<float>::quiet_NaN()};
        float baselineCorrected_{std::numeric_limits<float>::quiet_NaN()};
        bool baselinePairValid_{};
        EnterActivationCorrectedStartCandidate dialogueStart_{};
        struct WriterProvenance {
            bool valid{};
            std::uint64_t generation{}, settingsEpoch{}, gameplaySettingsEpoch{};
            std::uintptr_t receiver{}, output{};
            std::uint32_t thread{};
            std::uint64_t sequence{};
            float input{}, result{}, aspect{};
            std::uint8_t flags{};
            std::uintptr_t component{};
        } writer_{};
        bool cinematicPredecessor_{};
        std::uint64_t gameplaySettingsEpoch_{};
    };

    inline const char* EnterActivationStatusName(EnterActivationStatus status) noexcept
    {
        switch (status) {
        case EnterActivationStatus::Captured: return "captured";
        case EnterActivationStatus::MatchedSample: return "matched-sample";
        case EnterActivationStatus::CompletedEndpoint: return "completed-endpoint";
        case EnterActivationStatus::MissingCorrectedStart: return "missing-corrected-start";
        case EnterActivationStatus::ForeignSource: return "foreign-source";
        case EnterActivationStatus::WriterNotAdmitted: return "writer-not-admitted";
        case EnterActivationStatus::InvalidOutput: return "invalid-output";
        case EnterActivationStatus::StaleGeneration: return "stale-generation";
        case EnterActivationStatus::SettingsChanged: return "settings-changed";
        case EnterActivationStatus::EndpointMismatch: return "endpoint-mismatch";
        case EnterActivationStatus::InvalidSample: return "invalid-sample";
        case EnterActivationStatus::DegenerateEndpoints: return "degenerate-endpoints";
        case EnterActivationStatus::OutsideNativeSegment: return "outside-native-segment";
        default: return "unavailable";
        }
    }
}
