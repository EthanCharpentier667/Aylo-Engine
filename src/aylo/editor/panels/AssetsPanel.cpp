/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AssetsPanel
*/

#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "AssetKind.hpp"
#include "FieldEditor.hpp"
#include "Panels.hpp"
#include "aylo/ecs/resource/Resources.hpp"
#include "aylo/ecs/world/World.hpp"
#include "editor/EditorState.hpp"
#include "editor/behaviour/BehaviourFiles.hpp"
#include "editor/gui/Text.hpp"
#include "editor/scene/Isolation.hpp"
#include "editor/scene/SceneEdit.hpp"
#include "editor/shell/TextEditor.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/format/CapsuleText.hpp"
#include "plugins/shader/MaterialFile.hpp"

namespace aylo::editor::panels {

namespace {
namespace capsule = plugins::capsule;
constexpr const char* kPopup = "Asset##popup";  ///< The popup every action of the panel goes through.
constexpr float kMinIcon = 32.0F;               ///< The smallest icon the slider gives, in pixels.
constexpr float kMaxIcon = 128.0F;              ///< The biggest.
constexpr float kIconScale = 0.8F;              ///< The glyph's size relative to its cell, so it fits with margin.

/// @return The folder of the game @p root is: its assets or its scripts.
const std::filesystem::path& rootPath(const EditorState& state, AssetRoot root) {
    return root == AssetRoot::kScripts ? state.scripts : state.assets;
}

/// @return The name the panel gives @p root.
const char* rootName(AssetRoot root) { return root == AssetRoot::kScripts ? "scripts" : "assets"; }

/// @return @p path in @p root, with forward slashes; "" for the root itself.
std::string relativeOf(const EditorState& state, AssetRoot root, const std::filesystem::path& path) {
    const std::string relative = path.lexically_relative(rootPath(state, root)).generic_string();
    return relative == "." ? std::string{} : relative;
}

/// @return The kind of the entry @p relative of @p root: a script by what it is to the game, any other file by its
/// extension.
AssetKind kindIn(const EditorState& state, AssetRoot root, const std::string& relative, bool isFolder) {
    return root == AssetRoot::kScripts ? scriptKindOf(relative, state.scriptEntries, isFolder)
                                       : kindOf(relative, isFolder);
}

/// @return Whether @p kind is Luau, which a double-click opens in the text editor.
bool isLuau(AssetKind kind) {
    return kind == AssetKind::kScript || kind == AssetKind::kBehaviour || kind == AssetKind::kEntry ||
           kind == AssetKind::kTypes;
}

/// @brief Opens @p file in the text editor; why not goes to the status line.
void openText(EditorState& state, const std::filesystem::path& file) {
    if (!shell::openInTextEditor(state.textEditor, file)) {
        state.status = std::format("Could not run '{}'", state.textEditor);
    }
}

/// @return @p folder and @p name joined, in the assets.
std::string join(std::string_view folder, std::string_view name) {
    return folder.empty() ? std::string{name} : std::format("{}/{}", folder, name);
}

/// @brief Asks for the popup of @p action on @p target, in @p root.
void request(EditorState& state, AssetRoot root, AssetAction action, std::string target, std::string_view text = {}) {
    state.assetAction = action;
    state.assetActionRoot = root;
    state.assetTarget = std::move(target);
    state.textField.fill('\0');
    std::memcpy(state.textField.data(), text.data(), std::min(text.size(), state.textField.size() - 1));
    state.openAssetPopup = true;
}

/// @brief Stops editing the capsule @p id, renamed or deleted: its prefab mode ends, or its scene closes.
void forget(EditorState& state, capsule::CapsuleId id) {
    closeActor(state, id);
    if (state.scene == id) {
        state.scene.reset();
        state.selectedNode.reset();
    }
}

/// @return Whether the capsules know @p path, reading it first if they do not; why not goes to the status line.
bool ensureLoaded(EditorState& state, const std::string& path) {
    const auto* capsules = state.world->getResources().get<capsule::Capsules>();
    if (capsules == nullptr) {
        state.status = "The game has no capsules: list aylo.Capsule in its plugins";
        return false;
    }
    if (capsules->find(capsule::CapsuleId::fromPath(path)) != nullptr) {
        return true;
    }
    if (const std::vector<std::string> errors = scene::readFiles(*state.world, std::span{&path, 1}); !errors.empty()) {
        state.status = errors.front();
        return false;
    }
    return true;
}

/// @brief The menu of a folder of @p root: what can be made in it.
void folderMenu(EditorState& state, AssetRoot root, const std::string& folder) {
    if (root == AssetRoot::kAssets) {
        if (ImGui::MenuItem("New scene")) {
            request(state, root, AssetAction::kNewScene, folder, "level");
        }
        if (ImGui::MenuItem("New actor")) {
            request(state, root, AssetAction::kNewActor, folder, "actor");
        }
        if (ImGui::MenuItem("New material")) {
            request(state, root, AssetAction::kNewMaterial, folder, "material");
        }
    } else if (ImGui::MenuItem("New script")) {
        request(state, root, AssetAction::kNewScript, folder, "module");
    }
    if (ImGui::MenuItem("New folder")) {
        request(state, root, AssetAction::kNewFolder, folder, "folder");
    }
    ImGui::Separator();
    if (ImGui::MenuItem("New Behaviour")) {
        request(state, root, AssetAction::kNewBehaviour, folder, "Spinner");
    }
    if (ImGui::IsItemHovered()) {
        gui::tooltip("A script to attach to nodes, written in scripts/behaviours/ and opened in the text editor");
    }
}

/// @return The entries of @p folder but the hidden ones, folders first, by name.
std::vector<std::filesystem::directory_entry> listFolder(const std::filesystem::path& folder) {
    std::error_code error;
    std::vector<std::filesystem::directory_entry> entries;
    for (const auto& entry : std::filesystem::directory_iterator{folder, error}) {
        if (!entry.path().filename().string().starts_with('.')) {
            entries.push_back(entry);
        }
    }
    std::ranges::sort(entries, [](const auto& left, const auto& right) {
        if (left.is_directory() != right.is_directory()) {
            return left.is_directory();
        }
        return left.path().filename() < right.path().filename();
    });
    return entries;
}

/// @brief Opens the capsule file @p relative: a scene as the level; an actor, or any other capsule, alone (prefab
/// mode).
void openCapsule(EditorState& state, const std::string& relative) {
    if (std::filesystem::path{relative}.extension() == ".scene") {
        openScene(state, relative);
    } else {
        openPrefab(state, relative);
    }
}

/// @brief The menu of the file @p relative of @p root, named @p name.
void fileMenu(EditorState& state, AssetRoot root, const std::string& relative, const std::string& name,
              bool isCapsule) {
    const bool isScene = std::filesystem::path{relative}.extension() == ".scene";
    if (isCapsule && ImGui::MenuItem(isScene ? "Open" : "Open alone")) {
        openCapsule(state, relative);
    }
    if (ImGui::MenuItem("Open in text editor")) {
        openText(state, rootPath(state, root) / relative);
    }
    if (ImGui::MenuItem("Rename")) {
        request(state, root, AssetAction::kRename, relative, name);
    }
    if (ImGui::MenuItem("Delete")) {
        request(state, root, AssetAction::kDelete, relative);
    }
}

/// @return The color @p rgb (0xRRGGBB) as ImGui wants it.
ImU32 toImColor(std::uint32_t rgb) {
    constexpr float kByte = 255.0F;
    return ImGui::ColorConvertFloat4ToU32(ImVec4{static_cast<float>((rgb >> 16U) & 0xFFU) / kByte,
                                                 static_cast<float>((rgb >> 8U) & 0xFFU) / kByte,
                                                 static_cast<float>(rgb & 0xFFU) / kByte, 1.0F});
}

/// @brief Shows the folder @p relative of @p root in the content area, with nothing selected.
void enter(EditorState& state, AssetRoot root, const std::string& relative) {
    state.assetRoot = root;
    state.assetFolder = relative;
    state.assetSelected.clear();
}

/// @return The flags of a node of the folder tree.
// NOLINTBEGIN(bugprone-signed-bitwise): ImGui's flags are ints.
ImGuiTreeNodeFlags treeFlags(bool isLeaf, bool isShown, ImGuiTreeNodeFlags extra = 0) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow | extra;
    if (isLeaf) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    if (isShown) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    return flags;
}
// NOLINTEND(bugprone-signed-bitwise)

/// @brief The menu of the folder @p relative of @p root, named @p name, right-clicked.
void folderItemMenu(EditorState& state, AssetRoot root, const std::string& relative, const std::string& name) {
    folderMenu(state, root, relative);
    ImGui::Separator();
    if (ImGui::MenuItem("Rename")) {
        request(state, root, AssetAction::kRename, relative, name);
    }
    if (ImGui::MenuItem("Delete")) {
        request(state, root, AssetAction::kDelete, relative);
    }
}

/// @brief Draws the tree of the folders under @p folder of @p root (the files are in the content area), the one shown
/// selected.
// NOLINTNEXTLINE(misc-no-recursion): the depth is the depth of the game's folders.
void drawTree(EditorState& state, AssetRoot root, const std::filesystem::path& folder) {
    const bool browsed = root == state.assetRoot;
    for (const auto& entry : listFolder(folder)) {
        if (!entry.is_directory()) {
            continue;
        }
        const std::string relative = relativeOf(state, root, entry.path());
        const std::string name = entry.path().filename().string();
        const bool isShown = browsed && relative == state.assetFolder;
        const bool isLeaf =
            std::ranges::none_of(listFolder(entry.path()), [](const auto& child) { return child.is_directory(); });
        const bool onPath = browsed && state.assetFolder.starts_with(relative + "/");
        ImGui::PushID(relative.c_str());
        if (onPath) {
            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        }
        const std::string label = std::format("{}  {}", toUtf8(iconOf(AssetKind::kFolder, isShown)), name);
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_Text));
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg): a constant format.
        const bool open = ImGui::TreeNodeEx("##folder", treeFlags(isLeaf, isShown), "%s", label.c_str());
        ImGui::PopStyleColor();
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            enter(state, root, relative);
        }
        if (ImGui::BeginPopupContextItem()) {
            folderItemMenu(state, root, relative, name);
            ImGui::EndPopup();
        }
        if (open && !isLeaf) {
            drawTree(state, root, entry.path());
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}

