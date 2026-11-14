/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScenePanel
*/

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <glm/geometric.hpp>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "Panels.hpp"
#include "aylo/contract/renderer/IRenderer.hpp"
#include "aylo/contract/renderer/drawing/Lines3D.hpp"
#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "aylo/contract/renderer/view/CameraView.hpp"
#include "aylo/contract/renderer/view/CameraView3D.hpp"
#include "aylo/contract/renderer/view/ScenePreview.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/resource/Resources.hpp"
#include "aylo/ecs/world/World.hpp"
#include "editor/EditorState.hpp"
#include "editor/gui/GuiMesh.hpp"
#include "editor/gui/Text.hpp"
#include "editor/scene/Gizmo.hpp"
#include "editor/scene/Picking.hpp"
#include "editor/scene/SceneCamera.hpp"
#include "editor/scene/SceneEdit.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/ComponentValues.hpp"
#include "plugins/transform/Transform.hpp"

namespace aylo::editor::panels {

namespace {
namespace transform = plugins::transform;
using scene::GizmoHandle;
using scene::GizmoTool;
using scene::SceneMode;

constexpr float kHandlePixels = 80.0F;    ///< About how long a gizmo handle is on screen.
constexpr float kHandleTolerance = 7.0F;  ///< How near a handle the mouse grabs it, in pixels.
constexpr float kPickTolerance = 16.0F;   ///< 2D: how near an entity's origin a click picks it, in pixels.
constexpr float kFrameRadius = 1.0F;      ///< The size F frames a selection as.
constexpr int kGridHalf = 20;             ///< 3D: grid lines on each side of the pivot.
constexpr std::array<ImU32, 3> kAxisColors{IM_COL32(235, 80, 80, 255), IM_COL32(110, 210, 90, 255),
                                           IM_COL32(80, 140, 240, 255)};
constexpr ImU32 kActiveColor = IM_COL32(250, 220, 70, 255);  ///< A handle grabbed or under the mouse.

/// @return @p length window units as framebuffer pixels, at least 1.
std::uint32_t toPixels(float length, float scale) {
    return static_cast<std::uint32_t>(std::max(1.0F, std::round(length * scale)));
}

/// @brief What is selected, and how it was found.
struct Selection {
    ecs::Entity entity;                 ///< The entity the gizmo moves.
    std::optional<std::uint32_t> node;  ///< While editing, its node.
};

/// @return The capsule edited, if any.
plugins::capsule::Capsule* editedCapsule(EditorState& state) {
    const auto id = state.getEdited();
    auto capsules = state.world->getResources().getMut<plugins::capsule::Capsules>();
    if (!id || !capsules || capsules->find(*id) == nullptr) {
        return nullptr;
    }
    return &capsules->edit(*id);
}

/// @return What the gizmo moves: the entity selected while playing, the selected node's entity while editing.
std::optional<Selection> selectionOf(EditorState& state) {
    if (state.play.isPlaying()) {
        if (state.world->isAlive(state.selected)) {
            return Selection{.entity = state.selected};
        }
        return std::nullopt;
    }
    const plugins::capsule::Capsule* capsule = editedCapsule(state);
    if (capsule == nullptr || !state.selectedNode) {
        return std::nullopt;
    }
    const std::optional<ecs::Entity> entity = scene::entityOfNode(*state.world, *capsule, *state.selectedNode);
    if (!entity) {
        return std::nullopt;
    }
    return Selection{.entity = *entity, .node = state.selectedNode};
}

/// @brief Selects @p entity: itself while playing, its node while editing.
void select(EditorState& state, std::optional<ecs::Entity> entity) {
    if (state.play.isPlaying()) {
        state.selected = entity.value_or(ecs::Entity{});
        return;
    }
    const plugins::capsule::Capsule* capsule = editedCapsule(state);
    if (capsule == nullptr) {
        return;
    }
    state.selectedNode = entity ? scene::nodeOfEntity(*state.world, *capsule, *entity) : std::nullopt;
}

/// @return The grid of the 3D world around the pivot, on the ground (y = 0), and the X and Z axes.
std::vector<contract::renderer::Lines3D> gridAround(const scene::SceneCamera& camera) {
    contract::renderer::Lines3D grid{.color = {0.45F, 0.45F, 0.5F, 1.0F}};
    contract::renderer::Lines3D axisX{.color = {0.85F, 0.3F, 0.3F, 1.0F}};
    contract::renderer::Lines3D axisZ{.color = {0.3F, 0.5F, 0.9F, 1.0F}};
    const float baseX = std::round(camera.pivot.x);
    const float baseZ = std::round(camera.pivot.z);
    const auto extent = static_cast<float>(kGridHalf);
    for (int line = -kGridHalf; line <= kGridHalf; ++line) {
        const float x = baseX + static_cast<float>(line);
        const float z = baseZ + static_cast<float>(line);
        contract::renderer::Lines3D& alongZ = x == 0.0F ? axisZ : grid;
        alongZ.points.emplace_back(x, 0.0F, baseZ - extent);
        alongZ.points.emplace_back(x, 0.0F, baseZ + extent);
        contract::renderer::Lines3D& alongX = z == 0.0F ? axisX : grid;
        alongX.points.emplace_back(baseX - extent, 0.0F, z);
        alongX.points.emplace_back(baseX + extent, 0.0F, z);
    }
    return {std::move(grid), std::move(axisX), std::move(axisZ)};
}

/// @brief The picture the panel shows, and how to go between its points and the world.
struct Picture {
    ImVec2 corner;                                           ///< Its top-left corner, in window units.
    ImVec2 size;                                             ///< Its size, in window units.
    float scale{1.0F};                                       ///< Framebuffer pixels per window unit.
    std::optional<contract::renderer::CameraView3D> view3D;  ///< 3D: its view.
    std::optional<contract::renderer::CameraView> view2D;    ///< 2D: its view.

