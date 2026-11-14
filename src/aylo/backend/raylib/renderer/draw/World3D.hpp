/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** World3D
*/

#pragma once

#include <raylib.h>

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <optional>
#include <vector>

#include "aylo/contract/renderer/drawing/Color.hpp"
#include "aylo/contract/renderer/drawing/Lines3D.hpp"
#include "aylo/contract/renderer/drawing/StaticMesh.hpp"
#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "aylo/contract/renderer/view/CameraView3D.hpp"
#include "aylo/contract/renderer/view/View3D.hpp"
#include "backend/raylib/renderer/TextureTable.hpp"

namespace aylo::backend::raylib::renderer::draw {

/// @brief The 3D world of the raylib renderer: its static meshes, and the frame's view, lines and mesh draws.
///
/// @details raylib draws at once, while the contract asks for the 3D world under the 2D one, whenever in the frame its
/// draws come: they are kept, and draw() draws them all with the depth test, in the 3D view's letterboxed viewport,
/// before the first 2D view of the frame (the RaylibRenderer calls it then), and once more at the end of the frame for
/// what came later (the physics debug view's lines, drawn then over the 2D world). Meshes are unlit, as in Vulkan:
/// each part's base colour times its texture times the draw's colour.
class World3D {
  public:
    World3D() = default;
    /// @brief Frees the meshes left, if the window (and its OpenGL context) is still open.
    ~World3D();
    World3D(const World3D&) = delete;
    World3D& operator=(const World3D&) = delete;
    World3D(World3D&&) = delete;
    World3D& operator=(World3D&&) = delete;

    /// @brief Uploads a model: one raylib mesh per part, without indices (raylib's are 16 bits).
    /// @throws std::invalid_argument If an index names no vertex or a part goes past the indices.
    [[nodiscard]] contract::renderer::StaticMeshId create(const contract::renderer::StaticMeshDesc& desc);
    /// @brief Frees a model; does nothing for an unknown or destroyed id.
    void destroy(contract::renderer::StaticMeshId id);

    /// @brief Starts a frame: no view, nothing kept.
    void begin();
    /// @brief Sets the frame's view, for a framebuffer of @p framebuffer pixels and a game of @p viewSize.
    void setView(const contract::renderer::View3D& view, glm::vec2 viewSize, glm::uvec2 framebuffer);
    /// @brief Keeps lines for the next draw().
    void add(const contract::renderer::Lines3D& lines);
    /// @brief Keeps a mesh draw for the next draw().
    void add(const contract::renderer::StaticMeshDraw& draw);
    /// @brief Draws what was kept since the last call, then forgets it; nothing without a view. Called outside the 2D
    /// camera and its clip.
    void draw(const TextureTable& textures, glm::uvec2 framebuffer);

  private:
    /// @brief A part of a model, uploaded.
    struct Part {
        ::Mesh mesh{};                          ///< Its triangles, unindexed.
        contract::renderer::Color baseColor;    ///< Its colour.
        contract::renderer::TextureId texture;  ///< Its texture; none: white.
    };
    /// @brief A segment, already in the world.
    struct Segment {
        glm::vec3 from;                   ///< Where it starts.
        glm::vec3 to;                     ///< Where it ends.
        contract::renderer::Color color;  ///< Its colour.
    };

    /// @return The parts of @p id, or nullptr for an unknown or destroyed one.
    [[nodiscard]] const std::vector<Part>* find(contract::renderer::StaticMeshId id) const;
    /// @brief Draws the kept meshes.
    void drawMeshes(const TextureTable& textures);

    std::vector<std::optional<std::vector<Part>>> _meshes;   ///< By StaticMeshId::index; empty: a free slot.
    std::vector<std::uint32_t> _generations;                 ///< By StaticMeshId::index.
    std::vector<std::uint32_t> _freeSlots;                   ///< Slots to reuse.
    std::optional<contract::renderer::CameraView3D> _view;   ///< The frame's view; none: no 3D world.
    std::vector<Segment> _segments;                          ///< Kept since the last draw().
    std::vector<contract::renderer::StaticMeshDraw> _draws;  ///< Kept since the last draw().
    std::optional<::Material> _material;                     ///< raylib's default material, made on first use.
};

}  // namespace aylo::backend::raylib::renderer::draw
