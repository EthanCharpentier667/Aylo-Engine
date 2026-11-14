/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** FakeBackend
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "aylo/contract/platform/IPlatform.hpp"
#include "aylo/contract/platform/WindowConfig.hpp"
#include "aylo/contract/platform/event/Event.hpp"
#include "aylo/contract/platform/exceptions/PlatformExceptions.hpp"
#include "aylo/contract/renderer/IRenderer.hpp"
#include "aylo/contract/renderer/drawing/Color.hpp"
#include "aylo/contract/renderer/drawing/Mesh.hpp"
#include "aylo/contract/renderer/drawing/Shape.hpp"
#include "aylo/contract/renderer/drawing/Sprite.hpp"
#include "aylo/contract/renderer/drawing/Text.hpp"
#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "aylo/contract/renderer/particle/Particle.hpp"
#include "aylo/contract/renderer/particle/ParticleSystemDesc.hpp"
#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "aylo/contract/renderer/view/View.hpp"
#include "aylo/ecs/app/App.hpp"
#include "core/config/Settings.hpp"
#include "plugins/graphics/BackendRegistry.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/particle/ParticlePlugin.hpp"
#include "plugins/shader/ShaderPlugin.hpp"
#include "plugins/shape/ShapePlugin.hpp"
#include "plugins/sprite/SpriteDrawPlugin.hpp"
#include "plugins/sprite/SpritePlugin.hpp"
#include "plugins/text/TextPlugin.hpp"
#include "plugins/tilemap/TilemapPlugin.hpp"
#include "plugins/transform/TransformPlugin.hpp"

namespace aylo::test::plugins {
/// @brief What the fake platform and renderer do and record, shared with the test that drives them.
struct FakeBackendState {
    std::vector<std::vector<aylo::contract::platform::Event>> frames;  ///< The events each poll returns, in order.
    glm::uvec2 framebufferSize{800, 600};                              ///< 0x0 while minimized.
    glm::vec2 contentScale{1.0F, 1.0F};                ///< What getContentScale() returns: 2 for a screen set to 200%.
    bool open{true};                                   ///< Cleared to close the window.
    int closeAfterPolls{-1};                           ///< Closes the window after this many polls (-1: never).
    int initCount{0};                                  ///< platform.init() calls.
    int polls{0};                                      ///< pollEvents() calls.
    int waits{0};                                      ///< waitEvents() calls.
    int beginFrames{0};                                ///< renderer.beginFrame() calls.
    int endFrames{0};                                  ///< renderer.endFrame() calls.
    aylo::contract::renderer::Color clearColor{0.0F};  ///< The last clear colour.
    std::vector<glm::uvec2> resizes;                   ///< Every renderer.resize() call.
    std::vector<aylo::contract::renderer::TextureDesc> createdTextures;         ///< Every createTexture() desc.
    std::vector<std::vector<std::byte>> createdPixels;                          ///< Every createTexture() pixels.
    std::vector<aylo::contract::renderer::TextureId> destroyedTextures;         ///< Every destroy() call.
    std::vector<aylo::contract::renderer::View> views;                          ///< Every setView() call.
    std::vector<aylo::contract::renderer::View3D> views3D;                      ///< Every setView3D() call.
    std::vector<aylo::contract::renderer::Lines3D> lines3D;                     ///< Every drawLines3D() call.
    bool draws3D{true};                                                         ///< What supports3D() returns.
    std::vector<aylo::contract::renderer::StaticMeshDesc> createdStaticMeshes;  ///< Every createStaticMesh() call.
    std::vector<aylo::contract::renderer::StaticMeshId> destroyedStaticMeshes;  ///< Every destroyStaticMesh() call.
    std::vector<aylo::contract::renderer::StaticMeshDraw> staticMeshDraws;      ///< Every drawStaticMesh() call.
    std::vector<aylo::contract::renderer::Sprite> sprites;                      ///< Every draw(Sprite) call.
    std::vector<aylo::contract::renderer::Shape> shapes;                        ///< Every draw(Shape) call.
    std::vector<aylo::contract::renderer::Text> texts;                          ///< Every draw(Text) call.
    bool gpuParticles{false};  ///< What supportsGpuParticles() returns.
    std::vector<aylo::contract::renderer::ParticleSystemDesc> createdParticleSystems;  ///< Every create call.
    std::vector<aylo::contract::renderer::ParticleSystemId> destroyedParticleSystems;  ///< Every destroy() call.
    std::vector<aylo::contract::renderer::ParticleEmission> simulations;               ///< Every simulate() call.
    std::vector<aylo::contract::renderer::ParticleDraw> particleDraws;     ///< Every draw(ParticleDraw) call.
    bool shaders{false};                                                   ///< What supportsShaders() returns.
    std::vector<aylo::contract::renderer::CompiledShader> createdShaders;  ///< Every createShader() call.
    std::vector<aylo::contract::renderer::ShaderId> destroyedShaders;      ///< Every destroy(ShaderId) call.
    std::vector<aylo::contract::renderer::MaterialDesc> createdMaterials;  ///< Every createMaterial() call.
    std::vector<aylo::contract::renderer::MaterialDesc> updatedMaterials;  ///< Every updateMaterial() call.
    std::vector<aylo::contract::renderer::MaterialId> destroyedMaterials;  ///< Every destroy(MaterialId).
    std::vector<char> drawOrder;  ///< 's' (sprite), 'h' (shape), 't' (text) or 'p' (particles) per draw() call.
    std::vector<aylo::contract::renderer::Mesh> meshes;        ///< Every drawMesh() call.
    std::vector<std::optional<glm::uvec2>> sceneSizeRequests;  ///< Every setSceneSize() call.
    std::optional<glm::uvec2> sceneSize;                       ///< What getSceneSize() returns: the last request.
    aylo::contract::renderer::TextureId sceneTexture;          ///< What getSceneTexture() returns.
    std::vector<std::vector<aylo::contract::renderer::MaterialId>>
        screenMaterials;  ///< Every setScreenMaterials() call.
};

/// @brief A window that returns scripted events.
class FakePlatform : public aylo::contract::platform::IPlatform {
  public:
    explicit FakePlatform(std::shared_ptr<FakeBackendState> state) : _state{std::move(state)} {}
    ~FakePlatform() override = default;
    FakePlatform(const FakePlatform& other) = delete;
    FakePlatform& operator=(const FakePlatform& other) = delete;
    FakePlatform(FakePlatform&& other) = delete;
    FakePlatform& operator=(FakePlatform&& other) = delete;

