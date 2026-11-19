#pragma once

#include "lifecycle_authority_core.hpp"
#include "camera_correction_gate.hpp"
#include <bit>

namespace plugin::lifecycle_authority
{
    struct NativeDialogueSetter
    {
        std::uintptr_t callbackSource{}, component{}, receiver{};
        std::uint32_t thread{};
        float incomingFov{}, nativeStart{}, nativeTarget{};
        bool receiverValidated{}, nativeTargetReadable{};
    };

    class DialogueAdapterPort final
    {
    public:
        Decision Arm(const ProviderArmEvent& event) const
        {
            return gate_->IsHeldByCurrentThread() ? owner_->Accept(event) : Decision{};
        }

        SetterDecision ProcessNative(const NativeDialogueSetter& capture, float& nativeArgument) const
        {
            SetterEvent event;
            event.callbackSource = capture.callbackSource;
            event.nativeArgumentBefore = nativeArgument;
            event.nativeBoundary = true;
            auto& facts = event.input;
            facts.scope = facts.receiver = capture.receiver;
            facts.thread = capture.thread;
            facts.component = capture.component;
            facts.fov = capture.incomingFov;
            facts.nativeStart = capture.nativeStart;
            facts.nativeTarget = capture.nativeTarget;
            facts.nativeTargetReadable = capture.nativeTargetReadable;
            facts.fovBoundary = FovBoundary::DialogueSetterInput;
            facts.fovProvenance = FovProvenance::OriginalNative;
            facts.evidence.callbackBoundaryValidated = capture.callbackSource != 0 && capture.thread != 0;
            facts.evidence.sourceRelationValidated = capture.receiverValidated;
            return Process(event, nativeArgument);
        }

        SetterDecision Process(const SetterEvent& event, float& nativeArgument) const
        {
            SetterDecision result;
            result.outputFov = nativeArgument;
            if (!gate_->IsHeldByCurrentThread() ||
                event.input.fovBoundary != FovBoundary::DialogueSetterInput ||
                !event.nativeArgumentBefore ||
                std::bit_cast<std::uint32_t>(nativeArgument) !=
                    std::bit_cast<std::uint32_t>(*event.nativeArgumentBefore))
                return result;
            result = owner_->Accept(event);
            if (result.decision.accepted && result.prepareArgument)
                nativeArgument = static_cast<float>(result.outputFov);
            return result;
        }

    private:
        friend class Authority;
        DialogueAdapterPort(Authority& owner, const plugin::CameraCorrectionGate& gate)
            : owner_(&owner), gate_(&gate) {}
        Authority* owner_;
        const plugin::CameraCorrectionGate* gate_;
    };

    inline DialogueAdapterPort Authority::DialogueBoundary(const plugin::CameraCorrectionGate& gate) noexcept
    {
        return DialogueAdapterPort(*this, gate);
    }
}
