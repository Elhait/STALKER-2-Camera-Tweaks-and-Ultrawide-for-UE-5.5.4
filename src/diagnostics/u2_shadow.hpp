#pragma once
#include <cmath>
#include "../camera/horplus.hpp"
#include "../gameplay/gameplay_state.hpp"

namespace diagnostics {
struct U2ShadowInput {
    bool enabled{}, matchedActivation{}, receiverMatches{}, policyEligible{};
    float start{NAN}, target{NAN}, sample{NAN}, correctedEndpoint{NAN}, aspect{NAN};
    float correctedTarget{NAN};
};
struct U2ShadowResult {
    const char* status{"disabled"};
    float weight{NAN}, correctedTarget{NAN}, hypothetical{NAN};
    bool available{};
};
// Algebraic hypothesis only: no claim of the executed easing branch, native
// sample space, rendered start, or authority to modify this setter/writer.
inline U2ShadowResult EvaluateU2Shadow(const U2ShadowInput& in) noexcept {
    U2ShadowResult out{};
    if(!in.enabled) return out;
    out.status="missing-activation-or-context";
    if(!in.matchedActivation || !in.receiverMatches || !in.policyEligible) return out;
    const auto fov=[](float f){return std::isfinite(f) && f>1 && f<179;};
    out.status="invalid-endpoints-or-aspect";
    if(!fov(in.start)||!fov(in.target)||!fov(in.sample)||!fov(in.correctedEndpoint)||
        !std::isfinite(in.aspect)||in.aspect<=0) return out;
    out.status="degenerate-endpoints";
    if(std::fabs(in.target-in.start)<0.01f) return out;
    out.weight=(in.sample-in.start)/(in.target-in.start);
    out.status="outside-affine-segment";
    if(!std::isfinite(out.weight)||out.weight<0||out.weight>1) return out;
    out.correctedTarget=fov(in.correctedTarget) ? in.correctedTarget
        : camera::HorPlus(in.target,in.aspect,16.0f/9.0f);
    out.hypothetical=in.correctedEndpoint+(out.correctedTarget-in.correctedEndpoint)*out.weight;
    out.available=fov(out.hypothetical);
    out.status=out.available ? "conditional-affine-hypothesis" : "invalid-result";
    return out;
}
inline const char* U2ShadowWriterAction(const gameplay::HorPlusRecoverySample& sample,
    camera::CoordinatorState owner, float tolerance) noexcept {
    const auto action=gameplay::ResolveHorPlusRecoveryAction(owner,sample,
        gameplay::IsNativeHorPlusRecoverySample(sample,tolerance),tolerance);
    switch(action) {
    case gameplay::HorPlusRecoveryAction::TransformRecoveryInterpolation: return "would-remap";
    case gameplay::HorPlusRecoveryAction::ResumeGameplay: return "would-resume-gameplay";
    case gameplay::HorPlusRecoveryAction::HoldNativePassThrough: return "would-pass-through";
    default: return "not-recovery";
    }
}
}
