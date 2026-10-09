#include "../../src/plugin/runtime_settings.hpp"

#include <iostream>

namespace
{
    bool Check(bool condition, const char* name)
    {
        if (!condition) std::cerr << "FAIL: " << name << "\n";
        return condition;
    }

    bool ReadFacts(plugin::OverlaySemanticSnapshot& snapshot, void*)
    {
        snapshot = {};
        snapshot.gameplayEnabled = {true, true,
            plugin::SemanticProvenance::RuntimeSettings,
            plugin::SemanticFreshness::AtRead};
        snapshot.gameplayHookAvailable = {false, true,
            plugin::SemanticProvenance::GameplayHookGate,
            plugin::SemanticFreshness::AtRead};
        snapshot.runtimeViewportAspect = {0.0f, false,
            plugin::SemanticProvenance::RuntimeViewportResolver,
            plugin::SemanticFreshness::AtRead};
        snapshot.cameraState.evidence.nativeFov = {52.2812f, true,
            camera::EvidenceProvenance::NativeNumericObservation,
            camera::SnapshotFreshness::Current, 0x1234, true, 1};
        snapshot.cameraState.evidence.horPlusTargetFov = {89.1234f, true,
            camera::EvidenceProvenance::ModDerivedState,
            camera::SnapshotFreshness::Current, 0x1234, true, 1};
        snapshot.cameraStateValid = true;
        return true;
    }
}

int main()
{
    plugin::RuntimeSettingsApi api;
    plugin::OverlaySemanticSnapshot snapshot{};
    bool pass = true;
    pass &= Check(!api.SemanticSnapshot(snapshot), "missing_reader_fails_closed");
    pass &= Check(api.Publish({nullptr, nullptr, nullptr, nullptr,
        &ReadFacts, nullptr, nullptr, nullptr}), "semantic_reader_bundle_published");
    pass &= Check(api.SemanticSnapshot(snapshot), "semantic_reader_available");
    pass &= Check(snapshot.gameplayEnabled.valid && snapshot.gameplayEnabled.value &&
        snapshot.gameplayEnabled.provenance == plugin::SemanticProvenance::RuntimeSettings,
        "configured_fact_has_own_provenance");
    pass &= Check(snapshot.gameplayHookAvailable.valid && !snapshot.gameplayHookAvailable.value &&
        snapshot.gameplayHookAvailable.provenance == plugin::SemanticProvenance::GameplayHookGate,
        "capability_fact_is_independent_of_configuration");
    pass &= Check(!snapshot.runtimeViewportAspect.valid &&
        snapshot.runtimeViewportAspect.provenance ==
            plugin::SemanticProvenance::RuntimeViewportResolver,
        "unavailable_observation_keeps_invalidity_and_source");
    pass &= Check(snapshot.cameraStateValid &&
        snapshot.cameraState.evidence.nativeFov.valid &&
        snapshot.cameraState.evidence.horPlusTargetFov.valid &&
        snapshot.cameraState.evidence.nativeFov.value == 52.2812f &&
        snapshot.cameraState.evidence.horPlusTargetFov.value == 89.1234f,
        "camera_state_projection_preserves_shared_snapshot");
    std::cout << "Overlay semantic snapshot harness: " << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
