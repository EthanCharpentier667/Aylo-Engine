/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ShaderPlugin
*/

#include "plugins/shader/ShaderPlugin.hpp"

#include <array>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "aylo/core/reload/HotReloader.hpp"
#include "aylo/ecs/app/App.hpp"
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
#include "plugins/shader/MaterialAssets.hpp"
#include "plugins/shader/MaterialInstance.hpp"
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

/// @brief Render, client: gives the renderer the screen effects whose material is created, in their order.
void updateScreenMaterials(ecs::NonSendMut<plugins::graphics::Backend> backend, ecs::Res<ScreenMaterials> screen,
                           ecs::Res<ShaderHandles> handles) {
    if (backend->renderer == nullptr) {
        return;
    }
    std::vector<contract::renderer::MaterialId> materials;
    materials.reserve(screen->materials.size());
    for (const core::asset::AssetId id : screen->materials) {
        if (const ShaderHandles::Material* material = handles->getMaterial(id)) {
            materials.push_back(material->material);
        }
    }
    backend->renderer->setScreenMaterials(materials);
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
        }
    }
}

}  // namespace aylo::plugins::shader
