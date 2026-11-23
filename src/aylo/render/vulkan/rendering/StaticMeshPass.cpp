/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StaticMeshPass
*/

#include "StaticMeshPass.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "ShaderLayout.hpp"
#include "aylo/contract/renderer/drawing/Light3D.hpp"
#include "aylo/contract/renderer/drawing/StaticMesh.hpp"
#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/descriptor/DescriptorPool.hpp"
#include "render/vulkan/descriptor/DescriptorSetLayout.hpp"
#include "render/vulkan/descriptor/DescriptorWriter.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Buffer.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/ShaderCache.hpp"
#include "render/vulkan/rendering/LightData.hpp"
#include "render/vulkan/rendering/MeshDrawOrder.hpp"
#include "render/vulkan/rendering/MeshPartTextures.hpp"
#include "render/vulkan/rendering/StaticMeshDrawData.hpp"
#include "staticmesh.frag.h"
#include "staticmesh.vert.h"
#include "staticmesh_lit.frag.h"
#include "staticmesh_shadow.frag.h"
#include "staticmesh_shadow_cutout.frag.h"

namespace aylo::render::vulkan::rendering {

namespace {

using contract::renderer::StaticMeshVertex;

static_assert(sizeof(StaticMeshVertex) == 32 && offsetof(StaticMeshVertex, normal) == 12 &&
                  offsetof(StaticMeshVertex, uv) == 24,
              "StaticMeshVertex must stay tightly packed: the pipeline's attribute offsets assume it");
static_assert(sizeof(StaticMeshConstants) == 80 && offsetof(StaticMeshConstants, draw) == 64 &&
                  offsetof(StaticMeshConstants, uvRotation) == 72,
              "StaticMeshConstants must match AyloMeshPart of common.glsl");
static_assert(sizeof(StaticMeshConstants) <= kMaxPushConstantsSize, "The push constants must fit the shared range");

/// @brief Vertices the default vertex data covers at first; it doubles from there.
constexpr std::uint32_t kInitialDefaultVertices = 4096;
/// @brief Bytes of default data per vertex: a tangent, a colour, a second UV.
constexpr std::size_t kDefaultVertexBytes = (2 * sizeof(glm::vec4)) + sizeof(glm::vec2);

/// @brief Draws a buffer of mesh draws holds when it is first made: it doubles from there.
constexpr std::size_t kInitialDraws = 64;

/// @return A pool for one set of one storage buffer per frame in flight.
descriptor::DescriptorPool createPool(const core::Device& device, std::size_t framesInFlight) {
    const auto frames = static_cast<std::uint32_t>(framesInFlight);
    // Two sets per frame (the pictures' and the shadow pass's), each with three buffers, the atlas and the sky's two
    // maps.
    const std::array sizes{vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, frames * 6},
                           vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, frames * 6}};
    return descriptor::DescriptorPool{device, frames * 2, sizes};
}

/// @return The blend preset of @p blend, for a fragment whose COLOR is premultiplied.
pipeline::GraphicsPipeline::BlendMode blendModeOf(contract::renderer::MeshBlend blend) noexcept {
    switch (blend) {
        case contract::renderer::MeshBlend::kOpaque:
            return pipeline::GraphicsPipeline::BlendMode::kOpaque;
        case contract::renderer::MeshBlend::kAdditive:
            return pipeline::GraphicsPipeline::BlendMode::kPremultipliedAdditive;
        case contract::renderer::MeshBlend::kMultiply:
            return pipeline::GraphicsPipeline::BlendMode::kMultiply;
        case contract::renderer::MeshBlend::kAlpha:
        case contract::renderer::MeshBlend::kPremultiplied:
            break;
    }
    return pipeline::GraphicsPipeline::BlendMode::kPremultipliedAlpha;
}

