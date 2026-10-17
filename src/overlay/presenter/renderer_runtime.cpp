#include "renderer_runtime.hpp"
#include "presenter_runtime_policy.hpp"
#include "../optional_overlay_boundary.hpp"
#include "../../diagnostics/diagnostic_runtime.hpp"
#include "../../diagnostics/startup_journal.hpp"

#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <imgui_internal.h>
#include "../input/input_state.hpp"
#include "../localization/game_language_reader.hpp"
#include "../localization/localization_font.hpp"
#include "../presentation/selector_tooltip_state.hpp"
#include "../presentation/setting_tooltip_content.hpp"
#include "../presentation/selector_documentation_view.hpp"
#include "../presentation/tooltip_row_layout.hpp"
#include "../presentation/camera_state_view.hpp"
#include "../presentation/aspect_ratio_format.hpp"
#include "../presentation/overlay_layout_metrics.hpp"
#include "../presentation/placement_config.hpp"
#ifdef OVERLAY_SETTINGS_FRONTEND
#include "../presentation/hotkey_binding_presentation.hpp"
#endif
#ifdef OVERLAY_FONT_PLAYGROUND_EXPERIMENT
#include "font_playground_experiment.hpp"
#endif
#ifdef OVERLAY_FONT_RASTERIZATION_EXPERIMENT
#include "localization_font_rasterization_experiment.hpp"
#endif

extern "C" void PublishOverlayPopupState(bool open) noexcept;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND window, UINT message, WPARAM wParam, LPARAM lParam);

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <chrono>
#include <cstdio>
#include <cwchar>
#include <filesystem>
#include <initializer_list>
#include <limits>
#ifdef OVERLAY_SETTINGS_FRONTEND
#include "../../plugin/runtime.hpp"
#include "../../config/feature_config.hpp"
#include "../presentation/camera_integration.hpp"
#include "../presentation/feature_presentation.hpp"
#endif
#include <string>
#include <string_view>

namespace overlay
{
    namespace
    {
        constexpr std::string_view kInlineHotkeyToken = "__OVERLAY_HOTKEY_BADGE__";

        bool CloseActiveComboForEscapeRequest()
        {
            if (!GetInputState().ConsumeEscapePopupDismissRequest())
                return false;
            ImGui::CloseCurrentPopup();
            return true;
        }

        void DrawInlineHotkeyMessage(std::string_view message,
            std::string_view keyToken, std::string_view keyLabel)
        {
            const auto keyPosition = message.find(keyToken);
            if (keyPosition == std::string_view::npos) {
                ImGui::TextWrapped("%.*s", static_cast<int>(message.size()), message.data());
                return;
            }

            const std::string beforeKey(message.substr(0, keyPosition));
            const std::string afterKey(message.substr(keyPosition + keyToken.size()));
            if (!beforeKey.empty()) {
                ImGui::TextWrapped("%s", beforeKey.c_str());
                ImGui::SameLine(0.0f, 4.0f);
            }
            const auto punctuationBytes = inline_hotkey_layout::
                LeadingPunctuationBytes(afterKey);
            const std::string keyText = std::string(keyLabel) +
                afterKey.substr(0, punctuationBytes);
            const auto tail = inline_hotkey_layout::TrimLeadingWhitespace(
                std::string_view(afterKey).substr(punctuationBytes));
            const float keyGroupWidth = ImGui::CalcTextSize(keyText.c_str()).x;
            if (!beforeKey.empty() &&
                keyGroupWidth > ImGui::GetContentRegionAvail().x)
                ImGui::NewLine();
            ImGui::TextColored(ImVec4(0.18f, 0.78f, 1.0f, 1.0f), "%s",
                keyText.c_str());
            if (!tail.empty()) {
                ImGui::SameLine(0.0f, 4.0f);
                ImGui::TextWrapped("%.*s", static_cast<int>(tail.size()), tail.data());
            }
        }

        void DrawNotificationHeader(std::string_view title, float alpha)
        {
            const ImVec2 windowPos = ImGui::GetWindowPos();
            const float headerHeight = ImGui::GetTextLineHeight() +
                ImGui::GetStyle().FramePadding.y * 2.0f;
            ImGui::GetWindowDrawList()->AddRectFilled(windowPos,
                ImVec2(windowPos.x + ImGui::GetWindowWidth(),
                    windowPos.y + headerHeight),
                ImGui::GetColorU32(ImGuiCol_TitleBgActive, alpha));
            ImGui::SetCursorScreenPos(ImVec2(windowPos.x +
                ImGui::GetStyle().FramePadding.x, windowPos.y +
                ImGui::GetStyle().FramePadding.y));
            ImGui::PushStyleColor(ImGuiCol_Text,
                ImVec4(0.96f, 0.98f, 1.0f, alpha));
            ImGui::TextUnformatted(title.data(), title.data() + title.size());
            ImGui::PopStyleColor();
            ImGui::SetCursorScreenPos(ImVec2(windowPos.x +
                ImGui::GetStyle().WindowPadding.x, windowPos.y + headerHeight +
                ImGui::GetStyle().FramePadding.y));
        }

        void OverlayModulePathAnchor() noexcept
        {
        }

        std::filesystem::path OverlayModulePath()
        {
            HMODULE module{};
            if (!GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&OverlayModulePathAnchor), &module))
                return {};

            wchar_t modulePath[MAX_PATH]{};
            const DWORD length = GetModuleFileNameW(module, modulePath, MAX_PATH);
            if (length == 0 || length >= MAX_PATH) return {};
            return std::filesystem::path(modulePath);
        }

        HMODULE OverlayResourceModule()
        {
            HMODULE module{};
            return GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&OverlayModulePathAnchor), &module) ? module : nullptr;
        }

        ImVec2 ClampOverlayWindowPosition(const ImVec2& position,
            const ImVec2& windowSize, const ImGuiViewport* viewport,
            float dpiScale) noexcept
        {
            if (!viewport) return position;
            const auto clamped = layout_metrics::ClampPosition(
                {position.x, position.y}, {viewport->Pos.x, viewport->Pos.y},
                {viewport->Size.x, viewport->Size.y},
                {windowSize.x, windowSize.y}, dpiScale);
            return ImVec2(clamped.x, clamped.y);
        }

