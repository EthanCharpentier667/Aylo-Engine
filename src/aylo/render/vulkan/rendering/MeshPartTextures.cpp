/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MeshPartTextures
*/

#include "MeshPartTextures.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vulkan/vulkan_raii.hpp>

#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/descriptor/DescriptorAllocator.hpp"
#include "render/vulkan/descriptor/DescriptorSetLayout.hpp"
#include "render/vulkan/descriptor/DescriptorWriter.hpp"
#include "render/vulkan/material/GameMaterial.hpp"

namespace aylo::render::vulkan::rendering {

MeshPartTextures::MeshPartTextures(const core::Device& device, descriptor::DescriptorAllocator& descriptors,
                                   const descriptor::DescriptorSetLayout& layout, std::size_t framesInFlight) {
    _frames.reserve(framesInFlight);
    for (std::size_t frame = 0; frame < framesInFlight; ++frame) {
        const Frame& made = _frames.emplace_back(Frame{.set = descriptors.allocate(layout)});
        device.setName(*made.set, "Static mesh part textures");
    }
}

vk::DescriptorSet MeshPartTextures::prepare(const core::Device& device, std::size_t frameIndex,
                                            const MeshPartTextureIds& wanted,
                                            const material::GameMaterial::TextureFinder& findTexture,
                                            contract::renderer::TextureId fallback) {
    Frame& frame = _frames.at(frameIndex);
    descriptor::DescriptorWriter writer;
    bool written = false;
    for (std::size_t slot = 0; slot < wanted.size(); ++slot) {
        contract::renderer::TextureId texture = wanted.at(slot);
        std::optional<material::GameMaterial::TextureBinding> binding =
            texture == contract::renderer::TextureId{} ? std::nullopt : findTexture(texture);
        if (!binding) {
            texture = fallback;
            binding = findTexture(fallback);
        }
        if (binding && frame.bound.at(slot) != texture) {
            writer.writeImage(static_cast<std::uint32_t>(slot), binding->view, binding->sampler,
                              vk::ImageLayout::eShaderReadOnlyOptimal);
            frame.bound.at(slot) = texture;
            written = true;
        }
    }
    if (written) {
        writer.update(device, *frame.set);
    }
    return *frame.set;
}

}  // namespace aylo::render::vulkan::rendering