    /// @return Where @p point (window units) is in the picture's framebuffer pixels.
    [[nodiscard]] glm::vec2 toPixels(ImVec2 point) const {
        return {(point.x - corner.x) * scale, (point.y - corner.y) * scale};
    }
    /// @return Where @p pixels of the picture are in window units.
    [[nodiscard]] ImVec2 toWindow(glm::vec2 pixels) const {
        return {corner.x + (pixels.x / scale), corner.y + (pixels.y / scale)};
    }
};

/// @return The 3D gizmo's handles at @p entity, in window units; none when it is behind the camera.
std::optional<std::array<GizmoHandle, 3>> handles3D(const EditorState& state, const Picture& picture,
                                                    const contract::renderer::CameraView3D& view, ecs::Entity entity) {
    const auto* pose = state.world->getComponents().get<transform::GlobalTransform3D>(entity);
    const std::optional<glm::vec2> origin = pose != nullptr ? view.worldToScreen(pose->position) : std::nullopt;
    if (!origin) {
        return std::nullopt;
    }
    const float length = glm::length(state.sceneCamera.position() - pose->position) * 0.15F;
    std::array<GizmoHandle, 3> handles{};
    for (std::uint8_t axis = 0; axis < 3; ++axis) {
        const glm::vec3 end =
            pose->position + glm::vec3{axis == 0 ? length : 0.0F, axis == 1 ? length : 0.0F, axis == 2 ? length : 0.0F};
        const std::optional<glm::vec2> tip = view.worldToScreen(end);
        if (!tip) {
            return std::nullopt;
        }
        const ImVec2 from = picture.toWindow(*origin);
        const ImVec2 to = picture.toWindow(*tip);
        handles.at(axis) =
            GizmoHandle{.axis = axis, .from = {from.x, from.y}, .to = {to.x, to.y}, .worldLength = length};
    }
    return handles;
}

/// @return The 2D gizmo's handles (X and Y; the third is unused) at @p entity, in window units.
std::optional<std::array<GizmoHandle, 3>> handles2D(const EditorState& state, const Picture& picture,
                                                    const contract::renderer::CameraView& view, ecs::Entity entity) {
    const auto* pose = state.world->getComponents().get<transform::GlobalTransform2D>(entity);
    if (pose == nullptr) {
        return std::nullopt;
    }
    const float length = kHandlePixels / std::max(state.sceneCamera.zoom, 0.01F);
    const ImVec2 from = picture.toWindow(view.worldToScreen(pose->position));
    std::array<GizmoHandle, 3> handles{};
    for (std::uint8_t axis = 0; axis < 2; ++axis) {
        const glm::vec2 end = pose->position + glm::vec2{axis == 0 ? length : 0.0F, axis == 1 ? length : 0.0F};
        const ImVec2 to = picture.toWindow(view.worldToScreen(end));
        handles.at(axis) =
            GizmoHandle{.axis = axis, .from = {from.x, from.y}, .to = {to.x, to.y}, .worldLength = length};
    }
    return handles;
}

/// @return The gizmo's handles at the selection, in window units, and how many there are (3 in 3D, 2 in 2D); none
/// when it cannot be shown.
std::optional<std::array<GizmoHandle, 3>> handlesOf(const EditorState& state, const Picture& picture,
                                                    ecs::Entity entity, std::size_t& count) {
    if (picture.view3D) {
        count = 3;
        return handles3D(state, picture, *picture.view3D, entity);
    }
    count = 2;
    return picture.view2D ? handles2D(state, picture, *picture.view2D, entity) : std::nullopt;
}

/// @brief Draws the gizmo's handles, the one under the mouse or dragged in another colour.
void drawGizmo(ImDrawList& list, GizmoTool tool, std::span<const GizmoHandle> handles,
               std::optional<std::uint8_t> hot) {
    for (const GizmoHandle& handle : handles) {
        const ImU32 color = hot == handle.axis ? kActiveColor : kAxisColors.at(handle.axis);
        const ImVec2 from{handle.from.x, handle.from.y};
        const ImVec2 to{handle.to.x, handle.to.y};
        list.AddLine(from, to, color, 3.0F);
        switch (tool) {
            case GizmoTool::kMove:
                list.AddCircleFilled(to, 6.0F, color);
                break;
            case GizmoTool::kRotate:
                list.AddCircle(to, 8.0F, color, 0, 2.5F);
                break;
            case GizmoTool::kScale:
                list.AddRectFilled(ImVec2{to.x - 5.0F, to.y - 5.0F}, ImVec2{to.x + 5.0F, to.y + 5.0F}, color);
                break;
        }
    }
}

/// @brief Moves the camera with the mouse and the keys, as Unity's Scene view does.
/// @return Whether the right mouse button is down (fly mode), so the tool keys are left alone.
bool moveCamera(EditorState& state, const Picture& picture, bool hovered) {
    const ImGuiIO& io = ImGui::GetIO();
    scene::SceneCamera& camera = state.sceneCamera;
    const glm::vec2 delta{io.MouseDelta.x, io.MouseDelta.y};
    const bool flying = camera.mode == SceneMode::k3D && ImGui::IsMouseDown(ImGuiMouseButton_Right) &&
                        (hovered || ImGui::IsMouseDragging(ImGuiMouseButton_Right));
    if (flying) {
        camera.look(delta);
        glm::vec3 direction{0.0F};
        direction.x =
            static_cast<float>(ImGui::IsKeyDown(ImGuiKey_D)) - static_cast<float>(ImGui::IsKeyDown(ImGuiKey_A));
        direction.y =
            static_cast<float>(ImGui::IsKeyDown(ImGuiKey_E)) - static_cast<float>(ImGui::IsKeyDown(ImGuiKey_Q));
        direction.z =
            static_cast<float>(ImGui::IsKeyDown(ImGuiKey_W)) - static_cast<float>(ImGui::IsKeyDown(ImGuiKey_S));
        camera.fly(direction, io.DeltaTime, io.KeyShift);
    }
    if (!hovered && !ImGui::IsMouseDragging(ImGuiMouseButton_Middle) &&
        !ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        return flying;
    }
    if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
        camera.pan(delta, picture.size.y);
    }
    if (io.KeyAlt && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !state.sceneDrag) {
        if (camera.mode == SceneMode::k3D) {
            camera.orbit(delta);
        } else {
            camera.pan(delta, picture.size.y);
        }
    }
    if (hovered && io.MouseWheel != 0.0F) {
        camera.dolly(io.MouseWheel);
    }
    return flying;
}

/// @brief Ends the drag: while editing, writes the entity's Transform to its node and respawns the scene.
void endDrag(EditorState& state) {
    const std::optional<SceneDrag> drag = std::exchange(state.sceneDrag, std::nullopt);
    if (!drag || !drag->node || state.play.isPlaying()) {
        return;
    }
    plugins::capsule::Capsule* capsule = editedCapsule(state);
    if (capsule == nullptr || *drag->node >= capsule->getNodes().size()) {
        return;
    }
    ecs::Components& components = state.world->getComponents();
    plugins::capsule::ComponentValues& values = capsule->editNode(*drag->node).values;
    if (drag->spatial) {
        if (const auto* moved = components.get<transform::Transform3D>(drag->entity)) {
            values.set(components, *moved);
        }
    } else if (const auto* moved = components.get<transform::Transform2D>(drag->entity)) {
        values.set(components, *moved);
    }
    scene::respawn(*state.world, capsule->getId());
    state.markEditedUnsaved();
}

/// @brief The gizmo and the click: grabs a handle, drags it, or selects what is under the mouse.
void useGizmo(EditorState& state, const Picture& picture, bool hovered) {
    ImDrawList& list = *ImGui::GetWindowDrawList();
    const ImGuiIO& io = ImGui::GetIO();
    const glm::vec2 mouse{io.MousePos.x, io.MousePos.y};
    const std::optional<Selection> selection = selectionOf(state);
    std::size_t count = 0;
    const auto handles = selection ? handlesOf(state, picture, selection->entity, count) : std::nullopt;
    const std::span<const GizmoHandle> shown =
        handles ? std::span{handles->data(), count} : std::span<const GizmoHandle>{};
    std::optional<std::uint8_t> hot =
        state.sceneDrag ? std::optional{state.sceneDrag->handle.axis} : scene::hitGizmo(shown, mouse, kHandleTolerance);
    if (!hovered && !state.sceneDrag) {
        hot.reset();
    }
    drawGizmo(list, state.gizmoTool, shown, hot);

    ecs::Components& components = state.world->getComponents();
    if (state.sceneDrag) {
        const SceneDrag& drag = *state.sceneDrag;
        const glm::vec2 moved = mouse - glm::vec2{drag.startX, drag.startY};
        if (drag.spatial) {
            static_cast<void>(
                components.add(drag.entity, scene::dragGizmo(state.gizmoTool, drag.handle, drag.start3D, moved)));
        } else {
            static_cast<void>(
                components.add(drag.entity, scene::dragGizmo2D(state.gizmoTool, drag.handle, drag.start2D, moved)));
        }
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            endDrag(state);
        }
        return;
    }
    if (!hovered || io.KeyAlt || !ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        return;
    }
    if (hot && selection) {
        SceneDrag drag{.entity = selection->entity,
                       .handle = *std::next(shown.begin(), *hot),
                       .spatial = picture.view3D.has_value(),
                       .startX = mouse.x,
                       .startY = mouse.y,
                       .node = selection->node};
        const auto* local3D = components.get<transform::Transform3D>(selection->entity);
        const auto* local2D = components.get<transform::Transform2D>(selection->entity);
        if (drag.spatial && local3D != nullptr) {
            drag.start3D = *local3D;
            state.sceneDrag = drag;
        } else if (!drag.spatial && local2D != nullptr) {
            drag.start2D = *local2D;
            state.sceneDrag = drag;
        }
        return;
    }
    const glm::vec2 pixels = picture.toPixels(io.MousePos);
    if (picture.view3D) {
        select(state, scene::pick3D(*state.world, *picture.view3D, pixels));
    } else if (picture.view2D) {
        select(state, scene::pick2D(*state.world, *picture.view2D, pixels, kPickTolerance * picture.scale));
    }
}

