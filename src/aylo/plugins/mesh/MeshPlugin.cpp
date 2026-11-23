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
#include <glm/ext/matrix_float4x4.hpp>
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
#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/query/QueryTerms.hpp"
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
#include "plugins/mesh/detail/Model.hpp"
#include "plugins/mesh/detail/ModelLoader.hpp"
#include "plugins/shader/Material.hpp"
#include "plugins/shader/MaterialAssets.hpp"
#include "plugins/shader/MaterialFile.hpp"
#include "plugins/shader/MaterialInstance.hpp"
#include "plugins/shader/ShaderAssets.hpp"
#include "plugins/shader/ShaderHandles.hpp"
#include "plugins/shader/ShaderPlugin.hpp"
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

/// @brief Render, before the GraphicsPlugin loads the textures: reads the queued model files and asks for the textures
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
            auto model = std::make_shared<detail::Model>(detail::loadModel(entry.path, *meshes->getSource()));
            const contract::renderer::TextureDesc desc{.filter = contract::renderer::TextureFilter::kLinear,
                                                       .repeat = true};
            std::vector<std::optional<core::asset::AssetId>> images(model->images.size());
            for (std::size_t index = 0; index < model->images.size(); ++index) {
                const detail::ModelImage& image = model->images.at(index);
                images.at(index) = image.path.empty() ? textures->load(std::format("{}#image{}", entry.path, index),
                                                                       image.encoded, desc)
                                                      : textures->load(std::filesystem::path{image.path}, desc);
            }
            entry.partTextures.clear();
            for (const std::optional<std::size_t>& image : model->partImages) {
                entry.partTextures.push_back(image ? images.at(*image) : std::nullopt);
            }
            entry.partNames = model->partNames;
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

/// @return The material @p material was created as, a material of a `#pragma mesh` shader; nullptr (the default
/// material) for none, one not created yet, or one whose shader draws no mesh (warned about once).
const shader::ShaderHandles::Material* materialOf(core::asset::AssetId material, MeshAssets& meshes,
                                                  const shader::ShaderHandles& handles,
                                                  const shader::ShaderAssets& shaders) {
    if (material == core::asset::AssetId{}) {
        return nullptr;
    }
    const shader::ShaderHandles::Material* created = handles.getMaterial(material);
    if (created == nullptr) {
        return nullptr;
    }
    const contract::renderer::CompiledShader* compiled = shaders.get(created->shader);
    if (compiled != nullptr && compiled->findVariant(contract::renderer::ShaderTarget::kMesh) == nullptr) {
        if (meshes.firstWarningFor(material)) {
            std::cerr << std::format(
                "[Mesh] The material of a MeshRenderer has a 2D shader ('{}'): a model needs a "
                "'#pragma mesh' shader. It is drawn with the built-in one.\n",
                compiled->name);
        }
        return nullptr;
    }
    return created;
}

/// @brief What drawMeshes() reuses from one model to the next, so a frame allocates nothing once it is big enough.
struct PassScratch {
    std::vector<core::asset::AssetId> assets;             ///< Per part: the material of the pass drawn.
    std::vector<contract::renderer::MaterialId> created;  ///< Per part: what the renderer created it as.
};

/// @brief Draws the passes of @p model after its first: each part's material's next pass, then theirs,
/// kMaxMaterialPasses materials at most per part (a longer chain loops, which is warned about once).
void drawNextPasses(contract::renderer::IRenderer& renderer, contract::renderer::StaticMeshDraw draw,
                    PassScratch& scratch, MeshAssets& meshes, const shader::MaterialAssets& materials,
                    const shader::ShaderHandles& handles, const shader::ShaderAssets& shaders) {
    draw.material = {};
    draw.listedPartsOnly = true;
    for (std::size_t pass = 1;; ++pass) {
        bool any = false;
        for (core::asset::AssetId& asset : scratch.assets) {
            const shader::Material* material = asset != core::asset::AssetId{} ? materials.get(asset) : nullptr;
            asset = material != nullptr ? material->nextPass : core::asset::AssetId{};
            any = any || asset != core::asset::AssetId{};
        }
        if (!any) {
            return;
        }
        if (pass == shader::kMaxMaterialPasses) {
            if (meshes.firstWarningFor(scratch.assets.front())) {
                std::cerr << std::format(
                    "[Mesh] A material's next passes chain more than {} materials: the rest are "
                    "not drawn (do they loop?)\n",
                    shader::kMaxMaterialPasses);
            }
            return;
        }
        scratch.created.clear();
        for (const core::asset::AssetId asset : scratch.assets) {
            const shader::ShaderHandles::Material* created = materialOf(asset, meshes, handles, shaders);
            scratch.created.push_back(created != nullptr ? created->material : contract::renderer::MaterialId{});
        }
        draw.partMaterials = scratch.created;
        renderer.drawStaticMesh(draw);
    }
}