/// @return The faces @p cull discards.
vk::CullModeFlags cullModeOf(contract::renderer::CullMode cull) noexcept {
    switch (cull) {
        case contract::renderer::CullMode::kBack:
            return vk::CullModeFlagBits::eBack;
        case contract::renderer::CullMode::kFront:
            return vk::CullModeFlagBits::eFront;
        case contract::renderer::CullMode::kNone:
            break;
    }
    return vk::CullModeFlagBits::eNone;
}

/// @return A static mesh pipeline drawing with @p vertex and @p fragment; without @p state, the built-in one's:
/// premultiplied alpha, depth tested and written, no culling.
pipeline::GraphicsPipeline buildPipeline(const core::Device& device, const vk::raii::ShaderModule& vertex,
                                         const vk::raii::ShaderModule& fragment, const pipeline::PipelineLayout& layout,
                                         vk::Format colorFormat, vk::Format depthFormat,
                                         const vk::raii::PipelineCache* cache,
                                         const contract::renderer::RenderState* state) {
    // The vertices, then the tangents, the colours and the second UVs, each in a buffer of its own.
    const std::array bindings{
        vk::VertexInputBindingDescription{0, sizeof(StaticMeshVertex), vk::VertexInputRate::eVertex},
        vk::VertexInputBindingDescription{1, sizeof(glm::vec4), vk::VertexInputRate::eVertex},
        vk::VertexInputBindingDescription{2, sizeof(glm::vec4), vk::VertexInputRate::eVertex},
        vk::VertexInputBindingDescription{3, sizeof(glm::vec2), vk::VertexInputRate::eVertex}};
    const std::array attributes{
        vk::VertexInputAttributeDescription{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(StaticMeshVertex, position)},
        vk::VertexInputAttributeDescription{1, 0, vk::Format::eR32G32B32Sfloat, offsetof(StaticMeshVertex, normal)},
        vk::VertexInputAttributeDescription{2, 0, vk::Format::eR32G32Sfloat, offsetof(StaticMeshVertex, uv)},
        vk::VertexInputAttributeDescription{3, 1, vk::Format::eR32G32B32A32Sfloat, 0},
        vk::VertexInputAttributeDescription{4, 2, vk::Format::eR32G32B32A32Sfloat, 0},
        vk::VertexInputAttributeDescription{5, 3, vk::Format::eR32G32Sfloat, 0},
    };
    pipeline::GraphicsPipeline::Builder builder;
    builder.setShaders(vertex, fragment)
        .setVertexInput(bindings, attributes)
        .setBlend(pipeline::GraphicsPipeline::BlendMode::kPremultipliedAlpha)
        .setColorFormat(colorFormat)
        .setDepth(depthFormat, true)
        .setLayout(layout);
    if (state != nullptr) {
        builder.setBlend(blendModeOf(state->blend))
            .setCullMode(cullModeOf(state->cull))
            .setDepth(depthFormat, state->depth == contract::renderer::DepthMode::kTestWrite)
            .setDepthTest(state->depth != contract::renderer::DepthMode::kNoTest)
            .setDepthBias(state->depthBias, state->depthBiasSlope);
    }
    return builder.build(device, cache);
}

/// @throws std::invalid_argument If an index of @p desc names no vertex, or a part goes past its indices.
void validate(const contract::renderer::StaticMeshDesc& desc) {
    if (desc.vertices.empty() || desc.indices.empty()) {
        throw std::invalid_argument("A static mesh needs vertices and indices");
    }
    for (const std::uint32_t index : desc.indices) {
        if (index >= desc.vertices.size()) {
            throw std::invalid_argument(
                std::format("A static mesh index ({}) names no vertex (it has {})", index, desc.vertices.size()));
        }
    }
    for (const contract::renderer::StaticMeshPart& part : desc.parts) {
        if (static_cast<std::size_t>(part.indexOffset) + part.indexCount > desc.indices.size()) {
            throw std::invalid_argument("A static mesh part goes past its indices");
        }
    }
    const auto fits = [&desc](std::size_t count) { return count == 0 || count == desc.vertices.size(); };
    if (!fits(desc.tangents.size()) || !fits(desc.colors.size()) || !fits(desc.uv2.size())) {
        throw std::invalid_argument("A static mesh's tangents, colours and second UVs are empty or one per vertex");
    }
}

}  // namespace

