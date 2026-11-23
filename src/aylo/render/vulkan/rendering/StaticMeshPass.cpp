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
#include <format>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
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
#include "render/vulkan/rendering/StaticMeshDrawData.hpp"
#include "staticmesh.frag.h"
#include "staticmesh.vert.h"
#include "staticmesh_lit.frag.h"

namespace aylo::render::vulkan::rendering {

namespace {

using contract::renderer::StaticMeshVertex;

static_assert(sizeof(StaticMeshVertex) == 32 && offsetof(StaticMeshVertex, normal) == 12 &&
                  offsetof(StaticMeshVertex, uv) == 24,
              "StaticMeshVertex must stay tightly packed: the pipeline's attribute offsets assume it");
static_assert(sizeof(StaticMeshConstants) == 32 && offsetof(StaticMeshConstants, draw) == 16,
              "StaticMeshConstants must match AyloMeshPart of common.glsl");
static_assert(sizeof(StaticMeshConstants) <= kMaxPushConstantsSize, "The push constants must fit the shared range");

/// @brief Draws a buffer of mesh draws holds when it is first made: it doubles from there.
constexpr std::size_t kInitialDraws = 64;

/// @return A pool for one set of one storage buffer per frame in flight.
descriptor::DescriptorPool createPool(const core::Device& device, std::size_t framesInFlight) {
    const auto frames = static_cast<std::uint32_t>(framesInFlight);
    const std::array sizes{vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, frames * 2}};
    return descriptor::DescriptorPool{device, frames, sizes};
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
    const std::array bindings{
        vk::VertexInputBindingDescription{0, sizeof(StaticMeshVertex), vk::VertexInputRate::eVertex}};
    const std::array attributes{
        vk::VertexInputAttributeDescription{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(StaticMeshVertex, position)},
        vk::VertexInputAttributeDescription{1, 0, vk::Format::eR32G32B32Sfloat, offsetof(StaticMeshVertex, normal)},
        vk::VertexInputAttributeDescription{2, 0, vk::Format::eR32G32Sfloat, offsetof(StaticMeshVertex, uv)},
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

/// @return The built-in static mesh pipeline, unlit or @p lit.
pipeline::GraphicsPipeline createPipeline(const core::Device& device, pipeline::ShaderCache& shaders,
                                          const pipeline::PipelineLayout& layout, vk::Format colorFormat,
                                          vk::Format depthFormat, const vk::raii::PipelineCache* cache, bool lit) {
    return buildPipeline(device, shaders.get("staticmesh.vert", aylo_shader_staticmesh_vert),
                         lit ? shaders.get("staticmesh_lit.frag", aylo_shader_staticmesh_lit_frag)
                             : shaders.get("staticmesh.frag", aylo_shader_staticmesh_frag),
                         layout, colorFormat, depthFormat, cache, nullptr);
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
}

}  // namespace

StaticMeshPass::StaticMeshPass(const core::Device& device, const memory::Allocator& allocator,
                               pipeline::ShaderCache& shaders, const pipeline::PipelineLayout& layout,
                               const descriptor::DescriptorSetLayout& drawSetLayout, vk::Format colorFormat,
                               vk::Format depthFormat, const vk::raii::PipelineCache* cache, std::size_t framesInFlight)
    : _allocator{&allocator},
      _layout{&layout},
      _pipeline{createPipeline(device, shaders, layout, colorFormat, depthFormat, cache, false)},
      _litPipeline{createPipeline(device, shaders, layout, colorFormat, depthFormat, cache, true)},
      _pool{createPool(device, framesInFlight)},
      _frames(framesInFlight) {
    device.setName(*_pipeline.getPipeline(), "Static mesh pipeline");
    device.setName(*_litPipeline.getPipeline(), "Static mesh lit pipeline");
    device.setName(*_pool.getPool(), "Static mesh draws pool");
    // No light at first: the header alone, so the binding is valid before the first frame sets lights.
    const std::array<std::byte, sizeof(GpuLightsHeader)> noLights{};
    for (std::size_t index = 0; index < _frames.size(); ++index) {
        Frame& frame = _frames.at(index);
        frame.set = _pool.allocate(drawSetLayout);
        device.setName(*frame.set, "Static mesh draws set");
        uploadLights(index, noLights, false);
    }
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
        descriptor::DescriptorWriter{}
            .writeBuffer(kLightsBinding, *frame.lights->getBuffer(), 0, frame.lights->getSize(),
                         vk::DescriptorType::eStorageBuffer)
            .update(device, *frame.set);
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
    return GpuStaticMesh{.vertices = std::move(vertices),
                         .bounds = contract::renderer::computeBounds(desc),
                         .indices = std::move(indices),
                         .parts = std::move(parts)};
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
        _staging.push_back(makeStaticMeshDrawData(draw.world, draw.color, bounds, draw.instanceValues));
        const glm::vec3 center = glm::vec3{draw.world * glm::vec4{(bounds.min + bounds.max) * 0.5F, 1.0F}};
        frame.keys.push_back(MeshDrawKey{.center = center,
                                         .transparent = draw.material.pipeline != nullptr &&
                                                        contract::renderer::isTransparent(draw.material.state)});
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
        descriptor::DescriptorWriter{}
            .writeBuffer(kMeshDrawBinding, *frame.buffer->getBuffer(), 0, frame.buffer->getSize(),
                         vk::DescriptorType::eStorageBuffer)
            .update(device, *frame.set);
    }
    frame.buffer->write(bytes);
}

void StaticMeshPass::record(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex,
                            const vk::Viewport& viewport, const vk::Rect2D& scissor, const MeshViewpoint& viewpoint,
                            const SetFinder& findSet, vk::DescriptorSet whiteSet) {
    const Frame& frame = _frames.at(frameIndex);
    if (frame.draws.empty() || !frame.buffer) {
        return;
    }
    orderMeshDraws(frame.keys, viewpoint, _order);
    commandBuffer.setViewport(0, viewport);
    commandBuffer.setScissor(0, scissor);
    const vk::ShaderStageFlags stages = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
    // Every mesh pipeline shares the static mesh layout: the draws' set stays bound across them.
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *_layout->getLayout(), kMeshDrawSet, *frame.set,
                                     {});
    const pipeline::GraphicsPipeline* boundPipeline = nullptr;
    const GpuStaticMesh* boundMesh = nullptr;
    vk::DescriptorSet bound;
    for (const std::uint32_t index : _order) {
        const Draw& draw = frame.draws.at(index);
        if (draw.mesh == nullptr) {
            continue;
        }
        const pipeline::GraphicsPipeline& builtIn = frame.lit ? _litPipeline : _pipeline;
        const pipeline::GraphicsPipeline& used = draw.material.pipeline != nullptr ? *draw.material.pipeline : builtIn;
        const vk::PipelineLayout layout = *used.getLayout().getLayout();
        if (&used != boundPipeline) {
            used.bind(commandBuffer);
            boundPipeline = &used;
        }
        if (draw.material.pipeline != nullptr) {
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, kMaterialSet, draw.material.set,
                                             {});
        }
        if (draw.part >= draw.mesh->parts.size()) {
            continue;
        }
        if (draw.mesh != boundMesh) {
            // The parts of one mesh follow each other: their buffers are bound once.
            commandBuffer.bindVertexBuffers(0, *draw.mesh->vertices.getBuffer(), vk::DeviceSize{0});
            commandBuffer.bindIndexBuffer(*draw.mesh->indices.getBuffer(), 0, vk::IndexType::eUint32);
            boundMesh = draw.mesh;
        }
        const contract::renderer::StaticMeshPart& part = draw.mesh->parts.at(draw.part);
        vk::DescriptorSet set =
            part.texture != contract::renderer::TextureId{} ? findSet(part.texture) : vk::DescriptorSet{};
        if (!set) {
            set = whiteSet;
        }
        if (set != bound) {
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, kTextureSet, set, {});
            bound = set;
        }
        const StaticMeshConstants constants{.baseColor = part.baseColor, .draw = index};
        commandBuffer.pushConstants<StaticMeshConstants>(layout, stages, 0, constants);
        commandBuffer.drawIndexed(part.indexCount, 1, part.indexOffset, 0, 0);
    }
}

}  // namespace aylo::render::vulkan::rendering
