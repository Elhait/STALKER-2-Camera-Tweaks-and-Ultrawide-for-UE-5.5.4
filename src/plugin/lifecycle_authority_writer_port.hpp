#pragma once

#include "lifecycle_authority_policy.hpp"
#include "camera_correction_gate.hpp"

namespace plugin::lifecycle_authority
{
    // The token identifies a Writer observation window. The receiver, output
    // object and thread are sampled independently at the readback boundary.
    struct WriterReadback
    {
        EventId observationId{};
        TransactionId invocationId{};
        ScopeId scope{};
        std::uintptr_t receiver{};
        std::uintptr_t output{};
        std::uint32_t thread{};
        std::uint64_t generation{};
        std::uint64_t writerRevision{};
        FovBoundary boundary{};
        float value{};
    };

    struct WriterPortResult
    {
        Decision decision;
        TransactionId writerInvocationId{};
        EventId observationId{};
        bool observationUnavailable{};
        bool argumentPrepared{};
        std::optional<plugin::CameraWriterGameplayResult> physical;
    };

    // The native Writer adapter receives only this capability. It cannot call
    // legacy per-step events or retain a WriterInvocation execution permission.
    // Runtime owns Authority and gate and must outlive the port/callback drain.
    class WriterAdapterPort final
    {
    public:
        WriterPortResult ProcessNative(const plugin::CameraWriterInput& sample, float& argument,
            const plugin::CameraWriterEngineOperations& engine, bool observeOutput) const
        {
            if (!gate_->IsHeldByCurrentThread() || !sample.sourceIdentity ||
                !sample.outputObjectIdentity || !sample.threadId) return {};
            WriterEvent event;
            event.observeOutput = observeOutput;
            event.nativeBoundary = true;
            auto& facts = event.input;
            facts.scope = facts.receiver = sample.sourceIdentity;
            facts.outputObject = sample.outputObjectIdentity;
            facts.thread = sample.threadId;
            facts.fov = sample.incomingFov;
            facts.fovBoundary = FovBoundary::PrimaryWriterInput;
            facts.aspect = sample.aspect;
            facts.aspectValid = sample.aspectValid;
            facts.flags = sample.flags;
            facts.flagsValid = sample.flagsValid;
            facts.viewportAspect = sample.currentViewportAspect;
            facts.viewportAspectValid = sample.currentViewportAspectValid;
            facts.evidence.callbackBoundaryValidated = true;
            facts.evidence.sourceRelationValidated = sample.aspectValid && sample.flagsValid;

            return Process(event, argument, engine);
        }

        bool Observe(const WriterReadback& input) const
        {
            if (!gate_->IsHeldByCurrentThread() ||
                (input.boundary != FovBoundary::PostStore && input.boundary != FovBoundary::PostHelper) ||
                !plugin::IsValidCameraWriterFov(input.value)) return false;
            const auto found = owner_->state_.observations.find(input.observationId);
            if (found == owner_->state_.observations.end()) return false;
            auto facts = ObservationFacts(input, EventKind::EffectObserved);
            facts.fovProvenance = FovProvenance::EffectiveReadback;
            facts.evidence.effectBoundaryCorrelated = true;
            facts.effectResult = std::fabs(found->second.plannedOutputFov - input.value) <= 0.001
                ? EffectResult::Applied : EffectResult::Partial;
            return owner_->Accept(facts).accepted;
        }

        bool Close(const WriterReadback& input) const
        {
            return gate_->IsHeldByCurrentThread() && owner_->state_.observations.contains(input.observationId) &&
                owner_->Accept(ObservationFacts(input, EventKind::EffectClosed)).accepted;
        }

        WriterPortResult Process(const WriterEvent& input, float& nativeArgument,
            std::optional<plugin::CameraWriterEngineOperations> engine = {}) const
        {
            WriterPortResult result;
            if (!gate_->IsHeldByCurrentThread() || !input.input.aspectValid || !input.input.flagsValid ||
                nativeArgument != static_cast<float>(input.input.fov)) return result;
            auto event = input;
            event.engine = engine;
            PolicyFields fields{static_cast<float>(event.input.aspect),
                16.0f / 9.0f, event.input.flags};
            event.policyContext = &fields;
            event.evaluate = +[](void* context, const WriterPolicyContext& policy) {
                return EvaluateProductionPolicy(*static_cast<const PolicyFields*>(context), policy);
            };
            auto invocation = owner_->Accept(event);
            double prepared = nativeArgument;
            result.argumentPrepared = owner_->PrepareWriterArgument(invocation, prepared);
            if (result.argumentPrepared) nativeArgument = static_cast<float>(prepared);
            else owner_->FinalizeUnchangedWriterArgument(invocation, nativeArgument);
            result.decision = invocation.decision;
            result.writerInvocationId = invocation.writerInvocationId;
            result.observationId = invocation.observationId;
            result.observationUnavailable = invocation.observationUnavailable;
            result.physical = std::move(invocation.physical);
            // invocation is destroyed before returning to PRE; native MOVSS
            // has not yet happened, so this API exposes no Applied result.
            return result;
        }

    private:
        static EventFacts ObservationFacts(const WriterReadback& input, EventKind kind)
        {
            EventFacts facts;
            facts.kind = kind;
            facts.effectId = input.observationId;
            facts.transactionId = input.invocationId;
            facts.scope = input.scope;
            facts.receiver = input.receiver;
            facts.outputObject = input.output;
            facts.thread = input.thread;
            facts.generation = input.generation;
            facts.dependencyRevision = input.writerRevision;
            facts.fovBoundary = input.boundary;
            facts.fov = input.value;
            facts.evidence.callbackBoundaryValidated = true;
            return facts;
        }

        friend class Authority;
        WriterAdapterPort(Authority& owner, const plugin::CameraCorrectionGate& gate)
            : owner_(&owner), gate_(&gate) {}
        Authority* owner_;
        const plugin::CameraCorrectionGate* gate_;
    };

    inline WriterAdapterPort Authority::WriterBoundary(const plugin::CameraCorrectionGate& gate) noexcept
    {
        return WriterAdapterPort(*this, gate);
    }
}
