/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EnvironmentMaps
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <memory>
#include <optional>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "aylo/contract/renderer/drawing/Light3D.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/descriptor/DescriptorAllocator.hpp"
#include "render/vulkan/descriptor/DescriptorSetLayout.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Image.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/PipelineLayout.hpp"
#include "render/vulkan/pipeline/ShaderCache.hpp"

namespace aylo::render::vulkan::rendering {

/// @brief Size of the environment's first level, the sky the others are made from: equirectangular, 2:1.
inline constexpr vk::Extent2D kEnvironmentExtent{1024, 512};
/// @brief Size of the sky's reflection's first level (roughness 0); each level halves it, for a roughness more.
inline constexpr vk::Extent2D kSkySpecularExtent{512, 256};
/// @brief Levels of the sky's reflection: roughness 0, 1/6... 1.
inline constexpr std::uint32_t kSkySpecularLevels = 7;
/// @brief Size of the sky's irradiance: it varies slowly with the direction.
inline constexpr vk::Extent2D kSkyDiffuseExtent{64, 32};

/// @brief The sky of the 3D world (contract::renderer::Environment3D) and its light: the environment, an
/// equirectangular image of the sky with its mips; its reflection prefiltered for each roughness (GGX, split sum, Karis
/// 2013), a level a roughness; its irradiance. The lit meshes read the two last (set 3, bindings 4 and 5), the sky pass
/// the first.
///
/// @details Nothing is allocated until a sky is set: a game without one pays nothing. The maps are made again, on the
/// GPU, in the frame where the sky changes (its kind, image, colours or rotation; not its intensity), before the 3D
/// pass; they stay as they are otherwise. Barriers order them after the frames that still read the last ones.
class AYLO_RENDER_VULKAN_API EnvironmentMaps {
  public:
    /// @param sceneFormat, depthFormat The 3D pass's attachments, which the sky is drawn into.
    /// @param framesInFlight The frames recorded at once: each has its own descriptor set for the image it reads.
    EnvironmentMaps(const core::Device& device, const memory::Allocator& allocator, pipeline::ShaderCache& shaders,
                    vk::Format sceneFormat, vk::Format depthFormat, const vk::raii::PipelineCache* cache,
                    std::size_t framesInFlight);
    ~EnvironmentMaps() = default;
    EnvironmentMaps(const EnvironmentMaps&) = delete;
    EnvironmentMaps& operator=(const EnvironmentMaps&) = delete;
    EnvironmentMaps(EnvironmentMaps&&) = delete;
    EnvironmentMaps& operator=(EnvironmentMaps&&) = delete;

    /// @brief Records, outside a rendering scope, the making of the maps if @p environment's sky changed since the last
    /// call.
    /// @param skyTexture The view of the sky's image for SkyKind::kTexture, null if it is not loaded (no sky then);
    /// for SkyKind::kGradient, any image ready to be sampled, which is not read.
    /// @param sampler Reads @p skyTexture.
    void update(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex,
                const contract::renderer::Environment3D& environment, vk::ImageView skyTexture, vk::Sampler sampler);

    /// @return Whether a sky is drawn: one is set and its maps are made.
    [[nodiscard]] bool hasSky() const noexcept { return _shown; }

    /// @brief Records the sky over the whole of the 3D pass's attachment, inside its rendering scope, first.
    /// @param inverseViewProjection The 3D camera's clip space to the world.
    void recordSky(const vk::raii::CommandBuffer& commandBuffer, const glm::mat4& inverseViewProjection,
                   float intensity) const;

    /// @return The sky's reflection (kSkySpecularLevels mips), or null before a sky was ever set.
    [[nodiscard]] vk::ImageView getSpecular() const noexcept;
    /// @return The sky's irradiance, or null before a sky was ever set.
    [[nodiscard]] vk::ImageView getDiffuse() const noexcept;
    /// @return Reads the maps: linear, between mips too, wrapping around the horizon.
    [[nodiscard]] vk::Sampler getSampler() const noexcept { return *_sampler; }
    /// @return A number that changes each time the maps are made, for the descriptors that point at them.
    [[nodiscard]] std::uint64_t getVersion() const noexcept { return _version; }

  private:
    /// @brief The images, made at the first sky.
    struct Maps {
        memory::Image environment;                        ///< The sky, with its mips.
        memory::Image specular;                           ///< Its reflection, a level a roughness.
        memory::Image diffuse;                            ///< Its irradiance.
        vk::raii::ImageView environmentTop;               ///< The environment's first level, drawn into.
        std::vector<vk::raii::ImageView> specularLevels;  ///< Each level of the reflection, drawn into.
        vk::raii::DescriptorSet environmentSet;           ///< Samples the environment, all its mips.
    };

    /// @return The maps, made the first time.
    Maps& createMaps();
    /// @brief Records the making of @p maps for @p environment's sky, made from @p skyTexture.
    void recordMaps(const vk::raii::CommandBuffer& commandBuffer, std::size_t frameIndex, const Maps& maps,
                    const contract::renderer::Environment3D& environment, vk::ImageView skyTexture,
                    vk::Sampler sampler) const;
    /// @brief Records the environment's mips of @p maps, halving its first level.
    static void recordMips(const vk::raii::CommandBuffer& commandBuffer, const Maps& maps);

    const core::Device* _device;
    const memory::Allocator* _allocator;
    vk::raii::Sampler _sampler;
    std::unique_ptr<descriptor::DescriptorSetLayout> _setLayout;  ///< One combined image sampler.
    std::unique_ptr<descriptor::DescriptorAllocator> _descriptors;
    std::unique_ptr<pipeline::PipelineLayout> _passLayout;   ///< The making passes': the set, EnvironmentPassConstants.
    std::unique_ptr<pipeline::PipelineLayout> _skyLayout;    ///< The sky's: the set, SkyConstants.
    pipeline::GraphicsPipeline _source;                      ///< The environment's first level.
    pipeline::GraphicsPipeline _prefilter;                   ///< A level of the reflection.
    pipeline::GraphicsPipeline _irradiance;                  ///< The irradiance.
    pipeline::GraphicsPipeline _sky;                         ///< The sky, in the 3D pass.
    std::vector<vk::raii::DescriptorSet> _sourceSets;        ///< Per frame: samples the game's image.
    std::optional<Maps> _maps;                               ///< Once a sky was set.
    std::optional<contract::renderer::Environment3D> _made;  ///< The sky the maps were made for.
    vk::ImageView _madeFrom;                                 ///< The image they were made from.
    bool _shown{false};                                      ///< Whether a sky is set and made.
    std::uint64_t _version{0};                               ///< Bumped when the maps are made.
};

}  // namespace aylo::render::vulkan::rendering