/// @brief W, E, R pick the tool and F frames the selection, while the panel has the keyboard.
void useKeys(EditorState& state, bool flying) {
    if (flying || !ImGui::IsWindowFocused() || ImGui::GetIO().WantTextInput) {
        return;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_W, false)) {
        state.gizmoTool = GizmoTool::kMove;
    } else if (ImGui::IsKeyPressed(ImGuiKey_E, false)) {
        state.gizmoTool = GizmoTool::kRotate;
    } else if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
        state.gizmoTool = GizmoTool::kScale;
    }
    if (!ImGui::IsKeyPressed(ImGuiKey_F, false)) {
        return;
    }
    if (const std::optional<Selection> selection = selectionOf(state)) {
        const ecs::Components& components = state.world->getComponents();
        if (const auto* pose = components.get<transform::GlobalTransform3D>(selection->entity);
            pose != nullptr && state.sceneCamera.mode == SceneMode::k3D) {
            state.sceneCamera.frame(pose->position,
                                    kFrameRadius * std::max({pose->scale.x, pose->scale.y, pose->scale.z}));
        } else if (const auto* flat = components.get<transform::GlobalTransform2D>(selection->entity)) {
            state.sceneCamera.frame(glm::vec3{flat->position, 0.0F}, kFrameRadius);
        }
    }
}