    [[nodiscard]] glm::uvec2 getFramebufferSize() const override { return _state->framebufferSize; }
    [[nodiscard]] glm::vec2 getContentScale() const override { return _state->contentScale; }
    [[nodiscard]] const std::string& getTitle() const override { return _title; }
    void setTitle(const std::string& title) override { _title = title; }
    void setSize(glm::uvec2 /*size*/) override {}
    void init(const aylo::contract::platform::WindowConfig& config) override {
        _title = config.title;
        ++_state->initCount;
    }
    [[nodiscard]] std::vector<aylo::contract::platform::Event> pollEvents() override {
        ++_state->polls;
        return next();
    }
    [[nodiscard]] std::vector<aylo::contract::platform::Event> waitEvents() override {
        ++_state->waits;
        return next();
    }
    [[nodiscard]] bool isOpen() const noexcept override { return _state->open; }
    void close() override { _state->open = false; }
    void setCursorLocked(bool /*locked*/) override {}
    [[nodiscard]] double getTime() const override { return 0.0; }

  private:
    /// @return The next scripted frame of events, closing the window when its time has come.
    std::vector<aylo::contract::platform::Event> next() {
        const int calls = _state->polls + _state->waits;
        if (_state->closeAfterPolls >= 0 && calls >= _state->closeAfterPolls) {
            _state->open = false;
        }
        if (_state->frames.empty()) {
            return {};
        }
        std::vector<aylo::contract::platform::Event> events = std::move(_state->frames.front());
        _state->frames.erase(_state->frames.begin());
        return events;
    }

    std::shared_ptr<FakeBackendState> _state;  ///< Shared with the test.
    std::string _title;                        ///< From init().
};

/// @brief A renderer that draws nothing and records the frames.
class FakeRenderer : public aylo::contract::renderer::IRenderer {
  public:
    explicit FakeRenderer(std::shared_ptr<FakeBackendState> state) : _state{std::move(state)} {}
    ~FakeRenderer() override = default;
    FakeRenderer(const FakeRenderer& other) = delete;
    FakeRenderer& operator=(const FakeRenderer& other) = delete;
    FakeRenderer(FakeRenderer&& other) = delete;
    FakeRenderer& operator=(FakeRenderer&& other) = delete;

