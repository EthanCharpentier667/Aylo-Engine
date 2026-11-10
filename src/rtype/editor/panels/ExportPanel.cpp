/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ExportPanel
*/

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <string>

#include "Panels.hpp"
#include "editor/EditorSession.hpp"
#include "editor/EditorState.hpp"
#include "editor/shipping/GameExport.hpp"

namespace rtype::editor::panels {
namespace {
/// @brief Copies @p text into the buffer @p buffer, cut to fit with its terminator.
template <std::size_t Size>
void fill(std::array<char, Size>& buffer, const std::string& text) {
    const std::size_t count = std::min(text.size(), Size - 1);
    std::copy_n(text.begin(), count, buffer.begin());
    buffer.at(count) = '\0';
}

/// @return @p bytes in megabytes, with one decimal.
std::string megabytes(const std::uintmax_t bytes) {
    constexpr double kMegabyte = 1024.0 * 1024.0;
    return std::format("{:.1f} MB", static_cast<double>(bytes) / kMegabyte);
}

/// @brief Shows what the last export did.
void drawResult(const shipping::ExportResult& result) {
    ImGui::Separator();
    if (!result.ok) {
        ImGui::TextColored(ImVec4{1.0F, 0.45F, 0.4F, 1.0F}, "Export failed");
        ImGui::TextWrapped("%s", result.error.c_str());
        return;
    }
    ImGui::TextUnformatted(std::format("Exported to {} ({})", result.folder, megabytes(result.bytes)).c_str());
    for (const std::string& warning : result.warnings) {
        ImGui::TextColored(ImVec4{1.0F, 0.8F, 0.3F, 1.0F}, "warning:");
        ImGui::SameLine();
        ImGui::TextWrapped("%s", warning.c_str());
    }
    if (ImGui::CollapsingHeader(std::format("{} libraries kept", result.libraries.size()).c_str(),
                                ImGuiTreeNodeFlags_DefaultOpen)) {
        for (const std::string& library : result.libraries) {
            ImGui::BulletText("%s", library.c_str());
        }
    }
}
}  // namespace

void drawExportPanel(EditorState& state) {
    if (!state.showExport || !state.session) {
        return;
    }
    shipping::ExportPanelState& panel = state.session->exportPanel;
    // By default next to the game folder: `<folder of the game>/exports`.
    if (panel.output.front() == '\0' && !state.assets.empty()) {
        fill(panel.output, (state.assets.parent_path().parent_path() / "exports").generic_string());
    }
    ImGui::SetNextWindowSize(ImVec2{560.0F, 0.0F}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Export", &state.showExport)) {
        ImGui::TextWrapped(
            "Makes the folder a player runs: the program, only the plugins and backends your project.luau needs, and "
            "your game in one pack, as tools/aylo-export does.");
        ImGui::InputText("Output folder", panel.output.data(), panel.output.size());
        ImGui::InputTextWithHint("Name", "the game folder's", panel.name.data(), panel.name.size());
        ImGui::Checkbox("One pack (game.pak)", &panel.pack);
        if (ImGui::Button("Export")) {
            panel.last =
                shipping::runExport(state.session->exporter,
                                    {.output = panel.output.data(), .name = panel.name.data(), .pack = panel.pack});
            state.status = panel.last->ok ? std::format("Exported to {}", panel.last->folder) : "Export failed";
        }
        if (panel.last) {
            drawResult(*panel.last);
        }
    }
    ImGui::End();
}
}  // namespace rtype::editor::panels
