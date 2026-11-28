/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** BehavioursPanel
*/

#include <imgui.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>

#include "AssetKind.hpp"
#include "Panels.hpp"
#include "editor/EditorState.hpp"
#include "editor/behaviour/BehaviourCatalog.hpp"
#include "editor/gui/Text.hpp"
#include "editor/shell/TextEditor.hpp"
#include "plugins/capsule/Capsule.hpp"

namespace aylo::editor::panels {

namespace {
constexpr double kRefresh = 1.0;  ///< Seconds between two reads of the catalog: the scripts and capsules can change.

/// @return The hooks of @p entry, comma separated.
std::string hookList(const behaviour::BehaviourEntry& entry) {
    std::string list;
    for (const std::string_view hook : entry.hooks) {
        list += list.empty() ? "" : ", ";
        list += hook;
    }
    return list;
}

/// @brief Opens the capsule of @p carrier, a scene as the level and anything else alone, on its node.
void openCarrier(EditorState& state, const behaviour::BehaviourCarrier& carrier) {
    if (std::filesystem::path{carrier.capsule}.extension() == ".scene") {
        openScene(state, carrier.capsule);
    } else {
        openPrefab(state, carrier.capsule);
    }
    if (state.getEdited() == plugins::capsule::CapsuleId::fromPath(carrier.capsule)) {
        state.selectedNode = carrier.node;
    }
}

/// @brief Draws one Behaviour: its hooks, its script, where it runs.
void drawEntry(EditorState& state, const behaviour::BehaviourEntry& entry) {
    const std::string title = std::format("{}  {}", toUtf8(iconOf(AssetKind::kBehaviour)), entry.name);
    if (!ImGui::CollapsingHeader(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }
    ImGui::PushID(entry.name.c_str());
    ImGui::Indent();
    gui::textDisabled(std::format("Hooks: {}", hookList(entry)));
    if (entry.file) {
        if (ImGui::SmallButton("Open script") && !shell::openInTextEditor(state.textEditor, *entry.file)) {
            state.status = std::format("Could not run '{}'", state.textEditor);
        }
        ImGui::SameLine();
        gui::textDisabled(entry.file->lexically_relative(state.assets.parent_path()).generic_string());
    } else {
        gui::textDisabled("Its file was not found in scripts/behaviours/");
    }
    if (entry.carriers.empty()) {
        gui::textDisabled("No capsule carries it: it runs only on the entities a script gives it.");
    } else {
        ImGui::TextUnformatted("Carried by:");
        for (std::size_t index = 0; index < entry.carriers.size(); ++index) {
            const behaviour::BehaviourCarrier& carrier = entry.carriers.at(index);
            ImGui::PushID(static_cast<int>(index));
            const std::string label = std::format("{}  {}  {}", toUtf8(iconOf(kindOf(carrier.capsule, false))),
                                                  carrier.capsule, carrier.nodePath);
            if (ImGui::Selectable(label.c_str())) {
                openCarrier(state, carrier);
            }
            if (ImGui::IsItemHovered()) {
                gui::tooltip("Open the capsule on this node");
            }
            ImGui::PopID();
        }
    }
    ImGui::TextUnformatted(std::format("{} in the world now", entry.live).c_str());
    if (state.play.isPlaying() && entry.live > 0) {
        ImGui::SameLine();
        if (ImGui::SmallButton("Show in Hierarchy")) {
            state.filter.fill('\0');
            std::copy_n(entry.name.begin(), std::min(entry.name.size(), state.filter.size() - 1), state.filter.begin());
        }
    }
    ImGui::Unindent();
    ImGui::PopID();
}
}  // namespace

void drawBehavioursPanel(EditorState& state) {
    if (!state.showBehaviours) {
        return;
    }
    if (ImGui::Begin("Behaviours", &state.showBehaviours)) {
        const double now = ImGui::GetTime();
        if (state.behavioursRead < 0.0 || now - state.behavioursRead > kRefresh) {
            state.behaviours = behaviour::catalogBehaviours(*state.app, state.assets);
            state.behavioursRead = now;
        }
        if (state.behaviours.empty()) {
            gui::textDisabled("No Behaviour yet: right-click in the Assets panel, New Behaviour.");
        }
        for (const behaviour::BehaviourEntry& entry : state.behaviours) {
            drawEntry(state, entry);
        }
    }
    ImGui::End();
}

}  // namespace aylo::editor::panels
