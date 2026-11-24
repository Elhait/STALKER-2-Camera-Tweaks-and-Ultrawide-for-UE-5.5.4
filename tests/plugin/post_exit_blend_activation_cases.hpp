#include <string_view>

#include "../../src/diagnostics/post_exit_blend_setter_pairing.hpp"
#include "../../src/hooks/post_exit_blend_post_setter_resolver.hpp"

std::uintptr_t blendProbeEnter{}, blendProbeExit{}, blendProbeRelocatedEnter{}, blendProbeRelocatedExit{};
diagnostics::BlendCallerEvidence blendProbeObserved{};
void BlendProbeCall(SafetyHookContext&) {}
void BlendProbeArmer(SafetyHookContext& context)
{
    blendProbeObserved = diagnostics::ReadBlendCallerEvidence(context.rsp,
        diagnostics::kBlendArmerReturnSlotOffset, blendProbeEnter, blendProbeExit,
        [](std::uintptr_t address, std::uintptr_t& value) noexcept {
            return platform::win32::ReadMemory(address, &value, sizeof(value));
        }, blendProbeRelocatedEnter, blendProbeRelocatedExit);
}

void ExecutedBlendCallRelocationCases()
{
    Memory code;
    const std::uint8_t caller[]{0x48,0x83,0xEC,0x28,0xE8,0,0,0,0,
        0x48,0x83,0xC4,0x28,0xC3};
    const std::uint8_t armer[]{0x56,0x57,0x48,0x83,0xEC,0x28,
        0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,
        0x48,0x83,0xC4,0x28,0x5F,0x5E,0xC3};
    for (const auto offset : {0u,128u,256u}) {
        std::memcpy(code.bytes + offset, caller, sizeof(caller));
        code.Put<std::int32_t>(offset + 5, static_cast<std::int32_t>(512 - (offset + 9)));
    }
    std::memcpy(code.bytes + 512, armer, sizeof(armer));
    code.Protect(PAGE_EXECUTE_READ);
    Check(FlushInstructionCache(GetCurrentProcess(), code.bytes, 4096), "Probe code cache flush failed");
    auto enter = safetyhook::MidHook::create(code.bytes + 4, BlendProbeCall, safetyhook::MidHook::StartDisabled);
    auto exit = safetyhook::MidHook::create(code.bytes + 132, BlendProbeCall, safetyhook::MidHook::StartDisabled);
    auto observer = safetyhook::MidHook::create(code.bytes + 518, BlendProbeArmer, safetyhook::MidHook::StartDisabled);
    Check(enter && exit && observer, "Actual rel32 mid-hook fixture creation failed");
    const auto mapping = [](const safetyhook::MidHook& hook) {
        return diagnostics::ResolveRelocatedBlendCallReturn(hook.target_address(),
            hook.original_bytes().data(), hook.original_bytes().size(),
            hook.trampoline().data(), hook.trampoline().size());
    };
    blendProbeEnter = code.Address() + 9;
    blendProbeExit = code.Address() + 137;
    blendProbeRelocatedEnter = mapping(*enter);
    blendProbeRelocatedExit = mapping(*exit);
    Check(blendProbeRelocatedEnter && blendProbeRelocatedExit &&
        blendProbeRelocatedEnter != blendProbeEnter && blendProbeRelocatedExit != blendProbeExit,
        "Relocated CALL mapping was not distinct from original return");
    Check(!diagnostics::ResolveRelocatedBlendCallReturn(enter->target_address(),
        enter->original_bytes().data(), 4, enter->trampoline().data(), enter->trampoline().size()),
        "Truncated CALL evidence was accepted");
    auto wrong = enter->original_bytes();
    wrong[1] ^= 1;
    Check(!diagnostics::ResolveRelocatedBlendCallReturn(enter->target_address(), wrong.data(),
        wrong.size(), enter->trampoline().data(), enter->trampoline().size()),
        "Different original/relocated CALL target was accepted");
    Check(observer->enable() && enter->enable() && exit->enable(), "Probe hooks failed to enable");
    reinterpret_cast<void(*)()>(code.bytes)();
    Check(blendProbeObserved.kind == diagnostics::BlendCallerKind::CinematicEnter &&
        blendProbeObserved.returnAddress == blendProbeRelocatedEnter,
        "Executed ENTER CALL was not classified from its actual relocated return");
    reinterpret_cast<void(*)()>(code.bytes + 128)();
    Check(blendProbeObserved.kind == diagnostics::BlendCallerKind::CinematicExit &&
        blendProbeObserved.returnAddress == blendProbeRelocatedExit,
        "Executed EXIT CALL was not classified from its actual relocated return");
    reinterpret_cast<void(*)()>(code.bytes + 256)();
    Check(blendProbeObserved.kind == diagnostics::BlendCallerKind::Unknown,
        "Unhooked foreign CALL was promoted to a known caller");
    const auto neighbor = blendProbeRelocatedEnter + 1;
    const auto nearby = diagnostics::ReadBlendCallerEvidence(0x1000, 0,
        blendProbeEnter, blendProbeExit,
        [neighbor](std::uintptr_t, std::uintptr_t& value) { value = neighbor; return true; },
        blendProbeRelocatedEnter, blendProbeRelocatedExit);
    Check(nearby.kind == diagnostics::BlendCallerKind::Unknown,
        "Neighboring trampoline address was admitted as an exact return");
    blendProbeEnter = blendProbeExit = blendProbeRelocatedEnter = blendProbeRelocatedExit = 0;
    enter->reset(); exit->reset();
    reinterpret_cast<void(*)()>(code.bytes)();
    Check(blendProbeObserved.kind == diagnostics::BlendCallerKind::Unknown,
        "Retired hook mapping survived cleanup");
    observer->reset();
}

