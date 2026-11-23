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
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float4.hpp>
#include <memory>
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
#include "render/vulkan/material/GameMaterial.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Buffer.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/ShaderCache.hpp"
#include "render/vulkan/rendering/MaterialBinding.hpp"
#include "render/vulkan/rendering/MeshDrawOrder.hpp"
#include "render/vulkan/rendering/MeshPartTextures.hpp"
#include "render/vulkan/rendering/StaticMeshConstants.hpp"
#include "render/vulkan/rendering/StaticMeshDrawData.hpp"

namespace aylo::render::vulkan::rendering {

/// @brief A static mesh uploaded to the GPU: its buffers and parts.
struct GpuStaticMesh {
    memory::Buffer vertices;                                ///< Its StaticMeshVertex values.
    contract::renderer::StaticMeshBounds bounds;            ///< The box its vertices fit in.
    memory::Buffer indices;                                 ///< Its indices, 32-bit.
    std::vector<contract::renderer::StaticMeshPart> parts;  ///< Its runs of indices and their materials.
    std::optional<memory::Buffer> tangents;                 ///< Its tangents; none: the pass's defaults.
    std::optional<memory::Buffer> colors;                   ///< Its vertex colours; none: the pass's defaults.
    std::optional<memory::Buffer> uv2;                      ///< Its second texture coordinates; none: defaults.
    /// Each part's set 1 (its textures), made the first time the part is drawn.
    mutable std::vector<std::unique_ptr<MeshPartTextures>> partTextures;
};

/// @brief The format of the 3D world's depth buffer, which the static mesh pipelines test against: supported as a depth
/// attachment by every desktop GPU.
inline constexpr vk::Format kWorld3DDepthFormat = vk::Format::eD32Sfloat;

/// @brief The set of the frame's mesh draws (StaticMeshDrawData), in the static mesh pipelines' layout only.
inline constexpr std::uint32_t kMeshDrawSet = 3;
/// @brief The binding of the storage buffer of mesh draws in kMeshDrawSet.
inline constexpr std::uint32_t kMeshDrawBinding = 0;
/// @brief The binding of the storage buffer of the frame's lights (LightData.hpp) in kMeshDrawSet.
inline constexpr std::uint32_t kLightsBinding = 1;

/// @brief Draws static meshes in the 3D world: indexed triangles, with the depth test, each part with its material.
///
/// @details Owns the built-in pipelines (shaders/staticmesh.vert, staticmesh.frag and staticmesh_lit.frag: the shaders
/// ABI's mesh stages with no game code), one per lighting, culling (the part's doubleSided) and blending (its alpha
/// mode) used, built the first time; per frame in flight, the storage buffer of the frame's draws and the buffer of
/// its lights (set 3); and the default vertex data (tangents, colours, second UVs) of meshes without it. upload()
/// writes the frame's draws once; record() draws them, once per picture, the opaque ones first, then the blended ones
/// back to front. Each part is one indexed draw whose push constants are its material and its draw's index, its
/// textures in a set 1 of its own (MeshPartTextures). The meshes themselves live in the renderer (upload()).
/// @warning Destroy it before the Device and the Allocator.
class AYLO_RENDER_VULKAN_API StaticMeshPass {
  public:
    /// @brief One part of a mesh to draw this frame, found in the renderer's table.
    struct Draw {
        const GpuStaticMesh* mesh = nullptr;  ///< What to draw.
        std::uint32_t part{0};                ///< Which of its parts.
        glm::mat4 world{1.0F};                ///< Where.
        glm::vec4 color{1.0F};                ///< The tint.
        MaterialBinding material;             ///< Its game material for ShaderTarget::kMesh; a null pipeline: none.
        contract::renderer::ShaderInstanceValues instanceValues{};  ///< Its material's instance values.
    };

    /// @brief What the parts' textures are found and allocated with.
    struct PartTextureSource {
        descriptor::DescriptorAllocator* descriptors = nullptr;   ///< Allocates the parts' sets; outlives the meshes.
        const descriptor::DescriptorSetLayout* layout = nullptr;  ///< Set 1 of the static mesh layout.
        material::GameMaterial::TextureFinder findTexture;        ///< How a texture is sampled, if it exists.
        contract::renderer::TextureId fallback;                   ///< What a missing texture is drawn as: white.
        std::size_t framesInFlight{0};                            ///< Sets per part.
    };

    /// @param layout The static mesh layout (sets 0 to 3); must outlive the pass.
    /// @param drawSetLayout The layout of kMeshDrawSet: the draws and the lights; must outlive the pass.
    /// @param colorFormat The format of the images drawn into: the scene's.
    /// @param depthFormat The format of the depth attachment.
    /// @throws vk::SystemError If the descriptor pool or a buffer cannot be created.
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
    /// vertices against kWorld3DDepthFormat, drawn as @p state says (culling, blending, depth, depth bias); a
    /// GameShader::PipelineBuilder.
    [[nodiscard]] static pipeline::GraphicsPipeline createMaterialPipeline(
        const core::Device& device, const vk::raii::ShaderModule& vertex, const vk::raii::ShaderModule& fragment,
        bool instanceValues, const pipeline::PipelineLayout& layout, vk::Format colorFormat,
        const vk::raii::PipelineCache* cache, const contract::renderer::RenderState& state);