/// @brief The row over the picture: 2D or 3D, and the tool.
void drawToolRow(EditorState& state) {
    ImGui::SetCursorPos(ImVec2{8.0F, 6.0F});
    scene::SceneCamera& camera = state.sceneCamera;
    if (ImGui::RadioButton("2D", camera.mode == SceneMode::k2D)) {
        camera.mode = SceneMode::k2D;
        state.sceneDrag.reset();
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("3D", camera.mode == SceneMode::k3D)) {
        camera.mode = SceneMode::k3D;
        state.sceneDrag.reset();
    }
    ImGui::SameLine(0.0F, 18.0F);
    if (ImGui::RadioButton("Move (W)", state.gizmoTool == GizmoTool::kMove)) {
        state.gizmoTool = GizmoTool::kMove;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate (E)", state.gizmoTool == GizmoTool::kRotate)) {
        state.gizmoTool = GizmoTool::kRotate;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale (R)", state.gizmoTool == GizmoTool::kScale)) {
        state.gizmoTool = GizmoTool::kScale;
    }
    ImGui::SameLine(0.0F, 18.0F);
    gui::textDisabled(camera.mode == SceneMode::k3D
                          ? "right mouse + WASD/QE: fly, middle: pan, Alt + left: orbit, wheel: closer, F: frame"
                          : "middle mouse or Alt + left: pan, wheel: zoom, F: frame");
}
}  // namespace