void PostExitBlendActivationCases()
{
    CinematicConsumerCases();
    ExecutedBlendCallRelocationCases();
    diagnostics::PostExitBlendObservation timed;
    const auto zero = diagnostics::PostExitBlendObservation::Clock::time_point{};
    timed.Begin(1, 0, NAN, zero);
    Check(timed.Active(zero + std::chrono::seconds(59)) &&
        !timed.Active(zero + std::chrono::seconds(60)), "Capture wall-clock bound failed");
    timed.End();
    timed.Begin(2, 0, NAN, zero + std::chrono::seconds(61));
    Check(!timed.Active(zero + std::chrono::seconds(61)),
        "New generation renewed an expired capture window");
    Memory setterCode;
    auto* setterCall = setterCode.bytes + 384;
    std::memcpy(setterCall, hooks::signatures::PostExitBlendSetterReturn,
        sizeof(hooks::signatures::PostExitBlendSetterReturn));
    setterCode.Protect(PAGE_EXECUTE_READ);
    const hooks::ExecutableSpan setterSpan{setterCode.bytes, 4096};
    const auto setterResolution = hooks::ResolvePostExitBlendPostSetter(setterSpan);
    Check(setterResolution && setterResolution.matches == 1 &&
        setterResolution.hook == setterCall + 6,
        "Unique setter return continuation was not resolved after the call");
    Check(hooks::ValidatePostExitBlendPostSetterCall(setterCall, setterSpan),
        "Setter call and immediate continuation instruction contract was rejected");
    Check(hooks::IsIndependentDialogueSetterReturn(setterCall, 6, setterResolution.hook, setterSpan),
        "Normal six-byte primary hook rejected its own return observer");
    Check(!hooks::IsIndependentDialogueSetterReturn(setterCall, 14, setterResolution.hook, setterSpan),
        "Return observer overlapped a long primary trampoline span");
    const auto patch = reinterpret_cast<std::uintptr_t>(setterCall);
    Check(hooks::validation::PatchSpansOverlap(patch, 14, patch + 6, 8) &&
        hooks::validation::PatchSpansOverlap(patch + 6, 8, patch, 14) &&
        !hooks::validation::PatchSpansOverlap(patch, 6, patch + 6, 8),
        "Patch spans must distinguish overlap from byte-adjacent relocation");
    Check(!hooks::IsIndependentDialogueSetterReturn(setterCall, 6, setterCall + 10, setterSpan) &&
        !hooks::IsIndependentDialogueSetterReturn(setterCall, 6, setterResolution.hook, {setterCall, 6}),
        "Foreign or out-of-bounds Dialogue return observer was accepted");
    setterCode.Protect(PAGE_EXECUTE_READ | PAGE_GUARD);
    Check(!hooks::validation::IsExecutable(reinterpret_cast<std::uintptr_t>(setterCall)),
        "Guard page was accepted as an executable hook site");
    setterCode.Protect(PAGE_EXECUTE_READ);

    setterCode.Protect(PAGE_READWRITE);
    std::memcpy(setterCode.bytes + 768, setterCall,
        sizeof(hooks::signatures::PostExitBlendSetterReturn));
    setterCode.Protect(PAGE_EXECUTE_READ);
    const auto duplicateSetter = hooks::ResolvePostExitBlendPostSetter(setterSpan);
    Check(!duplicateSetter && duplicateSetter.matches == 2 &&
        std::string_view(duplicateSetter.reason) == "ambiguous-signature",
        "Duplicate setter return sites were not rejected as ambiguous");
    setterCode.Protect(PAGE_READWRITE);
    setterCode.bytes[768 + 2] ^= 1;
    setterCode.Protect(PAGE_EXECUTE_READ);
    Check(!hooks::ValidatePostExitBlendPostSetterCall(
            setterCode.bytes + 768, setterSpan),
        "A setter target with an altered virtual slot passed validation");
    setterCode.Protect(PAGE_READWRITE);
    setterCode.bytes[384 + 9] ^= 1;
    setterCode.Protect(PAGE_EXECUTE_READ);
    Check(!hooks::ValidatePostExitBlendPostSetterCall(setterCall, setterSpan),
        "A post-call continuation with an altered camera-field displacement passed validation");
    setterCode.Protect(PAGE_READWRITE);
    setterCode.bytes[384 + 9] ^= 1;
    setterCode.Protect(PAGE_EXECUTE_READ);
    Check(!hooks::ResolvePostExitBlendPostSetter(
            {setterCode.bytes, sizeof(hooks::signatures::PostExitBlendSetterReturn) - 1}),
        "Truncated setter return span was accepted");

    diagnostics::PostExitBlendSetterPairing setterPairing;
    diagnostics::BlendSetterInvocation outer{};
    outer.generation = 4; outer.epoch = 8; outer.component = 0x1000;
    outer.receiver = 0x9000; outer.stackPointer = 0xA000; outer.thread = 11;
    outer.start = 70.0f; outer.target = 90.3115f; outer.sample = 77.6f;
    outer.setterArgument = outer.sample; outer.recordSelected = true;
    auto inner = outer;
    inner.epoch = 9; inner.component = 0x2000; inner.stackPointer = 0xA100;
    Check(setterPairing.Push(outer) && setterPairing.Push(inner),
        "Nested setter invocations were not retained in bounded LIFO storage");
    const auto innerPair = setterPairing.Pop(inner.thread, inner.stackPointer,
        inner.component);
    const auto outerPair = setterPairing.Pop(outer.thread, outer.stackPointer,
        outer.component);
    Check(innerPair.status == diagnostics::BlendSetterPairStatus::Matched &&
        innerPair.invocation.epoch == inner.epoch &&
        outerPair.status == diagnostics::BlendSetterPairStatus::Matched &&
        outerPair.invocation.epoch == outer.epoch,
        "Nested post-setter observations were attached to the wrong invocation");
    Check(setterPairing.Push(outer) &&
        setterPairing.Pop(outer.thread, outer.stackPointer + 8,
            outer.component).status == diagnostics::BlendSetterPairStatus::Mismatch &&
        setterPairing.Pending() == 0,
        "Mismatched stack context retained stale setter evidence");
    Check(setterPairing.Push(outer) &&
        setterPairing.Pop(outer.thread + 1, outer.stackPointer,
            outer.component).status == diagnostics::BlendSetterPairStatus::Mismatch &&
        setterPairing.Pending() == 0,
        "Mismatched native thread retained stale setter evidence");
    auto wrongComponent = outer.component + 0x100;
    Check(setterPairing.Push(outer) &&
        setterPairing.Pop(outer.thread, outer.stackPointer,
            wrongComponent).status == diagnostics::BlendSetterPairStatus::Mismatch &&
        setterPairing.Pending() == 0,
        "Mismatched blend component retained stale setter evidence");
    for (std::size_t i = 0; i < diagnostics::PostExitBlendSetterPairing::Capacity; ++i) {
        auto entry = outer;
        entry.stackPointer += i * 0x10;
        Check(setterPairing.Push(entry),
            "In-bound setter pairing capacity rejected a valid frame");
    }
    Check(!setterPairing.Push(outer) && setterPairing.Overflow() == 1 &&
        setterPairing.Pending() == 0,
        "Setter pairing overflow did not fail closed and clear stale frames");

    Memory code;
    auto* sequence = code.bytes + 512;
    std::memcpy(sequence, hooks::signatures::PostExitBlendStartCapture,
        sizeof(hooks::signatures::PostExitBlendStartCapture));
    code.Protect(PAGE_EXECUTE_READ);
    const hooks::ExecutableSpan codeSpan{code.bytes, 4096};
    const auto resolved = hooks::ResolvePostExitBlendStartCapture(codeSpan);
    Check(resolved && resolved.matches == 1 && resolved.hook == sequence + 5,
        "Unique native start-capture sequence was not structurally resolved");
    Check(hooks::ValidatePostExitBlendStartCapture(sequence, codeSpan),
        "Native start-capture instruction/operand contract was rejected");

    code.Protect(PAGE_READWRITE);
    std::memcpy(code.bytes + 1024, sequence,
        sizeof(hooks::signatures::PostExitBlendStartCapture));
    code.Protect(PAGE_EXECUTE_READ);
    const auto duplicate = hooks::ResolvePostExitBlendStartCapture(codeSpan);
    Check(!duplicate && duplicate.matches == 2 &&
        std::string_view(duplicate.reason) == "ambiguous-signature",
        "Duplicate native sites were not rejected as ambiguous");

    code.Protect(PAGE_READWRITE);
    code.bytes[1024 + 3] ^= 1;
    code.Protect(PAGE_EXECUTE_READ);
    const auto wrongOperand = hooks::ResolvePostExitBlendStartCapture(codeSpan);
    Check(wrongOperand && wrongOperand.matches == 1 &&
        std::string_view(wrongOperand.reason) == "validated",
        "A single valid site was incorrectly rejected because unrelated bytes differed");
    Check(!hooks::ValidatePostExitBlendStartCapture(code.bytes + 1024, codeSpan),
        "Wrong start-store displacement passed decoded operand validation");
    Check(!hooks::ResolvePostExitBlendStartCapture(
            {code.bytes, sizeof(hooks::signatures::PostExitBlendStartCapture) - 1}),
        "Truncated executable span was accepted");

    static_assert(offsetof(SafetyHookContext, rsp) == 0x180);
    static_assert(diagnostics::kBlendArmerReturnSlotOffset == 0x38);
    Memory stack;
    const auto nativeRsp = stack.Address() + 0x100;
    constexpr std::uintptr_t enterReturn = 0x140012345;
    constexpr std::uintptr_t exitReturn = 0x140067890;
    stack.Put(0x100 + diagnostics::kBlendArmerReturnSlotOffset, exitReturn);
    const auto reader = [](std::uintptr_t address, std::uintptr_t& value) noexcept {
        return platform::win32::ReadMemory(address, &value, sizeof(value));
    };
    auto caller = diagnostics::ReadBlendCallerEvidence(nativeRsp,
        diagnostics::kBlendArmerReturnSlotOffset, enterReturn, exitReturn, reader);
    Check(caller.readable && caller.returnAddress == exitReturn &&
        caller.kind == diagnostics::BlendCallerKind::CinematicExit,
        "Native frame return slot did not classify the validated EXIT continuation");
    stack.Put(0x100 + diagnostics::kBlendArmerReturnSlotOffset, enterReturn);
    caller = diagnostics::ReadBlendCallerEvidence(nativeRsp,
        diagnostics::kBlendArmerReturnSlotOffset, enterReturn, exitReturn, reader);
    Check(caller.readable && caller.kind == diagnostics::BlendCallerKind::CinematicEnter,
        "Validated ENTER continuation was not distinguished from EXIT");
    stack.Put(0x100 + diagnostics::kBlendArmerReturnSlotOffset, std::uintptr_t{0x12345678});
    caller = diagnostics::ReadBlendCallerEvidence(nativeRsp,
        diagnostics::kBlendArmerReturnSlotOffset, enterReturn, exitReturn, reader);
    Check(caller.readable && caller.kind == diagnostics::BlendCallerKind::Unknown,
        "Unmatched readable return address was promoted to a known cinematic caller");
    caller = diagnostics::ReadBlendCallerEvidence(1, diagnostics::kBlendArmerReturnSlotOffset,
        enterReturn, exitReturn, reader);
    Check(!caller.readable && caller.kind == diagnostics::BlendCallerKind::Unreadable,
        "Unreadable stack location was not kept unknown");
    caller = diagnostics::ReadBlendCallerEvidence(
        (std::numeric_limits<std::uintptr_t>::max)() - 0x10,
        diagnostics::kBlendArmerReturnSlotOffset, enterReturn, exitReturn, reader);
    Check(!caller.readable && caller.kind == diagnostics::BlendCallerKind::Unreadable,
        "Stack address overflow was not rejected before reading");

    diagnostics::PostExitBlendActivationObservation tracker;
    tracker.Begin(27);
    diagnostics::BlendStartCapture a{};
    a.component = 0x1000; a.receiver = 0x9000; a.thread = 11;
    a.start = 70.0f; a.target = 90.3115f;
    a.caller = {exitReturn, diagnostics::BlendCallerKind::CinematicExit, true};
    const auto* first = tracker.ObserveStartCapture(a);
    Check(first && first->epoch == 1 && first->generation == 27,
        "First candidate epoch was not assigned");
    const auto firstEpoch = first->epoch;
    const auto matched = tracker.MatchTick(a.component, a.receiver, a.start, a.target);
    Check(matched.status == diagnostics::BlendActivationMatchStatus::Matched &&
        matched.candidate.epoch == first->epoch && matched.candidate.tickObserved,
        "Later matching native tick did not confirm the candidate epoch");
    Check(tracker.IsMatchedEpoch(firstEpoch, 27, a.component, a.receiver) &&
        !tracker.IsMatchedEpoch(firstEpoch, 27, a.component, a.receiver + 1) &&
        !tracker.IsMatchedEpoch(firstEpoch, 26, a.component, a.receiver),
        "Matched activation identity did not enforce epoch, generation, component, and receiver");

    auto b = a;
    b.component = 0x2000; b.target = 70.0f; b.thread = 22;
    const auto* second = tracker.ObserveStartCapture(b);
    Check(second && second->epoch == 2,
        "Overlapping component on the same receiver did not get an independent epoch");
    const auto stillB = tracker.MatchTick(b.component, b.receiver, b.start, b.target);
    Check(stillB.status == diagnostics::BlendActivationMatchStatus::Matched &&
        stillB.candidate.epoch == 2,
        "Persistent second blend context did not retain its independent epoch");
    const auto stillA = tracker.MatchTick(a.component, a.receiver, a.start, a.target);
    Check(stillA.status == diagnostics::BlendActivationMatchStatus::Matched &&
        stillA.candidate.epoch == 1,
        "Second context replaced the first context on a shared receiver");

    auto rearmedA = a;
    rearmedA.target = 85.0f;
    const auto* rearmed = tracker.ObserveStartCapture(rearmedA);
    Check(rearmed && rearmed->epoch == 3,
        "Same-pointer rearm did not create a fresh candidate epoch");
    const auto staleTick = tracker.MatchTick(a.component, a.receiver, a.start, a.target);
    Check(staleTick.status == diagnostics::BlendActivationMatchStatus::FieldsChanged &&
        tracker.MatchTick(a.component, a.receiver, rearmedA.start,
            rearmedA.target).status == diagnostics::BlendActivationMatchStatus::NoCandidate,
        "Stale tick mismatch did not conservatively retire the rearmed candidate");

    const auto* afterMismatch = tracker.ObserveStartCapture(rearmedA);
    Check(afterMismatch && afterMismatch->epoch == 4,
        "Fresh capture did not recover from explicit mismatch retirement");
    const auto unrelatedReceiver = tracker.MatchTick(rearmedA.component,
        rearmedA.receiver + 0x1000, rearmedA.start, rearmedA.target);
    Check(unrelatedReceiver.status == diagnostics::BlendActivationMatchStatus::FieldsChanged &&
        tracker.MatchTick(rearmedA.component, rearmedA.receiver,
            rearmedA.start, rearmedA.target).status ==
                diagnostics::BlendActivationMatchStatus::NoCandidate,
        "A receiver mismatch retained candidate attribution");
    tracker.Retire();
    Check(!tracker.Active() && tracker.MatchTick(a.component, a.receiver,
            rearmedA.start, rearmedA.target).status ==
            diagnostics::BlendActivationMatchStatus::NoCandidate,
        "Episode retirement left reusable activation evidence");
    Check(!tracker.IsMatchedEpoch(firstEpoch, 27, a.component, a.receiver),
        "Retired activation remained eligible for post-setter correlation");
    tracker.Begin(28);
    auto unknownCallerCapture = a;
    unknownCallerCapture.caller = {0x12345678,
        diagnostics::BlendCallerKind::Unknown, true};
    const auto* nextGeneration = tracker.ObserveStartCapture(unknownCallerCapture);
    Check(tracker.Generation() == 28 && nextGeneration &&
        nextGeneration->epoch == 5 && !nextGeneration->tickObserved &&
        nextGeneration->capture.caller.kind == diagnostics::BlendCallerKind::Unknown,
        "New generation resurrected prior candidate state");

    tracker.Begin(29);
    for (std::uintptr_t i = 1; i <= diagnostics::PostExitBlendActivationObservation::Capacity; ++i) {
        auto candidate = a;
        candidate.component = 0x10000 + i * 0x100;
        Check(tracker.ObserveStartCapture(candidate) != nullptr,
            "Activation capacity rejected an in-bound candidate");
    }
    auto overflow = a;
    overflow.component = 0x90000;
    Check(tracker.ObserveStartCapture(overflow) == nullptr && tracker.Overflow() == 1,
        "Activation capacity exhaustion was not explicit and bounded");
}
