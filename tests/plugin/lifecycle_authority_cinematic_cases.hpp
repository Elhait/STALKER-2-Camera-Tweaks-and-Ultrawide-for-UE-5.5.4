// Production and Authority receive the same captured native blend segment.
// Intermediate samples exercise the real activation mapper, not just endpoints.
namespace authority_cinematic
{
    bool Run()
    {
        namespace la = plugin::lifecycle_authority;
        bool pass = true;
        for (const bool diagnosticsOn : {false, true}) {
            authority_parity::Pair sequence(diagnosticsOn);
            sequence.Writer("blend/gameplay", 130.311f);
            sequence.Enter("blend/first-enter");
            const float firstTarget = sequence.production.cinematicTargetFov.load();
            sequence.CaptureCinematic(130.311f, firstTarget);
            for (const float weight : {0.0f, 0.25f, 0.5f, 0.75f, 1.0f})
                sequence.Writer("blend/first-cinematic-progress",
                    130.311f + (firstTarget - 130.311f) * weight, false, false, false,
                    la::FovProvenance::PreparedCinematic);
            sequence.Exit("blend/exit");
            sequence.Writer("blend/recovery", 130.311f, true, true);
            sequence.Arm("blend/provider-arm");
            sequence.Setter("blend/provider-descent", 130.264f, 70.0f, true);
            sequence.Writer("blend/provider-first", 130.264f, false, false, true);
            sequence.Writer("blend/provider-endpoint", 70.0f, false, false, true);
            sequence.Setter("blend/provider-endpoint-setter", 70.0f, 70.0f, true);
            sequence.Writer("blend/prepared-dialogue", 122.087f, true, false, false,
                la::FovProvenance::PreparedDialogue);
            sequence.Enter("blend/second-enter");
            const float secondTarget = sequence.production.cinematicTargetFov.load();
            sequence.CaptureCinematic(122.087f, secondTarget);
            for (const float weight : {0.0f, 0.25f, 0.5f, 0.75f, 1.0f})
                sequence.Writer("blend/dialogue-to-cinematic-progress",
                    122.087f + (secondTarget - 122.087f) * weight, false, false, false,
                    la::FovProvenance::PreparedCinematic);
            const auto state = sequence.authority.Read();
            const auto& scope = state.scopes.at(sequence.production.freshFields.expectedSource);
            const auto activation = scope.cinematicActivation.Read();
            pass &= Check(sequence.divergences == 0 && activation.correctedStartFromDialogue &&
                activation.status == cinematics::EnterActivationStatus::CompletedEndpoint &&
                scope.dialogue == la::DialoguePhase::None,
                "cinematic_full_progress_retains_dialogue_corrected_start_and_releases_dialogue_owner");
        }
        diagnostics::SetEnabled(false);
        std::cout << (pass ? "PASS" : "FAIL") << ": Authority Cinematic activation progress OFF/ON parity\n";
        return pass;
    }
}
