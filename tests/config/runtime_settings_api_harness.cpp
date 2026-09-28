#include "../../src/plugin/runtime_settings.hpp"

#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

namespace
{
    plugin::RuntimeSettingsSnapshot g_snapshot{};
    int g_persistCalls = 0;

    bool Check(bool condition, const char* name)
    {
        if (!condition) std::cerr << "FAIL: " << name << "\n";
        return condition;
    }

    plugin::RuntimeMutationResult Apply(
        const plugin::RuntimeSettingMutation& mutation, void* userData)
    {
        auto* count = static_cast<int*>(userData);
        ++*count;
        if (mutation.kind == plugin::RuntimeSettingKind::GameplayMode) {
            g_snapshot.gameplayMode = std::get<config::GameplayMode>(mutation.value);
            return {true, true};
        }
        if (mutation.kind == plugin::RuntimeSettingKind::OverlayLocaleCode) {
            g_snapshot.overlayLocaleCode = std::get<std::string>(mutation.value);
            const auto* locale = localization::FindLocaleDescriptor(
                g_snapshot.overlayLocaleCode);
            if (!locale) return {};
            g_snapshot.overlayFontSize = locale->initialFontSize;
            return {true, true};
        }
        if (mutation.kind == plugin::RuntimeSettingKind::OverlayFontSize) {
            const int size = std::get<int>(mutation.value);
            if (size < config::OverlayFontSizeMin || size > config::OverlayFontSizeMax)
                return {};
            g_snapshot.overlayFontSize = size;
            return {true, true};
        }
        return {true, false};
    }

    bool Snapshot(plugin::RuntimeSettingsSnapshot& snapshot, void*)
    { snapshot = g_snapshot; return true; }

    bool Persist(const plugin::RuntimeSettingMutation&, void*)
    { ++g_persistCalls; return true; }

    struct ConcurrentContext
    {
        int generation{};
        std::atomic<int> calls{};
        std::atomic<int> snapshotCalls{};
        std::atomic<int> semanticSnapshotCalls{};
        std::atomic<int> persistCalls{};
    };

    std::atomic<unsigned> g_observedGenerationMask{};

    void RecordGeneration(ConcurrentContext& context)
    {
        g_observedGenerationMask.fetch_or(1u << context.generation,
            std::memory_order_relaxed);
    }

    plugin::RuntimeMutationResult ConcurrentApply(
        const plugin::RuntimeSettingMutation&, void* userData)
    {
        auto* context = static_cast<ConcurrentContext*>(userData);
        RecordGeneration(*context);
        context->calls.fetch_add(1, std::memory_order_relaxed);
        return {true, context->generation == 1};
    }

    bool ConcurrentSnapshot(plugin::RuntimeSettingsSnapshot& snapshot, void* userData)
    {
        if (!userData) return false;
        auto* context = static_cast<ConcurrentContext*>(userData);
        RecordGeneration(*context);
        context->snapshotCalls.fetch_add(1, std::memory_order_relaxed);
        snapshot.gameplayEnabled = context->generation == 1;
        return true;
    }

    bool ConcurrentSemanticSnapshot(plugin::OverlaySemanticSnapshot& snapshot,
        void* userData)
    {
        if (!userData) return false;
        auto* context = static_cast<ConcurrentContext*>(userData);
        RecordGeneration(*context);
        context->semanticSnapshotCalls.fetch_add(1, std::memory_order_relaxed);
        snapshot.gameplayEnabled = {context->generation == 1, true,
            plugin::SemanticProvenance::RuntimeSettings,
            plugin::SemanticFreshness::AtRead};
        return true;
    }

    bool ConcurrentPersist(const plugin::RuntimeSettingMutation&, void* userData)
    {
        if (!userData) return false;
        auto* context = static_cast<ConcurrentContext*>(userData);
        RecordGeneration(*context);
        context->persistCalls.fetch_add(1, std::memory_order_relaxed);
        return context->generation == 1;
    }
}

