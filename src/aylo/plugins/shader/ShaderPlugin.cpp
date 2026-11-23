/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ShaderPlugin
*/

#include "plugins/shader/ShaderPlugin.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "aylo/contract/renderer/shaders/ScreenEffect.hpp"
#include "aylo/core/reload/HotReloader.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/resource/Resources.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/system/Side.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/AssetDirectory.hpp"
#include "plugins/HotReload.hpp"
#include "plugins/graphics/BackendRegistry.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/graphics/GraphicsResources.hpp"  // IWYU pragma: keep
#include "plugins/graphics/TextureAssets.hpp"
#include "plugins/shader/InstanceValues.hpp"
#include "plugins/shader/Material.hpp"
#include "plugins/shader/MaterialAssets.hpp"
#include "plugins/shader/MaterialInstance.hpp"
#include "plugins/shader/MaterialReloadable.hpp"
#include "plugins/shader/ScreenMaterials.hpp"
#include "plugins/shader/ShaderAssets.hpp"
#include "plugins/shader/ShaderCalls.hpp"
#include "plugins/shader/ShaderHandles.hpp"
#include "plugins/shader/ShaderReloadable.hpp"

namespace aylo::plugins::shader {

namespace {
/// @brief Render, client: creates, updates and destroys the shaders and materials in the renderer.
void updateShaders(ecs::NonSendMut<plugins::graphics::Backend> backend, ecs::ResMut<ShaderAssets> shaders,
                   ecs::ResMut<MaterialAssets> materials, ecs::Res<graphics::TextureAssets> textures,
                   ecs::ResMut<ShaderHandles> handles) {
    if (backend->renderer != nullptr) {
        handles->update(*shaders, *materials, *textures, *backend->renderer);
    }
}

/// @return @p screen's passes as the renderer takes them: their materials created (a pass whose material is not yet is
/// left out), their inputs by the slot of the sampler they name in its shader (an unknown one is said once and left
/// out).
std::vector<contract::renderer::ScreenEffectPass> passesOf(const ScreenMaterials& screen, const ShaderHandles& handles,
                                                           const ShaderAssets& shaders,
                                                           const MaterialAssets& materials) {
    std::vector<contract::renderer::ScreenEffectPass> passes;
    for (const ScreenMaterials::Pass& pass : screen.passes) {
        const ShaderHandles::Material* created = handles.getMaterial(pass.material);
        const Material* material = materials.get(pass.material);
        const contract::renderer::CompiledShader* shader =
            material != nullptr ? shaders.get(material->shader) : nullptr;
        if (created == nullptr || shader == nullptr) {
            continue;
        }
        contract::renderer::ScreenEffectPass made{.material = created->material,
                                                  .input = pass.input,
                                                  .output = pass.output,
                                                  .scale = pass.scale,
                                                  .size = pass.size,
                                                  .lowPrecision = pass.lowPrecision};
        for (const auto& [sampler, target] : pass.inputs) {
            const auto texture = std::ranges::find(shader->textures, sampler, &contract::renderer::ShaderTexture::name);
            if (texture != shader->textures.end()) {
                made.inputs.push_back(contract::renderer::ScreenEffectInput{.slot = texture->slot, .target = target});
            }
        }
        passes.push_back(std::move(made));
    }
    return passes;
}

/// @brief Render, client: gives the renderer the screen effects whose material is created, in their order: the passes
/// if a script gave some, else the chain of materials.
void updateScreenMaterials(ecs::NonSendMut<plugins::graphics::Backend> backend, ecs::Res<ScreenMaterials> screen,
                           ecs::Res<ShaderHandles> handles, ecs::Res<ShaderAssets> shaders,
                           ecs::Res<MaterialAssets> materials) {
    if (backend->renderer == nullptr) {
        return;
    }
    if (!screen->passes.empty()) {
        backend->renderer->setScreenEffects(passesOf(*screen, *handles, *shaders, *materials));
        return;
    }
    std::vector<contract::renderer::MaterialId> chain;
    chain.reserve(screen->materials.size());
    for (const core::asset::AssetId id : screen->materials) {
        if (const ShaderHandles::Material* material = handles->getMaterial(id)) {
            chain.push_back(material->material);
        }
    }
    backend->renderer->setScreenMaterials(chain);
}

/// @brief Render, client: writes the instance values scripts gave since the last frame into the entities'
/// MaterialInstance, before anything is drawn.
void updateInstanceValues(ecs::ResMut<InstanceValueRequests> requests, ecs::Res<ShaderAssets> shaders,
                          ecs::Res<MaterialAssets> materials, ecs::Query<MaterialInstance> instances,
                          ecs::Commands& commands) {
    if (!requests->requests.empty()) {
        applyInstanceValues(*requests, *shaders, *materials, instances, commands);
    }
}
}  // namespace

std::string_view ShaderPlugin::getName() const noexcept { return kName; }

std::span<const std::string_view> ShaderPlugin::getDependencies() const noexcept {
    static constexpr std::array<std::string_view, 1> kDependencies{graphics::GraphicsPlugin::kName};
    return kDependencies;
}

void ShaderPlugin::build(ecs::App& app) {
    ecs::Resources& resources = app.getWorld().getResources();
    resources.insertIfAbsent<ShaderAssets>();
    resources.insertIfAbsent<MaterialAssets>();
    resources.insertIfAbsent<ShaderHandles>();
    resources.insertIfAbsent<ScreenMaterials>();
    resources.insertIfAbsent<InstanceValueRequests>();
    resources.insertIfAbsent<graphics::TextureAssets>();
    static_cast<void>(app.getWorld().getComponents().registerType<MaterialInstance>());
    app.addSystem(ecs::Phase::kRender, std::string{kUpdateSystem}, &updateShaders)
        .after(std::string{graphics::GraphicsPlugin::kTexturesSystem})
        .before(std::string{graphics::GraphicsPlugin::kBeginFrameSystem})
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, std::string{kScreenSystem}, &updateScreenMaterials)
        .after(std::string{kUpdateSystem})
        .before(std::string{graphics::GraphicsPlugin::kBeginFrameSystem})
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, std::string{kInstanceValuesSystem}, &updateInstanceValues)
        .before(std::string{kUpdateSystem})
        .before(std::string{graphics::GraphicsPlugin::kBeginFrameSystem})
        .setSide(ecs::Side::kClient);
    addShaderCalls(app);
    resources.getMut<ShaderAssets>()->setSource(assetSourceOf(resources));
    // Only files on disk are followed: a pack has none to watch.
    const std::filesystem::path directory = assetDirectoryOf(resources);
    core::reload::HotReloader* reloader = hotReloaderOf(resources);
    if (reloader != nullptr && !directory.empty()) {
        // The reloader's paths are relative to the game; the assets' directory is a folder of it.
        const std::filesystem::path assets = directory.lexically_relative(reloader->root());
        if (!assets.empty() && *assets.begin() != "..") {
            reloader->add(std::make_unique<ShaderReloadable>(&*resources.getMut<ShaderAssets>(), assets));
            reloader->add(std::make_unique<MaterialReloadable>(&*resources.getMut<ShaderAssets>(),
                                                               &*resources.getMut<MaterialAssets>(),
                                                               &*resources.getMut<graphics::TextureAssets>(), assets));
        }
    }
}

}  // namespace aylo::plugins::shader