    /// @return @p desc uploaded into new buffers, written once (host-visible memory, like the other passes'), with its
    /// bounds; its tangents, colours and second UVs only if it has them.
    /// @throws std::invalid_argument If an index or a part goes past the vertices or the indices, or a list of vertex
    /// data is neither empty nor as long as the vertices.
    /// @throws vk::SystemError If a buffer cannot be created.
    [[nodiscard]] static GpuStaticMesh upload(const memory::Allocator& allocator,
                                              const contract::renderer::StaticMeshDesc& desc);

    /// @brief Writes @p draws into frame @p frameIndex's buffer of mesh draws, grown if needed, and keeps them for
    /// record(). Once per frame, after that frame's fence was waited for.
    /// @throws vk::SystemError If a bigger buffer cannot be created.
    void upload(std::size_t frameIndex, std::span<const Draw> draws);

    /// @brief Writes the packed lights @p bytes (packLights()) into frame @p frameIndex's buffer of lights, grown if
    /// needed. Once per frame, after that frame's fence was waited for.
    /// @param lit Whether the frame has lights: models without a material are then drawn lit (staticmesh_lit.frag).
    /// @throws vk::SystemError If a bigger buffer cannot be created.
    void uploadLights(std::size_t frameIndex, std::span<const std::byte> bytes, bool lit);

    /// @brief Records the draws of the last upload() of @p frameIndex inside dynamic rendering with a depth
    /// attachment; the frame's set 0 (the 3D view) must be bound. A draw with a game material is drawn with its
    /// pipeline and set 2, the others with the built-in one of their part. The opaque draws go first, then the blended
    /// ones back to front from @p viewpoint (orderMeshDraws()).
    void record(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex, const vk::Viewport& viewport,
                const vk::Rect2D& scissor, const MeshViewpoint& viewpoint, const PartTextureSource& textures);

  private:
    /// @brief What one frame in flight draws.
    struct Frame {
        std::optional<memory::Buffer> buffer;  ///< Its StaticMeshDrawData; none until a frame draws a mesh.
        std::optional<memory::Buffer> lights;  ///< Its lights (LightData.hpp), made with the pass.
        bool lit{false};                       ///< Whether it has lights: the built-in material is lit.
        vk::raii::DescriptorSet set{nullptr};  ///< kMeshDrawSet, pointing at buffer.
        std::vector<Draw> draws;               ///< The draws of its last upload().
        std::vector<MeshDrawKey> keys;         ///< Where each draw is and whether it is blended, to order them.
    };

    /// @brief Default vertex data, for meshes without their own: one buffer of the tangents, then the colours, then the
    /// second UVs of `capacity` vertices. Grown by replacing it; the old ones stay until the pass goes, as frames in
    /// flight may still read them.
    struct Defaults {
        std::optional<memory::Buffer> buffer;  ///< The current one.
        std::uint32_t capacity{0};             ///< Vertices it covers.
        std::vector<memory::Buffer> retired;   ///< The smaller ones before it.
    };

    /// @return The built-in pipeline for a part, @p lit or not, culling its back faces unless @p doubleSided, blended
    /// if @p blended; built the first time.
    const pipeline::GraphicsPipeline& builtInPipeline(bool lit, bool doubleSided, bool blended);

    /// @brief Makes the default vertex data cover @p vertices.
    void ensureDefaults(std::uint32_t vertices);

    /// @brief Binds @p mesh's vertex and index buffers, its missing data from the defaults.
    void bindMesh(const vk::raii::CommandBuffer& commandBuffer, const GpuStaticMesh& mesh);

    const core::Device* _device;              ///< Builds the built-in pipelines.
    const memory::Allocator* _allocator;      ///< Allocates the buffers of draws.
    pipeline::ShaderCache* _shaders;          ///< The built-in shaders.
    const pipeline::PipelineLayout* _layout;  ///< The static mesh layout.
    vk::Format _colorFormat;                  ///< What the pipelines draw into.
    vk::Format _depthFormat;                  ///< What they test against.
    const vk::raii::PipelineCache* _cache;    ///< Speeds up building them.
    /// The built-in pipelines, by lit, doubleSided and blended (builtInPipeline()).
    std::array<std::optional<pipeline::GraphicsPipeline>, 8> _builtIn;
    descriptor::DescriptorPool _pool;          ///< The frames' sets; declared before them.
    std::vector<Frame> _frames;                ///< One per frame in flight.
    Defaults _defaults;                        ///< Vertex data of meshes without their own.
    std::vector<StaticMeshDrawData> _staging;  ///< The draws of an upload(), reused.
    std::vector<std::uint32_t> _order;         ///< The order of a record(), reused.
};

}  // namespace aylo::render::vulkan::rendering
