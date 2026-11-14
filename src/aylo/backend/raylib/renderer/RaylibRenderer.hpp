/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** RaylibRenderer
*/

#pragma once

#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <optional>
#include <span>

#include "aylo/contract/platform/IPlatform.hpp"
#include "aylo/contract/renderer/IRenderer.hpp"
#include "aylo/contract/renderer/drawing/Color.hpp"
#include "aylo/contract/renderer/drawing/Lines3D.hpp"
#include "aylo/contract/renderer/drawing/Mesh.hpp"
#include "aylo/contract/renderer/drawing/Overlay.hpp"
#include "aylo/contract/renderer/drawing/Shape.hpp"
#include "aylo/contract/renderer/drawing/Sprite.hpp"
#include "aylo/contract/renderer/drawing/StaticMesh.hpp"
#include "aylo/contract/renderer/drawing/Text.hpp"
#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "aylo/contract/renderer/particle/Particle.hpp"
#include "aylo/contract/renderer/particle/ParticleSystemDesc.hpp"
#include "aylo/contract/renderer/view/View.hpp"
#include "aylo/contract/renderer/view/View3D.hpp"
#include "backend/raylib/Export.hpp"

namespace aylo::backend::raylib::renderer {

/// @brief A renderer on raylib, registered as the renderer "raylib": textures, sprites, shapes, MSDF text, particles
/// simulated by the engine on the CPU, the overlay and tool meshes, and the 3D world (lines and unlit static meshes,
/// draw/World3D), drawn under the 2D one.
///
/// @details It draws in the window and the OpenGL context of the platform "raylib", the only one it accepts. It only
/// dispatches: Frame keeps the view and the blend mode, TextureTable the textures, and the functions of draw/ turn each
/// drawing into raylib calls. What raylib's OpenGL 3.3 path cannot do is reported through the contract's capability
/// checks: no GPU particles (the particle plugin then simulates them on the CPU) and no user shaders (SPIR-V).
class AYLO_BACKEND_RAYLIB_API RaylibRenderer final : public contract::renderer::IRenderer {
  public:
    /// @param virtualResolution The resolution the game is designed for, fitted in the window with bars; nothing: the
    /// window itself.
    explicit RaylibRenderer(std::optional<glm::uvec2> virtualResolution);
    ~RaylibRenderer() override;
    RaylibRenderer(const RaylibRenderer&) = delete;
    RaylibRenderer& operator=(const RaylibRenderer&) = delete;
    RaylibRenderer(RaylibRenderer&&) = delete;
    RaylibRenderer& operator=(RaylibRenderer&&) = delete;

    /// @throws std::runtime_error If @p platform is not the platform "raylib", or its window is not open.
    void init(contract::platform::IPlatform& platform) override;
    void resize(glm::uvec2 framebufferSize) override;
    [[nodiscard]] glm::vec2 getViewSize() const override;

    [[nodiscard]] contract::renderer::TextureId createTexture(const contract::renderer::TextureDesc& desc,
                                                              std::span<const std::byte> pixels) override;
    void destroy(contract::renderer::TextureId texture) override;

    void beginFrame(const contract::renderer::Color& clearColor) override;
    void endFrame() override;
    void setView(const contract::renderer::View& view) override;
    void draw(const contract::renderer::Sprite& sprite) override;
    void draw(const contract::renderer::Shape& shape) override;
    void draw(const contract::renderer::Text& text) override;
    void drawOverlay(std::span<const contract::renderer::OverlayItem> items) override;

    /// @throws contract::platform::exceptions::UnsupportedFeatureException Always: particles run on the CPU.
    [[nodiscard]] contract::renderer::ParticleSystemId createParticleSystem(
        const contract::renderer::ParticleSystemDesc& desc) override;
    void destroy(contract::renderer::ParticleSystemId system) override;
    /// @throws contract::platform::exceptions::UnsupportedFeatureException Always: particles run on the CPU.
    void simulate(contract::renderer::ParticleSystemId system,
                  const contract::renderer::ParticleEmission& emission) override;
    void draw(const contract::renderer::ParticleDraw& particles) override;

    void drawMesh(const contract::renderer::Mesh& mesh) override;

    /// @return True: the 3D world is drawn, under the 2D one.
    [[nodiscard]] bool supports3D() const noexcept override { return true; }
    void setView3D(const contract::renderer::View3D& view) override;
    void drawLines3D(const contract::renderer::Lines3D& lines) override;
    /// @throws std::invalid_argument If an index names no vertex or a part goes past the indices.
    [[nodiscard]] contract::renderer::StaticMeshId createStaticMesh(
        const contract::renderer::StaticMeshDesc& desc) override;
    void destroyStaticMesh(contract::renderer::StaticMeshId mesh) override;
    void drawStaticMesh(const contract::renderer::StaticMeshDraw& draw) override;

  private:
    /// @brief Draws the 3D world kept so far, before any 2D drawing that follows.
    void flush3D();

    struct Parts;  ///< Frame, textures and text shader: they hold raylib types, kept out of this header.

    std::unique_ptr<Parts> _parts;  ///< Never null.
};

}  // namespace aylo::backend::raylib::renderer
