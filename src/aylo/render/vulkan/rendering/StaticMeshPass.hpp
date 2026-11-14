/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StaticMeshPass
*/

#pragma once

#include <functional>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float4.hpp>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "aylo/contract/renderer/drawing/StaticMesh.hpp"
#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Buffer.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/ShaderCache.hpp"

namespace aylo::render::vulkan::rendering {

/// @brief A static mesh uploaded to the GPU: its buffers and parts.
struct GpuStaticMesh {
    memory::Buffer vertices;                                ///< Its StaticMeshVertex values.
    memory::Buffer indices;                                 ///< Its indices, 32-bit.
    std::vector<contract::renderer::StaticMeshPart> parts;  ///< Its runs of indices and their materials.
};

/// @brief The push constants of a static mesh draw: what shaders/staticmesh.vert reads as `draw`, 96 bytes.
struct StaticMeshConstants {
    glm::mat4 world{1.0F};      ///< Places the mesh in the world.
    glm::vec4 color{1.0F};      ///< The draw's colour.
    glm::vec4 baseColor{1.0F};  ///< The part's base colour.
};

/// @brief Draws static meshes in the 3D world: indexed triangles, with the depth test and depth writes, premultiplied
/// alpha, unlit (the part's base colour times its texture, times the draw's colour).
///
/// @details Owns the pipeline of shaders/staticmesh.vert and staticmesh.frag, on the shared layout: the world matrix
/// and the colours travel as push constants, the texture at set 1. No face is culled: a glTF material is often double
/// sided, and the projection's Y flip reverses the winding. The meshes themselves live in the renderer (upload()).
/// @warning Destroy it before the Device.
class AYLO_RENDER_VULKAN_API StaticMeshPass {
  public:
    /// @brief Finds the descriptor set (set 1) of a texture; a null handle when it no longer exists.
    using SetFinder = std::function<vk::DescriptorSet(contract::renderer::TextureId)>;
    /// @brief One mesh to draw this frame, found in the renderer's table.
    struct Draw {
        const GpuStaticMesh* mesh = nullptr;  ///< What to draw.
        glm::mat4 world{1.0F};                ///< Where.
        glm::vec4 color{1.0F};                ///< The tint.
    };

    /// @param layout The shared layout (ShaderLayout.hpp); must outlive the pass.
    /// @param colorFormat The format of the images drawn into: the scene's.
    /// @param depthFormat The format of the depth attachment.
    /// @throws vk::SystemError If the pipeline cannot be created.
    StaticMeshPass(const core::Device& device, pipeline::ShaderCache& shaders, const pipeline::PipelineLayout& layout,
                   vk::Format colorFormat, vk::Format depthFormat, const vk::raii::PipelineCache* cache);
    ~StaticMeshPass() = default;
    StaticMeshPass(const StaticMeshPass&) = delete;
    StaticMeshPass& operator=(const StaticMeshPass&) = delete;
    StaticMeshPass(StaticMeshPass&&) = delete;
    StaticMeshPass& operator=(StaticMeshPass&&) = delete;

    /// @return @p desc uploaded into new buffers, written once (host-visible memory, like the other passes').
    /// @throws std::invalid_argument If an index or a part goes past the vertices or the indices.
    /// @throws vk::SystemError If a buffer cannot be created.
    [[nodiscard]] static GpuStaticMesh upload(const memory::Allocator& allocator,
                                              const contract::renderer::StaticMeshDesc& desc);

    /// @brief Records @p draws inside dynamic rendering with a depth attachment; the frame's set 0 (the 3D view) must
    /// be bound.
    /// @param findSet Gives the set of a part's texture.
    /// @param whiteSet The set of a part with no texture, or whose texture no longer exists: a white pixel.
    void record(const vk::raii::CommandBuffer& commandBuffer, std::span<const Draw> draws, const vk::Viewport& viewport,
                const vk::Rect2D& scissor, const SetFinder& findSet, vk::DescriptorSet whiteSet) const;

  private:
    pipeline::GraphicsPipeline _pipeline;  ///< The static mesh pipeline.
};

}  // namespace aylo::render::vulkan::rendering
