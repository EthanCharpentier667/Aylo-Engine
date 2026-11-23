/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MeshPartTextures
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/descriptor/DescriptorAllocator.hpp"
#include "render/vulkan/descriptor/DescriptorSetLayout.hpp"
#include "render/vulkan/material/GameMaterial.hpp"

namespace aylo::render::vulkan::rendering {

/// @brief Textures of a model's part, in the order of their bindings in the static mesh pipelines' set 1: base colour,
/// metallic-roughness, normal, occlusion, emissive (the shaders ABI's TEXTURE and ayloPart*Map).
inline constexpr std::uint32_t kMeshPartTextureCount = 5;

/// @brief The textures of a part, by binding; a default TextureId is drawn as the fallback texture.
using MeshPartTextureIds = std::array<contract::renderer::TextureId, kMeshPartTextureCount>;

/// @brief The set 1 of one part of a static mesh, per frame in flight: its five textures, rewritten by prepare() when
/// one changed or was destroyed, so a frame the GPU still draws is never written.
/// @warning Destroy it before its DescriptorAllocator.
class AYLO_RENDER_VULKAN_API MeshPartTextures {
  public:
    /// @throws vk::SystemError If a set cannot be allocated.
    MeshPartTextures(const core::Device& device, descriptor::DescriptorAllocator& descriptors,
                     const descriptor::DescriptorSetLayout& layout, std::size_t framesInFlight);

    /// @return Frame @p frameIndex's set, bound to @p wanted (a missing or destroyed one as @p fallback, which must
    /// exist).
    /// @warning Only while recording frame @p frameIndex, after its fence was waited for.
    [[nodiscard]] vk::DescriptorSet prepare(const core::Device& device, std::size_t frameIndex,
                                            const MeshPartTextureIds& wanted,
                                            const material::GameMaterial::TextureFinder& findTexture,
                                            contract::renderer::TextureId fallback);

  private:
    /// @brief One frame's set and what it points to.
    struct Frame {
        vk::raii::DescriptorSet set;  ///< Set 1 of the static mesh layout.
        MeshPartTextureIds bound{};   ///< The textures its bindings point to; default: not written yet.
    };
    std::vector<Frame> _frames;  ///< One per frame in flight.
};

}  // namespace aylo::render::vulkan::rendering
