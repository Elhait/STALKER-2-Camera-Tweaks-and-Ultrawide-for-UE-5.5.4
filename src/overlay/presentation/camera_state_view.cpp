#include "camera_state_view.hpp"
#include "aspect_ratio_format.hpp"
#include "camera_integration.hpp"

#include "../../camera/camera_state_snapshot.hpp"
#include "../../config/feature_config.hpp"

#include <imgui.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>

namespace overlay
{
    namespace
    {
        constexpr const char* Unavailable = "Unavailable";

        std::string Hex(const void* value)
        {
            char buffer[32]{};
            std::snprintf(buffer, sizeof(buffer), "0x%llx",
                static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(value)));
            return buffer;
        }

        void DrawField(const char* label, const char* value)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() +
                ImGui::GetColumnWidth());
            ImGui::TextDisabled("%s", label);
            ImGui::PopTextWrapPos();
            ImGui::TableSetColumnIndex(1);
            ImGui::TextWrapped("%s", value);
        }

        bool BeginFields(const char* id)
        {
            const float labelWidth = ImGui::GetContentRegionAvail().x * 0.34f;
            if (!ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingFixedFit |
                ImGuiTableFlags_NoSavedSettings)) return false;
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
            ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
            return true;
        }

        std::string Number(float value, bool valid, const char* format = "%.4f")
        {
            if (!valid || !std::isfinite(value)) return Unavailable;
            char buffer[48]{};
            std::snprintf(buffer, sizeof(buffer), format, static_cast<double>(value));
            return buffer;
        }

        std::string Aspect(float value, bool valid)
        {
            if (!valid) return Unavailable;
            const auto formatted = FormatAspectRatioNine(value);
            return formatted ? *formatted : Unavailable;
        }

        const char* Boolean(bool value) { return value ? "Yes" : "No"; }

        std::string HexByte(std::uint8_t value, bool valid)
        {
            if (!valid) return Unavailable;
            char buffer[8]{};
            std::snprintf(buffer, sizeof(buffer), "0x%02X", static_cast<unsigned>(value));
            return buffer;
        }

        std::string Source(bool valid, std::uintptr_t value)
        {
            return valid ? Hex(reinterpret_cast<const void*>(value)) : Unavailable;
        }

        std::string Counter(bool valid, std::uint64_t value)
        {
            return valid ? std::to_string(value) : Unavailable;
        }

        const char* Freshness(camera::SnapshotFreshness value)
        {
            switch (value) {
            case camera::SnapshotFreshness::Current: return "Current";
            case camera::SnapshotFreshness::Retained: return "Retained";
            case camera::SnapshotFreshness::Historical: return "Historical";
            case camera::SnapshotFreshness::Unavailable: return Unavailable;
            }
            return "Unknown";
        }

        const char* Match(CameraValueMatch value)
        {
            switch (value) {
            case CameraValueMatch::Waiting: return "Waiting";
            case CameraValueMatch::Matched: return "Match";
            case CameraValueMatch::Mismatch: return "Mismatch";
            case CameraValueMatch::CannotAssess: return Unavailable;
            case CameraValueMatch::Unknown: return "Unknown";
            }
            return "Unknown";
        }

        const char* Overall(OverallIntegrationAssessment value)
        {
            switch (value) {
            case OverallIntegrationAssessment::Waiting: return "Waiting";
            case OverallIntegrationAssessment::Seamless: return "Seamless";
            case OverallIntegrationAssessment::Mismatch: return "Mismatch";
            case OverallIntegrationAssessment::CannotAssess: return Unavailable;
            case OverallIntegrationAssessment::Unknown: return "Unknown";
            }
            return "Unknown";
        }
    }

    void DrawCameraStateView(const plugin::OverlaySemanticSnapshot& semantic,
        const CameraIntegrationStatus& integration)
    {
        const auto& state = semantic.cameraState;
        if (!semantic.cameraStateValid) {
            ImGui::TextDisabled("%s", Unavailable);
            constexpr const char* waiting = "Waiting for the first full camera publication.";
            ImGui::TextWrapped("%s", waiting);
        }

        ImGui::SeparatorText("Presentation");
        if (!BeginFields("camera_state_presentation")) return;
        DrawField("Last Full Publication", camera::PresentationStateName(state.presentation.state));
        const char* coordinator = Unavailable;
        if (semantic.coordinator.valid) {
            switch (semantic.coordinator.value) {
            case plugin::OverlayCoordinatorState::Gameplay: coordinator = "Gameplay"; break;
            case plugin::OverlayCoordinatorState::CinematicActive: coordinator = "CinematicActive"; break;
            case plugin::OverlayCoordinatorState::CinematicExiting: coordinator = "CinematicExiting"; break;
            }
        }
        DrawField("Current Coordinator", coordinator);
        DrawField("Provenance", camera::EvidenceProvenanceName(state.presentation.provenance));
        DrawField("Freshness", Freshness(state.freshness.presentation));
        const auto epoch = Counter(state.presentation.epochValid, state.presentation.epoch);
        DrawField("Epoch", epoch.c_str());
        ImGui::EndTable();

        ImGui::SeparatorText("Gameplay");
        if (!BeginFields("camera_state_gameplay")) return;
        const char* gameplayMode = state.gameplayMode.valid
            ? config::GameplayModeName(static_cast<config::GameplayMode>(state.gameplayMode.value))
            : Unavailable;
        DrawField("Mode", gameplayMode);
        DrawField("Provenance", camera::EvidenceProvenanceName(state.gameplayMode.provenance));
        DrawField("Valid", Boolean(state.gameplayMode.valid));
        DrawField("Freshness", Freshness(state.freshness.gameplayMode));
        ImGui::EndTable();

        ImGui::SeparatorText("Zoom");
        if (!BeginFields("camera_state_zoom")) return;
        DrawField("Direction", camera::ZoomDirectionName(state.zoom.direction));
        DrawField("Active", Boolean(state.zoom.active));
        DrawField("Evidence Valid", Boolean(state.zoom.valid));
        const auto zoomSequence = std::to_string(state.zoom.sequence);
        DrawField("Sequence", zoomSequence.c_str());
        DrawField("Provenance", camera::EvidenceProvenanceName(state.zoom.provenance));
        DrawField("Freshness", Freshness(state.freshness.zoom));
        const auto primary = Number(state.zoom.primaryWeight, state.zoom.valid);
        const auto secondary = Number(state.zoom.secondaryWeight, state.zoom.valid);
        DrawField("Primary", primary.c_str());
        DrawField("Secondary", secondary.c_str());
        const auto zoomSource = Source(state.zoom.source.valid, state.zoom.source.value);
        DrawField("Source", zoomSource.c_str());
        ImGui::EndTable();

        ImGui::SeparatorText("Dialogue");
        if (!BeginFields("camera_state_dialogue")) return;
        DrawField("State", camera::DialogueStateName(state.dialogue.state));
        DrawField("Provenance", camera::EvidenceProvenanceName(state.dialogue.provenance));
        DrawField("Freshness", Freshness(state.freshness.dialogue));
        DrawField("Policy Valid", Boolean(state.dialogue.activePolicyValid));
        DrawField("Recovery Excluded", Boolean(state.dialogue.recoveryExclusionActive));
        const auto dialogueTarget = Number(state.dialogue.nativeTarget,
            state.dialogue.nativeTargetValid);
        DrawField("Target", dialogueTarget.c_str());
        const auto dialogueSource = Source(state.dialogue.source.valid, state.dialogue.source.value);
        DrawField("Source", dialogueSource.c_str());
        ImGui::EndTable();

        ImGui::SeparatorText("FOV");
        if (!BeginFields("camera_state_fov")) return;
        const auto gameplayFov = Number(integration.gameplayFov, integration.gameplayFovValid);
        DrawField("Gameplay FOV", gameplayFov.c_str());
        DrawField("Provenance", camera::EvidenceProvenanceName(state.evidence.gameplayFov.provenance));
        DrawField("Freshness", Freshness(integration.gameplayFovFreshness));
        const auto nativeFov = Number(integration.nativeFov, integration.nativeFovValid);
        DrawField("Native FOV", nativeFov.c_str());
        DrawField("Provenance", camera::EvidenceProvenanceName(state.evidence.nativeFov.provenance));
        DrawField("Freshness", Freshness(integration.nativeFovFreshness));
        const auto targetFov = Number(integration.horPlusTargetFov, integration.horPlusTargetFovValid);
        DrawField("HorPlus Target", targetFov.c_str());
        DrawField("Provenance", camera::EvidenceProvenanceName(state.evidence.horPlusTargetFov.provenance));
        DrawField("Freshness", Freshness(integration.horPlusTargetFovFreshness));
        const auto outputFov = Number(integration.currentOutputFov, integration.currentOutputFovValid);
        DrawField("Current Output FOV", outputFov.c_str());
        DrawField("Provenance", camera::EvidenceProvenanceName(state.evidence.currentOutputFov.provenance));
        DrawField("Freshness", Freshness(integration.currentOutputFovFreshness));
        const auto writerAspect = Aspect(state.evidence.aspect, state.evidence.aspectValid);
        DrawField("Writer Aspect", writerAspect.c_str());
        DrawField("Provenance", camera::EvidenceProvenanceName(state.evidence.aspectProvenance));
        const auto policyAspect = Aspect(state.evidence.gameplayPolicyAspect,
            state.evidence.gameplayPolicyAspectValid);
        DrawField("Policy Aspect", policyAspect.c_str());
        DrawField("Provenance", camera::EvidenceProvenanceName(
            state.evidence.gameplayPolicyAspectProvenance));
        DrawField("Writer Observation Freshness", Freshness(state.freshness.writerObservation));
        const auto viewportAspect = Aspect(state.evidence.currentViewportAspect,
            state.evidence.currentViewportAspectValid);
        DrawField("Viewport Aspect", viewportAspect.c_str());
        const auto viewportProvenance = state.evidence.currentViewportAspectValid
            ? camera::EvidenceProvenanceName(state.evidence.currentViewportAspectProvenance)
            : Unavailable;
        DrawField("Provenance", viewportProvenance);
        const auto flags = HexByte(state.evidence.flags,
            state.freshness.writerObservation == camera::SnapshotFreshness::Current);
        DrawField("Flags", flags.c_str());

        ImGui::EndTable();

        if (state.gameplayMode.valid && static_cast<config::GameplayMode>(
                state.gameplayMode.value) == config::GameplayMode::RuntimeCombine) {
            const auto& e = state.evidence;
            ImGui::SeparatorText("RuntimeCombine");
            if (!BeginFields("camera_state_runtime_combine")) return;
            const auto before = Aspect(e.runtimeCombineAspectBefore, e.runtimeCombineValid);
            const auto target = Aspect(e.runtimeCombineTargetAspect, e.runtimeCombineValid);
            const auto after = Aspect(e.runtimeCombineAspectAfter, e.runtimeCombineReadbackValid);
            DrawField("Aspect Before", before.c_str());
            DrawField("Target Aspect", target.c_str());
            DrawField("Aspect After", after.c_str());
            DrawField("Mismatch", e.runtimeCombineValid ? Boolean(e.runtimeCombineMismatch) : Unavailable);
            DrawField("Write Attempted", e.runtimeCombineValid ? Boolean(e.runtimeCombineWriteAttempted) : Unavailable);
            DrawField("Write Succeeded", e.runtimeCombineValid ? Boolean(e.runtimeCombineWriteSucceeded) : Unavailable);
            DrawField("Readback Valid", e.runtimeCombineValid ? Boolean(e.runtimeCombineReadbackValid) : Unavailable);
            DrawField("Readback Confirmed", e.runtimeCombineValid ? Boolean(e.runtimeCombineConfirmed) : Unavailable);
            DrawField("FOV Applied", e.runtimeCombineValid ? Boolean(e.runtimeCombineFovApplied) : Unavailable);
            const auto flagsBefore = HexByte(e.runtimeCombineFlagsBefore, e.runtimeCombineValid);
            const auto flagsAfter = HexByte(e.runtimeCombineFlagsAfter, e.runtimeCombineReadbackValid);
            DrawField("Flags Before", flagsBefore.c_str());
            DrawField("Flags After", flagsAfter.c_str());
            const auto inputFov = Number(e.runtimeCombineFovInput, e.runtimeCombineValid);
            const auto outputFovValue = Number(e.runtimeCombineFovOutput,
                e.runtimeCombineValid);
            DrawField("Input FOV", inputFov.c_str());
            DrawField("Output FOV", outputFovValue.c_str());
            ImGui::EndTable();
        }

        ImGui::SeparatorText("Transition Evidence");
        if (BeginFields("camera_state_transition_evidence")) {
            if (integration.transitionBoundaryObserved) {
                const bool active = semantic.coordinator.valid && semantic.coordinator.value ==
                    plugin::OverlayCoordinatorState::CinematicActive;
                DrawField("Evidence", active ? "Active" : "Last Validated");
                const auto generation = std::to_string(integration.transitionGeneration);
                DrawField("Generation", generation.c_str());
                const auto gameplayAspect = Aspect(integration.gameplayBoundaryAspect,
                    integration.gameplayBoundaryAspectValid);
                const auto cinematicAspect = Aspect(integration.cinematicEnterAspect,
                    integration.cinematicEnterAspectValid);
                DrawField("Gameplay Boundary Aspect", gameplayAspect.c_str());
                DrawField("Cinematic ENTER Aspect", cinematicAspect.c_str());
                const auto aspectDelta = Number(integration.aspectDelta,
                    integration.aspectDeltaValid, "%.5f");
                DrawField("Aspect Delta", aspectDelta.c_str());
                DrawField("Aspect Match", Match(integration.aspect));
                const auto gameplayBoundaryFov = Number(integration.gameplayBoundaryFov,
                    integration.gameplayBoundaryFovValid);
                const auto cinematicEnterFov = Number(integration.cinematicEnterFov,
                    integration.cinematicEnterFovValid);
                DrawField("Gameplay Boundary FOV", gameplayBoundaryFov.c_str());
                DrawField("Cinematic ENTER FOV", cinematicEnterFov.c_str());
                const auto fovDelta = Number(integration.fovDelta,
                    integration.fovDeltaValid, "%.3f°");
                DrawField("FOV Delta", fovDelta.c_str());
                DrawField("FOV Match", Match(integration.fov));
                DrawField("Camera Transition", Overall(integration.overall));
            } else {
                DrawField("Evidence", Unavailable);
            }
            ImGui::EndTable();
        }

        ImGui::SeparatorText("Sources");
        if (!BeginFields("camera_state_sources")) return;
        const auto gameplayWriter = Source(state.source.gameplayWriter.valid,
            state.source.gameplayWriter.value);
        DrawField("Gameplay Writer", gameplayWriter.c_str());
        DrawField("Writer Observation Freshness", Freshness(state.freshness.writerObservation));
        ImGui::EndTable();

        ImGui::SeparatorText("Generation");
        if (!BeginFields("camera_state_generation")) return;
        const auto presentationEpoch = Counter(state.generation.presentationEpochValid,
            state.generation.presentationEpoch);
        const auto eventSequence = Counter(state.generation.eventSequenceValid,
            state.generation.eventSequence);
        DrawField("Presentation Epoch", presentationEpoch.c_str());
        DrawField("Event Sequence", eventSequence.c_str());
        ImGui::EndTable();
    }
}
