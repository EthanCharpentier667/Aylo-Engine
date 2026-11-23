/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** GameMaterial
*/

#include "GameMaterial.hpp"

#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <span>
#include <vulkan/vulkan_raii.hpp>

#include "aylo/contract/renderer/drawing/Texture.hpp"
#include "aylo/contract/renderer/shaders/CompiledShader.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/descriptor/DescriptorAllocator.hpp"
#include "render/vulkan/descriptor/DescriptorSetLayout.hpp"
#include "render/vulkan/descriptor/DescriptorWriter.hpp"
#include "render/vulkan/memory/Allocator.hpp"
#include "render/vulkan/memory/Buffer.hpp"

namespace aylo::render::vulkan::material {

GameMaterial::GameMaterial(const memory::Allocator& allocator, descriptor::DescriptorAllocator& descriptors,
                           const descriptor::DescriptorSetLayout& layout, std::uint32_t materialSize,
                           const contract::renderer::MaterialDesc& desc, std::size_t framesInFlight)
    : _shader{desc.shader}, _values{desc.values}, _textures{desc.textures}, _renderState{desc.renderState} {
    _frames.reserve(framesInFlight);
    for (std::size_t frame = 0; frame < framesInFlight; ++frame) {
        Frame& created = _frames.emplace_back(Frame{.set = descriptors.allocate(layout)});
        allocator.getDevice().setName(*created.set, std::format("Material set, frame {}", frame));
        if (materialSize > 0) {
            created.values.emplace(allocator, materialSize, vk::BufferUsageFlagBits::eUniformBuffer,
                                   memory::Allocator::MemoryAccess::kStaging);
            allocator.getDevice().setName(*created.values->getBuffer(),
                                          std::format("Material values, frame {}", frame));
        }
    }
}

void GameMaterial::update(const contract::renderer::MaterialDesc& desc) {
    _values = desc.values;
    _textures = desc.textures;
    _renderState = desc.renderState;
    ++_version;
}

void GameMaterial::writeCopy(const core::Device& device, std::size_t frameIndex, vk::DescriptorSet set,
                             std::span<const std::optional<TextureBinding>> overrides, const TextureFinder& findTexture,
                             contract::renderer::TextureId fallback) const {
    const Frame& frame = _frames.at(frameIndex);
    descriptor::DescriptorWriter writer;
    if (frame.values) {
        writer.writeBuffer(kMaterialValuesBinding, *frame.values->getBuffer(), 0, frame.values->getSize());
    }
    for (std::size_t slot = 0; slot < _textures.size(); ++slot) {
        std::optional<TextureBinding> binding =
            slot < overrides.size() ? overrides.subspan(slot, 1).front() : std::nullopt;
        if (!binding && _textures.at(slot) != contract::renderer::TextureId{}) {
            binding = findTexture(_textures.at(slot));
        }
        if (!binding) {
            binding = findTexture(fallback);
        }
        if (binding) {
            writer.writeImage(kMaterialFirstTextureBinding + static_cast<std::uint32_t>(slot), binding->view,
                              binding->sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
        }
    }
    writer.update(device, set);
}

vk::DescriptorSet GameMaterial::prepare(const core::Device& device, std::size_t frameIndex,
                                        const TextureFinder& findTexture, contract::renderer::TextureId fallback) {
    Frame& frame = _frames.at(frameIndex);
    descriptor::DescriptorWriter writer;
    bool written = false;
    if (frame.version != _version) {
        if (frame.values) {
            frame.values->write(_values);
            if (frame.version == 0) {
                writer.writeBuffer(kMaterialValuesBinding, *frame.values->getBuffer(), 0, frame.values->getSize());
                written = true;
            }
        }
        frame.version = _version;
    }
    for (std::size_t slot = 0; slot < _textures.size(); ++slot) {
        contract::renderer::TextureId texture = _textures.at(slot);
        std::optional<TextureBinding> binding =
            texture == contract::renderer::TextureId{} ? std::nullopt : findTexture(texture);
        if (!binding) {
            texture = fallback;
            binding = findTexture(fallback);
        }
        if (binding && frame.bound.at(slot) != texture) {
            writer.writeImage(kMaterialFirstTextureBinding + static_cast<std::uint32_t>(slot), binding->view,
                              binding->sampler, vk::ImageLayout::eShaderReadOnlyOptimal);
            frame.bound.at(slot) = texture;
            written = true;
        }
    }
    if (written) {
        writer.update(device, *frame.set);
    }
    return *frame.set;
}

}  // namespace aylo::render::vulkan::material
