#pragma once

#include "../config/feature_config.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>

namespace dialogue
{
    struct PostExitProviderActivation
    {
        std::uint64_t generation{};
        std::uint64_t dialogueSettingsEpoch{};
        std::uintptr_t component{};
        std::uintptr_t receiver{};
        std::uint32_t threadId{};
        float nativeStart{};
        float nativeTarget{};
        float nativeContinuityStart{};
        config::DialogueZoomPolicy dialoguePolicy{config::DialogueZoomPolicy::Native};
    };

    struct PostExitProviderWriterEvidence
    {
        PostExitProviderActivation activation{};
        float sample{};
        bool valid{};
    };

    enum class PostExitProviderReject : std::uint8_t
    {
        None,
        Unauthorized,
        InvalidActivation,
        NoActiveCandidate,
        ComponentMismatch,
        ReceiverMismatch,
        ThreadMismatch,
        GenerationMismatch,
        SettingsEpochMismatch,
        NativeStartMismatch,
        NativeTargetMismatch,
        InvalidCallbackSample,
        InvalidSetterArgument,
        CallbackSetterValueMismatch,
        NoMatchedSetterSample,
        InvalidWriterSample,
        WriterSampleMismatch
    };

    inline const char* PostExitProviderRejectName(PostExitProviderReject reason) noexcept
    {
        switch (reason) {
        case PostExitProviderReject::None: return "none";
        case PostExitProviderReject::Unauthorized: return "unauthorized";
        case PostExitProviderReject::InvalidActivation: return "invalid_activation";
        case PostExitProviderReject::NoActiveCandidate: return "no_active_candidate";
        case PostExitProviderReject::ComponentMismatch: return "component_mismatch";
        case PostExitProviderReject::ReceiverMismatch: return "receiver_mismatch";
        case PostExitProviderReject::ThreadMismatch: return "thread_mismatch";
        case PostExitProviderReject::GenerationMismatch: return "generation_mismatch";
        case PostExitProviderReject::SettingsEpochMismatch: return "settings_epoch_mismatch";
        case PostExitProviderReject::NativeStartMismatch: return "native_start_mismatch";
        case PostExitProviderReject::NativeTargetMismatch: return "native_target_mismatch";
        case PostExitProviderReject::InvalidCallbackSample: return "invalid_callback_sample";
        case PostExitProviderReject::InvalidSetterArgument: return "invalid_setter_argument";
        case PostExitProviderReject::CallbackSetterValueMismatch: return "callback_setter_value_mismatch";
        case PostExitProviderReject::NoMatchedSetterSample: return "no_matched_setter_sample";
        case PostExitProviderReject::InvalidWriterSample: return "invalid_writer_sample";
        case PostExitProviderReject::WriterSampleMismatch: return "writer_sample_mismatch";
        }
        return "unknown";
    }

    // One bounded, single-thread handoff from the validated post-EXIT provider
    // activation through its native setter sample to the immediately following
    // camera writer invocation. It never grants authority from FOV value alone.
    class PostExitProviderHandoff
    {
    public:
        void Reset() noexcept
        {
            activation_ = {};
            pending_ = {};
            active_ = false;
            applied_ = false;
            setterSampleReported_ = false;
            writerAwaitingSetterReported_ = false;
            writerTrajectoryActive_ = false;
            lastWriterProgress_ = std::numeric_limits<float>::quiet_NaN();
            recoveryCompleted_ = false;
            setterRejectReported_ = false;
            dialogueAdmissionConsumed_ = false;
            correctedDialogueSetterSampleValid_ = false;
            correctedDialogueSetterSample_ = std::numeric_limits<float>::quiet_NaN();
        }

        bool ObserveArmer(bool authorized, const PostExitProviderActivation& value,
            PostExitProviderReject* rejection = nullptr) noexcept
        {
            if (rejection) *rejection = PostExitProviderReject::None;
            pending_ = {};
            activation_ = {};
            active_ = false;
            applied_ = false;
            setterSampleReported_ = false;
            writerAwaitingSetterReported_ = false;
            writerTrajectoryActive_ = false;
            lastWriterProgress_ = std::numeric_limits<float>::quiet_NaN();
            recoveryCompleted_ = false;
            setterRejectReported_ = false;
            dialogueAdmissionConsumed_ = false;
            correctedDialogueSetterSampleValid_ = false;
            correctedDialogueSetterSample_ = std::numeric_limits<float>::quiet_NaN();
            if (!authorized) {
                if (rejection) *rejection = PostExitProviderReject::Unauthorized;
                return false;
            }
            if (value.generation == 0 ||
                !value.component || !value.receiver ||
                !value.threadId || !ValidFov(value.nativeStart) ||
                !ValidFov(value.nativeTarget) ||
                !ValidFov(value.nativeContinuityStart) ||
                (value.dialoguePolicy != config::DialogueZoomPolicy::Native &&
                    value.dialoguePolicy != config::DialogueZoomPolicy::Adaptive &&
                    value.dialoguePolicy != config::DialogueZoomPolicy::Reduced &&
                    value.dialoguePolicy != config::DialogueZoomPolicy::Disabled)) {
                if (rejection) *rejection = PostExitProviderReject::InvalidActivation;
                return false;
            }
            activation_ = value;
            active_ = true;
            return true;
        }