StaticMeshPass::StaticMeshPass(const core::Device& device, const memory::Allocator& allocator,
                               pipeline::ShaderCache& shaders, const pipeline::PipelineLayout& layout,
                               const descriptor::DescriptorSetLayout& drawSetLayout, vk::Format colorFormat,
                               vk::Format depthFormat, const vk::raii::PipelineCache* cache, std::size_t framesInFlight,
                               vk::ImageView shadowAtlas, vk::ImageView fallback, vk::Sampler sampler)
    : _device{&device},
      _allocator{&allocator},
      _shaders{&shaders},
      _layout{&layout},
      _colorFormat{colorFormat},
      _depthFormat{depthFormat},
      _cache{cache},
      _pool{createPool(device, framesInFlight)},
      _frames(framesInFlight) {
    device.setName(*_pool.getPool(), "Static mesh draws pool");
    // No light at first: the header alone, so the binding is valid before the first frame sets lights.
    const std::array<std::byte, sizeof(GpuLightsHeader)> noLights{};
    for (std::size_t index = 0; index < _frames.size(); ++index) {
        Frame& frame = _frames.at(index);
        frame.set = _pool.allocate(drawSetLayout);
        frame.shadowSet = _pool.allocate(drawSetLayout);
        device.setName(*frame.set, "Static mesh draws set");
        device.setName(*frame.shadowSet, "Static mesh shadow draws set");
        // No sky at first: the fallback, which the shaders do not read while the lights say no sky.
        descriptor::DescriptorWriter{}
            .writeImage(kShadowAtlasBinding, shadowAtlas, sampler, vk::ImageLayout::eShaderReadOnlyOptimal)
            .writeImage(kSkySpecularBinding, fallback, sampler, vk::ImageLayout::eShaderReadOnlyOptimal)
            .writeImage(kSkyDiffuseBinding, fallback, sampler, vk::ImageLayout::eShaderReadOnlyOptimal)
            .update(device, *frame.set);
        descriptor::DescriptorWriter{}
            .writeImage(kShadowAtlasBinding, fallback, sampler, vk::ImageLayout::eShaderReadOnlyOptimal)
            .writeImage(kSkySpecularBinding, fallback, sampler, vk::ImageLayout::eShaderReadOnlyOptimal)
            .writeImage(kSkyDiffuseBinding, fallback, sampler, vk::ImageLayout::eShaderReadOnlyOptimal)
            .update(device, *frame.shadowSet);
        frame.shadowTiles.emplace(allocator, kMaxShadowTiles * sizeof(ShadowTile),
                                  vk::BufferUsageFlagBits::eStorageBuffer, memory::Allocator::MemoryAccess::kStaging);
        device.setName(*frame.shadowTiles->getBuffer(), "Shadow tiles");
        writeBuffer(frame, kShadowTilesBinding, *frame.shadowTiles);
        uploadLights(index, noLights, false);
    }
}