void drawScenePanel(EditorState& state, contract::renderer::IRenderer& renderer) {
    if (!state.showScene) {
        state.sceneDrag.reset();
        return;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    const bool visible =
        ImGui::Begin("Scene", &state.showScene, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();
    if (!visible) {
        ImGui::End();
        return;
    }
    drawToolRow(state);
    ImGui::Dummy(ImVec2{0.0F, 4.0F});
    ImGui::SetCursorPosX(0.0F);
    const ImVec2 available = ImGui::GetContentRegionAvail();
    if (state.previewUsed || available.x < 1.0F || available.y < 1.0F) {
        if (state.previewUsed) {
            gui::textDisabled("The preview shows an Actor tab: put this panel in front of it.");
        } else {
            ImGui::Dummy(ImVec2{0.0F, 0.0F});
        }
        ImGui::End();
        return;
    }
    const float scale = ImGui::GetIO().DisplayFramebufferScale.x;
    const glm::uvec2 size{toPixels(available.x, scale),
                          toPixels(available.y, ImGui::GetIO().DisplayFramebufferScale.y)};
    const scene::SceneCamera& camera = state.sceneCamera;
    contract::renderer::ScenePreview preview{.view = camera.view2D(scale), .size = size};
    Picture picture{.corner = ImGui::GetCursorScreenPos(), .size = available, .scale = scale};
    if (camera.mode == SceneMode::k3D && renderer.supports3D()) {
        preview.view3D = camera.view3D();
        preview.lines = gridAround(camera);
        preview.world2D = false;
        picture.view3D.emplace(*preview.view3D, glm::vec2{0.0F}, size);
    } else {
        picture.view2D.emplace(preview.view, glm::vec2{0.0F}, size);
    }
    renderer.setScenePreview(preview);
    state.previewUsed = true;
    const contract::renderer::TextureId texture = renderer.getPreviewTexture();
    if (texture == contract::renderer::TextureId{}) {
        ImGui::Dummy(available);
        ImGui::End();
        return;
    }
    ImGui::Image(ImTextureRef{gui::toImGuiTexture(texture)}, available);
    const bool hovered = ImGui::IsItemHovered();
    if (hovered && (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle))) {
        ImGui::SetWindowFocus();  // the keys (WASD, F, W/E/R) then reach this panel
    }
    const bool flying = moveCamera(state, picture, hovered);
    useKeys(state, flying);
    useGizmo(state, picture, hovered);
    ImGui::End();
}

}  // namespace aylo::editor::panels
