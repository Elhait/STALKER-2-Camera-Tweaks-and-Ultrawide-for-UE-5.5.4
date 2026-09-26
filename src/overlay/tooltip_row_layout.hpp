#pragma once

#include <imgui.h>

#include <algorithm>
#include <cstddef>

namespace overlay::tooltip_row_layout
{
    inline constexpr float WrapWidthInFontUnits = 42.0f;

    template <typename NameAt, typename DrawDescriptionAt, typename DrawDetailsAt>
    void DrawAlignedTooltipRows(std::size_t count, NameAt&& nameAt,
        DrawDescriptionAt&& drawDescriptionAt, DrawDetailsAt&& drawDetailsAt)
    {
        const float fontSize = ImGui::GetFontSize();
        ImGui::PushTextWrapPos(fontSize * WrapWidthInFontUnits);
        float longestNameWidth = 0.0f;
        for (std::size_t index = 0; index < count; ++index) {
            const auto name = nameAt(index);
            if (name.empty())
                continue;
            const float nameWidth = ImGui::CalcTextSize(name.data(),
                name.data() + name.size()).x;
            longestNameWidth = (std::max)(longestNameWidth, nameWidth);
        }
        const float nameColumnX = ImGui::GetCursorPosX();
        const float separatorX = nameColumnX + longestNameWidth + fontSize * 0.5f;
        const float descriptionX = separatorX + fontSize * 1.25f;
        for (std::size_t index = 0; index < count; ++index) {
            const auto name = nameAt(index);
            if (!name.empty()) {
                ImGui::TextUnformatted(name.data(), name.data() + name.size());
                ImGui::SameLine(separatorX);
                ImGui::TextUnformatted("-");
                ImGui::SameLine(descriptionX);
            }
            drawDescriptionAt(index, descriptionX);
            drawDetailsAt(index);
        }
        ImGui::PopTextWrapPos();
    }
}
