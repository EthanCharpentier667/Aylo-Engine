/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StaticMeshPass
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float4.hpp>
#include <optional>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "aylo/contract/renderer/drawing/StaticMesh.hpp"
#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/descriptor/DescriptorPool.hpp"
#include "render/vulkan/descriptor/DescriptorSetLayout.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Buffer.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/ShaderCache.hpp"
#include "render/vulkan/rendering/MaterialBinding.hpp"
#include "render/vulkan/rendering/MeshDrawOrder.hpp"
#include "render/vulkan/rendering/StaticMeshDrawData.hpp"

namespace aylo::render::vulkan::rendering {

/// @brief A static mesh uploaded to the GPU: its buffers and parts.
struct GpuStaticMesh {
    memory::Buffer vertices;                                ///< Its StaticMeshVertex values.
    contract::renderer::StaticMeshBounds bounds;            ///< The box its vertices fit in.
    memory::Buffer indices;                                 ///< Its indices, 32-bit.
    std::vector<contract::renderer::StaticMeshPart> parts;  ///< Its runs of indices and their materials.
};

/// @brief The format of the 3D world's depth buffer, which the static mesh pipelines test against: supported as a depth
/// attachment by every desktop GPU.
inline constexpr vk::Format kWorld3DDepthFormat = vk::Format::eD32Sfloat;

/// @brief The set of the frame's mesh draws (StaticMeshDrawData), in the static mesh pipelines' layout only.
inline constexpr std::uint32_t kMeshDrawSet = 3;
/// @brief The binding of the storage buffer of mesh draws in kMeshDrawSet.
inline constexpr std::uint32_t kMeshDrawBinding = 0;

/// @brief The push constants of a static mesh part: what the shaders ABI reads as `ayloPart`, 32 bytes. The rest of
/// the draw is in the frame's buffer of mesh draws, at index `draw`.
struct StaticMeshConstants {
    glm::vec4 baseColor{1.0F};               ///< The part's base colour.
    std::uint32_t draw{0};                   ///< The draw's index in the buffer of mesh draws.
    std::array<std::uint32_t, 3> padding{};  ///< To a multiple of 16 bytes.
};

/// @brief Draws static meshes in the 3D world: indexed triangles, with the depth test and depth writes, premultiplied
/// alpha, unlit (the part's base colour times its texture, times the draw's colour).
///
/// @details Owns the pipeline of the default mesh shader (shaders/staticmesh.vert and staticmesh.frag: the shaders
/// ABI's mesh stages with no game code) and, per frame in flight, the storage buffer of the frame's draws
/// (StaticMeshDrawData, at kMeshDrawSet) with its set, which grows with the draws and is never shrunk. upload() writes
/// the frame's draws once; record() draws them, once per picture (the scene, the preview). Each part is one indexed
/// draw whose push constants are its base colour and its draw's index. No face is culled: a glTF material is often
/// double sided, and the projection's Y flip reverses the winding. The meshes themselves live in the renderer
/// (upload()).
/// @warning Destroy it before the Device and the Allocator.
class AYLO_RENDER_VULKAN_API StaticMeshPass {
  public:
    /// @brief Finds the descriptor set (set 1) of a texture; a null handle when it no longer exists.
    using SetFinder = std::function<vk::DescriptorSet(contract::renderer::TextureId)>;
    /// @brief One mesh to draw this frame, found in the renderer's table.
    struct Draw {
        const GpuStaticMesh* mesh = nullptr;  ///< What to draw.
        glm::mat4 world{1.0F};                ///< Where.
        glm::vec4 color{1.0F};                ///< The tint.
        MaterialBinding material;             ///< Its game material for ShaderTarget::kMesh; a null pipeline: none.
        contract::renderer::ShaderInstanceValues instanceValues{};  ///< Its material's instance values.
    };