const pipeline::GraphicsPipeline& StaticMeshPass::builtInPipeline(bool lit, bool doubleSided, bool blended) {
    const std::size_t index = (lit ? 4U : 0U) + (doubleSided ? 2U : 0U) + (blended ? 1U : 0U);
    std::optional<pipeline::GraphicsPipeline>& slot = _builtIn.at(index);
    if (!slot) {
        const contract::renderer::RenderState state{
            .cull = doubleSided ? contract::renderer::CullMode::kNone : contract::renderer::CullMode::kBack,
            .blend = blended ? contract::renderer::MeshBlend::kPremultiplied : contract::renderer::MeshBlend::kOpaque,
            .depth = blended ? contract::renderer::DepthMode::kTestNoWrite : contract::renderer::DepthMode::kTestWrite};
        slot.emplace(buildPipeline(*_device, _shaders->get("staticmesh.vert", aylo_shader_staticmesh_vert),
                                   lit ? _shaders->get("staticmesh_lit.frag", aylo_shader_staticmesh_lit_frag)
                                       : _shaders->get("staticmesh.frag", aylo_shader_staticmesh_frag),
                                   *_layout, _colorFormat, _depthFormat, _cache, &state));
        _device->setName(*slot->getPipeline(), lit ? "Static mesh lit pipeline" : "Static mesh pipeline");
    }
    return *slot;
}

void StaticMeshPass::writeBuffer(Frame& frame, std::uint32_t binding, const memory::Buffer& buffer) const {
    for (const vk::raii::DescriptorSet* set : {&frame.set, &frame.shadowSet}) {
        descriptor::DescriptorWriter{}
            .writeBuffer(binding, *buffer.getBuffer(), 0, buffer.getSize(), vk::DescriptorType::eStorageBuffer)
            .update(*_device, **set);
    }
}

pipeline::GraphicsPipeline StaticMeshPass::createShadowPipeline(const vk::raii::ShaderModule& vertex,
                                                                const vk::raii::ShaderModule* fragment,
                                                                contract::renderer::CullMode cull) const {
    const std::array bindings{
        vk::VertexInputBindingDescription{0, sizeof(StaticMeshVertex), vk::VertexInputRate::eVertex},
        vk::VertexInputBindingDescription{1, sizeof(glm::vec4), vk::VertexInputRate::eVertex},
        vk::VertexInputBindingDescription{2, sizeof(glm::vec4), vk::VertexInputRate::eVertex},
        vk::VertexInputBindingDescription{3, sizeof(glm::vec2), vk::VertexInputRate::eVertex}};
    const std::array attributes{
        vk::VertexInputAttributeDescription{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(StaticMeshVertex, position)},
        vk::VertexInputAttributeDescription{1, 0, vk::Format::eR32G32B32Sfloat, offsetof(StaticMeshVertex, normal)},
        vk::VertexInputAttributeDescription{2, 0, vk::Format::eR32G32Sfloat, offsetof(StaticMeshVertex, uv)},
        vk::VertexInputAttributeDescription{3, 1, vk::Format::eR32G32B32A32Sfloat, 0},
        vk::VertexInputAttributeDescription{4, 2, vk::Format::eR32G32B32A32Sfloat, 0},
        vk::VertexInputAttributeDescription{5, 3, vk::Format::eR32G32Sfloat, 0},
    };
    return pipeline::GraphicsPipeline::Builder{}
        .setShaders(vertex, fragment != nullptr
                                ? *fragment
                                : _shaders->get("staticmesh_shadow.frag", aylo_shader_staticmesh_shadow_frag))
        .setVertexInput(bindings, attributes)
        .setCullMode(cullModeOf(cull))
        .setDepth(kWorld3DDepthFormat, true)
        .setDynamicDepthBias()
        .setLayout(*_layout)
        .build(*_device, _cache);
}

const pipeline::GraphicsPipeline& StaticMeshPass::builtInShadowPipeline(bool cutout,
                                                                        contract::renderer::CullMode cull) {
    std::optional<pipeline::GraphicsPipeline>& slot =
        _builtInShadows.at((cutout ? 3U : 0U) + static_cast<std::size_t>(cull));
    if (!slot) {
        slot.emplace(createShadowPipeline(
            _shaders->get("staticmesh.vert", aylo_shader_staticmesh_vert),
            cutout ? &_shaders->get("staticmesh_shadow_cutout.frag", aylo_shader_staticmesh_shadow_cutout_frag)
                   : nullptr,
            cull));
        _device->setName(*slot->getPipeline(), "Static mesh shadow pipeline");
    }
    return *slot;
}