/// @brief The folders on the way to the folder shown, as buttons that go back to them, and the icon size slider.
void drawPathBar(EditorState& state) {
    if (ImGui::SmallButton(toUtf8(U'\uf062').c_str()) && !state.assetFolder.empty()) {
        enter(state, state.assetRoot, std::filesystem::path{state.assetFolder}.parent_path().generic_string());
    }
    ImGui::SameLine();
    std::optional<std::string> target;
    bool first = true;
    const auto crumb = [&](const std::string& label, const std::string& path) {
        if (!first) {
            ImGui::SameLine(0.0F, 2.0F);
            ImGui::TextUnformatted(toUtf8(U'\uf054').c_str());
            ImGui::SameLine(0.0F, 2.0F);
        }
        first = false;
        ImGui::PushID(path.c_str());
        if (ImGui::SmallButton(label.c_str())) {
            target = path;
        }
        ImGui::PopID();
    };
    crumb(rootName(state.assetRoot), "");
    std::string path;
    for (const auto& part : std::filesystem::path{state.assetFolder}) {
        path = join(path, part.string());
        crumb(part.string(), path);
    }
    if (target) {
        enter(state, state.assetRoot, *target);
    }
    constexpr float kSliderWidth = 90.0F;
    ImGui::SameLine(ImGui::GetContentRegionMax().x - kSliderWidth);
    ImGui::SetNextItemWidth(kSliderWidth);
    ImGui::SliderFloat("##iconSize", &state.assetIconSize, kMinIcon, kMaxIcon, "");
}