        bool ObserveSetterSample(bool authorized, std::uintptr_t component,
            std::uintptr_t receiver, std::uint32_t threadId,
            std::uint64_t generation, std::uint64_t dialogueSettingsEpoch,
            float nativeStart, float nativeTarget, float callbackSample,
            float setterArgument, float epsilon = 0.001f,
            PostExitProviderReject* rejection = nullptr) noexcept
        {
            if (rejection) *rejection = PostExitProviderReject::None;
            pending_ = {};
            const auto mismatch = MatchReject(authorized, component, receiver,
                threadId, generation, dialogueSettingsEpoch, nativeStart, nativeTarget);
            if (mismatch != PostExitProviderReject::None) {
                if (rejection) *rejection = mismatch;
                // The native EXIT callback can still issue a setter for the
                // same receiver after the provider armer has captured the
                // next target. Ignore that unrelated component/endpoints pair
                // and keep waiting for the exact provider setter. Retire only
                // when the authorization or activation identity is stale.
                if (active_ && RetireAfterSetterMismatch(mismatch)) Reset();
                return false;
            }
            // After the first exact setter-to-writer pair, subsequent writer
            // samples are owned by the same bounded activation trajectory.
            // Later Dialogue callbacks must not replace that sample cursor.
            if (writerTrajectoryActive_) return true;
            auto failure = mismatch;
            if (failure == PostExitProviderReject::None && !ValidFov(callbackSample))
                failure = PostExitProviderReject::InvalidCallbackSample;
            if (failure == PostExitProviderReject::None && !ValidFov(setterArgument))
                failure = PostExitProviderReject::InvalidSetterArgument;
            if (failure == PostExitProviderReject::None &&
                (!std::isfinite(epsilon) || epsilon < 0.0f ||
                    std::fabs(callbackSample - setterArgument) > epsilon))
                failure = PostExitProviderReject::CallbackSetterValueMismatch;
            if (failure != PostExitProviderReject::None) {
                if (rejection) *rejection = failure;
                return false;
            }
            pending_ = {activation_, callbackSample, true};
            return true;
        }

        PostExitProviderWriterEvidence TakeWriterSample(bool authorized,
            std::uintptr_t source, std::uint32_t threadId,
            std::uint64_t generation, std::uint64_t dialogueSettingsEpoch,
            float writerInput, float epsilon = 0.001f,
            PostExitProviderReject* rejection = nullptr) noexcept
        {
            if (rejection) *rejection = PostExitProviderReject::None;
            const auto pending = pending_;
            pending_ = {};
            if (!pending.valid) {
                const auto identityFailure = MatchReject(authorized,
                    activation_.component, source, threadId, generation,
                    dialogueSettingsEpoch, activation_.nativeStart,
                    activation_.nativeTarget);
                if (identityFailure != PostExitProviderReject::None) {
                    if (rejection) *rejection = identityFailure;
                    if (active_) Reset();
                    return {};
                }
                if (writerTrajectoryActive_) {
                    auto failure = PostExitProviderReject::None;
                    if (!ValidFov(writerInput))
                        failure = PostExitProviderReject::InvalidWriterSample;
                    const float progress = NativeProgress(writerInput);
                    if (failure == PostExitProviderReject::None &&
                        (!std::isfinite(progress) || progress < -0.002f ||
                            progress > 1.002f))
                        failure = PostExitProviderReject::WriterSampleMismatch;
                    if (failure == PostExitProviderReject::None &&
                        progress + 0.002f < lastWriterProgress_)
                        failure = PostExitProviderReject::WriterSampleMismatch;
                    if (failure != PostExitProviderReject::None) {
                        if (rejection) *rejection = failure;
                        Reset();
                        return {};
                    }
                    lastWriterProgress_ = (std::max)(lastWriterProgress_, progress);
                    return {activation_, writerInput, true};
                }
                // A writer may run between provider activation and its
                // Dialogue setter callback. Keep the validated activation
                // alive until that setter sample can be paired.
                if (rejection) *rejection = PostExitProviderReject::NoMatchedSetterSample;
                return {};
            }
            auto failure = PostExitProviderReject::None;
            if (failure == PostExitProviderReject::None)
                failure = MatchReject(authorized, activation_.component, source,
                    threadId, generation, dialogueSettingsEpoch,
                    activation_.nativeStart, activation_.nativeTarget);
            if (failure == PostExitProviderReject::None &&
                (!std::isfinite(epsilon) || epsilon < 0.0f || !std::isfinite(writerInput)))
                failure = PostExitProviderReject::InvalidWriterSample;
            if (failure == PostExitProviderReject::None &&
                std::fabs(writerInput - pending.sample) > epsilon)
                failure = PostExitProviderReject::WriterSampleMismatch;
            if (failure != PostExitProviderReject::None) {
                if (rejection) *rejection = failure;
                if (active_) Reset();
                return {};
            }
            const float progress = NativeProgress(writerInput);
            if (!std::isfinite(progress) || progress < -0.002f || progress > 1.002f) {
                if (rejection) *rejection = PostExitProviderReject::WriterSampleMismatch;
                Reset();
                return {};
            }
            writerTrajectoryActive_ = true;
            lastWriterProgress_ = progress;
            return pending;
        }

