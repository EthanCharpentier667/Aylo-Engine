/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Lines3DPass
*/

#include "Lines3DPass.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vulkan/vulkan_raii.hpp>

#include "lines3d.frag.h"
#include "lines3d.vert.h"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Buffer.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/ShaderCache.hpp"

namespace aylo::render::vulkan::rendering {

namespace {

/// @brief Vertices a buffer holds when it is first made: it doubles from there.
constexpr std::size_t kInitialCapacity = 1024;

static_assert(sizeof(LineVertex3D) == 28 && offsetof(LineVertex3D, color) == 12,
              "LineVertex3D must stay tightly packed: its attribute offsets assume it");

/// @return The lines pipeline.
pipeline::GraphicsPipeline createPipeline(const core::Device& device, pipeline::ShaderCache& shaders,
                                          const pipeline::PipelineLayout& layout, vk::Format colorFormat,
                                          vk::Format depthFormat, const vk::raii::PipelineCache* cache) {
    const std::array bindings{vk::VertexInputBindingDescription{0, sizeof(LineVertex3D), vk::VertexInputRate::eVertex}};
    const std::array attributes{
        vk::VertexInputAttributeDescription{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(LineVertex3D, position)},
        vk::VertexInputAttributeDescription{1, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(LineVertex3D, color)},
    };
    return pipeline::GraphicsPipeline::Builder{}
        .setShaders(shaders.get("lines3d.vert", aylo_shader_lines3d_vert),
                    shaders.get("lines3d.frag", aylo_shader_lines3d_frag))
        .setVertexInput(bindings, attributes)
        .setTopology(vk::PrimitiveTopology::eLineList)
        .setBlend(pipeline::GraphicsPipeline::BlendMode::kPremultipliedAlpha)
        .setColorFormat(colorFormat)
        .setDepth(depthFormat, true)
        .setLayout(layout)
        .build(device, cache);
}

}  // namespace

Lines3DPass::Lines3DPass(const core::Device& device, const memory::Allocator& allocator, pipeline::ShaderCache& shaders,
                         const pipeline::PipelineLayout& layout, vk::Format colorFormat, vk::Format depthFormat,
                         const vk::raii::PipelineCache* cache, std::size_t framesInFlight)
    : _allocator{&allocator},
      _pipeline{createPipeline(device, shaders, layout, colorFormat, depthFormat, cache)},
      _vertices(framesInFlight),
      _counts(framesInFlight, 0) {
    device.setName(*_pipeline.getPipeline(), "Lines 3D pipeline");
}

void Lines3DPass::upload(std::size_t frameIndex, std::span<const LineVertex3D> vertices) {
    _counts.at(frameIndex) = vertices.size();
    if (vertices.empty()) {
        return;
    }
    std::optional<memory::Buffer>& buffer = _vertices.at(frameIndex);
    const vk::DeviceSize needed = vertices.size_bytes();
    if (!buffer || buffer->getSize() < needed) {
        std::size_t capacity = kInitialCapacity;
        while (capacity < vertices.size()) {
            capacity *= 2;
        }
        buffer.emplace(*_allocator, capacity * sizeof(LineVertex3D), vk::BufferUsageFlagBits::eVertexBuffer,
                       memory::Allocator::MemoryAccess::kStaging);
        _allocator->getDevice().setName(*buffer->getBuffer(), "Lines 3D vertices");
    }
    buffer->write(std::as_bytes(vertices));
}

void Lines3DPass::record(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex,
                         const vk::Viewport& viewport, const vk::Rect2D& scissor, std::size_t vertices) const {
    const std::optional<memory::Buffer>& buffer = _vertices.at(frameIndex);
    const std::size_t count = std::min(vertices, _counts.at(frameIndex));
    if (count < 2 || !buffer) {
        return;
    }
    _pipeline.bind(commandBuffer);
    commandBuffer.setViewport(0, viewport);
    commandBuffer.setScissor(0, scissor);
    commandBuffer.bindVertexBuffers(0, *buffer->getBuffer(), vk::DeviceSize{0});
    commandBuffer.draw(static_cast<std::uint32_t>(count), 1, 0, 0);
}

}  // namespace aylo::render::vulkan::rendering
