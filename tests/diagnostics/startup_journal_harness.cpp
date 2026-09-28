#include "../../src/diagnostics/startup_journal.hpp"

#include <cstdio>
#include <cstring>

int main()
{
    using namespace diagnostics::startup_journal;
    const bool first = MarkOnce(15, "COMPOSITION_SURFACE_READY",
        "full_draw_commit", 7);
    const auto afterFirst = next.load(std::memory_order_acquire);
    const bool duplicateSuppressed = !MarkOnce(15, "DUPLICATE", "", 0) &&
        next.load(std::memory_order_acquire) == afterFirst;
    const bool invalidIdRejected = !MarkOnce(OnceCapacity, "INVALID", "", 0) &&
        next.load(std::memory_order_acquire) == afterFirst;
    const auto& event = events[afterFirst - 1];
    const bool milestonePublished = event.published.load(std::memory_order_acquire) &&
        std::strcmp(event.name, "COMPOSITION_SURFACE_READY") == 0 && event.value == 7;
    const bool passed = first && duplicateSuppressed && invalidIdRejected &&
        milestonePublished;
    std::printf("bounded_composition_startup_milestone=%s\n",
        passed ? "PASS" : "FAIL");
    return passed ? 0 : 1;
}
