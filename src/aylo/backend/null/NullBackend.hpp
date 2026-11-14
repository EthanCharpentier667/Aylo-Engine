/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** NullBackend
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_uint2.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "aylo/contract/platform/IPlatform.hpp"
#include "aylo/contract/platform/WindowConfig.hpp"
#include "aylo/contract/platform/event/Event.hpp"
#include "aylo/contract/renderer/IRenderer.hpp"
#include "aylo/contract/renderer/drawing/Color.hpp"
#include "aylo/contract/renderer/drawing/Lines3D.hpp"
#include "aylo/contract/renderer/drawing/Shape.hpp"
#include "aylo/contract/renderer/drawing/Sprite.hpp"
#include "aylo/contract/renderer/drawing/StaticMesh.hpp"
#include "aylo/contract/renderer/drawing/Text.hpp"
#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "aylo/contract/renderer/particle/Particle.hpp"
#include "aylo/contract/renderer/particle/ParticleSystemDesc.hpp"
#include "aylo/contract/renderer/view/View.hpp"
#include "aylo/contract/renderer/view/View3D.hpp"
#include "backend/null/Export.hpp"

namespace aylo::backend::null {

/// @brief A platform with no window: reports the size the config asks for, produces no event, and stays open until
/// close() (or the program) ends it. The clock is the steady clock.
class AYLO_BACKEND_NULL_API NullPlatform final : public contract::platform::IPlatform {
  public:
    [[nodiscard]] glm::uvec2 getFramebufferSize() const override { return m_size; }
    [[nodiscard]] const std::string& getTitle() const override { return m_title; }
    void setTitle(const std::string& title) override { m_title = title; }
    void setSize(glm::uvec2 size) override { m_size = size; }
    void init(const contract::platform::WindowConfig& config) override;
    [[nodiscard]] std::vector<contract::platform::Event> pollEvents() override;
    [[nodiscard]] bool isOpen() const noexcept override { return m_open; }
    void close() override { m_open = false; }
    void setCursorLocked(bool /*locked*/) override {}
    [[nodiscard]] double getTime() const override;
    void setClipboardText(std::string_view text) override { m_clipboard = text; }
    [[nodiscard]] std::string getClipboardText() const override { return m_clipboard; }

  private:
    glm::uvec2 m_size{1280, 720};
    std::string m_title;
    std::string m_clipboard;
    bool m_open{true};
    bool m_closedSent{false};
};

/// @brief A renderer that draws nothing. Textures, particle systems, shaders and materials get ids (so scripts and
/// systems that keep them work), nothing is stored.
class AYLO_BACKEND_NULL_API NullRenderer final : public contract::renderer::IRenderer {
  public:
    void init(contract::platform::IPlatform& /*platform*/) override {}
    void resize(glm::uvec2 /*framebufferSize*/) override {}
    [[nodiscard]] contract::renderer::TextureId createTexture(const contract::renderer::TextureDesc& desc,
                                                              std::span<const std::byte> pixels) override;
    void destroy(contract::renderer::TextureId /*texture*/) override {}
    void beginFrame(const contract::renderer::Color& /*clearColor*/) override {}
    void endFrame() override {}
    void setView(const contract::renderer::View& /*view*/) override {}
    /// @return True: the 3D world is accepted, and drawn nowhere, as the rest.
    [[nodiscard]] bool supports3D() const noexcept override { return true; }
    void setView3D(const contract::renderer::View3D& /*view*/) override {}
    void drawLines3D(const contract::renderer::Lines3D& /*lines*/) override {}
    /// @return A new id each time: the mesh exists nowhere.
    [[nodiscard]] contract::renderer::StaticMeshId createStaticMesh(
        const contract::renderer::StaticMeshDesc& /*desc*/) override {
        return contract::renderer::StaticMeshId{.index = m_nextStaticMesh++, .generation = 1};
    }
    void destroyStaticMesh(contract::renderer::StaticMeshId /*mesh*/) override {}
    void drawStaticMesh(const contract::renderer::StaticMeshDraw& /*draw*/) override {}
    void draw(const contract::renderer::Sprite& /*sprite*/) override {}
    void draw(const contract::renderer::Shape& /*shape*/) override {}
    void draw(const contract::renderer::Text& /*text*/) override {}
    [[nodiscard]] contract::renderer::ParticleSystemId createParticleSystem(
        const contract::renderer::ParticleSystemDesc& desc) override;
    void destroy(contract::renderer::ParticleSystemId /*system*/) override {}
    void simulate(contract::renderer::ParticleSystemId /*system*/,
                  const contract::renderer::ParticleEmission& /*emission*/) override {}
    void draw(const contract::renderer::ParticleDraw& /*particles*/) override {}
    using IRenderer::destroy;

  private:
    std::uint32_t m_nextTexture{1};
    std::uint32_t m_nextParticleSystem{1};
    std::uint32_t m_nextStaticMesh{1};
};

}  // namespace aylo::backend::null