vk::DescriptorSet StaticMeshPass::partTexturesOf(const Draw& draw, std::size_t frameIndex,
                                                 const PartTextureSource& textures) const {
    std::vector<std::unique_ptr<MeshPartTextures>>& sets = draw.mesh->partTextures;
    if (sets.size() < draw.mesh->parts.size()) {
        sets.resize(draw.mesh->parts.size());
    }
    std::unique_ptr<MeshPartTextures>& set = sets.at(draw.part);
    if (!set) {
        set = std::make_unique<MeshPartTextures>(*_device, *textures.descriptors, *textures.layout,
                                                 textures.framesInFlight);
    }
    const contract::renderer::StaticMeshPart& part = draw.mesh->parts.at(draw.part);
    const MeshPartTextureIds wanted{part.texture, part.metallicRoughnessTexture, part.normalTexture,
                                    part.occlusionTexture, part.emissiveTexture};
    return set->prepare(*_device, frameIndex, wanted, textures.findTexture, textures.fallback);
}

const pipeline::GraphicsPipeline* StaticMeshPass::shadowPipelineOf(const Draw& draw,
                                                                   const contract::renderer::StaticMeshPart& part) {
    if (draw.material.pipeline != nullptr) {
        return contract::renderer::isTransparent(draw.material.state) ? nullptr : draw.material.shadowPipeline;
    }
    if (part.alphaMode == contract::renderer::AlphaMode::kBlend) {
        return nullptr;
    }
    return &builtInShadowPipeline(
        part.alphaMode == contract::renderer::AlphaMode::kMask,
        part.doubleSided ? contract::renderer::CullMode::kNone : contract::renderer::CullMode::kBack);
}

void StaticMeshPass::uploadShadows(std::size_t frameIndex, std::span<const ShadowTile> tiles) {
    Frame& frame = _frames.at(frameIndex);
    if (frame.shadowTiles) {
        frame.shadowTiles->write(std::as_bytes(tiles.first(std::min<std::size_t>(tiles.size(), kMaxShadowTiles))));
    }
}

void StaticMeshPass::recordShadows(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex,
                                   const ShadowTile& tile, const vk::Viewport& viewport, const vk::Rect2D& scissor,
                                   const PartTextureSource& textures) {
    const Frame& frame = _frames.at(frameIndex);
    if (frame.draws.empty() || !frame.buffer) {
        return;
    }
    commandBuffer.setViewport(0, viewport);
    commandBuffer.setScissor(0, scissor);
    // A constant bias of a few depth steps and one along the slope, both scaled by the light's.
    commandBuffer.setDepthBias(1.25F * tile.bias.z, 0.0F, 1.75F * tile.bias.z);
    const vk::PipelineLayout layout = *_layout->getLayout();
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, kMeshDrawSet, *frame.shadowSet, {});
    const vk::ShaderStageFlags stages = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
    const pipeline::GraphicsPipeline* boundPipeline = nullptr;
    const GpuStaticMesh* boundMesh = nullptr;
    for (std::uint32_t index = 0; index < frame.draws.size(); ++index) {
        const Draw& draw = frame.draws.at(index);
        if (!draw.castShadows || draw.mesh == nullptr || draw.part >= draw.mesh->parts.size()) {
            continue;
        }
        const contract::renderer::StaticMeshPart& part = draw.mesh->parts.at(draw.part);
        const pipeline::GraphicsPipeline* used = shadowPipelineOf(draw, part);
        if (used == nullptr) {
            continue;
        }
        if (used != boundPipeline) {
            used->bind(commandBuffer);
            boundPipeline = used;
        }
        if (draw.material.pipeline != nullptr) {
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, kMaterialSet, draw.material.set,
                                             {});
        }
        if (draw.mesh != boundMesh) {
            bindMesh(commandBuffer, *draw.mesh);
            boundMesh = draw.mesh;
        }
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, kTextureSet,
                                         partTexturesOf(draw, frameIndex, textures), {});
        commandBuffer.pushConstants<StaticMeshConstants>(layout, stages, 0, StaticMeshConstants::of(part, index));
        commandBuffer.drawIndexed(part.indexCount, 1, part.indexOffset, 0, 0);
    }
}