    /// @param layout The static mesh layout (ShaderLayout.hpp and kMeshDrawSet): sets 0 to 3; must outlive the pass.
    /// @param drawSetLayout The layout of kMeshDrawSet: one storage buffer; must outlive the pass.
    /// @param colorFormat The format of the images drawn into: the scene's.
    /// @param depthFormat The format of the depth attachment.
    /// @throws vk::SystemError If the pipeline or the descriptor pool cannot be created.
    StaticMeshPass(const core::Device& device, const memory::Allocator& allocator, pipeline::ShaderCache& shaders,
                   const pipeline::PipelineLayout& layout, const descriptor::DescriptorSetLayout& drawSetLayout,
                   vk::Format colorFormat, vk::Format depthFormat, const vk::raii::PipelineCache* cache,
                   std::size_t framesInFlight);
    ~StaticMeshPass() = default;
    StaticMeshPass(const StaticMeshPass&) = delete;
    StaticMeshPass& operator=(const StaticMeshPass&) = delete;
    StaticMeshPass(StaticMeshPass&&) = delete;
    StaticMeshPass& operator=(StaticMeshPass&&) = delete;

    /// @return The pipeline of a game shader's ShaderTarget::kMesh variant (a `#pragma mesh` shader): the static mesh
    /// vertices, the depth test against kWorld3DDepthFormat, premultiplied alpha, no culling; a GameShader::
    /// PipelineBuilder. A mesh reads its instance values from the buffer of mesh draws, so @p instanceValues adds
    /// nothing.
    [[nodiscard]] static pipeline::GraphicsPipeline createMaterialPipeline(
        const core::Device& device, const vk::raii::ShaderModule& vertex, const vk::raii::ShaderModule& fragment,
        bool instanceValues, const pipeline::PipelineLayout& layout, vk::Format colorFormat,
        const vk::raii::PipelineCache* cache, const contract::renderer::RenderState& state);

    /// @return @p desc uploaded into new buffers, written once (host-visible memory, like the other passes'), with its
    /// bounds.
    /// @throws std::invalid_argument If an index or a part goes past the vertices or the indices.
    /// @throws vk::SystemError If a buffer cannot be created.
    [[nodiscard]] static GpuStaticMesh upload(const memory::Allocator& allocator,
                                              const contract::renderer::StaticMeshDesc& desc);

    /// @brief Writes @p draws into frame @p frameIndex's buffer of mesh draws, grown if needed, and keeps them for
    /// record(). Once per frame, after that frame's fence was waited for.
    /// @throws vk::SystemError If a bigger buffer cannot be created.
    void upload(std::size_t frameIndex, std::span<const Draw> draws);

    /// @brief Records the draws of the last upload() of @p frameIndex inside dynamic rendering with a depth
    /// attachment; the frame's set 0 (the 3D view) must be bound. A draw with a game material is drawn with its
    /// pipeline and set 2, the others with the built-in one.
    /// @param findSet Gives the set of a part's texture.
    /// @param whiteSet The set of a part with no texture, or whose texture no longer exists: a white pixel.
    /// The opaque draws go first, then the blended ones back to front from @p viewpoint (orderMeshDraws()).
    void record(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex, const vk::Viewport& viewport,
                const vk::Rect2D& scissor, const MeshViewpoint& viewpoint, const SetFinder& findSet,
                vk::DescriptorSet whiteSet);

  private:
    /// @brief What one frame in flight draws.
    struct Frame {
        std::optional<memory::Buffer> buffer;  ///< Its StaticMeshDrawData; none until a frame draws a mesh.
        vk::raii::DescriptorSet set{nullptr};  ///< kMeshDrawSet, pointing at buffer.
        std::vector<Draw> draws;               ///< The draws of its last upload().
        std::vector<MeshDrawKey> keys;         ///< Where each draw is and whether it is blended, to order them.
    };

    const memory::Allocator* _allocator;       ///< Allocates the buffers of draws.
    const pipeline::PipelineLayout* _layout;   ///< The static mesh layout.
    pipeline::GraphicsPipeline _pipeline;      ///< The static mesh pipeline.
    descriptor::DescriptorPool _pool;          ///< The frames' sets; declared before them.
    std::vector<Frame> _frames;                ///< One per frame in flight.
    std::vector<StaticMeshDrawData> _staging;  ///< The draws of an upload(), reused.
    std::vector<std::uint32_t> _order;         ///< The order of a record(), reused.
};

}  // namespace aylo::render::vulkan::rendering