    void init(aylo::contract::platform::IPlatform& /*platform*/) override {}
    void resize(glm::uvec2 framebufferSize) override { _state->resizes.push_back(framebufferSize); }
    [[nodiscard]] aylo::contract::renderer::TextureId createTexture(const aylo::contract::renderer::TextureDesc& desc,
                                                                    std::span<const std::byte> pixels) override {
        _state->createdTextures.push_back(desc);
        _state->createdPixels.emplace_back(pixels.begin(), pixels.end());
        return {.index = static_cast<std::uint32_t>(_state->createdTextures.size() - 1), .generation = 1};
    }
    void destroy(aylo::contract::renderer::TextureId texture) override { _state->destroyedTextures.push_back(texture); }
    void beginFrame(const aylo::contract::renderer::Color& clearColor) override {
        ++_state->beginFrames;
        _state->clearColor = clearColor;
    }
    void endFrame() override { ++_state->endFrames; }
    void setView(const aylo::contract::renderer::View& view) override { _state->views.push_back(view); }
    [[nodiscard]] bool supports3D() const noexcept override { return _state->draws3D; }
    void setView3D(const aylo::contract::renderer::View3D& view) override { _state->views3D.push_back(view); }
    void drawLines3D(const aylo::contract::renderer::Lines3D& lines) override { _state->lines3D.push_back(lines); }
    [[nodiscard]] aylo::contract::renderer::StaticMeshId createStaticMesh(
        const aylo::contract::renderer::StaticMeshDesc& desc) override {
        if (!_state->draws3D) {
            throw aylo::contract::platform::exceptions::UnsupportedFeatureException("FakeRenderer: no 3D");
        }
        _state->createdStaticMeshes.push_back(desc);
        return {.index = static_cast<std::uint32_t>(_state->createdStaticMeshes.size()), .generation = 1};
    }
    void destroyStaticMesh(aylo::contract::renderer::StaticMeshId mesh) override {
        _state->destroyedStaticMeshes.push_back(mesh);
    }
    void drawStaticMesh(const aylo::contract::renderer::StaticMeshDraw& draw) override {
        _state->staticMeshDraws.push_back(draw);
    }
    void draw(const aylo::contract::renderer::Sprite& sprite) override {
        _state->sprites.push_back(sprite);
        _state->drawOrder.push_back('s');
    }
    void draw(const aylo::contract::renderer::Shape& shape) override {
        _state->shapes.push_back(shape);
        _state->drawOrder.push_back('h');
    }
    void draw(const aylo::contract::renderer::Text& text) override {
        _state->texts.push_back(text);
        _state->drawOrder.push_back('t');
    }
    [[nodiscard]] bool supportsGpuParticles() const noexcept override { return _state->gpuParticles; }
    [[nodiscard]] aylo::contract::renderer::ParticleSystemId createParticleSystem(
        const aylo::contract::renderer::ParticleSystemDesc& desc) override {
        if (!_state->gpuParticles) {
            throw aylo::contract::platform::exceptions::UnsupportedFeatureException("FakeRenderer: no GPU particles");
        }
        _state->createdParticleSystems.push_back(desc);
        return {.index = static_cast<std::uint32_t>(_state->createdParticleSystems.size() - 1), .generation = 1};
    }
    void destroy(aylo::contract::renderer::ParticleSystemId system) override {
        _state->destroyedParticleSystems.push_back(system);
    }
    void simulate(aylo::contract::renderer::ParticleSystemId /*system*/,
                  const aylo::contract::renderer::ParticleEmission& emission) override {
        _state->simulations.push_back(emission);
    }
    // Copies the draw: the CPU instances it points to are only valid until the frame is drawn.
    void draw(const aylo::contract::renderer::ParticleDraw& particles) override {
        _state->particleDraws.push_back(particles);
        _state->drawOrder.push_back('p');
    }
    [[nodiscard]] bool supportsShaders() const noexcept override { return _state->shaders; }
    [[nodiscard]] aylo::contract::renderer::ShaderId createShader(
        const aylo::contract::renderer::CompiledShader& shader) override {
        if (!_state->shaders) {
            throw aylo::contract::platform::exceptions::UnsupportedFeatureException("FakeRenderer: no shaders");
        }
        aylo::contract::renderer::validate(shader);
        _state->createdShaders.push_back(shader);
        return {.index = static_cast<std::uint32_t>(_state->createdShaders.size() - 1), .generation = 1};
    }
    void destroy(aylo::contract::renderer::ShaderId shader) override { _state->destroyedShaders.push_back(shader); }
    [[nodiscard]] aylo::contract::renderer::MaterialId createMaterial(
        const aylo::contract::renderer::MaterialDesc& desc) override {
        if (!_state->shaders) {
            throw aylo::contract::platform::exceptions::UnsupportedFeatureException("FakeRenderer: no shaders");
        }
        _state->createdMaterials.push_back(desc);
        return {.index = static_cast<std::uint32_t>(_state->createdMaterials.size() - 1), .generation = 1};
    }
    void updateMaterial(aylo::contract::renderer::MaterialId /*material*/,
                        const aylo::contract::renderer::MaterialDesc& desc) override {
        _state->updatedMaterials.push_back(desc);
    }
    void destroy(aylo::contract::renderer::MaterialId material) override {
        _state->destroyedMaterials.push_back(material);
    }
    void drawMesh(const aylo::contract::renderer::Mesh& mesh) override { _state->meshes.push_back(mesh); }
    void setScreenMaterials(std::span<const aylo::contract::renderer::MaterialId> materials) override {
        _state->screenMaterials.emplace_back(materials.begin(), materials.end());
    }
    void setSceneSize(std::optional<glm::uvec2> size) override {
        _state->sceneSizeRequests.push_back(size);
        _state->sceneSize = size;
    }
    [[nodiscard]] std::optional<glm::uvec2> getSceneSize() const override { return _state->sceneSize; }
    [[nodiscard]] aylo::contract::renderer::TextureId getSceneTexture() const override { return _state->sceneTexture; }

