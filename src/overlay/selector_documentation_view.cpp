#include "selector_documentation_view.hpp"

#include "localization_keys.hpp"
#include "selector_examples_geometry.hpp"
#include "tooltip_row_layout.hpp"

#include <imgui.h>

#include <algorithm>
#include <string>

namespace overlay
{
    namespace
    {
        inline constexpr float NextOptionGapAtBaseScale = 12.0f;

        std::string Tr(const LocalizationManager& i18n, loc::Key key)
        {
            return i18n.Text(key);
        }

        void DrawTooltipExamplesBlock(const LocalizationManager& i18n,
            const setting_tooltip_content::Option& option)
        {
            if (option.examples.empty())
                return;

            const float fontSize = ImGui::GetFontSize();
            const auto heading = Tr(i18n, loc::common::Examples);
            const auto context = option.examplesContext.path.empty()
                ? std::string{} : Tr(i18n, option.examplesContext);
            float contentWidth = ImGui::CalcTextSize(heading.c_str()).x;
            if (!context.empty())
                contentWidth = (std::max)(contentWidth, ImGui::CalcTextSize(context.c_str()).x);
            for (const auto key : option.examples) {
                const auto example = Tr(i18n, key);
                contentWidth = (std::max)(contentWidth,
                    ImGui::CalcTextSize(example.c_str()).x);
            }
            const float maxContentWidth = fontSize *
                tooltip_row_layout::WrapWidthInFontUnits;
            contentWidth = (std::clamp)(contentWidth, fontSize * 18.0f,
                maxContentWidth);

            const float uiScale = selector_examples_geometry::UiScale(fontSize);
            const float outerGap = 6.0f * uiScale;
            const float headingContextGap = 2.0f * uiScale;
            const float contextExamplesGap = 2.0f * uiScale;
            ImGui::Dummy(ImVec2(0.0f, outerGap));
            const ImVec2 frameMin = ImGui::GetCursorScreenPos();
            const auto geometry = selector_examples_geometry::CalculateFrameGeometry(
                frameMin, contentWidth, 0.0f, fontSize);

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->ChannelsSplit(2);
            drawList->ChannelsSetCurrent(1);
            ImGui::SetCursorScreenPos(geometry.contentMin);
            // Cursor positioning establishes the initial inset. Indent keeps
            // subsequent wrapped text on that same content column; it is
            // restored before measuring the explicit frame rectangle.
            const float horizontalInset = geometry.contentMin.x - frameMin.x;
            ImGui::Indent(horizontalInset);
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + contentWidth);
            ImGui::TextDisabled("%s", heading.c_str());
            if (!context.empty()) {
                ImGui::Dummy(ImVec2(0.0f, headingContextGap));
                ImGui::TextDisabled("%s", context.c_str());
            }
            ImGui::Dummy(ImVec2(0.0f, contextExamplesGap));
            for (const auto key : option.examples) {
                const auto example = Tr(i18n, key);
                ImGui::TextWrapped("%s", example.c_str());
            }
            ImGui::PopTextWrapPos();
            ImGui::Unindent(horizontalInset);
            const float contentBottom = ImGui::GetItemRectMax().y;
            const auto completedGeometry =
                selector_examples_geometry::CalculateFrameGeometry(frameMin,
                    contentWidth, contentBottom - geometry.contentMin.y, fontSize);

            drawList->ChannelsSetCurrent(0);
            drawList->AddRectFilled(completedGeometry.frameMin,
                completedGeometry.frameMax,
                IM_COL32(18, 20, 24, 255), 2.0f);
            drawList->AddRect(completedGeometry.frameMin,
                completedGeometry.frameMax,
                IM_COL32(74, 78, 88, 220), 2.0f, 0, 1.0f);
            drawList->ChannelsMerge();
            ImGui::SetCursorScreenPos(frameMin);
            ImGui::Dummy(ImVec2(completedGeometry.frameMax.x - frameMin.x,
                completedGeometry.frameMax.y - frameMin.y));
        }
    }

    void ShowSettingOptionsTooltip(const LocalizationManager& i18n,
        std::span<const setting_tooltip_content::Option> options,
        SelectorInteractionState selectorState)
    {
        // The caller passes the state captured on the selector item itself.
        // Do not query LastItemData here: the caller has already rendered its
        // group/label, which changes the item ImGui considers last.
        if (!IsSelectorTooltipEligible(selectorState) || !ImGui::BeginTooltip())
            return;

        // Explicit tooltip gaps below are the sole vertical spacing source.
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
            ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));
        tooltip_row_layout::DrawAlignedTooltipRows(options.size(),
            [&options](std::size_t index) {
                return options[index].canonicalName;
            },
            [&i18n, &options](std::size_t index, float descriptionX) {
                const auto& option = options[index];
                for (std::size_t descriptionIndex = 0;
                    descriptionIndex < option.descriptions.size(); ++descriptionIndex) {
                    const auto description = Tr(i18n, option.descriptions[descriptionIndex]);
                    if (descriptionIndex > 0) {
                        ImGui::Spacing();
                        ImGui::SetCursorPosX(descriptionX);
                    }
                    ImGui::TextWrapped("%s", description.c_str());
                }
            },
            [&i18n, &options](std::size_t index) {
                const auto& option = options[index];
                if (option.examples.empty())
                    return;
                DrawTooltipExamplesBlock(i18n, options[index]);
                if (index + 1 < options.size())
                    ImGui::Dummy(ImVec2(0.0f,
                        NextOptionGapAtBaseScale * selector_examples_geometry::UiScale(
                            ImGui::GetFontSize())));
            });
        ImGui::PopStyleVar();
        ImGui::EndTooltip();
    }
}
