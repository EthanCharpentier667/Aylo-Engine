/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StaticMeshPass
*/

#include "StaticMeshPass.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "ShaderLayout.hpp"
#include "aylo/contract/renderer/drawing/StaticMesh.hpp"
#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Buffer.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/ShaderCache.hpp"
#include "staticmesh.frag.h"
#include "staticmesh.vert.h"

namespace aylo::render::vulkan::rendering {

namespace {

using contract::renderer::StaticMeshVertex;

static_assert(sizeof(StaticMeshVertex) == 32 && offsetof(StaticMeshVertex, normal) == 12 &&
                  offsetof(StaticMeshVertex, uv) == 24,
              "StaticMeshVertex must stay tightly packed: the pipeline's attribute offsets assume it");
static_assert(sizeof(StaticMeshConstants) <= kMaxPushConstantsSize, "The push constants must fit the shared range");

/// @return The static mesh pipeline.
pipeline::GraphicsPipeline createPipeline(const core::Device& device, pipeline::ShaderCache& shaders,
                                          const pipeline::PipelineLayout& layout, vk::Format colorFormat,
                                          vk::Format depthFormat, const vk::raii::PipelineCache* cache) {
    const std::array bindings{
        vk::VertexInputBindingDescription{0, sizeof(StaticMeshVertex), vk::VertexInputRate::eVertex}};
    const std::array attributes{
        vk::VertexInputAttributeDescription{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(StaticMeshVertex, position)},
        vk::VertexInputAttributeDescription{1, 0, vk::Format::eR32G32B32Sfloat, offsetof(StaticMeshVertex, normal)},
        vk::VertexInputAttributeDescription{2, 0, vk::Format::eR32G32Sfloat, offsetof(StaticMeshVertex, uv)},
    };
    return pipeline::GraphicsPipeline::Builder{}
        .setShaders(shaders.get("staticmesh.vert", aylo_shader_staticmesh_vert),
                    shaders.get("staticmesh.frag", aylo_shader_staticmesh_frag))
        .setVertexInput(bindings, attributes)
        .setBlend(pipeline::GraphicsPipeline::BlendMode::kPremultipliedAlpha)
        .setColorFormat(colorFormat)
        .setDepth(depthFormat, true)
        .setLayout(layout)
        .build(device, cache);
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

StaticMeshPass::StaticMeshPass(const core::Device& device, pipeline::ShaderCache& shaders,
                               const pipeline::PipelineLayout& layout, vk::Format colorFormat, vk::Format depthFormat,
                               const vk::raii::PipelineCache* cache)
    : _pipeline{createPipeline(device, shaders, layout, colorFormat, depthFormat, cache)} {
    device.setName(*_pipeline.getPipeline(), "Static mesh pipeline");
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
    return GpuStaticMesh{.vertices = std::move(vertices), .indices = std::move(indices), .parts = std::move(parts)};
}

void StaticMeshPass::record(const vk::raii::CommandBuffer& commandBuffer, std::span<const Draw> draws,
                            const vk::Viewport& viewport, const vk::Rect2D& scissor, const SetFinder& findSet,
                            vk::DescriptorSet whiteSet) const {
    if (draws.empty()) {
        return;
    }
    _pipeline.bind(commandBuffer);
    commandBuffer.setViewport(0, viewport);
    commandBuffer.setScissor(0, scissor);
    const vk::PipelineLayout layout = *_pipeline.getLayout().getLayout();
    const vk::ShaderStageFlags stages = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
    vk::DescriptorSet bound;
    for (const Draw& draw : draws) {
        if (draw.mesh == nullptr) {
            continue;
        }
        commandBuffer.bindVertexBuffers(0, *draw.mesh->vertices.getBuffer(), vk::DeviceSize{0});
        commandBuffer.bindIndexBuffer(*draw.mesh->indices.getBuffer(), 0, vk::IndexType::eUint32);
        for (const contract::renderer::StaticMeshPart& part : draw.mesh->parts) {
            vk::DescriptorSet set =
                part.texture != contract::renderer::TextureId{} ? findSet(part.texture) : vk::DescriptorSet{};
            if (!set) {
                set = whiteSet;
            }
            if (set != bound) {
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, kTextureSet, set, {});
                bound = set;
            }
            const StaticMeshConstants constants{.world = draw.world, .color = draw.color, .baseColor = part.baseColor};
            commandBuffer.pushConstants<StaticMeshConstants>(layout, stages, 0, constants);
            commandBuffer.drawIndexed(part.indexCount, 1, part.indexOffset, 0, 0);
        }
    }
}

}  // namespace aylo::render::vulkan::rendering