int main()
{
    plugin::RuntimeSettingsApi api;
    int calls = 0;
    bool pass = true;
    pass &= Check(!api.Apply(plugin::RuntimeSettingMutation::Gameplay(
        config::GameplayMode::HorPlus)).accepted, "unconfigured_rejects");

    plugin::RuntimeSettingsSnapshot snapshot{};
    plugin::OverlaySemanticSnapshot semanticSnapshot{};
    pass &= Check(!api.Snapshot(snapshot) && !api.SemanticSnapshot(semanticSnapshot) &&
        !api.Persist(plugin::RuntimeSettingMutation::GameplayEnabled(false)),
        "all_consumers_fail_closed_before_publication");

    const plugin::RuntimeSettingsApi::CallbackBundle callbacks{
        &Apply, &calls, &Snapshot, nullptr, nullptr, nullptr, &Persist, nullptr};
    pass &= Check(api.Publish(callbacks), "complete_bundle_publishes_once");
    pass &= Check(!api.Publish({}), "repeated_publication_is_rejected");
    const auto accepted = api.Apply(plugin::RuntimeSettingMutation::Gameplay(
        config::GameplayMode::AspectRecalculation));
    pass &= Check(accepted.accepted && accepted.changed && calls == 1,
        "typed_mutation_reaches_handler");
    pass &= Check(api.Snapshot(snapshot) &&
        snapshot.gameplayMode == config::GameplayMode::AspectRecalculation,
        "coherent_snapshot_reads_effective_value");
    pass &= Check(!api.SemanticSnapshot(semanticSnapshot),
        "unavailable_semantic_reader_fails_closed_after_publication");
    pass &= Check(api.Persist(plugin::RuntimeSettingMutation::GameplayEnabled(false)) &&
        g_persistCalls == 1, "typed_persistence_reaches_handler");
    pass &= Check(api.Persist(plugin::RuntimeSettingMutation::GameplayHotkey(VK_F6)) &&
        g_persistCalls == 2, "hotkey_persistence_reaches_handler");
    pass &= Check(api.Apply(plugin::RuntimeSettingMutation::GameplayEnabled(false)).accepted,
        "typed_bool_mutation_is_accepted");
    pass &= Check(api.Apply(plugin::RuntimeSettingMutation::GameplayHotkey(VK_F6)).accepted,
        "integer_hotkey_mutation_is_accepted");
    pass &= Check(api.Apply(plugin::RuntimeSettingMutation::OverlayToggleHotkey(VK_HOME)).accepted,
        "overlay_toggle_hotkey_mutation_is_accepted");
    const auto language = api.Apply(plugin::RuntimeSettingMutation::LocaleCode("uk"));
    pass &= Check(language.accepted && language.changed && api.Snapshot(snapshot) &&
        snapshot.overlayLocaleCode == "uk" && snapshot.overlayFontSize == 15,
        "locale_code_mutation_applies_registry_initial_font_size");
    const auto fontSize = api.Apply(plugin::RuntimeSettingMutation::FontSize(20));
    pass &= Check(fontSize.accepted && fontSize.changed && api.Snapshot(snapshot) &&
        snapshot.overlayFontSize == 20 &&
        api.Persist(plugin::RuntimeSettingMutation::FontSize(20)),
        "font_size_mutation_snapshot_and_persistence_route");
    const auto cjkLocale = api.Apply(plugin::RuntimeSettingMutation::LocaleCode("ja"));
    pass &= Check(cjkLocale.accepted && cjkLocale.changed && api.Snapshot(snapshot) &&
        snapshot.overlayLocaleCode == "ja" && snapshot.overlayFontSize == 18,
        "locale_switch_selects_its_initial_font_size");
    pass &= Check(!api.Apply(plugin::RuntimeSettingMutation::FontSize(25)).accepted,
        "font_size_out_of_range_rejected");
    const plugin::RuntimeSettingMutation invalid{
        plugin::RuntimeSettingKind::GameplayMode,
        config::CinematicFovMode::NativeHorPlus};
    pass &= Check(!api.Apply(invalid).accepted && calls == 8,
        "mismatched_value_rejected");
    pass &= Check(!plugin::ShouldDispatchHotkeyAction(false, false, true, false, false) &&
        !plugin::ShouldDispatchHotkeyAction(true, true, true, false, false) &&
        !plugin::ShouldDispatchHotkeyAction(true, false, true, true, false) &&
        !plugin::ShouldDispatchHotkeyAction(true, false, true, false, true) &&
        plugin::ShouldDispatchHotkeyAction(true, false, true, false, false),
        "hotkey_policy_gate_and_capture_consumption");

    plugin::RuntimeSettingsApi concurrentApi;
    ConcurrentContext contexts[2]{{1}, {2}};
    std::atomic<bool> start{false};
    std::atomic<int> readyThreads{};
    std::atomic<int> invalidObservations{};
    // Competing bundles encode distinct generations in every callback; a reader
    // that combines callbacks/contexts from both bundles sets both mask bits.
    std::vector<std::thread> readers;
    for (int i = 0; i < 8; ++i) {
        readers.emplace_back([&] {
            readyThreads.fetch_add(1, std::memory_order_release);
            while (!start.load(std::memory_order_acquire)) {}
            for (int iteration = 0; iteration < 512; ++iteration) {
                const auto mutation = concurrentApi.Apply(
                    plugin::RuntimeSettingMutation::GameplayEnabled(true));
                if (mutation.accepted && mutation.changed !=
                    ((g_observedGenerationMask.load(std::memory_order_relaxed) & 2u) != 0))
                    invalidObservations.fetch_add(1, std::memory_order_relaxed);
                plugin::RuntimeSettingsSnapshot concurrentSnapshot{};
                concurrentApi.Snapshot(concurrentSnapshot);
                plugin::OverlaySemanticSnapshot concurrentSemantic{};
                concurrentApi.SemanticSnapshot(concurrentSemantic);
                concurrentApi.Persist(
                    plugin::RuntimeSettingMutation::GameplayEnabled(true));
            }
        });
    }
    std::atomic<int> successfulPublishers{};
    std::atomic<int> winningGeneration{};
    std::vector<std::thread> publishers;
    for (int i = 0; i < 2; ++i) {
        publishers.emplace_back([&, i] {
            readyThreads.fetch_add(1, std::memory_order_release);
            while (!start.load(std::memory_order_acquire)) {}
            auto& context = contexts[i];
            const plugin::RuntimeSettingsApi::CallbackBundle competingCallbacks{
                &ConcurrentApply, &context, &ConcurrentSnapshot, &context,
                &ConcurrentSemanticSnapshot, &context, &ConcurrentPersist, &context};
            if (concurrentApi.Publish(competingCallbacks)) {
                successfulPublishers.fetch_add(1, std::memory_order_relaxed);
                winningGeneration.store(context.generation, std::memory_order_release);
            }
        });
    }
    while (readyThreads.load(std::memory_order_acquire) != 10) {}
    start.store(true, std::memory_order_release);
    for (auto& reader : readers) reader.join();
    for (auto& publisher : publishers) publisher.join();
    plugin::RuntimeSettingsSnapshot concurrentSnapshot{};
    plugin::OverlaySemanticSnapshot concurrentSemantic{};
    const auto finalMutation = concurrentApi.Apply(
        plugin::RuntimeSettingMutation::GameplayEnabled(true));
    const bool finalSnapshot = concurrentApi.Snapshot(concurrentSnapshot);
    const bool finalSemantic = concurrentApi.SemanticSnapshot(concurrentSemantic);
    const bool finalPersist = concurrentApi.Persist(
        plugin::RuntimeSettingMutation::GameplayEnabled(true));
    const int winner = winningGeneration.load(std::memory_order_acquire);
    const bool winnerValid = winner == 1 || winner == 2;
    const unsigned expectedGenerationMask = winnerValid ? 1u << winner : 0u;
    pass &= Check(successfulPublishers.load(std::memory_order_relaxed) == 1 &&
        winnerValid,
        "exactly_one_competing_complete_bundle_is_published");
    pass &= Check(winnerValid && finalMutation.accepted &&
        finalMutation.changed == (winner == 1) &&
        finalSnapshot && concurrentSnapshot.gameplayEnabled == (winner == 1) &&
        finalSemantic && concurrentSemantic.gameplayEnabled.valid &&
        concurrentSemantic.gameplayEnabled.value == (winner == 1) &&
        finalPersist == (winner == 1),
        "all_published_callback_kinds_are_available");
    const auto& winningContext = contexts[winnerValid ? winner - 1 : 0];
    pass &= Check(invalidObservations.load(std::memory_order_relaxed) == 0 &&
        g_observedGenerationMask.load(std::memory_order_relaxed) == expectedGenerationMask &&
        winningContext.calls.load(std::memory_order_relaxed) > 0 &&
        winningContext.snapshotCalls.load(std::memory_order_relaxed) > 0 &&
        winningContext.semanticSnapshotCalls.load(std::memory_order_relaxed) > 0 &&
        winningContext.persistCalls.load(std::memory_order_relaxed) > 0,
        "concurrent_readers_observe_only_the_winning_complete_bundle");

    plugin::RuntimeSettingsApi missingCallbacksApi;
    pass &= Check(missingCallbacksApi.Publish({&Apply, &calls, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr}), "bundle_with_optional_missing_callbacks_publishes");
    pass &= Check(missingCallbacksApi.Apply(
        plugin::RuntimeSettingMutation::GameplayEnabled(true)).accepted &&
        !missingCallbacksApi.Snapshot(snapshot) &&
        !missingCallbacksApi.SemanticSnapshot(semanticSnapshot) &&
        !missingCallbacksApi.Persist(plugin::RuntimeSettingMutation::GameplayEnabled(false)),
        "missing_callbacks_remain_fail_closed");

    plugin::RuntimeSettingsApi throwingApi;
    throwingApi.Publish({
        [](const plugin::RuntimeSettingMutation&, void*) -> plugin::RuntimeMutationResult { throw 1; }, nullptr,
        [](plugin::RuntimeSettingsSnapshot&, void*) -> bool { throw 2; }, nullptr,
        [](plugin::OverlaySemanticSnapshot&, void*) -> bool { throw 3; }, nullptr,
        [](const plugin::RuntimeSettingMutation&, void*) -> bool { throw 4; }, nullptr});
    pass &= Check(!throwingApi.Apply(plugin::RuntimeSettingMutation::GameplayEnabled(true)).accepted &&
        !throwingApi.Snapshot(snapshot) && !throwingApi.SemanticSnapshot(semanticSnapshot) &&
        !throwingApi.Persist(plugin::RuntimeSettingMutation::GameplayEnabled(true)),
        "noexcept_api_contains_throwing_callbacks_without_terminate");
    std::cout << "Runtime settings API harness: " << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 1;
}