#ifdef OVERLAY_SETTINGS_FRONTEND
        const std::string Tr(const LocalizationManager& manager, loc::Key key)
        { return manager.Text(key); }

        const std::string Tr(const LocalizationManager& manager, loc::Key key,
            const TextArguments& arguments)
        { return manager.Format(key, arguments); }

        bool DrawCameraStateToggle(const LocalizationManager& localization,
            bool expanded)
        {
            const std::string label = Tr(localization,
                loc::camera_state::DetailsToggleLabel);
            const std::string action = Tr(localization, expanded
                ? loc::camera_state::DetailsToggleClose
                : loc::camera_state::DetailsToggleOpen);
            const ImGuiStyle& style = ImGui::GetStyle();
            const float height = ImGui::GetFrameHeight();
            const ImVec2 size(ImGui::GetContentRegionAvail().x, height);
            const bool clicked = ImGui::InvisibleButton("camera_state_details_toggle",
                size);
            const ImVec2 minimum = ImGui::GetItemRectMin();
            const ImVec2 maximum = ImGui::GetItemRectMax();
            const bool hovered = ImGui::IsItemHovered();
            const bool active = ImGui::IsItemActive();
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            const ImU32 background = active ? IM_COL32(48, 82, 119, 245)
                : expanded ? IM_COL32(43, 70, 102, 235)
                : hovered ? IM_COL32(39, 62, 89, 230)
                : IM_COL32(34, 52, 75, 215);
            const ImU32 border = hovered || active ? IM_COL32(92, 151, 210, 255)
                : IM_COL32(68, 103, 143, 230);
            drawList->AddRectFilled(minimum, maximum, background,
                style.FrameRounding);
            drawList->AddRect(minimum, maximum, border, style.FrameRounding,
                ImDrawFlags_None, 1.0f);

            const float centerY = (minimum.y + maximum.y) * 0.5f;
            const float iconLeft = minimum.x + style.FramePadding.x + 2.0f;
            const float iconTop = centerY - 6.0f;
            const float iconRight = iconLeft + 13.0f;
            const float iconBottom = centerY + 6.0f;
            const ImU32 iconColor = IM_COL32(166, 202, 239, 255);
            drawList->AddRect(ImVec2(iconLeft, iconTop),
                ImVec2(iconRight, iconBottom), iconColor, 1.5f, ImDrawFlags_None,
                1.2f);
            const float dividerX = expanded ? iconRight - 4.0f : iconLeft + 4.0f;
            drawList->AddLine(ImVec2(dividerX, iconTop + 1.5f),
                ImVec2(dividerX, iconBottom - 1.5f), iconColor, 1.2f);
            if (expanded) {
                drawList->AddLine(ImVec2(iconLeft + 3.0f, centerY),
                    ImVec2(iconLeft + 8.0f, centerY), iconColor, 1.2f);
                drawList->AddLine(ImVec2(iconLeft + 3.0f, centerY),
                    ImVec2(iconLeft + 5.5f, centerY - 2.5f), iconColor, 1.2f);
                drawList->AddLine(ImVec2(iconLeft + 3.0f, centerY),
                    ImVec2(iconLeft + 5.5f, centerY + 2.5f), iconColor, 1.2f);
            } else {
                drawList->AddLine(ImVec2(iconLeft + 5.0f, centerY),
                    ImVec2(iconLeft + 10.0f, centerY), iconColor, 1.2f);
                drawList->AddLine(ImVec2(iconLeft + 10.0f, centerY),
                    ImVec2(iconLeft + 7.5f, centerY - 2.5f), iconColor, 1.2f);
                drawList->AddLine(ImVec2(iconLeft + 10.0f, centerY),
                    ImVec2(iconLeft + 7.5f, centerY + 2.5f), iconColor, 1.2f);
            }

            const ImU32 labelColor = ImGui::GetColorU32(ImGuiCol_Text);
            const ImU32 actionColor = ImGui::GetColorU32(hovered || active
                ? ImVec4(0.96f, 0.98f, 1.0f, 1.0f)
                : ImVec4(0.70f, 0.82f, 0.96f, 1.0f));
            const ImVec2 labelSize = ImGui::CalcTextSize(label.c_str());
            const ImVec2 actionSize = ImGui::CalcTextSize(action.c_str());
            const float textY = centerY - ImGui::GetTextLineHeight() * 0.5f;
            const float labelX = iconRight + style.ItemInnerSpacing.x;
            drawList->AddText(ImVec2(labelX, textY), labelColor, label.c_str());
            const float actionRight = maximum.x - style.FramePadding.x - 12.0f;
            const float actionX = (std::max)(labelX + labelSize.x +
                style.ItemSpacing.x, actionRight - actionSize.x);
            drawList->AddText(ImVec2(actionX, textY), actionColor, action.c_str());
            const float arrowX = maximum.x - style.FramePadding.x - 3.0f;
            const float arrowDirection = expanded ? -1.0f : 1.0f;
            drawList->AddLine(ImVec2(arrowX - arrowDirection * 3.0f,
                centerY - 3.0f), ImVec2(arrowX, centerY), actionColor, 1.2f);
            drawList->AddLine(ImVec2(arrowX, centerY),
                ImVec2(arrowX - arrowDirection * 3.0f, centerY + 3.0f),
                actionColor, 1.2f);
            return clicked;
        }

        float IntrinsicOptionWidth(std::string_view option)
        {
            const ImGuiStyle& style = ImGui::GetStyle();
            return ImGui::CalcTextSize(option.data(),
                option.data() + option.size()).x +
                style.FramePadding.x * 2.0f + ImGui::GetFrameHeight();
        }

        float IntrinsicComboWidth(const char* const* items, int count)
        {
            float width = 0.0f;
            for (int index = 0; index < count; ++index)
                width = (std::max)(width, IntrinsicOptionWidth(items[index]));
            return width;
        }

        template <typename Value>
        float IntrinsicComboWidth(std::span<const setting_tooltip_content::SelectorOptionBinding<Value>> options)
        {
            float width = 0.0f;
            for (const auto& option : options)
                width = (std::max)(width,
                    IntrinsicOptionWidth(option.documentation->canonicalName));
            return width;
        }

        template <typename Value>
        struct SelectorComboResult
        {
            bool valueChanged{};
            Value value{};
            SelectorInteractionState interaction{};
        };

        template <typename Value>
        SelectorComboResult<Value> DrawSelectorCombo(const char* id, Value selectedValue,
            std::span<const setting_tooltip_content::SelectorOptionBinding<Value>> options)
        {
            int selectedIndex = -1;
            for (std::size_t index = 0; index < options.size(); ++index) {
                if (options[index].value == selectedValue) {
                    selectedIndex = static_cast<int>(index);
                    break;
                }
            }
            const char* preview = selectedIndex >= 0
                ? options[static_cast<std::size_t>(selectedIndex)].documentation->canonicalName.data()
                : nullptr;
            // Capture the Combo item's interaction immediately: BeginCombo may
            // open its popup and later option/group rendering replaces ImGui's
            // LastItemData. BeginCombo's result here means its popup body was
            // successfully begun in this rendering path, not a generic popup query.
            const bool popupOpen = ImGui::BeginCombo(id, preview);
            const SelectorInteractionState interaction{
                ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip),
                ImGui::IsItemActive(),
                popupOpen
            };
            if (!popupOpen)
                return {false, selectedValue, interaction};

            // Keyboard navigation stays disabled globally, so ImGui's default
            // NavUpdateCancelRequest cannot dismiss Combo popups with Escape.
            // Consume the window-procedure request only inside the active popup.
            bool valueChanged = false;
            if (!CloseActiveComboForEscapeRequest()) {
                ImGuiListClipper clipper;
                clipper.Begin(static_cast<int>(options.size()));
                if (selectedIndex >= 0)
                    clipper.IncludeItemByIndex(selectedIndex);
                while (clipper.Step()) {
                    for (int index = clipper.DisplayStart; index < clipper.DisplayEnd; ++index) {
                        const auto& option = options[static_cast<std::size_t>(index)];
                        ImGui::PushID(option.documentation->canonicalName.data());
                        const bool selected = option.value == selectedValue;
                        if (ImGui::Selectable(
                                option.documentation->canonicalName.data(), selected) && !selected) {
                            valueChanged = true;
                            selectedValue = option.value;
                        }
                        if (selected)
                            ImGui::SetItemDefaultFocus();
                        ImGui::PopID();
                    }
                }
            }
            ImGui::EndCombo();
            return {valueChanged, selectedValue, interaction};
        }

        ImVec4 FeatureStatusColor(FeatureStatusTone tone)
        {
            switch (tone) {
            case FeatureStatusTone::Positive: return ImVec4(0.35f, 0.82f, 0.45f, 1.0f);
            case FeatureStatusTone::Warning: return ImVec4(0.95f, 0.72f, 0.25f, 1.0f);
            case FeatureStatusTone::Error: return ImVec4(0.95f, 0.32f, 0.30f, 1.0f);
            case FeatureStatusTone::Neutral: return ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
            }
            return ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
        }

        void DrawParameterLabel(const LocalizationManager& i18n, loc::Key label)
        {
            const auto text = Tr(i18n, label);
            ImGui::TextUnformatted(text.c_str());
        }

        struct TooltipOption
        {
            std::string_view fixedName;
            loc::Key nameKey;
            loc::Key descriptionKey;
        };

        void DrawFeatureStatus(const LocalizationManager& i18n,
            const FeaturePresentation& status)
        {
            const auto label = Tr(i18n, status.label);
            ImGui::TextColored(FeatureStatusColor(status.tone), "%s", label.c_str());
            if (!status.detail.path.empty()) {
                const auto detail = Tr(i18n, status.detail);
                ImGui::TextWrapped("%s", detail.c_str());
            }
        }

        const char* RuntimeStatusValue(FeatureStatusKind kind)
        {
            switch (kind) {
            case FeatureStatusKind::CannotAssess: return "Unavailable";
            case FeatureStatusKind::Unavailable: return "Unavailable";
            case FeatureStatusKind::Disabled: return "Disabled";
            case FeatureStatusKind::Active: return "Active";
            case FeatureStatusKind::EnabledStateful: return "Enabled";
            case FeatureStatusKind::Ready: return "Ready";
            case FeatureStatusKind::Applying: return "Applying";
            case FeatureStatusKind::Starting: return "Starting";
            case FeatureStatusKind::Ending: return "Ending";
            case FeatureStatusKind::Waiting: return "Waiting";
            }
            return "Unknown";
        }

        bool BeginRuntimeTelemetryGrid(const char* id)
        {
            if (!ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingStretchProp |
                ImGuiTableFlags_NoSavedSettings))
                return false;
            ImGui::TableSetupColumn("field", ImGuiTableColumnFlags_WidthStretch, 0.46f);
            ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch, 0.54f);
            return true;
        }

        struct RuntimeTelemetryRowBounds
        {
            ImVec2 min{};
            ImVec2 max{};
            float contentBottom{};
        };

        RuntimeTelemetryRowBounds BeginRuntimeTelemetryRow(std::string_view field)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            const auto cursor = ImGui::GetCursorScreenPos();
            const auto cellPadding = ImGui::GetStyle().CellPadding;
            RuntimeTelemetryRowBounds bounds{
                ImVec2(cursor.x - cellPadding.x, cursor.y - cellPadding.y), {}, 0.0f};
            ImGui::TextUnformatted(field.data(), field.data() + field.size());
            bounds.contentBottom = ImGui::GetItemRectMax().y;
            ImGui::TableSetColumnIndex(1);
            const auto valueCursor = ImGui::GetCursorScreenPos();
            bounds.max.x = valueCursor.x + ImGui::GetContentRegionAvail().x +
                cellPadding.x;
            return bounds;
        }

        void FinishRuntimeTelemetryRow(RuntimeTelemetryRowBounds& bounds)
        {
            bounds.contentBottom = (std::max)(bounds.contentBottom,
                ImGui::GetItemRectMax().y);
            bounds.max.y = bounds.contentBottom + ImGui::GetStyle().CellPadding.y;
        }

        void DrawRuntimeTelemetryRow(std::string_view field, const char* value,
            bool valueUnavailable = false)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(field.data(), field.data() + field.size());
            ImGui::TableSetColumnIndex(1);
            if (valueUnavailable)
                ImGui::PushStyleColor(ImGuiCol_Text,
                    ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::TextWrapped("%s", value);
            if (valueUnavailable)
                ImGui::PopStyleColor();
        }

        void DrawRuntimeTelemetrySubheading(std::string_view heading)
        {
            ImGui::TableNextRow(ImGuiTableRowFlags_None, 5.0f);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.73f, 0.79f, 1.0f));
            ImGui::TextUnformatted(heading.data(), heading.data() + heading.size());
            ImGui::PopStyleColor();
            const auto headingRect = ImGui::GetItemRectMax();
            const auto windowPosition = ImGui::GetWindowPos();
            const auto contentMinLocal = ImGui::GetWindowContentRegionMin();
            const auto contentMaxLocal = ImGui::GetWindowContentRegionMax();
            const ImVec2 contentMin(windowPosition.x + contentMinLocal.x,
                windowPosition.y + contentMinLocal.y);
            const ImVec2 contentMax(windowPosition.x + contentMaxLocal.x,
                windowPosition.y + contentMaxLocal.y);
            auto lineColor = ImGui::GetStyleColorVec4(ImGuiCol_Border);
            lineColor.w *= 0.78f;
            const float lineY = (headingRect.y + ImGui::GetItemRectMin().y) * 0.5f;
            auto* drawList = ImGui::GetWindowDrawList();
            drawList->PushClipRect(contentMin, contentMax, false);
            drawList->AddLine(
                ImVec2(headingRect.x + ImGui::GetStyle().ItemSpacing.x, lineY),
                ImVec2(contentMax.x, lineY),
                ImGui::GetColorU32(lineColor), 1.0f);
            drawList->PopClipRect();
        }

        void DrawActiveStatus(const LocalizationManager& i18n, FeatureStatusTone tone,
            loc::Key detailKey, const std::string& activeValue)
        {
            const auto value = Tr(i18n, loc::transition::ActiveValue,
                {{"value", activeValue}});
            ImGui::TextColored(FeatureStatusColor(tone), "%s", value.c_str());
            const auto detail = Tr(i18n, detailKey);
            ImGui::TextWrapped("%s", detail.c_str());
        }

        template <std::size_t Count>
        void DrawOptionListTooltipContent(const LocalizationManager& i18n,
            const TooltipOption (&options)[Count])
        {
            tooltip_row_layout::DrawAlignedTooltipRows(Count,
                [&i18n, &options](std::size_t index) {
                    const auto& option = options[index];
                    return option.nameKey.path.empty()
                        ? std::string(option.fixedName) : Tr(i18n, option.nameKey);
                },
                [&i18n, &options](std::size_t index, float) {
                    const auto description = Tr(i18n, options[index].descriptionKey);
                    ImGui::TextWrapped("%s", description.c_str());
                },
                [](std::size_t) {});
        }

        template <std::size_t Count>
        void ShowOptionListTooltip(const LocalizationManager& i18n,
            const TooltipOption (&options)[Count])
        {
            if (!ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip) || !ImGui::BeginTooltip())
                return;
            DrawOptionListTooltipContent(i18n, options);
            ImGui::EndTooltip();
        }

        template <std::size_t Count>
        void ShowRuntimeRowTooltip(const LocalizationManager& i18n,
            const TooltipOption (&options)[Count],
            const RuntimeTelemetryRowBounds& bounds)
        {
            if (!ImGui::IsWindowHovered() ||
                !ImGui::IsMouseHoveringRect(bounds.min, bounds.max, false) ||
                !ImGui::BeginTooltip())
                return;
            DrawOptionListTooltipContent(i18n, options);
            ImGui::EndTooltip();
        }

        void ShowDirectTooltip(const LocalizationManager& i18n, loc::Key key)
        {
            const TooltipOption options[] = {{{}, {}, key}};
            ShowOptionListTooltip(i18n, options);
        }

        void DrawRuntimeSectionStatus(const LocalizationManager& i18n,
            std::string_view label, const FeaturePresentation& status)
        {
            auto bounds = BeginRuntimeTelemetryRow(label);
            const auto headingKey = label == "Gameplay Status"
                ? loc::tooltip::runtime::GameplayStatus
                : loc::tooltip::runtime::CinematicStatus;
            const TooltipOption options[] = {
                {{}, {}, headingKey},
                {{}, {}, status.detail},
            };
            ImGui::TextColored(FeatureStatusColor(status.tone), "%s",
                RuntimeStatusValue(status.kind));
            FinishRuntimeTelemetryRow(bounds);
            ShowRuntimeRowTooltip(i18n, options, bounds);
        }

        template <std::size_t Count>
        void DrawRuntimeTelemetryRow(std::string_view field, const char* value,
            const LocalizationManager& i18n, const TooltipOption (&options)[Count],
            bool valueUnavailable = false)
        {
            auto bounds = BeginRuntimeTelemetryRow(field);
            if (valueUnavailable)
                ImGui::PushStyleColor(ImGuiCol_Text,
                    ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::TextWrapped("%s", value);
            if (valueUnavailable)
                ImGui::PopStyleColor();
            FinishRuntimeTelemetryRow(bounds);
            ShowRuntimeRowTooltip(i18n, options, bounds);
        }

        void DrawRuntimeTelemetryRow(std::string_view field, const char* value,
            const LocalizationManager& i18n, loc::Key tooltip,
            bool valueUnavailable = false)
        {
            auto bounds = BeginRuntimeTelemetryRow(field);
            if (valueUnavailable)
                ImGui::PushStyleColor(ImGuiCol_Text,
                    ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::TextWrapped("%s", value);
            if (valueUnavailable)
                ImGui::PopStyleColor();
            FinishRuntimeTelemetryRow(bounds);
            const TooltipOption options[] = {{{}, {}, tooltip}};
            ShowRuntimeRowTooltip(i18n, options, bounds);
        }

        void ShowGameplayEnabledTooltip(const LocalizationManager& i18n)
        {
            static constexpr TooltipOption options[] = {
                {{}, loc::common::Enabled, loc::tooltip::gameplay::Enabled},
                {{}, loc::common::Disabled, loc::tooltip::gameplay::Disabled},
            };
            ShowOptionListTooltip(i18n, options);
        }

        void ShowCinematicsEnabledTooltip(const LocalizationManager& i18n)
        {
            ShowDirectTooltip(i18n, loc::tooltip::cinematics::Enabled);
        }

        void ShowDialogueEnabledTooltip(const LocalizationManager& i18n)
        {
            ShowDirectTooltip(i18n, loc::tooltip::dialogue::Enabled);
        }

        void ShowHotkeysEnabledTooltip(const LocalizationManager& i18n)
        {
            static constexpr TooltipOption options[] = {
                {{}, loc::common::Enabled, loc::tooltip::hotkeys::Enabled},
                {{}, loc::common::Disabled, loc::tooltip::hotkeys::Disabled},
            };
            ShowOptionListTooltip(i18n, options);
        }

        void ShowHotkeyBindingTooltipContent(const LocalizationManager& i18n,
            loc::Key semantics)
        {
            const TooltipOption options[] = {
                {{}, {}, semantics},
                {{}, {}, loc::tooltip::hotkeys::Change},
                {{}, {}, loc::tooltip::hotkeys::Rebind},
                {{}, {}, loc::tooltip::hotkeys::SingleKey},
            };
            ShowOptionListTooltip(i18n, options);
        }

        void ShowGameplayHotkeyTooltip(const LocalizationManager& i18n)
        { ShowHotkeyBindingTooltipContent(i18n, loc::tooltip::hotkeys::GameplayCycle); }
        void ShowCinematicAspectHotkeyTooltip(const LocalizationManager& i18n)
        { ShowHotkeyBindingTooltipContent(i18n, loc::tooltip::hotkeys::CinematicAspectCycle); }
        void ShowCinematicFovHotkeyTooltip(const LocalizationManager& i18n)
        { ShowHotkeyBindingTooltipContent(i18n, loc::tooltip::hotkeys::CinematicFovCycle); }
        void ShowDialogueZoomHotkeyTooltip(const LocalizationManager& i18n)
        { ShowHotkeyBindingTooltipContent(i18n, loc::tooltip::hotkeys::DialogueCycle); }
        void ShowOverlayToggleHotkeyTooltip(const LocalizationManager& i18n)
        {
            static constexpr TooltipOption options[] = {
                {{}, {}, loc::tooltip::overlay::AlwaysActive},
                {{}, {}, loc::tooltip::hotkeys::ChangeOverlay},
                {{}, {}, loc::tooltip::hotkeys::Rebind},
                {{}, {}, loc::tooltip::hotkeys::SingleKey},
            };
            ShowOptionListTooltip(i18n, options);
        }

        bool IsCapturingAction(HotkeyAction action)
        {
            return GetInputState().IsCapturing(action);
        }

        void DrawHotkeyBinding(const LocalizationManager& i18n, loc::Key label,
            HotkeyAction action, int key)
        {
            ImGui::BeginGroup();
            const auto localizedLabel = Tr(i18n, label);
            const std::string captureLabel = Tr(i18n, loc::rebind::PressKey);
            const char* buttonLabel = IsCapturingAction(action)
                ? captureLabel.c_str() : config::HotkeyName(key);
            const float buttonWidth = ImGui::CalcTextSize(buttonLabel).x +
                ImGui::GetStyle().FramePadding.x * 2.0f;
            std::string stableButtonLabel{buttonLabel};
            stableButtonLabel += "##hotkey_binding";
            ImGui::TextUnformatted(localizedLabel.c_str());
            ImGui::SameLine();
            ImGui::PushID(static_cast<int>(action));
            const bool clicked = ImGui::Button(stableButtonLabel.c_str(),
                ImVec2(buttonWidth, 0.0f));
            ImGui::PopID();
            if (clicked) {
                GetInputState().BeginRebind(action, key);
                plugin::SetHotkeyRebindCaptureActive(true);
            }
            ImGui::EndGroup();
            switch (action) {
            case HotkeyAction::GameplayMode: ShowGameplayHotkeyTooltip(i18n); break;
            case HotkeyAction::CinematicAspect: ShowCinematicAspectHotkeyTooltip(i18n); break;
            case HotkeyAction::CinematicFov: ShowCinematicFovHotkeyTooltip(i18n); break;
            case HotkeyAction::DialogueZoom: ShowDialogueZoomHotkeyTooltip(i18n); break;
            case HotkeyAction::OverlayToggle: ShowOverlayToggleHotkeyTooltip(i18n); break;
            }
        }

        FeatureStatusTone CameraTransitionTone(
            OverallIntegrationAssessment assessment)
        {
            switch (assessment) {
            case OverallIntegrationAssessment::Waiting:
                return FeatureStatusTone::Neutral;
            case OverallIntegrationAssessment::Seamless:
                return FeatureStatusTone::Positive;
            case OverallIntegrationAssessment::Mismatch:
                return FeatureStatusTone::Warning;
            case OverallIntegrationAssessment::CannotAssess:
                return FeatureStatusTone::Neutral;
            case OverallIntegrationAssessment::Unknown:
                return FeatureStatusTone::Neutral;
            }
            return FeatureStatusTone::Neutral;
        }

        loc::Key CameraTransitionDetail(
            OverallIntegrationAssessment assessment)
        {
            switch (assessment) {
            case OverallIntegrationAssessment::Waiting:
                return loc::transition::DetailExpectedSeamless;
            case OverallIntegrationAssessment::Seamless:
                return loc::transition::DetailAligned;
            case OverallIntegrationAssessment::Mismatch:
                return loc::transition::DetailNotAligned;
            case OverallIntegrationAssessment::CannotAssess:
                return loc::transition::DetailCannotAssess;
            case OverallIntegrationAssessment::Unknown:
                return loc::transition::DetailCannotAssess;
            }
            return loc::transition::DetailCannotAssess;
        }

        std::string ActiveDialoguePolicyLabel(config::DialogueZoomPolicy policy)
        {
            return config::DialogueZoomPolicyName(policy);
        }

        loc::Key CameraValueMatchLabel(CameraValueMatch assessment)
        {
            switch (assessment) {
            case CameraValueMatch::Waiting: return loc::common::Waiting;
            case CameraValueMatch::Matched: return loc::common::Yes;
            case CameraValueMatch::Mismatch: return loc::common::No;
            case CameraValueMatch::CannotAssess: return loc::common::CannotAssess;
            case CameraValueMatch::Unknown: return loc::common::CannotAssess;
            }
            return loc::common::CannotAssess;
        }

        loc::Key OverallIntegrationLabel(OverallIntegrationAssessment assessment)
        {
            switch (assessment) {
            case OverallIntegrationAssessment::Waiting:
                return loc::transition::Waiting;
            case OverallIntegrationAssessment::Seamless:
                return loc::transition::Aligned;
            case OverallIntegrationAssessment::Mismatch: return loc::transition::NotAligned;
            case OverallIntegrationAssessment::CannotAssess: return loc::common::CannotAssess;
            case OverallIntegrationAssessment::Unknown: return loc::common::CannotAssess;
            }
            return loc::common::CannotAssess;
        }

        bool ApplySetting(const plugin::RuntimeSettingMutation& mutation,
            const Renderer::LogFunction& logger)
        {
            auto& api = plugin::GetRuntimeSettingsApi();
            const auto result = api.ApplyAndPersist(mutation);
            if (!result.mutation.accepted) {
                if (logger) logger("OVERLAY_SETTING_REJECTED");
                return false;
            }
            if (!result.persisted) {
                if (logger) logger("OVERLAY_SETTING_PERSIST_FAILED");
                plugin::OverlayNotificationAction action =
                    plugin::OverlayNotificationAction::AnotherSetting;
                switch (mutation.kind) {
                case plugin::RuntimeSettingKind::GameplayEnabled:
                case plugin::RuntimeSettingKind::GameplayMode:
                    action = plugin::OverlayNotificationAction::GameplayMode; break;
                case plugin::RuntimeSettingKind::CinematicsEnabled:
                case plugin::RuntimeSettingKind::CinematicAspectPolicy:
                    action = plugin::OverlayNotificationAction::CinematicAspect; break;
                case plugin::RuntimeSettingKind::CinematicFovMode:
                    action = plugin::OverlayNotificationAction::CinematicFov; break;
                case plugin::RuntimeSettingKind::DialogueEnabled:
                case plugin::RuntimeSettingKind::DialogueZoomPolicy:
                    action = plugin::OverlayNotificationAction::DialogueZoom; break;
                case plugin::RuntimeSettingKind::OverlayToggleKey:
                    action = plugin::OverlayNotificationAction::OverlayToggle; break;
                default: break;
                }
                plugin::PublishOverlayNotification(
                    plugin::OverlayNotificationKind::PersistenceFailed, action,
                    {}, {}, plugin::OverlayNotificationStatus::Error);
            }
            return true;
        }

        loc::Key NotificationActionLabel(plugin::OverlayNotificationAction action)
        {
            switch (action) {
            case plugin::OverlayNotificationAction::GameplayMode: return loc::setting::GameplayMode;
            case plugin::OverlayNotificationAction::CinematicAspect: return loc::setting::CinematicAspect;
            case plugin::OverlayNotificationAction::CinematicFov: return loc::setting::CinematicFov;
            case plugin::OverlayNotificationAction::DialogueZoom: return loc::setting::DialogueZoom;
            case plugin::OverlayNotificationAction::OverlayToggle: return loc::setting::OverlayToggle;
            case plugin::OverlayNotificationAction::AnotherSetting: return loc::setting::Mode;
            }
            return loc::setting::Mode;
        }
#endif
    }

    void Renderer::Log(const std::string& message) const noexcept
    {
        if (logger_) TryOptionalOverlayLog(logger_, message);
    }

    void Renderer::Log(const char* message) const noexcept
    {
        if (logger_) TryOptionalOverlayLog(logger_, message);
    }

    void Renderer::LogStartupGate(const char* event,
        const char* reason) const noexcept
    {
        if (!diagnostics::Enabled() || !logger_ || !event || !reason) return;
        try {
            Log(std::string("STARTUP_GATE_") + event + " ts_ms=" +
                std::to_string(static_cast<std::uint64_t>(GetTickCount64())) +
                " reason=" + reason);
        } catch (...) {}
    }

    void Renderer::LogStartupGateEligibility() noexcept
    {
        if (!startupReadiness_.eligible() || startupGateEligibilityLogged_) return;
        startupGateEligibilityLogged_ = true;
        LogStartupGate("ELIGIBLE", startupReadiness_.gameDataObserved()
            ? "runtime_overlay_and_game_data_ready"
            : "runtime_overlay_ready_explicit_fallback");
    }

    namespace
    {
        void LogDiagnostic(const Renderer::LogFunction& logger,
            const std::string& message) noexcept
        {
            if (logger && diagnostics::Enabled())
                TryOptionalOverlayLog(logger, message);
        }

        void LogDiagnostic(const Renderer::LogFunction& logger,
            const char* message) noexcept
        {
            if (logger && diagnostics::Enabled())
                TryOptionalOverlayLog(logger, message);
        }
    }

    bool Renderer::Initialize(HWND gameWindow, HWND overlayWindow, UINT width,
        UINT height, UINT dpi) noexcept
    {
        bool initialized = false;
        const auto outcome = RunOptionalOverlayWork([&]() {
            initialized = InitializeImpl(gameWindow, overlayWindow, width,
                height, dpi);
        }, [this]() noexcept { FailAfterException(); });
        return outcome == OptionalOverlayWorkResult::Completed && initialized;
    }

    bool Renderer::RebindWindow(HWND gameWindow, UINT width, UINT height,
        UINT dpi) noexcept
    {
        bool rebound = false;
        const auto outcome = RunOptionalOverlayWork([&]() {
            if (!gameWindow || !IsWindow(gameWindow) || !overlayWindow_ ||
                !IsWindow(overlayWindow_) || !width || !height ||
                disabled() || !state_.cameraCoreReady())
                return;
            SurfaceUpdateToken token{};
            if (!state_.BeginSurfaceUpdate({width, height}, token)) return;
            if (!SetOverlayWindowVisibility(false)) {
                Disable("window_rebind_hide_failed");
                return;
            }
            overlayPlacement_.valid = false;
            DetachImGui();
            if (!ResetComposition()) {
                Disable("window_rebind_composition_detach_failed");
                return;
            }
            gameWindow_ = gameWindow;
            menuFrameAdmission_.Invalidate();
            inputBindingReady_ = false;
            hostActive_ = GetForegroundWindow() == gameWindow_;
            hostMinimized_ = IsIconic(gameWindow_) != FALSE;
            if (!ApplyWindowDpi(dpi)) {
                Disable("window_rebind_dpi_setup_failed");
                return;
            }
            if (!CreateGraphicsDevice() || !CreateCompositionTarget(overlayWindow_) ||
                !BuildImGui() || !CreateSurface(width, height, surface_) ||
                FAILED(visual_->SetContent(surface_.Get())) ||
                FAILED(compositionTarget_->SetRoot(visual_.Get())) ||
                !DrawImGuiSurface(surface_.Get(), width, height, nullptr,
                    {0, 0, static_cast<int>(width), static_cast<int>(height)}) ||
                !state_.PublishSurfaceUpdate(token, true, true)) {
                Disable("window_rebind_failed");
                return;
            }
            surfaceWidth_ = width;
            surfaceHeight_ = height;
            surfaceGeneration_ = state_.generation();
            compositionReady_ = true;
            visualAttached_ = true;
            LogCompositionAttachment();
            LogNotificationLifecycle("root_attach", "window_rebind_surface_commit");
            surfaceDirty_ = true;
            forceFullDamage_ = true;
            damageTracker_.Invalidate();
            firstFrameLogged_ = false;
            Log("OVERLAY_DCOMP_WINDOW_REBOUND generation=" +
                std::to_string(surfaceGeneration_));
            rebound = RenderImpl(surface_.Get(), width, height);
            if (rebound) rebound = StartPendingNotificationLifetimes();
        }, [this]() noexcept { FailAfterException(); });
        return outcome == OptionalOverlayWorkResult::Completed && rebound;
    }

    bool Renderer::InitializeImpl(HWND gameWindow, HWND overlayWindow,
        UINT width, UINT height, UINT dpi)
    {
        state_.MarkCameraCoreReady();
        startupReadiness_.MarkRuntimeInitialized();
        LogStartupGate("RUNTIME_READY", "camera_core_initialization_complete");
        LogStartupGate("WAITING", "overlay_presenter_and_input_binding");
        if (!gameWindow || !IsWindow(gameWindow) || !overlayWindow ||
            !IsWindow(overlayWindow) || !width || !height ||
            !state_.MarkWindowAvailable({width, height}))
            return false;
        gameWindow_ = gameWindow;
        inputBindingReady_ = false;
        overlayWindow_ = overlayWindow;
        hostActive_ = GetForegroundWindow() == gameWindow_;
        hostMinimized_ = IsIconic(gameWindow_) != FALSE;
        if (!ApplyWindowDpi(dpi)) {
            Disable("window_dpi_setup_failed");
            return false;
        }
        Log("OVERLAY_DCOMP_INIT_BEGIN");
        if (!CreateGraphicsDevice() || !CreateCompositionTarget(overlayWindow_) ||
            !BuildImGui()) {
            Disable("composition_initialization_failed");
            return false;
        }
        if (!CreateSurface(width, height, surface_) ||
            FAILED(visual_->SetContent(surface_.Get())) ||
            FAILED(compositionTarget_->SetRoot(visual_.Get()))) {
            Disable("composition_surface_create_failed");
            return false;
        }
        if (!DrawImGuiSurface(surface_.Get(), width, height, nullptr,
                {0, 0, static_cast<int>(width), static_cast<int>(height)})) {
            Disable("composition_initial_commit_failed");
            return false;
        }
        SurfaceUpdateToken token{};
        if (!state_.BeginSurfaceUpdate({width, height}, token) ||
            !state_.PublishSurfaceUpdate(token, true, true)) {
            Disable("composition_initial_generation_failed");
            return false;
        }
        surfaceWidth_ = width;
        surfaceHeight_ = height;
        surfaceGeneration_ = state_.generation();
        compositionReady_ = true;
        visualAttached_ = true;
        LogCompositionAttachment();
        LogNotificationLifecycle("root_attach", "initial_surface_commit");
        surfaceDirty_ = true;
        forceFullDamage_ = true;
        damageTracker_.Invalidate();
        PrepareStartupNotification();
        Log("OVERLAY_DCOMP_SURFACE_READY generation=" +
            std::to_string(surfaceGeneration_) + " width=" +
            std::to_string(width) + " height=" + std::to_string(height));
#if defined(OVERLAY_STARTUP_JOURNAL)
        diagnostics::startup_journal::MarkOnce(6, "COMPOSITION_SURFACE_READY",
            "full_draw_commit", surfaceGeneration_);
        diagnostics::startup_journal::MarkOnce(7, "RENDERER_ACTIVATED",
            "directcomposition_surface_committed", surfaceGeneration_);
        diagnostics::startup_journal::Flush();
#endif
        const bool rendered = RenderImpl(surface_.Get(), width, height);
        return rendered && StartPendingNotificationLifetimes();
    }

    void Renderer::SetInputBindingReady(HWND window) noexcept
    {
        if (!OwnsWindow(window)) return;
        inputBindingReady_ = true;
        ReconcileHostActivity();
        surfaceDirty_ = true;
        if (ready() && startupReadiness_.MarkOverlayReady())
            LogStartupGate("OVERLAY_READY",
                "presenter_and_game_input_binding_ready");
    }

    void Renderer::CompleteStartupGameDataProbe(bool readSucceeded) noexcept
    {
        const bool wasFallback = startupReadiness_.fallbackResolved();
        const bool changed = startupReadiness_.ResolveGameDataProbe(readSucceeded);
        if (readSucceeded && changed)
            LogStartupGate("GAME_DATA_OBSERVED", wasFallback
                ? "late_validated_interface_language_read"
                : "validated_interface_language_read");
        else if (!readSucceeded && changed)
            LogStartupGate("FALLBACK", "native_interface_language_read_failed");
        LogStartupGateEligibility();
        surfaceDirty_ = true;
    }

    bool Renderer::ResolveStartupGameDataProbeTimeout() noexcept
    {
        if (!startupReadiness_.ResolveProbeTimeout(GetTickCount64())) return false;
        plugin::RuntimeSettingsSnapshot settings{};
        if (plugin::GetRuntimeSettingsApi().Snapshot(settings) &&
            settings.overlayLocaleAuto && !settings.overlayLocaleAutoSynchronized)
            plugin::SetDetectedOverlayLocale("en");
        LogStartupGate("FALLBACK", "game_data_probe_timeout_3000ms");
        LogStartupGateEligibility();
        surfaceDirty_ = true;
        return true;
    }

    bool Renderer::CreateGraphicsDevice()
    {
        UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        D3D_FEATURE_LEVEL selected{};
        constexpr D3D_FEATURE_LEVEL levels[]{D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0};
        HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE,
            nullptr, flags, levels, static_cast<UINT>(std::size(levels)),
            D3D11_SDK_VERSION, device_.GetAddressOf(), &selected, nullptr);
        if (result == E_INVALIDARG) {
            result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE,
                nullptr, flags, &levels[1], 1, D3D11_SDK_VERSION,
                device_.GetAddressOf(), &selected, nullptr);
        }
        if (FAILED(result) || FAILED(device_.As(&dxgiDevice_))) return false;
        result = DCompositionCreateDevice2(dxgiDevice_.Get(),
            __uuidof(IDCompositionDesktopDevice),
            reinterpret_cast<void**>(compositionDevice_.GetAddressOf()));
        if (FAILED(result)) return false;
        Log("OVERLAY_DCOMP_DEVICE_READY featureLevel=" +
            std::to_string(static_cast<unsigned>(selected)));
