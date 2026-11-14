/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MeshPlugin
*/

#include "plugins/mesh/MeshPlugin.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "aylo/contract/renderer/IRenderer.hpp"
#include "aylo/contract/renderer/drawing/StaticMesh.hpp"
#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/resource/Resources.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/system/Side.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/AssetDirectory.hpp"
#include "plugins/graphics/BackendRegistry.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/graphics/GraphicsResources.hpp"  // IWYU pragma: keep
#include "plugins/graphics/TextureAssets.hpp"
#include "plugins/mesh/MeshAssets.hpp"
#include "plugins/mesh/MeshCalls.hpp"
#include "plugins/mesh/MeshRenderer.hpp"
#include "plugins/mesh/detail/GltfLoader.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformMath.hpp"
#include "plugins/transform/TransformPlugin.hpp"

namespace aylo::plugins::mesh {
namespace {

/// @return The renderer, or null before the GraphicsPlugin created the backend.
contract::renderer::IRenderer* rendererOf(graphics::Backend& backend) noexcept {
    return backend.platform != nullptr ? backend.renderer.get() : nullptr;
}

/// @brief Fails @p entry with @p error, said on the error output.
void fail(MeshAssets::Entry& entry, std::string error) {
    std::cerr << "[Mesh] " << error << '\n';
    entry.error = std::move(error);
    entry.state = MeshState::kFailed;
    entry.model.reset();
}

/// @brief Render, before the GraphicsPlugin loads the textures: reads the queued glTF files and asks for the textures
/// of their parts (TextureAssets is appended to, as by the text and tilemap plugins).
void readMeshes(ecs::ResMut<MeshAssets> meshes, ecs::ResAppend<graphics::TextureAssets> textures) {
    for (const core::asset::AssetId id : meshes->getIds()) {
        MeshAssets::Entry& entry = *meshes->getMut(id);
        if (entry.state != MeshState::kQueued) {
            continue;
        }
        if (meshes->getSource() == nullptr) {
            fail(entry, std::format("Mesh '{}': there is no assets folder to read it from", entry.path));
            continue;
        }
        try {
            auto model = std::make_shared<detail::GltfModel>(detail::loadGltf(entry.path, *meshes->getSource()));
            const contract::renderer::TextureDesc desc{.filter = contract::renderer::TextureFilter::kLinear,
                                                       .repeat = true};
            std::vector<std::optional<core::asset::AssetId>> images(model->images.size());
            for (std::size_t index = 0; index < model->images.size(); ++index) {
                const detail::GltfImage& image = model->images.at(index);
                images.at(index) = image.path.empty() ? textures->load(std::format("{}#image{}", entry.path, index),
                                                                       image.encoded, desc)
                                                      : textures->load(std::filesystem::path{image.path}, desc);
            }
            entry.partTextures.clear();
            for (const std::optional<std::size_t>& image : model->partImages) {
                entry.partTextures.push_back(image ? images.at(*image) : std::nullopt);
            }
            entry.model = std::move(model);
            entry.state = MeshState::kWaitingTextures;
        } catch (const std::exception& error) {
            fail(entry, error.what());
        }
    }
}

/// @return Whether every texture of @p entry is done loading (loaded or failed).
bool texturesSettled(const MeshAssets::Entry& entry, const graphics::TextureAssets& textures) {
    return std::ranges::all_of(entry.partTextures, [&textures](const std::optional<core::asset::AssetId>& texture) {
        const graphics::TextureAssets::Entry* known = texture ? textures.get(*texture) : nullptr;
        return known == nullptr || known->state != graphics::AssetState::kQueued;
    });
}

/// @brief Creates the meshes whose textures are settled.
void uploadMeshes(contract::renderer::IRenderer& renderer, MeshAssets& meshes,
                  const graphics::TextureAssets& textures) {
    for (const core::asset::AssetId id : meshes.getIds()) {
        MeshAssets::Entry& entry = *meshes.getMut(id);
        if (entry.state != MeshState::kWaitingTextures || !texturesSettled(entry, textures)) {
            continue;
        }
        if (!renderer.supports3D()) {
            fail(entry, std::format("Mesh '{}': this renderer does not draw 3D", entry.path));
            continue;
        }
        contract::renderer::StaticMeshDesc desc = entry.model->mesh;
        for (std::size_t part = 0; part < desc.parts.size() && part < entry.partTextures.size(); ++part) {
            if (const std::optional<core::asset::AssetId>& texture = entry.partTextures.at(part)) {
                desc.parts.at(part).texture = textures.getTexture(*texture).value_or(contract::renderer::TextureId{});
            }
        }
        try {
            entry.mesh = renderer.createStaticMesh(desc);
            entry.state = MeshState::kLoaded;
            entry.model.reset();
        } catch (const std::exception& error) {
            fail(entry, std::format("Mesh '{}': {}", entry.path, error.what()));
        }
    }
}

/// @brief Render: creates the meshes whose textures are ready, then draws each MeshRenderer whose mesh is loaded, at
/// its GlobalTransform3D. One system, placed among the GraphicsPlugin's own, so no other plugin's use of the backend
/// needs an order with it.
void drawMeshes(ecs::NonSendMut<graphics::Backend> backend, ecs::ResMut<MeshAssets> meshes,
                ecs::Res<graphics::TextureAssets> textures,
                ecs::Query<const transform::GlobalTransform3D, const MeshRenderer> renderers) {
    contract::renderer::IRenderer* renderer = rendererOf(*backend);
    if (renderer == nullptr) {
        return;
    }
    uploadMeshes(*renderer, *meshes, *textures);
    if (!renderer->supports3D()) {
        return;
    }
    for (auto [entity, global, model] : renderers) {
        if (const std::optional<contract::renderer::StaticMeshId> mesh = meshes->getMesh(model.mesh)) {
            renderer->drawStaticMesh(contract::renderer::StaticMeshDraw{
                .mesh = *mesh, .world = transform::toMatrix(global), .color = model.color});
        }
    }
}

}  // namespace

std::string_view MeshPlugin::getName() const noexcept { return kName; }

std::span<const std::string_view> MeshPlugin::getDependencies() const noexcept {
    static constexpr std::array<std::string_view, 2> kDependencies{graphics::GraphicsPlugin::kName,
                                                                   transform::TransformPlugin::kName};
    return kDependencies;
}

void MeshPlugin::build(ecs::App& app) {
    ecs::Resources& resources = app.getWorld().getResources();
    resources.insertIfAbsent<MeshAssets>();
    resources.getMut<MeshAssets>()->setSource(assetSourceOf(resources));
    static_cast<void>(app.getWorld().getComponents().registerType<MeshRenderer>());
    addMeshCalls(app);

    app.addSystem(ecs::Phase::kRender, std::string{kReadSystem}, &readMeshes)
        .before(std::string{graphics::GraphicsPlugin::kTexturesSystem})
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, std::string{kDrawSystem}, &drawMeshes)
        .after(std::string{graphics::GraphicsPlugin::kLines3DSystem})
        .before(std::string{graphics::GraphicsPlugin::kCameraSystem})
        .setSide(ecs::Side::kClient);
}

}  // namespace aylo::plugins::mesh
