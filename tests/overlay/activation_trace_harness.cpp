#include "../../src/diagnostics/diagnostic_runtime.hpp"
#include "../../src/diagnostics/overlay_activation_trace.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    std::vector<std::string> events;

    void Collect(const std::string& line)
    {
        events.push_back(line);
    }

    void Check(bool value, const char* message)
    {
        if (!value) throw std::runtime_error(message);
    }
}

int main()
{
    try {
        using diagnostics::overlay_activation_trace::BeginRequest;
        using diagnostics::overlay_activation_trace::RecordStage;
        using diagnostics::overlay_activation_trace::Stage;

        diagnostics::SetEnabled(false);
        Check(BeginRequest(&Collect, true, true) == 0 && events.empty() &&
                diagnostics::overlay_activation_trace::CurrentRequest() == 0,
            "Diagnostic OFF does not allocate request identity or emit trace");

        diagnostics::SetEnabled(true);
        const auto request = BeginRequest(&Collect, true, true);
        Check(request != 0 && events.size() == 1 &&
                events[0].find("event=toggle_requested") != std::string::npos &&
                events[0].find("qpc=") != std::string::npos &&
                events[0].find("qpcFrequency=") != std::string::npos,
            "activation request is correlated with a monotonic QPC timestamp");
        Check(RecordStage(&Collect, request, Stage::VisibilityRequested,
                    "visibility_requested", true, true) &&
                RecordStage(&Collect, request, Stage::PresenterReadiness,
                    "presenter_readiness", true, true) &&
                RecordStage(&Collect, request, Stage::FrameBegin,
                    "frame_begin", true, true) &&
                RecordStage(&Collect, request, Stage::SemanticReadBegin,
                    "semantic_read_begin", true, true) &&
                RecordStage(&Collect, request, Stage::SemanticReadEnd,
                    "semantic_read_end", true, true, true) &&
                RecordStage(&Collect, request, Stage::RenderSubmitted,
                    "render_submitted", true, true) &&
                RecordStage(&Collect, request, Stage::FirstVisibleFrame,
                    "first_visible_frame", true, true),
            "one activation records every readiness and first-frame boundary");
        const auto count = events.size();
        Check(!RecordStage(&Collect, request, Stage::FrameBegin,
                    "frame_begin", true, true) && events.size() == count,
            "per-activation frame telemetry is bounded and deduplicated");

        diagnostics::SetEnabled(false);
        Check(RecordStage(&Collect, request, Stage::SemanticReadBegin,
                    "semantic_read_begin", true, true) == false &&
                events.size() == count &&
                diagnostics::overlay_activation_trace::CurrentRequest() == 0,
            "Diagnostic OFF disables trace reads/emission without altering overlay state");
        diagnostics::SetEnabled(true);
        const auto nextRequest = BeginRequest(&Collect, false, true);
        Check(nextRequest == request + 1 &&
                RecordStage(&Collect, nextRequest, Stage::VisibilityRequested,
                    "visibility_requested", false, true),
            "a later hide request receives a distinct correlation identity");
        diagnostics::SetEnabled(false);
    } catch (const std::exception&) {
        diagnostics::SetEnabled(false);
        return 1;
    }
    return 0;
}