#if defined(OVERLAY_STARTUP_JOURNAL)
        diagnostics::startup_journal::MarkOnce(5, "COMPOSITION_DEVICE_READY",
            "private_d3d11_device");
        diagnostics::startup_journal::Flush();
#endif
        return true;
    }

    bool Renderer::CreateCompositionTarget(HWND window)
    {
        if (!compositionDevice_ || !window || !IsWindow(window)) return false;
        HRESULT result = compositionDevice_->CreateTargetForHwnd(window, TRUE,
            compositionTarget_.GetAddressOf());
        if (FAILED(result)) {
            Log("OVERLAY_DCOMP_TOPMOST_TARGET_UNAVAILABLE; using non-topmost target");
            result = compositionDevice_->CreateTargetForHwnd(window, FALSE,
                compositionTarget_.GetAddressOf());
        }
        return SUCCEEDED(result) && SUCCEEDED(compositionDevice_->CreateVisual(
            visual_.GetAddressOf()));
    }

    bool Renderer::CreateSurface(UINT width, UINT height,
        Microsoft::WRL::ComPtr<IDCompositionSurface>& surface) noexcept
    {
        if (!compositionDevice_ || !width || !height ||
            width > 16384 || height > 16384)
            return false;
        return SUCCEEDED(compositionDevice_->CreateSurface(width, height,
            DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ALPHA_MODE_PREMULTIPLIED,
            surface.GetAddressOf()));
    }

    bool Renderer::ApplyWindowDpi(UINT dpi) noexcept
    {
        try {
        const UINT normalizedDpi = dpi ? dpi : 96u;
        if (normalizedDpi == windowDpi_) return true;
        windowDpi_ = normalizedDpi;
        dpiScale_ = static_cast<float>(windowDpi_) / 96.0f;
        if (ImGui::GetCurrentContext() && baseStyleCaptured_) {
            ImGui::GetStyle() = baseStyle_;
            ImGui::GetStyle().ScaleAllSizes(dpiScale_);
        }
        forceFullDamage_ = true;
        surfaceDirty_ = true;
        if (imguiReady_) {
            plugin::RuntimeSettingsSnapshot settings{};
            if (!plugin::GetRuntimeSettingsApi().Snapshot(settings)) return false;
            const auto* descriptor = localization::FindLocaleDescriptor(
                settings.overlayLocaleCode);
            const std::string_view profile = descriptor
                ? descriptor->fontProfileCode : std::string_view{"base"};
            if (!RebuildFontAtlas(settings.overlayFontSize, profile)) return false;
        }
        Log("OVERLAY_DPI_CHANGED dpi=" + std::to_string(windowDpi_) +
            " scale=" + std::to_string(dpiScale_) +
            " coordinate_contract=physical_client_pixels");
        return true;
        } catch (...) {
            return false;
        }
    }

    bool Renderer::RecoverGraphicsDevice(UINT width, UINT height) noexcept
    {
        try {
            if (!gameWindow_ || !IsWindow(gameWindow_) || !overlayWindow_ ||
                !IsWindow(overlayWindow_) || !width || !height ||
                !ImGui::GetCurrentContext())
                return false;
            auto& input = GetInputState();
            auto& inputBridge = GetInputEventBridge();
            const auto inputOwnershipBefore = CaptureInputOwnership(input,
                inputBridge);
            if (!inputOwnershipBefore.Coherent()) return false;
            imguiRenderer_.Shutdown(ImGui::GetIO().Fonts);
            if (!ResetComposition()) return false;
            if (!CreateGraphicsDevice() || !CreateCompositionTarget(overlayWindow_) ||
                !imguiRenderer_.Initialize(device_.Get(), ImGui::GetIO().Fonts) ||
                !CreateSurface(width, height, surface_) ||
                FAILED(visual_->SetContent(surface_.Get())) ||
                FAILED(compositionTarget_->SetRoot(visual_.Get())) ||
                !DrawImGuiSurface(surface_.Get(), width, height, nullptr,
                    {0, 0, static_cast<int>(width), static_cast<int>(height)}))
                return false;
            if (!InputOwnershipUnchanged(inputOwnershipBefore, input,
                    inputBridge) ||
                !state_.CompleteDeviceRecovery(true, {width, height}))
                return false;
            surfaceWidth_ = width;
            surfaceHeight_ = height;
            surfaceGeneration_ = state_.generation();
            compositionReady_ = true;
            visualAttached_ = true;
            LogCompositionAttachment();
            LogNotificationLifecycle("root_attach", "device_recovery_surface_commit");
            surfaceDirty_ = true;
            forceFullDamage_ = true;
            damageTracker_.Invalidate();
            Log("OVERLAY_DCOMP_DEVICE_RECOVERED generation=" +
                std::to_string(surfaceGeneration_));
            return true;
        } catch (...) {
            state_.CompleteDeviceRecovery(false, {});
            return false;
        }
    }

    bool Renderer::BuildImGui()
    {
        if (!device_ || !gameWindow_) return false;
        const auto resourceModule = OverlayResourceModule();
#ifdef OVERLAY_SETTINGS_FRONTEND
        std::string localeError;
        if (!localization_.Initialize(localization::CanonicalLocaleCode,
            resourceModule, localeError)) {
            Log("OVERLAY_CATALOGS_UNAVAILABLE: " + localeError);
            return false;
        }
#endif
        if (!ImGui::GetCurrentContext()) {
            ImGui::CreateContext();
            imguiContextCreated_ = true;
            ImGuiIO& io = ImGui::GetIO();
            // Every mod window uses NoSavedSettings; placement belongs to the
            // main INI. Avoid ImGui's otherwise automatic CRT FILE load/save.
            io.IniFilename = nullptr;
            int fontSizeSetting = config::OverlayFontSizeDefault;
#ifdef OVERLAY_SETTINGS_FRONTEND
            plugin::RuntimeSettingsSnapshot settings{};
            if (plugin::GetRuntimeSettingsApi().Snapshot(settings))
                fontSizeSetting = settings.overlayFontSize;
#endif
            const int fontSizePixels = presentation::EffectiveFontPixels(
                fontSizeSetting, windowDpi_);
#ifdef OVERLAY_FONT_PLAYGROUND_EXPERIMENT
            io.FontDefault = font_playground_experiment::AddSelectedFont(fontSizePixels);
#elif defined(OVERLAY_FONT_RASTERIZATION_EXPERIMENT)
            io.FontDefault = AddProggyVectorOnly(fontSizePixels);
#else
            std::string_view profileCode{"base"};
#ifdef OVERLAY_SETTINGS_FRONTEND
            if (plugin::GetRuntimeSettingsApi().Snapshot(settings)) {
                if (const auto* descriptor = localization::FindLocaleDescriptor(
                        settings.overlayLocaleCode))
                    profileCode = descriptor->fontProfileCode;
                localization_.SetLocale(settings.overlayLocaleCode);
                activeLocaleCode_ = settings.overlayLocaleCode;
            }
#endif
            io.FontDefault = AddLocalizationFont(fontSizePixels, profileCode);
            if (io.FontDefault && !AddLocalizationSelectorFonts(fontSizePixels))
                io.FontDefault = nullptr;
            activeFontProfileCode_ = profileCode;
#endif
            if (!io.FontDefault) {
                Log("OVERLAY_UI_FONT_UNAVAILABLE");
                return false;
            }
            if (!io.Fonts->Build()) return false;
            activeFontSizePixels_ = fontSizePixels;
            activeFontSetting_ = fontSizeSetting;
#ifdef OVERLAY_FONT_RASTERIZATION_EXPERIMENT
            activeFontRasterizationMode_ = font_rasterization_experiment::CurrentMode();
#endif
#ifdef OVERLAY_FONT_PLAYGROUND_EXPERIMENT
            activeFontChoice_ = font_playground_experiment::CurrentFontChoice();
#endif
        }
        if (!overlayPlacementLoaded_) {
            bool migrated = false;
            auto modulePath = OverlayModulePath();
            if (modulePath.empty()) {
                Log("OVERLAY_POSITION_LOAD_FAILED");
                overlayPlacementLoaded_ = true;
            } else {
                overlayConfigPath_ = modulePath.replace_filename(
                    L"STALKER2CameraTweaks.ini").wstring();
                const auto legacyPath = modulePath.replace_filename(
                    L"STALKER2CameraTweaksOverlay.ini").wstring();
                if (!placement_config::Load(overlayConfigPath_, legacyPath,
                        overlayPositionX_, overlayPositionY_, migrated))
                    Log("OVERLAY_POSITION_LOAD_FAILED");
                else if (migrated)
                    Log("OVERLAY_POSITION_MIGRATED_FROM_LEGACY_INI");
                overlayPlacementLoaded_ = true;
            }
        }
        ImGui::StyleColorsDark();
        baseStyle_ = ImGui::GetStyle();
        baseStyleCaptured_ = true;
        ImGui::GetStyle().ScaleAllSizes(dpiScale_);
        if (!imguiReady_) {
            if (!ImGui_ImplWin32_Init(gameWindow_)) return false;
            if (!imguiRenderer_.Initialize(device_.Get(), ImGui::GetIO().Fonts)) {
                ImGui_ImplWin32_Shutdown();
                return false;
            }
            imguiReady_ = true;
        }
        return true;
    }

    bool Renderer::RebuildFontAtlas(int fontSizeSetting,
        std::string_view fontProfileCode)
    {
        if (!imguiReady_ || !ImGui::GetCurrentContext() ||
            fontSizeSetting < config::OverlayFontSizeMin ||
            fontSizeSetting > config::OverlayFontSizeMax)
            return false;
        const int fontSizePixels = presentation::EffectiveFontPixels(
            fontSizeSetting, windowDpi_);
#ifdef OVERLAY_FONT_RASTERIZATION_EXPERIMENT
        const int requestedRasterizationMode =
            font_rasterization_experiment::CurrentMode();
        const bool rasterizationChanged =
            requestedRasterizationMode != activeFontRasterizationMode_;
#else
        const bool rasterizationChanged = false;
#endif
#ifdef OVERLAY_FONT_PLAYGROUND_EXPERIMENT
        const int requestedFontChoice =
            font_playground_experiment::CurrentFontChoice();
        const bool fontChoiceChanged = requestedFontChoice != activeFontChoice_;
#else
        const bool fontChoiceChanged = false;
#endif
        const bool fontProfileChanged = fontProfileCode != activeFontProfileCode_;
        if (fontSizePixels == activeFontSizePixels_ &&
            fontProfileCode == activeFontProfileCode_ && !rasterizationChanged &&
            !fontChoiceChanged) {
            activeFontSetting_ = fontSizeSetting;
            return true;
        }
        imguiRenderer_.InvalidateFontTexture(ImGui::GetIO().Fonts);
        ImFontAtlas* atlas = ImGui::GetIO().Fonts;
        atlas->Clear();
#ifdef OVERLAY_FONT_PLAYGROUND_EXPERIMENT
        ImGui::GetIO().FontDefault =
            font_playground_experiment::AddSelectedFont(fontSizePixels);
#elif defined(OVERLAY_FONT_RASTERIZATION_EXPERIMENT)
        ImGui::GetIO().FontDefault = AddProggyVectorOnly(fontSizePixels);
#else
        ImGui::GetIO().FontDefault = AddLocalizationFont(fontSizePixels,
            fontProfileCode);
        if (ImGui::GetIO().FontDefault &&
            !AddLocalizationSelectorFonts(fontSizePixels))
            ImGui::GetIO().FontDefault = nullptr;
#endif
        bool rebuilt = ImGui::GetIO().FontDefault && atlas->Build() &&
            imguiRenderer_.CreateFontTexture(atlas);
        if (rebuilt) {
            activeFontSizePixels_ = fontSizePixels;
            activeFontSetting_ = fontSizeSetting;
            activeFontProfileCode_ = fontProfileCode;
            forceFullDamage_ = true;
#ifdef OVERLAY_FONT_RASTERIZATION_EXPERIMENT
            activeFontRasterizationMode_ = requestedRasterizationMode;
#endif
#ifdef OVERLAY_FONT_PLAYGROUND_EXPERIMENT
            activeFontChoice_ = requestedFontChoice;
#endif
            Log("OVERLAY_FONT_ATLAS_REBUILT size=" + std::to_string(fontSizePixels) +
                " profile=" + std::string(activeFontProfileCode_));
            return true;
        }

        Log("OVERLAY_FONT_ATLAS_REBUILD_FAILED size=" + std::to_string(fontSizePixels));
        imguiRenderer_.InvalidateFontTexture(atlas);
        atlas->Clear();
#ifdef OVERLAY_FONT_PLAYGROUND_EXPERIMENT
        font_playground_experiment::SetCurrentFontChoice(activeFontChoice_);
        ImGui::GetIO().FontDefault =
            font_playground_experiment::AddSelectedFont(activeFontSizePixels_);
#elif defined(OVERLAY_FONT_RASTERIZATION_EXPERIMENT)
        font_rasterization_experiment::SetCurrentMode(activeFontRasterizationMode_);
        ImGui::GetIO().FontDefault = AddProggyVectorOnly(activeFontSizePixels_);
#else
        ImGui::GetIO().FontDefault = AddLocalizationFont(activeFontSizePixels_,
            activeFontProfileCode_);
        if (ImGui::GetIO().FontDefault &&
            !AddLocalizationSelectorFonts(activeFontSizePixels_))
            ImGui::GetIO().FontDefault = nullptr;
#endif
        const bool restored = ImGui::GetIO().FontDefault && atlas->Build() &&
            imguiRenderer_.CreateFontTexture(atlas);
        if (!restored) {
            Log("OVERLAY_FONT_ATLAS_RESTORE_FAILED");
            return false;
        }

        if (fontProfileChanged) {
            Log("OVERLAY_FONT_PROFILE_CHANGE_REJECTED profile=" +
                std::string(fontProfileCode));
            return false;
        }

#ifdef OVERLAY_SETTINGS_FRONTEND
        ApplySetting(plugin::RuntimeSettingMutation::FontSize(activeFontSetting_), logger_);
#endif
        Log("OVERLAY_FONT_ATLAS_RESTORED size=" + std::to_string(activeFontSizePixels_));
        return true;
    }

    bool Renderer::DrawImGuiSurface(IDCompositionSurface* targetSurface,
        UINT width, UINT height, const ImDrawData* drawData,
        presentation::Rect damage) noexcept
    {
        if (!targetSurface || !device_ || !compositionDevice_ || !width || !height)
            return false;
        const RECT fullRect{0, 0, static_cast<LONG>(width),
            static_cast<LONG>(height)};
        const RECT requested{damage.left, damage.top, damage.right, damage.bottom};
        const bool fullUpdate = requested.left == 0 && requested.top == 0 &&
            requested.right == fullRect.right && requested.bottom == fullRect.bottom;
        const auto drawUpdate = [&](const RECT& updateRect, bool updateWholeSurface) {
            Microsoft::WRL::ComPtr<IDXGISurface> updateSurface;
            POINT offset{};
            const RECT* updatePointer = updateWholeSurface ? nullptr : &updateRect;
            const HRESULT beginResult = targetSurface->BeginDraw(updatePointer,
                __uuidof(IDXGISurface),
                reinterpret_cast<void**>(updateSurface.GetAddressOf()), &offset);
            if (FAILED(beginResult)) return false;

            Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
            Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target;
            bool drew = false;
            if (SUCCEEDED(updateSurface.As(&texture)) &&
                SUCCEEDED(device_->CreateRenderTargetView(texture.Get(), nullptr,
                    target.GetAddressOf()))) {
                if (drawData) {
                    drew = imguiRenderer_.Render(drawData, target.Get(), width,
                        height, updateRect, offset);
                } else {
                    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
                    Microsoft::WRL::ComPtr<ID3D11DeviceContext1> context1;
                    device_->GetImmediateContext(context.GetAddressOf());
                    if (context && SUCCEEDED(context.As(&context1))) {
                        const float transparent[]{0, 0, 0, 0};
                        const LONG updateWidth = updateRect.right - updateRect.left;
                        const LONG updateHeight = updateRect.bottom - updateRect.top;
                        const D3D11_RECT clearRect{offset.x, offset.y,
                            offset.x + updateWidth, offset.y + updateHeight};
                        context1->ClearView(target.Get(), transparent, &clearRect, 1);
                        context->OMSetRenderTargets(0, nullptr, nullptr);
                        drew = true;
                    } else if (fullUpdate && context) {
                        const float transparent[]{0, 0, 0, 0};
                        context->ClearRenderTargetView(target.Get(), transparent);
                        context->OMSetRenderTargets(0, nullptr, nullptr);
                        drew = true;
                    }
                }
            }
            target.Reset();
            texture.Reset();
            updateSurface.Reset();
            const HRESULT endResult = targetSurface->EndDraw();
            return drew && SUCCEEDED(endResult);
        };

        bool drew = drawUpdate(requested, fullUpdate);
        if (!drew && !fullUpdate) {
            Log("OVERLAY_DAMAGE_UPDATE_FALLBACK reason=partial_draw_unavailable");
            drew = drawUpdate(fullRect, true);
        }
        return drew && SUCCEEDED(compositionDevice_->Commit());
    }

    bool Renderer::OnWindowGeometry(HWND window, UINT width, UINT height,
        bool minimized, UINT dpi) noexcept
    {
        bool updated = false;
        const auto outcome = RunOptionalOverlayWork([&]() {
            if (!OwnsWindow(window)) return;
            hostMinimized_ = minimized;
            if (minimized) menuFrameAdmission_.Invalidate();
            const bool dpiChanged = dpi != windowDpi_;
            if (dpiChanged && !ApplyWindowDpi(dpi)) {
                Disable("window_dpi_update_failed");
                return;
            }
            if (!UpdateOverlayWindowBounds()) {
                Disable("overlay_window_geometry_update_failed");
                return;
            }
            if (minimized) {
                const bool wasMinimized = state_.phase() == PresenterPhase::Minimized;
                state_.SetMinimized(true);
                if (!wasMinimized) Log("OVERLAY_WINDOW_MINIMIZED");
                updated = true;
                return;
            }
            if (!width || !height) return;
            state_.SetMinimized(false);
            if (width == surfaceWidth_ && height == surfaceHeight_) {
                if (dpiChanged) forceFullDamage_ = true;
                surfaceDirty_ = true;
                updated = RenderImpl(surface_.Get(), width, height);
                if (updated) updated = StartPendingNotificationLifetimes();
                return;
            }
            SurfaceUpdateToken token{};
            if (!state_.BeginSurfaceUpdate({width, height}, token)) return;
            surfaceDirty_ = true;
            forceFullDamage_ = true;
            Microsoft::WRL::ComPtr<IDCompositionSurface> replacement;
            if (!CreateSurface(width, height, replacement) ||
                !RenderImpl(replacement.Get(), width, height)) {
                Log("OVERLAY_DCOMP_RESIZE_PREPARE_FAILED width=" +
                    std::to_string(width) + " height=" + std::to_string(height));
                Disable("composition_resize_prepare_failed");
                return;
            }
            HRESULT result = visual_->SetContent(replacement.Get());
            if (SUCCEEDED(result)) result = compositionDevice_->Commit();
            if (FAILED(result)) {
                visual_->SetContent(surface_.Get());
                compositionDevice_->Commit();
                Log("OVERLAY_DCOMP_RESIZE_COMMIT_FAILED hr=" +
                    std::to_string(static_cast<long>(result)));
                Disable("composition_resize_commit_failed");
                return;
            }
            if (!PublishCommittedSurfaceResource(state_, token, surface_,
                    std::move(replacement), true, true)) {
                visual_->SetContent(surface_.Get());
                compositionDevice_->Commit();
                Log("OVERLAY_DCOMP_RESIZE_GENERATION_REJECTED");
                Disable("composition_resize_generation_rejected");
                return;
            }
            surfaceWidth_ = width;
            surfaceHeight_ = height;
            surfaceGeneration_ = state_.generation();
            surfaceDirty_ = false;
            damageTracker_.Invalidate();
            forceFullDamage_ = true;
            if (!StartPendingNotificationLifetimes()) return;
            Log("OVERLAY_DCOMP_SURFACE_RESIZED generation=" +
                std::to_string(surfaceGeneration_) + " width=" +
                std::to_string(width) + " height=" + std::to_string(height));
            updated = true;
        }, [this]() noexcept { FailAfterException(); });
        return outcome == OptionalOverlayWorkResult::Completed && updated;
    }

    bool Renderer::OnHostActivation(HWND window, bool /*active*/) noexcept
    {
        bool updated = false;
        const auto outcome = RunOptionalOverlayWork([&]() {
            if (!OwnsWindow(window)) return;
            // Queued activation can already be stale when the owner consumes it.
            ReconcileHostActivity();
            surfaceDirty_ = true;
            if (!UpdateOverlayWindowBounds(true)) {
                Disable("overlay_window_activation_update_failed");
                return;
            }
            if (hostActive_ && overlayWindowVisible_)
                StartPendingNotificationLifetimes();
            updated = true;
        }, [this]() noexcept { FailAfterException(); });
        return outcome == OptionalOverlayWorkResult::Completed && updated;
    }

    bool Renderer::DisplayWindowAboveHost() const noexcept
    {
        if (!gameWindow_ || !overlayWindow_ ||
            !IsWindow(gameWindow_) || !IsWindow(overlayWindow_)) return false;
        // Geometry and WS_VISIBLE can remain unchanged when the host raises
        // itself over this unowned display. Bound traversal if windows churn.
        HWND above = GetWindow(gameWindow_, GW_HWNDPREV);
        for (unsigned visited = 0; above && visited < 256; ++visited) {
            if (above == overlayWindow_) return true;
            above = GetWindow(above, GW_HWNDPREV);
        }
        return false;
    }

    bool Renderer::UpdateOverlayWindowBounds(bool forcePlacement) noexcept
    {
        if (!overlayWindow_ || !IsWindow(overlayWindow_)) return false;
        const bool shouldBeVisible = visualAttached_ && hostActive_ &&
            !hostMinimized_ && gameWindow_ && IsWindow(gameWindow_);
        if (!shouldBeVisible) {
            return SetOverlayWindowVisibility(false);
        }

        RECT client{};
        POINT origin{};
        if (!GetClientRect(gameWindow_, &client) ||
            !ClientToScreen(gameWindow_, &origin))
            return false;
        const LONG width = client.right - client.left;
        const LONG height = client.bottom - client.top;
        if (width <= 0 || height <= 0) {
            return SetOverlayWindowVisibility(false);
        }

        const bool overlayTopmost = (GetWindowLongPtrW(overlayWindow_, GWL_EXSTYLE) &
            WS_EX_TOPMOST) != 0;
        // A non-activating unowned display in the normal z-band can remain
        // behind the foreground host even after an accepted HWND_TOP call.
        // Promote only our visible client-sized display; inactive UI is hidden.
        const DisplayPlacement requestedPlacement{origin.x, origin.y,
            width, height, windowDpi_, true, true};
        const bool placementChanged = forcePlacement || !overlayWindowVisible_ ||
            !IsWindowVisible(overlayWindow_) ||
            !overlayTopmost || !DisplayWindowAboveHost() ||
            RequiresDisplayPlacementUpdate(overlayPlacement_, requestedPlacement);
        const bool shown = CommitDisplayWindowVisibility(true,
            [this, origin, width, height, placementChanged]() {
                if (!placementChanged) return true;
                return SetWindowPos(overlayWindow_, HWND_TOPMOST, origin.x,
                    origin.y, width, height, SWP_NOACTIVATE |
                        SWP_NOOWNERZORDER | SWP_SHOWWINDOW) != FALSE;
            }, [this]() {
                return IsWindow(overlayWindow_) &&
                    IsWindowVisible(overlayWindow_);
            });
        if (!shown) return false;
        overlayWindowVisible_ = true;
        overlayPlacement_ = requestedPlacement;
        LogDisplayState();
        return true;
    }

    void Renderer::ReconcileHostActivity() noexcept
    {
        hostActive_ = ResolveHostDisplaySnapshot(gameWindow_ && IsWindow(gameWindow_),
            GetForegroundWindow() == gameWindow_,
            !gameWindow_ || IsIconic(gameWindow_) != FALSE);
        if (!hostActive_) menuFrameAdmission_.Invalidate();
    }

    bool Renderer::SetOverlayWindowVisibility(bool visible) noexcept
    {
        if (!overlayWindow_ || !IsWindow(overlayWindow_)) return false;
        const bool changed = overlayWindowVisible_ != visible;
        const UINT visibilityFlag = visible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW;
        const bool accepted = CommitDisplayWindowVisibility(visible,
            [this, visibilityFlag]() {
                return SetWindowPos(overlayWindow_, nullptr, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
                        SWP_NOOWNERZORDER | visibilityFlag) != FALSE;
            }, [this]() {
                return IsWindow(overlayWindow_) &&
                    IsWindowVisible(overlayWindow_);
            });
        if (!accepted) return false;
        overlayWindowVisible_ = visible;
        LogDisplayState();
        if (changed) {
            LogNotificationLifecycle(visible ? "display_show" : "display_hide",
                "display_visibility_reconciled");
        }
        return true;
    }

    void Renderer::LogDisplayState() noexcept
    {
        if (!diagnostics::Enabled() || !logger_) return;
        const unsigned observed = (visible() ? 1u : 0u) |
            (visualAttached_ ? 2u : 0u) | (hostActive_ ? 4u : 0u) |
            (hostMinimized_ ? 8u : 0u) | (overlayWindowVisible_ ? 16u : 0u) |
            (DisplayWindowAboveHost() ? 32u : 0u);
        if (observed == diagnosticDisplayState_) return;
        diagnosticDisplayState_ = observed;
        try {
            Log("OVERLAY_DISPLAY_STATE requested=" + std::to_string(visible()) +
                " root_attached=" + std::to_string(visualAttached_) +
                " host_active=" + std::to_string(hostActive_) +
                " host_minimized=" + std::to_string(hostMinimized_) +
                " display_visible=" + std::to_string(overlayWindowVisible_) +
                " z_order_above_game=" + std::to_string((observed & 32u) != 0) +
                " game_hwnd=" + std::to_string(reinterpret_cast<std::uintptr_t>(gameWindow_)) +
                " foreground_hwnd=" + std::to_string(reinterpret_cast<std::uintptr_t>(GetForegroundWindow())) +
                " display_hwnd=" + std::to_string(reinterpret_cast<std::uintptr_t>(overlayWindow_)));
        } catch (...) {}
    }

    void Renderer::LogNotificationLifecycle(const char* event,
        const char* reason, std::uint64_t notificationId) const noexcept
    {
        if (!diagnostics::Enabled() || !logger_) return;
        try {
            const auto now = static_cast<std::uint64_t>(GetTickCount64());
            std::string queueState;
            for (const auto& active : notifications_) {
                if (!queueState.empty()) queueState.push_back(',');
                const auto startedAt = active.lifetime.visibleAtMs();
                const auto duration = static_cast<std::uint64_t>(
                    active.notification.durationMs);
                const auto deadline = startedAt &&
                    startedAt <= (std::numeric_limits<std::uint64_t>::max)() - duration
                    ? startedAt + duration : 0;
                queueState += std::to_string(active.notification.id) + ':' +
                    (startedAt ? "started" : "pending") + ':' +
                    std::to_string(active.lifetime.AgeMs(now)) + ':' +
                    std::to_string(deadline);
            }
            Log("OVERLAY_TOAST_LIFECYCLE ts_ms=" + std::to_string(now) +
                " tid=" + std::to_string(GetCurrentThreadId()) +
                " event=" + (event ? event : "unknown") +
                " reason=" + (reason ? reason : "unknown") +
                " id=" + std::to_string(notificationId) +
                " queue_count=" + std::to_string(notifications_.size()) +
                " queue=" + (queueState.empty() ? "-" : queueState) +
                " startup_pending=" + std::to_string(startupNotification_.pending()) +
                " menu_requested=" + std::to_string(visible()) +
                " root_attached=" + std::to_string(visualAttached_) +
                " display_visible=" + std::to_string(overlayWindowVisible_) +
                " hwnd_visible=" + std::to_string(overlayWindow_ &&
                    IsWindowVisible(overlayWindow_)) +
                " host_active=" + std::to_string(hostActive_) +
                " minimized=" + std::to_string(hostMinimized_) +
                " generation=" + std::to_string(surfaceGeneration_) +
                " z_above_host=" + std::to_string(DisplayWindowAboveHost()));
        } catch (...) {
            // Optional lifecycle diagnostics cannot affect notification state.
        }
    }

    bool Renderer::ResetComposition() noexcept
    {
        menuFrameAdmission_.Invalidate();
        const bool wasAttached = visualAttached_;
        if (compositionTarget_) {
            const HRESULT detachResult = compositionTarget_->SetRoot(nullptr);
            if (FAILED(detachResult)) {
                try {
                    Log("OVERLAY_DCOMP_DETACH_FAILED hr=" +
                        std::to_string(static_cast<std::uint32_t>(detachResult)));
                } catch (...) {}
                return false;
            }
            if (!compositionDevice_) return false;
            const HRESULT commitResult = compositionDevice_->Commit();
            if (FAILED(commitResult)) {
                try {
                    Log("OVERLAY_DCOMP_DETACH_COMMIT_FAILED hr=" +
                        std::to_string(static_cast<std::uint32_t>(commitResult)));
                } catch (...) {}
                return false;
            }
        }
        surface_.Reset();
        visual_.Reset();
        compositionTarget_.Reset();
        compositionDevice_.Reset();
        dxgiDevice_.Reset();
        device_.Reset();
        surfaceWidth_ = 0;
        surfaceHeight_ = 0;
        compositionReady_ = false;
        visualAttached_ = false;
        if (wasAttached)
            LogNotificationLifecycle("root_detach", "composition_reset");
        return true;
    }

    void Renderer::DetachImGui() noexcept
    {
        if (ImGui::GetCurrentContext()) {
            auto& io = ImGui::GetIO();
            // Initialize can acquire device/pipeline references before failing.
            // Readiness is not ownership; release the backend in either state.
            imguiRenderer_.Shutdown(io.Fonts);
            if (io.BackendPlatformUserData) ImGui_ImplWin32_Shutdown();
            if (imguiContextCreated_) ImGui::DestroyContext();
        } else {
            imguiRenderer_.Shutdown(nullptr);
        }
        imguiReady_ = false;
        imguiContextCreated_ = false;
    }

    void Renderer::UpdateNotifications() noexcept
    {
        try {
            std::vector<plugin::OverlayNotification> incoming;
            plugin::DrainOverlayNotifications(incoming);
            for (auto& notification : incoming) {
                if (notifications_.size() >= 3) {
                    LogNotificationLifecycle("queue_evict",
                        "active_notification_limit",
                        notifications_.front().notification.id);
                    notifications_.erase(notifications_.begin());
                }
                const auto notificationId = notification.id;
                notifications_.push_back({std::move(notification), {}});
                LogNotificationLifecycle("queue_add", "runtime_notification",
                    notificationId);
            }
            const auto now = static_cast<std::uint64_t>(GetTickCount64());
            for (auto active = notifications_.begin();
                active != notifications_.end();) {
                if (!active->lifetime.IsExpired(now,
                        active->notification.durationMs)) {
                    ++active;
                    continue;
                }
                LogNotificationLifecycle("queue_expire", "ttl_deadline",
                    active->notification.id);
                active = notifications_.erase(active);
                surfaceDirty_ = true;
            }
        } catch (...) {
            // Toasts are optional and must never affect rendering or settings.
        }
    }

    void Renderer::EnsureStartupNotification(
        const plugin::RuntimeSettingsSnapshot& settings) noexcept
    {
        if (!startupNotification_.pending()) return;
        try {
            plugin::OverlayNotification hint{};
            hint.kind = plugin::OverlayNotificationKind::StartupOverlayHint;
            hint.action = plugin::OverlayNotificationAction::OverlayToggle;
            hint.primaryValue = config::HotkeyName(settings.overlayToggleKey);
            hint.status = plugin::OverlayNotificationStatus::Ready;
            hint.durationMs = StartupHintDurationMs;
            hint.id = 0;
            if (notifications_.size() >= 3) {
                LogNotificationLifecycle("queue_evict",
                    "startup_hint_capacity", notifications_.front().notification.id);
                notifications_.erase(notifications_.begin());
            }
            notifications_.push_back({std::move(hint), {}});
            startupNotification_.MarkCreated(ready(), true);
            surfaceDirty_ = true;
            Log("OVERLAY_STARTUP_HINT_CREATED presenter_lifecycle=1");
            LogNotificationLifecycle("startup_created", "ready_and_locale_resolved", 0);
        } catch (...) {
            Log("OVERLAY_STARTUP_HINT_CREATE_FAILED");
        }
    }

    void Renderer::PrepareStartupNotification() noexcept
    {
        if (!startupNotification_.ShouldWakePresenter(ready(), inputBindingReady_)) return;
        try {
            plugin::RuntimeSettingsSnapshot settings{};
            if (!plugin::GetRuntimeSettingsApi().Snapshot(settings)) return;
            if (ready() && inputBindingReady_ &&
                startupReadiness_.MarkOverlayReady())
                LogStartupGate("OVERLAY_READY",
                    "presenter_and_game_input_binding_ready");

            if (!startupReadiness_.probeStarted() &&
                !startupReadiness_.fallbackResolved() &&
                !startupReadiness_.gameDataObserved()) {
                if (!startupReadiness_.BeginGameDataProbe(GetTickCount64(),
                        StartupGameDataProbeTimeoutMs))
                    return;
                if (RequestStartupGameDataProbe(gameWindow_)) {
                    LogStartupGate("WAITING",
                        "validated_game_interface_language_probe");
                } else {
                    startupReadiness_.ResolveFallback();
                    LogStartupGate("FALLBACK", "startup_probe_post_rejected");
                    if (settings.overlayLocaleAuto &&
                        !settings.overlayLocaleAutoSynchronized) {
                        plugin::SetDetectedOverlayLocale("en");
                        if (!plugin::GetRuntimeSettingsApi().Snapshot(settings)) {
                            settings.overlayLocaleCode = "en";
                            settings.overlayLocaleAutoSynchronized = true;
                        }
                    }
                    LogStartupGateEligibility();
                }
            }

            if (StartupHintMustWaitForAutoLocale(settings.overlayLocaleAuto,
                    settings.overlayLocaleAutoSynchronized,
                    startupReadiness_.probePending())) {
                if (!startupAutoLocaleWaitLogged_) {
                    startupAutoLocaleWaitLogged_ = true;
                    Log("OVERLAY_STARTUP_HINT_WAITING_FOR_AUTO_LOCALE");
                    LogNotificationLifecycle("startup_wait_locale",
                        "accepted_startup_game_data_probe");
                }
            }
            if (!startupReadiness_.eligible())
                return;

            LogStartupGateEligibility();
            EnsureStartupNotification(settings);
        } catch (...) {
            Log("OVERLAY_STARTUP_PREPARATION_FAILED");
        }
    }

    void Renderer::LogCompositionAttachment() const noexcept
    {
        try {
            Log(visualAttached_ ? "OVERLAY_DCOMP_ROOT_ATTACHMENT attached=1"
                : "OVERLAY_DCOMP_ROOT_ATTACHMENT attached=0");
        } catch (...) {
            // Lifecycle observation cannot invalidate an accepted transaction.
        }
    }

    bool Renderer::ReconcileCompositionAttachment() noexcept
    {
        ReconcileHostActivity();
        const bool required = CompositionRootRequired(visible(),
            notifications_.size(), startupNotification_.pending());
        if (visualAttached_ == required) {
            const bool displayRequired = required && hostActive_ && !hostMinimized_;
            if ((displayRequired != overlayWindowVisible_ ||
                    (displayRequired && !DisplayWindowAboveHost())) &&
                    !UpdateOverlayWindowBounds()) {
                Disable("overlay_window_attachment_update_failed");
                return false;
            }
            LogDisplayState();
            return true;
        }
        const char* reason = visible() ? "menu_requested"
            : !notifications_.empty() ? "active_notification"
            : startupNotification_.pending() ? "startup_pending"
            : "no_display_demand";
        bool committed = false;
        try {
            if (compositionTarget_ && compositionDevice_ && visual_) {
                committed = CommitCompositionAttachment(visualAttached_, required,
                    [this](bool attach) {
                        return SUCCEEDED(compositionTarget_->SetRoot(
                            attach ? visual_.Get() : nullptr));
                    },
                    [this]() { return SUCCEEDED(compositionDevice_->Commit()); });
            }
        } catch (...) {
            // Keep failures inside the optional renderer boundary.
        }
        if (!committed) {
            Disable("composition_attachment_commit_failed");
            return false;
        }
        if (!UpdateOverlayWindowBounds()) {
            Disable("overlay_window_attachment_update_failed");
            return false;
        }
        LogCompositionAttachment();
        LogNotificationLifecycle(required ? "root_attach" : "root_detach",
            reason);
        return true;
    }

    bool Renderer::StartPendingNotificationLifetimes() noexcept
    {
        // The current surface has been drawn/cleared and, for resize, published.
        // Attach it before starting visible toast lifetime, or detach settled UI.
        if (!ReconcileCompositionAttachment()) return false;
        menuFrameAdmission_.Commit(ready() && visualAttached_ &&
            overlayWindowVisible_ && hostActive_ && !hostMinimized_);
        if (!overlayWindowVisible_) return true;
        const auto committedAt = static_cast<std::uint64_t>(GetTickCount64());
        for (auto& active : notifications_) {
            if (StartNotificationLifetimeAfterCommit(active.lifetime,
                    active.notification.id,
                    std::span<const std::uint64_t>{submittedNotificationIds_.data(),
                        submittedNotificationCount_}, true, committedAt)) {
                try {
                    Log("OVERLAY_NOTIFICATION_LIFETIME_STARTED id=" +
                        std::to_string(active.notification.id));
                    LogNotificationLifecycle("ttl_start",
                        "submitted_draw_and_attachment_commit",
                        active.notification.id);
                    if (active.notification.kind ==
                            plugin::OverlayNotificationKind::StartupOverlayHint &&
                        !startupHintFrameLogged_) {
                        startupHintFrameLogged_ = true;
                        Log("OVERLAY_STARTUP_HINT_DRAWN_AND_COMMITTED");
                    }
                } catch (...) {
                    // Notification diagnostics are optional.
                }
            }
        }
        return true;
    }

    void Renderer::DrawNotifications()
    {
        const auto now = static_cast<std::uint64_t>(GetTickCount64());
        float y = 30.0f;
        for (auto& active : notifications_) {
            active.frameDrawListToken = 0;
            const auto& notification = active.notification;
            if (!active.lifetime.IsDrawable(now, notification.durationMs)) continue;
            const auto age = active.lifetime.AgeMs(now);
            const float fadeIn = static_cast<float>(age) / 220.0f < 1.0f
                ? static_cast<float>(age) / 220.0f : 1.0f;
            const auto fadeStart = notification.durationMs > 400
                ? notification.durationMs - 400 : 0;
            const float fadeOut = age >= fadeStart
                ? (1.0f - static_cast<float>(age - fadeStart) / 400.0f > 0.0f
                    ? 1.0f - static_cast<float>(age - fadeStart) / 400.0f : 0.0f)
                : 1.0f;
            const float alpha = fadeIn * fadeOut;
            if (alpha <= 0.0f) continue;

            const std::string windowId = "##overlay_notification_" +
                std::to_string(notification.id);
            ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, y),
                ImGuiCond_Always, ImVec2(0.5f, 0.0f));
            ImGui::SetNextWindowSizeConstraints(ImVec2(250.0f * dpiScale_, 0.0f),
                ImVec2(440.0f * dpiScale_, FLT_MAX));
            ImGui::SetNextWindowBgAlpha(0.92f * alpha);
            ImGui::Begin(windowId.c_str(), nullptr,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_AlwaysAutoResize);
            active.frameDrawListToken = reinterpret_cast<std::uintptr_t>(
                ImGui::GetWindowDrawList());
            std::string localizedValue;
            std::string localizedTitle;
            if (notification.kind == plugin::OverlayNotificationKind::StartupOverlayHint) {
                localizedTitle = Tr(localization_, loc::app::Title);
                localizedValue = Tr(localization_, loc::notification::OpenOverlayHint,
                    {{"key", std::string(kInlineHotkeyToken)}});
            } else if (notification.kind == plugin::OverlayNotificationKind::PersistenceFailed) {
                localizedTitle = Tr(localization_, NotificationActionLabel(notification.action));
                localizedValue = Tr(localization_, loc::notification::PersistenceFailed);
            } else if (notification.kind == plugin::OverlayNotificationKind::HotkeyConflict) {
                const auto actionLabel = Tr(localization_, NotificationActionLabel(notification.action));
                localizedTitle = Tr(localization_, loc::notification::HotkeyConflict);
                localizedValue = Tr(localization_, loc::notification::ConflictValue,
                    {{"key", notification.primaryValue}, {"setting", actionLabel}});
            } else if (notification.kind == plugin::OverlayNotificationKind::BindingChanged) {
                const auto actionLabel = Tr(localization_, NotificationActionLabel(notification.action));
                localizedTitle = Tr(localization_, loc::notification::HotkeyTitle,
                    {{"setting", actionLabel}});
                localizedValue = Tr(localization_, loc::notification::BindingValue,
                    {{"oldKey", notification.primaryValue}, {"newKey", notification.secondaryValue}});
            } else {
                const auto actionLabel = Tr(localization_, NotificationActionLabel(notification.action));
                localizedTitle = actionLabel;
                localizedValue = Tr(localization_, loc::notification::SettingChanged,
                    {{"oldValue", notification.primaryValue}, {"newValue", notification.secondaryValue}});
            }
            DrawNotificationHeader(localizedTitle, alpha);
            if (notification.kind == plugin::OverlayNotificationKind::StartupOverlayHint) {
                DrawInlineHotkeyMessage(localizedValue, kInlineHotkeyToken,
                    notification.primaryValue);
            } else {
                ImGui::TextUnformatted(localizedValue.c_str());
            }
            if (notification.kind != plugin::OverlayNotificationKind::StartupOverlayHint) {
            const FeatureStatusTone statusTone = notification.status ==
                plugin::OverlayNotificationStatus::Error
                ? FeatureStatusTone::Error
                : notification.status == plugin::OverlayNotificationStatus::Pending
                    ? FeatureStatusTone::Warning : FeatureStatusTone::Positive;
            ImVec4 statusColor = FeatureStatusColor(statusTone);
            statusColor.w = alpha;
            const auto statusKey = notification.status == plugin::OverlayNotificationStatus::Applied
                ? loc::common::Applied : notification.status == plugin::OverlayNotificationStatus::Pending
                    ? loc::common::Pending : notification.status == plugin::OverlayNotificationStatus::Ready
                        ? loc::common::Ready : loc::common::Error;
            const auto localizedStatus = Tr(localization_, statusKey);
            ImGui::TextColored(statusColor, "%s", localizedStatus.c_str());
            }
            y += ImGui::GetWindowSize().y + 8.0f;
            ImGui::End();
        }
    }

    bool Renderer::Render() noexcept
    {
        bool rendered = false;
        const auto outcome = RunOptionalOverlayWork([&]() {
            if (needsRenderWork() && surface_) {
                rendered = RenderImpl(surface_.Get(), surfaceWidth_, surfaceHeight_);
                if (rendered) rendered = StartPendingNotificationLifetimes();
            }
        }, [this]() noexcept { FailAfterException(); });
        return outcome == OptionalOverlayWorkResult::Completed && rendered;
    }

    void Renderer::ApplyInputEvents(UINT width, UINT height)
    {
        std::vector<InputEvent> events;
        GetInputEventBridge().Drain(events);
        if (!ImGui::GetCurrentContext()) return;
        auto& io = ImGui::GetIO();
        for (const auto& event : events) {
            switch (event.kind) {
            case InputEventKind::Reset:
                io.AddFocusEvent(false);
                for (int button = 0; button < 5; ++button)
                    io.AddMouseButtonEvent(button, false);
                virtualCursorPosition_.Reset();
                io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
                break;
            case InputEventKind::AbsoluteMousePosition:
                virtualCursorPosition_.SetAbsolute(event.x, event.y,
                    static_cast<int>(width), static_cast<int>(height));
                if (!firstAbsoluteMouseAppliedLogged_) {
                    firstAbsoluteMouseAppliedLogged_ = true;
                    Log("OVERLAY_INPUT_ABSOLUTE_MOUSE_APPLIED x=" +
                        std::to_string(virtualCursorPosition_.point().x) + " y=" +
                        std::to_string(virtualCursorPosition_.point().y));
                }
                break;
            case InputEventKind::RelativeMouseMotion:
                virtualCursorPosition_.ApplyRelative(event.x, event.y,
                    static_cast<int>(width), static_cast<int>(height));
                if (!firstRawMouseAppliedLogged_) {
                    firstRawMouseAppliedLogged_ = true;
                    Log("OVERLAY_INPUT_RAW_MOUSE_APPLIED dx=" +
                        std::to_string(event.x) + " dy=" +
                        std::to_string(event.y) + " x=" +
                        std::to_string(virtualCursorPosition_.point().x) + " y=" +
                        std::to_string(virtualCursorPosition_.point().y));
                }
                break;
            case InputEventKind::MouseButton: {
                int button = -1;
                bool down = true;
                switch (event.message) {
                case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
                    button = 0; down = event.message != WM_LBUTTONUP; break;
                case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
                    button = 1; down = event.message != WM_RBUTTONUP; break;
                case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:
                    button = 2; down = event.message != WM_MBUTTONUP; break;
                case WM_XBUTTONDOWN: case WM_XBUTTONUP: case WM_XBUTTONDBLCLK:
                    button = HIWORD(event.wParam) == XBUTTON1 ? 3 : 4;
                    down = event.message != WM_XBUTTONUP;
                    break;
                default: break;
                }
                if (button >= 0) io.AddMouseButtonEvent(button, down);
                break;
            }
            case InputEventKind::MouseWheel:
                if (event.message == WM_MOUSEWHEEL)
                    io.AddMouseWheelEvent(0.0f,
                        static_cast<float>(static_cast<short>(HIWORD(event.wParam))) /
                            WHEEL_DELTA);
                else
                    io.AddMouseWheelEvent(
                        static_cast<float>(static_cast<short>(HIWORD(event.wParam))) /
                            WHEEL_DELTA, 0.0f);
                break;
            case InputEventKind::NativeKeyboardMessage:
                ImGui_ImplWin32_WndProcHandler(gameWindow_, event.message,
                    event.wParam, event.lParam);
                break;
            case InputEventKind::Focus:
                io.AddFocusEvent(event.wParam != FALSE);
                break;
            }
        }
        if (visible() && virtualCursorPosition_.known()) {
            const POINT position = virtualCursorPosition_.point();
            io.AddMousePosEvent(static_cast<float>(position.x),
                static_cast<float>(position.y));
        }
    }

    void Renderer::BeginImGuiFrame(UINT width, UINT height) noexcept
    {
        auto& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(static_cast<float>(width),
            static_cast<float>(height));
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = hasImGuiFrameTime_
            ? std::chrono::duration<float>(now - lastImGuiFrameTime_).count()
            : 1.0f / 60.0f;
        io.DeltaTime = (std::max)(elapsed, 1.0e-4f);
        lastImGuiFrameTime_ = now;
        hasImGuiFrameTime_ = true;
    }

    bool Renderer::RenderImpl(IDCompositionSurface* targetSurface,
        UINT width, UINT height)
    {
        if (!ready() || !targetSurface || !width || !height || !imguiReady_)
            return false;
        submittedNotificationCount_ = 0;
        submittedDrawLists_.clear();
        for (auto& notification : notifications_)
            notification.frameDrawListToken = 0;
        plugin::RuntimeSettingsSnapshot frameSettings{};
        bool frameSettingsAvailable = false;
        PrepareStartupNotification();
        UpdateNotifications();
        const bool overlayVisible = visible_.load(std::memory_order_acquire);
        if (!overlayVisible && notifications_.empty() && !surfaceDirty_)
            return false;
        if (ImGui::GetCurrentContext())
            ImGui::GetIO().MouseDrawCursor = overlayVisible;
#ifdef OVERLAY_SETTINGS_FRONTEND
        if (!frameSettingsAvailable)
            frameSettingsAvailable =
                plugin::GetRuntimeSettingsApi().Snapshot(frameSettings);
        if (frameSettingsAvailable) {
            const auto* localeDescriptor = localization::FindLocaleDescriptor(
                frameSettings.overlayLocaleCode);
            const std::string_view fontProfileCode = localeDescriptor
                ? localeDescriptor->fontProfileCode : std::string_view{"base"};
            bool fontSettingsChanged =
                frameSettings.overlayFontSize != activeFontSetting_ ||
                fontProfileCode != activeFontProfileCode_;
#ifdef OVERLAY_FONT_RASTERIZATION_EXPERIMENT
            fontSettingsChanged = fontSettingsChanged ||
                font_rasterization_experiment::CurrentMode() != activeFontRasterizationMode_;
#endif
#ifdef OVERLAY_FONT_PLAYGROUND_EXPERIMENT
            fontSettingsChanged = fontSettingsChanged ||
                font_playground_experiment::CurrentFontChoice() != activeFontChoice_;
#endif
            if (fontSettingsChanged && !RebuildFontAtlas(
                    frameSettings.overlayFontSize, fontProfileCode)) {
                Disable("font_atlas_rebuild_failed");
                return false;
            }
            if (frameSettings.overlayLocaleCode != activeLocaleCode_)
                forceFullDamage_ = true;
            activeLocaleCode_ = frameSettings.overlayLocaleCode;
            localization_.SetLocale(frameSettings.overlayLocaleCode);
        }
#endif
        BeginImGuiFrame(width, height);
        ApplyInputEvents(width, height);
        ImGui::NewFrame();
        if (!ImGui::IsPopupOpen(static_cast<ImGuiID>(0), ImGuiPopupFlags_AnyPopup))
            GetInputState().ConsumeEscapePopupDismissRequest();
        ImGui::GetStyle().CellPadding.x = 6.0f * dpiScale_;
        bool cameraStateLayoutChanged = false;
#ifdef OVERLAY_SETTINGS_FRONTEND
#endif
        if (overlayVisible) {
            const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
            const float maxOverlayWidth = layout_metrics::AvailableWindowWidth(
                mainViewport->Size.x, dpiScale_);
            const bool expandedLayout = cameraStateExpanded_ &&
                compactOverlayHeightLogical_ > 0.0f;
            const bool restoreCompactLayout = !expandedLayout &&
                restoreCameraStateCompactPosition_ &&
                cameraStateCompactPositionValid_;
            const float compactWidth = compactOverlayWidthLogical_ > 0.0f
                ? compactOverlayWidthLogical_ * dpiScale_
                : (std::min)(1024.0f * dpiScale_, maxOverlayWidth);
            const auto expandedLayoutMetrics = layout_metrics::
                ComputeCameraStatePanelLayout(mainViewport->Size.x,
                    compactWidth, compactOverlayHeightLogical_ * dpiScale_,
                    640.0f * dpiScale_,
                    ImGui::GetStyle().WindowPadding.x, expandedLayout, dpiScale_);
            if (restoreCompactLayout) {
                ImGui::SetNextWindowPos(cameraStateCompactPosition_,
                    ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(expandedLayoutMetrics.windowWidth,
                    expandedLayoutMetrics.windowHeight), ImGuiCond_Always);
                ImGui::SetNextWindowSizeConstraints(
                    ImVec2(expandedLayoutMetrics.windowWidth,
                        expandedLayoutMetrics.windowHeight),
                    ImVec2(expandedLayoutMetrics.windowWidth,
                        expandedLayoutMetrics.windowHeight));
                restoreCameraStateCompactPosition_ = false;
            } else {
                ImGui::SetNextWindowPos(ImVec2(overlayPositionX_, overlayPositionY_),
                    ImGuiCond_Once);
            }
            const float preferredOverlayWidth = 1024.0f * dpiScale_;
            if (expandedLayout) {
                ImGui::SetNextWindowSize(ImVec2(expandedLayoutMetrics.windowWidth,
                    expandedLayoutMetrics.windowHeight), ImGuiCond_Always);
                ImGui::SetNextWindowSizeConstraints(
                    ImVec2(expandedLayoutMetrics.windowWidth,
                        expandedLayoutMetrics.windowHeight),
                    ImVec2(expandedLayoutMetrics.windowWidth,
                        expandedLayoutMetrics.windowHeight));
            } else if (!restoreCompactLayout) {
                ImGui::SetNextWindowSizeConstraints(
                    ImVec2((std::min)(preferredOverlayWidth, maxOverlayWidth), 0.0f),
                    ImVec2(maxOverlayWidth, FLT_MAX));
            }
            const auto overlayTitle = Tr(localization_, loc::app::Title);
            const ImGuiWindowFlags overlayWindowFlags = ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoCollapse |
                ((expandedLayout || restoreCompactLayout) ? ImGuiWindowFlags_NoResize
                    : ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Begin(overlayTitle.c_str(), nullptr,
                overlayWindowFlags);
            if (!expandedLayout && !restoreCompactLayout) {
                const ImVec2 compactSize = ImGui::GetWindowSize();
                compactOverlayWidthLogical_ = compactSize.x / dpiScale_;
                compactOverlayHeightLogical_ = compactSize.y / dpiScale_;
            }
            const ImVec2 positionClampSize = restoreCompactLayout
                ? ImVec2(expandedLayoutMetrics.windowWidth,
                    expandedLayoutMetrics.windowHeight)
                : ImGui::GetWindowSize();
            const ImVec2 clampedPosition = ClampOverlayWindowPosition(
                ImGui::GetWindowPos(), positionClampSize,
                ImGui::GetMainViewport(), dpiScale_);
            if (clampedPosition.x != ImGui::GetWindowPos().x ||
                clampedPosition.y != ImGui::GetWindowPos().y)
                ImGui::SetWindowPos(clampedPosition, ImGuiCond_Always);
            if (expandedLayout) {
                // Temporary panel expansion/clamping must not rewrite the
                // user's persisted compact-window placement.
                lastOverlayPositionX_ = clampedPosition.x;
                lastOverlayPositionY_ = clampedPosition.y;
                overlayPositionObserved_ = true;
            } else if (restoreCompactLayout || !overlayPositionObserved_) {
                lastOverlayPositionX_ = clampedPosition.x;
                lastOverlayPositionY_ = clampedPosition.y;
                overlayPositionObserved_ = true;
            } else if (clampedPosition.x != lastOverlayPositionX_ ||
                clampedPosition.y != lastOverlayPositionY_) {
                placementSaveState_.PositionChanged();
                lastOverlayPositionX_ = clampedPosition.x;
                lastOverlayPositionY_ = clampedPosition.y;
            }
            const bool leftMouseButtonDown =
                ImGui::IsMouseDown(ImGuiMouseButton_Left);
            if (!expandedLayout && placementSaveState_.ShouldAttemptSave(
                    leftMouseButtonDown)) {
                const bool saved = !overlayConfigPath_.empty() &&
                    placement_config::Save(overlayConfigPath_,
                        static_cast<int>(clampedPosition.x),
                        static_cast<int>(clampedPosition.y));
                if (!saved) Log("OVERLAY_POSITION_PERSIST_FAILED");
                placementSaveState_.CompleteAttempt(saved);
            }
#ifdef OVERLAY_SETTINGS_FRONTEND
        plugin::OverlaySemanticSnapshot semantic{};
        const bool semanticReady = plugin::GetOverlaySemanticSnapshot(semantic) &&
            semantic.gameplayEnabled.valid && semantic.gameplayMode.valid &&
            semantic.cinematicAspectPolicy.valid && semantic.cinematicFovMode.valid &&
            semantic.dialogueEnabled.valid && semantic.dialogueZoomPolicy.valid;
        const bool settingsReady = semanticReady && frameSettingsAvailable;
        if (!settingsReady) {
            const auto unavailable = Tr(localization_, loc::common::Unavailable);
            ImGui::TextColored(FeatureStatusColor(FeatureStatusTone::Error), "%s", unavailable.c_str());
        } else {
            const auto runtimeSettingsLabel = Tr(localization_, loc::section::RuntimeSettings);
            const std::string toggleKeyName = config::HotkeyName(frameSettings.overlayToggleKey);
            const auto mouseCaptureNotice = Tr(localization_, loc::notice::MouseCapture,
                {{"key", std::string(kInlineHotkeyToken)}});
            ImGui::TextUnformatted(runtimeSettingsLabel.c_str());
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.94f, 0.98f, 1.0f));
            DrawInlineHotkeyMessage(mouseCaptureNotice, kInlineHotkeyToken,
                toggleKeyName);
            ImGui::PopStyleColor();
            const auto integration = ProjectCameraIntegration(semantic);
            const auto transitionHeader = Tr(localization_, loc::section::CameraTransition);
            ImGui::SeparatorText(transitionHeader.c_str());
            ImGui::BeginGroup();
            ImGui::TextColored(FeatureStatusColor(
                CameraTransitionTone(integration.potential.overall)), "%s",
                Tr(localization_, OverallIntegrationLabel(integration.potential.overall)).c_str());
            ImGui::EndGroup();
            const auto transitionDetail = Tr(localization_, CameraTransitionDetail(integration.potential.overall));
            ImGui::TextWrapped("%s", transitionDetail.c_str());
            const auto gameplayModes = setting_tooltip_content::GameplaySelectorOptions();
            const auto cinematicAspects = setting_tooltip_content::CinematicAspectSelectorOptions();
            const auto cinematicFovs = setting_tooltip_content::CinematicFovSelectorOptions();
            const auto dialoguePolicies = setting_tooltip_content::DialogueSelectorOptions();
            float commonSelectWidth = IntrinsicComboWidth(gameplayModes);
            commonSelectWidth = (std::max)(commonSelectWidth,
                IntrinsicComboWidth(cinematicAspects));
            commonSelectWidth = (std::max)(commonSelectWidth,
                IntrinsicComboWidth(cinematicFovs));
            commonSelectWidth = (std::max)(commonSelectWidth,
                IntrinsicComboWidth(dialoguePolicies));
            constexpr char autoGameLanguageLabel[] = "Auto (Game Language)";
            commonSelectWidth = (std::max)(commonSelectWidth,
                IntrinsicOptionWidth(autoGameLanguageLabel));
            for (const auto& locale : localization_.locales()) {
                commonSelectWidth = (std::max)(commonSelectWidth,
                    IntrinsicOptionWidth(locale.displayName));
            }

            const ImGuiTableFlags layoutFlags = ImGuiTableFlags_BordersInnerV |
                ImGuiTableFlags_NoSavedSettings |
                (expandedLayout ? ImGuiTableFlags_SizingFixedFit
                    : ImGuiTableFlags_SizingStretchProp);
            if (ImGui::BeginTable("settings_runtime_layout",
                    expandedLayout ? 3 : 2, layoutFlags)) {
                if (expandedLayout) {
                    const float leftWidth = expandedLayoutMetrics.existingColumnsWidth * 0.44f;
                    const float runtimeWidth = expandedLayoutMetrics.existingColumnsWidth - leftWidth;
                    ImGui::TableSetupColumn("settings", ImGuiTableColumnFlags_WidthFixed,
                        leftWidth);
                    ImGui::TableSetupColumn("runtime", ImGuiTableColumnFlags_WidthFixed,
                        runtimeWidth);
                    ImGui::TableSetupColumn("camera_state", ImGuiTableColumnFlags_WidthFixed,
                        expandedLayoutMetrics.cameraStateWidth);
                } else {
                    ImGui::TableSetupColumn("settings", ImGuiTableColumnFlags_WidthStretch,
                        0.44f);
                    ImGui::TableSetupColumn("runtime", ImGuiTableColumnFlags_WidthStretch,
                        0.56f);
                }
                ImGui::TableNextColumn();
            const auto gameplayHeader = Tr(localization_, loc::section::Gameplay);
            ImGui::SeparatorText(gameplayHeader.c_str());
            ImGui::BeginGroup();
            bool gameplayEnabled = frameSettings.gameplayEnabled;
            if (ImGui::Checkbox("##gameplay_enabled", &gameplayEnabled) &&
                !ApplySetting(plugin::RuntimeSettingMutation::GameplayEnabled(gameplayEnabled), logger_))
                gameplayEnabled = frameSettings.gameplayEnabled;
            ImGui::SameLine();
            DrawParameterLabel(localization_, loc::common::Enabled);
            ImGui::EndGroup();
            ShowGameplayEnabledTooltip(localization_);
            ImGui::BeginGroup();
            ImGui::SetNextItemWidth(commonSelectWidth);
            const auto gameplayCombo = DrawSelectorCombo("##gameplay_mode",
                frameSettings.gameplayMode, gameplayModes);
            if (gameplayCombo.valueChanged)
                ApplySetting(plugin::RuntimeSettingMutation::Gameplay(gameplayCombo.value), logger_);
            ImGui::SameLine();
            DrawParameterLabel(localization_, loc::setting::Mode);
            ImGui::EndGroup();
            ShowSettingOptionsTooltip(localization_, setting_tooltip_content::Gameplay(),
                gameplayCombo.interaction);
            DrawFeatureStatus(localization_, ProjectGameplayPresentation(semantic));

            const auto cinematicsHeader = Tr(localization_, loc::section::Cinematics);
            ImGui::SeparatorText(cinematicsHeader.c_str());
            ImGui::BeginGroup();
            bool cinematicsEnabled = frameSettings.cinematicsEnabled;
            if (ImGui::Checkbox("##cinematics_enabled", &cinematicsEnabled)) {
                if (ApplySetting(plugin::RuntimeSettingMutation::CinematicsEnabled(
                    cinematicsEnabled), logger_))
                    frameSettings.cinematicsEnabled = cinematicsEnabled;
            }
            ImGui::SameLine();
            DrawParameterLabel(localization_, loc::common::Enabled);
            ImGui::EndGroup();
            ShowCinematicsEnabledTooltip(localization_);
            ImGui::BeginDisabled(!frameSettings.cinematicsEnabled);
            ImGui::BeginGroup();
            ImGui::SetNextItemWidth(commonSelectWidth);
            const auto cinematicAspectCombo = DrawSelectorCombo("##cinematic_aspect",
                frameSettings.cinematicAspectPolicy, cinematicAspects);
            if (cinematicAspectCombo.valueChanged)
                ApplySetting(plugin::RuntimeSettingMutation::CinematicAspect(
                    cinematicAspectCombo.value), logger_);
            ImGui::SameLine();
            DrawParameterLabel(localization_, loc::setting::CinematicFraming);
            ImGui::EndGroup();
            ShowSettingOptionsTooltip(localization_, setting_tooltip_content::CinematicAspect(),
                cinematicAspectCombo.interaction);
            ImGui::BeginGroup();
            ImGui::SetNextItemWidth(commonSelectWidth);
            const auto cinematicFovCombo = DrawSelectorCombo("##cinematic_fov",
                frameSettings.cinematicFovMode, cinematicFovs);
            if (cinematicFovCombo.valueChanged)
                ApplySetting(plugin::RuntimeSettingMutation::CinematicFov(
                    cinematicFovCombo.value), logger_);
            ImGui::SameLine();
            DrawParameterLabel(localization_, loc::setting::FovMode);
            ImGui::EndGroup();
            ImGui::EndDisabled();
            ShowSettingOptionsTooltip(localization_, setting_tooltip_content::CinematicFov(),
                cinematicFovCombo.interaction);
            const auto cinematicStatus = ProjectCinematicPresentation(semantic);
            if (cinematicStatus.kind == FeatureStatusKind::Active) {
                DrawActiveStatus(localization_, cinematicStatus.tone,
                    loc::status::CinematicActive,
                    std::string(config::CinematicAspectPolicyName(semantic.activeCinematicAspectPolicy.value)) +
                        " / " + config::CinematicFovModeName(semantic.activeCinematicFovMode.value));
            } else {
                DrawFeatureStatus(localization_, cinematicStatus);
            }

            const auto dialogueHeader = Tr(localization_, loc::section::Dialogue);
            ImGui::SeparatorText(dialogueHeader.c_str());
            ImGui::BeginGroup();
            bool dialogueEnabled = frameSettings.dialogueEnabled;
            if (ImGui::Checkbox("##dialogue_enabled", &dialogueEnabled) &&
                ApplySetting(plugin::RuntimeSettingMutation::DialogueEnabled(
                    dialogueEnabled), logger_))
                frameSettings.dialogueEnabled = dialogueEnabled;
            ImGui::SameLine();
            DrawParameterLabel(localization_, loc::common::Enabled);
            ImGui::EndGroup();
            ShowDialogueEnabledTooltip(localization_);
            ImGui::BeginDisabled(!frameSettings.dialogueEnabled);
            ImGui::BeginGroup();
            ImGui::SetNextItemWidth(commonSelectWidth);
            const auto dialogueCombo = DrawSelectorCombo("##dialogue_zoom",
                frameSettings.dialogueZoomPolicy, dialoguePolicies);
            if (dialogueCombo.valueChanged)
                ApplySetting(plugin::RuntimeSettingMutation::Dialogue(
                    dialogueCombo.value), logger_);
            ImGui::SameLine();
            DrawParameterLabel(localization_, loc::setting::Mode);
            ImGui::EndGroup();
            ImGui::EndDisabled();
            ShowSettingOptionsTooltip(localization_, setting_tooltip_content::Dialogue(),
                dialogueCombo.interaction);
            const auto dialogueStatus = ProjectDialoguePresentation(semantic);
            if (dialogueStatus.kind == FeatureStatusKind::Active) {
                DrawActiveStatus(localization_, dialogueStatus.tone,
                    loc::status::DialogueActive,
                    ActiveDialoguePolicyLabel(semantic.activeDialogueZoomPolicy.value));
            } else {
                DrawFeatureStatus(localization_, dialogueStatus);
            }

            const auto overlayHeader = Tr(localization_, loc::section::Overlay);
            ImGui::SeparatorText(overlayHeader.c_str());
            ImGui::BeginGroup();
            const auto* selectedLocale = localization::FindLocaleDescriptor(
                frameSettings.overlayLocaleCode);
            ImGui::SetNextItemWidth(commonSelectWidth);
            if (ImGui::BeginCombo("##overlay_language",
                    frameSettings.overlayLocaleAuto ? "Auto (Game Language)" :
                    selectedLocale ? selectedLocale->displayName.data() : "")) {
                if (!CloseActiveComboForEscapeRequest()) {
                    const bool autoSelected = frameSettings.overlayLocaleAuto;
                    if (ImGui::Selectable("Auto (Game Language)", autoSelected)) {
                        if (ApplySetting(plugin::RuntimeSettingMutation::AutoLocale(true), logger_)) {
                            frameSettings.overlayLocaleAuto = true;
                            if (!RequestAutoLanguageSynchronization(gameWindow_) && logger_)
                                logger_("OVERLAY_AUTO_LANGUAGE_SYNC_REQUEST_FAILED");
                        }
                    }
                    if (autoSelected) ImGui::SetItemDefaultFocus();
                    for (const auto& locale : localization_.locales()) {
                        const bool isSelected =
                            !frameSettings.overlayLocaleAuto &&
                            locale.code == frameSettings.overlayLocaleCode;
                        ImFont* selectorFont = LocalizationSelectorFont(locale.fontProfileCode);
                        if (selectorFont) ImGui::PushFont(selectorFont);
                        if (ImGui::Selectable(locale.displayName.data(), isSelected)) {
                            const std::string targetLocaleCode(locale.code);
                            if (ApplySetting(plugin::RuntimeSettingMutation::LocaleCode(
                                    targetLocaleCode), logger_)) {
                                frameSettings.overlayLocaleCode = targetLocaleCode;
                                frameSettings.overlayLocaleAuto = false;
                                frameSettings.overlayFontSize = locale.initialFontSize;
                                pendingFontSizePixels_ = locale.initialFontSize;
                            }
                        }
                        if (selectorFont) ImGui::PopFont();
                        if (isSelected) ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            DrawParameterLabel(localization_, loc::setting::Language);
            ImGui::EndGroup();
            ImGui::BeginGroup();
            if (pendingFontSizePixels_ == 0)
                pendingFontSizePixels_ = frameSettings.overlayFontSize;
            ImGui::SetNextItemWidth(ImGui::CalcTextSize("  24 px  ").x +
                ImGui::GetStyle().FramePadding.x * 2.0f + 36.0f);
            ImGui::SliderInt("##overlay_font_size", &pendingFontSizePixels_,
                config::OverlayFontSizeMin, config::OverlayFontSizeMax, "%d px");
            const bool fontSizeEditFinished = ImGui::IsItemDeactivatedAfterEdit();
            if (fontSizeEditFinished && pendingFontSizePixels_ != frameSettings.overlayFontSize) {
                const auto mutation = plugin::RuntimeSettingMutation::FontSize(
                    pendingFontSizePixels_);
                if (ApplySetting(mutation, logger_)) {
                    frameSettings.overlayFontSize = pendingFontSizePixels_;
                }
            } else if (!ImGui::IsItemActive() && !fontSizeEditFinished) {
                pendingFontSizePixels_ = frameSettings.overlayFontSize;
            }
            ImGui::SameLine();
            DrawParameterLabel(localization_, loc::setting::FontSize);
            ImGui::EndGroup();
#ifdef OVERLAY_FONT_PLAYGROUND_EXPERIMENT
            ImGui::BeginGroup();
            int fontChoice = font_playground_experiment::CurrentFontChoice();
            static constexpr const char* fontChoices[] = {
                "ProggyClean + Proggy Vector Cyrillic",
                "Proggy Vector",
                "Gohu",
                "Terminus"
            };
            ImGui::SetNextItemWidth(IntrinsicComboWidth(fontChoices,
                IM_ARRAYSIZE(fontChoices)));
            if (ImGui::Combo("Font##font_playground_choice", &fontChoice,
                    fontChoices, IM_ARRAYSIZE(fontChoices)))
                font_playground_experiment::SetCurrentFontChoice(fontChoice);
            ImGui::EndGroup();
            ImGui::LabelText("##font_rasterizer", "Rasterizer", "stb Pixel Crisp");
#endif
#ifdef OVERLAY_FONT_RASTERIZATION_EXPERIMENT
            ImGui::BeginGroup();
            int rasterizationMode = font_rasterization_experiment::CurrentMode();
            static constexpr const char* rasterizationModes[] = {
                "Pixel Crisp", "Smooth 2x", "Smooth H2", "Smooth 3x"
            };
            ImGui::SetNextItemWidth(IntrinsicComboWidth(rasterizationModes,
                IM_ARRAYSIZE(rasterizationModes)));
            if (ImGui::Combo("Rasterization##font_rasterization_mode", &rasterizationMode,
                    rasterizationModes, IM_ARRAYSIZE(rasterizationModes)))
                font_rasterization_experiment::SetCurrentMode(rasterizationMode);
            ImGui::EndGroup();
#endif
            if (const auto* binding = FindHotkeyPresentation(
                    config::HotkeyBindingId::OverlayToggle))
                DrawHotkeyBinding(localization_, binding->label, binding->id,
                    frameSettings.*(binding->settingKey));

            const auto hotkeysHeader = Tr(localization_, loc::section::Hotkeys);
            ImGui::SeparatorText(hotkeysHeader.c_str());
            ImGui::BeginGroup();
            bool hotkeysEnabled = frameSettings.hotkeysEnabled;
            if (ImGui::Checkbox("##hotkeys_enabled", &hotkeysEnabled)) {
                const auto mutation = plugin::RuntimeSettingMutation::HotkeysEnabled(
                    hotkeysEnabled);
                ApplySetting(mutation, logger_);
            }
            ImGui::SameLine();
            DrawParameterLabel(localization_, loc::common::Enabled);
            ImGui::EndGroup();
            ShowHotkeysEnabledTooltip(localization_);
            for (const auto& binding : HotkeyBindingPresentations) {
                if (binding.group != HotkeyBindingGroup::Hotkeys) continue;
                DrawHotkeyBinding(localization_, binding.label, binding.id,
                    frameSettings.*(binding.settingKey));
            }

            ImGui::TableNextColumn();
            ImGui::SeparatorText("Current State");
            constexpr const char* unavailableValue = "Unavailable";
            const auto viewportAspect = semantic.runtimeViewportAspect.valid
                ? FormatAspectRatioNine(semantic.runtimeViewportAspect.value)
                : std::nullopt;
            const std::string viewportAspectValue = viewportAspect
                ? *viewportAspect : unavailableValue;
            if (BeginRuntimeTelemetryGrid("runtime_viewport_telemetry")) {
                DrawRuntimeTelemetryRow("Viewport Aspect", viewportAspectValue.c_str(),
                    localization_, loc::tooltip::runtime::Viewport,
                    !viewportAspect.has_value());
                ImGui::EndTable();
            }

            ImGui::SeparatorText("Camera Integration");
            static constexpr TooltipOption gameplayFovTooltip[] = {
                {{}, loc::camera_state::GameplayFov, loc::tooltip::runtime::GameplayFov},
                {{}, loc::common::Unavailable, loc::tooltip::runtime::GameplayFovUnavailable},
            };
            const auto formatFovFact = [this, &unavailableValue](float value,
                bool valid, camera::SnapshotFreshness freshness) {
                if (!valid) return std::string(unavailableValue);
                char numeric[32]{};
                std::snprintf(numeric, sizeof(numeric), "%.2f", value);
                const char* freshnessText = freshness == camera::SnapshotFreshness::Current
                    ? "Current" : freshness == camera::SnapshotFreshness::Retained
                        ? "Retained" : freshness == camera::SnapshotFreshness::Historical
                            ? "Historical" : unavailableValue;
                return std::string(numeric) + " (" + freshnessText + ")";
            };
            const auto gameplayFovValue = formatFovFact(integration.gameplayFov,
                integration.gameplayFovValid, integration.gameplayFovFreshness);

            static constexpr TooltipOption nativeFovTooltip[] = {
                {{}, loc::camera_state::Native, loc::tooltip::runtime::NativeFov},
                {{}, loc::common::Unavailable, loc::tooltip::runtime::NativeFovUnavailable},
            };
            const auto nativeFovValue = formatFovFact(integration.nativeFov,
                integration.nativeFovValid, integration.nativeFovFreshness);

            static constexpr TooltipOption horPlusFovTooltip[] = {
                {{}, loc::camera_state::HorPlus, loc::tooltip::runtime::HorPlusFov},
                {{}, loc::common::Unavailable, loc::tooltip::runtime::HorPlusFovUnavailable},
            };
            const auto horPlusFovValue = formatFovFact(integration.horPlusTargetFov,
                integration.horPlusTargetFovValid,
                integration.horPlusTargetFovFreshness);
            const auto currentOutputFovValue = formatFovFact(
                integration.currentOutputFov, integration.currentOutputFovValid,
                integration.currentOutputFovFreshness);

            char gameplayBoundaryFovValue[64]{};
            if (integration.gameplayBoundaryFovValid)
                std::snprintf(gameplayBoundaryFovValue,
                    sizeof(gameplayBoundaryFovValue), "%.2f",
                    integration.gameplayBoundaryFov);
            else
                std::snprintf(gameplayBoundaryFovValue,
                    sizeof(gameplayBoundaryFovValue), "%s",
                    unavailableValue);

            char cinematicFovValue[64]{};
            if (integration.cinematicEnterFovValid)
                std::snprintf(cinematicFovValue, sizeof(cinematicFovValue), "%.2f",
                    integration.cinematicEnterFov);
            else
                std::snprintf(cinematicFovValue, sizeof(cinematicFovValue), "%s",
                    unavailableValue);

            const auto nativeWriterAspect = integration.nativeWriterAspectValid
                ? FormatAspectRatioNine(integration.nativeWriterAspect)
                : std::nullopt;
            const std::string nativeWriterAspectValue =
                nativeWriterAspect ? *nativeWriterAspect : unavailableValue;
            const auto gameplayPolicyAspect = integration.horPlusPolicyAspectValid
                ? FormatAspectRatioNine(integration.horPlusPolicyAspect)
                : std::nullopt;
            const std::string gameplayPolicyAspectValue =
                gameplayPolicyAspect ? *gameplayPolicyAspect : unavailableValue;
            const auto gameplayBoundaryAspect = integration.gameplayBoundaryAspectValid
                ? FormatAspectRatioNine(integration.gameplayBoundaryAspect) : std::nullopt;
            const auto cinematicEnterAspect = integration.cinematicEnterAspectValid
                ? FormatAspectRatioNine(integration.cinematicEnterAspect) : std::nullopt;
            const std::string gameplayBoundaryAspectValue = gameplayBoundaryAspect
                ? *gameplayBoundaryAspect : unavailableValue;
            const std::string cinematicEnterAspectValue = cinematicEnterAspect
                ? *cinematicEnterAspect : unavailableValue;
            char aspectDeltaValue[48]{};
            if (integration.aspectDeltaValid)
                std::snprintf(aspectDeltaValue, sizeof(aspectDeltaValue), "%.5f",
                    integration.aspectDelta);
            else std::snprintf(aspectDeltaValue, sizeof(aspectDeltaValue), "%s",
                unavailableValue);
            char fovDeltaValue[48]{};
            if (integration.fovDeltaValid)
                std::snprintf(fovDeltaValue, sizeof(fovDeltaValue), "%.3f°",
                    integration.fovDelta);
            else std::snprintf(fovDeltaValue, sizeof(fovDeltaValue), "%s",
                unavailableValue);

            const auto assessmentValue = [](CameraValueMatch value) -> const char* {
                switch (value) {
                case CameraValueMatch::Waiting: return "Waiting";
                case CameraValueMatch::Matched: return "Match";
                case CameraValueMatch::Mismatch: return "Mismatch";
                case CameraValueMatch::CannotAssess: return "Unavailable";
                case CameraValueMatch::Unknown: return "Unknown";
                }
                return "Unknown";
            };
            const auto overallValue = [](OverallIntegrationAssessment value) -> const char* {
                switch (value) {
                case OverallIntegrationAssessment::Waiting: return "Waiting";
                case OverallIntegrationAssessment::Seamless: return "Seamless";
                case OverallIntegrationAssessment::Mismatch: return "Mismatch";
                case OverallIntegrationAssessment::CannotAssess: return "Unavailable";
                case OverallIntegrationAssessment::Unknown: return "Unknown";
                }
                return "Unknown";
            };
            const char* aspectValue = assessmentValue(integration.aspect);
            const char* fovMatchValue = assessmentValue(integration.fov);
            const char* potentialAspectValue = assessmentValue(integration.potential.aspect);
            const char* potentialFovValue = assessmentValue(integration.potential.fov);
            const char* potentialOverallValue = overallValue(integration.potential.overall);
            static constexpr TooltipOption aspectTooltip[] = {
                {{}, loc::common::Waiting, loc::transition::DetailWaiting},
                {"Match", {}, loc::tooltip::runtime::AspectMatched},
                {"Mismatch", {}, loc::tooltip::runtime::AspectMismatch},
                {"Unavailable", {}, loc::tooltip::runtime::AspectCannotAssess},
            };

            static constexpr TooltipOption cinematicAspectTooltip[] = {
                {{}, loc::setting::CinematicAspect, loc::tooltip::runtime::CinematicAspect},
                {{}, loc::common::Default, loc::tooltip::runtime::CinematicDefault},
                {{}, loc::common::Unavailable, loc::tooltip::runtime::CinematicAspectUnavailable},
            };
            const auto cinematicAspect = integration.cinematicAspectPreviewValid
                ? FormatAspectRatioNine(integration.cinematicAspectPreview)
                : std::nullopt;
            const std::string defaultValue = "Default";
            const bool showCinematicDefault = integration.cinematicEnabledKnown &&
                !integration.cinematicCorrectionEnabled;
            const std::string cinematicAspectValue = showCinematicDefault
                ? defaultValue
                : cinematicAspect ? *cinematicAspect : unavailableValue;
            std::string cinematicContextValue;
            if (integration.cinematicContext !=
                CinematicAssessmentContext::CannotAssess) {
                cinematicContextValue = integration.cinematicContext ==
                    CinematicAssessmentContext::ActiveSelection ? "Active" : "Next";
            } else {
                cinematicContextValue = unavailableValue;
            }
            const auto cinematicFovMode = integration.cinematicContext ==
                CinematicAssessmentContext::ActiveSelection &&
                semantic.activeCinematicFovMode.valid
                ? config::CinematicFovModeName(semantic.activeCinematicFovMode.value)
                : integration.cinematicContext ==
                    CinematicAssessmentContext::NextSelection &&
                    semantic.cinematicFovMode.valid
                    ? config::CinematicFovModeName(semantic.cinematicFovMode.value)
                    : nullptr;
            const std::string cinematicFovModeValue = showCinematicDefault
                ? defaultValue
                : cinematicFovMode ? cinematicFovMode : unavailableValue;
            const char* cinematicFovRowLabel = "FOV Mode";

            if (BeginRuntimeTelemetryGrid("camera_integration_telemetry")) {
                DrawRuntimeSectionStatus(localization_, "Gameplay Status",
                    ProjectGameplayPresentation(semantic));
                static constexpr TooltipOption gameplayModeTooltip[] = {
                    {"AspectRecalculation", {}, loc::tooltip::gameplay::AspectRecalculation},
                    {"HorPlus", {}, loc::tooltip::gameplay::HorPlus},
                    {"RuntimeCombine", {}, loc::tooltip::gameplay::RuntimeCombine},
                };
                DrawRuntimeTelemetryRow("Mode", config::GameplayModeName(
                    frameSettings.gameplayMode), localization_, gameplayModeTooltip);
                DrawRuntimeTelemetryRow("Writer Aspect", nativeWriterAspectValue.c_str(),
                    localization_, loc::tooltip::runtime::WriterAspect,
                    !nativeWriterAspect.has_value());
                DrawRuntimeTelemetryRow("Policy Aspect", gameplayPolicyAspectValue.c_str(),
                    localization_, loc::tooltip::runtime::PolicyAspect,
                    !gameplayPolicyAspect.has_value());

                DrawRuntimeTelemetrySubheading("FOV");
                DrawRuntimeTelemetryRow("Gameplay FOV", gameplayFovValue.c_str(),
                    localization_, gameplayFovTooltip, !integration.gameplayFovValid);
                DrawRuntimeTelemetryRow("Native FOV", nativeFovValue.c_str(),
                    localization_, nativeFovTooltip, !integration.nativeFovValid);
                DrawRuntimeTelemetryRow("HorPlus Target", horPlusFovValue.c_str(),
                    localization_, horPlusFovTooltip, !integration.horPlusTargetFovValid);
                DrawRuntimeTelemetryRow("Current Output FOV",
                    currentOutputFovValue.c_str(),
                    localization_, loc::tooltip::runtime::CurrentOutputFov,
                    !integration.currentOutputFovValid);
                DrawRuntimeTelemetrySubheading("Camera Transition");
                DrawRuntimeTelemetrySubheading("Potential");
                DrawRuntimeTelemetryRow("Aspect Match", potentialAspectValue,
                    localization_, aspectTooltip,
                    integration.potential.aspect == CameraValueMatch::CannotAssess);
                DrawRuntimeTelemetryRow("FOV Match", potentialFovValue,
                    localization_, loc::tooltip::runtime::FovMatch,
                    integration.potential.fov == CameraValueMatch::CannotAssess);
                DrawRuntimeTelemetryRow("Camera Transition", potentialOverallValue,
                    localization_, loc::tooltip::runtime::CameraTransition,
                    integration.potential.overall ==
                        OverallIntegrationAssessment::CannotAssess);

                DrawRuntimeTelemetrySubheading("Actual");
                if (integration.transitionBoundaryObserved) {
                    const char* evidenceLabel = "Last Validated";
                    if (semantic.coordinator.valid && semantic.coordinator.value ==
                        plugin::OverlayCoordinatorState::CinematicActive)
                        evidenceLabel = "Active";
                    DrawRuntimeTelemetryRow("Evidence", evidenceLabel,
                        localization_, loc::tooltip::runtime::TransitionEvidence);
                    char generationValue[32]{};
                    std::snprintf(generationValue, sizeof(generationValue), "%llu",
                        static_cast<unsigned long long>(integration.transitionGeneration));
                    DrawRuntimeTelemetryRow("Generation", generationValue,
                        localization_, loc::tooltip::runtime::TransitionGeneration);
                    DrawRuntimeTelemetryRow("Gameplay Boundary Aspect",
                        gameplayBoundaryAspectValue.c_str(), localization_,
                        loc::tooltip::runtime::GameplayBoundaryAspect,
                        !integration.gameplayBoundaryAspectValid);
                    DrawRuntimeTelemetryRow("Cinematic ENTER Aspect",
                        cinematicEnterAspectValue.c_str(), localization_,
                        loc::tooltip::runtime::CinematicEnterAspect,
                        !integration.cinematicEnterAspectValid);
                    DrawRuntimeTelemetryRow("Aspect Delta", aspectDeltaValue,
                        localization_, loc::tooltip::runtime::AspectDelta,
                        !integration.aspectDeltaValid);
                    DrawRuntimeTelemetryRow("Aspect Match", aspectValue,
                        localization_, aspectTooltip,
                        integration.aspect == CameraValueMatch::CannotAssess);
                    DrawRuntimeTelemetryRow("Gameplay Boundary FOV",
                        gameplayBoundaryFovValue, localization_,
                        loc::tooltip::runtime::GameplayBoundaryFov,
                        !integration.gameplayBoundaryFovValid);
                    DrawRuntimeTelemetryRow("Cinematic ENTER FOV", cinematicFovValue,
                        localization_, loc::tooltip::runtime::CinematicEnterFov,
                        !integration.cinematicEnterFovValid);
                    DrawRuntimeTelemetryRow("FOV Delta", fovDeltaValue,
                        localization_, loc::tooltip::runtime::FovDelta,
                        !integration.fovDeltaValid);
                    DrawRuntimeTelemetryRow("FOV Match", fovMatchValue,
                        localization_, loc::tooltip::runtime::FovMatch,
                        integration.fov == CameraValueMatch::CannotAssess);
                    DrawRuntimeTelemetryRow("Camera Transition", overallValue(integration.overall),
                        localization_, loc::tooltip::runtime::CameraTransition,
                        integration.overall == OverallIntegrationAssessment::CannotAssess);
                } else {
                    DrawRuntimeTelemetryRow("Evidence", unavailableValue,
                        localization_, loc::tooltip::runtime::TransitionEvidence, true);
                    DrawRuntimeTelemetryRow("Aspect Match", aspectValue,
                        localization_, aspectTooltip,
                        integration.aspect == CameraValueMatch::CannotAssess);
                    DrawRuntimeTelemetryRow("FOV Match", fovMatchValue,
                        localization_, loc::tooltip::runtime::FovMatch,
                        integration.fov == CameraValueMatch::CannotAssess);
                    DrawRuntimeTelemetryRow("Camera Transition", overallValue(integration.overall),
                        localization_, loc::tooltip::runtime::CameraTransition,
                        integration.overall == OverallIntegrationAssessment::CannotAssess);
                }

                DrawRuntimeTelemetrySubheading("Cinematics");
                DrawRuntimeSectionStatus(localization_, "Cinematic Status",
                    ProjectCinematicPresentation(semantic));
                DrawRuntimeTelemetryRow("Cinematic Aspect Preview",
                    cinematicAspectValue.c_str(),
                    localization_, cinematicAspectTooltip,
                    !showCinematicDefault && !cinematicAspect.has_value());
                DrawRuntimeTelemetryRow("Selection", cinematicContextValue.c_str(),
                    localization_, integration.cinematicContext ==
                        CinematicAssessmentContext::ActiveSelection
                        ? loc::transition::ActiveSelectionContext
                        : loc::transition::NextSelectionContext,
                    integration.cinematicContext ==
                        CinematicAssessmentContext::CannotAssess);
                const bool gameplayCinematicFov = cinematicFovMode &&
                    std::string_view(cinematicFovMode) == "GameplayHorPlus";
                DrawRuntimeTelemetryRow(cinematicFovRowLabel, cinematicFovModeValue.c_str(),
                    localization_, showCinematicDefault
                        ? loc::tooltip::runtime::CinematicDefault
                        : gameplayCinematicFov
                            ? loc::tooltip::cinematics::FovGameplay
                            : loc::tooltip::cinematics::FovNative,
                    !showCinematicDefault && cinematicFovMode == nullptr);
                ImGui::EndTable();
            }
            if (DrawCameraStateToggle(localization_, cameraStateExpanded_)) {
                const bool cameraStateOpen = !cameraStateExpanded_;
                if (cameraStateOpen) {
                    cameraStateCompactPosition_ = clampedPosition;
                    cameraStateCompactPositionValid_ = true;
                } else if (cameraStateCompactPositionValid_) {
                    restoreCameraStateCompactPosition_ = true;
                }
                cameraStateExpanded_ = cameraStateOpen;
                cameraStateLayoutChanged = true;
            }
            if (expandedLayout) {
                ImGui::TableNextColumn();
                ImGui::SeparatorText("Camera State");
                const float panelHeight = (std::max)(0.0f,
                    ImGui::GetContentRegionAvail().y);
                if (ImGui::BeginChild("camera_state_panel_scroll",
                        ImVec2(0.0f, panelHeight), true,
                        ImGuiWindowFlags_NoSavedSettings)) {
                    DrawCameraStateView(semantic, integration);
                }
                ImGui::EndChild();
            }
                ImGui::EndTable();
            }
        }
#else
#endif
            ImGui::End();
        }
        DrawNotifications();
        PublishOverlayPopupState(ImGui::IsPopupOpen(
            static_cast<ImGuiID>(0), ImGuiPopupFlags_AnyPopup));
        ImGui::Render();
        const ImDrawData* drawData = ImGui::GetDrawData();
        if (drawData) {
            submittedDrawLists_.reserve(static_cast<std::size_t>(
                (std::max)(0, drawData->CmdListsCount)));
            for (int index = 0; index < drawData->CmdListsCount; ++index) {
                const ImDrawList* list = drawData->CmdLists[index];
                if (list && list->VtxBuffer.Size > 0)
                    submittedDrawLists_.push_back({
                        reinterpret_cast<std::uintptr_t>(list),
                        static_cast<std::size_t>(list->VtxBuffer.Size)});
            }
            for (const auto& notification : notifications_) {
                if (!notification.frameDrawListToken ||
                    submittedNotificationCount_ >=
                        submittedNotificationIds_.size())
                    continue;
                if (NotificationDrawListWasSubmitted(
                        notification.frameDrawListToken, submittedDrawLists_)) {
                    submittedNotificationIds_[submittedNotificationCount_++] =
                        notification.notification.id;
                    if (notification.notification.kind ==
                            plugin::OverlayNotificationKind::StartupOverlayHint &&
                        !startupHintDrawSubmissionLogged_) {
                        startupHintDrawSubmissionLogged_ = true;
                        LogNotificationLifecycle("toast_draw_submitted",
                            "imgui_drawlist_has_vertices",
                            notification.notification.id);
                    }
                }
            }
        }
        const presentation::Rect surfaceExtent{0, 0,
            static_cast<int>(width), static_cast<int>(height)};
        presentation::Rect currentBounds{};
        if (!ComputeImGuiDrawBounds(drawData, width, height, currentBounds)) {
            currentBounds = surfaceExtent;
            forceFullDamage_ = true;
        }
        if (!imguiRenderer_.SupportsPartialSurfaceUpdates())
            currentBounds = surfaceExtent;
        const presentation::Rect damage = damageTracker_.Plan(currentBounds,
            surfaceExtent, forceFullDamage_);
        if (!damage.empty() && !DrawImGuiSurface(targetSurface, width, height,
                drawData, damage)) {
            const HRESULT removedReason = device_ ? device_->GetDeviceRemovedReason() : E_FAIL;
            Log("OVERLAY_DCOMP_DRAW_FAILED hr=" +
                std::to_string(static_cast<long>(removedReason)));
            if (FAILED(removedReason)) {
                if (state_.BeginDeviceRecovery()) {
                    Log("OVERLAY_DCOMP_DEVICE_LOST recovery=started");
                    if (RecoverGraphicsDevice(width, height)) return false;
                    state_.CompleteDeviceRecovery(false, {});
                    Disable("dcomp_device_lost_recovery_failed");
                } else {
                    Disable("dcomp_device_lost");
                }
            } else {
                Disable("dcomp_surface_draw_failed");
            }
            return false;
        }
        damageTracker_.Publish(currentBounds, surfaceExtent);
        menuFrameAdmission_.DrawCompleted(visible());
        forceFullDamage_ = false;
        surfaceDirty_ = cameraStateLayoutChanged;
        if (!firstFrameLogged_) {
            firstFrameLogged_ = true;
            LogDiagnostic(logger_, "OVERLAY_FIRST_FRAME");
#if defined(OVERLAY_STARTUP_JOURNAL)
            diagnostics::startup_journal::MarkOnce(9, "OVERLAY_FIRST_FRAME",
                "first_directcomposition_surface_commit", surfaceGeneration_);
            diagnostics::startup_journal::Flush();
#endif
        }
        return true;
    }

    void Renderer::Disable(const char* reason) noexcept
    {
        menuFrameAdmission_.Request(false);
        visible_.store(false, std::memory_order_release);
        PublishOverlayPopupState(false);
        try {
            GetInputState().Close();
        } catch (...) {
            // The atomic visibility gate is closed; continue owner handoff.
        }
        try {
            Log(std::string("OVERLAY_RENDER_DISABLED reason=") + reason);
        } catch (...) {
            // Diagnostics are optional; terminal cleanup is not.
        }
#if defined(OVERLAY_STARTUP_JOURNAL)
        try {
            diagnostics::startup_journal::MarkOnce(10, "OVERLAY_DISABLED", reason);
            diagnostics::startup_journal::Flush();
        } catch (...) {
            // A journal failure must not prevent terminal owner handoff.
        }
#endif
        state_.DisableOverlay();
        if (terminalRequest_) terminalRequest_();
    }

    void Renderer::FailAfterException() noexcept
    {
        Disable("overlay_exception");
    }

    bool Renderer::Shutdown() noexcept
    {
        menuFrameAdmission_.Request(false);
        visible_.store(false, std::memory_order_release);
        PublishOverlayPopupState(false);
        try {
            GetInputState().Close();
        } catch (...) {
            // Input admission is already terminal; continue independent teardown.
        }
        try {
            plugin::SetHotkeyRebindCaptureActive(false);
            plugin::MarkHotkeyRebindKeyConsumed(0);
        } catch (...) {
            // Rebind state is optional and cannot block resource finalization.
        }
        const bool hidden = !overlayWindow_ || !IsWindow(overlayWindow_) ||
            SetOverlayWindowVisibility(false);
        if (!hidden) {
            overlayWindowVisible_ = IsWindowVisible(overlayWindow_) != FALSE;
            try { Log("OVERLAY_DISPLAY_HIDE_UNCONFIRMED"); }
            catch (...) {}
        }
        DetachImGui();
        if (!ResetComposition()) return false;
        gameWindow_ = nullptr;
        overlayWindow_ = nullptr;
        overlayPlacement_.valid = false;
        hostActive_ = false;
        hostMinimized_ = false;
        surfaceGeneration_ = 0;
        firstFrameLogged_ = false;
        surfaceDirty_ = true;
        return OwnedResourcesReleased();
    }
}