  private:
    std::shared_ptr<FakeBackendState> _state;  ///< Shared with the test.
};

/// @return A registry holding the fake platform and renderer as "fake", both driven by `state`.
inline std::unique_ptr<aylo::plugins::graphics::BackendRegistry> makeFakeRegistry(
    const std::shared_ptr<FakeBackendState>& state) {
    auto registry = std::make_unique<aylo::plugins::graphics::BackendRegistry>();
    registry->platforms.add("fake", [state](const aylo::core::config::Settings& /*settings*/) {
        return std::make_unique<FakePlatform>(state);
    });
    registry->renderers.add("fake", [state](const aylo::core::config::Settings& /*settings*/) {
        return std::make_unique<FakeRenderer>(state);
    });
    return registry;
}

/// @return Settings naming the fake platform and renderer.
inline aylo::core::config::Settings makeFakeSettings() {
    aylo::core::config::Settings settings;
    settings.set("platform", "fake");
    settings.set("renderer", "fake");
    settings.set("window.title", "Test");
    return settings;
}
/// @brief Adds the GraphicsPlugin and every plugin that draws, as a client does.
inline void addDrawingPlugins(aylo::ecs::App& app, std::unique_ptr<aylo::plugins::graphics::BackendRegistry> registry,
                              aylo::core::config::Settings settings) {
    app.addPlugin(aylo::plugins::transform::TransformPlugin{});
    app.addPlugin(aylo::plugins::input::InputPlugin{});
    app.addPlugin(aylo::plugins::graphics::GraphicsPlugin{std::move(registry), std::move(settings)});
    app.addPlugin(aylo::plugins::shader::ShaderPlugin{});
    app.addPlugin(aylo::plugins::sprite::SpritePlugin{});
    app.addPlugin(aylo::plugins::sprite::SpriteDrawPlugin{});
    app.addPlugin(aylo::plugins::shape::ShapePlugin{});
    app.addPlugin(aylo::plugins::tilemap::TilemapPlugin{});
    app.addPlugin(aylo::plugins::text::TextPlugin{});
    app.addPlugin(aylo::plugins::particle::ParticlePlugin{});
}

}  // namespace aylo::test::plugins
