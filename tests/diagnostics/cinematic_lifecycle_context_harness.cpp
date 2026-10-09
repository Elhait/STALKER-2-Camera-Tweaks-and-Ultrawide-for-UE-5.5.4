#include "../../src/diagnostics/cinematic_lifecycle_context.hpp"
#include "../../src/cinematics/cinematic_exit_authority.hpp"

#include <iostream>

namespace
{
    bool Check(bool condition, const char* name)
    {
        if (!condition) std::cerr << name << ": FAIL\n";
        return condition;
    }
}

int main()
{
    using namespace diagnostics;
    bool pass = true;

    const CinematicLifecycleGprSnapshot enterGprs{
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x08, 0x09,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15 };
    CinematicLifecycleEnterContext enter{};
    pass &= Check(!CaptureCinematicLifecycleEnter(false, 7, 90.0f,
        enterGprs, 1, 7, true, enter) && !enter.valid,
        "diagnostic_off_skips_enter_context_capture");

    pass &= Check(CaptureCinematicLifecycleEnter(true, 7, 90.0f,
        enterGprs, 2, 7, true, enter) && enter.valid &&
        enter.generation == 7 && enter.xmm0 == 90.0f &&
        enter.gprs.r15 == 0x15 && enter.coordinator == 2 &&
        enter.selectionGeneration == 7 && enter.selectionValid,
        "accepted_enter_captures_xmm_gprs_and_selection");

    CinematicLifecycleExitContext exit{};
    pass &= Check(CaptureCinematicLifecycleExit(true,
        CinematicExitCandidateForm::Legacy, 127.0f, enterGprs,
        2, 7, true, true, 7, 0xA0, 0xA1, true, false, exit) &&
        exit.valid && exit.form == CinematicExitCandidateForm::Legacy &&
        exit.xmm0 == 127.0f && exit.acceptedByF7A &&
        exit.associationGeneration == 7 &&
        exit.aspectStoreTargetObject == 0xA0 &&
        exit.associatedWriterRsi == 0xA1 && exit.associationValid &&
        !exit.associationConflict,
        "raw_accepted_legacy_exit_captures_association_context");

    pass &= Check(CaptureCinematicLifecycleExit(true,
        CinematicExitCandidateForm::Indexed, 90.0f, enterGprs,
        0, 7, false, false, 7, 0xB0, 0, false, true, exit) &&
        exit.valid && exit.form == CinematicExitCandidateForm::Indexed &&
        !exit.acceptedByF7A && !exit.selectionValid &&
        exit.gprs.rbx == 0x02 && exit.gprs.rax == 0x01 &&
        exit.associationConflict && !exit.associationValid,
        "raw_rejected_indexed_exit_preserves_native_operand_registers");

    const CinematicLifecycleGprSnapshot exitGprs{
        0x01, 0x22, 0x33, 0x04, 0x05, 0x66, 0x08, 0x09,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15 };
    const auto equality = CompareCinematicLifecycleGprs(enterGprs, exitGprs);
    pass &= Check((equality & (1u << 0)) != 0 &&
        (equality & (1u << 1)) == 0 && (equality & (1u << 13)) != 0,
        "register_equality_matrix_is_diagnostic_only");

    const auto beforeEnter = enter;
    const auto beforeExit = exit;
    static_cast<void>(CompareCinematicLifecycleGprs(enterGprs, exitGprs));
    pass &= Check(enter.generation == beforeEnter.generation &&
        enter.gprs.rbx == beforeEnter.gprs.rbx &&
        exit.form == beforeExit.form &&
        exit.acceptedByF7A == beforeExit.acceptedByF7A &&
        exit.associationGeneration == beforeExit.associationGeneration,
        "diagnostic_capture_and_comparison_do_not_mutate_lifecycle_inputs");

    const auto dispatchWithDiagnostics = [&](bool diagnosticMode,
        camera::CoordinatorState coordinator, bool selectionValid) {
        CinematicLifecycleExitContext context{};
        if (diagnosticMode)
            static_cast<void>(CaptureCinematicLifecycleExit(true,
                CinematicExitCandidateForm::Legacy, 90.0f, enterGprs,
                static_cast<std::uint32_t>(coordinator), 9, selectionValid,
                cinematics::IsAcceptedCinematicExit(coordinator, selectionValid),
                9, 0x90, 0x91, true, false, context));
        int acceptedMutations = 0;
        const bool accepted = cinematics::DispatchCinematicExit(coordinator,
            selectionValid, [] {}, [&] { ++acceptedMutations; });
        return std::pair{ accepted, acceptedMutations };
    };
    const auto acceptedOff = dispatchWithDiagnostics(false,
        camera::CoordinatorState::CinematicActive, true);
    const auto acceptedOn = dispatchWithDiagnostics(true,
        camera::CoordinatorState::CinematicActive, true);
    const auto rejectedOff = dispatchWithDiagnostics(false,
        camera::CoordinatorState::Gameplay, false);
    const auto rejectedOn = dispatchWithDiagnostics(true,
        camera::CoordinatorState::Gameplay, false);
    pass &= Check(acceptedOff == acceptedOn && acceptedOn.first &&
        acceptedOn.second == 1 && rejectedOff == rejectedOn &&
        !rejectedOn.first && rejectedOn.second == 0,
        "diagnostic_capture_does_not_change_f7a_acceptance_or_mutation");

    std::cout << "cinematic_lifecycle_context=" << (pass ? "PASS" : "FAIL") << '\n';
    return pass ? 0 : 1;
}