void StaticMeshPass::ensureDefaults(std::uint32_t vertices) {
    if (_defaults.buffer && _defaults.capacity >= vertices) {
        return;
    }
    std::uint32_t capacity = std::max(_defaults.capacity, kInitialDefaultVertices);
    while (capacity < vertices) {
        capacity *= 2;
    }
    if (_defaults.buffer) {
        _defaults.retired.push_back(std::move(*_defaults.buffer));
    }
    std::vector<std::byte> bytes(capacity * kDefaultVertexBytes);
    const glm::vec4 tangent{1.0F, 0.0F, 0.0F, 1.0F};
    const glm::vec4 color{1.0F};
    for (std::uint32_t vertex = 0; vertex < capacity; ++vertex) {
        std::memcpy(std::span{bytes}.subspan(vertex * sizeof(glm::vec4), sizeof(glm::vec4)).data(), &tangent,
                    sizeof(tangent));
        std::memcpy(std::span{bytes}.subspan((capacity + vertex) * sizeof(glm::vec4), sizeof(glm::vec4)).data(), &color,
                    sizeof(color));
    }
    _defaults.buffer.emplace(*_allocator, bytes.size(), vk::BufferUsageFlagBits::eVertexBuffer,
                             memory::Allocator::MemoryAccess::kStaging);
    _defaults.buffer->write(bytes);
    _defaults.capacity = capacity;
    _device->setName(*_defaults.buffer->getBuffer(), "Static mesh default vertex data");
}

void StaticMeshPass::bindMesh(const vk::raii::CommandBuffer& commandBuffer, const GpuStaticMesh& mesh) {
    ensureDefaults(static_cast<std::uint32_t>(mesh.vertices.getSize() / sizeof(StaticMeshVertex)));
    if (!_defaults.buffer) {
        return;
    }
    const vk::Buffer defaults = *_defaults.buffer->getBuffer();
    const vk::DeviceSize capacity = _defaults.capacity;
    const std::array buffers{*mesh.vertices.getBuffer(), mesh.tangents ? *mesh.tangents->getBuffer() : defaults,
                             mesh.colors ? *mesh.colors->getBuffer() : defaults,
                             mesh.uv2 ? *mesh.uv2->getBuffer() : defaults};
    const std::array<vk::DeviceSize, 4> offsets{0, 0, mesh.colors ? 0 : capacity * sizeof(glm::vec4),
                                                mesh.uv2 ? 0 : capacity * 2 * sizeof(glm::vec4)};
    commandBuffer.bindVertexBuffers(0, buffers, offsets);
    commandBuffer.bindIndexBuffer(*mesh.indices.getBuffer(), 0, vk::IndexType::eUint32);
}

void StaticMeshPass::setSky(std::size_t frameIndex, vk::ImageView specular, vk::ImageView diffuse,
                            vk::Sampler sampler) {
    descriptor::DescriptorWriter{}
        .writeImage(kSkySpecularBinding, specular, sampler, vk::ImageLayout::eShaderReadOnlyOptimal)
        .writeImage(kSkyDiffuseBinding, diffuse, sampler, vk::ImageLayout::eShaderReadOnlyOptimal)
        .update(*_device, *_frames.at(frameIndex).set);
}

