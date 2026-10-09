#include "../../src/cinematics/cinematic_exit_authority.hpp"
#include "../../src/gameplay/horplus_gameplay.hpp"

#include <cmath>
#include <cstdint>
#include <iostream>

namespace
{
    struct ExitState
    {
        camera::CoordinatorState coordinator{camera::CoordinatorState::Gameplay};
        bool selectionValid{};
        std::uint64_t generation{};
        bool associationValid{};
        std::uintptr_t associationSource{};
        std::uintptr_t recoveryOwner{};
        float exitTarget{90.0f};
        float cinematicTarget{127.0f};
        bool initialEstablishment{true};
        bool continuationPending{true};
        bool handoffArmed{true};
        bool exclusionActive{true};
        float writerOutput{90.0f};
    };

    bool Check(bool value, const char* name)
    {
        if (!value) std::cerr << name << ": FAIL\n";
        return value;
    }

    bool SameState(const ExitState& left, const ExitState& right)
    {
        return left.coordinator == right.coordinator &&
            left.selectionValid == right.selectionValid &&
            left.generation == right.generation &&
            left.associationValid == right.associationValid &&
            left.associationSource == right.associationSource &&
            left.recoveryOwner == right.recoveryOwner &&
            left.exitTarget == right.exitTarget &&
            left.cinematicTarget == right.cinematicTarget &&
            left.initialEstablishment == right.initialEstablishment &&
            left.continuationPending == right.continuationPending &&
            left.handoffArmed == right.handoffArmed &&
            left.exclusionActive == right.exclusionActive &&
            left.writerOutput == right.writerOutput;
    }

    bool InvokeExit(ExitState& state, bool hasAssociation)
    {
        return cinematics::DispatchCinematicExit(state.coordinator,
            state.selectionValid, [] {}, [&] {
                state.recoveryOwner = hasAssociation ? state.associationSource : 0;
                state.exitTarget = 88.0f;
                state.cinematicTarget = 0.0f;
                state.initialEstablishment = false;
                state.continuationPending = false;
                state.handoffArmed = false;
                state.exclusionActive = hasAssociation;
                state.associationValid = false;
                state.selectionValid = false;
                state.coordinator = hasAssociation
                    ? camera::CoordinatorState::CinematicExiting
                    : camera::CoordinatorState::Gameplay;
            });
    }

    bool CheckRejectedExitIsObservationOnly(const char* name,
        camera::CoordinatorState coordinator, bool selected)
    {
        ExitState state{};
        state.coordinator = coordinator;
        state.selectionValid = selected;
        state.generation = 41;
        state.associationValid = true;
        state.associationSource = 0xA11;
        state.recoveryOwner = 0xA11;
        state.exitTarget = 91.0f;
        state.cinematicTarget = 127.393f;
        state.initialEstablishment = true;
        state.continuationPending = true;
        state.handoffArmed = true;
        state.exclusionActive = true;
        const ExitState before = state;
        const bool accepted = InvokeExit(state, true);

        const auto expectedOutput = before.coordinator == camera::CoordinatorState::Gameplay
            ? gameplay::EvaluateHorPlus(90.0f, 3440.0f / 1440.0f, 0x4,
                16.0f / 9.0f).outputFov
            : 90.0f;
        const auto actualOutput = state.coordinator == camera::CoordinatorState::Gameplay
            ? gameplay::EvaluateHorPlus(90.0f, 3440.0f / 1440.0f, 0x4,
                16.0f / 9.0f).outputFov
            : 90.0f;
        return Check(!accepted, name) &&
            Check(SameState(state, before), "rejected_exit_preserves_all_mutation_state") &&
            Check(std::fabs(actualOutput - expectedOutput) < 0.001f,
                "rejected_exit_preserves_next_gameplay_output");
    }

    bool CheckAcceptedNoAssociationStillFailsClosed()
    {
        ExitState state{};
        state.coordinator = camera::CoordinatorState::CinematicActive;
        state.selectionValid = true;
        state.generation = 5;
        state.associationValid = false;
        state.associationSource = 0;
        const bool accepted = InvokeExit(state, false);
        return Check(accepted, "active_generation_without_association_is_accepted_exit") &&
            Check(state.coordinator == camera::CoordinatorState::Gameplay &&
                state.recoveryOwner == 0 && !state.selectionValid &&
                !state.associationValid && !state.continuationPending &&
                !state.initialEstablishment && !state.handoffArmed,
                "accepted_no_owner_exit_fails_closed_without_stuck_recovery");
    }

    bool CheckRapidExitEnterRemainsValid()
    {
        ExitState state{};
        state.coordinator = camera::CoordinatorState::CinematicActive;
        state.selectionValid = true;
        state.generation = 10;
        state.associationValid = true;
        state.associationSource = 0xA11;
        const bool acceptedExit = InvokeExit(state, true);
        state.coordinator = camera::CoordinatorState::CinematicActive;
        state.selectionValid = true;
        ++state.generation;
        state.associationValid = true;
        state.associationSource = 0xB22;
        return Check(acceptedExit && state.generation == 11 &&
            state.coordinator == camera::CoordinatorState::CinematicActive &&
            state.selectionValid && state.associationSource == 0xB22,
            "legitimate_exit_then_enter_accepts_new_generation");
    }

    bool CheckDuplicateExitSequence()
    {
        ExitState state{};
        state.coordinator = camera::CoordinatorState::CinematicActive;
        state.selectionValid = true;
        state.generation = 21;
        state.associationValid = true;
        state.associationSource = 0xA11;
        const bool firstAccepted = InvokeExit(state, true);
        const ExitState afterFirst = state;
        const bool duplicateAccepted = InvokeExit(state, true);
        return Check(firstAccepted && !duplicateAccepted &&
            SameState(state, afterFirst),
            "accepted_exit_then_duplicate_preserves_recovery_transaction");
    }
}

int main()
{
    bool pass = true;
    pass &= CheckRejectedExitIsObservationOnly("exit_without_enter_rejected",
        camera::CoordinatorState::Gameplay, false);
    pass &= CheckRejectedExitIsObservationOnly("active_without_valid_selection_rejected",
        camera::CoordinatorState::CinematicActive, false);
    pass &= CheckRejectedExitIsObservationOnly("duplicate_exit_after_exit_rejected",
        camera::CoordinatorState::CinematicExiting, false);
    pass &= CheckRejectedExitIsObservationOnly("duplicate_exit_after_recovery_rejected",
        camera::CoordinatorState::Gameplay, false);
    pass &= CheckAcceptedNoAssociationStillFailsClosed();
    pass &= CheckRapidExitEnterRemainsValid();
    pass &= CheckDuplicateExitSequence();
    std::cout << "cinematic_exit_authority=" << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