/// @brief Draws one entry of the folder shown as an icon with its name: a click selects it, a double-click enters a
/// folder, opens a capsule or a script, a drag carries an asset's path, a right-click opens its menu.
void drawItem(EditorState& state, const std::filesystem::directory_entry& entry, const ImVec2& cell) {
    const AssetRoot root = state.assetRoot;
    const bool inAssets = root == AssetRoot::kAssets;
    const std::string relative = relativeOf(state, root, entry.path());
    const std::string name = entry.path().filename().string();
    const bool isFolder = entry.is_directory();
    const AssetKind kind = kindIn(state, root, relative, isFolder);
    const bool isCapsule = inAssets && !isFolder && capsule::format::isCapsuleFile(relative);
    const bool isOpen = isCapsule && state.getEdited() == capsule::CapsuleId::fromPath(relative);
    ImGui::PushID(relative.c_str());
    ImGui::Selectable("##item", relative == state.assetSelected || isOpen, ImGuiSelectableFlags_AllowDoubleClick, cell);
    const ImVec2 min = ImGui::GetItemRectMin();
    if (ImGui::IsItemClicked()) {
        state.assetSelected = relative;
        if (inAssets && kind == AssetKind::kMaterial && state.inspectedMaterial != relative) {
            state.inspectedMaterial = relative;  // the Inspector shows it, as Unity does
            state.materialUnsaved = false;
            state.materialError.clear();
        }
    }
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        if (isFolder) {
            enter(state, root, relative);
        } else if (isCapsule) {
            openCapsule(state, relative);
        } else if (isLuau(kind)) {
            openText(state, entry.path());
        }
    }
    if (ImGui::IsItemHovered() && !isFolder) {
        gui::tooltip(std::format("{}: {}/{}", labelOf(kind), rootName(root), relative));
    }
    if (inAssets && !isFolder && ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload(kAssetPayload, relative.data(), relative.size());
        ImGui::TextUnformatted(std::format("{}  {}", toUtf8(iconOf(kind)), relative).c_str());
        ImGui::EndDragDropSource();
    }
    if (ImGui::BeginPopupContextItem()) {
        state.assetSelected = relative;
        if (isFolder) {
            if (ImGui::MenuItem("Open")) {
                enter(state, root, relative);
            }
            ImGui::Separator();
            folderItemMenu(state, root, relative, name);
        } else {
            fileMenu(state, root, relative, name, isCapsule);
        }
        ImGui::EndPopup();
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImFont* font = ImGui::GetFont();
    const float iconSize = state.assetIconSize * kIconScale;
    const std::string icon = toUtf8(iconOf(kind));
    const ImVec2 iconExtent = font->CalcTextSizeA(iconSize, FLT_MAX, 0.0F, icon.c_str());
    const ImVec2 iconPos{min.x + ((cell.x - iconExtent.x) * 0.5F),
                         min.y + ((state.assetIconSize - iconExtent.y) * 0.5F) + 4.0F};
    draw->AddText(font, iconSize, iconPos, toImColor(colorOf(kind)), icon.c_str());
    const float labelY = min.y + state.assetIconSize + 6.0F;
    const ImVec2 labelExtent = ImGui::CalcTextSize(name.c_str());
    const float labelX = labelExtent.x < cell.x - 4.0F ? min.x + ((cell.x - labelExtent.x) * 0.5F) : min.x + 2.0F;
    draw->PushClipRect({min.x + 2.0F, labelY}, {min.x + cell.x - 2.0F, labelY + ImGui::GetTextLineHeight()}, true);
    draw->AddText({labelX, labelY}, ImGui::GetColorU32(ImGuiCol_Text), name.c_str());
    draw->PopClipRect();
    ImGui::PopID();
}