        // Recovery authority ends at the paired endpoint, but the native
        // activation can still tick. Preserve its identity only for admission.
        void CompleteRecovery() noexcept
        {
            recoveryCompleted_ = active_;
            active_ = false;
            pending_ = {};
            writerTrajectoryActive_ = false;
            correctedDialogueSetterSampleValid_ = false;
            correctedDialogueSetterSample_ = std::numeric_limits<float>::quiet_NaN();
        }

        bool RecoveryCompleted() const noexcept { return recoveryCompleted_; }

        // The provider can keep writing its native endpoint after the
        // correction handoff has finished. Quarantine only samples still
        // attributable to that exact activation and endpoint neighborhood.
        bool BlocksGameplayBaselineAdmission(std::uintptr_t receiver,
            std::uint32_t threadId, std::uint64_t generation,
            std::uint64_t dialogueSettingsEpoch, float writerInput,
            float endpointTolerance) const noexcept
        {
            if (!recoveryCompleted_ || receiver != activation_.receiver ||
                threadId != activation_.threadId ||
                generation != activation_.generation ||
                dialogueSettingsEpoch != activation_.dialogueSettingsEpoch ||
                !ValidFov(writerInput) || !std::isfinite(endpointTolerance) ||
                endpointTolerance < 0.0f)
                return false;
            const float tolerance = endpointTolerance < 0.25f
                ? 0.25f : endpointTolerance;
            return std::fabs(writerInput - activation_.nativeTarget) <= tolerance;
        }

        bool BlocksRepeatedAdmission(std::uintptr_t component,
            std::uintptr_t receiver, std::uint32_t threadId,
            std::uint64_t generation, std::uint64_t settingsEpoch,
            float nativeStart, float nativeTarget) noexcept
        {
            if (!recoveryCompleted_) return false;
            if (generation != activation_.generation ||
                settingsEpoch != activation_.dialogueSettingsEpoch) {
                Reset();
                return false;
            }
            return component == activation_.component &&
                receiver == activation_.receiver && threadId == activation_.threadId &&
                SameFloat(nativeStart, activation_.nativeStart) &&
                SameFloat(nativeTarget, activation_.nativeTarget);
        }

        bool ConsumeDialogueAdmission(std::uintptr_t component,
            std::uintptr_t receiver, std::uint32_t threadId,
            std::uint64_t generation, std::uint64_t settingsEpoch,
            float nativeStart, float nativeTarget) noexcept
        {
            if (!BlocksRepeatedAdmission(component, receiver, threadId,
                    generation, settingsEpoch, nativeStart, nativeTarget) ||
                dialogueAdmissionConsumed_)
                return false;
            dialogueAdmissionConsumed_ = true;
            return true;
        }

        // Called only for a setter argument from the confirmed provider
        // lifecycle under the camera gate. The native camera can retain this
        // field across several writer calls until another setter updates it.
        bool RecordCorrectedDialogueSetterSample(float correctedSample) noexcept
        {
            if (!recoveryCompleted_ || !dialogueAdmissionConsumed_ ||
                !ValidFov(correctedSample)) return false;
            correctedDialogueSetterSample_ = correctedSample;
            correctedDialogueSetterSampleValid_ = true;
            return true;
        }

        void ClearCorrectedDialogueSetterSample() noexcept
        {
            correctedDialogueSetterSampleValid_ = false;
            correctedDialogueSetterSample_ = std::numeric_limits<float>::quiet_NaN();
        }

