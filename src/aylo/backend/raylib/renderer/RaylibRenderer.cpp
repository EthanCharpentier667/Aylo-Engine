/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** RaylibRenderer
*/

#include "RaylibRenderer.hpp"

#include <raylib.h>

#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <variant>

#include "Frame.hpp"
#include "TextureTable.hpp"
#include "aylo/contract/platform/IPlatform.hpp"
#include "aylo/contract/platform/exceptions/PlatformExceptions.hpp"
#include "aylo/contract/renderer/drawing/BlendMode.hpp"
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
#include "backend/raylib/platform/RaylibPlatform.hpp"
#include "backend/raylib/renderer/draw/Meshes.hpp"
#include "backend/raylib/renderer/draw/MsdfText.hpp"
#include "backend/raylib/renderer/draw/Particles.hpp"
#include "backend/raylib/renderer/draw/Shapes.hpp"
#include "backend/raylib/renderer/draw/Sprites.hpp"
#include "backend/raylib/renderer/draw/World3D.hpp"

namespace aylo::backend::raylib::renderer {

namespace cr = contract::renderer;

struct RaylibRenderer::Parts {
    explicit Parts(std::optional<glm::uvec2> virtualResolution) : frame(virtualResolution) {}

    Frame frame;            ///< The view, the blend mode and the drawing spaces.
    TextureTable textures;  ///< The textures by id.
    draw::MsdfText text;    ///< The text shader.
    draw::World3D world3D;  ///< The static meshes, and the frame's 3D world.
};

namespace {
/// @return The size of the window's framebuffer, in pixels.
glm::uvec2 framebufferSize() {
    return {static_cast<unsigned>(GetRenderWidth()), static_cast<unsigned>(GetRenderHeight())};
}
}  // namespace

void RaylibRenderer::flush3D() {
    _parts->frame.inScreenSpace([this] { _parts->world3D.draw(_parts->textures, framebufferSize()); });
}

RaylibRenderer::RaylibRenderer(std::optional<glm::uvec2> virtualResolution)
    : _parts(std::make_unique<Parts>(virtualResolution)) {}

RaylibRenderer::~RaylibRenderer() = default;

void RaylibRenderer::init(contract::platform::IPlatform& platform) {
    const auto* raylibPlatform = dynamic_cast<const platform::RaylibPlatform*>(&platform);
    if (raylibPlatform == nullptr || !raylibPlatform->isWindowOpened()) {
        throw std::runtime_error(R"(The renderer "raylib" needs the platform "raylib" (engine.settings.platform))");
    }
    _parts->text.load();
}

void RaylibRenderer::resize(glm::uvec2 /*framebufferSize*/) {}

glm::vec2 RaylibRenderer::getViewSize() const { return _parts->frame.getViewSize(); }

cr::TextureId RaylibRenderer::createTexture(const cr::TextureDesc& desc, std::span<const std::byte> pixels) {
    return _parts->textures.create(desc, pixels);
}

void RaylibRenderer::destroy(cr::TextureId texture) { _parts->textures.destroy(texture); }

void RaylibRenderer::beginFrame(const cr::Color& clearColor) {
    _parts->frame.begin(clearColor);
    _parts->world3D.begin();
}

void RaylibRenderer::endFrame() {
    flush3D();
    _parts->frame.end();
}

void RaylibRenderer::setView(const cr::View& view) {
    flush3D();
    _parts->frame.setView(view);
}

void RaylibRenderer::setView3D(const cr::View3D& view) {
    _parts->world3D.setView(view, _parts->frame.getViewSize(), framebufferSize());
}

void RaylibRenderer::drawLines3D(const cr::Lines3D& lines) { _parts->world3D.add(lines); }

cr::StaticMeshId RaylibRenderer::createStaticMesh(const cr::StaticMeshDesc& desc) {
    return _parts->world3D.create(desc);
}

void RaylibRenderer::destroyStaticMesh(cr::StaticMeshId mesh) { _parts->world3D.destroy(mesh); }

void RaylibRenderer::drawStaticMesh(const cr::StaticMeshDraw& draw) { _parts->world3D.add(draw); }

void RaylibRenderer::draw(const cr::Sprite& sprite) {
    flush3D();
    if (const ::Texture2D* texture = _parts->textures.find(sprite.texture); texture != nullptr) {
        _parts->frame.useBlend(sprite.blend);
        draw::drawSprite(*texture, sprite);
    }
}

void RaylibRenderer::draw(const cr::Shape& shape) {
    flush3D();
    _parts->frame.useBlend(shape.blend);
    draw::drawShape(shape);
}

void RaylibRenderer::draw(const cr::Text& text) {
    flush3D();
    if (const ::Texture2D* atlas = _parts->textures.find(text.texture); atlas != nullptr && !text.glyphs.empty()) {
        _parts->frame.useBlend(text.blend);
        _parts->text.draw(*atlas, text);
    }
}

void RaylibRenderer::drawOverlay(std::span<const cr::OverlayItem> items) {
    flush3D();
    _parts->frame.forEachOverlayItem(items, [this](const cr::OverlayItem& item) {
        std::visit([this](const auto& drawing) { draw(drawing); }, item.drawing);
    });
}

cr::ParticleSystemId RaylibRenderer::createParticleSystem(const cr::ParticleSystemDesc& /*desc*/) {
    throw contract::platform::exceptions::UnsupportedFeatureException(
        "The raylib renderer has no GPU particles: the particle plugin simulates them on the CPU");
}

void RaylibRenderer::destroy(cr::ParticleSystemId /*system*/) {}

void RaylibRenderer::simulate(cr::ParticleSystemId /*system*/, const cr::ParticleEmission& /*emission*/) {
    throw contract::platform::exceptions::UnsupportedFeatureException(
        "The raylib renderer has no GPU particles: the particle plugin simulates them on the CPU");
}

void RaylibRenderer::draw(const cr::ParticleDraw& particles) {
    flush3D();
    const auto* instances = std::get_if<std::span<const cr::ParticleInstance>>(&particles.particles);
    const ::Texture2D* texture = _parts->textures.find(particles.texture);
    if (instances != nullptr && texture != nullptr) {
        _parts->frame.useBlend(particles.blend);
        draw::drawParticles(*texture, *instances, particles);
    }
}

void RaylibRenderer::drawMesh(const cr::Mesh& mesh) {
    if (mesh.commands.empty()) {
        return;
    }
    _parts->frame.inScreenSpace([this, &mesh] {
        _parts->frame.useBlend(cr::BlendMode::kAlpha);
        draw::drawMesh(mesh, _parts->textures);
    });
}

}  // namespace aylo::backend::raylib::renderer
