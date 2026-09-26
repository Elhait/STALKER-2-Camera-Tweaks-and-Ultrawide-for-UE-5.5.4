#include "camera_state_view.hpp"

#include "localization_keys.hpp"
#include "../camera/camera_state_snapshot.hpp"
#include "../config/feature_config.hpp"

#include <imgui.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>

namespace overlay
{
    namespace
    {
        std::string Tr(const LocalizationManager& i18n, loc::Key key)
        {
            return i18n.Text(key);
        }

        std::string Tr(const LocalizationManager& i18n, loc::Key key,
            const TextArguments& arguments)
        {
            return i18n.Format(key, arguments);
        }

        std::string Hex(const void* value)
        {
            char buffer[32]{};
            std::snprintf(buffer, sizeof(buffer), "0x%llx",
                static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(value)));
            return buffer;
        }

        void DrawCameraStateField(const LocalizationManager& i18n, loc::Key label,
            const char* value)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            const auto localizedLabel = Tr(i18n, label);
            ImGui::TextDisabled("%s", localizedLabel.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::TextWrapped("%s", value);
        }

        bool BeginCameraStateFields(const char* id)
        {
            const float labelWidth = ImGui::GetContentRegionAvail().x * 0.34f;
            if (!ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingFixedFit |
                ImGuiTableFlags_NoSavedSettings))
                return false;
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed,
                labelWidth);
            ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
            return true;
        }

        std::string CameraStateFloat(const LocalizationManager& i18n, float value,
            bool valid)
        {
            if (!valid || !std::isfinite(value))
                return Tr(i18n, loc::common::UnavailableValue);
            char formatted[48]{};
            std::snprintf(formatted, sizeof(formatted), "%.4f",
                static_cast<double>(value));
            return formatted;
        }

        std::string CameraStateBoolean(const LocalizationManager& i18n, bool value)
        {
            return Tr(i18n, value ? loc::common::Yes : loc::common::No);
        }

        std::string CameraStateSource(const LocalizationManager& i18n, bool valid,
            std::uintptr_t source)
        {
            return valid ? Hex(reinterpret_cast<const void*>(source))
                : Tr(i18n, loc::common::UnavailableValue);
        }

        std::string CameraStateCounter(const LocalizationManager& i18n, bool valid,
            std::uint64_t value)
        {
            return valid ? std::to_string(value)
                : Tr(i18n, loc::camera_state::UnavailableCounter,
                    {{"value", std::to_string(value)}});
        }
    }

    void DrawCameraStateView(const LocalizationManager& i18n,
        const plugin::OverlaySemanticSnapshot& semantic)
    {
        if (!semantic.cameraStateValid) {
            const auto unavailable = Tr(i18n, loc::common::Unavailable);
            const auto waiting = Tr(i18n, loc::camera_state::Waiting);
            ImGui::TextDisabled("%s", unavailable.c_str());
            ImGui::TextWrapped("%s", waiting.c_str());
            return;
        }

        const auto& state = semantic.cameraState;

        const auto presentationHeading = Tr(i18n, loc::section::Presentation);
        ImGui::SeparatorText(presentationHeading.c_str());
        if (!BeginCameraStateFields("camera_state_presentation")) return;
        DrawCameraStateField(i18n, loc::camera_state::State,
            camera::PresentationStateName(state.presentation.state));
        DrawCameraStateField(i18n, loc::camera_state::Provenance,
            camera::EvidenceProvenanceName(state.presentation.provenance));
        const std::string presentationEpoch = CameraStateCounter(i18n,
            state.presentation.epochValid, state.presentation.epoch);
        DrawCameraStateField(i18n, loc::camera_state::Epoch, presentationEpoch.c_str());
        ImGui::EndTable();

        const auto gameplayHeading = Tr(i18n, loc::section::Gameplay);
        ImGui::SeparatorText(gameplayHeading.c_str());
        if (!BeginCameraStateFields("camera_state_gameplay")) return;
        std::string gameplayMode = Tr(i18n, loc::common::UnavailableValue);
        if (state.gameplayMode.valid)
            gameplayMode = config::GameplayModeName(
                static_cast<config::GameplayMode>(state.gameplayMode.value));
        DrawCameraStateField(i18n, loc::camera_state::Mode, gameplayMode.c_str());
        DrawCameraStateField(i18n, loc::camera_state::Provenance,
            camera::EvidenceProvenanceName(state.gameplayMode.provenance));
        const auto gameplayValid = CameraStateBoolean(i18n, state.gameplayMode.valid);
        DrawCameraStateField(i18n, loc::camera_state::Valid, gameplayValid.c_str());
        ImGui::EndTable();

        const auto zoomHeading = Tr(i18n, loc::section::Zoom);
        ImGui::SeparatorText(zoomHeading.c_str());
        if (!BeginCameraStateFields("camera_state_zoom")) return;
        DrawCameraStateField(i18n, loc::camera_state::Direction,
            camera::ZoomDirectionName(state.zoom.direction));
        const auto zoomActive = CameraStateBoolean(i18n, state.zoom.active);
        DrawCameraStateField(i18n, loc::camera_state::Active, zoomActive.c_str());
        const auto zoomValid = CameraStateBoolean(i18n, state.zoom.valid);
        DrawCameraStateField(i18n, loc::camera_state::EvidenceValid, zoomValid.c_str());
        const std::string zoomSequence = std::to_string(state.zoom.sequence);
        DrawCameraStateField(i18n, loc::camera_state::Sequence, zoomSequence.c_str());
        DrawCameraStateField(i18n, loc::camera_state::Provenance,
            camera::EvidenceProvenanceName(state.zoom.provenance));
        const std::string primaryWeight = CameraStateFloat(i18n,
            state.zoom.primaryWeight, state.zoom.valid);
        DrawCameraStateField(i18n, loc::camera_state::Primary, primaryWeight.c_str());
        const std::string secondaryWeight = CameraStateFloat(i18n,
            state.zoom.secondaryWeight, state.zoom.valid);
        DrawCameraStateField(i18n, loc::camera_state::Secondary, secondaryWeight.c_str());
        const std::string zoomSource = CameraStateSource(i18n,
            state.zoom.source.valid, state.zoom.source.value);
        DrawCameraStateField(i18n, loc::camera_state::Source, zoomSource.c_str());
        ImGui::EndTable();

        const auto dialogueHeading = Tr(i18n, loc::section::Dialogue);
        ImGui::SeparatorText(dialogueHeading.c_str());
        if (!BeginCameraStateFields("camera_state_dialogue")) return;
        DrawCameraStateField(i18n, loc::camera_state::State,
            camera::DialogueStateName(state.dialogue.state));
        DrawCameraStateField(i18n, loc::camera_state::Confidence,
            camera::EvidenceProvenanceName(state.dialogue.provenance));
        const auto policyValid = CameraStateBoolean(i18n,
            state.dialogue.activePolicyValid);
        DrawCameraStateField(i18n, loc::camera_state::PolicyValid, policyValid.c_str());
        const auto recoveryExcluded = CameraStateBoolean(i18n,
            state.dialogue.recoveryExclusionActive);
        DrawCameraStateField(i18n, loc::camera_state::RecoveryExcluded,
            recoveryExcluded.c_str());
        const std::string dialogueTarget = CameraStateFloat(i18n,
            state.dialogue.nativeTarget, state.dialogue.nativeTargetValid);
        DrawCameraStateField(i18n, loc::camera_state::Target, dialogueTarget.c_str());
        const std::string dialogueSource = CameraStateSource(i18n,
            state.dialogue.source.valid, state.dialogue.source.value);
        DrawCameraStateField(i18n, loc::camera_state::Source, dialogueSource.c_str());
        ImGui::EndTable();

        const auto fovHeading = Tr(i18n, loc::section::Fov);
        ImGui::SeparatorText(fovHeading.c_str());
        if (!BeginCameraStateFields("camera_state_fov")) return;
        const std::string gameplayFov = CameraStateFloat(i18n,
            state.evidence.retainedGameplayFov,
            state.evidence.retainedGameplayFovValid);
        DrawCameraStateField(i18n, loc::camera_state::GameplayFov, gameplayFov.c_str());
        const auto settingKnown = CameraStateBoolean(i18n,
            state.evidence.configuredGameplayFovKnown);
        DrawCameraStateField(i18n, loc::camera_state::SettingKnown, settingKnown.c_str());
        DrawCameraStateField(i18n, loc::camera_state::GameSource,
            camera::EvidenceProvenanceName(
                state.evidence.retainedGameplayFovProvenance));
        const std::string nativeFov = CameraStateFloat(i18n,
            state.evidence.nativeWriterFov,
            state.evidence.nativeWriterFovValid);
        DrawCameraStateField(i18n, loc::camera_state::Native, nativeFov.c_str());
        DrawCameraStateField(i18n, loc::camera_state::NativeSource,
            camera::EvidenceProvenanceName(
                state.evidence.nativeWriterFovProvenance));
        const std::string horPlusFov = CameraStateFloat(i18n,
            state.evidence.transformedFov,
            state.evidence.transformedFovValid);
        DrawCameraStateField(i18n, loc::camera_state::HorPlus, horPlusFov.c_str());
        DrawCameraStateField(i18n, loc::camera_state::HorPlusSource,
            camera::EvidenceProvenanceName(
                state.evidence.transformedFovProvenance));
        const std::string aspect = CameraStateFloat(i18n, state.evidence.aspect,
            state.evidence.aspectValid);
        DrawCameraStateField(i18n, loc::camera_state::Aspect, aspect.c_str());
        DrawCameraStateField(i18n, loc::camera_state::AspectSource,
            camera::EvidenceProvenanceName(
                state.evidence.aspectProvenance));
        char flags[16]{};
        std::snprintf(flags, sizeof(flags), "0x%02X",
            static_cast<unsigned>(state.evidence.flags));
        DrawCameraStateField(i18n, loc::camera_state::Flags, flags);
        ImGui::EndTable();

        const auto sourcesHeading = Tr(i18n, loc::section::Sources);
        ImGui::SeparatorText(sourcesHeading.c_str());
        if (!BeginCameraStateFields("camera_state_sources")) return;
        const std::string gameplayWriter = CameraStateSource(i18n,
            state.source.gameplayWriter.valid,
            state.source.gameplayWriter.value);
        DrawCameraStateField(i18n, loc::camera_state::GameplayWriter,
            gameplayWriter.c_str());
        ImGui::EndTable();

        const auto generationHeading = Tr(i18n, loc::section::Generation);
        ImGui::SeparatorText(generationHeading.c_str());
        if (!BeginCameraStateFields("camera_state_generation")) return;
        const std::string generationEpoch = CameraStateCounter(i18n,
            state.generation.presentationEpochValid,
            state.generation.presentationEpoch);
        DrawCameraStateField(i18n, loc::camera_state::PresentationEpoch,
            generationEpoch.c_str());
        const std::string eventSequence = CameraStateCounter(i18n,
            state.generation.eventSequenceValid,
            state.generation.eventSequence);
        DrawCameraStateField(i18n, loc::camera_state::EventSequence,
            eventSequence.c_str());
        ImGui::EndTable();
    }
}