        bool MatchesCorrectedDialogueSetterSample(std::uintptr_t receiver,
            std::uint32_t threadId, std::uint64_t generation,
            std::uint64_t settingsEpoch, float writerInput,
            float epsilon = 0.001f) noexcept
        {
            if (!correctedDialogueSetterSampleValid_ || receiver != activation_.receiver ||
                threadId != activation_.threadId || generation != activation_.generation ||
                settingsEpoch != activation_.dialogueSettingsEpoch)
                return false;
            if (!ValidFov(writerInput) || !std::isfinite(epsilon) || epsilon < 0.0f) {
                ClearCorrectedDialogueSetterSample();
                return false;
            }
            const bool matched = std::fabs(writerInput - correctedDialogueSetterSample_) <= epsilon;
            // A different field value ends retention; it is no longer the
            // recorded setter result. Matching numbers corroborate the owned
            // setter record, and never create that provenance on their own.
            if (!matched) ClearCorrectedDialogueSetterSample();
            return matched;
        }

        bool DialogueAdmissionConsumed() const noexcept
        {
            return dialogueAdmissionConsumed_;
        }

        bool Active() const noexcept { return active_; }
        bool MarkAppliedOnce() noexcept
        {
            if (!active_ || applied_) return false;
            applied_ = true;
            return true;
        }
        bool MarkSetterSampleOnce() noexcept
        {
            if (!active_ || setterSampleReported_) return false;
            setterSampleReported_ = true;
            return true;
        }
        bool MarkSetterRejectReportedOnce() noexcept
        {
            if (setterRejectReported_) return false;
            setterRejectReported_ = true;
            return true;
        }
        bool MarkWriterAwaitingSetterOnce() noexcept
        {
            if (!active_ || writerAwaitingSetterReported_) return false;
            writerAwaitingSetterReported_ = true;
            return true;
        }
        const PostExitProviderActivation& Activation() const noexcept
        {
            return activation_;
        }

    private:
        static bool RetireAfterSetterMismatch(PostExitProviderReject reason) noexcept
        {
            return reason == PostExitProviderReject::Unauthorized ||
                reason == PostExitProviderReject::ReceiverMismatch ||
                reason == PostExitProviderReject::GenerationMismatch ||
                reason == PostExitProviderReject::SettingsEpochMismatch;
        }

        float NativeProgress(float sample) const noexcept
        {
            const float span = activation_.nativeTarget - activation_.nativeStart;
            if (!std::isfinite(span) || std::fabs(span) < 0.01f)
                return std::numeric_limits<float>::quiet_NaN();
            return (sample - activation_.nativeStart) / span;
        }

        static bool ValidFov(float value) noexcept
        {
            return std::isfinite(value) && value > 1.0f && value < 179.0f;
        }

        static bool SameFloat(float a, float b) noexcept
        {
            return std::memcmp(&a, &b, sizeof(float)) == 0;
        }

        PostExitProviderReject MatchReject(bool authorized, std::uintptr_t component,
            std::uintptr_t receiver, std::uint32_t threadId,
            std::uint64_t generation, std::uint64_t dialogueSettingsEpoch,
            float nativeStart, float nativeTarget) const noexcept
        {
            if (!authorized) return PostExitProviderReject::Unauthorized;
            if (!active_) return PostExitProviderReject::NoActiveCandidate;
            if (component != activation_.component)
                return PostExitProviderReject::ComponentMismatch;
            if (receiver != activation_.receiver)
                return PostExitProviderReject::ReceiverMismatch;
            if (threadId != activation_.threadId)
                return PostExitProviderReject::ThreadMismatch;
            if (generation != activation_.generation)
                return PostExitProviderReject::GenerationMismatch;
            if (dialogueSettingsEpoch != activation_.dialogueSettingsEpoch)
                return PostExitProviderReject::SettingsEpochMismatch;
            if (!SameFloat(nativeStart, activation_.nativeStart))
                return PostExitProviderReject::NativeStartMismatch;
            if (!SameFloat(nativeTarget, activation_.nativeTarget))
                return PostExitProviderReject::NativeTargetMismatch;
            return PostExitProviderReject::None;
        }

        PostExitProviderActivation activation_{};
        PostExitProviderWriterEvidence pending_{};
        bool active_{};
        bool applied_{};
        bool setterSampleReported_{};
        bool writerAwaitingSetterReported_{};
        bool writerTrajectoryActive_{};
        float lastWriterProgress_{std::numeric_limits<float>::quiet_NaN()};
        bool recoveryCompleted_{};
        bool setterRejectReported_{};
        bool dialogueAdmissionConsumed_{};
        bool correctedDialogueSetterSampleValid_{};
        float correctedDialogueSetterSample_{std::numeric_limits<float>::quiet_NaN()};
    };
}