/// @brief The content area: the files and folders of the folder shown, as a grid of icons.
void drawGrid(EditorState& state) {
    const std::filesystem::path folder = rootPath(state, state.assetRoot) / state.assetFolder;
    const std::vector<std::filesystem::directory_entry> entries = listFolder(folder);
    const ImVec2 cell{state.assetIconSize + 20.0F, state.assetIconSize + ImGui::GetTextLineHeight() + 14.0F};
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const int columns =
        std::max(1, static_cast<int>((ImGui::GetContentRegionAvail().x + spacing) / (cell.x + spacing)));
    int index = 0;
    for (const auto& entry : entries) {
        if (index % columns != 0) {
            ImGui::SameLine();
        }
        drawItem(state, entry, cell);
        ++index;
    }
    if (entries.empty()) {
        gui::textDisabled("This folder is empty. Right-click to make something.");
    }
    // Clicking the empty space selects nothing; right-clicking it makes something in the folder.
    ImGui::SetCursorPos(ImVec2{0.0F, 0.0F});
    if (ImGui::BeginPopupContextWindow("##background",
                                       ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
        folderMenu(state, state.assetRoot, state.assetFolder);
        ImGui::EndPopup();
    }
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
        state.assetSelected.clear();
    }
}

/// @brief The explorer: the folders in a narrow tree on the left, the folder shown as a grid of icons on the right.
void drawExplorer(EditorState& state) {
    std::error_code error;
    if (!std::filesystem::is_directory(rootPath(state, state.assetRoot) / state.assetFolder, error)) {
        enter(state, AssetRoot::kAssets, "");  // it was renamed or deleted
    }
    constexpr float kTreeWidth = 180.0F;
    // NOLINTBEGIN(bugprone-signed-bitwise): ImGui's flags are ints.
    constexpr ImGuiTableFlags kTableFlags =
        ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoSavedSettings;
    // NOLINTEND(bugprone-signed-bitwise)
    if (!ImGui::BeginTable("##explorer", 2, kTableFlags)) {
        return;
    }
    ImGui::TableSetupColumn("##tree", ImGuiTableColumnFlags_WidthFixed, kTreeWidth);
    ImGui::TableSetupColumn("##content", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    if (ImGui::BeginChild("##tree", ImVec2{0.0F, 0.0F})) {
        for (const AssetRoot root : {AssetRoot::kAssets, AssetRoot::kScripts}) {
            if (!std::filesystem::is_directory(rootPath(state, root), error)) {
                continue;  // a game with no scripts folder yet
            }
            const bool rootShown = state.assetRoot == root && state.assetFolder.empty();
            const std::string label =
                std::format("{}  {}", toUtf8(iconOf(AssetKind::kFolder, rootShown)), rootName(root));
            ImGui::PushID(rootName(root));
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg): a constant format.
            if (ImGui::TreeNodeEx("##root", treeFlags(false, rootShown, ImGuiTreeNodeFlags_DefaultOpen), "%s",
                                  label.c_str())) {
                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                    enter(state, root, "");
                }
                if (ImGui::BeginPopupContextItem()) {
                    folderMenu(state, root, "");
                    ImGui::EndPopup();
                }
                drawTree(state, root, rootPath(state, root));
                ImGui::TreePop();
            } else if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                enter(state, root, "");
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
    ImGui::TableSetColumnIndex(1);
    drawPathBar(state);
    ImGui::Separator();
    if (ImGui::BeginChild("##items", ImVec2{0.0F, 0.0F})) {
        drawGrid(state);
    }
    ImGui::EndChild();
    ImGui::EndTable();
}

/// @brief Writes the module @p text in the folder of `scripts/` the popup is on, and opens it.
void newScript(EditorState& state, const std::filesystem::path& base, const std::string& text) {
    const std::string path = join(state.assetTarget, text.ends_with(".luau") ? text : text + ".luau");
    if (std::filesystem::exists(base / path)) {
        state.status = std::format("scripts/{} exists already", path);
        return;
    }
    std::error_code error;
    std::filesystem::create_directories((base / path).parent_path(), error);
    std::ofstream{base / path} << std::format(
        "-- scripts/{}: a module. Another file runs it with require(\"@scripts/{}\").\n\nlocal Module = {{}}\n\n"
        "return Module\n",
        path, std::filesystem::path{path}.replace_extension().generic_string());
    openText(state, base / path);
}

/// @brief Writes the material @p text in the folder of the assets the popup is on, and inspects it.
void newMaterial(EditorState& state, const std::string& text) {
    const std::string_view extension = plugins::shader::kMaterialExtension;
    const std::string path = join(state.assetTarget, text.ends_with(extension) ? text : text + std::string{extension});
    if (std::filesystem::exists(state.assets / path)) {
        state.status = std::format("{} exists already", path);
        return;
    }
    std::ofstream{state.assets / path} << plugins::shader::MaterialFile{}.write();
    state.inspectedMaterial = path;
    state.materialUnsaved = false;
    state.materialError.clear();
}

/// @brief Writes the Behaviour @p text in `scripts/behaviours/`, and opens it.
void newBehaviour(EditorState& state, const std::string& text) {
    std::filesystem::path file;
    if (std::string failure = behaviour::createBehaviour(behaviour::behavioursFolder(state.assets), text, file);
        !failure.empty()) {
        state.status = std::move(failure);
        return;
    }
    const std::string name = file.stem().string();
    state.status = std::format("Wrote scripts/behaviours/{}.luau: game.{} is in Add component once the game reloads it",
                               name, name);
    if (!shell::openInTextEditor(state.textEditor, file)) {
        state.status = std::format("Wrote {}, but could not run '{}'", file.generic_string(), state.textEditor);
    }
}

/// @brief Writes the scene or actor @p text in the folder of the assets the popup is on, and opens it.
void newCapsule(EditorState& state, const std::string& text) {
    const std::string_view extension = state.assetAction == AssetAction::kNewScene ? ".scene" : ".actor";
    const std::string path = join(state.assetTarget, text.ends_with(extension) ? text : text + std::string{extension});
    if (const auto failure = scene::createCapsuleFile(*state.world, state.assets, path)) {
        state.status = *failure;
        return;
    }
    openScene(state, path);
}

/// @brief Does what the popup asked for, once confirmed.
void runAction(EditorState& state) {
    std::error_code error;
    const std::string text{state.textField.data()};
    const bool inAssets = state.assetActionRoot == AssetRoot::kAssets;
    const std::filesystem::path& base = rootPath(state, state.assetActionRoot);
    const std::filesystem::path target = base / state.assetTarget;
    switch (state.assetAction) {
        case AssetAction::kNewScene:
        case AssetAction::kNewActor:
            newCapsule(state, text);
            return;
        case AssetAction::kNewFolder:
            std::filesystem::create_directories(base / join(state.assetTarget, text), error);
            break;
        case AssetAction::kNewScript:
            newScript(state, base, text);
            return;
        case AssetAction::kNewMaterial:
            newMaterial(state, text);
            return;
        case AssetAction::kNewBehaviour:
            newBehaviour(state, text);
            return;
        case AssetAction::kRename: {
            const std::filesystem::path renamed = target.parent_path() / text;
            std::filesystem::rename(target, renamed, error);
            if (!error && inAssets && capsule::format::isCapsuleFile(renamed)) {
                const std::string path = relativeOf(state, AssetRoot::kAssets, renamed);
                static_cast<void>(scene::readFiles(*state.world, std::span{&path, 1}));
            }
            if (!error && inAssets) {
                forget(state, capsule::CapsuleId::fromPath(state.assetTarget));  // its capsule keeps the old path
            }
            break;
        }
        case AssetAction::kDelete:
            std::filesystem::remove_all(target, error);
            if (!error && inAssets) {
                forget(state, capsule::CapsuleId::fromPath(state.assetTarget));
            }
            break;
        case AssetAction::kNone:
            return;
    }
    state.status = error ? std::format("'{}': {}", state.assetTarget, error.message()) : std::string{};
}

/// @return What the popup asks to type for @p action.
const char* nameLabel(AssetAction action) {
    if (action == AssetAction::kRename) {
        return "New name";
    }
    if (action == AssetAction::kNewScript) {
        return "Name (a module, in this folder of scripts/)";
    }
    return action == AssetAction::kNewBehaviour ? "Name (game.<Name>, in scripts/behaviours/)" : "Name";
}

/// @brief The popup of the panel's action: a name to type, or a deletion to confirm.
void drawPopup(EditorState& state) {
    if (state.openAssetPopup) {
        ImGui::OpenPopup(kPopup);
        state.openAssetPopup = false;
    }
    if (!ImGui::BeginPopupModal(kPopup, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }
    bool confirmed = false;
    if (state.assetAction == AssetAction::kDelete) {
        ImGui::TextUnformatted(std::format("Delete '{}'? This cannot be undone.", state.assetTarget).c_str());
    } else {
        ImGui::TextUnformatted(nameLabel(state.assetAction));
        if (ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
        }
        confirmed = ImGui::InputText("##name", state.textField.data(), state.textField.size(),
                                     ImGuiInputTextFlags_EnterReturnsTrue);
    }
    confirmed = ImGui::Button("OK") || confirmed;
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        state.assetAction = AssetAction::kNone;
        ImGui::CloseCurrentPopup();
    }
    if (confirmed && (state.assetAction == AssetAction::kDelete || state.textField.front() != '\0')) {
        runAction(state);
        state.assetAction = AssetAction::kNone;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}
}  // namespace

void openScene(EditorState& state, const std::string& path) {
    if (state.play.isPlaying()) {
        state.status = "Stop the game before opening a scene";
        return;
    }
    if (!ensureLoaded(state, path)) {
        return;
    }
    closePrefab(state);
    const capsule::CapsuleId id = capsule::CapsuleId::fromPath(path);
    if (const auto error = scene::open(*state.world, id)) {
        state.status = *error;
        return;
    }
    state.scene = id;
    state.selectedNode = 0;
    state.status = std::format("Editing {}", path);
}

void openPrefab(EditorState& state, const std::string& path) {
    if (state.play.isPlaying()) {
        state.status = "Stop the game before opening a capsule";
        return;
    }
    if (!ensureLoaded(state, path)) {
        return;
    }
    const capsule::CapsuleId id = capsule::CapsuleId::fromPath(path);
    const bool known = std::ranges::find(state.actors, id) != state.actors.end();
    if (!state.prefab || state.prefab->capsule != id) {
        if (state.prefab) {
            scene::endIsolation(*state.world, *state.prefab);
            state.prefab.reset();
        }
        std::string error;
        auto isolation = scene::isolate(*state.world, id, error);
        if (!isolation) {
            state.status = error;
            if (known) {
                std::erase(state.actors, id);
            }
            return;
        }
        state.prefab = *isolation;
        state.selectedNode = 0;
    }
    if (!known) {
        state.actors.push_back(id);
    }
    state.focusActor = id;
    state.dockActor = !known;
    state.status = std::format("Editing {} alone", path);
}

void closeActor(EditorState& state, capsule::CapsuleId id) {
    std::erase(state.actors, id);
    if (state.focusActor == id) {
        state.focusActor.reset();
    }
    if (state.prefab && state.prefab->capsule == id) {
        scene::endIsolation(*state.world, *state.prefab);
        state.prefab.reset();
        state.selectedNode = state.scene ? std::optional<std::uint32_t>{0} : std::nullopt;
    }
}

void closePrefab(EditorState& state) {
    state.actors.clear();
    state.focusActor.reset();
    if (!state.prefab) {
        return;
    }
    scene::endIsolation(*state.world, *state.prefab);
    state.prefab.reset();
    state.selectedNode = state.scene ? std::optional<std::uint32_t>{0} : std::nullopt;
}

void drawAssetsPanel(EditorState& state) {
    if (!state.showAssets) {
        return;
    }
    if (ImGui::Begin("Assets", &state.showAssets)) {
        if (state.assets.empty()) {
            ImGui::TextUnformatted("The game has no assets folder.");
        } else {
            drawExplorer(state);
        }
        drawPopup(state);
    }
    ImGui::End();
}

}  // namespace aylo::editor::panels
