/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Text
*/

#include "Text.hpp"

#include <imgui.h>

#include <string>

namespace aylo::editor::gui {

void textDisabled(const std::string& text) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::TextUnformatted(text.c_str());
    ImGui::PopStyleColor();
}

void tooltip(const std::string& text) {
    if (ImGui::BeginTooltip()) {
        ImGui::TextUnformatted(text.c_str());
        ImGui::EndTooltip();
    }
}

}  // namespace aylo::editor::gui