void StaticMeshPass::uploadLights(std::size_t frameIndex, std::span<const std::byte> bytes, bool lit) {
    Frame& frame = _frames.at(frameIndex);
    frame.lit = lit;
    if (!frame.lights || frame.lights->getSize() < bytes.size()) {
        // Room for every light a frame may have: the buffer is made once.
        const vk::DeviceSize size = std::max<vk::DeviceSize>(
            bytes.size(), sizeof(GpuLightsHeader) + (contract::renderer::kMaxLights3D * sizeof(GpuLight)));
        frame.lights.emplace(*_allocator, size, vk::BufferUsageFlagBits::eStorageBuffer,
                             memory::Allocator::MemoryAccess::kStaging);
        const core::Device& device = _allocator->getDevice();
        device.setName(*frame.lights->getBuffer(), "Lights 3D");
        writeBuffer(frame, kLightsBinding, *frame.lights);
    }
    frame.lights->write(bytes);
}

pipeline::GraphicsPipeline StaticMeshPass::createMaterialPipeline(
    const core::Device& device, const vk::raii::ShaderModule& vertex, const vk::raii::ShaderModule& fragment,
    bool /*instanceValues*/, const pipeline::PipelineLayout& layout, vk::Format colorFormat,
    const vk::raii::PipelineCache* cache, const contract::renderer::RenderState& state) {
    return buildPipeline(device, vertex, fragment, layout, colorFormat, kWorld3DDepthFormat, cache, &state);
}

GpuStaticMesh StaticMeshPass::upload(const memory::Allocator& allocator,
                                     const contract::renderer::StaticMeshDesc& desc) {
    validate(desc);
    memory::Buffer vertices{allocator, std::as_bytes(std::span{desc.vertices}).size(),
                            vk::BufferUsageFlagBits::eVertexBuffer, memory::Allocator::MemoryAccess::kStaging};
    vertices.write(std::as_bytes(std::span{desc.vertices}));
    memory::Buffer indices{allocator, std::as_bytes(std::span{desc.indices}).size(),
                           vk::BufferUsageFlagBits::eIndexBuffer, memory::Allocator::MemoryAccess::kStaging};
    indices.write(std::as_bytes(std::span{desc.indices}));
    allocator.getDevice().setName(*vertices.getBuffer(), "Static mesh vertices");
    allocator.getDevice().setName(*indices.getBuffer(), "Static mesh indices");
    std::vector<contract::renderer::StaticMeshPart> parts = desc.parts;
    if (parts.empty()) {
        // No part: the whole mesh in plain white.
        parts.push_back(contract::renderer::StaticMeshPart{
            .indexOffset = 0, .indexCount = static_cast<std::uint32_t>(desc.indices.size())});
    }
    GpuStaticMesh mesh{.vertices = std::move(vertices),
                       .bounds = contract::renderer::computeBounds(desc),
                       .indices = std::move(indices),
                       .parts = std::move(parts)};
    const auto extra = [&allocator](std::span<const std::byte> bytes, const char* name) {
        std::optional<memory::Buffer> buffer;
        if (!bytes.empty()) {
            buffer.emplace(allocator, bytes.size(), vk::BufferUsageFlagBits::eVertexBuffer,
                           memory::Allocator::MemoryAccess::kStaging);
            buffer->write(bytes);
            allocator.getDevice().setName(*buffer->getBuffer(), name);
        }
        return buffer;
    };
    mesh.tangents = extra(std::as_bytes(std::span{desc.tangents}), "Static mesh tangents");
    mesh.colors = extra(std::as_bytes(std::span{desc.colors}), "Static mesh colours");
    mesh.uv2 = extra(std::as_bytes(std::span{desc.uv2}), "Static mesh second UVs");
    return mesh;
}

