/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Lines3DPass
*/

#pragma once

#include <cstddef>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <optional>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Buffer.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/ShaderCache.hpp"

namespace aylo::render::vulkan::rendering {

/// @brief One end of a 3D line in the vertex buffer: 28 bytes, read by shaders/lines3d.vert as
/// layout(location = 0) in vec3 position; layout(location = 1) in vec4 color;
struct LineVertex3D {
    glm::vec3 position{0.0F};  ///< In the world. Location 0.
    glm::vec4 color{1.0F};     ///< Linear RGBA, not premultiplied. Location 1.
};

/// @brief Draws the frame's 3D lines (IRenderer::drawLines3D()): a line list in the world, with the depth test and
/// depth writes, premultiplied alpha.
///
/// @details Owns the pipeline of shaders/lines3d.vert and lines3d.frag (on the shared layout) and, per frame in flight,
/// a CPU-written vertex buffer, rewritten or replaced without waiting, as MeshPass's. The frame's set 0 must hold the
/// 3D view's projection while it records, and the rendering scope a depth attachment of the depth format given.
/// @warning Destroy it before the Allocator and the Device.
class AYLO_RENDER_VULKAN_API Lines3DPass {
  public:
    /// @param shaders Gives the built-in line shaders.
    /// @param layout The shared layout (ShaderLayout.hpp); must outlive the pass.
    /// @param colorFormat The format of the images drawn into: the scene's.
    /// @param depthFormat The format of the depth attachment.
    /// @param cache Speeds up the pipeline's creation, or null.
    /// @throws vk::SystemError If the pipeline cannot be created.
    Lines3DPass(const core::Device& device, const memory::Allocator& allocator, pipeline::ShaderCache& shaders,
                const pipeline::PipelineLayout& layout, vk::Format colorFormat, vk::Format depthFormat,
                const vk::raii::PipelineCache* cache, std::size_t framesInFlight);
    ~Lines3DPass() = default;
    Lines3DPass(const Lines3DPass&) = delete;
    Lines3DPass& operator=(const Lines3DPass&) = delete;
    Lines3DPass(Lines3DPass&&) = delete;
    Lines3DPass& operator=(Lines3DPass&&) = delete;

    /// @brief Copies @p vertices (two per line) into frame @p frameIndex's buffer, for record().
    /// @throws vk::SystemError If a bigger buffer cannot be created.
    void upload(std::size_t frameIndex, std::span<const LineVertex3D> vertices);

    /// @brief Records the lines uploaded for this frame inside dynamic rendering. Nothing without lines.
    /// @param viewport Where the 3D world goes in the image (the letterboxed viewport).
    void record(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex, const vk::Viewport& viewport,
                const vk::Rect2D& scissor) const;

  private:
    const memory::Allocator* _allocator;                   ///< Allocates the buffers.
    pipeline::GraphicsPipeline _pipeline;                  ///< The lines pipeline.
    std::vector<std::optional<memory::Buffer>> _vertices;  ///< The vertices, per frame in flight.
    std::vector<std::size_t> _counts;                      ///< The vertices uploaded, per frame in flight.
};

}  // namespace aylo::render::vulkan::rendering
