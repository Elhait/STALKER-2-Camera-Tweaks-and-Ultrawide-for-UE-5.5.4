#include "../../src/plugin/camera_correction_gate.hpp"
#include "../../src/dialogue/dialogue_state.hpp"

#include <array>
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

namespace
{
    bool Check(bool value, const char* label)
    {
        if (!value) std::cerr << "FAIL: " << label << '\n';
        return value;
    }
}

int main()
{
    bool pass = true;
    plugin::CameraCorrectionGate gate;
    std::atomic<bool> enabled{true};
    std::atomic<std::uint64_t> epoch{1};
    std::atomic<unsigned> modeReads{};
    std::atomic<float> output{90.0f};
    std::atomic<bool> correctionLifecycleActive{};
    std::mutex rendezvousMutex;
    std::condition_variable rendezvous;
    bool correctionPaused = false;
    bool releaseCorrection = false;
    bool offAttempted = false;
    std::atomic<bool> offReturned{};

    std::thread correction([&] {
        auto operation = gate.Acquire();
        if (!enabled.load(std::memory_order_acquire)) return;
        ++modeReads;
        const unsigned selectedMode = 1;
        (void)selectedMode;
        {
            std::unique_lock rendezvousLock(rendezvousMutex);
            correctionPaused = true;
            rendezvous.notify_all();
            rendezvous.wait(rendezvousLock, [&] { return releaseCorrection; });
        }
        output.store(127.0f, std::memory_order_release);
        correctionLifecycleActive.store(true, std::memory_order_release);
    });

    {
        std::unique_lock rendezvousLock(rendezvousMutex);
        rendezvous.wait(rendezvousLock, [&] { return correctionPaused; });
    }
    std::thread disable([&] {
        {
            std::lock_guard rendezvousLock(rendezvousMutex);
            offAttempted = true;
            rendezvous.notify_all();
        }
        auto settingsCommit = gate.Acquire();
        enabled.store(false, std::memory_order_release);
        epoch.fetch_add(1, std::memory_order_acq_rel);
        correctionLifecycleActive.store(false, std::memory_order_release);
        offReturned = true;
    });
    {
        std::unique_lock rendezvousLock(rendezvousMutex);
        rendezvous.wait(rendezvousLock, [&] { return offAttempted; });
        pass &= Check(!offReturned.load(std::memory_order_acquire),
            "off_cannot_return_while_previously_admitted_correction_is_in_flight");
        releaseCorrection = true;
        rendezvous.notify_all();
    }
    correction.join();
    disable.join();
    pass &= Check(output.load() == 127.0f && offReturned.load() && epoch.load() == 2 &&
        !correctionLifecycleActive.load(),
        "in_flight_commit_linearizes_before_off_and_off_clears_lifecycle_before_return");

    // The next production-equivalent dispatch enters through the same gate.
    // OFF is terminal before any mode read, target calculation or mutation.
    output.store(91.0f, std::memory_order_release);
    {
        auto operation = gate.Acquire();
        if (enabled.load(std::memory_order_acquire)) {
            ++modeReads;
            output.store(127.0f, std::memory_order_release);
            correctionLifecycleActive.store(true, std::memory_order_release);
        }
    }
    pass &= Check(output.load() == 91.0f && modeReads.load() == 1 &&
        !correctionLifecycleActive.load(),
        "post_off_dispatch_is_native_and_does_not_read_mode_or_reactivate");

    // OFF-first interleaving: the hook begins only after publication.
    enabled.store(true, std::memory_order_release);
    {
        auto settingsCommit = gate.Acquire();
        enabled.store(false, std::memory_order_release);
        epoch.fetch_add(1, std::memory_order_acq_rel);
    }
    output.store(92.0f, std::memory_order_release);
    {
        auto operation = gate.Acquire();
        if (enabled.load(std::memory_order_acquire)) {
            ++modeReads;
            output.store(127.0f, std::memory_order_release);
        }
    }
    pass &= Check(output.load() == 92.0f && modeReads.load() == 1 && epoch.load() == 3,
        "off_published_before_dispatch_blocks_output_mutation");

    constexpr std::array<unsigned, 5> correctionModes{0, 1, 2, 3, 4};
    bool offModesInert = true;
    for (const auto mode : correctionModes) {
        output.store(93.0f, std::memory_order_release);
        auto operation = gate.Acquire();
        if (enabled.load(std::memory_order_acquire)) {
            ++modeReads;
            output.store(static_cast<float>(mode), std::memory_order_release);
        }
        offModesInert &= output.load() == 93.0f;
    }
    pass &= Check(offModesInert && modeReads.load() == 1,
        "all_mode_values_are_inert_while_enabled_is_off");

    dialogue::CorrectionActivation dialogueActivation;
    dialogueActivation.AcceptedStart(true);
    {
        auto settingsCommit = gate.Acquire();
        enabled.store(false, std::memory_order_release);
        epoch.fetch_add(1, std::memory_order_acq_rel);
        dialogueActivation.ModuleEnabledChanged(false);
    }
    {
        auto settingsCommit = gate.Acquire();
        enabled.store(true, std::memory_order_release);
        epoch.fetch_add(1, std::memory_order_acq_rel);
    }
    pass &= Check(!dialogueActivation.CanCorrect(true),
        "dialogue_off_then_on_does_not_rearm_current_lifecycle");
    dialogueActivation.AcceptedStart(true);
    pass &= Check(dialogueActivation.CanCorrect(true),
        "next_accepted_dialogue_start_uses_new_enabled_epoch");

    bool deferredRanAfterUnlock = false;
    bool deferredCouldReenter = false;
    {
        auto operation = gate.Acquire();
        pass &= Check(gate.DeferUntilReleased([&] {
            deferredRanAfterUnlock = true;
            auto reentrantOperation = gate.Acquire();
            deferredCouldReenter = true;
        }), "critical_section_log_work_is_deferred");
    }
    pass &= Check(deferredRanAfterUnlock && deferredCouldReenter,
        "deferred_work_runs_only_after_correction_gate_unlock");

    std::cout << (pass ? "PASS" : "FAIL") << ": camera correction gate\n";
    return pass ? 0 : 1;
}
