#include "../../src/plugin/runtime_settings.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

namespace
{
    struct MutationPersistenceFixture
    {
        std::mutex mutex;
        std::condition_variable changed;
        int active{};
        int saved{};
        int applyCount{};
        bool firstApplyEntered{};
        bool releaseFirstApply{};
        bool failPersistence{};
    };

    plugin::RuntimeMutationResult ApplyMutation(
        const plugin::RuntimeSettingMutation& mutation, void* userData)
    {
        auto& fixture = *static_cast<MutationPersistenceFixture*>(userData);
        const auto value = std::get<int>(mutation.value);
        std::unique_lock lock(fixture.mutex);
        fixture.active = value;
        ++fixture.applyCount;
        if (value == 11) {
            fixture.firstApplyEntered = true;
            fixture.changed.notify_all();
            fixture.changed.wait(lock, [&] { return fixture.releaseFirstApply; });
        }
        return {true, true};
    }

    bool PersistMutation(const plugin::RuntimeSettingMutation& mutation, void* userData)
    {
        auto& fixture = *static_cast<MutationPersistenceFixture*>(userData);
        std::lock_guard lock(fixture.mutex);
        if (fixture.failPersistence) return false;
        fixture.saved = std::get<int>(mutation.value);
        return true;
    }

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

    bool TestMutationPersistenceOrdering()
    {
        MutationPersistenceFixture fixture{};
        plugin::RuntimeSettingsApi api;
        if (!api.Publish({&ApplyMutation, &fixture, nullptr, nullptr,
            nullptr, nullptr, &PersistMutation, &fixture})) return false;

        const auto first = plugin::RuntimeSettingMutation::FontSize(11);
        const auto second = plugin::RuntimeSettingMutation::FontSize(22);
        plugin::RuntimeSettingsCommitResult firstResult{};
        plugin::RuntimeSettingsCommitResult secondResult{};
        std::thread firstProducer([&] { firstResult = api.ApplyAndPersist(first); });
        {
            std::unique_lock lock(fixture.mutex);
            fixture.changed.wait(lock, [&] { return fixture.firstApplyEntered; });
        }
        std::atomic<bool> secondAttempting{};
        std::thread secondProducer([&] {
            secondAttempting.store(true, std::memory_order_release);
            secondResult = api.ApplyAndPersist(second);
        });
        while (!secondAttempting.load(std::memory_order_acquire))
            std::this_thread::yield();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        bool serialized = false;
        {
            std::lock_guard lock(fixture.mutex);
            serialized = fixture.applyCount == 1 && fixture.active == 11 && fixture.saved == 0;
            fixture.releaseFirstApply = true;
        }
        fixture.changed.notify_all();
        firstProducer.join();
        secondProducer.join();

        bool ordered = false;
        {
            std::lock_guard lock(fixture.mutex);
            ordered = fixture.applyCount == 2 && fixture.active == 22 && fixture.saved == 22;
            fixture.failPersistence = true;
        }
        const auto failedSave = api.ApplyAndPersist(
            plugin::RuntimeSettingMutation::FontSize(33));
        bool failureTruthful = false;
        {
            std::lock_guard lock(fixture.mutex);
            failureTruthful = failedSave.mutation.accepted && !failedSave.persisted &&
                fixture.active == 33 && fixture.saved == 22;
        }
        return serialized && firstResult.mutation.accepted && firstResult.persisted &&
            secondResult.mutation.accepted && secondResult.persisted && ordered &&
            failureTruthful;
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
    pass &= Check(TestMutationPersistenceOrdering(),
        "runtime mutation and persistence are serialized; failed save stays explicitly unsaved");
    std::cout << "Overlay semantic snapshot harness: " << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