/// @brief What drawModel() reads besides the model: the assets and the renderer's materials.
struct ModelContext {
    MeshAssets* meshes;
    const shader::MaterialAssets* materials;
    const shader::ShaderHandles* handles;
    const shader::ShaderAssets* shaders;
};

/// @brief Draws @p model at @p world: each part with its material (MeshRenderer::materials, or else
/// MeshRenderer::material) and @p instance's values (or else its first material's defaults), then the next passes.
void drawModel(contract::renderer::IRenderer& renderer, contract::renderer::StaticMeshId mesh, const glm::mat4& world,
               const MeshRenderer& model, const shader::MaterialInstance* instance, PassScratch& scratch,
               const ModelContext& context) {
    contract::renderer::StaticMeshDraw draw{.mesh = mesh, .world = world, .color = model.color};
    const shader::ShaderHandles::Material* first =
        materialOf(model.material, *context.meshes, *context.handles, *context.shaders);
    if (first != nullptr) {
        draw.material = first->material;
    }
    const std::size_t parts = context.meshes->get(model.mesh)->partNames.size();
    scratch.assets.assign(parts, model.material);
    scratch.created.clear();
    for (std::size_t part = 0; part < parts; ++part) {
        const core::asset::AssetId own =
            part < model.materials.size() ? model.materials.at(part) : core::asset::AssetId{};
        const shader::ShaderHandles::Material* created =
            materialOf(own, *context.meshes, *context.handles, *context.shaders);
        if (own != core::asset::AssetId{}) {
            scratch.assets.at(part) = own;
        }
        first = first != nullptr ? first : created;
        scratch.created.push_back(created != nullptr ? created->material : contract::renderer::MaterialId{});
    }
    if (first != nullptr) {
        draw.instanceValues = instance != nullptr ? instance->values : first->instanceDefaults;
    }
    draw.partMaterials = scratch.created;
    renderer.drawStaticMesh(draw);
    drawNextPasses(renderer, draw, scratch, *context.meshes, *context.materials, *context.handles, *context.shaders);
}

/// @brief Render: creates the meshes whose textures are ready, then draws each MeshRenderer whose mesh is loaded, at
/// its GlobalTransform3D (drawModel()). One system, placed among the GraphicsPlugin's own, so no other plugin's use of
/// the backend needs an order with it.
void drawMeshes(
    ecs::NonSendMut<graphics::Backend> backend, ecs::ResMut<MeshAssets> meshes,
    ecs::Res<graphics::TextureAssets> textures, ecs::Res<shader::ShaderHandles> handles,
    ecs::Res<shader::ShaderAssets> shaders, ecs::Res<shader::MaterialAssets> materials,
    ecs::Query<const transform::GlobalTransform3D, const MeshRenderer, ecs::Optional<const shader::MaterialInstance>>
        renderers) {
    contract::renderer::IRenderer* renderer = rendererOf(*backend);
    if (renderer == nullptr) {
        return;
    }
    uploadMeshes(*renderer, *meshes, *textures);
    if (!renderer->supports3D()) {
        return;
    }
    PassScratch scratch;
    const ModelContext context{
        .meshes = &*meshes, .materials = &*materials, .handles = &*handles, .shaders = &*shaders};
    for (auto [entity, global, model, instance] : renderers) {
        if (const std::optional<contract::renderer::StaticMeshId> mesh = meshes->getMesh(model.mesh)) {
            drawModel(*renderer, *mesh, transform::toMatrix(global), model, instance, scratch, context);
        }
    }
}

}  // namespace

std::string_view MeshPlugin::getName() const noexcept { return kName; }

std::span<const std::string_view> MeshPlugin::getDependencies() const noexcept {
    static constexpr std::array<std::string_view, 3> kDependencies{
        graphics::GraphicsPlugin::kName, shader::ShaderPlugin::kName, transform::TransformPlugin::kName};
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