void StaticMeshPass::upload(std::size_t frameIndex, std::span<const Draw> draws) {
    Frame& frame = _frames.at(frameIndex);
    frame.draws.assign(draws.begin(), draws.end());
    if (draws.empty()) {
        return;
    }
    _staging.clear();
    frame.keys.clear();
    for (const Draw& draw : draws) {
        const contract::renderer::StaticMeshBounds bounds =
            draw.mesh != nullptr ? draw.mesh->bounds : contract::renderer::StaticMeshBounds{};
        _staging.push_back(
            makeStaticMeshDrawData(draw.world, draw.color, bounds, draw.instanceValues, draw.receiveShadows));
        const glm::vec3 center = glm::vec3{draw.world * glm::vec4{(bounds.min + bounds.max) * 0.5F, 1.0F}};
        const bool blendedPart = draw.mesh != nullptr && draw.part < draw.mesh->parts.size() &&
                                 draw.mesh->parts.at(draw.part).alphaMode == contract::renderer::AlphaMode::kBlend;
        frame.keys.push_back(MeshDrawKey{.center = center,
                                         .transparent = draw.material.pipeline != nullptr
                                                            ? contract::renderer::isTransparent(draw.material.state)
                                                            : blendedPart});
    }
    const std::span<const std::byte> bytes = std::as_bytes(std::span{_staging});
    if (!frame.buffer || frame.buffer->getSize() < bytes.size()) {
        std::size_t capacity = kInitialDraws;
        while (capacity < draws.size()) {
            capacity *= 2;
        }
        // The frame's fence was waited for: neither its old buffer nor its set is in use any more.
        frame.buffer.emplace(*_allocator, capacity * sizeof(StaticMeshDrawData),
                             vk::BufferUsageFlagBits::eStorageBuffer, memory::Allocator::MemoryAccess::kStaging);
        const core::Device& device = _allocator->getDevice();
        device.setName(*frame.buffer->getBuffer(), "Static mesh draws");
        writeBuffer(frame, kMeshDrawBinding, *frame.buffer);
    }
    frame.buffer->write(bytes);
}

void StaticMeshPass::record(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex,
                            const vk::Viewport& viewport, const vk::Rect2D& scissor, const MeshViewpoint& viewpoint,
                            const PartTextureSource& textures) {
    const Frame& frame = _frames.at(frameIndex);
    if (frame.draws.empty() || !frame.buffer) {
        return;
    }
    orderMeshDraws(frame.keys, viewpoint, _order);
    commandBuffer.setViewport(0, viewport);
    commandBuffer.setScissor(0, scissor);
    const vk::ShaderStageFlags stages = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
    // Every mesh pipeline shares the static mesh layout: the draws' set stays bound across them.
    const vk::PipelineLayout layout = *_layout->getLayout();
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, kMeshDrawSet, *frame.set, {});
    const pipeline::GraphicsPipeline* boundPipeline = nullptr;
    const GpuStaticMesh* boundMesh = nullptr;
    for (const std::uint32_t index : _order) {
        const Draw& draw = frame.draws.at(index);
        if (draw.mesh == nullptr || draw.part >= draw.mesh->parts.size()) {
            continue;
        }
        const contract::renderer::StaticMeshPart& part = draw.mesh->parts.at(draw.part);
        const pipeline::GraphicsPipeline& used =
            draw.material.pipeline != nullptr
                ? *draw.material.pipeline
                : builtInPipeline(frame.lit, part.doubleSided, part.alphaMode == contract::renderer::AlphaMode::kBlend);
        if (&used != boundPipeline) {
            used.bind(commandBuffer);
            boundPipeline = &used;
        }
        if (draw.material.pipeline != nullptr) {
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, kMaterialSet, draw.material.set,
                                             {});
        }
        if (draw.mesh != boundMesh) {
            // The parts of one mesh follow each other: their buffers are bound once.
            bindMesh(commandBuffer, *draw.mesh);
            boundMesh = draw.mesh;
        }
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, kTextureSet,
                                         partTexturesOf(draw, frameIndex, textures), {});
        commandBuffer.pushConstants<StaticMeshConstants>(layout, stages, 0, StaticMeshConstants::of(part, index));
        commandBuffer.drawIndexed(part.indexCount, 1, part.indexOffset, 0, 0);
    }
}

}  // namespace aylo::render::vulkan::rendering
